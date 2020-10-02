/***************************************************************************//**
* @file  microphone_driver.c
* @brief Driver for the SPK0838HT4H-B MEMS microphone (Source file)
*******************************************************************************
* # License
* <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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

#include "em_gpio.h"
#include "em_cmu.h"
#include "em_ldma.h"
#include "em_usart.h"
#include "Microphone_driver.h"

/********************************//**
 * Global variables
 ********************************/
// LDMA link descriptors
LDMA_Descriptor_t leftDesc[2];
LDMA_Descriptor_t rightDesc;

// Single byte to dispose of right microphone data
uint8_t dummy_buffer;

/***************************************************************************//**
 * @name: Init_MIC
 *
 * @brief: Initializes MEMS microphone setting up the UART port as I2S
 *         The GPIO pin for the microphone is configured and set
 *
 * @return: None
 ******************************************************************************/
void Init_MIC(void)
{
  // Enable clock for USART_DEV
  CMU_ClockEnable(USART_CLOCK, true);

  // Enable GPIO clock and I2S pins
  GPIO_PinModeSet(USART_I2S_PORT, USART_I2S_RX_PIN, gpioModeInputPullFilter, 0);
  GPIO_PinModeSet(USART_I2S_PORT, USART_I2S_TX_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(USART_I2S_PORT, USART_I2S_CLK_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(USART_I2S_PORT, USART_I2S_CS_PIN, gpioModePushPull, 0);

  // USART initialization
  //   Default initialization
  USART_InitI2s_TypeDef USART_config = USART_INITI2S_DEFAULT;

  // Application specific configuration
  USART_config.sync.enable   = usartDisable;               //Don't enable RX/TX upon initialization
  USART_config.sync.databits = usartDatabits8;             //8-bit frame
  USART_config.sync.autoTx   = true;                       //Transmit as long as RX is not full
  USART_config.sync.baudrate = MIC_BCLK;                   //Set baudrate to the desired sample frequency
  USART_config.format        = usartI2sFormatW32D32;       //32 bit words, 32 bit data per channel
  USART_config.mono          = false;                      //Stereo format
  USART_config.justify       = usartI2sJustifyLeft;        //Left justified in the frame, following I2S standard
  USART_config.delay         = true;                       //MSB of next word is 1 -bit delayed, following I2S standard
  USART_config.dmaSplit      = true;                       //Separate DMA request for the left and right channels

  USART_InitI2s(USART_DEV, &USART_config);

  // Configure RX & TX enabling through PRS channel 0
  USART_DEV->TRIGCTRL = (USART_TRIGCTRL_RXTEN
                         | USART_TRIGCTRL_TXTEN
                         | USART_TRIGCTRL_TSEL_PRSCH0);

  // Enable route to GPIO pins for I2S transfer on route #5
  USART_DEV->ROUTEPEN = (USART_ROUTEPEN_TXPEN
                         | USART_ROUTEPEN_RXPEN
                         | USART_ROUTEPEN_CSPEN
                         | USART_ROUTEPEN_CLKPEN);

  USART_DEV->ROUTELOC0 = (USART_ROUTELOC0_TXLOC_LOC5
                          | USART_ROUTELOC0_RXLOC_LOC5
                          | USART_ROUTELOC0_CSLOC_LOC5
                          | USART_ROUTELOC0_CLKLOC_LOC5);

  // Initialize mic_enable pin (PD0)
  GPIO_PinModeSet(MIC_ENABLE_PORT, MIC_ENABLE_PIN, gpioModePushPull, 1);
}

/***************************************************************************//**
 * @name: MIC_disable
 *
 * @brief: Disable the receiver and transmitter of the UART device
 *
 * @return: None
 ******************************************************************************/
void MIC_disable(void)
{
	USART_Enable(USART_DEV, usartDisable);
}

/***************************************************************************//**
 * @name: MIC_LDMA_init
 *
 * @brief: Initializes the LDMA peripheral. used to transfer data from the USART_DEV
 *         RX buffer to the specified memory buffers in an autonomous way
 *
 * @param[in]:
 * 		buffer1: Pointer to buffer A for left microphone
 * 		buffer2: Pointer to buffer B for left microphone
 *		buffer_size: Number of elements in the buffers
 *
 * @return: None
 ******************************************************************************/
void InitLDMA_MIC(uint32_t *buffer1, uint32_t *buffer2, uint32_t buffer_size)
{
  // Default LDMA init
  LDMA_Init_t init = LDMA_INIT_DEFAULT;
  LDMA_Init(&init);

  // LDMA transfer configurations
  //   Left microphone data configuration - Peripheral (USART) to memory
  LDMA_TransferCfg_t leftCfg = LDMA_TRANSFER_CFG_PERIPHERAL(ldmaPeripheralSignal_USART3_RXDATAV);

  //  Right microphone data configuration - Peripheral (USART) to memory
  LDMA_TransferCfg_t rightCfg = LDMA_TRANSFER_CFG_PERIPHERAL(ldmaPeripheralSignal_USART3_RXDATAVRIGHT);

  //LDMA descriptors configuration
  //  Left microphone descriptors: looped descriptors - peripheral to memory
  //                               Source: RXDATA
  //                               Destination: buffer1
  //                               Total transfered bytes: 4*#buffer elements. The buffer is of uint32_t type
  //                               Destination: Casted as uint8_t as 8-bit frames are read from RXDATA, 1 byte transfered per execution
  //                               LinkJump:
  //                                  * Descriptor 1: Go to the next descriptor in memory
  //                                  * Descriptor 2: Go to the previous descriptor in memory
  LDMA_Descriptor_t leftXfer[] = {LDMA_DESCRIPTOR_LINKREL_P2M_BYTE(&USART_DEV->RXDATA,
                                                                   (uint8_t *)buffer1,
                                                                   (4 * buffer_size),
                                                                    1),

                                  LDMA_DESCRIPTOR_LINKREL_P2M_BYTE(&USART_DEV->RXDATA,
                                                                   (uint8_t *)buffer2,
                                                                   (4 * buffer_size),
                                                                    -1)
  };

  // Globally store and configure link descriptors for left microphone transfer
  leftDesc[0] = leftXfer[0];
  leftDesc[1] = leftXfer[1];

  // Application specific xfer descriptor configuration
  //   Descriptor 0
  leftDesc[0].xfer.size = ldmaCtrlSizeByte;  //Byte sized transfers
  leftDesc[0].xfer.doneIfs = 1;              //Trigger interrupt on transfer complete (buffer full)
  leftDesc[0].xfer.ignoreSrec = 0;           //Single requests are ignored
  //   Descriptor 1
  leftDesc[1].xfer.size = ldmaCtrlSizeByte;
  leftDesc[1].xfer.doneIfs = 1;
  leftDesc[1].xfer.ignoreSrec = 0;

  //  Right microphone descriptor: looped descriptor - right data is "discarded", stored in a dummy_buffer
  //                               Source: RXDATA
  //                               Destination: buffer2
  //                               Total transfered bytes: 1 byte
  //                               LinkJump: No linkage, use the same descriptor
  LDMA_Descriptor_t rightXfer = LDMA_DESCRIPTOR_LINKREL_P2M_BYTE(&USART_DEV->RXDATA,
                                                                 &dummy_buffer,
                                                                 1,
                                                                 0);


  // Globally store and configure link descriptors for right microphone transfer
  rightDesc = rightXfer;

  //  Application specific xfer descriptor configuration
  rightDesc.xfer.size = ldmaCtrlSizeByte;  //Byte sized transfers
  rightDesc.xfer.doneIfs = 0;              //Don't trigger interrupts upon transfer complete
  rightDesc.xfer.ignoreSrec = 0;           //Single request are ignored
  rightDesc.xfer.dstInc = 0;               //Don't increase destination source

  // Start left and right transfers
  LDMA_StartTransfer(0, (void *)&leftCfg, (void *)&leftDesc);
  LDMA_StartTransfer(1, (void *)&rightCfg, (void *)&rightDesc);
}
