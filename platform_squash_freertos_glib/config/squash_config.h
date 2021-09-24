/***************************************************************************//**
 * @file
 * @brief Squah demo application
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
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
 ******************************************************************************
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/

#ifndef SQUASH_CONF_H
#define SQUASH_CONF_H

// -----------------------------------------------------------------------------
//                                   Includes
// -----------------------------------------------------------------------------

#include "em_gpio.h"


// -----------------------------------------------------------------------------
//                              Macros and Typedefs
// -----------------------------------------------------------------------------

/*  Define if LED demo is used  */
#define LED_DEMO 1

/*  Greeting text position  */
#define GTEXT_X 25
#define GTEXT_Y  1

/* GPIO Porting */
#define GPIO_PORT gpioPortF
#define GPIO_MOD  gpioModeInput

/*  PINS here buttons are allocated   */
#define BUTTON_0_PIN 6
#define BUTTON_1_PIN 7

/*  GPIO interrupt indexes  */
#define BUTTON_0_INT 6
#define BUTTON_1_INT 7

/*  Interrupt priorities  */
#define BUTTON0_INT_PRIO 5
#define BUTTON1_INT_PRIO 5

/*  GPIO interrupt (IRQ) numbers  */
#define BUTTON0_INT_NUM 10
#define BUTTON1_INT_NUM 18

/* Task priorities and stack sizes*/

 #ifdef LED_DEMO
  #define STACK_BLINK_SIZE  256
 #endif
 #define STACK_ROCKET_DOWN_SIZE  256
 #define STACK_UP_DOWN_SIZE  256
 #define STACK_BALL_SIZE  256

 #ifdef LED_DEMO
  #define TASK_BLINK_PRIORITY  2
 #endif
 #define TASK_ROCKET_DOWN_PRIORITY 2
 #define TASK_ROCKET_UP_PRIORITY 2
 #define TASK_BALL_PRIORITY 2

/*  Blinky task delay  */
#ifdef LED_DEMO
  #define BLINK_TASK_DLY  2000
#endif

/*  Racket vertical movement step  */
#define MOV_STEP_Y 2

/*  Ball movement step  */
#define MOV_BALL_X 2
#define MOV_BALL_Y 2

/*  LCD geometry  */
#define LCD_WIDTH 128

#define LCD_HEIGHT 128

#endif // SQUASH_CONF_H
