/**************************************************************************//**
 * @file main_tg11.c
 * @version 1.0.0
 ******************************************************************************
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
 *     in a product, an acknowledgment in the product documentation would be
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

#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_vdac.h"
#include "em_adc.h"

// Variables used in VDAC calibration
float prev_error;  // Previous error obtained during calibration
float current_error; // Current error obtained during calibration
uint32_t sample;  // sample value of the DAC output
uint8_t gain_val;  // gain value used in calibration
uint32_t cal;
uint8_t acc_gain; // most accurate gain value
uint32_t vdacValue;  // value of the VDAC data register

/**************************************************************************//**
 * @brief  Returns the absolute value of a given value
 *****************************************************************************/
float absolute_val(float value)
{
  if(value<0){
    value = 0-value;
  }
  return value;
}

/**************************************************************************//**
 * @brief  Initialize ADC function
 *****************************************************************************/
void initADC (void)
{
  // Enable ADC0 clock
  CMU_ClockEnable(cmuClock_ADC0, true);

  // Declare init structs
  ADC_Init_TypeDef init = ADC_INIT_DEFAULT;
  ADC_InitSingle_TypeDef initSingle = ADC_INITSINGLE_DEFAULT;

  // Modify init structs and initialize
  // Init to max ADC clock for Series 1
  init.prescale = ADC_PrescaleCalc(16000000, 0);
  init.timebase = ADC_TimebaseCalc(0);
  init.ovsRateSel = adcOvsRateSel4096; //highest oversampling rate

  initSingle.diff       = false;        // single ended
  initSingle.reference  = adcRef2V5;    // internal 2.5V reference
  // set acquisition time to max for best accuracy
  initSingle.acqTime    = adcAcqTime256;
  initSingle.resolution = adcResOVS;  //oversampling resolution, 16 bits

  // Select ADC input. DAC0OUT0 selected first
  initSingle.posSel = adcPosSelDAC0OUT0;

  ADC_Init(ADC0, &init);
  ADC_InitSingle(ADC0, &initSingle);
}

/**************************************************************************//**
 * @brief
 *    VDAC initialization
 *
 * @details
 *    In order to operate in EM2/3 mode, the clock must be set to asynchronous
 *    mode. The prescaler is set because the maximum frequency for the VDAC
 *    clock is 1 MHz. Of course, if you'd like to lower the clock (increase the
 *    prescaler) that is fine too.
 *****************************************************************************/
void initVdac(void)
{
  // Use default settings
  VDAC_Init_TypeDef        init        = VDAC_INIT_DEFAULT;
  VDAC_InitChannel_TypeDef initChannel = VDAC_INITCHANNEL_DEFAULT;

  // Enable the VDAC clock
  CMU_ClockEnable(cmuClock_VDAC0, true);

  // VDAC clock source for asynchronous mode is 12 MHz internal VDAC oscillator
  init.asyncClockMode = true;

  // Calculate the VDAC clock prescaler value resulting in a 1 MHz VDAC clock.
  init.prescaler = VDAC_PrescaleCalc(1000000, false, 0);

  // Set reference to internal 2.5V reference
  init.reference = vdacRef2V5;

  // Initialize the VDAC and VDAC channel
  VDAC_Init(VDAC0, &init);
  VDAC_InitChannel(VDAC0, &initChannel, 0);
  VDAC_InitChannel(VDAC0, &initChannel, 1);

  // Disable aport output, enable main output and alternate output
  VDAC0->OPA[0].OUT &= ~(VDAC_OPA_OUT_APORTOUTEN);
  VDAC0->OPA[0].OUT |= VDAC_OPA_OUT_MAINOUTEN | (VDAC_OPA_OUT_ALTOUTEN  // PC1
                                                + VDAC_OPA_OUT_ALTOUTPADEN_OUT1);

  // Disable main output and aport output, enable alternate output
  VDAC0->OPA[1].OUT &= ~(VDAC_OPA_OUT_MAINOUTEN | VDAC_OPA_OUT_APORTOUTEN);
  VDAC0->OPA[1].OUT |= VDAC_OPA_OUT_ALTOUTEN + VDAC_OPA_OUT_ALTOUTPADEN_OUT2; // PC14

  // Enable the VDAC
  VDAC_Enable(VDAC0, 0, true);
  VDAC_Enable(VDAC0, 1, true);
}

/**************************************************************************//**
 * @brief
 *    VDAC calibration routine
 * @detail
 *    This function implements the calibration routine for
 *    VDAC channel 0 and channel 1. It uses the internal
 *    ADC to measure the DAC output voltage and adjust
 *    the calibration register accordingly. For detailed
 *    calibration routine description please refer to the
 *    readme.md file
 *****************************************************************************/
