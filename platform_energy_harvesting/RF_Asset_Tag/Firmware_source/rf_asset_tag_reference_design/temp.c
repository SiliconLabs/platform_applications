/***************************************************************************//**
 * @file temp.c
 * @brief Temperature sensor
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

#include "temp.h"

#include <tempdrv.h>


int measure_temperature(void)
{
  int temperature;
  TEMPDRV_Init();
  temperature = (int)(EMU_TemperatureGet() * 100.0f);
  TEMPDRV_DeInit();
  return temperature;
}
