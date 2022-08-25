/***************************************************************************//**
 * @file dac70501_dac.c
 * @brief dac70501 i2c driver.
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
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
 * # Evaluation Quality
 * This code has been minimally tested to ensure that it builds and is suitable
 * as a demonstration for evaluation purposes only. This code will be maintained
 * at the sole discretion of Silicon Labs.
 ******************************************************************************/

#include <stdio.h>
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_i2c.h"
#include "dac70501_dac.h"
#include "ads1220_adc.h"
#include "efr32bg22_adc.h"

// i2c read protocol description:
// |MSB  ...  LSB| ACK |MSB  ...  LSB| ACK |MSB ... LSB| ACK |MSB ... LSB| ACK
// Address byte         Command byte           MSDB              LSDB
// DB [32:24]            DB [23:16]          DB [15:8]          DB [7:0]
// slaveAddress         targetAddress,       rxBuff[0],         rxBuff[1]

// i2c write protocol description:
// |MSB  ...  LSB| ACK |MSB  ...  LSB| ACK |MSB ... LSB| ACK |MSB ... LSB| ACK
// Address byte         Command byte           MSDB              LSDB
// DB [32:24]            DB [23:16]          DB [15:8]          DB [7:0]
// slaveAddress         targetAddress,       rxBuff[0],         rxBuff[0]

// TX and RX Buffers size
#define I2C_TXBUFFER_SIZE                  10
#define I2C_RXBUFFER_SIZE                  10
// TX and RX buffers
uint8_t i2c_tx_buffer[I2C_TXBUFFER_SIZE];
uint8_t i2c_rx_buffer[I2C_RXBUFFER_SIZE];

// Ports and pins for I2C interface (I2C0)
#define I2C0_CLK_PORT                      gpioPortB
#define I2C0_CLK_PIN                       1
#define I2C0_SDA_PORT                      gpioPortB
#define I2C0_SDA_PIN                       2

// DAC70501 registers
#define I2C_SLAVE_ADDRESS                  0x90U  // Slave address
#define DAC70501_REG_NOOP                  0x00U  // No operation
#define DAC70501_REG_DEVICE_ID             0x01U  // ID of DAC
#define DAC70501_REG_SYNC_EN               0x02U  // Sync mode
#define DAC70501_REG_CONFIG                0x03U  // Reference
#define DAC70501_REG_GAIN                  0x04U  // DIV and gain
#define DAC70501_REG_TRIGGER               0x05U  // Soft reset
#define DAC70501_REG_STATUS                0x07U  // Status
#define DAC70501_REG_DAC0                  0x08U  // DAC data

// Register masks
#define DAC70501_MASK_DEVICE_ID_8CH        0x7080
#define DAC70501_MASK_SYNC_EN              0x1
#define DAC70501_MASK_CONFIG_REF_PWDWN     0x101
#define DAC70501_MASK_GAIN_BUFF_GAIN       0x1
#define DAC70501_MASK_GAIN_REFDIV_EN       0x100
#define DAC70501_MASK_TRIGGER_SYNC         0x10
#define DAC70501_MASK_TRIGGER_SOFT_RESET   0xF
#define DAC70501_MASK_STATUS_REF_ALM       0x1

// I2C initialized
float dac_Volt;
uint8_t initialized = 0;

/**************************************************************************//**
 * @brief
 *    I2C0 initialization
 * @param[in]
 *    none
 * @return
 *    none
 *****************************************************************************/
void dac70501_init_i2c0(void)
{
  // Using default settings
  I2C_Init_TypeDef i2c_init = I2C_INIT_DEFAULT;

  // Enabling clock to the GPIO and I2C0
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(cmuClock_I2C0, true);

  // Using PB1 (SCL) and PB2 (SDA)
  GPIO_PinModeSet(I2C0_CLK_PORT, I2C0_CLK_PIN, 
                  gpioModeWiredAndPullUpFilter, 1);
  GPIO_PinModeSet(I2C0_SDA_PORT, I2C0_SDA_PIN, 
                  gpioModeWiredAndPullUpFilter, 1);

  // Route GPIO pins to I2C0 module
  GPIO->I2CROUTE[0].SDAROUTE =                                \
    (GPIO->I2CROUTE[0].SDAROUTE & ~_GPIO_I2C_SDAROUTE_MASK)   \
    | (I2C0_SDA_PORT << _GPIO_I2C_SDAROUTE_PORT_SHIFT         \
    | (I2C0_SDA_PIN << _GPIO_I2C_SDAROUTE_PIN_SHIFT));
  GPIO->I2CROUTE[0].SCLROUTE =                                \
    (GPIO->I2CROUTE[0].SCLROUTE & ~_GPIO_I2C_SCLROUTE_MASK)   \
    | (I2C0_CLK_PORT << _GPIO_I2C_SCLROUTE_PORT_SHIFT         \
    | (I2C0_CLK_PIN << _GPIO_I2C_SCLROUTE_PIN_SHIFT));
  GPIO->I2CROUTE[0].ROUTEEN =                                 \
    GPIO_I2C_ROUTEEN_SDAPEN | GPIO_I2C_ROUTEEN_SCLPEN;      

  // Initializing the I2C
  I2C_Init(I2C0, &i2c_init);
  I2C0->CTRL = I2C_CTRL_AUTOSN;
}

