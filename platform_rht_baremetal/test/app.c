/***************************************************************************//**
 * @file  app.c
 * @brief Top level application functions.
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************
 *
 * EVALUATION QUALITY
 * This code has been minimally tested to ensure that it builds with
 * the specified dependency versions and is suitable as a demonstration for
 * evaluation purposes only.
 * This code will be maintained at the sole discretion of Silicon Labs.
 *
 ******************************************************************************/
// -----------------------------------------------------------------------------
//                                   Includes
// -----------------------------------------------------------------------------
#include <stdbool.h>
#include "printf.h"
#include "em_emu.h"
#include "app.h"
#include "app_rht.h"
#include "app_display.h"

// -----------------------------------------------------------------------------
//                              Macros and Typedefs
// -----------------------------------------------------------------------------
#define VCOM_PRINT_EXTENSION (1) // Used to enable VCOM printing
#define LCD_SCREEN_EXTENSION (1) // Used to enable the LCD display feedback

#define INTER_MEASUREMENT_DELAY 1000 // Time between measurements

// -----------------------------------------------------------------------------
//                                Global Variables
// -----------------------------------------------------------------------------
volatile bool rht_ongoing_delay = false;

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  // Allow debug in EM2 - Useful to set breakpoints and read RH and Temperature
  // variables. Comment this line to improve energy consumption in EM2
  EMU->CTRL |= EMU_CTRL_EM2DBGEN;

  // Verify RHT sensor presence
  rht_init();

#if LCD_SCREEN_EXTENSION
  // Initialize display
  display_init();
  display_rht_screen();
#endif
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
  float temperature = 0;
  float humidity = 0;
  uint32_t hum_data = 0;
  int32_t temp_data = 0;

  if (!rht_ongoing_delay) {
    // Get RH and temperature data
    rht_get_hum_and_temp(&hum_data, &temp_data);

    temperature = (float)temp_data / 1000;
    humidity = (float)hum_data / 1000;

#if LCD_SCREEN_EXTENSION
    // Update display with new values
    display_rht_screen_update(humidity, temperature);
#endif
#if VCOM_PRINT_EXTENSION
    // Re-target new values to CLI
    printf("Temperature: %2.1f C, Humidity: %2.1f%%\r\n", temperature, humidity);
#endif

    // Set delay for next measurement
    rht_intermeasurement_delay(INTER_MEASUREMENT_DELAY, &rht_ongoing_delay);
  }
}
