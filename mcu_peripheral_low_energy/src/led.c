/*
 * led.c
 *
 *  Created on: Jun 3, 2019
 *      Author: siwoo
 */

#include "led.h"

void DebugLED_Init(void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);
  GPIO_PinModeSet(DEBUG_LED0_PORT, DEBUG_LED0_PIN, gpioModePushPull, 1);
}

void DebugLED_Toggle(void)
{
  GPIO_PinOutToggle(DEBUG_LED0_PORT, DEBUG_LED0_PIN);
}

void DebugLED_Off(void)
{
  GPIO_PinOutSet(DEBUG_LED0_PORT, DEBUG_LED0_PIN);
}

void DebugLED_On(void)
{
  GPIO_PinOutClear(DEBUG_LED0_PORT, DEBUG_LED0_PIN);
}

