/***************************************************************************//**
* @file main.c
* @version 1.0.0
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

#include <stdio.h>
#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_adc.h"

// max ADC clock for Series 1 devices
#define adcFreq                     16000000

// VIN attenuation factor must be set to maximum to use the ADC as a RNG
#define ADC_SINGLECTRLX_VINATT_MAX  0xF

// The LSB[2:0] of each ADC sample will be a random number
// Hence the mask is defined as 0x7 to only select the relevant bits from an
// ADC sample
#define ADC_RND_BIT_MASK            0x7

// The LSB[2:0] of each ADC sample is a random number. This variable is used
// to shift each new sample by 3 bits to create a larger random number.
#define ADC_RND_BIT_SHIFT           3

volatile uint32_t sample, random_number;

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
  init.prescale = ADC_PrescaleCalc(adcFreq, 0);
  init.timebase = ADC_TimebaseCalc(0);

  // This initialization is required to use the ADC as a random number
  // generator
  initSingle.diff       = true;               // Differential inputs
  initSingle.reference  = adcRefVEntropy;     // internal 2.5V reference
  initSingle.resolution = adcRes12Bit;        // 12-bit resolution
  initSingle.posSel     = adcPosSelVSS;       // POSSEL connected to VSS
  initSingle.negSel     = adcNegSelVSS;       // NEGSEL connected to VSS

  // Initialize the ADC
  ADC_Init(ADC0, &init);
  ADC_InitSingle(ADC0, &initSingle);
}

/**************************************************************************//**
 * @brief  Main function
 *****************************************************************************/
int main(void)
{
  uint32_t i;

  // Since this example generates a 32-bit random number and since the ADC
  // produces 3 bits of random data per sample, we will need to go through 11
  // rounds of cascading samples (11 x 3 bits = 33 bits) to get a 32-bit long
  // random number. Similarly, if an 8-bit random number is required, 3 rounds
  // of cascading ADC samples will be required.
  uint32_t rounds = 11;

  CHIP_Init();

  // Function to initialize the ADC
  initADC();

  // The VIN attenuator is used to widen the available input range of the ADC
  // beyond the reference source. This must be set to its maximum value (15) to
  // use the ADC as a Random Number Generator
  ADC0->SINGLECTRLX |= ADC_SINGLECTRLX_VINATT_MAX << \
      _ADC_SINGLECTRLX_VINATT_SHIFT;

  // Clear the ADC Single FIFO before starting the conversion
  ADC0->SINGLEFIFOCLEAR |= ADC_SINGLEFIFOCLEAR_SINGLEFIFOCLEAR;

  for (i = 0; i < rounds; i++){
    // Start conversion
    ADC_Start(ADC0, adcStartSingle);

    // Wait for conversion to be complete
    while (!(ADC0->IF & _ADC_IF_SINGLE_MASK));

    // Get ADC result and since only LSB[2:0] of a sample will be a random
    // number, the ADC_RND_BIT_MASK is used to only use the relevant bits and
    // discard the rest
    sample = (ADC_DataSingleGet(ADC0) & ADC_RND_BIT_MASK);

    // Start cascading samples to generate a large random number. This is
    // repeated "rounds" number of times. The random number is left shifted by
    // 3 bits every time a new sample is obtained. For example, if the first
    // sample is 2 and the second sample is 3, the resulting random number is
    // 0x1A (011010b) and this cascading continues.
    random_number |= sample << (i * ADC_RND_BIT_SHIFT);
  }

  // Infinite loop
  while (1);
}