/***************************************************************************//**
 * @brief
 *    I2C read numBytes starting at target address (I2C_FLAG_WRITE_READ)
 * @param[in]
 *    slaveAddress:  slave address
 *    targetAddress: register address need to read
 *    rxBuff:        buffer to save register (read-back) value
 *    numBytes:      number of bytes to read
 * @return
 *    none
 ******************************************************************************/
void dac70501_reg_read(uint16_t slaveAddress, uint8_t targetAddress,
                       uint8_t *rxBuff, uint8_t numBytes)
{
  // Transfer structure
  I2C_TransferSeq_TypeDef i2c_Transfer;
  I2C_TransferReturn_TypeDef result;

  // Initializing I2C transfer
  // I2C slave device address
  i2c_Transfer.addr = slaveAddress;
  // use write+read to read
  i2c_Transfer.flags = I2C_FLAG_WRITE_READ;
  // must write target (register) address before reading
  i2c_Transfer.buf[0].data = &targetAddress;
  // write slave address + register address
  i2c_Transfer.buf[0].len = 1;
  // read back numBytes bytes
  i2c_Transfer.buf[1].data = rxBuff;
  i2c_Transfer.buf[1].len = numBytes;

  result = I2C_TransferInit(I2C0, &i2c_Transfer);

  // Sending data
  while (result == i2cTransferInProgress) {
    result = I2C_Transfer(I2C0);
  }

  if (result != i2cTransferDone) {
    while (1) {}
  }
}

/**************************************************************************//**
 * @brief
 * I2C write numBytes starting at target address (I2C_FLAG_WRITE)
 * @param[in]
 *    slaveAddress:  slave address
 *    targetAddress: register address need to write
 *    txBuff:        data to be written
 *    numBytes:      number of data to be written
 * @return
 *    none
 *****************************************************************************/
void dac70501_reg_write(uint16_t slaveAddress, uint8_t targetAddress,
                        uint8_t *txBuff, uint8_t numBytes)
{
  // Transfer structure
  I2C_TransferSeq_TypeDef i2c_Transfer;
  I2C_TransferReturn_TypeDef result;

  // Local buffer
  uint8_t tx_buffer_local[I2C_TXBUFFER_SIZE + 1];
  // copy local write data
  tx_buffer_local[0] = targetAddress;
  for (int i = 0; i < numBytes; i++) {
    tx_buffer_local[i + 1] = txBuff[i];
  }

  // Initializing I2C transfer
  i2c_Transfer.addr = slaveAddress;
  // use write flag to write
  i2c_Transfer.flags = I2C_FLAG_WRITE;
  // must write target (register) address
  i2c_Transfer.buf[0].data = tx_buffer_local;
  // write slave address + register address + data
  i2c_Transfer.buf[0].len = numBytes + 1;
  // don't need to read
  i2c_Transfer.buf[1].data = NULL;
  i2c_Transfer.buf[1].len = 0;

  result = I2C_TransferInit(I2C0, &i2c_Transfer);

  // Sending data
  while (result == i2cTransferInProgress) {
    result = I2C_Transfer(I2C0);
  }

  if (result != i2cTransferDone) {
    while (1) {}
  }
}

/**************************************************************************//**
 * @brief
 *    read and confirm DAC ID
 * @param[in]
 *    none
 * @return
 *    devID, expect 0x1195
 *****************************************************************************/
uint16_t dac70501_read_id(void)
{
  // reserved   resolution    reserved         rstsel            reserved
  // 15         14:12         11:8             7                 6:0
  // b0         b001          b0001            b1                b0010101
  dac70501_reg_read(I2C_SLAVE_ADDRESS, 
                    DAC70501_REG_DEVICE_ID, i2c_rx_buffer, 2);

  // expect 0x1195
  return i2c_rx_buffer[1] | (i2c_rx_buffer[0] << 8);
}

/**************************************************************************//**
 * @brief
 *    check ID
 * @param[in]
 *    none
 * @return
 *    1, if pass
 *    stay in while loop, not pass
******************************************************************************/
uint16_t dacx070501_check_id(uint16_t devID)
{
  if (devID == 0x1195) {
    return 1;
  } else {
    while (1) {}
  }
}

