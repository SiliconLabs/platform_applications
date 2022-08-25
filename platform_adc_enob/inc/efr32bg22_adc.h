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

void reset_iadc(void);                           // bg22 iadc reset
void rescale_iadc(uint32_t newScale);            // bg22 iadc rescale
void init_iadc(void);                            // bg22 iadc initialization
void bg22_save_cal_data(uint32_t scale);         // bg22 iadc cal data save
void bg22_restore_cal_data(void);                // bg22 iadc cal data restore
double iadc_poll_single_result(void);            // bg22 iadc voltage polling
uint32_t iadc_differential_calibrate();          // bg22 iadc calibration

void light_led(uint8_t onoff);                   // led on/off
void init_letimer(void);
void letimer_delay(uint32_t msec);               // simple delay
void init_button_em2(void);                      // button in EM2
float get_die_temperature(void);                 // bg22 emu die temperature
double rms_cal(double buffer[], double adcAve);
extern double buffer[ADC_BUFFER_SIZE];           // buffer to save adc data
extern double adc_gain_result;                   // adc gain cal result
extern double adc_offset_result;                 // adc offset cal result
extern double adc_enob_result;                   // adc enob result

#ifdef __cplusplus
}
#endif 

#endif /* EFM32BG22_ADC_H */
