/**************************************************************************//**
 * @file debug.c
 * @brief Functions to enable and toggle GPIO pins for debugging
 * @author Silicon Labs
 * @version x.xx
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
#include "em_cmu.h"
#include "em_gpio.h"
#include "config.h"

void debugInit(void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);

  GPIO_PinModeSet(DBG_GPIO_PORT0, DBG_GPIO_PIN0, gpioModeInputPull, 1);
  GPIO_PinModeSet(DBG_GPIO_PORT1, DBG_GPIO_PIN1, gpioModeInputPull, 1);
}

void debug0Toggle(void)
{
  GPIO_PinOutToggle(DBG_GPIO_PORT0, DBG_GPIO_PIN0);
}

void debug1Toggle(void)
{
  GPIO_PinOutToggle(DBG_GPIO_PORT1, DBG_GPIO_PIN1);
}
