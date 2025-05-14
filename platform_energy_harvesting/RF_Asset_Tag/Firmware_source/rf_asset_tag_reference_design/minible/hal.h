/***************************************************************************//**
 * @file hal.h
 * @brief HAL definitions for minible library
 *******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

#ifndef HAL_H
#define HAL_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <rail.h>

typedef enum {
  resetReasonUnknown,
  resetReasonHci,
} ResetReason_t;

typedef enum {
  halPhy1M = 0x01,
  halPhy2M = 0x02,
} HalPhy_t;

void hal_initHw();
void hal_selectRfPath(RAIL_Handle_t railHandle, uint8_t antenna);
void hal_enablePti();
void hal_readDeviceAddress(uint8_t address[6]);
void hal_getRandomData(void *buf, unsigned int size);
void hal_reset(ResetReason_t reason);
ResetReason_t hal_getResetReason();
void hal_setResetReason(ResetReason_t reason);
void hal_softwareBreakpoint();
HalPhy_t hal_getSupportedPhys();
void hal_configureSMUToDefault();

#endif
