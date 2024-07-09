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
#include <stdint.h>

#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_lesense.h"
#include "em_adc.h"

#define BSP_GPIO_LED0_PORT   gpioPortH
#define BSP_GPIO_LED0_PIN    10
#define LESENSE_SCAN_FREQ    1 // LESENSE scan frequency set to 1Hz
#define adcFreq              16000000 // Init to max ADC clock for Series 1

#define NUM_INPUTS           2 // Set number of ADC inputs

static volatile uint32_t inputs[NUM_INPUTS]; // ADC result buffer
static uint8_t index = 0;

static void GPIO_config(void);
static void ADC_config(void);
static void LESENSE_config(void);

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  GPIO_config();
  ADC_config();
  LESENSE_config();
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
  EMU_EnterEM2(true);        // put system into EM2 mode
}

/***************************************************************************//**
 * @brief  GPIO configuration
 ******************************************************************************/
static void GPIO_config(void)
{
  // Configure GPIO Clock.
  CMU_ClockEnable(cmuClock_GPIO, true);
  // Configure LED0 and LED1 as a push pull output
  GPIO_PinModeSet(BSP_GPIO_LED0_PORT, BSP_GPIO_LED0_PIN, gpioModePushPull, 0);
}

/***************************************************************************//**
 * @brief  ADC Initializer
 * @detail Enable ADC with scan mode. Configure ADC scan channel to be
 *         triggered by LESENSE scan
 ******************************************************************************/
static void ADC_config(void)
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
  CMU_AUXHFRCOBandSet(cmuAUXHFRCOFreq_16M0Hz);
  init.timebase = ADC_TimebaseCalc(CMU_AUXHFRCOBandGet());
  init.prescale = ADC_PrescaleCalc(adcFreq, CMU_AUXHFRCOBandGet());
  init.em2ClockConfig = adcEm2ClockOnDemand;

  initScan.diff = 0;                 // single ended
  initScan.reference = adcRef2V5;    // internal 2.5V reference
  initScan.resolution = adcRes12Bit; // 12-bit resolution
  initScan.acqTime = adcAcqTime4;    // set acquisition time

  // Select ADC inputs. See README for corresponding EXP header pin.
  ADC_ScanSingleEndedInputAdd(&initScan,
                              adcScanInputGroup0,
                              adcPosSelAPORT3XCH8);
  ADC_ScanSingleEndedInputAdd(&initScan,
                              adcScanInputGroup0,
                              adcPosSelAPORT3YCH9);

  // Set scan data valid level (DVL) to 2
  ADC0->SCANCTRLX |= (NUM_INPUTS - 1) << _ADC_SCANCTRLX_DVL_SHIFT;

  // Clear ADC Scan fifo
  ADC0->SCANFIFOCLEAR = ADC_SCANFIFOCLEAR_SCANFIFOCLEAR;

  // Initialize ADC and Scan
  ADC_Init(ADC0, &init);
  ADC_InitScan(ADC0, &initScan);
}

/***************************************************************************//**
 * @brief LESENSE Configuration
 *        This functions configures and enables the LESENSE block
 ******************************************************************************/
static void LESENSE_config(void)
{
  LESENSE_Init_TypeDef initLesense = LESENSE_INIT_DEFAULT;
  LESENSE_ChDesc_TypeDef initLesenseCh = LESENSE_CH_CONF_DEFAULT;

  // Normal warm up mode
  initLesense.perCtrl.warmupMode = lesenseWarmupModeNormal;
  initLesense.coreCtrl.storeScanRes = false;  // Do not store scan results

  // Channel Configuration
  initLesenseCh.enaScanCh = true;  // Enable scan channel
  initLesenseCh.sampleDelay = 0x2;  // Two LF Clock cycle sample delay
  initLesenseCh.sampleMode = lesenseSampleModeADC;  // Scan mode ADC
  initLesenseCh.storeCntRes = true;

  CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);
  CMU_ClockEnable(cmuClock_HFLE, true);
  CMU_ClockEnable(cmuClock_LESENSE, true);

  // Initialize LESENSE interface
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
 * @brief  LESENSE interrupt handler
 * @detail Clear BUFDATA_VALID flag, read the ADC data sampled and toggle
 *         LED0.
 ******************************************************************************/
void LESENSE_IRQHandler(void)
{
  // Clear all LESENSE interrupt flag
  LESENSE_IntClear(LESENSE_IFC_BUFDATAV);
  inputs[index] = LESENSE->BUFDATA; // Read the unread ADC data
  index = (index + 1) % NUM_INPUTS; // Update buffer index

  // If entire scan sequence is complete, toggle LED0
  if (index == (NUM_INPUTS - 1)) {
    GPIO_PinOutToggle(BSP_GPIO_LED0_PORT, BSP_GPIO_LED0_PIN);
  }
}
