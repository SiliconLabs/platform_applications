/***************************************************************************//**
 * @file
 * @brief Header file for Bootloader I2C communication.
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
#ifndef SRC_I2C_COMMUNICATION_H_
#define SRC_I2C_COMMUNICATION_H_
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Bootloader command byte definitions
#define BOOT_GBL_DOWNLOAD                   (0x10)
#define BOOT_VERSION                        (0x20)
#define BOOT_BOOT_APP                       (0x30)
#define BOOT_VERIFY                         (0x60)
#define BOOT_ACTIVATE_UPGRADE               (0xA9)

#define BOOT_GET_LAST_CMD_STATUS            (0x55)
#define BOOT_ABORT_OPERATION                (0xAA)

#define BOOT_DOWNLOAD_FRAME                 (0x11)
#define BOOT_DOWNLOAD_COMPLETE              (0X1F)

/// Bootloader response byte definitions
#define BOOT_REPLY_OK                       (0x00)
#define BOOT_REPLY_PENDING                  (0x81)

#define BOOT_REPLY_ERR_INCOMPLETE           (0xF9)
#define BOOT_REPLY_ERR_FRAME_SEQUENCE       (0xFA)
#define BOOT_REPLY_ERR_PARSE                (0xFB)
#define BOOT_REPLY_ERR_CRC                  (0xFC)
#define BOOT_REPLY_ERR_LENGTH               (0xFD)
#define BOOT_REPLY_ERR_VERIFY               (0xFE)
#define BOOT_REPLY_ERR_UNKNOWN              (0xFF)

/// internal operation
#define BOOT_OPERATION_NONE                 (0x00)
#define BOOT_OPERATION_DOWNLOAD             (0x10)
#define BOOT_OPERATION_VERIFY               (0x20)
#define BOOT_OPERATION_BOOTING              (0x30)

#define BOOT_OPERATION_DOWNLOAD_FRAME       (0x11)
#define BOOT_OPERATION_DOWNLOAD_PENDING     (0x12)

/// constants for protocol
#define BOOT_MAX_DOWNLOAD_FRAME_DATA_LENGTH (128)

/// constants for master settings
/// maximum number of repetitions on frame download fails
#define BOOT_SEQUENCE_REPETITIONS_MAX       (5)
/// maximum number of failed status requests
#define BOOT_STATUS_REQUEST_MAX             (5)

/// packing the structure if not defined before (for GCC)
#ifndef __PACKED
#define __PACKED                            __attribute__((packed))
#endif

/// this frame is used for downloading data to slave
typedef struct __PACKED {
  /// command byte
  uint8_t  command;
  /// frame length incl. header
  uint8_t  length;
  /// crc16 of the frame.
  uint16_t crc16;
  /// sequence number for the current frame
  uint16_t frame_seq_nr;
  /// the downloading content bytes
  uint8_t  frame_data[];
} i2c_download_frame_t;

#define BOOT_DOWNLOAD_FRAME_HEADER_SIZE \
  (sizeof(i2c_download_frame_t))

#define BOOT_MAX_DOWNLOAD_FRAME_SIZE \
  (BOOT_MAX_DOWNLOAD_FRAME_DATA_LENGTH + BOOT_DOWNLOAD_FRAME_HEADER_SIZE)

/// version info data
typedef struct __PACKED {
  uint16_t  major;
  uint16_t  minor;
  uint16_t  patch;
} version_info_t;

/// version structure serialization
typedef union {
  version_info_t boot_version;
  uint8_t bytes[sizeof(version_info_t)];
} btl_version_info_t;

/// download frame structure serialization
typedef union {
  i2c_download_frame_t frame;
  uint8_t bytes[BOOT_MAX_DOWNLOAD_FRAME_SIZE];
} i2c_download_frame_data_t;

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* SRC_I2C_COMMUNICATION_H_ */
