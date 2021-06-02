/***************************************************************************//**
 * @file
 * @brief Header for I2C slave mode driver
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

#ifndef _I2C_SLAVE_DRIVER_H_
#define _I2C_SLAVE_DRIVER_H_

#include "btl_i2c_queue.h"
#include "hal-config.h"
#include "bootloader-configuration.h"
#include "em_gpio.h"
#include "em_i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

#define I2C_OPERATION_WRITE 0
#define I2C_OPERATION_READ  1
#define I2C_OPERATION_NONE  2

#ifndef BTL_DRIVER_I2C_ADDRESS
 #ifdef BTL_I2C_APP_ADDRESS
  #define BTL_DRIVER_I2C_ADDRESS BTL_I2C_APP_ADDRESS
 #else
  #define BTL_DRIVER_I2C_ADDRESS 0x04
 #endif
#endif


#define HAL_PORT_I2C0 (0)
#define HAL_PORT_I2C1 (1)
#define HAL_PORT_I2C2 (2)

#if BSP_I2C_APP_PORT == HAL_PORT_I2C0

#define BTL_DRIVER_I2C_PORT      I2C0
#define BTL_DRIVER_I2C_NUM       0
#define BTL_DRIVER_SDA_PIN       BSP_I2C0_SDA_PIN
#define BTL_DRIVER_SDA_PORT      BSP_I2C0_SDA_PORT
#define BTL_DRIVER_SCL_PIN       BSP_I2C0_SCL_PIN
#define BTL_DRIVER_SCL_PORT      BSP_I2C0_SCL_PORT
#define BTL_DRIVER_I2C_CLOCK     cmuClock_I2C0
#define BTL_DRIVER_I2C_IRQ       I2C0_IRQn
#define BTL_DRIVER_IRQ_HANDLER() I2C0_IRQHandler(void)

#elif BSP_I2C_APP_PORT == HAL_PORT_I2C1

#define BTL_DRIVER_I2C_PORT      I2C1
#define BTL_DRIVER_I2C_NUM       1
#define BTL_DRIVER_SDA_PIN       BSP_I2C1_SDA_PIN
#define BTL_DRIVER_SDA_PORT      BSP_I2C1_SDA_PORT
#define BTL_DRIVER_SCL_PIN       BSP_I2C1_SCL_PIN
#define BTL_DRIVER_SCL_PORT      BSP_I2C1_SCL_PORT
#define BTL_DRIVER_I2C_CLOCK     cmuClock_I2C1
#define BTL_DRIVER_I2C_IRQ       I2C1_IRQn
#define BTL_DRIVER_IRQ_HANDLER() I2C1_IRQHandler(void)

#elif BSP_I2C_APP_PORT == HAL_PORT_I2C2

#define BTL_DRIVER_I2C_PORT      I2C2
#define BTL_DRIVER_I2C_NUM       2
#define BTL_DRIVER_SDA_PIN       BSP_I2C2_SDA_PIN
#define BTL_DRIVER_SDA_PORT      BSP_I2C2_SDA_PORT
#define BTL_DRIVER_SCL_PIN       BSP_I2C2_SCL_PIN
#define BTL_DRIVER_SCL_PORT      BSP_I2C2_SCL_PORT
#define BTL_DRIVER_I2C_CLOCK     cmuClock_I2C2
#define BTL_DRIVER_I2C_IRQ       I2C2_IRQn
#define BTL_DRIVER_IRQ_HANDLER() I2C2_IRQHandler(void)

#endif


/*******************************************************************************
 ********************************   TYPEDEFS   **********************************
******************************************************************************/


typedef void (*rx_callback_t)(queue_t *rx_queue, queue_t *tx_queue);


/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Initialize I2C peripheral.
 *
 * @details
 *   This driver supports slave mode only.
 ******************************************************************************/
void I2C_slave_init(void);

/***************************************************************************//**
 * @brief
 *  ShutdownI2C peripheral.
 ******************************************************************************/
void I2C_slave_shutdown(void);

/***************************************************************************//**
 * @brief
 *   Perform I2C transfer.
 *
 * @param[in] i2c
 *   Pointer to the peripheral port
 *
 * @param[in] seq
 *   Pointer to sequence structure defining the I2C transfer to take place. The
 *   referenced structure must exist until the transfer has fully completed.
 *
 * @return
 *   Returns status of ongoing transfer
 ******************************************************************************/
void I2C_set_transaction_callback(const rx_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif /* I2C_SLAVE_DRIVER_H */
