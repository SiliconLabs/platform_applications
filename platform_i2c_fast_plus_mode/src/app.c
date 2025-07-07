/***************************************************************************//**
 * @file
 * @brief Top level application functions
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
#include <stdio.h>
#include "em_i2c.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "sl_simple_button.h"

// Defines
#define I2C_FOLLOWER_ADDRESS              0xE2
#define I2C_TXBUFFER_SIZE                 10
#define I2C_RXBUFFER_SIZE                 10

#define LED0_PORT                         gpioPortA
#define LED0_PIN                          4

#define LED1_PORT                         gpioPortA
#define LED1_PIN                          7

#define SCL_PORT                          gpioPortB
#define SCL_PIN                           4

#define SDA_PORT                          gpioPortB
#define SDA_PIN                           5

// Buffers
uint8_t i2c_txBuffer[I2C_TXBUFFER_SIZE];
uint8_t i2c_rxBuffer[I2C_RXBUFFER_SIZE];

// Transmission flags
volatile bool i2c_startTx;

/***************************************************************************//**
 * @brief Enable clocks
 ******************************************************************************/
void initCMU(void)
{
  // Enable clocks to the I2C and GPIO
  CMU_ClockEnable(cmuClock_I2C0, true);
  CMU_ClockEnable(cmuClock_GPIO, true);
}

/***************************************************************************//**
 * @brief GPIO initialization
 ******************************************************************************/
