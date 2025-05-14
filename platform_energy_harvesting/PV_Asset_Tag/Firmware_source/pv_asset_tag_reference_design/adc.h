/***************************************************************************//**
 * @file adc.h
 * @brief ADC driver
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

#ifndef ADC__H
#define ADC__H
#include "config.h"
#define RESISTOR_DIVIDER                        10
#if ASSET_TAG_TYPE == ASSET_TAG_TYPE_RF
#define VFULL                                   3700
#define VCHRDY                                  3400
#define VSOURCE_LIMIT                           900
#define MIN_CHARGING_DELTA_V                    10
#elif ASSET_TAG_TYPE == ASSET_TAG_TYPE_PV
#define VFULL                                   2400
#define VCHRDY                                  1500
#define VSOURCE_LIMIT                           900
#define MIN_CHARGING_DELTA_V                    10
#else
#error unsupported asset tag type!
#endif
#include "types.h"

void measure_voltages(sl_harvester_voltages_t *hv);

#endif // ADC__H
