/***************************************************************************//**
 * @file  squash_freertos_config.h
 * @brief Squash FreeRTOS config.
 * @version 0.0.1
 *******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
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
 *
 * EVALUATION QUALITY
 * This code has been minimally tested to ensure that it builds with the
 * specified dependency versions and is suitable as a demonstration for
 * evaluation purposes only.
 * This code will be maintained at the sole discretion of Silicon Labs.
 *
 ******************************************************************************/

#ifndef SQUASH_CONF_H
#define SQUASH_CONF_H

#ifdef __cplusplus
extern "C"
{
#endif
// <<< Use Configuration Wizard in Context Menu >>>

// <h> General
// <q LED_DEMO> Define if LED demo is used
// <i> Default: 1
#define LED_DEMO                    1

// <o GTEXT_X> Greeting text position_X
// <d> 25
#define GTEXT_X 25

// <o GTEXT_Y> Greeting text position_Y
// <d> 1
#define GTEXT_Y 1

// <o LCD_WIDTH> LCD geometry - WIDTH
// <d> 128
#define LCD_WIDTH 128

// <o LCD_HEIGHT> LCD geometry - HEIGHT
// <d> 128
#define LCD_HEIGHT 128

#if (LED_DEMO == 1)
// <o BLINK_TASK_DLY> Blinky task delay (ms)
// <d> 2000
#define BLINK_TASK_DLY 2000
#endif
// </h>


// <h> Stack Size
// <o STACK_ROCKET_DOWN_SIZE> STACK_ROCKET_DOWN_STACK_SIZE
// <d> 256
#define STACK_ROCKET_DOWN_SIZE 256

// <o STACK_UP_DOWN_SIZE> STACK_UP_DOWN_SIZE
// <d> 256
#define STACK_UP_DOWN_SIZE 256

// <o STACK_BALL_SIZE> STACK_BALL_SIZE
// <d> 256
#define STACK_BALL_SIZE 256

#if (LED_DEMO == 1)
// <o STACK_BLINK_SIZE> STACK_BLINK_SIZE
// <d> 256
#define STACK_BLINK_SIZE 256
#endif
// </h>


// <h> Task Priority
// <o TASK_ROCKET_DOWN_PRIORITY> TASK_ROCKET_DOWN_PRIORITY
// <d> 2
#define TASK_ROCKET_DOWN_PRIORITY 2

// <o TASK_ROCKET_UP_PRIORITY> TASK_ROCKET_UP_PRIORITY
// <d> 2
#define TASK_ROCKET_UP_PRIORITY 2

// <o TASK_BALL_PRIORITY> TASK_BALL_PRIORITY
// <d> 2
#define TASK_BALL_PRIORITY 2

#if (LED_DEMO == 1)
// <o TASK_BLINK_PRIORITY> TASK_BLINK_PRIORITY
// <d> 2
#define TASK_BLINK_PRIORITY 2
#endif
// </h>


// <h> Move Step
// <o MOV_STEP_Y> Racket vertical movement step
// <d> 2
#define MOV_STEP_Y 2

// <o MOV_BALL_X> Ball movement step_X
// <d> 2
#define MOV_BALL_X 2

// <o MOV_BALL_Y> Ball movement step_Y
// <d> 2
#define MOV_BALL_Y 2
// </h>


// <h> Button Interrupt Parameters
// <o BUTTON_0_INT> GPIO interrupt indexes - Button 0
// <d> 6
#define BUTTON_0_INT 6

// <o BUTTON_1_INT> GPIO interrupt indexes - Button 1
// <d> 7
#define BUTTON_1_INT 7

// <o BUTTON0_INT_PRIO> Interrupt priorities - Button 0
// <d> 5
#define BUTTON0_INT_PRIO 5

// <o BUTTON1_INT_PRIO> Interrupt priorities - Button 1
// <d> 5
#define BUTTON1_INT_PRIO 5

// <o BUTTON0_INT_NUM> GPIO interrupt (IRQ) numbers - Button 0
// <d> 10
#define BUTTON0_INT_NUM 10

// <o BUTTON1_INT_NUM> GPIO interrupt (IRQ) numbers - Button 1
// <d> 18
#define BUTTON1_INT_NUM 18
// </h>
// <<< end of configuration section >>>

// <<< sl:start pin_tool >>>
// <gpio optional=true> Button_0_GPIO_Port
// $[GPIO_BUTTON_0]
#ifndef BUTTON_0_PORT
#define BUTTON_0_PORT                        gpioPortF
#endif
#ifndef BUTTON_0_PIN
#define BUTTON_0_PIN                         6
#endif
// [GPIO_BUTTON_0]$

// <gpio optional=true> Button_1_GPIO_Port
// $[GPIO_BUTTON_1]
#ifndef BUTTON_1_PORT
#define BUTTON_1_PORT                        gpioPortF
#endif
#ifndef BUTTON_1_PIN
#define BUTTON_1_PIN                         7
#endif
// [GPIO_BUTTON_1]$
// <<< sl:end pin_tool >>>

#ifdef __cplusplus
}
#endif

#endif // SQUASH_CONF_H
