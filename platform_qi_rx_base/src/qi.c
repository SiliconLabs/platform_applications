/***************************************************************************//**
 * @file qi.c
 * @brief QI helper functions.
 * Helper routines for QI interface per Qi Wireless Power Transfer System
 * for PRx transfer (Power Receiver to Transmitter) compliant to v1.3.0 of
 * the standard
 * https://www.wirelesspowerconsortium.com/data/downloadables/3/3/2/3/qi-v13-public.zip
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided \'as-is\', without any express or implied
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
 *******************************************************************************
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include "em_emu.h"

#include "qi.h"

// -----------------------------------------------------------------------------
//                                   Defines
// -----------------------------------------------------------------------------

#define MSG_BUFSIZE 32

// -----------------------------------------------------------------------------
//                            Local function prototypes
// -----------------------------------------------------------------------------

static qi_status_t qi_create_packet(uint16_t* buffer,
                                    const qi_message_t* message);

static qi_status_t qi_add_to_spi_stream(uint8_t *out,
                                        uint8_t *curbyte,
                                        uint8_t *curbit,
                                        const uint16_t in,
                                        uint16_t len);

static uint16_t qi_code_byte(uint8_t);

// -----------------------------------------------------------------------------
//                            Local functions
// -----------------------------------------------------------------------------

/**************************************************************************//**
 * This function takes the QI header + message and creates a full QI data packet
 * structure as per Qi-v1.3-comms-physical.pdf par 3.4 adding checksum and will
 * code it as bit array representing the byte encoded words by adding start bit,
 * parity bit and stop bit for each byte from the data packet.
 *
 * The result is returned in a uint16_t buffer array which has
 * message->msglen + 2 elements due to adding header and checksum.
 *****************************************************************************/
static qi_status_t qi_create_packet(uint16_t *buffer, const qi_message_t *message)
{

  uint8_t curbyte = 0;
  uint8_t chksum = 0;
  uint8_t len = message->msglen;

  if ((buffer == NULL) || (message == NULL)) {
    return (QI_ERR_PARAM);
  }

  // add all bytes, we will maintain checksum along the way
  // start by adding the header
  buffer[curbyte++] = qi_code_byte(message->header);
  chksum ^= message->header;
  // add payload
  for (int i = 0; i < len; i++) {
    buffer[curbyte++] = qi_code_byte(message->message[i]);
    chksum ^= message->message[i];
  }
  // add checksum
  buffer[curbyte] = qi_code_byte(chksum);

  return (QI_OK);
}

/**************************************************************************//**
 * This function adds the <len> LSbits of <in> to <out> starting at
 * <curbyte>:<curbit>, coded as per QI spec.
 *****************************************************************************/
static qi_status_t qi_add_to_spi_stream(uint8_t *out,
                                        uint8_t *curbyte,
                                        uint8_t *curbit,
                                        const uint16_t in,
                                        uint16_t len)
{
  uint8_t cby = *curbyte; // set start byte
  uint8_t cbi = *curbit;  // set start bit

  // check for validity of input to prevent memory corruption
  if ((out == NULL) || (len <= 0)) {
    return (QI_ERR_PARAM);
  }

  for (int i = len - 1; i >= 0; i--) {
    out[cby] |= ((in & (0x0001 << i)) ? (1 << cbi) : 0);
    if (cbi == 0) {
      cbi = 7;
      cby++;
    } else {
      cbi--;
    }
  }
  *curbyte = cby;
  *curbit = cbi;
  return (QI_OK);
}

/**************************************************************************//**
 * This function performs the Byte encoding scheme as specified in par 3.3 of
 * the QI-v1.3-comms-physical.pdf.
 *****************************************************************************/
