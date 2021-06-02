/***************************************************************************//**
 * @file
 * @brief I2C slave mode driver
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

#include <stddef.h>
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_i2c.h"
#include "em_chip.h"
#include "em_device.h"
#include "em_assert.h"
#include "plugin/debug/btl_debug.h"
#include "btl_i2c_slave_driver.h"

static queue_t i2c_rxQueue;
static queue_t i2c_txQueue;
static volatile uint8_t i2c_currentOperation;
static volatile uint16_t i2c_targetAddress;
static rx_callback_t i2c_callback = NULL;

/**************************************************************************//**
 * @brief  enables I2C slave interrupts
 *****************************************************************************/
static void enableI2cSlaveInterrupts()
{
  I2C_IntClear(BTL_DRIVER_I2C_PORT,
               I2C_IF_ADDR
               | I2C_IF_RSTART
               | I2C_IF_RXDATAV
               | I2C_IF_ACK
               | I2C_IF_SSTOP);
  I2C_IntEnable(BTL_DRIVER_I2C_PORT,
                I2C_IEN_ADDR
                | I2C_IEN_RSTART
                | I2C_IEN_RXDATAV
                | I2C_IEN_ACK
                | I2C_IEN_SSTOP);
  NVIC_EnableIRQ(BTL_DRIVER_I2C_IRQ);
}

/**************************************************************************//**
 * @brief  disables I2C interrupts
 *****************************************************************************/
static void disableI2cInterrupts()
{
  NVIC_DisableIRQ(BTL_DRIVER_I2C_IRQ);
  I2C_IntDisable(BTL_DRIVER_I2C_PORT,
                 I2C_IEN_ADDR
                 | I2C_IEN_RSTART
                 | I2C_IEN_RXDATAV
                 | I2C_IEN_ACK
                 | I2C_IEN_SSTOP);
  I2C_IntClear(BTL_DRIVER_I2C_PORT,
               I2C_IF_ADDR
               | I2C_IF_RSTART
               | I2C_IF_RXDATAV
               | I2C_IF_ACK
               | I2C_IF_SSTOP);
}

/**************************************************************************//**
 * @brief  initializes CMU for I2C port
 *****************************************************************************/
static void initializeCMU()
{
#if defined(_CMU_HFPERCLKEN0_MASK)
  CMU_ClockEnable(cmuClock_HFPER, true);
#endif
  CMU_ClockEnable(cmuClock_GPIO, true);
  /* Select I2C peripheral clock */
  CMU_ClockEnable(BTL_DRIVER_I2C_CLOCK, true);
}

/**************************************************************************//**
 * @brief  initializes GPIO for I2C port
 *****************************************************************************/
static void initializeGPIO()
{

  /* Output value must be set to 1 to not drive lines low. Set
     SCL first, to ensure it is high before changing SDA. */
  GPIO_PinModeSet(BTL_DRIVER_SCL_PORT, BTL_DRIVER_SCL_PIN,
                  gpioModeWiredAndPullUp, 1);
  GPIO_PinModeSet(BTL_DRIVER_SDA_PORT, BTL_DRIVER_SDA_PIN,
                  gpioModeWiredAndPullUp, 1);

  /* In some situations, after a reset during an I2C transfer, the slave
     device may be left in an unknown state. Send 9 clock pulses to
     set slave in a defined state. */
  for (int i = 0; i < 9; i++) {
    GPIO_PinOutSet(BTL_DRIVER_SCL_PORT, BTL_DRIVER_SCL_PIN);
    GPIO_PinOutClear(BTL_DRIVER_SDA_PORT, BTL_DRIVER_SDA_PIN);
  }

  /* Enable pins and set location */
    GPIO->I2CROUTE[BTL_DRIVER_I2C_NUM].ROUTEEN =
      GPIO_I2C_ROUTEEN_SDAPEN | GPIO_I2C_ROUTEEN_SCLPEN;
    GPIO->I2CROUTE[BTL_DRIVER_I2C_NUM].SCLROUTE =
      (BTL_DRIVER_SCL_PIN << _GPIO_I2C_SCLROUTE_PIN_SHIFT)
      | (BTL_DRIVER_SCL_PORT << _GPIO_I2C_SCLROUTE_PORT_SHIFT);
    GPIO->I2CROUTE[BTL_DRIVER_I2C_NUM].SDAROUTE =
      (BTL_DRIVER_SDA_PIN << _GPIO_I2C_SDAROUTE_PIN_SHIFT)
      | (BTL_DRIVER_SDA_PORT << _GPIO_I2C_SDAROUTE_PORT_SHIFT);
}

/*******************************************************************************
 * @brief Initalize I2C slave address
 ******************************************************************************/
void I2C_setAddress( uint8_t i2cAddress, uint8_t i2cAddressMask)
{
  I2C_SlaveAddressSet(BTL_DRIVER_I2C_PORT, (i2cAddress & 0x7f) <<1);
  I2C_SlaveAddressMaskSet(BTL_DRIVER_I2C_PORT, (i2cAddressMask & 0x7F) <<1);
}

/*******************************************************************************
 * @brief Initalize I2C peripheral as slave
 ******************************************************************************/
