/***************************************************************************//**
 * @file
 * @brief Bootloader I2C slave communication.
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
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
 *******************************************************************************
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/

#include <string.h>
#include "config/btl_config.h"
#include "api/btl_interface.h"
#if defined(BOOTLOADER_NONSECURE)
// NS headers
#include "core/btl_reset_ns.h"
#include "core/btl_bootload_ns.h"
#else
#include "core/btl_reset.h"
#include "core/btl_bootload.h"
#endif
#include "driver/btl_driver_delay.h"
#include "core/btl_reset.h"
#include "debug/btl_debug.h"
#include "btl_i2c_communication.h"
#include "btl_i2c_queue.h"
#include "btl_i2c_slave_driver.h"
#include "bootloader-version.h"
#include "security/btl_crc16.h"

// Parser
#include "parser/gbl/btl_gbl_parser.h"
#include "security/btl_security_types.h"

static volatile uint8_t i2c_comm_current_operation = BOOT_OPERATION_NONE;
static volatile uint8_t i2c_comm_last_operation_status = BOOT_REPLY_OK;
static volatile bool got_message = false;
static uint8_t message[BOOT_MAX_DOWNLOAD_FRAME_SIZE];
static uint8_t message_length;

static btl_version_info_t boot_version = {
  .boot_version = {
    .major = _BOOTLOADER_VERSION_MAJOR,
    .minor = _BOOTLOADER_VERSION_MINOR,
    .patch = _BOOTLOADER_VERSION_PATCH
  }
};

static ImageProperties_t imageProps = {
  .contents = 0U,
  .instructions = 0U,
  .imageCompleted = false,
  .imageVerified = false,
  .bootloaderVersion = 0,
  .application = { 0 },
#if defined(SEMAILBOX_PRESENT) || defined(CRYPTOACC_PRESENT)
  .seUpgradeVersion = 0
#endif
};

ParserContext_t parserContext = { 0 };
DecryptContext_t decryptContext = { 0 };
AuthContext_t authContext = { 0 };

static const BootloaderParserCallbacks_t parseCb = {
  .context = NULL,
  .applicationCallback = bootload_applicationCallback,
  .metadataCallback = NULL,
  .bootloaderCallback = bootload_bootloaderCallback
};

static void verify(void);
static void start_download(void);
static void boot_application(void);
static bool handle_interrupt_level_commands(queue_t *rx_queue,
                                            queue_t *tx_queue);
static void process_message(queue_t *rx_queue);

/**
 * I2C communication handler callback.
 * @param rx_queue The receive queue from I2C driver where received bytes
 *        stored.
 * @param tx_queue The transmit queue, where the sending bytes stored.
 */
static void  i2c_comm_read(queue_t *rx_queue, queue_t *tx_queue)
{
  if (!handle_interrupt_level_commands(rx_queue, tx_queue)) {
    process_message(rx_queue);
  }
}

/**
 * process non IRQ level messages, like firmware download, verify, etc.
 * @param rx_queue The receive queue from I2C driver where received bytes
 *        stored.
 */
static void process_message(queue_t *rx_queue)
{
  uint16_t c;
  if (!got_message) {
    /* If the communication main is in idle state and has no message to process.
     * The first important thing: set the response to pending to avoid
     * the master to see some incorrect result from the previously processed
     * message.
     */
    i2c_comm_last_operation_status = BOOT_REPLY_PENDING;
    for (message_length = 0;
         ((c = queue_pop(rx_queue)) != QUEUE_EOF);
         message[message_length++] = c & 0xFF) {
      // get out all of received bytes
    }
    // notify the communication main about the message.
    got_message = true;
  }
}

/**
 * @brief Processing interrupt level commands. Such ones are executing in sort
 *        time to avoid to miss interrupts.
 * @param rx_queue The receive queue from I2C driver where received bytes
 *        stored.
 * @param tx_queue The transmit queue, where the sending bytes stored.
 * @return true when the message has been processed.
 */
