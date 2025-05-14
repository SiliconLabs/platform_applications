/***************************************************************************//**
 * @file radio.h
 * @brief minible library header
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

#ifndef RADIO_H
#define RADIO_H

#include <stdint.h>
#include <bt_types.h>
#ifndef UTEST
#include <rail_types.h>
#else
#define RAIL_TIME_DISABLED          1
#define RAIL_TIME_DELAY             2
#define RAIL_TIME_ABSOLUTE          3
#endif

#define RADIO_COMPANY_ID   0x2ff

enum radio_feature {
  BT_FEATURE_LE_DATA_PACKET_LENGTH_EXTENSION = (1 << 5),
  BT_FEATURE_LE_2M_PHY                       = (1 << 8),
  BT_FEATURE_LE_CODED_PHY                    = (1 << 11),
  BT_FEATURE_LE_EXTENDED_ADVERTISING         = (1 << 12),
};

#define RADIO_BYTE_TIME_US          8
#define RADIO_PACKET_LENGTH_DEFAULT 27
#define RADIO_PACKET_LENGTH_MAX     251
#define RADIO_PACKET_CRC_LENGTH     3

enum radio_transition {
  radio_transition_none = 0,
  radio_transition_auto,
};

enum radio_timeMode {
  radio_timeMode_now = RAIL_TIME_DISABLED,
  radio_timeMode_relative = RAIL_TIME_DELAY,
  radio_timeMode_absolute = RAIL_TIME_ABSOLUTE,
};

struct radio_parameters {
  uint32_t startTime;
  uint32_t endTime;   // valid only for rx
  enum radio_timeMode startTimeMode;
  enum radio_timeMode endTimeMode;   // valid only for rx
  enum radio_transition transition;
  uint8_t qtxPower;   // valid only for tx
};

typedef enum {
  radioPhyNone,
  radioPhy1M,
  radioPhy2M,
} RadioPhy_t;

typedef struct {
  uint32_t accessAddress;
  uint32_t crcInit;
  RadioPhy_t phy : 8;
  uint8_t channel;
} RadioConfig_t;

struct radio_rxPacket {
  uint8_t channel;
  bool crcStatus : 1;
  uint16_t length;
  const uint8_t *data;
};

enum radio_event {
  radio_event_unknown,
  radio_event_sent,
  radio_event_received,
  radio_event_timeout,
  radio_event_failure = 0xff,
};
typedef void (*radio_callback)(enum radio_event, const void *data);

void radio_init(const BtConfig_t *config);
void radio_selectRfPath(uint8_t antenna);
void radio_phyInit();
void radio_configure(RadioConfig_t *config);
void radio_loadData(const void *data, uint16_t dataLength);
void radio_tx(const struct radio_parameters *parameters, radio_callback callback);
void radio_rx(const struct radio_parameters *parameters, radio_callback callback);
uint32_t radio_getRxPacketTimestamp();
void radio_abort();
const BtAddress_t *radio_getAddress();
enum radio_feature radio_getSupportedFeatures();
void radio_enableStateTraces(bool enable);
#endif
