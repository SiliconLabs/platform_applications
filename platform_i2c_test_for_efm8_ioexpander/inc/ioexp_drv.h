/***************************************************************************//**
 * @ioexp_drv.h
 * @brief Test application for evaluation of EFM8-IOExpander (see AN1304)
 * @version 0.0.1
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#ifndef INC_IOEXP_DRV_H_
#define INC_IOEXP_DRV_H_

#define IOEXP_DEFAULT_I2C_ADDR              0xF0 // 0x78 << 1 !!!
#define IOEXP_DEVICE_ID                     0x494F5850
#define IOEXP_REG_DEVICE_ID0                0xF8
#define IOEXP_REG_DEVICE_ID1                0xF9
#define IOEXP_REG_DEVICE_ID2                0xFA
#define IOEXP_REG_DEVICE_ID3                0xFB

#define IOEXP_STATUS_OK                     0     /* Return code, no errors. */
#define IOEXP_STATUS_FAILURE                (-1)  /* Return code, ioexpander
                                                   *   communication failed */

#define IOEXP_I2C_DEFAULT                                              \
  { I2C0,               /* Use I2C instance 0 */                       \
    (gpioPortC),        /* SCL port */                                 \
    (10U),              /* SCL pin */                                  \
    (gpioPortC),        /* SDA port */                                 \
    (11U),              /* SDA pin */                                  \
    (14U),              /* Location of SCL */                          \
    (16U),              /* Location of SDA */                          \
    0,                  /* Use currently configured reference clock */ \
    I2C_FREQ_STANDARD_MAX, /* Set to standard rate  */                 \
    i2cClockHLRStandard, /* Set to use 4:4 low/high duty cycle */      \
  }

int ioexp_init(void);
int ioexp_read_regs(uint8_t addr, uint8_t len, uint8_t *value);
int ioexp_write_regs(uint8_t addr, uint8_t len, uint8_t *value);
uint16_t ioexp_get_i2c_addr(void);
int ioexp_set_i2c_addr(uint16_t addr);

#endif /* INC_IOEXP_DRV_H_ */
