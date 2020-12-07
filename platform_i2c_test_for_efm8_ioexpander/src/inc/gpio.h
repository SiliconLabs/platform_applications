/***************************************************************************//**
 * @gpio.h
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

#ifndef INC_GPIO_H_
#define INC_GPIO_H_

#include "hal-config.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>


#define GPIO_LED0_PIN                          (4U)
#define GPIO_LED0_PORT                         (gpioPortF)
#define GPIO_IRQ_INPUT_PIN                     (9U)
#define GPIO_IRQ_INPUT_PORT                    (gpioPortA)


void gpio_init(void);
void gpio_callback(uint8_t pin);


#endif /* INC_GPIO_H_ */
