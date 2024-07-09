/***************************************************************************//**
 * @file app.c
 * @brief Top level application functions
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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

#include "rtcc.h"
#include "lcd.h"
#include "wdog.h"
#include "gpio.h"
#include "stdio.h"
#include "sl_power_manager.h"

#define WDOG_MAX_COUNT          8
#define WDOG_FEED_COUNT         6

#define APP_IS_OK_TO_SLEEP      (true)
#define APP_SLEEP_ON_ISR_EXIT   (SL_POWER_MANAGER_IGNORE)

bool app_is_ok_to_sleep(void)
{
  return APP_IS_OK_TO_SLEEP;
} // app_is_ok_to_sleep()

sl_power_manager_on_isr_exit_t app_sleep_on_isr_exit(void)
{
  return APP_SLEEP_ON_ISR_EXIT;
}

// SL_POWER_MANAGER_DEBUG

volatile uint32_t flag_time;
volatile uint32_t flag_warning;
volatile uint32_t flag_reset;
volatile uint32_t flag_gpio;
int count = 0;

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  /* init the different peripherals being used */
  init_rtcc();
  init_wdog();
  init_lcd();
  init_gpio();
  sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM2);
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
  /* The aim of the project is to display the time and date on the LCD.
   * If the LCD isn't updated regularly enough,
   * the Watchdog timer will reset the system and bring it to a safe known
   *   state.
   *
   * For testing purposes, either of the push buttons can be used to stop the
   *   RTCC from updating the LCD,
   * leading to a system reset.
   */

  /* If warning flag is set,
   * then display the relevant string on LCD */
  flag_warning = get_flag_warning();
  if (flag_warning == 1) {
    reset_flag_warning();
    update_lcd_warning();
  }

  /* If reset flag is set,
   * then display the relevant string on LCD.
   * Works only if resetDisable is set to true for WDOG*/
  flag_reset = get_flag_reset();
  if (flag_reset == 1) {
    reset_flag_reset();
    update_lcd_reset();
  }

  /*If time flag is set, it indicates that a sec has passed.
   * The LCD is updated and WDOG timer might be fed,
   * depending on the situation.*/
  flag_time = get_flag_time();
  if (flag_time == 1) {
    reset_flag_time();

    /* increment counter that keeps track of
     * the number of times the LCD is updated */
    count++;

    /* Get time from RTCC */
    get_time(&g_time_info);

    /*Get date from RTCC */
    get_date(&g_time_info);

    /*Update time and date on LCD */
    update_lcd_date(g_time_info.year,
                    g_time_info.month,
                    g_time_info.day_of_month);
    update_lcd_time(g_time_info.hour, g_time_info.min, g_time_info.sec);

    /* If this condition is satisfied, the LCD is being updated regularly
     *   enough.
     * It would be better to keep the condition, count == 8 .
     * The issue is that WDOG might overflow while the count variable is being
     *   set to 8 and
     * WDOG needs a few perclk cycles to clear the counter. Thus, it would be
     *   better if the WDOG
     * is cleared a little earlier than intended.
     */

    flag_gpio = get_flag_gpio();
    if ((count >= WDOG_FEED_COUNT) && (flag_gpio == 0)) {
      feed_wdog();
    }

    /* Arbitrary reason to clear the LCD at this point.
     * It would be easier this way than if the LCD was cleared immediately after
     *   feeding WDOG
     */
    if (count == WDOG_MAX_COUNT) {
      clear_lcd();
    }

    /* Sync count with the WDOG timer*/
    if (count == WDOG_MAX_COUNT) {
      count = 0;
    }
  }
}
