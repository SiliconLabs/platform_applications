/***************************************************************************//**
 * @file power.c
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

#include "power.h"

#include "adc.h"
#include <em_cmu.h>

#define BURAM_KEY_IDX                           0
#define BURAM_COUNTER_IDX                       1
#define BURAM_CHANNEL_IDX                       2
#define BURAM_LAST_STO_IDX                      3
#define BURAM_SKIP_NEXT_IDX                     4

#define ADV_FIRST_CHANNEL                       37
#define ADV_LAST_CHANNEL                        39

#define BURAM_KEY                               0x8b2eba4f

typedef struct {
  unsigned int period_time;
  unsigned int beacon_count;
  unsigned int beacon_interval;
} timing_table_t;

typedef struct {
  unsigned int tx_power_cBm;
  bool short_payload;
  unsigned int period_time_shift;
  unsigned int beacon_count_shift;
} power_settings_t;

static const timing_table_t timing_table[4] = {
#ifdef DEBUG_EM4
  { .period_time =  6000, .beacon_count =  3, .beacon_interval =  100, },
#else
  { .period_time = 30000, .beacon_count =  3, .beacon_interval =  100, },
#endif
  { .period_time =  60000, .beacon_count =  6, .beacon_interval =  200, },
  { .period_time = 120000, .beacon_count = 12, .beacon_interval =  500, },
  { .period_time = 300000, .beacon_count = 12, .beacon_interval = 1000, },
};

static const power_settings_t power_settings[5] = {
  [POWER_LEVEL_SKIP_2ND] = {
    .tx_power_cBm = 0,
    .short_payload = true,
    .period_time_shift = 0,
    .beacon_count_shift = 0,
  },

  [POWER_LEVEL_MIN] = {
    .tx_power_cBm = 0,
    .short_payload = true,
    .period_time_shift = 0,
    .beacon_count_shift = 0,
  },

  [POWER_LEVEL_MEDIUM] = {
    .tx_power_cBm = 30,
    .short_payload = false,
    .period_time_shift = 0,
    .beacon_count_shift = 1,
  },

  [POWER_LEVEL_HIGH] = {
    .tx_power_cBm = 50,
    .short_payload = false,
    .period_time_shift = 1,
    .beacon_count_shift = 1,
  },

  [POWER_LEVEL_MAX] = {
    .tx_power_cBm = 60,
    .short_payload = false,
    .period_time_shift = 1,
    .beacon_count_shift = 1,
  },
};

sl_beacon_power_level_t decide_power_level(sl_harvester_voltages_t *hv)
{
  // if the capacitor is fully charged, then use the maximum power settings.
  if (hv->storage_voltage_millivolts >= VFULL)
  {
    return POWER_LEVEL_MAX;
  }
  // otherwise, if we still have enough charge set the power according to the charging state
  if (hv->storage_voltage_millivolts  > VCHRDY)
  {
    // did we gain energy since last time?
    // note: because of noise, we must consider a small positive delta.
    if (hv->delta_storage_voltage_millivolts >= MIN_CHARGING_DELTA_V)
    {
      // our capacitor voltage increased since last time. We can use more power. Use even more
      // if we detected a high power source.
      return (hv->source_voltage_millivolts >= VSOURCE_LIMIT) ? POWER_LEVEL_MAX : POWER_LEVEL_HIGH;
    }
    else
    {
      // the capacitor is not being charged. Lower the threshold
      return POWER_LEVEL_MEDIUM;
    }
  }
  // we are in power saving. Reduce as much the consumption, also depending if the power source
  // is present.
  if (hv->delta_storage_voltage_millivolts >= MIN_CHARGING_DELTA_V)
  {
    return (hv->source_voltage_millivolts >= VSOURCE_LIMIT) ? POWER_LEVEL_MEDIUM : POWER_LEVEL_MIN;
  }

  return POWER_LEVEL_SKIP_2ND;
}

unsigned int apply_power_settings(int mode_select, sl_beacon_power_level_t power_level, sl_beacon_settings_t *settings)
{
  const power_settings_t *power = &power_settings[power_level];
  const timing_table_t *timing = &timing_table[mode_select];

  settings->short_payload = power->short_payload;
  settings->period_time = timing->period_time >> power->period_time_shift;
  settings->beacon_count = timing->beacon_count << power->beacon_count_shift;
  settings->beacon_interval = timing->beacon_interval;

  return power->tx_power_cBm;
}


sl_beacon_power_level_t decide_power_settings_and_update_buram(sl_harvester_voltages_t *hv, sl_beacon_settings_t * settings)
{
  CMU_ClockEnable(cmuClock_BURAM, true);

  if (BURAM->RET[BURAM_KEY_IDX].REG == BURAM_KEY)
  {
    settings->wakeup_counter = ++BURAM->RET[BURAM_COUNTER_IDX].REG;

    settings->channel = BURAM->RET[BURAM_CHANNEL_IDX].REG + 1;
    if (settings->channel > ADV_LAST_CHANNEL)
    {
      settings->channel = ADV_FIRST_CHANNEL;
    }
    BURAM->RET[BURAM_CHANNEL_IDX].REG = settings->channel;

    hv->delta_storage_voltage_millivolts = hv->storage_voltage_millivolts - BURAM->RET[BURAM_LAST_STO_IDX].REG;
    BURAM->RET[BURAM_LAST_STO_IDX].REG = hv->storage_voltage_millivolts ;
  }
  else
  {
    BURAM->RET[BURAM_COUNTER_IDX].REG = settings->wakeup_counter = 1;
    BURAM->RET[BURAM_CHANNEL_IDX].REG = settings->channel = ADV_FIRST_CHANNEL;
    BURAM->RET[BURAM_LAST_STO_IDX].REG = hv->storage_voltage_millivolts ;
    BURAM->RET[BURAM_SKIP_NEXT_IDX].REG = settings->skip_next = false;
    BURAM->RET[BURAM_KEY_IDX].REG = BURAM_KEY;

    hv->delta_storage_voltage_millivolts = 0;
  }

  sl_beacon_power_level_t power_level = decide_power_level(hv);

  if (power_level == POWER_LEVEL_SKIP_2ND)
  {
    settings->skip_next = BURAM->RET[BURAM_SKIP_NEXT_IDX].REG;
    BURAM->RET[BURAM_SKIP_NEXT_IDX].REG = !settings->skip_next;
  }
  else
  {
    BURAM->RET[BURAM_SKIP_NEXT_IDX].REG = settings->skip_next = false;
  }

  CMU_ClockEnable(cmuClock_BURAM, false);
  return power_level;
}

