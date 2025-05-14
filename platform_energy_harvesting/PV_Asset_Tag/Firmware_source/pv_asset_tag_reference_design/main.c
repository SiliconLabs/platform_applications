/***************************************************************************//**
 * @file
 * @brief main.c
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/

// #define ADV_LE_FEATURES
// #define ADV_POWER_LEVEL
// #define ADV_APPEARANCE
// #define ADV_NEXT_INTERVAL
// #define ADV_LOCAL_NAME
#define ADV_PAYLOAD_HARVESTER
// #define ADV_PAYLOAD_PACKET_TYPE
// #define ADV_PAYLOAD_COUNTER
#define ADV_CRC32
//
#define ASSET_TAG_APPID         (0x53424C53)
#define DEVICE_ID               { 0x05, 0x18, 0x7a, 0x65, 0x25, 0x68, 0x6b, 0xfe }  //
//
#include "sl_component_catalog.h"

#include <em_chip.h>
#include <em_core.h>
#include <em_emu.h>
#include <em_cmu.h>
#include <em_gpio.h>
#include <em_rmu.h>

//#include "hal.h"
#include "radio.h"
#include "adv.h"
#include "adc.h"
#include "temp.h"
#include "power.h"
#include "sleep.h"
#include "system.h"

#include "pin_config.h"

#define NO_SHIELD                               0   // set to 1 if there is no shield present on the EFR32xG22E devkit. No energy awarenesa.


#define EM4WU_ST_STO_RDY                        (1U << (3 + _GPIO_EM4WUEN_EM4WUEN_SHIFT))
#define EM4WU_ST_LOAD                           (1U << (4 + _GPIO_EM4WUEN_EM4WUEN_SHIFT))
#define EM4WU_ST_STO_OVDIS                      (1U << (9 + _GPIO_EM4WUEN_EM4WUEN_SHIFT))
#define EM4WU_WAKE_UP                           (1U << (6 + _GPIO_EM4WUEN_EM4WUEN_SHIFT))
#define EM4WU_BUTTON0                           (1U << (8 + _GPIO_EM4WUEN_EM4WUEN_SHIFT))

#define DEVICE_NAME                             "Harvester"

#define BLE_GAP_MANUFACTURER_CODE_SILABS        0x02ff
#define ADV_PACKET_TYPE_HARVESTER               4242

#if defined(ADV_PAYLOAD_HARVESTER)
PACKSTRUCT(struct harvesterPayload {
  uint16_t companyId;
#if defined(ADV_PAYLOAD_PACKET_TYPE)
  uint16_t packetType;
#endif
  uint8_t deviceId[8];
  int16_t temperature;
  uint16_t Vcap_mV;
  int16_t deltaVcap_mV;
  uint8_t lightIntensity;
#if !defined(ADV_POWER_LEVEL)
  int8_t powerLevel_cBm;
#endif
#if !defined(ADV_NEXT_INTERVAL)
  uint8_t nextInterval_ms[3];
#endif
  uint8_t advState : 4;
  uint8_t DIPSwitch : 4;
#if defined(ADV_PAYLOAD_COUNTER)
  uint8_t counter[3];
#endif
#if defined(ADV_CRC32)  // CRC32 so that the reader will correctly interpret just ASSET TAG data
  uint32_t crc32;
#endif
});
PACKSTRUCT(struct harvesterShortPayload {
  uint16_t companyId;
#if defined(ADV_PAYLOAD_PACKET_TYPE)
  uint16_t packetType;
#endif
  uint8_t deviceId[8];
#if defined(ADV_CRC32)  // CRC32 so that the reader will correctly interpret just ASSET TAG data
  uint32_t crc32;
#endif
});
//

#endif

extern void * __ram_end__;

BtConfig_t btConfig = {
  .txPower = 0,
};

static const uint8_t adv_flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

#if defined(ADV_LE_FEATURES)
static const uint16_t adv_le_features = BT_FEATURE_LE_2M_PHY |
                                        BT_FEATURE_LE_CODED_PHY |
                                        BT_FEATURE_LE_EXTENDED_ADVERTISING;
#endif

#if defined(ADV_APPEARANCE)
static const uint16_t adv_appearance = 0x0200; // Generic Tag
#endif

#if defined(ADV_PAYLOAD_HARVESTER)
struct harvesterPayload harvester_payload = {
  .companyId = BLE_GAP_MANUFACTURER_CODE_SILABS,
  .deviceId = DEVICE_ID,
#if defined(ADV_PAYLOAD_PACKET_TYPE)
  .packetType = ADV_PACKET_TYPE_HARVESTER,
#endif
};
struct harvesterShortPayload harvester_short_payload = {
  .companyId = BLE_GAP_MANUFACTURER_CODE_SILABS,
  .deviceId = DEVICE_ID,
#if defined(ADV_PAYLOAD_PACKET_TYPE)
  .packetType = ADV_PACKET_TYPE_HARVESTER,
#endif
};

#endif

static void initGPCRC(void)
{
   CMU_ClockEnable(cmuClock_GPCRC, true);
   GPCRC->CTRL = GPCRC_CTRL_AUTOINIT | GPCRC_CTRL_POLYSEL_CRC32 ;
   GPCRC->INIT = 0;
   GPCRC->EN = GPCRC_EN_EN;
   GPCRC->CMD = GPCRC_CMD_INIT;
}
static void deinitGPCRC(void)
{
   GPCRC->EN = 0;
   CMU_ClockEnable(cmuClock_GPCRC, false);
}
static uint32_t crc32Calculate(uint8_t *payload, uint32_t length)
{
    GPCRC->INPUTDATA = ASSET_TAG_APPID;
    for (uint32_t i = 0; i < length; i++)
    {
        GPCRC->INPUTDATABYTE = payload[i];
    }
    return GPCRC->DATA;
}
int main(void)
{
  sl_harvester_voltages_t harvester_voltages;
  sl_beacon_settings_t beacon_settings;
  unsigned int mode_select;
  int temperature;
  CHIP_Init();
#if GET_RESET_CAUSE
  int reset_cause = RMU_ResetCauseGet();
  RMU_ResetCauseClear();
#endif
  CMU_ClockEnable(cmuClock_GPIO, true);
#if GET_EM4WU_CAUSE
  int em4wu_mask = GPIO_EM4GetPinWakeupCause();
#endif
  GPIO_IntClear(_GPIO_IF_MASK);
  EMU_UnlatchPinRetention();

  EMU_RamPowerDown((uint32_t)&__ram_end__, 0);

  system_init();
#if NO_SHIELD
  mode_select = 3;    // longest sleep time
  // EM4 wakeup requires the input filter
  GPIO_PinModeSet(BUTTON0_PORT, BUTTON0_PIN, gpioModeInputPullFilter, 1);
  // trigger on a falling edge of WAKE_UP
  // (active-low, button pressed)
  // trigger on a falling edge of BUTTON0
  // (active low, button pressed)
  GPIO_EM4EnablePinWakeup(EM4WU_BUTTON0,  0);
#else
  GPIO_PinModeSet(MODE_SELECT_PORT, MODE_SELECT_PIN, gpioModePushPull, 1);
  GPIO_PinModeSet(MODE_SWITCH0_PORT, MODE_SWITCH0_PIN, gpioModeInput, 0);
  GPIO_PinModeSet(MODE_SWITCH1_PORT, MODE_SWITCH1_PIN, gpioModeInput, 0);
  mode_select = (GPIO_PinInGet(MODE_SWITCH1_PORT, MODE_SWITCH1_PIN) << 1) |
                (GPIO_PinInGet(MODE_SWITCH0_PORT, MODE_SWITCH0_PIN) << 0);
  GPIO_PinModeSet(MODE_SELECT_PORT, MODE_SELECT_PIN, gpioModeDisabled, 0);
  GPIO_PinModeSet(MODE_SWITCH0_PORT, MODE_SWITCH0_PIN, gpioModeDisabled, 0);
  GPIO_PinModeSet(MODE_SWITCH1_PORT, MODE_SWITCH1_PIN, gpioModeDisabled, 0);
  // EM4 wakeup requires the input filter
  GPIO_PinModeSet(ST_STO_RDY_PORT, ST_STO_RDY_PIN, gpioModeInput, 1);
  GPIO_PinModeSet(WAKE_UP_PORT, WAKE_UP_PIN, gpioModeInput, 1);
  GPIO_PinModeSet(BUTTON0_PORT, BUTTON0_PIN, gpioModeInput, 1);


  // trigger on a falling edge of WAKE_UP
  // (active-low, button pressed)
  // trigger on a rising edge of ST_STO_RDY
  // (active high, voltage reaches VCHRDY)
  // trigger on a falling edge of BUTTON0
  // (active low, button pressed)
  GPIO_EM4EnablePinWakeup(EM4WU_ST_STO_RDY | EM4WU_WAKE_UP | EM4WU_BUTTON0,
                          EM4WU_ST_STO_RDY | 0 | 0);
#endif
  temperature = measure_temperature();
  measure_voltages(&harvester_voltages);
  sl_beacon_power_level_t power_level = decide_power_settings_and_update_buram(&harvester_voltages, &beacon_settings);
  btConfig.txPower = apply_power_settings(mode_select, power_level, &beacon_settings);

  if (!GPIO_PinInGet(WAKE_UP_PORT, WAKE_UP_PIN))
  {
    beacon_settings.skip_next = false;
    beacon_settings.short_payload = false;
  }
  if (beacon_settings.skip_next == false)
  {
    unsigned int data_start;

    // going to send packets, have to select HFXO
    prepare_hfxo();

#if defined(ADV_PAYLOAD_HARVESTER)
    if (beacon_settings.short_payload == false)
    {
      harvester_payload.temperature = temperature;
      harvester_payload.Vcap_mV = harvester_voltages.storage_voltage_millivolts;
      harvester_payload.deltaVcap_mV = harvester_voltages.delta_storage_voltage_millivolts;
      harvester_payload.lightIntensity = harvester_voltages.source_voltage_millivolts * 255 / 4000;
      harvester_payload.advState = power_level;
      harvester_payload.DIPSwitch = mode_select;

#if defined(ADV_PAYLOAD_COUNTER)
      memcpy(&harvester_payload.counter, &wakeup_counter, 3);
#endif

#if !defined(ADV_POWER_LEVEL)
      harvester_payload.powerLevel_cBm = btConfig.txPower;
#endif

#if !defined(ADV_NEXT_INTERVAL)
      // Little Endian ordering is assumed here...
      memcpy(&harvester_payload.nextInterval_ms,
             &beacon_settings.beacon_interval,
             sizeof(harvester_payload.nextInterval_ms));
#endif
    }

#endif

    select_hfxo();

    hal_initHw();
    radio_init(&btConfig);
    setup_modem_signals();
    // For demonstration with more than one tag, we sum to the last 6 device ID the
    // address bytes
    const BtAddress_t *address = radio_getAddress();
    for (int i = 0; i < 6; i++)
        harvester_payload.deviceId[i + 2] += address->address[i];
    //
    // calculate Crc32 so that the reader will only use the devices running the correct firmware
#if defined(ADV_PAYLOAD_HARVESTER) && defined(ADV_CRC32)
    initGPCRC();
    if (beacon_settings.short_payload == false)
    {
        uint32_t crc32 = crc32Calculate((uint8_t *) &harvester_payload, sizeof(harvester_payload) - sizeof(harvester_payload.crc32));
        harvester_payload.crc32 = crc32;
    }
    else
    {
        memcpy (&harvester_short_payload,  &harvester_payload, sizeof(harvester_short_payload));
        uint32_t crc32 = crc32Calculate((uint8_t *) &harvester_short_payload, sizeof(harvester_short_payload) - sizeof(harvester_payload.crc32));
        harvester_short_payload.crc32 = crc32;
    }
    deinitGPCRC();

#endif

    adv_init();

    adv_push(BLE_GAP_AD_TYPE_FLAGS, &adv_flags, sizeof(adv_flags));
    //

#if defined(ADV_PAYLOAD_HARVESTER)
    if (beacon_settings.short_payload == false)
    {
        data_start = adv_push(BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA,
                              &harvester_payload,
                              sizeof(harvester_payload)
                               );
    }
    else
    {
        data_start = adv_push(BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA,
                              &harvester_short_payload,
                              sizeof(harvester_short_payload)
                                );
    }
#endif

    if (beacon_settings.short_payload == false)
    {
#if defined(ADV_POWER_LEVEL)
      int8_t adv_tx_power = (btConfig.txPower < 0) ?
        (btConfig.txPower - 5) / 10 :
        (btConfig.txPower + 5) / 10;

      adv_push(BLE_GAP_AD_TYPE_TX_POWER_LEVEL, &adv_tx_power, sizeof(adv_tx_power));
#endif

#if defined(ADV_NEXT_INTERVAL)
      unsigned int next_interval_units = (beacon_interval * 16) / 10;

      // Little Endian ordering is assumed here...
      adv_push(BLE_GAP_AD_TYPE_ADVERTISING_INTERVAL,
               &next_interval_units,
               (next_interval_units < 65536) ? 2 : 3);
#endif

#if defined(ADV_APPEARANCE)
      adv_push(BLE_GAP_AD_TYPE_APPEARANCE, &adv_appearance, sizeof(adv_appearance));
#endif

#if defined(ADV_LE_FEATURES)
      adv_push(BLE_GAP_AD_TYPE_LE_SUPPORTED_FEATURES, &adv_le_features, sizeof(adv_le_features));
#endif

#if defined(ADV_LOCAL_NAME)
      adv_push(BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME, DEVICE_NAME, strlen(DEVICE_NAME));
#endif
    }

    // configure EM4 wakeup here to set this point as the calibrated EM4 wakeup time
    set_wakeup(beacon_settings.period_time);

    // at least 1 beacon is assumed to be the last one
    beacon_settings.beacon_count--;

    // if a "first" one is still there
    if (beacon_settings.beacon_count)
    {
      // send it
      adv_send(beacon_settings.channel);
      beacon_settings.beacon_count--;

      // send the rest, waiting the interval before each turn
      for (unsigned int i = 0; i < beacon_settings.beacon_count; i++)
      {
        // the measured sleep overhead including TX overhead is 680 usecs
        sleep(beacon_settings.beacon_interval);
        adv_send(beacon_settings.channel);
      }

      // wait an interval before sending the last one
      sleep(beacon_settings.beacon_interval);
    }

    // update the interval in the packet
#if defined(ADV_PAYLOAD_HARVESTER)
#if !defined(ADV_NEXT_INTERVAL)
    if (beacon_settings.short_payload == false)
    {
        adv_patch(data_start,
                __builtin_offsetof(struct harvesterPayload, nextInterval_ms),
                &beacon_settings.period_time,
                sizeof(harvester_payload.nextInterval_ms));
        // update CRC32 as well
        // a) copy the new interval
        // Little Endian ordering is assumed here...
        memcpy(&harvester_payload.nextInterval_ms,
               &beacon_settings.period_time,
               sizeof(harvester_payload.nextInterval_ms));

        initGPCRC();

        uint32_t crc32 = crc32Calculate((uint8_t *)&harvester_payload, sizeof(harvester_payload) - sizeof(harvester_payload.crc32));
        harvester_payload.crc32 = crc32;
        deinitGPCRC();
        //
        adv_patch(data_start,
                __builtin_offsetof(struct harvesterPayload, crc32),
                &crc32,
                sizeof(harvester_payload.crc32));
    }
#endif
#endif

    // send the last advertisement
    adv_send(beacon_settings.channel);
  }
  EMU_EnterEM4();
  while (1) __COMPILER_BARRIER();
}
