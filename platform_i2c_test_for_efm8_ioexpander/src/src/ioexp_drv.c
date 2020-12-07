/***************************************************************************//**
 * @ioexp_drv.c
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

#include "ioexp_drv.h"

#include "i2cspm.h"
#include "em_cmu.h"
#include "em_gpio.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>


/***************************************************************************//**
 * Global variables
 ******************************************************************************/
static uint16_t i2c_dest_addr = IOEXP_DEFAULT_I2C_ADDR;

/***************************************************************************//**
 * Function prototypes
 ******************************************************************************/
static uint32_t ioexp_get_device_id(void);


/***************************************************************************//**
 * @brief
 *    Enable I/O expander.
 ******************************************************************************/
int ioexp_init(void)
{
  I2CSPM_Init_TypeDef i2cInit = IOEXP_I2C_DEFAULT;

  /* Enable GPIO clock */
  CMU_ClockEnable(cmuClock_GPIO, true);

  /* Initialize the I2C Simple Polled Mode driver */
  I2CSPM_Init(&i2cInit);

  /* Check that the device is responding */
  if (ioexp_get_device_id() != IOEXP_DEVICE_ID) {
    ioexp_set_i2c_addr(ioexp_get_i2c_addr() + 2 ); // Note for 7bit addr!
    if (ioexp_get_device_id() != IOEXP_DEVICE_ID) {
      return IOEXP_STATUS_FAILURE;
    }
  }

  return IOEXP_STATUS_OK;
}


/***************************************************************************//**
 * @brief
 *    Reads the device ID of the IO expander
 *
 * @return
 *    Returns the device ID read from the device
 ******************************************************************************/
static uint32_t ioexp_get_device_id(void)
{
  uint32_t result;
  uint8_t *pU8;

  pU8 = (uint8_t*)&result;

  ioexp_read_regs(IOEXP_REG_DEVICE_ID0, 1, pU8++);
  ioexp_read_regs(IOEXP_REG_DEVICE_ID1, 1, pU8++);
  ioexp_read_regs(IOEXP_REG_DEVICE_ID2, 1, pU8++);
  ioexp_read_regs(IOEXP_REG_DEVICE_ID3, 1, pU8++);

  return result;
}

/***************************************************************************//**
 * @brief
 *    Performs register read through the I2C bus from the IO expander
 *
 * @param[in] addr
 *    The register address to read
 *
 * @param[in] len
 *    The number of registers to read
 *
 * @param[out] result
 *    The data read from the device
 *
 * @return
 *    Returns zero on OK, non-zero otherwise
 ******************************************************************************/
int ioexp_read_regs(uint8_t addr, uint8_t len, uint8_t *value)
{
  I2C_TransferSeq_TypeDef seq;
  I2C_TransferReturn_TypeDef ret;
  int status;

  seq.addr        = i2c_dest_addr;
  seq.flags       = I2C_FLAG_WRITE_READ;
  seq.buf[0].len  = 1;
  seq.buf[0].data = &addr;
  seq.buf[1].len  = len;
  seq.buf[1].data = value;

  ret = I2CSPM_Transfer(I2C0, &seq);

  if (ret == i2cTransferDone) {
    status = IOEXP_STATUS_OK;
  } else {
    status = IOEXP_STATUS_FAILURE;
  }

  return status;
}

/***************************************************************************//**
 * @brief
 *    Performs register write through the I2C bus to the IO expander
 *
 * @param[in] addr
 *    The register address to write
 *
 * @param[in] len
 *    The number of registers to read
 *
 * @param[in] value
 *    The data to write to the register
 *
 * @return
 *    Returns zero on OK, non-zero otherwise
 ******************************************************************************/
int ioexp_write_regs(uint8_t addr, uint8_t len, uint8_t *value)
{
  I2C_TransferSeq_TypeDef seq;
  I2C_TransferReturn_TypeDef ret;
  int status;

  seq.addr        = i2c_dest_addr;
  seq.flags       = I2C_FLAG_WRITE_WRITE;
  seq.buf[0].len  = 1;
  seq.buf[0].data = &addr;
  seq.buf[1].len  = len;
  seq.buf[1].data = value;

  ret = I2CSPM_Transfer(I2C0, &seq);

  if (ret == i2cTransferDone) {
    status = IOEXP_STATUS_OK;
  } else {
    status = IOEXP_STATUS_FAILURE;
  }

  return status;
}

/***************************************************************************//**
 * @brief
 *    Get current i2c slave address.
 ******************************************************************************/
uint16_t ioexp_get_i2c_addr(void)
{
  return i2c_dest_addr;
}

/***************************************************************************//**
 * @brief
 *    Set i2c slave address.
 ******************************************************************************/
int ioexp_set_i2c_addr(uint16_t addr)
{
  i2c_dest_addr = addr;

  return IOEXP_STATUS_OK;
}

