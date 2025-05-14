/***************************************************************************//**
 * @file power.h
 * @brief Power level definitions
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

#ifndef POWER__H
#define POWER__H

#include "types.h"

unsigned int apply_power_settings(int mode_select, sl_beacon_power_level_t power_level, sl_beacon_settings_t *settings);
sl_beacon_power_level_t decide_power_settings_and_update_buram(sl_harvester_voltages_t *hv, sl_beacon_settings_t * settings);
sl_beacon_power_level_t decide_power_level(sl_harvester_voltages_t *hv);
#endif
