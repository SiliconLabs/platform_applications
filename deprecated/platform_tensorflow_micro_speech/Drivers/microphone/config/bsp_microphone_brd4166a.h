/******************************************************************************
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

#pragma once



#define BSP_MICROPHONE_USART                   USART1
#define BSP_MICROPHONE_USART_CLOCK             cmuClock_USART1
#define BSP_MICROPHONE_USART_DMA_LEFT_SIGNAL   ldmaPeripheralSignal_USART1_RXDATAV
#define BSP_MICROPHONE_USART_DMA_RIGHT_SIGNAL  ldmaPeripheralSignal_USART1_RXDATAVRIGHT
#define BSP_MICROPHONE_USART_ROUTELOC0         (USART_ROUTELOC0_RXLOC_LOC11 | USART_ROUTELOC0_CSLOC_LOC11 | USART_ROUTELOC0_CLKLOC_LOC11)

#define BSP_MICROPHONE_USART_RX_PIN            gpioPortC, 7 // MIC_I2S_SD  -> US1_RX#11  -> MCU_PC7
#define BSP_MICROPHONE_USART_CLK_PIN           gpioPortC, 8 // MIC_I2S_SCK -> US1_CLK#11 -> MCU_PC8
#define BSP_MICROPHONE_USART_CS_PIN            gpioPortC, 9 // MIC_I2S_WS  -> US1_CS#11  -> MCU_PC9
#define BSP_MICROPHONE_ENABLE_PIN              gpioPortF, 10 // MIC_ENABLE ->            -> MCU_PF10

#define MICROPHONE_RESOLUTION 24
