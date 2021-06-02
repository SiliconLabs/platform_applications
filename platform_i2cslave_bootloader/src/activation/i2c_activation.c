/***************************************************************************//**
 * @file
 * @brief Bootloader I2C activation.
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
#include "core/btl_bootload.h"
#include "driver/btl_driver_delay.h"
#include "core/btl_reset.h"
#include "plugin/debug/btl_debug.h"
#include "btl_i2c_communication.h"
#include "btl_i2c_queue.h"
#include "btl_i2c_slave_driver.h"

#ifndef BOOTLOADER_I2C_ACTIVATION_DELAY
  #define BOOTLOADER_I2C_ACTIVATION_DELAY 2500
#endif

#ifdef BOOTLOADER_SUPPORT_COMMUNICATION
#include "plugin/communication/btl_communication.h"

static volatile bool enterBootloader = false;

/**
  * @brief This function called by the I2C slave driver.
  * @param  rx_queue The receive queue from I2C driver where received bytes
  *         stored.
  * @return void
  */
static void  i2c_comm_activation(queue_t *rx_queue)
{
  switch (queue_peek(rx_queue)) {
    case BOOT_ACTIVATE_UPGRADE:
      queue_pop(rx_queue);
      BTL_DEBUG_PRINTLN("BTL i2c activated");
      enterBootloader = true;
      break;
    default:
      break;
  }
}

/**
 *  @brief Initializes the activation feature. Also initializes I2C driver
 *         and sets the I2C callback
 *  @return void
 */
void i2c_activation_init(void)
{
  enterBootloader = false;
  communication_init();
  delay_init();
  I2C_set_transaction_callback((rx_callback_t)i2c_comm_activation);
}

void i2c_activation_done(void)
{
  communication_shutdown();
}

/**
 * @brief Checks whether bootloader should enter to upgrade mode
 *        instead of booting application.
 *        Note: it blocks the execution up to BOOTLOADER_I2C_ACTIVATION_DELAY
 *        miliseconds.
 */
bool   i2c_enter_bootloader(void)
{
  BTL_DEBUG_PRINTLN("i2c_enter_bootloader");
  delay_milliseconds(BOOTLOADER_I2C_ACTIVATION_DELAY, false);
  while(!delay_expired() && !enterBootloader);

  return enterBootloader;
}
#else
/**
 * @brief As BOOTLOADER_SUPPORT_COMMUNICATION has not defined,
 *        empty function is defined.
 */
void i2c_activation_init(void)
{
  BTL_DEBUG_PRINTLN("Warning: BOOTLOADER_SUPPORT_COMMUNICATION is not"
                    " defined");
}

/**
 * @brief As BOOTLOADER_SUPPORT_COMMUNICATION has not defined,
 *        empty function is defined.
 */
bool   i2c_enter_bootloader(void)
{
  return false;
}

void i2c_activation_done(void)
{
  BTL_DEBUG_PRINTLN("Warning: BOOTLOADER_SUPPORT_COMMUNICATION is not"
                    " defined");
}
#endif
