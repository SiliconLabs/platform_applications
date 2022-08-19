/***************************************************************************//**
 * @file efr32bg22_adc.h
 * @brief efr32bg22 adc driver header file.
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
#ifndef EFM32BG22_ADC_H
#define EFM32BG22_ADC_H

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ADC_BUFFER_SIZE 1024

void resetIADC(void);                           // bg22 iadc reset
void rescaleIADC(uint32_t newScale);            // bg22 iadc rescale
void initIADC(void);                            // bg22 iadc initialization
void bg22SaveCalData(uint32_t scale);           // bg22 iadc cal data save
void bg22RestoreCalData(void);                  // bg22 iadc cal data restore
double iadcPollSingleResult(void);              // bg22 iadc voltage polling
uint32_t iadcDifferentialCalibrate();           // bg22 iadc calibration

void lightLED(uint8_t onoff);                   // led on/off
void initLetimer(void);
void letimerDelay(uint32_t msec);               // simple delay
void initButtonEM2(void);                       // button in EM2
float getDieTemperature(void);                  // bg22 emu die temperature

double rmsCal(double buffer[], double adcAve);
extern double buffer[ADC_BUFFER_SIZE];          // buffer to save adc data
extern double adcGainResult;                    // adc gain cal result
extern double adcOffsetResult;                  // adc offset cal result
extern double adcEnobResult;                    // adc enob result

#ifdef __cplusplus
}
#endif /* EFM32BG22_ADC_H */

#endif