/**************************************************************************//**
 * @brief
 *    set DAC sync
 * @param[in]
 *    syncMode, valid value is 0 or 1.
 * @return
 *    1
******************************************************************************/
uint16_t dac70501_sync_mode(uint8_t syncMode)
{
  // reserved        DAC_SYNC_EN
  // 15~1            0
  // 0x0             0x1
  i2c_tx_buffer[0] = 0x0;
  // synchronous or immediately mode
  // refer to trigger register
  i2c_tx_buffer[1] = syncMode;

  // use default immediately mode
  dac70501_reg_write(I2C_SLAVE_ADDRESS,
                     DAC70501_REG_SYNC_EN, i2c_tx_buffer, 2);
  return 1;
}

/**************************************************************************//**
 * @brief
 *    power down DAC
 * @param[in]
 *    dac_pwdwn - dac, 0 or 1
 *    ref_pwdwn - ref, 0 or 1
 * @return
 *    1
******************************************************************************/
uint16_t dac070501_power_down(uint8_t dac_pwdwn, uint8_t ref_pwdwn)
{
  // reserved        REF_PWDWN      reserved       DAC_PWDWN
  // 15~9            8              7:1            0
  // 0x0             b0             0x0            b0

  // power down dac and (or) internal ref
  i2c_tx_buffer[0] = ref_pwdwn;
  i2c_tx_buffer[1] = dac_pwdwn;

  // enable both dac and internal ref (default)
  dac70501_reg_write(I2C_SLAVE_ADDRESS, DAC70501_REG_CONFIG, i2c_tx_buffer, 2);
  return 1;
}

/**************************************************************************//**
 * @brief
 *    set DAC gain and div
 * @param[in]
 *    gain: gain set, 0 or 1
 *    div: divider set, 1 or 1
 * @return
 *    1
 *****************************************************************************/
uint16_t dac70501_set_gain(uint8_t gain, uint8_t div)
{
  // reserved        REF-DIV        reserved       BUFF-GAIN
  // 15~9            8              7:1            0
  // 0x0             b0             0x0            b0

  // reference divider
  //   0 - the reference voltage is unaffected
  //   1 - reference voltage is internally divided by a factor of 2.
  i2c_tx_buffer[0] = div;

  // buffer gain:
  //   0 - the buffer amplifier for corresponding DAC has a gain of 1.
  //   1 - the buffer amplifier for corresponding DAC has a gain of 2.
  i2c_tx_buffer[1] = gain;

  dac70501_reg_write(I2C_SLAVE_ADDRESS, DAC70501_REG_GAIN, i2c_tx_buffer, 2);

  return 1;
}

/**************************************************************************//**
 * @brief
 *    soft reset DAC
 * @param[in]
 *    ldacMode:
 *      0 -
 *      1 -
 * @return
 *    self resetting
******************************************************************************/
uint16_t dac70501_update_ldac(uint8_t ldacMode)
{
  // reserved        LDAC   SOFT-RESET
  // 15~5            4         3:0
  // 0x0             bx        0xa

  // refer to register sync
  i2c_tx_buffer[0] = 0x0;
  i2c_tx_buffer[1] = ldacMode << 4;

  dac70501_reg_write(I2C_SLAVE_ADDRESS, 
                     DAC70501_REG_TRIGGER, i2c_tx_buffer, 2);

  return 1;
}

/**************************************************************************//**
 * @brief
 *    soft reset DAC
 * @param[in]
 *    none
 * @return
 *    1
******************************************************************************/
uint16_t dac70501_reset_soft(void)
{
  // reserved        LDAC   SOFT-RESET
  // 15~5            4         3:0
  // 0x0             bx        0xa

  // soft reset command
  i2c_tx_buffer[0] = 0x0;
  i2c_tx_buffer[1] = 0xa;

  dac70501_reg_write(I2C_SLAVE_ADDRESS, 
                     DAC70501_REG_TRIGGER, i2c_tx_buffer, 2);
  letimer_delay(1000);
  return 1;
}

/**************************************************************************//**
 * @brief
 *    read DAC status
 * @param[in]
 *    none
 * @return
 *    status, expect 0x0000
 *****************************************************************************/
uint16_t dac70501_read_status(void)
{
  // reserved REF-ALARM
  // 15~1      0
  // 0x0       b0

  dac70501_reg_read(I2C_SLAVE_ADDRESS, DAC70501_REG_STATUS, i2c_rx_buffer, 2);

  // expect 0x0000
  return i2c_rx_buffer[1] | (i2c_rx_buffer[0] << 8);
}

/**************************************************************************//**
 * @brief
 *    read back gain
 * @param[in]
 *    none
 * @return
 *    div and gain set
 *****************************************************************************/
