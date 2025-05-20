/***************************************************************************//**
 * @file app.c
 * @brief Demonstrates polled calibration of the HFRCO against the LFXO
 * with output to a pin.
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
*******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 *******************************************************************************
 * # Evaluation Quality
 * This code has been minimally tested to ensure that it builds and is suitable
 * as a demonstration for evaluation purposes only. This code will be maintained
 * at the sole discretion of Silicon Labs.
 ******************************************************************************/

#include <stddef.h>

#include "em_device.h"
#include "em_cmu.h"
#include "em_gpio.h"

#include "sl_clock_manager.h"
#include "sl_sleeptimer.h"

/*
 * Top value for the calibration down counter.  Maximum allowed is
 * 2^20-1 = 0xFFFFF, which equates to 2^20 counts.  A larger value
 * translates into greater accuracy at the expense of a longer
 * calibration period
 *
 * In this example, calibration is run for DOWNCOUNT counts of the
 * HFRCODPLL.  At the same time, the LFXO clocks the up counter.  At a
 * given frequency, DOWNCOUNT counts of the HFRCO to take a certain
 * amount of time:
 *
 *    DOWNCOUNT
 * --------------- = calibration time
 * HFRCO frequency
 *
 * It follows that the up counter will run for some number of counts
 * of the LFXO during the same period of time:
 *
 *       UP
 * -------------- = calibration time
 * LFXO frequency
 *
 * So, to calibrate for a specified HFRCODPLL frequency...
 *
 *      LFXO frequency * DOWNCOUNT
 * UP = --------------------------
 *          HFRCODPLL frequency
 *
 * For example, if the desired HFRCODPLL frequency is 19 MHz, then the up
 * counter should reach...
 *
 * 32768 x 1048576
 * --------------- = 1808 (rounded down from 1808.4)
 *    19000000
 *
 * ...when clocked from a 32.768 kHz crystal connected to the LFXO.
 *
 * This can be run for any desired frequency to the extent that it is
 * within the range of the selected HFRCODPLL tuning band.  There is
 * overlap among tuning bands, so it might be necessary to include logic
 * that selects the one in which the target frequency is centered best.
 */
#define DOWNCOUNT   0xFFFFF

// Sleeptimer handle and status
sl_sleeptimer_timer_handle_t delay_sleeptimer_handle;
sl_status_t delay_sleeptimer_status;

/*
 * Before calling this function make sure the HFRCODPLL is set to the
 * tuning band that best centers the target calibration frequency.
 *
 * Calibration is an iterative process because the CMU hardware simply
 * returns a count.  It doesn't determine whether or not a specific
 * tuningVal is correct.  To keep things simple, this function implements
 * the search algorithm and not any of the setup that might differ from
 * system to system.
 */
