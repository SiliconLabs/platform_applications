/*
 * led.h
 *
 *  Created on: Jun 3, 2019
 *      Author: siwoo
 */

#ifndef SRC_LED_H_
#define SRC_LED_H_

#include <stdbool.h>
#include "em_cmu.h"
#include "em_gpio.h"

#define DEBUG_LED0_PORT      gpioPortH
#define DEBUG_LED0_PIN       10

void DebugLED_Init(void);

void DebugLED_Toggle(void);

void DebugLED_On(void);

void DebugLED_Off(void);

#endif /* SRC_LED_H_ */
