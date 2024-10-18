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
#include <string.h>
#include "em_chip.h"
#include "em_device.h"
#include "em_cmu.h"
#include "em_i2c.h"
#include "em_gpio.h"

// I2C pins (SCL = PC5/EXP15; SDA = PC7/EXP16)
// PD03 is the I2C domain that enables pull ups on the board
#define I2C_SCL_PORT                    gpioPortC
#define I2C_SCL_PIN                     5
#define I2C_SDA_PORT                    gpioPortC
#define I2C_SDA_PIN                     7
#define I2C_DOMAIN_POWER_PORT           gpioPortD
#define I2C_DOMAIN_POWER_PIN            3

// push buttons
// PB0 - READ, PB1 - WRITE
#define PUSH_BTN0_PORT                  gpioPortB
#define PUSH_BTN0_PIN                   1
#define PUSH_BTN1_PORT                  gpioPortB
#define PUSH_BTN1_PIN                   3

// Address of the I2C follower device (left-shifted to bits [7:1])
#define I2C_FOLLOWER_ADDRESS            0xE2

// Read/write bit/mask for the I2C follower device address
#define I2C_RNOTW_BIT                   0x01
#define I2C_RNOTW_MASK                  0xFE

// I2C interrupt group
#define I2C_LEADER_INTERRUPT            I2C_IEN_START | I2C_IEN_ACK \
  | I2C_IEN_NACK |                                                  \
  I2C_IEN_RXDATAV | I2C_IEN_MSTOP

#define I2C_ERROR_INTERRUPT             I2C_IEN_SDAERR | I2C_IEN_SCLERR\
  | I2C_IEN_CLERR | I2C_IEN_BUSERR

#define I2C_BUFFER_SIZE                 10
#define I2C_WRITE_TARGET                0  // offset of the target (follower)
                                           // array to written to

#define DEBUG_I2C                       0

// get I2C ready for a transfer
void i2cTransferReset(uint32_t transferType);

// state machine enum
enum i2c_state_machine {
  I2C_IDLE_STATE = 0,
  I2C_START_STATE = 0x1,
  I2C_ACK_TRANSMISSION_STATE = 0x2,
  I2C_STOP_STATE = 0x5,
  I2C_NACK_STATE = 0x6,
  I2C_ERR_STATE = 0x7
};

// transfer type enum
enum i2c_transfer_type {
  I2C_TRANSFER_READ,
  I2C_TRANSFER_WRITE
};

uint8_t receiveBuffer[I2C_BUFFER_SIZE];
uint8_t transmitBuffer[I2C_BUFFER_SIZE];
uint8_t targetAddressSent;  // flag check for offset sent to follower
uint32_t i2c_state;  // i2c state machine
uint32_t i2c_transfer;  // i2c transfer type
uint32_t receiveIndex;
uint32_t transmitIndex;
uint8_t i2c_startTransfer;

/***************************************************************************//**
 * GPIO odd pins interrupt handler
 ******************************************************************************/
void GPIO_ODD_IRQHandler(void)
{
  uint32_t flags = GPIO_IntGet();
  GPIO_IntClear(flags);

  if (i2c_startTransfer == true) {
    // middle of transfer, ignore request
    return;
  }

  if (flags & GPIO_IF_EXTIF1) {
    // push button 0 is triggered
    i2cTransferReset(I2C_TRANSFER_READ);
    GPIO_PinOutSet(gpioPortB, 2);
    return;
  }

  if (flags & GPIO_IF_EXTIF3) {
    // push button 1 is triggered
    i2cTransferReset(I2C_TRANSFER_WRITE);
    GPIO_PinOutSet(gpioPortB, 4);
    return;
  }
}

/***************************************************************************//**
 * I2C0 interrupt handler
 ******************************************************************************/
