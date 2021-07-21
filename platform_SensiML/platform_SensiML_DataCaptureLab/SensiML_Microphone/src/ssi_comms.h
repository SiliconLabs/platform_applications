/** @file ssi_comms.h */

/*==========================================================
 * Copyright 2021 QuickLogic Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *==========================================================*/

#ifndef SSI_COMMS_H_
#define SSI_COMMS_H_

#include <inttypes.h>
#define SSI_JSON_CONFIG_VERSION    (2)     /* 2 => Use enhance SSI protocol, 1 => use original SSI protocol */
#define SSI_SYNC_DATA              (0xFF)
#define SSI_HEADER_SIZE            (9)     ///< SSI v2 header size in bytes
#define SSI_MAX_CHANNELS           (4)
#define SSI_CHANNEL_DEFAULT        (0)

extern void ssi_seqnum_init(uint8_t channel);
extern void ssi_seqnum_reset(uint8_t channel);
extern uint32_t ssi_seqnum_update(uint8_t channel);
extern uint32_t ssi_seqnum_get(uint8_t channel);
extern uint8_t ssi_payload_checksum_get(uint8_t *p_data, uint16_t len);

extern void ssiv2_publish_sensor_data(uint8_t channel, uint8_t* p_source, int ilen);

#endif /* SSI_COMMS_H_ */
