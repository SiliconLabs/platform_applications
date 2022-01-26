/***************************************************************************//**
 * @file app_qidrv_prx.c
 * @brief QI driver for PRx communication.
 * Routines to generate and send PRx messages over QI interface per Qi Wireless
 * Power Transfer System for PRx transfer (Power Receiver to Transmitter)
 * compliant to v1.3.0 of the standard
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
#include <app_qidrv_prx.h>
#include <string.h>
#include <stdio.h>
#include "em_emu.h"
#include "spidrv.h"
#include "sl_spidrv_instances.h"
#include "sl_sleeptimer.h"
#include "qi.h"

// -----------------------------------------------------------------------------
//                                   Defines
// -----------------------------------------------------------------------------

// use SPI handle for EXP header (configured in project settings)
#define SPI_HANDLE                  sl_spidrv_exp_handle

// size of transmission and reception buffers
#define QI_TX_BUFFER_SIZE 4096 // can likely be smaller for most use cases
#define QI_CE_INTERVAL 250     // interval between QI code error messages
#define QI_PREAMBLE_BITS 11

// define to generate static (flash) code for special POR/GPIO
// hw triggered operation
#define QI_GENERATE_CODE 0

// set the Power Receiver Manufacturing Code as registered with the WPC
#define WPC_POWER_RECEIVER_MANUFACTURING_CODE 0xFFFF

// -----------------------------------------------------------------------------
//                            Variable declarations
// -----------------------------------------------------------------------------


// Example "SIG" message (QI Spec Communications Protocol v1.3 par 8.17) with:
// signal strength = 0xff.
// This brings the transmitter from state 1 ping phase to state 2 in
// configuration phase.
// This message has to come within tping (par 4.2) from start of Power ON
// by Power Transmitter, so there is no delay at the start.
static const qi_message_t qi_signalstrength_message = {
  0,
  QI_PREAMBLE_BITS,
  QI_PR_SIG,
  1,
  {
    0xff
  }
};

// Example "ID" message (QI Spec Communications Protocol v1.3 par 8.11) with:
// Major version = 1,
// Minor version = 1 (implying base line protocol),
// EXT = 0,
// Basic Device Identifier set to 0xEFFFFFF.
// This brings the transmitter from state 2 to state 4 in configuration phase.
// The start of this packet has to adhere to timing constraints as per par 5.2,
// meaning: minimum delay 6, target 7, max delay less than 22ms, so we choose
// 7 ms (target)
static const qi_message_t qi_ident_message = {
  7,
  QI_PREAMBLE_BITS,
  QI_PR_ID,
  7,
  {
    0x11,
    (uint8_t) (WPC_POWER_RECEIVER_MANUFACTURING_CODE >> 8),
    (uint8_t) (WPC_POWER_RECEIVER_MANUFACTURING_CODE & 0x00FF) ,
    0xEF,
    0xFF,
    0xFF,
    0xFF
  }
};

// Example "CFG/bp" message (QI Spec Communications Protocol v1.3 par 8.4) with:
// Neg = 0, (so AI = OB = Dup = Buffer Size = Pol = Depth = 0 as per spec)
// Reference Power = 10,
// Window size = 8, offset = 2.
// This brings the transmitter from state 4 in configuration phase into baseline
// protocol power transfer phase state 11. The start of this packet has to
// adhere to timing constraints as per par 5.2, meaning: minimum delay 6,
// max delay less than 22ms timeout, so we choose 14ms.
static const qi_message_t qi_config_message = {
  14,
  QI_PREAMBLE_BITS,
  QI_PR_CFG,
  5,
  {
    0x0a,
    0x00,
    0x00,
    0x42,
    0x00
  }
};

// Example Control Error (CE) message (QI Spec Communications Protocol v1.3
// par 8.5) with:
// Control Error = 0 (which means, no change request to power).
// The start of this packet has to adhere to timing constraints as per par 5.2
// meaning: minimum delay 6, max delay less than 22ms timeout so we choose 14ms
// the default CE message is triggered from SW delay and requires no delay.
static const qi_message_t qi_default_control_error_message = {
  0,
  QI_PREAMBLE_BITS,
  QI_PR_CE,
  1,
  {
    0x00
  }
};

#if (QI_GENERATE_CODE)
// The default Control Error (CE) message for full LDMA is continuously running
// and requires a delay added to fulfill timing requirements as per QI Spec
// Communications Protocol v1.3 par 7.3.2.
// So the target interval is 250ms from end of last packet. This is also true
// for first CE packet after entering the Power Transfer phase.
static const qi_message_t qi_predelay_control_error_message = {
  QI_CE_INTERVAL,
  QI_PREAMBLE_BITS,
  QI_PR_CE,
  1,
  {
    0x00
  }
};
#endif

// Flag to signal that transfer is complete
static volatile uint8_t qi_step = 0;

// Transmission buffer
static uint8_t tx_buffer[QI_TX_BUFFER_SIZE];

#if (QI_GENERATE_CODE)
  uint8_t temp_buffer[QI_TX_BUFFER_SIZE];
#endif

// -----------------------------------------------------------------------------
//                            Local function prototypes
// -----------------------------------------------------------------------------

void transfer_callback(SPIDRV_HandleData_t *handle,
                       Ecode_t transfer_status,
                       int items_transferred);

// -----------------------------------------------------------------------------
//                            Local functions
// -----------------------------------------------------------------------------

/**************************************************************************//**
 * This function is called after the SPI transfer has finished. If the transfer
 * was ok it will increase the step variable so the ticker function can decide
 * next step based on progress.
 *****************************************************************************/