void I2C_slave_init()
{
  I2C_Init_TypeDef i2cInit;
  i2c_callback = NULL;
  initializeCMU();
  initializeGPIO();
  queue_init(&i2c_rxQueue);
  queue_init(&i2c_txQueue);

  /* Set emlib init parameters */
  i2cInit.enable = true;
  i2cInit.master = false;
  i2cInit.freq = I2C_FREQ_FAST_MAX;
  i2cInit.refFreq = 0;
  i2cInit.clhr = i2cClockHLRAsymetric;
  I2C_setAddress(BTL_DRIVER_I2C_ADDRESS, 0xFF);
  I2C_Init(BTL_DRIVER_I2C_PORT, &i2cInit);
  enableI2cSlaveInterrupts();

}

/*******************************************************************************
 * @brief Initalize I2C peripheral as slave
 ******************************************************************************/
void I2C_slave_shutdown(void)
{
  disableI2cInterrupts();
  I2C_Reset(BTL_DRIVER_I2C_PORT);
}
/**************************************************************************//**
 * @brief I2C Interrupt Handler.
 *****************************************************************************/
void BTL_DRIVER_IRQ_HANDLER()
{
  uint32_t pending;
  uint32_t rxData;
  uint16_t txData;
  pending = BTL_DRIVER_I2C_PORT->IF;

  /* If some sort of fault, abort transfer. */
  if (pending & (I2C_IF_BUSERR | I2C_IF_ARBLOST)) {
    i2c_currentOperation = I2C_OPERATION_NONE;
  } else if (pending & I2C_IF_ADDR) {
    rxData = BTL_DRIVER_I2C_PORT->RXDATA;
    i2c_currentOperation = rxData & 0x1;
    if(pending & I2C_IF_RSTART) {
      if (i2c_targetAddress != ((rxData >> 1) & 0x7F)
          || i2c_currentOperation != I2C_OPERATION_READ) {
        BTL_DRIVER_I2C_PORT->CMD = I2C_CMD_NACK;
      } else {
        if (i2c_callback && !queue_is_empty(&i2c_rxQueue)) {
            i2c_callback(&i2c_rxQueue, &i2c_txQueue);
          if ((txData = queue_pop(&i2c_txQueue)) != QUEUE_EOF) {
            BTL_DRIVER_I2C_PORT->TXDATA = txData & 0xFF;
            BTL_DRIVER_I2C_PORT->CMD = I2C_CMD_ACK;
          } else {
            BTL_DRIVER_I2C_PORT->CMD = I2C_CMD_NACK;
          }
        } else {
          BTL_DRIVER_I2C_PORT->CMD = I2C_CMD_NACK;
        }
      }
      I2C_IntClear(BTL_DRIVER_I2C_PORT, I2C_IF_RSTART);
    } else {
        i2c_targetAddress = (rxData >> 1) & 0x7F;
        if (i2c_currentOperation == I2C_OPERATION_READ) {
          if ((txData = queue_pop(&i2c_txQueue)) != QUEUE_EOF) {
            BTL_DRIVER_I2C_PORT->TXDATA = txData & 0xFF;
            BTL_DRIVER_I2C_PORT->CMD = I2C_CMD_ACK;
          } else {
            BTL_DRIVER_I2C_PORT->CMD = I2C_CMD_NACK;
          }
        } else {
          BTL_DRIVER_I2C_PORT->CMD = I2C_CMD_ACK;
        }
    }
    I2C_IntClear(BTL_DRIVER_I2C_PORT, I2C_IF_ADDR | I2C_IF_RXDATAV);
  } else if (pending & I2C_IF_RXDATAV) {
    rxData = BTL_DRIVER_I2C_PORT->RXDATA;
    if(i2c_currentOperation == I2C_OPERATION_WRITE
       && (i2c_callback &&
           queue_push(&i2c_rxQueue, rxData & 0xFF) == QUEUE_OK)) {
      BTL_DRIVER_I2C_PORT->CMD = I2C_CMD_ACK;
    } else {
      BTL_DRIVER_I2C_PORT->CMD = I2C_CMD_NACK;
    }
    I2C_IntClear(BTL_DRIVER_I2C_PORT, I2C_IF_RXDATAV);
  } else if(pending & I2C_IF_ACK) {
    /******************************************************/
    /* Master ACK'ed, so requesting more data.            */
    /******************************************************/
    if((txData = queue_pop(&i2c_txQueue)) != QUEUE_EOF)
    {
      // transfer data
      BTL_DRIVER_I2C_PORT->TXDATA     = txData & 0xFF;
    } else {
      // there is no sending data; transfer data as if slave non-responsive
      BTL_DRIVER_I2C_PORT->TXDATA     = 0xFF;
    }
    I2C_IntClear(BTL_DRIVER_I2C_PORT, I2C_IF_ACK);
  } else if(pending & I2C_IF_SSTOP) {
    // end of transaction
    if (i2c_callback && !queue_is_empty(&i2c_rxQueue)) {
      i2c_callback(&i2c_rxQueue, &i2c_txQueue);
    }
    i2c_currentOperation = I2C_OPERATION_NONE;
    I2C_IntClear(BTL_DRIVER_I2C_PORT, I2C_IF_SSTOP);
  }
}

void I2C_set_transaction_callback(const rx_callback_t callback)
{
  i2c_callback = callback;
}
