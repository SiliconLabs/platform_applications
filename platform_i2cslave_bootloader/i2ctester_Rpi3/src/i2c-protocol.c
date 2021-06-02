/***************************************************************************//**
 * @file
 * @brief Bootloader I2C communication protocol (master).
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

#include "i2c-protocol-prototypes.h"
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#define CRC16_INIT             0xFFFFU

uint16_t crc16(const uint8_t newByte, uint16_t prevResult)
{
  prevResult = (prevResult >> 8) | (prevResult << 8);
  prevResult ^= newByte;
  prevResult ^= (prevResult & 0xFF) >> 4;
  prevResult ^= (prevResult << 8) << 4;

  prevResult ^= ((uint8_t) ((uint8_t) ((uint8_t) (prevResult & 0xFF)) << 5))
                | ((uint16_t) ((uint8_t) ((uint8_t) (prevResult & 0xFF))
                               >> 3) << 8);

  return prevResult;
}

uint16_t crc16s(const uint8_t *buffer,
                         size_t        length,
                         uint16_t      prevResult)
{
  size_t position = 0;
  for (; position < length; position++) {
    prevResult = crc16(buffer[position], prevResult);
  }
  return prevResult;
}

/***************************************************************************//**
* @brief writes the I2C bus
* @param i2c_handle        I2C handle
* @param address           7-bit slave address
* @param commandSeq        sending seqence bytes
* @param commandSeqLength  length of the sequence
* @return negative on error, 0 on success
*******************************************************************************/
static int i2c_write(int i2c_handle,
                     uint8_t address,
                     uint8_t *commandSeq,
                     uint8_t commandSeqLength)
{
  struct i2c_msg i2c_msgs[2] = {
    {.addr = address, .flags = 0, .len = commandSeqLength, .buf = commandSeq }
  };
  struct i2c_rdwr_ioctl_data msgset = {
    .msgs = i2c_msgs, .nmsgs = 1
  };

  if (ioctl(i2c_handle, I2C_RDWR, &msgset) < 0) {
    perror("ioctl(I2C_RDWR) in i2c_write");
    for(int k = 0; k<commandSeqLength; k++) {
      if (!(k&0x1F)) {
        fprintf(stderr, "\n");
      }
      fprintf(stderr, "%02X ", commandSeq[k]);
    }
    return -1;
  }
  return 0;
}

/***************************************************************************//**
* @brief reads from I2C bus
* @param i2c_handle        I2C handle
* @param address           7-bit slave address
* @param commandSeq        sending seqence bytes
* @param commandSeqLength  length of the sequence
* @param readBuffer        buffer to store the bytes readed
* @param readBufferLength  Number of reading bytes
* @return -1 on error, 0 on success
*******************************************************************************/
static int i2c_read(int i2c_handle,
                    uint8_t address,
                    uint8_t *commandSeq,
                    uint8_t commandSeqLength,
                    uint8_t *readBuffer,
                    uint8_t readBufferLength)
{
  struct i2c_msg i2c_msgs[2] = {
    {
     .addr = address,
     .flags = 0,
     .len = commandSeqLength,
     .buf = commandSeq
    },
    {
     .addr = address,
     .flags = I2C_M_RD,
     .len = readBufferLength,
     .buf = readBuffer
    }
  };
  struct i2c_rdwr_ioctl_data msgset = {
    .msgs = i2c_msgs, .nmsgs = 2
  };

  if (ioctl(i2c_handle, I2C_RDWR, &msgset) < 0) {
    perror("ioctl(I2C_RDWR) in i2c_read");
    for(int k = 0; k<commandSeqLength; k++) {
      fprintf(stderr, "%02X ", commandSeq[k]);
    }
    fprintf(stderr, "\n");
    return -1;
  }
  return 0;
}

/***************************************************************************//**
* @brief opens the I2C bus device
* @param bus I2C bus' system id from /dev
* @return I2C handle or error (negative value)
*******************************************************************************/
int open_i2c(int bus)
{
  char filename[64];
  int file;

  snprintf(filename, 64, "/dev/i2c-%d", bus);
  file = open(filename, O_RDWR);

  if (file < 0 && (errno == ENOENT || errno == ENOTDIR)) {
    snprintf(filename, 64, "/dev/i2c/%d", bus);
    open(filename, O_RDWR);
  }
  return file;
}

