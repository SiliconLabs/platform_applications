/*
 * systick.h
 *
 *  Created on: Aug 25, 2017
 *      Author: jobodnar
 */

#ifndef DRIVERS_SYSTICK_H_
#define DRIVERS_SYSTICK_H_

#include "em_device.h"
#include "em_assert.h"
#include "core_cm33.h"

void SysTick_Start(void);
void SysTick_Stop(void);
void SysTick_IntEnable(void);
void SysTick_IntDisable(void);
void SysTick_SetPeriod(uint32_t);
uint32_t SysTick_GetPeriod(void);
uint32_t SysTick_GetValue(void);

#endif /* DRIVERS_SYSTICK_H_ */