void transfer_callback(SPIDRV_HandleData_t *handle,
                       Ecode_t transfer_status,
                       int items_transferred)
{
  (void) &handle;
  (void) items_transferred;

  // Post semaphore to signal to application
  // task that transfer is successful
  if (transfer_status == ECODE_EMDRV_SPIDRV_OK) {
    qi_step++;
  }
}

// -----------------------------------------------------------------------------
//                           Global functions
// -----------------------------------------------------------------------------

/*******************************************************************************
 * Initialize example.
 ******************************************************************************/
void qi_drv_init(void)
{
  Ecode_t ecode;
  const uint32_t Tx_allff = 0xFFFF;

  memset(tx_buffer, 0, QI_TX_BUFFER_SIZE);

  // make sure we start with Tx high
  ecode = SPIDRV_MTransmitB(SPI_HANDLE, &Tx_allff, 1);
  EFM_ASSERT(ecode == ECODE_OK);
}

/***************************************************************************//**
 * Ticking function
 ******************************************************************************/
void qi_drv_app_process_action(void)
{
  Ecode_t ecode;
  qi_status_t qi_status;
  uint8_t cur_qi_step;
  uint16_t tx_index = 0;

  while (1) {
    cur_qi_step = qi_step;
    switch (cur_qi_step) {
      case 0:
        // Initialize Qi power transfer.
        // This is the initial Qi stream to be sent, immediately after POR.
        // It consist of 3 messages: SIG, IDENT and CONFIG each preceded by a
        // short pause as specified in the message definition.
        // spi_create_spi_stream will concatenate these into a single Tx buffer.

        // add first message: SIG
        qi_status = qi_create_spi_stream_buffer(tx_buffer, &tx_index,
            &qi_signalstrength_message);
        EFM_ASSERT(qi_status == QI_OK);

        // add second message: IDENT
        qi_status = qi_create_spi_stream_buffer(tx_buffer, &tx_index,
            &qi_ident_message);
        EFM_ASSERT(qi_status == QI_OK);

        // add third message: CONFIG
        qi_status = qi_create_spi_stream_buffer(tx_buffer, &tx_index,
            &qi_config_message);
        EFM_ASSERT(qi_status == QI_OK);

#if (QI_GENERATE_CODE)
        qi_generate_c_code("QI_init", tx_buffer, tx_index);
#endif
        break;

      default:
        // now we keep on repeating the default control error packet forever

        // reset and clear buffer
        tx_index = 0;
        memset(tx_buffer, 0, QI_TX_BUFFER_SIZE);

        // create message: CE (Control Error)
        qi_status = qi_create_spi_stream_buffer(tx_buffer, &tx_index,
            &qi_default_control_error_message);
        EFM_ASSERT(qi_status == QI_OK);

#if (QI_GENERATE_CODE)
        if (qi_step == 1) {
          uint16_t temp_index = 0;
          qi_status = qi_create_spi_stream_buffer(temp_buffer, &temp_index,
              &qi_predelay_control_error_message);
          EFM_ASSERT(qi_status == QI_OK);
          qi_generate_c_code("QI_control_error_PREDELAY", temp_buffer,
              temp_index);
        }
#endif
        break;
    }

    // now send the packet using DMA, all timing is done in hardware

    ecode = SPIDRV_MTransmit(SPI_HANDLE, tx_buffer, tx_index,
        transfer_callback);
    EFM_ASSERT(ecode == ECODE_OK);

    //wait until Tx complete (in Micrium we can do lots of other things)
    while (cur_qi_step == qi_step) {
      EMU_EnterEM1();
    }
#if !(QI_GENERATE_CODE)
    // when in charge phase we have time to print something as well
    if (qi_step > 0) {
      printf("In charge mode, sent control error packet %d\n", qi_step);
    }
#endif
    // and delay until next Tx (and we can do a lot of other things again)
    sl_sleeptimer_delay_millisecond(QI_CE_INTERVAL);
  }
}
