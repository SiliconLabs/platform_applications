/** @file ssi_comms.c */

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

#include <inttypes.h>
#include "ssi_comms.h"
#include "sl_iostream.h"

static uint32_t ssi_conn_seqnum[SSI_MAX_CHANNELS] = {0};
void ssi_seqnum_init(uint8_t channel)
{
    if (channel >= SSI_MAX_CHANNELS)
      return;
    ssi_conn_seqnum[channel] = 0;
}

void ssi_seqnum_reset(uint8_t channel)
{
    if (channel >= SSI_MAX_CHANNELS)
      return;
    ssi_conn_seqnum[channel] = 0;
}

uint32_t ssi_seqnum_update(uint8_t channel)
{
    if (channel >= SSI_MAX_CHANNELS)
      return 0;
    ssi_conn_seqnum[channel]++;
    return ssi_conn_seqnum[channel];
}

uint32_t ssi_seqnum_get(uint8_t channel)
{
    if (channel >= SSI_MAX_CHANNELS)
      return 0;
    return ssi_conn_seqnum[channel];
}

uint8_t ssi_payload_checksum_get(uint8_t *p_data, uint16_t len)
{
    uint8_t crc8 = p_data[0];
    for (int k = 1; k < len; k++)
    {
        crc8 ^= p_data[k];
    }
    return crc8;
}

void ssiv2_publish_sensor_data(uint8_t channel, uint8_t* buffer, int size)
{
    uint32_t seqnum;
    uint16_t u16len;
    uint8_t crc8 = 0xFF;
    // compute 8-bit checksum
    crc8 = ssi_payload_checksum_get(buffer, size);

    // Send SYNc data
    uint8_t sync_data = SSI_SYNC_DATA;
    sl_iostream_write(SL_IOSTREAM_STDOUT, &sync_data, sizeof(sync_data));

    // Add channel number
    sl_iostream_write(SL_IOSTREAM_STDOUT, &channel, sizeof(channel));

    // Add sequence number
    seqnum = ssi_seqnum_update(channel);
    sl_iostream_write(SL_IOSTREAM_STDOUT, &seqnum, sizeof(seqnum));

    // Add payload length
    u16len = size;
    sl_iostream_write(SL_IOSTREAM_STDOUT, &u16len, sizeof(u16len));

    sl_iostream_write(SL_IOSTREAM_STDOUT, buffer, size);

    // Add 8-bit checksum
    sl_iostream_write(SL_IOSTREAM_STDOUT, &crc8, 1);
}