void initGPIO(void)
{
  // Configure LED0 and LED1 as output
  GPIO_PinModeSet(LED0_PORT, LED0_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(LED1_PORT, LED1_PIN, gpioModePushPull, 0);

  // Using PB5 (SDA) and PB4 (SCL)
  GPIO_PinModeSet(SCL_PORT, SCL_PIN, gpioModeWiredAndPullUpFilter, 1);
  GPIO_PinModeSet(SDA_PORT, SDA_PIN, gpioModeWiredAndPullUpFilter, 1);
}

/***************************************************************************//**
 * @brief Setup I2C
 ******************************************************************************/
void initI2C(void)
{
  // Use default settings
  I2C_Init_TypeDef i2cInit = I2C_INIT_DEFAULT;

  i2cInit.freq = I2C_FREQ_FASTPLUS_MAX;

  // Route I2C pins to GPIO
  GPIO->I2CROUTE[0].SDAROUTE =
    (GPIO->I2CROUTE[0].SDAROUTE & ~_GPIO_I2C_SDAROUTE_MASK)
    | (SDA_PORT << _GPIO_I2C_SDAROUTE_PORT_SHIFT
       | (SDA_PIN << _GPIO_I2C_SDAROUTE_PIN_SHIFT));
  GPIO->I2CROUTE[0].SCLROUTE =
    (GPIO->I2CROUTE[0].SCLROUTE & ~_GPIO_I2C_SCLROUTE_MASK)
    | (SCL_PORT << _GPIO_I2C_SCLROUTE_PORT_SHIFT
       | (SCL_PIN << _GPIO_I2C_SCLROUTE_PIN_SHIFT));
  GPIO->I2CROUTE[0].ROUTEEN = GPIO_I2C_ROUTEEN_SDAPEN | GPIO_I2C_ROUTEEN_SCLPEN;

  // Initialize the I2C
  I2C_Init(I2C0, &i2cInit);

  // Set the status flags and index
  i2c_startTx = false;

  // Enable automatic STOP on NACK
  I2C0->CTRL = I2C_CTRL_AUTOSN;
}

/***************************************************************************//**
 * @brief I2C read numBytes from follower device starting at target address
 ******************************************************************************/
void I2C_LeaderRead(uint16_t followerAddress,
                    uint8_t targetAddress,
                    uint8_t *rxBuff,
                    uint8_t numBytes)
{
  // Transfer structure
  I2C_TransferSeq_TypeDef i2cTransfer;
  I2C_TransferReturn_TypeDef result;

  // Initialize I2C transfer
  i2cTransfer.addr = followerAddress;
  i2cTransfer.flags = I2C_FLAG_WRITE_READ; // must write target address before reading
  i2cTransfer.buf[0].data = &targetAddress;
  i2cTransfer.buf[0].len = 1;
  i2cTransfer.buf[1].data = rxBuff;
  i2cTransfer.buf[1].len = numBytes;

  result = I2C_TransferInit(I2C0, &i2cTransfer);

  // Send data
  while (result == i2cTransferInProgress) {
    result = I2C_Transfer(I2C0);
  }

  if (result != i2cTransferDone) {
    // LED1 ON and infinite while loop to indicate I2C transmission problem
    GPIO_PinOutSet(LED1_PORT, LED1_PIN);
    while (1) {}
  }
}

/***************************************************************************//**
 * @brief I2C write numBytes to follower device starting at target address
 ******************************************************************************/
void I2C_LeaderWrite(uint16_t followerAddress,
                     uint8_t targetAddress,
                     uint8_t *txBuff,
                     uint8_t numBytes)
{
  // Transfer structure
  I2C_TransferSeq_TypeDef i2cTransfer;
  I2C_TransferReturn_TypeDef result;
  uint8_t txBuffer[I2C_TXBUFFER_SIZE + 1];

  txBuffer[0] = targetAddress;
  for (int i = 0; i < numBytes; i++)
  {
    txBuffer[i + 1] = txBuff[i];
  }

  // Initialize I2C transfer
  i2cTransfer.addr = followerAddress;
  i2cTransfer.flags = I2C_FLAG_WRITE;
  i2cTransfer.buf[0].data = txBuffer;
  i2cTransfer.buf[0].len = numBytes + 1;
  i2cTransfer.buf[1].data = NULL;
  i2cTransfer.buf[1].len = 0;

  result = I2C_TransferInit(I2C0, &i2cTransfer);

  // Send data
  while (result == i2cTransferInProgress) {
    result = I2C_Transfer(I2C0);
  }

  if (result != i2cTransferDone) {
    // LED1 ON and infinite while loop to indicate I2C transmission problem
    GPIO_PinOutSet(LED1_PORT, LED1_PIN);
    while (1) {}
  }
}

/***************************************************************************//**
 * @brief I2C Read/Increment/Write/Verify
 ******************************************************************************/
bool testI2C(void)
{
  int i;
  bool I2CWriteVerify;

  // Initial read of bytes from follower
  I2C_LeaderRead(I2C_FOLLOWER_ADDRESS, 0, i2c_rxBuffer, I2C_RXBUFFER_SIZE);

  // Increment received values and prepare to write back to follower
  for (i = 0; i < I2C_RXBUFFER_SIZE; i++) {
    i2c_txBuffer[i] = i2c_rxBuffer[i] + 1;
  }

  // Block write new values to follower
  I2C_LeaderWrite(I2C_FOLLOWER_ADDRESS, 0, i2c_txBuffer, I2C_TXBUFFER_SIZE);

  // Block read from follower
  I2C_LeaderRead(I2C_FOLLOWER_ADDRESS, 0, i2c_rxBuffer, I2C_RXBUFFER_SIZE);

  // Verify I2C transmission
  I2CWriteVerify = true;
  for (i = 0; i < I2C_RXBUFFER_SIZE; i++) {
    if (i2c_txBuffer[i] != i2c_rxBuffer[i]) {
      I2CWriteVerify = false;
      break;
    }
  }

  return I2CWriteVerify;
}

/***************************************************************************//**
 * @brief GPIO Interrupt handler
 ******************************************************************************/
void sl_button_on_change(const sl_button_t *handle)
{
  (void)handle;

  I2C_Enable(I2C0, true);
  i2c_startTx = true;
}

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  initGPIO();
  initCMU();
  initI2C();
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
  if (i2c_startTx) {
    // Transmitting data
    if (testI2C() == false) {
      // Indicate error with LED1
      GPIO_PinOutSet(LED1_PORT, LED1_PIN);

      // Sit in infinite while loop
      while (1) {}
    } else {
      // Toggle LED0 with each pass
      GPIO_PinOutToggle(LED0_PORT, LED0_PIN);

      // Transmission complete
      i2c_startTx = false;
    }
  }
}
