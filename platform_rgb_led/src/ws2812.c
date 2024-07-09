/***************************************************************************//**
 * @file ws2812.c
 * @brief Driver File for WS2812 RGB LEDs
 * @version v1.0
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
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

#include "ws2812.h"

// ******** How This Driver Works ********//
//  https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf
//  According to the WS2812 datasheet the protocol runs at 800kHz
//  A '1' bit is a pulse which starts high and ends low with a duty cycle of 64%
//  (.8uS HIGH and 0.4uS LOW)
//  A '0' bit is a pulse which starts high and ends low with a duty cycle of 32%
//  (.4uS HIGH and 0.85uS LOW)
//  The tolerance on every edge is 150nS
//  By using a frequency of 3 x 800 kHz = 2.4MHz, 3 bits at 2.4MHz represent
//  each bit at 800KHz
//  Each series of 3 bit begins with a '1' and ends with a '0'
//  If the middle bit is set to '1' the duty cycle will be 66%
//  (0.833uS HIGH and 0.416uS LOW)
//  If the middle bit is set to '0' the duty cycle will be 33%
//  (0.416uS HIGH and 0.833uS LOW)
//  These HIGH and LOW times are within 40nS of the specs from the datasheet
//  This driver uses the USART peripheral with the CLK, CS and RX disabled as
//  only the TX pin is needed

// 3 color channels, 8 bits each
#define NUMBER_OF_COLOR_BITS     (NUMBER_OF_LEDS * 3 * 8)

// 3 USART bits are required to make a full 1.25uS color bit,
// each USART bit is 416nS
#define USART_NUMBER_OF_BITS     (NUMBER_OF_COLOR_BITS * 3)

// How big the USART buffer should be,
// the first 15 bytes should be empty to provide a 50uS reset signal
#define USART_BUFFER_SIZE_BYTES  ((USART_NUMBER_OF_BITS / 8) + 15)

// Output buffer for USART
static uint8_t USART_tx_buffer[USART_BUFFER_SIZE_BYTES];

// Frequency for the protocol in Hz, 800 kHz gives a 1.25uS duty cycle
#define PROTOCOL_FREQUENCY       800000

// 3 USART bits are required to make a full 1.25uS color bit
// USART frequency should therefore be 3x the protocol frequency
#define REQUIRED_USART_FREQUENCY (PROTOCOL_FREQUENCY * 3)

// Descriptor and config for the LDMA operation for sending data
static LDMA_Descriptor_t ldmaTXDescriptor;
static LDMA_TransferCfg_t ldmaTXConfig;

// Default byte values for protocol, x is defaulted to 0
// Full 3-byte sequence for a color byte is 1x01 x01x 01x0 1x01 x01x 01x0
#define FIRST_BYTE_DEFAULT  0x92; // 1x01 x01x  1st Byte has color bits 7,6 & 5
#define SECOND_BYTE_DEFAULT 0x49; // 01x0 1x01  2nd Byte has color bits 4 & 3
#define THIRD_BYTE_DEFAULT  0x24; // x01x 01x0  3rd Byte has color bits 2,1 & 0

/**************************************************************************//**
 * @brief
 *  Transform rgb array into protocol-compatible buffer and begin output
 *
 * @param[in] input_color_buffer
 *  Array of colors to be output.  Arranged in series of G,R then B
 *****************************************************************************/
