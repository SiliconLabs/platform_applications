/***************************************************************************//**
 * @file qi.h
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
#ifndef QI_H_
#define QI_H_

#include "stdint.h"

// Link Layer HEADER definitions for Power Receiver to Power Transmitter
// communication per Qi Specification - Communication Protocol v1.3
// Chapter 8, Table 17, only the relevant codes for Class 0 are added.

typedef enum {
  QI_PR_SIG = 0x01,
  QI_PR_EPT = 0x02,
  QI_PR_CE = 0x03,
  QI_PR_RP8 = 0x04,
  QI_PR_CHS = 0x05,
  QI_PR_PCH = 0x06,
  QI_PR_CFG = 0x51,
  QI_PR_ID = 0x71,
  QI_PR_XID = 0x81
} qi_ll_prx_header_t;

typedef enum {
  QI_OK = 0,
  QI_ERR_PARAM,
  QI_ERROR
} qi_status_t;

typedef struct {
  uint16_t             pre_delay;      // delay ms before start of pre-amble
  uint8_t              preamble_bits;  // pre-amble length (min 11, max 25)
  qi_ll_prx_header_t   header;         // header byte defining type of Qi msg
  uint8_t              msglen;         // length of message
  uint8_t              message[];      // message
} qi_message_t;

/**************************************************************************//**
 * Creates SPI stream from a Qi message and adds it to a buffer.
 *
 * @param outbuffer Pointer to start of writing result.
 * @param index Index into outbuffer from where to start adding the stream.
 * @param qi_message Pointer to qi_message structure
 *  to be converted into stream.
 *
 * @returns The result of the creation.
 *****************************************************************************/
qi_status_t qi_create_spi_stream_buffer(uint8_t *outbuffer,
                                        uint16_t *index,
                                        const qi_message_t *qi_message);

/**************************************************************************//**
 * Generates C source code from an SPI stream.
 *
 * @param init_descriptor Name of the static buffer
 *  as declared in C source code.
 * @param buffer Pointer to stream to be converted to C source code.
 * @param buflen The length of buffer content.
 *****************************************************************************/
void qi_generate_c_code(const char *init_descriptor,
                        uint8_t *buffer,
                        uint16_t buflen);

#endif /* QI_H_ */
