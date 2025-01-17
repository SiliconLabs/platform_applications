/**************************************************************************//**
 * @file sensorless_motor.c
 * @brief Control functions specific to sensorless commutation
 * @author Silicon Labs
 * @version x.xx (leave as is with x.xx, Correct version is automatically inserted by auto-generation)
 ******************************************************************************
 * @section License
 * <b>(C) Copyright 2014 Silicon Labs, http://www.silabs.com</b>
 *******************************************************************************
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Labs has no
 * obligation to support this Software. Silicon Labs is providing the
 * Software "AS IS", with no express or implied warranties of any kind,
 * including, but not limited to, any implied warranties of merchantability
 * or fitness for any particular purpose or warranties against infringement
 * of any proprietary rights of a third party.
 *
 * Silicon Labs will not be liable for any consequential, incidental, or
 * special damages, or any other relief, or for any claim by any third party,
 * arising from your use of this Software.
 *
 ******************************************************************************/
#include <math.h>
#include <stdbool.h>
#include "em_device.h"
#include "em_assert.h"
#include "config.h"
#include "acmp.h"
#include "logging.h"
#include "pwm.h"
#include "sensorless_motor.h"
#include "em_timer.h"

/* The current elecrical period (TIMER1 counts per 6th commutation) */
extern int currentPeriod;

/* The current commutation state */
extern int pwmCurState;

/**********************************************************
 * Start the motor with sensorless (back-emf measurement)
 * commutation.
 *********************************************************/
void sensorlessStartMotor(void)
{
  acmpInit();
  sensorlessStartup();
  acmpSetInput(pwmCurState);
}

/**********************************************************
 * Perform the startup sequence for a sensorless motor.
 * During startup we have no information about the
 * speed or position of the motor. This function starts
 * slow and ramps up the commutation speed linearly
 * until it reaches the configured final speed.
 * At this point, back-emf triggered commutation
 * can start.
 *********************************************************/
void sensorlessStartup(void)
{
  int i;
  int startupDelays[3];
  int prescalerValue;

  prescalerValue = PRESCALER_TIMER1;

  /* Precalculate the 3 first initial delays */
  startupDelays[0] =
    (int)(STARTUP_INITIAL_PERIOD_MS * (CORE_FREQUENCY / 1000)
          / (float)(prescalerValue));
  startupDelays[1] = (int)(0.5 * startupDelays[0] * 1.236f);
  startupDelays[2] =
    (int)(0.5 * (startupDelays[0] + startupDelays[1])
          * (sqrt(1 + (4 * startupDelays[1])
                  / (float)(startupDelays[0] + startupDelays[1])) - 1));

  /* Precalculate the initial sum of the delays */
  float sum = 0;
  for (i = 0; i < 3; i++) {
    sum += startupDelays[0];
  }

  /* Calculate the minimum top value corresponding to the final speed */
  int minTop = (10 * (CORE_FREQUENCY / prescalerValue))
               / (STARTUP_FINAL_SPEED_RPM_SENSORLESS * MOTOR_POLE_PAIRS);

  /* Reset counter before starting the motor */
  TIMER1->CNT = 0;

  i = 0;
  float top = startupDelays[0];
  int period = 0;

  while (top > minTop) {
    /* Go to the next commutation state */
    pwmNextState();

    if (i < 3) {
      /* Use hardcoded values for the first three commutations */
      top = startupDelays[i];
    } else {
      /* Calculate the next term and update sum */
      top = top * sum / (sum + top);
      sum += top;
    }

    /* Calculate speed */
    if ((i > 0) && ((i % 6) == 0)) {
#if defined BLDC_LOGGING_ENABLED
      int16_t speed = (60 * (CORE_FREQUENCY / prescalerValue))
                      / (period * MOTOR_POLE_PAIRS);
#endif
      currentPeriod = period;
      period = 0;

      LOG_SET_SPEED(speed);
    }
    period += top;

    /* Delay until next commutation */
    while (TIMER1->CNT < (uint32_t)top) {}
    TIMER1->CNT = 0;

    i++;
  }
}
