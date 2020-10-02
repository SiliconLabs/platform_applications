/***************************************************************************//**
* @file  microphone_config.h
* @brief Configuration file for the microphone driver. See corresponding STK/MCU user guide/datasheet for proper routing
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

#ifndef MICROPHONE_CONFIG_H_
#define MICROPHONE_CONFIG_H_

/********************************//**
 * Board define macros
 ********************************/
// USART configuration
#define USART_DEV             USART3
#define USART_CLOCK           cmuClock_USART3

// I2S GPIO routing
#define USART_I2S_PORT        gpioPortI
#define USART_I2S_TX_PIN      12
#define USART_I2S_RX_PIN      13
#define USART_I2S_CLK_PIN     14
#define USART_I2S_CS_PIN      15

// I2S pin alias
#define MIC_I2S_DATA_PIN      USART_I2S_RX_PIN
#define MIC_I2S_BCLK_PIN      USART_I2S_CLK_PIN
#define MIC_I2S_LRCLK_PIN     USART_I2S_CS_PIN

// Microphone pin alias
#define MIC_ENABLE_PORT   gpioPortD
#define MIC_ENABLE_PIN    0

/********************************//**
 * Microphone operation notes
 ********************************/
// According to the codec datasheet, the sampling rate should range from 4 kHz to 96 kHz
// The clock frequency of the I2S bus should be at least x64 thats of the sampling frequency
// with a minimum value of 1 Mhz in order to maintain the PDM microphones in active state
// hence the minimum sampling frequency is 15625

#define MIC_LRCLK  2000            //Minimum of 4kHz and maximum 96 Khz
#define MIC_BCLK   MIC_LRCLK * 64  //Bit clock or baudrate = 64 * MIC_LRCLK
                                   //  (32 bits of data per channel) * LRCLK frequency

#define MIC_SAMPLING_FREQ_HZ MIC_LRCLK

//PDM microphone codec generates a PDM clock at 64x the sample rate (LRCLK)
// Since LRCLK/sample rate should be between 4kHz and 96 Khz, the clock range is 256 KHz to 6.144 MHz
// The PDM MEMS microphone need a minimum PDM clock of 1 MHz for active operation hence:
// PDM clock: 1 Mhz
// LRCLK: 15.625 Khz

#endif /* MICROPHONE_CONFIG_H_ */
