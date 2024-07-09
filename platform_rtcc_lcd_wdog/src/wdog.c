/***************************************************************************//**
 * @file wdog.c
 * @brief WDOG functions
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

#include "wdog.h"
#include "em_wdog.h"
#include "assert.h"

volatile uint32_t g_flag_warning = 0;
volatile uint32_t g_flag_reset = 0;
volatile int int_flags;

/**************************************************************************//**
 * @brief WDOG initialization
 *****************************************************************************/
void init_wdog(void)
{
  WDOGn_Enable(WDOG0, false);

  WDOG_Init_TypeDef wdog_config;
  wdog_config.enable = false;
  wdog_config.debugRun = true;
  wdog_config.em2Run = true;
  wdog_config.clkSel = wdogClkSelLFXO;

  /* 256k count period was selected to provide enough time period
   * to display the functionalities of the project. If the time period
   * is kept too short, the output displayed on the LCD would occur too
   * quickly to be observable.
   */
  wdog_config.perSel = wdogPeriod_256k;

  /* Warning time is configured as 75% so as to provide enough time to the
   * user to observe it's output on the LCD.
   */
  wdog_config.warnSel = wdogWarnTime75pct;
  wdog_config.em3Run = true;
  wdog_config.em4Block = false;
  wdog_config.swoscBlock = false;
  wdog_config.resetDisable = false;

  /* wdog_config.resetDisable can be set to true
   * to prevent the system from being reset. In this case the TOUT interrupt
   *   from
   * the WDOG will occur and "System reset string will be displayed on the LCD
   */

  WDOGn_Init(WDOG0, &wdog_config);

  WDOGn_IntEnable(WDOG0, (WDOG_IF_TOUT | WDOG_IF_WARN));
  WDOGn_IntClear(WDOG0, (WDOG_IF_TOUT | WDOG_IF_WARN));
  __NVIC_EnableIRQ(WDOG0_IRQn);
  WDOGn_Enable(WDOG0, true);

  g_flag_warning = 0;
  g_flag_reset = 0;
}

/**************************************************************************//**
 * @brief WDOG IRQ Handler
 *****************************************************************************/
void WDOG0_IRQHandler(void)
{
  int_flags = WDOGn_IntGet(WDOG0);

  WDOGn_IntClear(WDOG0, int_flags);
  if (int_flags & WDOG_IFS_WARN) {
    g_flag_warning = 1;
  } else if (int_flags & WDOG_IFS_TOUT) {
    g_flag_reset = 1;
  }
}

/**************************************************************************//**
 * @brief Feed WDOG timer
 *****************************************************************************/
void feed_wdog()
{
  WDOGn_Feed(WDOG0);
}

/**************************************************************************//**
 * @brief Wrapper function to return flag
 *****************************************************************************/
uint32_t get_flag_warning(void)
{
  return g_flag_warning;
}

/**************************************************************************//**
 * @brief Wrapper function to reset flag
 *****************************************************************************/
void reset_flag_warning(void)
{
  g_flag_warning = 0;
}

/**************************************************************************//**
 * @brief Wrapper function to return flag
 *****************************************************************************/
uint32_t get_flag_reset(void)
{
  return g_flag_reset;
}

/**************************************************************************//**
 * @brief Wrapper function to reset flag
 *****************************************************************************/
void reset_flag_reset(void)
{
  g_flag_reset = 0;
}