void set_color_buffer(const uint8_t *input_color_buffer)
{
  // Each color bit is encoded by 3 bits
  // The first bit is always 1 and the third bit is always 0
  // The actual color bit value is encoded into the second bit
  // Each color channel(1 byte) therefore requires 3 bytes to be transmitted
  // The entire 3-byte sequence is 8 repetitions of (1x0) ->  1x01 x01x 01x0
  // 1x01 x01x 01x0
  // Each x represents a color bit
  // Isolate the first byte
  const uint8_t *input_color_byte = input_color_buffer;
  uint32_t usart_buffer_index = 0;
  while (usart_buffer_index < USART_BUFFER_SIZE_BYTES) {
    // FIRST_BYTE
    // Isolate bit 7 and shift to position 6
    uint8_t bit_7 = (uint8_t)((*input_color_byte & 0x80) >> 1);
    // Isolate bit 6 and shift to position 3
    uint8_t bit_6 = (uint8_t)((*input_color_byte & 0x40) >> 3);
    // Isolate bit 5 and shift to position 0
    uint8_t bit_5 = (uint8_t)((*input_color_byte & 0x20) >> 5);
    // Load byte into the TX buffer
    USART_tx_buffer[usart_buffer_index] = bit_7 | bit_6 | bit_5
                                          | FIRST_BYTE_DEFAULT;
    usart_buffer_index++;  // Increment USART_tx_buffer pointer

    // SECOND BYTE
    // Isolate bit 4 and shift to position 5
    uint8_t bit_4 = (uint8_t)((*input_color_byte & 0x10) << 1);
    // Isolate bit 3 and shift to position 2
    uint8_t bit_3 = (uint8_t)((*input_color_byte & 0x08) >> 1);
    // Load byte into the TX buffer
    USART_tx_buffer[usart_buffer_index] = bit_4 | bit_3 | SECOND_BYTE_DEFAULT;
    usart_buffer_index++; // Increment USART_tx_buffer pointer

    // THIRD BYTE
    // Isolate bit 2 and shift to position 7
    uint8_t bit_2 = (uint8_t)((*input_color_byte & 0x04) << 5);
    // Isolate bit 1 and shift to position 4
    uint8_t bit_1 = (uint8_t)((*input_color_byte & 0x02) << 3);
    // Isolate bit 0 and shift to position 1
    uint8_t bit_0 = (uint8_t)((*input_color_byte & 0x01) << 1);
    // Load byte into the TX buffer
    USART_tx_buffer[usart_buffer_index] = bit_2 | bit_1 | bit_0
                                          | THIRD_BYTE_DEFAULT;
    usart_buffer_index++;

    input_color_byte++; // move to the next color byte
  }

  LDMA_StartTransfer(TX_DMA_CHANNEL, &ldmaTXConfig, &ldmaTXDescriptor);
}

/**************************************************************************//**
 * @brief
 *  Initializes serial output and LDMA
 *****************************************************************************/
void init_ws2812_driver(void)
{
  init_serial_output();
  init_LDMA();
}

/**************************************************************************//**
 * @brief
 *  Initializes serial output
 *****************************************************************************/
void init_serial_output(void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(USART_CMU_CLK, true);

  // Configure GPIO mode, US0_TX(MOSI) is push pull
  GPIO_PinModeSet(USART_TX_PORT, USART_TX_PIN, gpioModePushPull, 1);

  // Start with default config, then modify as necessary
  USART_InitSync_TypeDef config = USART_INITSYNC_DEFAULT;
  config.master = true;                  // Master mode
  config.baudrate = REQUIRED_USART_FREQUENCY;
  config.clockMode = usartClockMode0;    // Clock idle low, sample on
                                         // rising/first edge
  config.msbf = true;                    // Send MSB first
  config.enable = usartDisable;          // Keep USART disabled until it's all
                                         // set up
  USART_InitSync(USART_PERIPHERAL, &config);

  // Set TX Pin Location
  USART_PERIPHERAL->ROUTELOC0 = USART_TX_LOCATION;
  // Only enable TX pin
  USART_PERIPHERAL->ROUTEPEN = USART_ROUTEPEN_TXPEN;
  // Enable USART

  USART_Enable(USART_PERIPHERAL, usartEnableTx);
}

/**************************************************************************//**
 * @brief
 *  LDMA handler.  Clears LDMA interrupt and does nothing else
 *****************************************************************************/
void LDMA_IRQHandler(void)
{
  uint32_t flags = LDMA_IntGet();
  LDMA_IntClear(flags);
}

/**************************************************************************//**
 * @brief
 *  Initializes LDMA
 *****************************************************************************/
void init_LDMA(void)
{
  CMU_ClockEnable(cmuClock_LDMA, true);
  LDMA_Init_t ldmaInit = LDMA_INIT_DEFAULT;
  LDMA_Init(&ldmaInit); // Initializing default LDMA settings

  // Memory to peripheral transfer, Source: TxBuffer, Destination:
  // USART0->TXDATA
  ldmaTXDescriptor = (LDMA_Descriptor_t)LDMA_DESCRIPTOR_SINGLE_M2P_BYTE(
    USART_tx_buffer,
    &(USART_PERIPHERAL->TXDATA),
    USART_BUFFER_SIZE_BYTES);

  // One byte will transfer everytime the USART TXBL flag is high
  ldmaTXConfig = (LDMA_TransferCfg_t)LDMA_TRANSFER_CFG_PERIPHERAL(
    LDMA_PERIPHERAL_SIGNAL);
}