uint16_t dac70501_read_gain(void)
{
  // reserved      REF-DIV       reserved   BUFF-GAIN
  // 15~9          8                 7:1         b0
  // 0x1           b1                0x0         b0
  dac70501_reg_read(I2C_SLAVE_ADDRESS, DAC70501_REG_GAIN, i2c_rx_buffer, 2);

  // expect 0x0001
  return i2c_rx_buffer[1] | (i2c_rx_buffer[0] << 8);
}

/**************************************************************************//**
 * @brief
 *    get DAC vref output voltage
 * @param[in]
 *    none
 * @return
 *    return output voltage in volt unit
 *****************************************************************************/
float dac70501_read_ref(void)
{
  float volt_value;
  uint16_t reg_value;
  // DAC-DATA high    DAC-DATA low    reserved
  // 15:8             7:2             1:0
  // dacValueHigh     dacValueLow     b00
  dac70501_reg_read(I2C_SLAVE_ADDRESS, DAC70501_REG_DAC0, i2c_rx_buffer, 2);

  // convert to voltage in mV unit
  reg_value = (i2c_rx_buffer[0]) << 8 | (i2c_rx_buffer[1] & 0xfc);
  volt_value = 1.25 * reg_value / 4 / 16383;

  return volt_value;
}

/**************************************************************************//**
 * @brief
 *    set DAC vref output registers
 * @param[in]
 *    dacValueHigh - 15:8
 *    dacValueLow  - 7:2
 *    Vout = data / 16384 * vref / div * gain
 * @return
 *    1
 *****************************************************************************/
uint16_t dac70501_set_ref(uint8_t dacValueHigh, uint8_t dacValueLow)
{
  // DAC-DATA high    DAC-DATA low    reserved
  // 15:8             7:2             1:0
  // dacValueHigh     dacValueLow     b00
  i2c_tx_buffer[0] = dacValueHigh;
  i2c_tx_buffer[1] = dacValueLow;

  dac70501_reg_write(I2C_SLAVE_ADDRESS, DAC70501_REG_DAC0, i2c_tx_buffer, 2);

  return 1;
}

/**************************************************************************//**
 * @brief
 *    set dac output in volt unit
 * @param[in]
 *    voltValue: target voltage in volt unit
 * @return
 *    1
******************************************************************************/
uint16_t dac70501_set_volt(float voltValue)
{
  uint16_t dac_reg_val;
  uint8_t dac_high, dac_low;

  if (voltValue > 1.25) {
    voltValue = 1.25;
  }
  if (voltValue < 0.0) {
    voltValue = 0.0;
  }

  // calculate registers value based on target voltage
  dac_reg_val = (uint16_t)(voltValue / 1.25f * 4.0f * 16383.0f);

  // separate as high and low registers DAC value
  dac_high = (uint8_t)((dac_reg_val & 0xff00) >> 8);
  dac_low = (uint8_t)dac_reg_val & 0xff;

  // write to DAC register
  dac70501_set_ref(dac_high, dac_low);

  return 1;
}

/**************************************************************************//**
 * @brief
 *    dac70501 initialization
 * @param[in]
 *    voltage set
 * @return
 *    dac70501 status register value
 * @comment
 *    Vout = data/16384 * vref/div * gain
******************************************************************************/
uint16_t dac70501_init(void)
{
  uint16_t dev_ID, status;

  // init I2C
  dac70501_init_i2c0();

  // soft reset
  dac70501_reset_soft();
  letimer_delay(1000);
  // read and confirm id
  dev_ID = dac70501_read_id();
  dacx070501_check_id(dev_ID);

  // gain = 0, div = 1
  // this get 1.25v full range
  dac70501_set_gain(0, 1);

  // set in sync mode and then read output voltage,
  // Vout = data / 16384 * vref / div * gain
  // sync mode
  dac70501_sync_mode(1);
  dac_Volt = 0.625f;
  dac70501_set_volt(dac_Volt);
  // update DAC register
  dac70501_update_ldac(1);
  // immediately mode
  dac70501_sync_mode(0);
  dac_Volt = dac70501_read_ref();

  // set output voltage in unit volt
  dac_Volt = 1.20f;
  dac70501_set_volt(dac_Volt);

  // read status
  status = dac70501_read_status();

  return status;
}

uint16_t dac70501_restart(void)
{
  uint16_t status;

  // soft reset
  dac70501_reset_soft();
  letimer_delay(1000);
  // gain = 0, div = 1
  // this get 1.25v full range
  dac70501_set_gain(0, 1);

  // set output voltage in unit volt
  dac_Volt = 1.20f;
  dac70501_set_volt(dac_Volt);

  // read status
  status = dac70501_read_status();

  return status;
}
