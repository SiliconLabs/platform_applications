/***************************************************************************//**
 * @file app.c
 * @brief application.
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
 *
 * # Evaluation Quality
 * This code has been minimally tested to ensure that it builds and is suitable
 * as a demonstration for evaluation purposes only. This code will be maintained
 * at the sole discretion of Silicon Labs.
 ******************************************************************************/
#include "efr32bg22_adc.h"
#include "dac70501_dac.h"
#include "ads1220_adc.h"
#include "math.h"
#include "app_log.h"

#define ADC_REF_VOLT 1250                          // adc reference voltage
float dac_voltage_value;                           // dac70501 output voltage
float bg22_die_temperature;                        // bg22 emu die temp
double ads_adc_temperature;                        // ads1220 adc temp
uint32_t bg22_adc_scale_result;                    // bg22 scale cal result
double adc_min = 1.26;                             // for enob calculation
double adc_max = 0.0;                              // for enob calculation
double adc_peak = 0.0;                             // for enob calculation
double adc_ave = 0.0;                              // for enob calculation
double adc_rms = 0.0;                              // for enob calculation
double adc_sinad, adc_enob_result;                 // for enob calculation
uint32_t adc_snr = 0.0;                            // for enob calculation

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  // Initialize letimer for delay function
  init_letimer();

  // Initialize button for EM2 interrupt
  init_button_em2();

  // Turn on LED
  light_led(1);

  // Initialize dac70501
  dac70501_init();
  dac70501_set_volt(1.00f);

  // dac set voltage test
  dac_voltage_value = 1.00f;
  dac70501_set_volt(dac_voltage_value);
  // suggested by dac70501
  letimer_delay(10);

  // Initialize ads1220
  ads1220_init();
  // suggested by ads1220
  letimer_delay(10);
  // ads1220 gain and offset calibration
  ads1220_calibrate();
  letimer_delay(10);

  // Read temperature
  ads_adc_temperature = ads1220_get_adc_temp();
  bg22_die_temperature = get_die_temperature();
  app_log("ads1220 temp - %f degC\r\n", ads_adc_temperature);
  app_log("efr32bg temp - %f degC\r\n", bg22_die_temperature);
  app_log("\r\n");

  // collect 10 samples
  // buffer data should be close to 1.0v
  for (uint32_t i = 0; i < 10; i++) {
    buffer[i] = ads1220_get_adc_data_volt();
  }

  // dump adc result via terminal
  // buffer data should be close to 1.0v
  app_log("ADS1220 adc voltage in mV:\r\n");
  app_log("sample - value\r\n");
  for (uint32_t i = 0; i < 10; i++) {
    app_log("%d - %f \r\n", i, buffer[i]);
  }
  app_log("\r\n");

  // efr32bg22 iadc calibration
  // buffer data should be close to 0.6v
  bg22_adc_scale_result = iadc_differential_calibrate();

  // Initialize the IADC
  init_iadc();
  // set dac voltage as 1.0v
  dac70501_set_volt(dac_voltage_value);

  // collect 1024 samples for ENOB calculation
  // and get the max, min, average
  adc_max = 0.0;
  adc_min = 1.26;
  adc_ave = 0.0;
  for (uint32_t i = 0; i < ADC_BUFFER_SIZE; i++) {
    buffer[i] = iadc_poll_single_result();
    adc_ave += buffer[i];
    if (buffer[i] < adc_min) {
      adc_min = buffer[i];
    }
    if (buffer[i] > adc_max) {
      adc_max = buffer[i];
    }
  }

  // statistic calculation
  adc_peak = (adc_max - adc_min) * 1000;            // in mV unit
  adc_ave = adc_ave / ADC_BUFFER_SIZE;              // in V unit
  adc_rms = rms_cal(buffer, adc_ave);
  adc_ave *= 1000;                                  // in mV unit

  // snr based on peak-peak, please refer to readme.md
  adc_peak = adc_peak / 6.6;                        // in mV unit
  adc_snr = (uint32_t)(ADC_REF_VOLT * 2 / adc_peak);// signal to noise ratio

  // enob calculation
  // adcEnobResult should be higher than 14.3
  adc_sinad = 20 * log10(adc_snr);
  adc_enob_result = (adc_sinad - 1.76f) / 6.02;

  // dump efr32bg22 adc result via terminal
  // buffer data should be close to 1.0v
  app_log("efr32bg22 adc voltage in mV:\r\n");
  app_log("sample - value\r\n");
  for (uint32_t i = 0; i < 10; i++) {
    app_log("%d - %f \r\n", i, buffer[i]);
  }
  app_log("\r\n");

  app_log("enob - %f \r\n", adc_enob_result);
  // to save power, you can power down ads1220 and dac70751 here
  // and reset efr32bg22 iadc
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
}
