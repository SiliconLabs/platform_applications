/**************************************************************************//**
 * @file kit.c
 * @brief Kit specific configuration
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

void kitInit(void)
{
#ifdef VCOM_DISABLE
  GPIO_PinModeSet(VCOM_DISABLE_PORT, VCOM_DISABLE_PIN, gpioModePushPull, 0);
#endif

#ifdef DISPLAY_DISABLE
  GPIO_PinModeSet(DISPLAY_DISABLE_PORT, DISPLAY_DISABLE_PIN, gpioModePushPull,
                  0);
#endif

#ifdef SENSOR_DISABLE
  GPIO_PinModeSet(SENSOR_DISABLE_PORT, SENSOR_DISABLE_PIN, gpioModePushPull, 0);
#endif
}
