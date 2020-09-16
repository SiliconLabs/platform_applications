/*
 * systick.h
 *
 *  Created on: July 29, 2020
 *      Author: jobodnar
 */

#ifndef DRIVERS_SYSTICK_H_
#define DRIVERS_SYSTICK_H_

#include "em_device.h"
#include "em_device.h"
#include "em_assert.h"

void SysTick_Start(void);
void SysTick_Stop(void);
void SysTick_IntEnable(void);
void SysTick_IntDisable(void);
void SysTick_SetPeriod(uint32_t);
uint32_t SysTick_GetPeriod(void);
uint32_t SysTick_GetValue(void);
void SysTick_SetValue(uint32_t );

#endif /* DRIVERS_SYSTICK_H_ */
