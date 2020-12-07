/***************************************************************************//**
 * @gpio.c
 * @brief Test application for evaluation of EFM8-IOExpander (see AN1304)
 * @version 0.0.1
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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

#include "gpio.h"

#include "gpiointerrupt.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>


/**************************************************************************//**
 * @brief   A callback to handle incoming IRQ signal
 *
 * @param[in] pin  The pin used to identify the IRQ that has been set.
 *****************************************************************************/
void gpio_callback(uint8_t pin)
{
  if (GPIO_PinInGet(GPIO_IRQ_INPUT_PORT, GPIO_IRQ_INPUT_PIN))
  {
    GPIO_PinOutClear(GPIO_LED0_PORT, GPIO_LED0_PIN);
  }
  else
  {
    GPIO_PinOutSet(GPIO_LED0_PORT, GPIO_LED0_PIN);
  }
}

void gpio_init(void)
{
  GPIO_PinModeSet(GPIO_IRQ_INPUT_PORT, GPIO_IRQ_INPUT_PIN, gpioModeInputPull, 1);

  // Button Interrupt Config
  GPIOINT_Init();
  GPIOINT_CallbackRegister(GPIO_IRQ_INPUT_PIN, gpio_callback);
  GPIO_IntConfig(GPIO_IRQ_INPUT_PORT, GPIO_IRQ_INPUT_PIN, true, true, true);
}
