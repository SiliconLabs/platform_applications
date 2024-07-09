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

#ifndef _BOARD_I2C_SLAVE_CONFIG_H_
#define _BOARD_I2C_SLAVE_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_PORT_I2C0          (0)
#define HAL_PORT_I2C1          (1)
#define HAL_PORT_I2C2          (2)

#define BSP_I2C_APP_PORT       HAL_PORT_I2C1
#define BSP_I2C_SDA_PIN        3
#define BSP_I2C_SDA_PORT       gpioPortD
#define BSP_I2C_SCL_PIN        2
#define BSP_I2C_SCL_PORT       gpioPortD

#define BTL_I2C_APP_ADDRESS    0x05

#ifdef __cplusplus
}
#endif

#endif /* _BOARD_I2C_SLAVE_CONFIG_H_ */
