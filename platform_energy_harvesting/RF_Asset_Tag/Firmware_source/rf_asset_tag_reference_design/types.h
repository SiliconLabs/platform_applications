/***************************************************************************//**
 * @file types.h
 * @brief Common type definitions
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

#ifndef TYPES__H
#define TYPES__H

#include <stdbool.h>

typedef enum {
  POWER_LEVEL_SKIP_2ND = 0,
  POWER_LEVEL_MIN = 1,
  POWER_LEVEL_MEDIUM = 2,
  POWER_LEVEL_HIGH = 3,
  POWER_LEVEL_MAX = 4,
} sl_beacon_power_level_t;
typedef struct
{
    int storage_voltage_millivolts;
    int delta_storage_voltage_millivolts;
    int source_voltage_millivolts;
} sl_harvester_voltages_t;
typedef struct
{
    int channel;
    unsigned int wakeup_counter;
    bool skip_next;
    bool short_payload;
    unsigned int period_time;
    unsigned int beacon_count;
    unsigned int beacon_interval;
} sl_beacon_settings_t;
#endif