void cal_hfrcodpll(uint32_t freq)
{
  bool tuned, lastUpGT, lastUpLT;
  uint32_t tuningVal, idealCount, upCount, prevUp, prevTuning;

  // Get current tuning value
  sl_clock_manager_get_rc_oscillator_calibration(SL_OSCILLATOR_HFRCODPLL, &tuningVal);

  /*
   * Determine the ideal up counter value based on the desired
   * frequency.  Note that this is done to maintain accuracy throughout
   * the calculation.
   *
   * In most applications, it is probably better to calculate the ideal
   * up counter value(s) in advance.  For example, the ideal counter
   * value at the default 19 MHz HFRCODPLL frequency is (when calibrating
   * against the LFXO and using the maximum down counter value of
   * 0xFFFFF):
   *
   * 32768 x 1048576
   * --------------- = 1808 (rounded down from 1808.4)
   *    19000000
   */
  idealCount =
    (uint32_t)(((float)SystemLFXOClockGet() / (float)freq)
               * (float)(DOWNCOUNT + 1));

  /*
   * Haven't run a calibration cycle yet so not tuned, and the last up
   * count greater than and less than flags are ALSO false.
   */
  tuned = false;
  lastUpGT = false;
  lastUpLT = false;

  // Setup the calibration circuit
  sl_clock_manager_configure_rco_calibration(DOWNCOUNT,
                                             SL_CLOCK_MANAGER_CLOCK_CALIBRATION_HFRCODPLL,
                                             SL_CLOCK_MANAGER_CLOCK_CALIBRATION_LFXO,
                                             false);

  while (!tuned) {
    // Start the up counter
    sl_clock_manager_start_rco_calibration();

    // Wait for down counter to finish
    sl_clock_manager_wait_rco_calibration();

    // Get the up counter value
    sl_clock_manager_get_rco_calibration_count(&upCount);

    /*
     * If the up counter result is less than the tuned value, the LFRCO
     * is running at a lower frequency, so the tuning value has to be
     * incremented.
     */
    if (upCount < idealCount) {
      // Was the up counter greater than the tuned value on the last run?
      if (lastUpGT) {
        /*
         * If the difference between the ideal count and the up count
         * from this run is greater than it was on the last run, then
         * the last run produced a more accurate tuning, so revert to
         * the previous tuning value.  If not, the current value gets
         * us the most accurate tuning.
         */
        if ((idealCount - upCount) > (prevUp - idealCount)) {
          tuningVal = prevTuning;
        }
        // Done tuning now
        tuned = true;
        // Up counter for the last run not greater than the tuned value
      } else {
        /*
         * If the difference is 1, incrementing the tuning value again
         * will only increase the frequency further away from the
         * intended target, so tuning is now complete.
         */
        if ((idealCount - upCount) == 1) {
          tuned = true;

          /*
           * The difference between this run and the ideal count for the
           * desired frequency is > 1, so increase the tuning value to
           * increase the HFRCODPLL frequency.  After the next calibration run,
           * the up counter value will decrease.  Save the tuning value
           * from this run; if it's close, it might be more accurate than
           * the result from the next run.
           */
        } else {
          prevTuning = tuningVal;
          tuningVal++;
          lastUpLT = true;  // Remember that the up counter was less than the ideal this run
          prevUp = upCount;
        }
      }
    }

    /*
     * If the up counter result is greater than the tuned value, the
     * HFRCODPLL is running at a higher frequency, so the tuning value has
     * to be decremented.
     */
    if (upCount > idealCount) {
      // Was the up counter less than the tuned value on the last run?
      if (lastUpLT) {
        /*
         * If the difference between the up count and the ideal count
         * from this run is greater than it was on the last run, then
         * the last run produced a more accurate tuning, so revert to
         * the previous tuning value.  If not, the current value gets
         * the most accurate tuning.
         */
        if ((upCount - idealCount) > (idealCount - prevUp)) {
          tuningVal = prevTuning;
        }
        // Done tuning now
        tuned = true;
        // Up counter for the last run not less than the tuned value
      } else {
        /*
         * If the difference is 1, decrementing the tuning value again
         * will only decrease the frequency further away from the
         * intended target, so tuning is now complete.
         */
        if ((upCount - idealCount) == 1) {
          tuned = true;

          /*
           * The difference between this run and the ideal count for the
           * desired frequency is > 1, so decrease the tuning value to
           * decrease the HFRCODPLL frequency.  After the next calibration run,
           * the up counter value will increase.  Save the tuning value
           * from this run; if it's close, it might be more accurate than
           * the result from the next run.
           */
        } else {
          prevTuning = tuningVal;
          tuningVal--;
          lastUpGT = true;  // Remember that the up counter was greater than the ideal this run
          prevUp = upCount;
        }
      }
    }
    // Up counter result is equal to the desired value, end of calibration
    if (upCount == idealCount) {
      tuned = true;
      // Otherwise set new tuning value
    } else {
      sl_clock_manager_set_rc_oscillator_calibration(SL_OSCILLATOR_HFRCODPLL, tuningVal);
    }
  }
}

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  // Drive HFRCODPLL onto PB1 via CMU_CLKOUT2 to observe calibration
  sl_clock_manager_enable_bus_clock(SL_BUS_CLOCK_GPIO);
  GPIO_PinModeSet(gpioPortB, 1, gpioModePushPull, 0);
  sl_clock_manager_set_gpio_clock_output(SL_CLOCK_MANAGER_EXPORT_CLOCK_SOURCE_HFRCODPLL,
                                         SL_CLOCK_MANAGER_EXPORT_CLOCK_OUTPUT_SELECT_2,
                                         1,
                                         gpioPortB,
                                         1);
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
  // Run calibration
  cal_hfrcodpll(19000000);

  // Delay 16 seconds before the next calibration run
  delay_sleeptimer_status =
      sl_sleeptimer_start_timer(&delay_sleeptimer_handle,
                                sl_sleeptimer_ms_to_tick(16000),
                                (void *)NULL,
                                (void *)NULL,
                                0,
                                0);

  // Stop here if an error occurs
  if (delay_sleeptimer_status != SL_STATUS_OK)
    __BKPT(0);
}
