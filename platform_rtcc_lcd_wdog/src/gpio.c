/***************************************************************************//**
 * @file gpio.c
 * @brief GPIO functions
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

#include "gpio.h"
#include "stdbool.h"
#include "em_gpio.h"
#include "em_cmu.h"

volatile uint32_t g_flag_gpio = 0;

/**************************************************************************//**
 * @brief GPIO initialization
 *****************************************************************************/
void init_gpio(void)
{
  /* Enable clock to GPIO  */
  CMU_ClockEnable(cmuClock_GPIO, true);

  /* Configure GPIO pins connected to BTN0 and BTN1 as input */
  GPIO_PinModeSet(gpioPortC, 8, gpioModeInputPullFilter, true);
  GPIO_PinModeSet(gpioPortC, 9, gpioModeInputPullFilter, true);

  /*Enable IRQ for both the buttons */
  __NVIC_EnableIRQ(GPIO_EVEN_IRQn);
  __NVIC_EnableIRQ(GPIO_ODD_IRQn);

  /* Configure the conditions for the interrupt to be triggered */
  GPIO_ExtIntConfig(gpioPortC, 8, 8, true, true, true);
  GPIO_ExtIntConfig(gpioPortC, 9, 9, true, true, true);

  g_flag_gpio = 0;
}

/**************************************************************************//**
 * @brief GPIO IRQ Handler
 *****************************************************************************/
void GPIO_EVEN_IRQHandler(void)
{
  int int_flag = GPIO_IntGet();

  if (int_flag & 0x0100) {
    GPIO_IntClear(0x0100);
    g_flag_gpio = 1;
  }
}

/**************************************************************************//**
 * @brief GPIO IRQ Handler
 *****************************************************************************/
void GPIO_ODD_IRQHandler(void)
{
  int int_flag = GPIO_IntGet();

  if (int_flag & 0x0200) {
    GPIO_IntClear(0x0200);
    g_flag_gpio = 1;
  }
}

/**************************************************************************//**
 * @brief Wrapper function to return flag
 *****************************************************************************/
uint32_t get_flag_gpio(void)
{
  return g_flag_gpio;
}
