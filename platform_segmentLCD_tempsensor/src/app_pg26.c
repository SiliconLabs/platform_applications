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
#include "sl_sht4x.h"
#include "sl_sleeptimer.h"
#include "sl_segmentlcd_config.h"

#define TIMEOUT_MS         5000     // Periodic timer duration in ms
#define NEGATIVE_CASE      0        // Used for negative temperature readings
#define THREE_DIGIT_CASE   100000   // Used for three-digit temperature readings

static sl_sleeptimer_timer_handle_t periodic_timer;

volatile bool take_measurement_flag = false;

/***************************************************************************//**
 * Periodic timer callback function
 ******************************************************************************/
void
on_periodic_timeout(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  // This prevents unused parameter warnings
  (void) handle;
  (void) data;

  take_measurement_flag = true;
}

void
measure_temperature(void)
{
  uint32_t rh_data = 0;               // Relative humidity data
  int32_t temp_data = 0;              // Temperature data

  // Measure the values for relative humidity and temperature
  sl_sht4x_measure_rh_and_temp(sl_i2cspm_sensor,
                               SHT4X_ADDR,
                               &rh_data, &temp_data);

  // Read the values for relative humidity and temperature
  sl_sht4x_read_rh_and_temp(sl_i2cspm_sensor,
                            SHT4X_ADDR,
                            &rh_data, &temp_data);

#if defined (SL_SEGMENT_LCD_MODULE_CE322_1002)

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
    sl_segment_lcd_number(temp_data);              // Display the value of
                                                   //   temp_data
    sl_segment_lcd_symbol(SL_LCD_SYMBOL_DEGC, 1);     // Display Degree C symbol
    sl_segment_lcd_symbol(SL_LCD_SYMBOL_P3, 1);       // Display decimal symbol
  }

  // Used when the temperature is above 100 degrees Celsius
  if (temp_data >= THREE_DIGIT_CASE) {
    temp_data = temp_data / 10;
    sl_segment_lcd_number(temp_data);              // Display the value of
                                                   //   temp_data
    sl_segment_lcd_symbol(SL_LCD_SYMBOL_DEGC, 1);     // Display Degree C symbol
    sl_segment_lcd_symbol(SL_LCD_SYMBOL_P3, 1);       // Display decimal symbol
  }
  // Used when the temperature is between 0-100 degrees Celsius
  else {
    sl_segment_lcd_number(temp_data);              // Display the value of
                                                   //   temp_data
    sl_segment_lcd_symbol(SL_LCD_SYMBOL_DEGC, 1);     // Display Degree C symbol
    sl_segment_lcd_symbol(SL_LCD_SYMBOL_P2, 1);       // Display decimal symbol
  }
#elif defined (SL_SEGMENT_LCD_MODULE_CL010_1087)

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
  sl_segment_lcd_lower_number(temp_data);      // Display the value of temp_data
  sl_segment_lcd_symbol(SL_LCD_SYMBOL_DEGC, 1);  // Display Degree C symbol
  sl_segment_lcd_symbol(SL_LCD_SYMBOL_DP5, 1);   // Display decimal symbol
#endif
}

/***************************************************************************//**
 * Disable Unused LCD Segments
 ******************************************************************************/
