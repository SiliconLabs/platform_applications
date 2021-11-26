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
#include "efr32bg22_adc.h"
#include "dac70501_dac.h"
#include "ads1220_adc.h"
#include "math.h"

float dacVoltageValue;                            /* dac70501 output voltage */
float bg22DieTemperature;                         /* bg22 emu die temp */
double adsAdcTemperature;                         /* ads1220 adc temp */
uint32_t bg22AdcScaleResult;                      /* bg22 scale cal result */
double adcMax = 0.0;                              /* for enob calculation */
double adcMin = 1.26;                             /* for enob calculation */
double adcPeak = 0.0;                             /* for enob calculation */
double adcAve = 0.0;                              /* for enob calculation */
double adcRms = 0.0;                              /* for enob calculation */
double adcSinad, adcEnobResult;                   /* for enob calculation */
uint32_t adcSnr = 0.0;                            /* for enob calculation */

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  uint32_t adcValueIntmV, adcValueFramV;
  /* Initialize letimer for delay function */
  initLetimer();
  /* Initialize button for EM2 interrupt */
  //initButtonEM2();
  /* Turn on LED */
  //lightLED();
  /* Initialize dac70501 */
  dac70501_init();
  /* Initialize ads1220 */
  ads1220_init();

  /* Read temperature */
  bg22DieTemperature = getDieTemperature();
  adsAdcTemperature = ads1220_getAdcTemp();

  /* dac set voltage test */
  dacVoltageValue = 1.00f;
  dac70501_setVolt(dacVoltageValue);
  letimerDelay(10);

  /* ads1220 gain and offset calibration */
  ads1220_Calibrate();
  /* collect 10 samples */
  for(uint32_t i = 0; i < 10; i++)
    buffer[i] = ads1220_getAdcDataVolt();

  /* efr32bg22 iadc calibration */
  bg22AdcScaleResult = iadcDifferentialCalibrate();
  //bg22SaveCalData(bg22AdcScaleResult);

  /* Initialize the IADC */
  initIADC();
  //bg22RestoreCalData();
  //rescaleIADC(bg22AdcScaleResult);
  dac70501_setVolt(dacVoltageValue);
  /* collect 1024 samples for ENOB calculation */
  adcMax = 0.0;
  adcMin = 1.26;
  adcAve = 0.0;
  for(uint32_t i = 0; i < ADC_BUFFER_SIZE; i++)
  {
    buffer[i] = iadcPollSingleResult();
    adcAve += buffer[i];
    if(buffer[i] < adcMin)
      adcMin = buffer[i];
    if(buffer[i] > adcMax)
      adcMax = buffer[i];
  }
  /* dump adc result via to terminal */
  for(uint32_t i = 0; i < ADC_BUFFER_SIZE; i++)
  {
    adcValueIntmV = (uint32_t) (buffer[i] * 1000);
    adcValueFramV = (uint32_t) ((buffer[i] * 1000 - adcValueIntmV) * 1000);
    //app_log("adc voltage %d.%d mV\r\n", adcValueIntmV, adcValueFramV);
  }

  /* statistic calcualtion  */
  adcPeak = (adcMax - adcMin) * 1000;             /* in mV unit */
  adcAve = adcAve / ADC_BUFFER_SIZE;              /* in V unit */
  adcRms = rmsCal(buffer, adcAve);
  adcAve *= 1000;                                 /* in V unit */

  /* snr based on peak-peak */
  adcPeak = adcPeak/6.6;                          /* in mV unit */
  adcSnr = (uint32_t)(1250 * 2 / adcPeak);        /* signal to noise ratio */

  /* enob calculation */
  adcSinad = 20* log10(adcSnr);
  adcEnobResult = (adcSinad - 1.76f) / 6.02;

  //dac070501_powerDown(1, 1);
  //letimerDelay(1000);
  //resetIADC();
  //ads1220_powerDown();
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
}
