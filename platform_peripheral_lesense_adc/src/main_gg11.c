/***************************************************************************//**
 * @file main_gg11.c
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

#include <stdint.h>

#include "em_device.h"
#include "em_adc.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_core.h"
#include "em_lesense.h"

#include "bspconfig.h"
#include "bsp.h"

/***************************************************************************//**
 * Macro definitions*/
 /******************************************************************************/

  #define LESENSE_SCAN_FREQ 1  // LESENSE scan frequency set to 1Hz
  #define adcFreq   16000000   // Init to max ADC clock for Series 1

  #define NUM_INPUTS  2        // Set number of ADC inputs

  uint32_t inputs[NUM_INPUTS]; // ADC result buffer
  uint8_t index = 0;

/**************************************************************************//**
 * @brief  ADC Initializer
 * @detail Enable ADC with scan mode. Configure ADC scan channel to be
 *         triggered by LESENSE scan
 *****************************************************************************/
void initADC (void)
{
  // Enable ADC0 clock
  CMU_ClockEnable(cmuClock_ADC0, true);
  CMU_ClockEnable(cmuClock_HFPER, true);

  // Declare init structs
  ADC_Init_TypeDef init = ADC_INIT_DEFAULT;
  ADC_InitScan_TypeDef initScan = ADC_INITSCAN_DEFAULT;

  // Select AUXHFRCO for ADC ASYNC mode so it can run in EM2
  CMU->ADCCTRL = CMU_ADCCTRL_ADC0CLKSEL_AUXHFRCO;

  // Set AUXHFRCO frequency and use it to setup the ADC
  CMU_AUXHFRCOFreqSet(cmuAUXHFRCOFreq_16M0Hz);
  init.timebase = ADC_TimebaseCalc(CMU_AUXHFRCOBandGet());
  init.prescale = ADC_PrescaleCalc(adcFreq, CMU_AUXHFRCOBandGet());
  init.em2ClockConfig = adcEm2ClockOnDemand;

  initScan.diff       = 0;            // single ended
  initScan.reference  = adcRef2V5;    // internal 2.5V reference
  initScan.resolution = adcRes12Bit;  // 12-bit resolution
  initScan.acqTime    = adcAcqTime4;  // set acquisition time to meet minimum requirement

  // Select ADC inputs. See README for corresponding EXP header pin.
  ADC_ScanSingleEndedInputAdd(&initScan, adcScanInputGroup0, adcPosSelAPORT3XCH8);
  ADC_ScanSingleEndedInputAdd(&initScan, adcScanInputGroup0, adcPosSelAPORT3YCH9);

  // Set scan data valid level (DVL) to 2
  ADC0->SCANCTRLX |= (NUM_INPUTS - 1) << _ADC_SCANCTRLX_DVL_SHIFT;

  // Clear ADC Scan fifo
  ADC0->SCANFIFOCLEAR = ADC_SCANFIFOCLEAR_SCANFIFOCLEAR;

  // Initialize ADC and Scan
  ADC_Init(ADC0, &init);
  ADC_InitScan(ADC0, &initScan);
}

/**************************************************************************//**
 * @brief GPIO initialization
 *****************************************************************************/
void initGPIO(void)
{
  // Enable GPIO clock
  CMU_ClockEnable(cmuClock_GPIO, true);

  //Configure LED0 for output
  GPIO_PinModeSet(BSP_GPIO_LED0_PORT, BSP_GPIO_LED0_PIN, gpioModePushPull, 1);
}

/****************************************************************************//*
 * @brief  Sets up the LESENSE
 * @detail Set up LESENSE to trigger ADC scan conversion during sampling
 *         phase. LESENSE channel should match with ADC scan channel to
 *         ensure the correct ADC input channel is scanned
 ******************************************************************************/
void setupLESENSE(void)
{
  LESENSE_Init_TypeDef initLesense = LESENSE_INIT_DEFAULT;
  LESENSE_ChDesc_TypeDef initLesenseCh = LESENSE_CH_CONF_DEFAULT;

  // Normal warm up mode
  initLesense.perCtrl.warmupMode = lesenseWarmupModeNormal;
  initLesense.coreCtrl.storeScanRes = false;  // Do not store scan results

  // Channel Configuration
  initLesenseCh.enaScanCh = true;  // Enable scan channel
  initLesenseCh.sampleDelay = 0x2;	// Two LF Clock cycle sample delay
  initLesenseCh.sampleMode = lesenseSampleModeADC;  // Scan mode ADC
  initLesenseCh.storeCntRes = true;

  CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);
  CMU_ClockEnable(cmuClock_HFLE, true);
  CMU_ClockEnable(cmuClock_LESENSE, true);

  //Initialize LESENSE interface
  LESENSE_Init(&initLesense, true);

  // Configure channel 0, 1
  LESENSE_ChannelConfig(&initLesenseCh, 0);
  LESENSE_ChannelConfig(&initLesenseCh, 1);

  // Set scan frequency to desired frequency
  LESENSE_ScanFreqSet(0, LESENSE_SCAN_FREQ);

  // Set clock divisor for LF clock
  LESENSE_ClkDivSet(lesenseClkLF, lesenseClkDiv_1);

  LESENSE_IntEnable(LESENSE_IEN_BUFDATAV);
  NVIC_ClearPendingIRQ(LESENSE_IRQn);

  // Enable interrupt in NVIC
  NVIC_EnableIRQ(LESENSE_IRQn);

  // Start continuous scan
  LESENSE_ScanStart();
}

/***************************************************************************//**
 * @brief  Main function
 ******************************************************************************/
int main(void)
{
  // Initialize DCDC for series 1 board
  EMU_DCDCInit_TypeDef dcdcInit = EMU_DCDCINIT_STK_DEFAULT;

  // Chip errata
  CHIP_Init();

  // Init DCDC regulator with kit specific parameters
  EMU_DCDCInit(&dcdcInit);

  initADC();
  // Initialize GPIO
  initGPIO();
  // Initialize LESENSE
  setupLESENSE();

  while(1) {
    EMU_EnterEM2(true);        //put system into EM2 mode
  }
}

/****************************************************************************//*
 * @brief  LESENSE interrupt handler
 * @detail Clear BUFDATA_VALID flag, read the ADC data sampled and toggle
 *         LED0.
 ******************************************************************************/
void LESENSE_IRQHandler(void)
{
  // Clear all LESENSE interrupt flag
  LESENSE_IntClear(LESENSE_IFC_BUFDATAV);
  inputs[index] = LESENSE -> BUFDATA; // Read the unread ADC data
  index = (index+1)%NUM_INPUTS; // Update buffer index

  // If entire scan sequence is complete, toggle LED0
  if(index == (NUM_INPUTS-1)){
    GPIO_PinOutToggle(BSP_GPIO_LED0_PORT, BSP_GPIO_LED0_PIN);
  }
}

