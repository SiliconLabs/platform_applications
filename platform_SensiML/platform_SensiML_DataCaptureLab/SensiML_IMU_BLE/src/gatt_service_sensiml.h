/***************************************************************************//**
 * @file
 * @brief Top level application functions
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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

#ifndef SL_GATT_SERVICE_IMU_H
#define SL_GATT_SERVICE_IMU_H

#include "sl_bt_api.h"

/**************************************************************************//**
 * Bluetooth stack event handler.
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void gatt_service_sensiml_imu_on_event(sl_bt_msg_t *evt);

/**************************************************************************//**
 * IMU GATT service sensiml event handler.
 *****************************************************************************/
void gatt_service_sensiml_imu_step(void);

/**************************************************************************//**
 * Getter for Orientation and Acceleration characteristic values.
 * @param[out] data Six dimensional acceleration (3 dimensions) + orientation vector (3 dimensions) .
 * @return Status of the operation.
 * @note To be implemented in user code.
 *****************************************************************************/
sl_status_t gatt_service_sensiml_imu_get(int16_t data[6]);


void gatt_service_sensiml_imu_enable(bool enable);

#endif // SL_GATT_SERVICE_IMU_H
