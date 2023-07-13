/***************************************************************************//**
 * @file
 * @brief Top level application functions
 *******************************************************************************
 * # License
 * <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
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
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/

#include "sl_segmentlcd.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "sl_i2cspm.h"
#include "sl_i2cspm_instances.h"
#include "sl_si70xx.h"
#include "sl_sleeptimer.h"

#define TIMEOUT_MS         5000     // Periodic timer duration in ms

uint32_t rh_data = 0;               // Relative humidity data
int32_t  temp_data = 0;             // Temperature data

static sl_sleeptimer_timer_handle_t periodic_timer;

/***************************************************************************//**
 * Periodic timer callback function
 ******************************************************************************/
void on_periodic_timeout(sl_sleeptimer_timer_handle_t *handle,
                         void *data)
{
  // This prevents unused parameter warnings
  (void)&handle;
  (void)&data;

  // Measure the values for relative humidity and temperature
  sl_si70xx_measure_rh_and_temp(sl_i2cspm_sensor,
                                SI7021_ADDR,
                                &rh_data,
                                &temp_data);

  // Read the values for relative humidity and temperature
  sl_si70xx_read_rh_and_temp(sl_i2cspm_sensor,
                             SI7021_ADDR,
                             &rh_data,
                             &temp_data);

/***************************************************************************//**
 * The following statements are written to handle the cases when the sensor
 * measures negative temperature values, three digit temperature values, and
 * temperature values from 0-100 degrees Celsius. Keep in mind, the Si7021 part
 * has an operating temperature range of -40 to +125 degrees Celsius.
 * The temp_data variable stores a temperature read by the Si7021 that is
 * multiplied by a factor of 1000. Using SegmentLCD_LowerNumber(), the lower
 * segments on the display are utilized. SegmentLCD_LowerNumber() converts
 * signed value to text and will print alpha-numeric minus symbol when
 * measurement is negative. Turn on decimal symbol at appropriate
 * location.
 ******************************************************************************/
  SegmentLCD_LowerNumber(temp_data);      // Display the value of temp_data
  SegmentLCD_Symbol(LCD_SYMBOL_DEGC, 1);  // Display Degree C symbol
  SegmentLCD_Symbol(LCD_SYMBOL_DP5, 1);   // Display decimal symbol
}

/***************************************************************************//**
* Disable Unused LCD Segments
*******************************************************************************/
void disableUnusedLCDSeg(void)
{
/***************************************************************************//**
* The LCD driver enables all segments, even those that are not mapped to
* segments on the pro kit board. These are disabled below in order to
* minimize current consumption.
*******************************************************************************/
  LCD_SegmentEnable(SEG_S15, false);
  LCD_SegmentEnable(SEG_S16, false);
  LCD_SegmentEnable(SEG_S17, false);
  LCD_SegmentEnable(SEG_S18, false);
  LCD_SegmentEnable(SEG_S19, false);
}

/***************************************************************************//**
 * Initialize GPIO
 ******************************************************************************/
void initGPIO(void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Configure PA12 (SENSOR_ENABLE) as output with filter enabled
  GPIO_PinModeSet(gpioPortA, 12, gpioModePushPull, 1);
}

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  // Enable sensor external MUX connections
  initGPIO();

  // Initialize the Si7021 sensor, sleeptimer, and Segment LCD display
  sl_si70xx_init(sl_i2cspm_sensor, SI7021_ADDR);
  sl_sleeptimer_init();

  // Configure LCD to use step down mode and disable unused segments
  // Default display value 0
  SegmentLCD_Init(false);
  LCD->BIASCTRL_SET = LCD_BIASCTRL_VDDXSEL_AVDD;
  disableUnusedLCDSeg();

  // Display 25 degC upon initialization
  SegmentLCD_LowerNumber(25000);
  SegmentLCD_Symbol(LCD_SYMBOL_DEGC, 1);  // Display Degree C symbol
  SegmentLCD_Symbol(LCD_SYMBOL_DP5, 1);   // Display decimal symbol
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
  // Periodic timer for updating the temperature value displayed
  sl_sleeptimer_start_periodic_timer_ms(&periodic_timer,
                                        TIMEOUT_MS,
                                        on_periodic_timeout,
                                        NULL,
                                        0,
                                        0);
}