static bool handle_interrupt_level_commands(queue_t *rx_queue,
                                            queue_t *tx_queue)
{
  bool result = false;
  // peek queue to get the command byte
  switch (queue_peek(rx_queue)) {
    case BOOT_ABORT_OPERATION:
      /* aborting operation is setting the current operation to NONE.
       * download process checks that the variable is still set to
       * BOOT_OPERATION_DOWNLOAD. If not, just returns to communication main
       * without closing any parsing process.
       */
      i2c_comm_current_operation = BOOT_OPERATION_NONE;
      // pop the command byte
      queue_pop(rx_queue);
      // the message considered to processed
      result = true;
      break;
    case BOOT_GET_LAST_CMD_STATUS:
      // gets the last command's status
      // pop the command byte
      queue_pop(rx_queue);
      // pushing response to the transmit queue
      queue_push(tx_queue, i2c_comm_last_operation_status);
      // the message considered to processed
      result = true;
      break;
    case BOOT_VERSION:
      // gets the boot version info
      // pop the command byte
      queue_pop(rx_queue);

      for (unsigned int i = 0; i < sizeof(btl_version_info_t);
           queue_push(tx_queue, boot_version.bytes[i++])) {
        // just copy the structure to the transmit queue
      }
      // the message considered to processed
      result = true;
      break;
    default:
      result = false;
      break;
  }
  return result;
}

void communication_init(void)
{
  BTL_DEBUG_PRINTLN("i2c slave initialization");
  I2C_slave_init();
}

int32_t communication_start(void)
{
  BTL_DEBUG_PRINTLN("set i2c transaction callback");
  I2C_set_transaction_callback(i2c_comm_read);
  return BOOTLOADER_OK;
}

int32_t communication_main(void)
{
  int32_t ret = BOOTLOADER_OK;
  uint8_t command;
  while (true) {
    i2c_comm_current_operation = BOOT_OPERATION_NONE;
    while (!got_message) {
      // wait for message
    }
    command = message[0];
    switch (command) {
      case BOOT_GBL_DOWNLOAD:
        // software download
        i2c_comm_current_operation = BOOT_OPERATION_DOWNLOAD_PENDING;
        start_download();
        break;
      case BOOT_VERIFY:
        // verify the stored app
        i2c_comm_current_operation = BOOT_OPERATION_VERIFY;
        verify();
        break;
      case BOOT_BOOT_APP:
        // boot the stored app
        i2c_comm_current_operation = BOOT_OPERATION_BOOTING;
        boot_application();
        // if we are here, something strange happend
        i2c_comm_last_operation_status = BOOT_REPLY_ERR_UNKNOWN;
        break;
      default:
        // unsupported operation
        break;
    }
    // done with the current message
    got_message = false;
  }
  return ret;
}

void communication_shutdown(void)
{
  I2C_slave_shutdown();
  I2C_set_transaction_callback(NULL);
}

/**
 * @brief Boots the stored application or bootloader update
 */
static void boot_application(void)
{
  if (imageProps.imageCompleted && imageProps.imageVerified) {
#if defined(SEMAILBOX_PRESENT) || defined(CRYPTOACC_PRESENT)
    if ((imageProps.contents & BTL_IMAGE_CONTENT_SE)
        && (bootload_checkSeUpgradeVersion(imageProps.seUpgradeVersion))) {
      // Install SE upgrade
#if defined(BOOTLOADER_NONSECURE)
      bootload_commitSeUpgrade();
#else
      bootload_commitSeUpgrade(BTL_UPGRADE_LOCATION);
#endif
      // If we get here, the SE upgrade failed
    } else {
#endif
    if ((imageProps.contents & BTL_IMAGE_CONTENT_BOOTLOADER)
        && (imageProps.bootloaderVersion
            > mainBootloaderTable->header.version)) {
      // Install bootloader upgrade
      bootload_commitBootloaderUpgrade(BTL_UPGRADE_LOCATION,
                                       imageProps.bootloaderUpgradeSize);
    } else {
      // Enter app
      communication_shutdown();
      reset_resetWithReason(BOOTLOADER_RESET_REASON_GO);
    }
#if defined(SEMAILBOX_PRESENT) || defined(CRYPTOACC_PRESENT)
  }
#endif
  }
}

/**
 * verifies the stored application
 */
static void verify(void)
{
  // set operation to verify
  i2c_comm_current_operation = BOOT_OPERATION_VERIFY;
  bool result = bootload_verifyApplication(BTL_APPLICATION_BASE);
  // update status
  i2c_comm_current_operation = BOOT_OPERATION_NONE;
  if (result != 0) {
    i2c_comm_last_operation_status = BOOT_REPLY_OK;
  } else {
    i2c_comm_last_operation_status = BOOT_REPLY_ERR_VERIFY;
  }
}

