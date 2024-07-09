/***************************************************************************//**
 * @file
 * @brief Header file for Bootloader I2C communication prototypes on master.
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
#ifndef _I2C_PROTOCOL_PROTOTYPES_H
#define _I2C_PROTOCOL_PROTOTYPES_H
#include "btl_i2c_communication.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
* @brief opens the I2C bus device
* @param bus I2C bus' system id from /dev
* @return I2C handle or error (negative value)
*******************************************************************************/
int open_i2c(int bus);

/***************************************************************************//**
* @brief closes the I2C bus device
* @param i2c_handle I2C handle
* @return 0 if ok or negative value on error
*******************************************************************************/
int close_i2c(int i2c_handle);

/***************************************************************************//**
* @brief gets the last command's status from the slave
* @param i2c_handle I2C handle
* @param address 7-bit slave address
* @return -1 on error, otherwise slave bootloader's status
*******************************************************************************/
int last_command_status(int i2c_handle, int address);

/***************************************************************************//**
* @brief aborts slave's current operation (effective for DOWNLOAD)
* @param i2c_handle I2C handle
* @param address 7-bit slave address
* @return -1 on error, 0 on success
*******************************************************************************/
int abort_last_command(int i2c_handle, int address);

/***************************************************************************//**
* @brief gets the bootloader version information from the slave
* @param i2c_handle   I2C handle
* @param address      7-bit slave address
* @param versionInfo  pointer to the versionInfo structure
* @return -1 on error, 0 on success
*******************************************************************************/
int get_version_info(int i2c_handle,
                     int address,
                     btl_version_info_t *versionInfo);

/***************************************************************************//**
* @brief boots the application on slave from the slot
* @param i2c_handle       I2C handle
* @param address          7-bit slave address
* @param application_slot negative value (for example, -1) when no slot used
*                         zero or positive number: slot id (not supported yet)
* @return -1 on error, 0 on success
*******************************************************************************/
int boot_application(int i2c_handle, int address, int application_slot);

/***************************************************************************//**
* @brief verify the application image on slave
* @param i2c_handle       I2C handle
* @param address          7-bit slave address
* @param verifyResult     bootloader' verify result (pointer to an int8_t)
* @return negative on error, 0 on success
*******************************************************************************/
int verify_app(int i2c_handle, int address, uint8_t *verifyResult);

/***************************************************************************//**
* @brief download the GBL image file to the slave
* @param i2c_handle   I2C handle
* @param address      7-bit slave address
* @param filename     name of the GBL image file
* @return negative value on error, 0 on success
*******************************************************************************/
int download_gbl_file(int i2c_handle, int address, const char *filename);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