void
disableUnusedLCDSeg(void)
{
  /*************************************************************************//**
   * The LCD driver enables all segments, even those that are not mapped to
   * segments on the pro kit board. These are disabled below in order to
   * minimize current consumption.
   ****************************************************************************/
#if defined (SL_SEGMENT_LCD_MODULE_CE322_1002)
  LCD_SegmentEnable(9, false);
  LCD_SegmentEnable(11, false);
  LCD_SegmentEnable(12, false);
  LCD_SegmentEnable(13, false);
  LCD_SegmentEnable(14, false);
  LCD_SegmentEnable(15, false);
  LCD_SegmentEnable(16, false);
  LCD_SegmentEnable(17, false);
#elif defined (SL_SEGMENT_LCD_MODULE_CL010_1087)
  LCD_SegmentEnable(15, false);
  LCD_SegmentEnable(16, false);
  LCD_SegmentEnable(17, false);
  LCD_SegmentEnable(18, false);
  LCD_SegmentEnable(19, false);
#endif
}

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void
app_init(void)
{
  // Initialize the SHT4x sensor, sleeptimer, and Segment LCD display

  /*************************************************************************//**
   * sl_sht4x_init(sl_i2cspm_sensor, SHT4X_ADDR);
   * This function sends command for to read UID and checks for I2C response.
   * Current implementation does not give enough time for SHT4x to load response
   * Can be enabled in next version of SiSDK
   ****************************************************************************/

  // Configure LCD to use step down mode and disable unused segments
  // Default display value 0
  sl_segment_lcd_init(false);
  LCD->BIASCTRL_SET = LCD_BIASCTRL_VDDXSEL_AVDD;
  disableUnusedLCDSeg();

#if defined (SL_SEGMENT_LCD_MODULE_CE322_1002)
  // Display all 0's upon initialization
  sl_segment_lcd_number(0);
#elif defined (SL_SEGMENT_LCD_MODULE_CL010_1087)
  // Display 25 degC upon initialization
  sl_segment_lcd_lower_number(25000);
  sl_segment_lcd_symbol(SL_LCD_SYMBOL_DEGC, 1);  // Display Degree C symbol
  sl_segment_lcd_symbol(SL_LCD_SYMBOL_DP5, 1);   // Display decimal symbol
#endif

  // Periodic timer for updating the temperature value displayed
  sl_sleeptimer_start_periodic_timer_ms(&periodic_timer,
                                        TIMEOUT_MS,
                                        on_periodic_timeout,
                                        NULL,
                                        0, 0);
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void
app_process_action(void)
{
  uint32_t rh_data = 0;               // Relative humidity data
  int32_t temp_data = 0;              // Temperature data

  if (take_measurement_flag == true) {
    // Measure the values for relative humidity and temperature
    sl_sht4x_measure_rh_and_temp(sl_i2cspm_sensor,
                                 SHT4X_ADDR,
                                 &rh_data, &temp_data);

    // Read the values for relative humidity and temperature
    sl_sht4x_read_rh_and_temp(sl_i2cspm_sensor,
                              SHT4X_ADDR,
                              &rh_data, &temp_data);

#if defined (SL_SEGMENT_LCD_MODULE_CE322_1002)

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
      sl_segment_lcd_number(temp_data);              // Display the value of
                                                     //   temp_data
      sl_segment_lcd_symbol(SL_LCD_SYMBOL_DEGC, 1);     // Display Degree C symbol
      sl_segment_lcd_symbol(SL_LCD_SYMBOL_P3, 1);       // Display decimal symbol
    }

    // Used when the temperature is above 100 degrees Celsius
    if (temp_data >= THREE_DIGIT_CASE) {
      temp_data = temp_data / 10;
      sl_segment_lcd_number(temp_data);              // Display the value of
                                                     //   temp_data
      sl_segment_lcd_symbol(SL_LCD_SYMBOL_DEGC, 1);     // Display Degree C symbol
      sl_segment_lcd_symbol(SL_LCD_SYMBOL_P3, 1);       // Display decimal symbol
    }
    // Used when the temperature is between 0-100 degrees Celsius
    else {
      sl_segment_lcd_number(temp_data);              // Display the value of
                                                     //   temp_data
      sl_segment_lcd_symbol(SL_LCD_SYMBOL_DEGC, 1);     // Display Degree C symbol
      sl_segment_lcd_symbol(SL_LCD_SYMBOL_P2, 1);       // Display decimal symbol
    }
#elif defined (SL_SEGMENT_LCD_MODULE_CL010_1087)

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
    sl_segment_lcd_lower_number(temp_data);      // Display the value of temp_data
    sl_segment_lcd_symbol(SL_LCD_SYMBOL_DEGC, 1);  // Display Degree C symbol
    sl_segment_lcd_symbol(SL_LCD_SYMBOL_DP5, 1);   // Display decimal symbol
#endif

    take_measurement_flag = false;
  }
}