/**
 * checks the currently received frame data correctness
 * @param frame the received frame data
 * @param received_length the length of received message
 * @return 0 if ok, or BOOT_REPLY_ERR_xxxx
 */
static uint8_t check_frame(i2c_download_frame_data_t *frame,
                           uint8_t received_length)
{
  if (frame->frame.length != received_length) {
    return BOOT_REPLY_ERR_LENGTH;
  }
  uint16_t crc16 = frame->frame.crc16;
  frame->frame.crc16 = 0;
  uint16_t calc_crc16 = btl_crc16Stream(frame->bytes, frame->frame.length,
                                        BTL_CRC16_START);
  frame->frame.crc16 = crc16;
  if (calc_crc16 != crc16) {
    return BOOT_REPLY_ERR_CRC;
  }
  return 0;
}

/**
 * the download communication handler
 */
static void start_download(void)
{
  uint8_t status;
  bool downloading = true;
  got_message = false;
  i2c_download_frame_data_t *downloadFrame =
    (i2c_download_frame_data_t *)message;
  uint32_t frame_sequence_number = 1;
  BTL_DEBUG_PRINTLN("start_download()");
  // Initialize EBL parser
  parser_init(&parserContext,
              &decryptContext,
              &authContext,
              PARSER_FLAG_PARSE_CUSTOM_TAGS);
  memset(&imageProps, 0, sizeof(ImageProperties_t));
  imageProps.instructions = 0xFFU;
  // set the current operation
  i2c_comm_current_operation = BOOT_OPERATION_DOWNLOAD;
  // update status
  i2c_comm_last_operation_status = BOOT_REPLY_OK;
  while (downloading) {
    i2c_comm_current_operation = BOOT_OPERATION_DOWNLOAD;
    while (!got_message
           && i2c_comm_current_operation == BOOT_OPERATION_DOWNLOAD) {
      // wait for abort or message
    }

    // operation aborted
    if (!got_message) {
      break;
    }
    switch (downloadFrame->frame.command) {
      case BOOT_DOWNLOAD_FRAME:
        i2c_comm_current_operation = BOOT_OPERATION_DOWNLOAD_FRAME;
        status = check_frame(downloadFrame, message_length);
        if (status) {
          // check_frame returned an error;
          i2c_comm_last_operation_status = status;
          break;
        }
        if (frame_sequence_number != downloadFrame->frame.frame_seq_nr) {
          // sequence is missmatched.
          i2c_comm_last_operation_status = BOOT_REPLY_ERR_FRAME_SEQUENCE;
          break;
        }
        BTL_DEBUG_PRINT("Frame: ");
        BTL_DEBUG_PRINT_SHORT_HEX(frame_sequence_number);
        BTL_DEBUG_PRINT_LF();
        // Packet is OK, update sequence number and parse its contents
        frame_sequence_number++;
        int32_t code = parser_parse(&parserContext,
                                    &imageProps,
                                    downloadFrame->frame.frame_data,
                                    downloadFrame->frame.length
                                    - BOOT_DOWNLOAD_FRAME_HEADER_SIZE,
                                    &parseCb);
        if (code != BOOTLOADER_OK) {
          BTL_DEBUG_PRINT("parser_error: ");
          BTL_DEBUG_PRINT_WORD_HEX(code);
          BTL_DEBUG_PRINT_LF();
          i2c_comm_last_operation_status = BOOT_REPLY_ERR_PARSE;
        } else {
          i2c_comm_last_operation_status = BOOT_REPLY_OK;
        }
        break;

      case BOOT_DOWNLOAD_COMPLETE:
        // master mentioning that download is complete
        // check whether parser is okay with this
        if (imageProps.imageCompleted && imageProps.imageVerified) {
          i2c_comm_last_operation_status = BOOT_REPLY_OK;
        } else {
          i2c_comm_last_operation_status = BOOT_REPLY_ERR_INCOMPLETE;
        }
        downloading = false;
        break;

      default:
        // unknown frame command
        BTL_DEBUG_PRINT("frame.unknown");
        BTL_DEBUG_PRINT_LF();
        i2c_comm_last_operation_status = BOOT_REPLY_ERR_UNKNOWN;
        break;
    }
    got_message = false;
  }
}