void calibrateVdac()
{
  float temp;
  cal = VDAC0 -> CAL;  // calibration register

  // Enable ADC
  initADC();

  // Set VDAC output for both channel 0 and 1
  // Set to 80 percent of full scale
  VDAC_Channel0OutputSet(VDAC0, 0xCCC);
  VDAC_Channel1OutputSet(VDAC0, 0xCCC);

  // Perform calibration on channel 0
  // Set gain_val to highest value possible
  // load gain value
  gain_val = 0x3F;
  acc_gain = gain_val;

  // Clear GAINERRTRIM field
  cal = cal & (~_VDAC_CAL_GAINERRTRIM_MASK);

  // Populate GAINERRTRIM field with new value
  cal = cal | (gain_val << _VDAC_CAL_GAINERRTRIM_SHIFT);

  sample = 4095;
  prev_error = 4095;
  current_error = 4095;

  // find the smallest error for DAC 0
  // terminate while loop if gain_val = 0 or if we found the smallest error
  while(gain_val >0){

    // Populate VDAC calibration register
    VDAC0 -> CAL = cal;

    // Update previous error
    prev_error = current_error;

    // take one ADC measurement of DAC output 0
    ADC_Start(ADC0, adcStartSingle);

    // Wait for conversion to be complete
    while(!(ADC0->STATUS & _ADC_STATUS_SINGLEDV_MASK));

    // Get ADC result
    sample = ADC_DataSingleGet(ADC0) >> 4;

    // calculate error, formula given in the readme.md file
    temp = (sample/3276.0)-1.0;
    current_error = absolute_val(temp); // calculate absolute value of temp

    // If smallest error found, break out of the loop
    if(prev_error < current_error){
      break;
    }

    acc_gain = gain_val;

    // Update gain_val;
    gain_val-=1;
    cal = cal & (~_VDAC_CAL_GAINERRTRIM_MASK);
    cal = cal | (gain_val << _VDAC_CAL_GAINERRTRIM_SHIFT);
  }

  // Smallest error found, populate cal register with most accurate gain value
  cal = cal & (~_VDAC_CAL_GAINERRTRIM_MASK);
  cal = cal | (acc_gain << _VDAC_CAL_GAINERRTRIM_SHIFT);

  // Populate VDAC
  VDAC0 -> CAL = cal;

  // Perform calibration on channel 1
  // Set gain_val to highest value possible
  // load gain value
   gain_val = 0xF;
   acc_gain = gain_val;
   cal = cal & (~_VDAC_CAL_GAINERRTRIMCH1_MASK);
   cal = cal | (gain_val << _VDAC_CAL_GAINERRTRIMCH1_SHIFT);

   sample = 4095;
   prev_error = 4095;
   current_error = 4095;

   // Set channel 1 output
   VDAC_Channel1OutputSet(VDAC0, 0xCCC);

   // Update ADC input selection
   ADC0 -> SINGLECTRL &= (~_ADC_SINGLECTRL_POSSEL_MASK);
   ADC0 -> SINGLECTRL |= ADC_SINGLECTRL_POSSEL_DAC0OUT1;

   // find the smallest error for DAC 0
   // terminate while loop if gain_val = 0 or if we found the smallest error
   while(gain_val >0){

     // Populate VDAC calibration register
     VDAC0 -> CAL = cal;

     // Update previous error
     prev_error = current_error;

     // take one ADC measurement of DAC output 1
     ADC_Start(ADC0, adcStartSingle);

     // Wait for conversion to be complete
     while(!(ADC0->STATUS & _ADC_STATUS_SINGLEDV_MASK));

     // Get ADC result, result is 16 bits, need shift right 4 bits
     sample = ADC_DataSingleGet(ADC0) >> 4;

     // calculate error
     temp = (sample/3276.0)-1.0;
     current_error = absolute_val(temp); // calculate absolute value of temp

     // If smallest error found, break from while loop
     if(prev_error < current_error){
       break;
     }

     // Update acc_gain, this is the most accurate gain_val currently
     acc_gain = gain_val;

     // Update gain_val;
     gain_val-=1;
     cal = cal & (~_VDAC_CAL_GAINERRTRIMCH1_MASK);
     cal = cal | (gain_val << _VDAC_CAL_GAINERRTRIMCH1_SHIFT);
   }

   // Smallest error found, populate cal register with most accurate gain value
   cal = cal & (~_VDAC_CAL_GAINERRTRIMCH1_MASK);
   cal = cal | (acc_gain << _VDAC_CAL_GAINERRTRIMCH1_SHIFT);

   // Populate VDAC CAL register
   VDAC0 -> CAL = cal;
}

/**************************************************************************//**
 * @brief
 *    Calculate the digital value that maps to the desired output voltage
 *
 * @note
 *    The vRef parameter must match the reference voltage selected during
 *    initialization
 *
 * @param [in] vOut
 *    Desired output voltage
 *
 * @param [in] vRef
 *    Reference voltage used by the VDAC
 *
 * @return
 *    The digital value that maps to the desired output voltage
 *****************************************************************************/
uint32_t getVdacValue(float vOut, float vRef)
{
  return (uint32_t)((vOut * 4095) / vRef);
}

/**************************************************************************//**
 * @brief
 *    Continuously output 0.6 volts to VDAC channel 0 and channel 1
 *****************************************************************************/
int main(void)
{
  // Chip errata
  CHIP_Init();

  // Init DCDC regulator with kit specific parameters
  EMU_DCDCInit_TypeDef dcdcInit = EMU_DCDCINIT_DEFAULT;
  EMU_DCDCInit(&dcdcInit);

  // Initialization
  initVdac();

  initADC();

  // VDAC calibration routine
  calibrateVdac();

  // Set VdacValue to 1.25V
  vdacValue = getVdacValue(1.25, 2.5);

  // Write the output value to VDAC DATA register
  VDAC_ChannelOutputSet(VDAC0, 0, vdacValue);
  VDAC_ChannelOutputSet(VDAC0, 1, vdacValue);

  while (1) {
    EMU_EnterEM3(false); // Enter EM3 while the VDAC is doing continuous conversions
  }
}