static uint16_t qi_code_byte(uint8_t u8data)
{
  uint8_t parity = 0;
  // init "result" with start bit zero
  uint16_t result = 0;

  // next we need to shift the u8Data with LSB first
  // (see Qi-v1.3-comms-physical.pdf, par 3.3 fig 8)
  // we will keep track of parity along the way
  for (int i = 0; i < 8; i++) {
    result <<= 1;
    if (u8data & (1 << i)) {
      result++; // add a 1
      parity++; // update parity
    }
  }

  // add parity bit
  result <<= 1;
  result += ((parity & 0x1) ? 0 : 1);

  // add stop bit
  result <<= 1;
  result++;

  return (result);
}

// -----------------------------------------------------------------------------
//                           Global functions
// -----------------------------------------------------------------------------

/**************************************************************************//**
 * Creates SPI stream from a Qi message and adds it to a buffer
 *****************************************************************************/
qi_status_t qi_create_spi_stream_buffer(uint8_t *outbuffer,
                                        uint16_t *index,
                                        const qi_message_t *qi_message)
{
  qi_status_t qi_status = QI_OK;
  uint8_t curbyte = *index; // continue in case of multiple calls
  uint8_t curbit = 7;
  uint8_t last_level;
  uint16_t bitbuf;
  uint16_t msgbuf[MSG_BUFSIZE]; // use fixed length buffer, to be optimized

  if ((outbuffer == NULL) || (qi_message == NULL)
      || (qi_message->msglen > MSG_BUFSIZE)) {
    return (QI_ERR_PARAM);
  }

  // we use 8 bits transfer in SPI at 4kHz transfer rate
  // start with pre delay in ms
  // we need to add 4 "1" bits per ms
  bitbuf = 0x000f;
  for (uint8_t i = 0; i < (qi_message->pre_delay); i++) {
    qi_status = qi_add_to_spi_stream(outbuffer, &curbyte, &curbit, bitbuf, 4);
    EFM_ASSERT(qi_status == QI_OK);
  }

  // preamble: add 10 sequence per preamble bit
  bitbuf = 0x0002;
  for (int i = 0; i < qi_message->preamble_bits; i++) {
    qi_status = qi_add_to_spi_stream(outbuffer, &curbyte, &curbit, bitbuf, 2);
    EFM_ASSERT(qi_status == QI_OK);
  };
  last_level = 0;

  // now code the message first
  qi_status = qi_create_packet(msgbuf, qi_message);
  EFM_ASSERT(qi_status == QI_OK);
  // add every bit to the outbuffer
  for (int i = 0; i < qi_message->msglen + 2; i++) {
    for (int j = 10; j >= 0; j--) {
      if (msgbuf[i] & (1 << j)) {
        // bit is a 1 so change
        bitbuf = (last_level ? 0x1 : 0x2);
      } else {
        bitbuf = (last_level ? 0x0 : 0x3);
        last_level = (last_level ? 0 : 1);
      }
      qi_status = qi_add_to_spi_stream(outbuffer, &curbyte, &curbit, bitbuf, 2);
      EFM_ASSERT(qi_status == QI_OK);
    }
  }

  if ((curbit != 7) || (last_level != 1)) {
    // we want to be sure to end with high level so add 1s until the end
    bitbuf = 0x0001;
    for (int i = curbit; i >= 0; i--) {
      qi_status = qi_add_to_spi_stream(outbuffer, &curbyte, &curbit, bitbuf, 1);
      EFM_ASSERT(qi_status == QI_OK);
    }
    curbyte++;
  }

  // after this index is pointing to first non used byte
  *index = curbyte;

  return QI_OK;
}

/**************************************************************************//**
 * Generates C source code from an SPI stream
 *****************************************************************************/
void qi_generate_c_code(const char *init_descriptor,
                        uint8_t *buffer,
                        uint16_t buflen)
{
  printf("\n");
  printf("/******************************************************\n");
  printf(" * generated buffer for QI transmit over SPI, rev 0.1\n");
  printf(" ******************************************************/\n");
  printf("const uint8_t %s[] = {", init_descriptor);
  for (int i = 0; i < buflen; i++) {
    if ((i % 8) == 0) {
      if (i != 0)
        printf(",");
      printf("\n  ");
    } else
      printf(", ");
    printf("0x%02X", buffer[i]);
  }
  printf("\n};\n");
}