/***************************************************************************//**
* @brief closes the I2C bus device
* @param i2c_handle I2C handle
* @return 0 if ok or negative value on error
*******************************************************************************/
int close_i2c(int i2c_handle)
{
  return close(i2c_handle);
}

/***************************************************************************//**
* @brief gets the last command's status from the slave
* @param i2c_handle I2C handle
* @param address 7-bit slave address
* @return -1 on error, otherwise slave bootloader's status
*******************************************************************************/
int last_command_status(int i2c_handle, int address)
{
  uint8_t result;
  uint8_t command = BOOT_GET_LAST_CMD_STATUS;
  uint8_t num_retries = BOOT_STATUS_REQUEST_MAX;
  while (--num_retries) {
    /// the slave should have time to process the issued command
    /// we have to wait a while to prevent flooding
    usleep(1000);
    if (!i2c_read(i2c_handle, address, &command,1, &result, 1 )) {
      return result;
    }
  }
  return -1;
}

/***************************************************************************//**
* @brief aborts slave's current operation (effective for DOWNLOAD)
* @param i2c_handle I2C handle
* @param address 7-bit slave address
* @return -1 on error, 0 on success
*******************************************************************************/
int abort_last_command(int i2c_handle, int address)
{
  uint8_t command = BOOT_ABORT_OPERATION;
  if (i2c_write(i2c_handle, address, &command,1)) {
    return -1;
  }
  return 0;
}

/***************************************************************************//**
* @brief gets the bootloader version information from the slave
* @param i2c_handle   I2C handle
* @param address      7-bit slave address
* @param versionInfo  pointer to the versionInfo structure
* @return -1 on error, 0 on success
*******************************************************************************/
int get_version_info(int i2c_handle, int address,
                     btl_version_info_t *versionInfo)
{
  uint8_t command = BOOT_VERSION;
  if (i2c_read(i2c_handle, address, &command,1,
                (uint8_t*)versionInfo, sizeof(btl_version_info_t))) {
    return -1;
  }
  return 0;
}

/***************************************************************************//**
* @brief boots the application on slave from the slot
* @param i2c_handle       I2C handle
* @param address          7-bit slave address
* @param application_slot negative value (for example, -1) when no slot used
*                         zero or positive number: slot id (not supported yet)
* @return -1 on error, 0 on success
*******************************************************************************/
int boot_application(int i2c_handle, int address, int application_slot)
{
  uint8_t command[2] = {BOOT_BOOT_APP, application_slot &0xFF};
  if (i2c_write(i2c_handle, address, command,(application_slot<0) ? 1 : 2)) {
    return -1;
  }
  return 0;
}

/***************************************************************************//**
* @brief verify the application image on slave
* @param i2c_handle       I2C handle
* @param address          7-bit slave address
* @param verifyResult     bootloader' verify result (pointer to an int8_t)
* @return negative on error, 0 on success
*******************************************************************************/
int verify_app(int i2c_handle, int address, uint8_t *verifyResult)
{
  uint8_t command = BOOT_VERIFY;
  int status;
  if (i2c_write(i2c_handle, address, &command,1)) {
    return -1;
  }
  do {
    status = last_command_status(i2c_handle, address)
  } while (status == BOOT_REPLY_PENDING);
  if (status <0) {
    *verifyResult = BOOT_REPLY_ERR_UNKNOWN;
    return status;
  }
  *verifyResult = status;
  return 0;
}

