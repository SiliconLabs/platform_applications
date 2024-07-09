/***************************************************************************//**
 * @file peripherals.h
 * @brief peripheral functions
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

#ifndef PERIPHERALS_H_
#define PERIPHERALS_H_

#include <stdint.h>

/***************************************************************************//**
 * Initialize ADC
 *
 * ADC0 and ADC1 are configured to be triggered by GPIO (BTN0) via PRS.
 * ADC0 samples the signal on ch13 (PA13) while ADC1 samples the signal
 * on ch11 (PE11).
 * The data sampled by each ADC is then stored in their FIFO.
 ******************************************************************************/
void adc_init(void);

/***************************************************************************//**
 * Initialize LDMA
 *
 * The LDMA is configured to start reading data from the ADC0 and ADC1 FIFO,
 * one sample from each FIFO. The samples are then stored in their individual
 * buffers. Currently it is configured to store 4 such samples and then print
 * them before overwriting the individual buffers in memory.
 ******************************************************************************/
void dma_init(void);

/***************************************************************************//**
 * Initialize GPIO
 *
 * Configure BTN0 as input and to generate PRS interrupt signals when pressed.
 ******************************************************************************/
void gpio_init(void);

/***************************************************************************//**
 * Initialize LDMA transfers
 ******************************************************************************/
void dma_tx(void);

/***************************************************************************//**
 * Wrapper function to access array
 ******************************************************************************/
uint32_t *get_adc_data1(void);

/***************************************************************************//**
 * Wrapper function to access array
 ******************************************************************************/
uint32_t *get_adc_data0(void);

#endif /* PERIPHERALS_H_ */
