/***************************************************************************//**
 * @file
 * @brief Top level application functions
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
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
#include "sl_i2cspm.h"
#include "sl_i2cspm_instances.h"
#include "sl_si70xx.h"
#include "sl_sleeptimer.h"

#define TIMEOUT_MS         5000     // Periodic timer duration in ms
#define NEGATIVE_CASE      0        // Used for negative temperature readings
#define THREE_DIGIT_CASE   100000   // Used for three-digit temperature readings

uint32_t rh_data = 0;               // Relative humidity data
int32_t temp_data = 0;              // Temperature data

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
 * multiplied by a factor of 1000. In order to display the temperature correctly
 * in the negative and three digit cases, the temp_data variable must be divided
 * by 10 to remove one digit from the variable. Without this, the value of
 * temp_data passed to the SegmentLCD_Number() function may be higher than the
 * max value the Segment LCD could display.
 ******************************************************************************/
// Used when the temperature is below 0 degrees Celsius
  if (temp_data < NEGATIVE_CASE) {
    temp_data = temp_data / 10;
    SegmentLCD_Number(temp_data);           // Display the value of temp_data
    SegmentLCD_Symbol(LCD_SYMBOL_DEGC, 1);  // Display Degree C symbol
    SegmentLCD_Symbol(LCD_SYMBOL_P3, 1);    // Display decimal symbol
  }

// Used when the temperature is above 100 degrees Celsius
  if (temp_data >= THREE_DIGIT_CASE) {
    temp_data = temp_data / 10;
    SegmentLCD_Number(temp_data);           // Display the value of temp_data
    SegmentLCD_Symbol(LCD_SYMBOL_DEGC, 1);  // Display Degree C symbol
    SegmentLCD_Symbol(LCD_SYMBOL_P3, 1);    // Display decimal symbol
  }
// Used when the temperature is between 0-100 degrees Celsius
  else {
    SegmentLCD_Number(temp_data);           // Display the value of temp_data
    SegmentLCD_Symbol(LCD_SYMBOL_DEGC, 1);  // Display Degree C symbol
    SegmentLCD_Symbol(LCD_SYMBOL_P2, 1);    // Display decimal symbol
  }
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
  LCD_SegmentEnable(9, false);
  LCD_SegmentEnable(11, false);
  LCD_SegmentEnable(12, false);
  LCD_SegmentEnable(13, false);
  LCD_SegmentEnable(14, false);
  LCD_SegmentEnable(15, false);
  LCD_SegmentEnable(16, false);
  LCD_SegmentEnable(17, false);
  LCD_SegmentEnable(18, false);
  LCD_SegmentEnable(19, false);
}

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  // Initialize the Si7021 sensor, sleeptimer, and Segment LCD display
  sl_si70xx_init(sl_i2cspm_sensor, SI7021_ADDR);
  sl_sleeptimer_init();

  // Configure LCD to use step down mode and disable unused segments
  // Default display value 0
  SegmentLCD_Init(false);
  LCD->BIASCTRL_SET = LCD_BIASCTRL_VDDXSEL_AVDD;
  disableUnusedLCDSeg();

  // Display all 0's upon initialization
  SegmentLCD_Number(0);
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