void I2C0_IRQHandler(void)
{
  uint32_t flags = I2C_IntGet(I2C0);
  I2C_IntClear(I2C0, flags);

  // if in the middle of a transfer
  if (!i2c_startTransfer) {
    return;
  }

  // stop condition reached
  if (flags & I2C_IF_MSTOP) {
    i2c_state = I2C_STOP_STATE;
    i2c_startTransfer = false;  // transfer done
    return;
  }

  if (flags & (I2C_ERROR_INTERRUPT)) {
    i2c_state = I2C_ERR_STATE;
    return;
  }

  if (flags & I2C_IF_NACK) {
    // transmission NACKed, stop transmission
    I2C0 -> CMD_SET = I2C_CMD_STOP;
    return;
  }

  // I2C state machine
  switch (i2c_state) {
    case I2C_IDLE_STATE:
      // should expect start condition
      // All condition will start with a write request to set target address
      if (flags & I2C_IF_START) {
        if ((i2c_transfer == I2C_TRANSFER_READ) & targetAddressSent) {
          // target address sent already, issue a read request
          I2C0 -> TXDATA = (I2C_FOLLOWER_ADDRESS | I2C_RNOTW_BIT);
        } else {
            // write target address and write request
            I2C0 -> TXDATA = I2C_FOLLOWER_ADDRESS;
        }
        i2c_state = I2C_START_STATE;
      }
      break;

    case I2C_START_STATE:
      // should expect a ACK or NACK from follower
      if (flags & I2C_IF_ACK) {
        i2c_state = I2C_ACK_TRANSMISSION_STATE;
        // Address recognized, check RXDATAV if read
        if ((i2c_transfer == I2C_TRANSFER_READ) & (flags & I2C_IF_RXDATAV) & targetAddressSent) {
          // data valid, read from RXDATA
          receiveBuffer[receiveIndex++] = I2C0 -> RXDATA;
          I2C_IntClear(I2C0, I2C_IF_RXDATAV);
          if (receiveIndex >= I2C_BUFFER_SIZE) {
                // end transmission
                I2C0 -> CMD_SET = I2C_CMD_STOP | I2C_CMD_NACK;
          } else {
                 I2C0 -> CMD_SET = I2C_CMD_ACK;
          }
        }
        else if((i2c_transfer == I2C_TRANSFER_WRITE) | !targetAddressSent) {
            // first send the offset to write to
            I2C0 -> TXDATA = (uint8_t)I2C_WRITE_TARGET;
        }
      }
      break;

    case I2C_ACK_TRANSMISSION_STATE:
      // if read, RXDATA should be valid
      // if write, should expect an ACK
      // Reset targetAddress Sent
      if (i2c_transfer == I2C_TRANSFER_READ) {
        if(flags & I2C_IF_ACK) {
            // This is an Ack for setting the target address
            targetAddressSent = true;
            // start the read
            i2c_state = I2C_IDLE_STATE;
            I2C0 -> CMD_SET = I2C_CMD_START;
        }
        if (flags & I2C_IF_RXDATAV) {
            targetAddressSent = false;
            receiveBuffer[receiveIndex++] = (uint8_t)I2C0 -> RXDATA;
            if (receiveIndex >= I2C_BUFFER_SIZE) {
                // stop command issued
                I2C0 -> CMD_SET = I2C_CMD_STOP | I2C_CMD_NACK;
            } else {
                I2C0 -> CMD_SET = I2C_CMD_ACK;
            }
            I2C_IntClear(I2C0, I2C_IF_RXDATAV);
        }
      } else {
        // if write, should expect an ACK from the secondary device
        if (flags & I2C_IF_ACK) {
            // continue transmission unless stop stage reached
            I2C0 -> TXDATA = (uint8_t)transmitBuffer[transmitIndex++];
            // no more data left to send, stop ongoing transfer
            if (transmitIndex >= I2C_BUFFER_SIZE) {
                I2C0 -> CMD_SET = I2C_CMD_STOP;
            }
        }
      }
      break;

    default:
      break;
  }
}

/***************************************************************************//**
 * @brief Setup I2C
 ******************************************************************************/
