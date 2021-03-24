/***************************************************************************//**
 * @file
 * @brief LED functions
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided \'as-is\', without any express or implied
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


#include "sl_simple_led.h"
#include "sl_simple_led_instances.h"
#include "sl_sleeptimer.h"
#include "app_led.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

#ifndef LED_INSTANCE0
#define LED_INSTANCE0    sl_led_led0
#endif

#ifndef LED_INSTANCE1
#define LED_INSTANCE1    sl_led_led1
#endif

#ifndef TOGGLE_DELAY_MS
#define TOGGLE_DELAY_MS         500
#endif

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/

sl_sleeptimer_timer_handle_t timer;
bool toggle_timeout = false;

/*******************************************************************************
 *********************   LOCAL FUNCTION PROTOTYPES   ***************************
 ******************************************************************************/

static void on_timeout(sl_sleeptimer_timer_handle_t *handle,
                       void *data);

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * Initialize leds; led0 (red) for sending config; led1 (green) as heartbeat.
 ******************************************************************************/
void app_led_init(void)
{
  // Create timer for waking up the system periodically to toggle heartbeat.
  sl_sleeptimer_start_periodic_timer_ms(&timer,
                                        TOGGLE_DELAY_MS,
                                        on_timeout, NULL,
                                        0,
                                        SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);

  // Turn on LED0 (red) to indicate current app state is sending config JSON
  sl_led_turn_on(&LED_INSTANCE0);
}

/***************************************************************************//**
 * LED ticking function.
 ******************************************************************************/
void app_led_process_action(void)
{
  if (toggle_timeout == true) {
    sl_led_toggle(&LED_INSTANCE1);
    toggle_timeout = false;
  }
}

/***************************************************************************//**
 * Turn of configuration LED indicator
 ******************************************************************************/
void app_config_led_control(bool state)
{
  if(state == ON) {
      sl_led_turn_on(&LED_INSTANCE0);
  }
  else if(state == OFF) {
      sl_led_turn_off(&LED_INSTANCE0);
  }
}

/***************************************************************************//**
 * Sleeptimer timeout callback.
 ******************************************************************************/
static void on_timeout(sl_sleeptimer_timer_handle_t *handle,
                       void *data)
{
  (void)&handle;
  (void)&data;
  toggle_timeout = true;
}