/***************************************************************************//**
* @brief send a chunk frame to slave
* @param i2c_handle            I2C handle
* @param address               7-bit slave address
* @param frame_sequence_number frame sequence number
* @param chunkData             chunk data bytes
* @param chunkSize             amount of data bytes
* @return negative on error, 0 on success
*******************************************************************************/
static int send_chunk_of_stream(int i2c_handle,
                                int address,
                                uint32_t frame_sequence_number,
                                uint8_t *chunkData,
                                uint8_t chunkSize)
{
  i2c_download_frame_data_t downloadData;
  int status;
  downloadData.frame.frame_seq_nr = frame_sequence_number;
  downloadData.frame.crc16   = 0;
  downloadData.frame.length  = chunkSize + BOOT_DOWNLOAD_FRAME_HEADER_SIZE;
  downloadData.frame.command = BOOT_OPERATION_DOWNLOAD_FRAME;
  for(int k = 0; k< chunkSize; k++) {
    downloadData.frame.frame_data[k] = chunkData[k];
  }

  downloadData.frame.crc16 = crc16s(downloadData.bytes,
                                    downloadData.frame.length,
                                    CRC16_INIT);

  if (i2c_write(i2c_handle, address,
                downloadData.bytes, downloadData.frame.length))  {
    return -1;
  }
  do {
    status = last_command_status(i2c_handle, address);
  } while (status == BOOT_OPERATION_DOWNLOAD_PENDING
           || status == BOOT_REPLY_PENDING);
  return status;
}

/***************************************************************************//**
* @brief puts a spiner character to stderr (console)
*******************************************************************************/
static void progress_spinner() {
  static int spinner_index = 0;
  const char *spinnerCharset = "-\\|/";
  fprintf(stderr, "\b%c", spinnerCharset[spinner_index++]);
  spinner_index &=0x3;
}

/***************************************************************************//**
* @brief send a file stream to slave
* @param i2c_handle   I2C handle
* @param address      7-bit slave address
* @param fhandle      file (stream) handle
* @return negative value on error, 0 on success
*******************************************************************************/
static int send_stream_as_file(int i2c_handle, int address, int fhandle) {
  uint8_t chunk[BOOT_MAX_DOWNLOAD_FRAME_DATA_LENGTH];
  size_t readSize;
  int status = BOOT_REPLY_OK;
  uint8_t command = BOOT_GBL_DOWNLOAD;
  uint32_t frame_sequence =0;
  uint32_t repetitions =0;

  if (fhandle <0) {
    return -1;
  }
  if (i2c_write(i2c_handle, address, &command, 1)) {
    return -2;
  }
  while (last_command_status(i2c_handle, address) != BOOT_REPLY_OK);
  fprintf(stderr, "Download ... ");
  do {
    if (status == BOOT_REPLY_OK) {
      readSize = read(fhandle, chunk, BOOT_MAX_DOWNLOAD_FRAME_DATA_LENGTH);
      frame_sequence++;
      progress_spinner();
      repetitions = 0;
    } else {
      if (++repetitions == BOOT_SEQUENCE_REPETITIONS_MAX)
        break;
    }
    if (readSize) {
      status = send_chunk_of_stream(i2c_handle, address, frame_sequence,
                                    chunk, readSize);
      if (status) {
        fprintf(stderr, "chunk %d would be repeated\n", frame_sequence);
      }
    }
  } while (readSize);

  command = status ? BOOT_ABORT_OPERATION: BOOT_DOWNLOAD_COMPLETE;

  if (i2c_write(i2c_handle, address, &command, 1)) {
    return (status ? status : -3);
  }
  do {
    status = last_command_status(i2c_handle, address)
  } while (status == BOOT_REPLY_PENDING);
  return status;
}

/***************************************************************************//**
* @brief download the GBL image file to the slave
* @param i2c_handle   I2C handle
* @param address      7-bit slave address
* @param filename     name of the GBL image file
* @return negative value on error, 0 on success
*******************************************************************************/
int download_gbl_file(int i2c_handle, int address, const char* filename)
{
  int fhandle = open(filename, O_RDONLY);
  struct timeval startTime, endTime;
  int elapsedMicrosecs;
  struct stat buffer;
  int         status;

  gettimeofday(&startTime,NULL);
  int result = send_stream_as_file(i2c_handle, address, fhandle);
  if (!result) {
    status = stat(filename, &buffer);
    if(status == 0) {
      gettimeofday(&endTime, NULL);
      elapsedMicrosecs = ((endTime.tv_sec - startTime.tv_sec) * 1000000)
                          + (endTime.tv_usec - startTime.tv_usec);
      fprintf(stderr, "\n Transfer speed was %8.2f B/s",
              1000000.0*((double)buffer.st_size/(double)elapsedMicrosecs));
    }
  }
  close(fhandle);
  return result;
}