void initI2C(void)
{
  CMU_ClockEnable(cmuClock_I2C0, true);

  // Use default settings
  I2C_Init_TypeDef i2cInit = I2C_INIT_DEFAULT;

  /*
   * Power up the Si7021 RHT sensor domain on the mainboard.  This
   * also powers up the sensor's local SCL and SDA pull-ups, which
   * eliminates the need to jumper resistors to the EXP header pins.
   */
  GPIO_PinModeSet(I2C_DOMAIN_POWER_PORT,
                  I2C_DOMAIN_POWER_PIN,
                  gpioModePushPull,
                  1);

  // Configure SCL and SDA for open-drain operation
  GPIO_PinModeSet(I2C_SCL_PORT,
                  I2C_SCL_PIN,
                  gpioModeWiredAndPullUpFilter,
                  1);
  GPIO_PinModeSet(I2C_SDA_PORT,
                  I2C_SDA_PIN,
                  gpioModeWiredAndPullUpFilter,
                  1);

  // Route I2C pins to GPIO
  GPIO->I2CROUTE[0].SCLROUTE = (I2C_SCL_PORT << _GPIO_I2C_SCLROUTE_PORT_SHIFT
                             | (I2C_SCL_PIN <<
                                 _GPIO_I2C_SCLROUTE_PIN_SHIFT));
  GPIO->I2CROUTE[0].SDAROUTE = (I2C_SDA_PORT << _GPIO_I2C_SDAROUTE_PORT_SHIFT
                             | (I2C_SDA_PIN  <<
                                 _GPIO_I2C_SDAROUTE_PIN_SHIFT));
  GPIO->I2CROUTE[0].ROUTEEN = GPIO_I2C_ROUTEEN_SCLPEN | GPIO_I2C_ROUTEEN_SDAPEN;

  // Initialize the I2C
  I2C_Init(I2C0, &i2cInit);

  // Set the status flags and index
  i2c_startTransfer = false;

}

/***************************************************************************//**
 * @brief reset I2C and prepare for another transfer. transferType is either read
 *        or write. Reset the transmit and receive index and flush I2C buffer
 ******************************************************************************/
void i2cTransferReset(uint32_t transferType)
{
  GPIO_PinModeSet(gpioPortB, 2, gpioModePushPull, 0);
  GPIO_PinModeSet(gpioPortB, 4, gpioModePushPull, 0);
  i2c_state = I2C_IDLE_STATE;
  i2c_transfer = transferType;
  transmitIndex = 0;
  receiveIndex = 0;
  targetAddressSent = false;

  I2C0->CMD_SET = I2C_CMD_ABORT;
  /* Ensure buffers are empty. */
  I2C0->CMD = I2C_CMD_CLEARPC | I2C_CMD_CLEARTX;

  // Flush RX
  while (I2C0->STATUS & I2C_STATUS_RXDATAV) {
    I2C0->RXDATA;
  }

#if defined(_SILICON_LABS_32B_SERIES_2)

  /* SW needs to clear RXDATAV IF on Series 2 devices.
     Flag is kept high by HW if buffer is not empty. */
  I2C_IntClear(I2C0, I2C_IF_RXDATAV);
#endif

  // Clear all I2C interrupt
  I2C_IntClear(I2C0, _I2C_IF_MASK);
  // Enable I2C interrupt groups
  I2C_IntEnable(I2C0, I2C_LEADER_INTERRUPT | I2C_ERROR_INTERRUPT);
  NVIC_ClearPendingIRQ(I2C0_IRQn);
  NVIC_EnableIRQ(I2C0_IRQn);

  if(transferType == I2C_TRANSFER_WRITE){
    // populate transmit buffer
    for (int i = 0; i < I2C_BUFFER_SIZE; i++) {
      transmitBuffer[i] = receiveBuffer[i]+1;
    }
  }

  i2c_startTransfer = true;
  // Send start condition
  I2C0->CMD_SET = I2C_CMD_START;
}

/***************************************************************************//**
 * @brief GPIO Initialization
 ******************************************************************************/
void initGPIO(void)
{
  // initialize push button 0 and 1
  GPIO_PinModeSet(PUSH_BTN0_PORT, PUSH_BTN0_PIN, gpioModeInputPull, 1);
  GPIO_PinModeSet(PUSH_BTN1_PORT, PUSH_BTN1_PIN, gpioModeInputPull, 1);

  GPIO_PinModeSet(gpioPortB, 2, gpioModePushPull, 0);
  GPIO_PinModeSet(gpioPortB, 4, gpioModePushPull, 0);

  // configure push button 0 and 1 interrupt
  GPIO_ExtIntConfig(PUSH_BTN0_PORT,
                    PUSH_BTN0_PIN,
                    PUSH_BTN0_PIN,
                    false,
                    true,
                    true);
  GPIO_ExtIntConfig(PUSH_BTN1_PORT,
                    PUSH_BTN1_PIN,
                    PUSH_BTN1_PIN,
                    false,
                    true,
                    true);

  // Enable NVIC interrupt
  NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
  NVIC_EnableIRQ(GPIO_ODD_IRQn);
}

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  // Initialize I2C
  initI2C();
  I2C_Enable(I2C0, true);

  // Enable push button interrupts
  initGPIO();
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
  // while loop
}
