/***************************************************************************//**
 * @file ads1220_adc.h
 * @brief ads1220 spi driver header file.
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
 *
 * # Evaluation Quality
 * This code has been minimally tested to ensure that it builds and is suitable
 * as a demonstration for evaluation purposes only. This code will be maintained
 * at the sole discretion of Silicon Labs.
 ******************************************************************************/
#ifndef ADS1220_ADC_H
#define ADS1220_ADC_H

uint32_t ads1220_init(void);                    /* ads1220 initialization */
double ads1220_getAdcTemp(void);                /* ads1220 temperature */
double ads1220_getAdcDataVolt(void);            /* ads1220 get voltage */
void ads1220_Calibrate(void);                   /* ads1220 calibration */
void ads1220_powerDown(void);                   /* ads1220 power down */

#endif /* ADS1220_ADC_H */
