/***************************************************************************//**
 * @file peripherals.c
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

#include <peripherals.h>
#include <stdbool.h>

#include "em_chip.h"
#include "em_cmu.h"
#include "em_adc.h"
#include "em_ldma.h"
#include "em_gpio.h"
#include "em_prs.h"
#include "dmadrv.h"
#include "os.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/
#define ADC_FREQ          (4000000)

// Change this to increase or decrease number of samples.
#define ADC_BUFFER_SIZE   4

// Change this to set how many samples get sent at once
#define ADC_DVL           (1)

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/

// Buffer for ADC single conversion
static uint32_t adcBuffer1[ADC_BUFFER_SIZE];
static uint32_t adcBuffer0[ADC_BUFFER_SIZE];

static LDMA_TransferCfg_t ADC0Transfer =
  (LDMA_TransferCfg_t)LDMA_TRANSFER_CFG_PERIPHERAL(
    ldmaPeripheralSignal_ADC0_SINGLE);

static LDMA_Descriptor_t ADC0_MemLoop =
  LDMA_DESCRIPTOR_LINKREL_P2M_WORD(&(ADC0->SINGLEDATA),
                                   &adcBuffer0[0],
                                   ADC_BUFFER_SIZE,
                                   0);

static LDMA_TransferCfg_t ADC1Transfer =
  (LDMA_TransferCfg_t)LDMA_TRANSFER_CFG_PERIPHERAL(
    ldmaPeripheralSignal_ADC1_SINGLE);

static LDMA_Descriptor_t ADC1_MemLoop =
  LDMA_DESCRIPTOR_LINKREL_P2M_WORD(&(ADC1->SINGLEDATA),
                                   &adcBuffer1[0],
                                   ADC_BUFFER_SIZE,
                                   0);

static unsigned int DMA_ADC0_CH = 0;
static unsigned int DMA_ADC1_CH = 1;

extern OS_SEM sem_adc0;
extern OS_SEM sem_adc1;

/*******************************************************************************
 **************************    LOCAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 *  DMA channel 0 callback function.
 ******************************************************************************/
static bool dma_adc0_callback(unsigned int channel,
                              unsigned int sequenceNo,
                              void *userParam)
{
  (void)channel;
  (void)sequenceNo;
  (void)userParam;

  RTOS_ERR err;

  OSSemPost(&sem_adc0,
            OS_OPT_POST_ALL,
            &err);
  EFM_ASSERT((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE));

  return false;
}

/***************************************************************************//**
 *  DMA channel 1 callback function.
 ******************************************************************************/
static bool dma_adc1_callback(unsigned int channel,
                              unsigned int sequenceNo,
                              void *userParam)
{
  (void)channel;
  (void)sequenceNo;
  (void)userParam;

  RTOS_ERR err;

  OSSemPost(&sem_adc1,
            OS_OPT_POST_ALL,
            &err);
  EFM_ASSERT((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE));

  return false;
}

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * Initialize ADC
 *
 * ADC0 and ADC1 are configured to be triggered by GPIO (BTN0) via PRS.
 * ADC0 samples the signal on ch13 (PA13) while ADC1 samples the signal
 * on ch11 (PE11).
 * The data sampled by each ADC is then stored in their FIFO.
 ******************************************************************************/
void adc_init(void)
{
  // Declare init structs
  ADC_Init_TypeDef init = ADC_INIT_DEFAULT;
  ADC_InitSingle_TypeDef initSingle0 = ADC_INITSINGLE_DEFAULT;
  ADC_InitSingle_TypeDef initSingle1 = ADC_INITSINGLE_DEFAULT;

  // Enable ADC clock
  CMU_ClockEnable(cmuClock_HFPER, true);
  CMU_ClockEnable(cmuClock_ADC0, true);
  CMU_ClockEnable(cmuClock_ADC1, true);

  // Select AUXHFRCO for ADC ASYNC mode so it can run in EM2
  CMU->ADCCTRL |=
    (CMU_ADCCTRL_ADC0CLKSEL_AUXHFRCO | CMU_ADCCTRL_ADC1CLKSEL_AUXHFRCO);

  // Set AUXHFRCO frequency and use it to setup the ADC
  CMU_AUXHFRCOBandSet(cmuAUXHFRCOFreq_4M0Hz);
  init.timebase = ADC_TimebaseCalc(CMU_AUXHFRCOBandGet());
  init.prescale = ADC_PrescaleCalc(ADC_FREQ, CMU_AUXHFRCOBandGet());

  // Let the ADC enable its clock on demand in EM2
  init.em2ClockConfig = adcEm2ClockOnDemand;

  // DMA is available in EM2 for processing SINGLEFIFO DVL request
  initSingle0.singleDmaEm2Wu = 1;
  initSingle1.singleDmaEm2Wu = 1;

  // Add external ADC input. See README for corresponding EXP header pin.
  // ADC0 uses Ch13 as input and is connected to the PA13 pin.
  // ADC1 uses ch11 as input and is connected to the PE11 pin.
  initSingle0.posSel = adcPosSelAPORT2XCH13;
  initSingle1.posSel = adcPosSelAPORT3YCH11;

  // Basic ADC0 single configuration
  initSingle0.diff = false;                // single-ended
  initSingle0.reference = adcRef2V5;       // 2.5V reference
  initSingle0.resolution = adcRes12Bit;    // 12-bit resolution
  initSingle0.acqTime = adcAcqTime4;       // set acquisition time to meet
                                           // minimum requirements
  // Basic ADC1 single configuration
  initSingle1.diff = false;                // single-ended
  initSingle1.reference = adcRef2V5;       // 2.5V reference
  initSingle1.resolution = adcRes12Bit;    // 12-bit resolution
  initSingle1.acqTime = adcAcqTime4;       // set acquisition time to meet
                                           // minimum requirements

  // Enable PRS trigger and select channel 0
  initSingle0.prsEnable = true;
  initSingle0.prsSel = adcPRSSELCh0;
  // Enable PRS trigger and select channel 0
  initSingle1.prsEnable = true;
  initSingle1.prsSel = adcPRSSELCh0;

  // Initialize ADC0 and ADC1
  ADC_Init(ADC0, &init);
  ADC_Init(ADC1, &init);
  ADC_InitSingle(ADC0, &initSingle0);
  ADC_InitSingle(ADC1, &initSingle1);

  // Clear the Single FIFO
  ADC0->SINGLEFIFOCLEAR = ADC_SINGLEFIFOCLEAR_SINGLEFIFOCLEAR;

  // Clear the Single FIFO
  ADC1->SINGLEFIFOCLEAR = ADC_SINGLEFIFOCLEAR_SINGLEFIFOCLEAR;
}

/***************************************************************************//**
 * Initialize DMADRV
 *
 * The LDMA is configured to start reading data from the ADC0 and ADC1 FIFO,
 * one sample from each FIFO. The samples are then stored in their individual
 * buffers. Currently it is configured to store 4 such samples and then print
 * them before overwriting the individual buffers in memory.
 ******************************************************************************/
void dma_init(void)
{
  Ecode_t ecode;

  ecode = DMADRV_Init();
  if ((ecode != ECODE_OK)
      && (ecode != ECODE_EMDRV_DMADRV_ALREADY_INITIALIZED)) {
    return;
  }

  ADC0_MemLoop.xfer.decLoopCnt = true;

  ecode = DMADRV_AllocateChannel(&DMA_ADC0_CH, NULL);
  if (ecode != ECODE_EMDRV_DMADRV_OK) {
    return;
  }

  ecode = DMADRV_AllocateChannel(&DMA_ADC1_CH, NULL);
  if (ecode != ECODE_EMDRV_DMADRV_OK) {
    return;
  }

  // Initialize LDMA transfers
  DMADRV_LdmaStartTransfer(DMA_ADC0_CH,
                           &ADC0Transfer,
                           &ADC0_MemLoop,
                           dma_adc0_callback,
                           NULL);
  DMADRV_LdmaStartTransfer(DMA_ADC1_CH,
                           &ADC1Transfer,
                           &ADC1_MemLoop,
                           dma_adc1_callback,
                           NULL);
}

/***************************************************************************//**
 * Initialize DMA transfers
 ******************************************************************************/
void dma_tx(void)
{
  // Initialize LDMA transfers
  DMADRV_LdmaStartTransfer(DMA_ADC0_CH,
                           &ADC0Transfer,
                           &ADC0_MemLoop,
                           dma_adc0_callback,
                           NULL);
  DMADRV_LdmaStartTransfer(DMA_ADC1_CH,
                           &ADC1Transfer,
                           &ADC1_MemLoop,
                           dma_adc1_callback,
                           NULL);
}

/***************************************************************************//**
 * Initialize GPIO
 *
 * Configure BTN0 as input and to generate PRS interrupt signals when pressed.
 ******************************************************************************/
void gpio_init(void)
{
  // Enable clock for GPIO
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Set Push Button 0 to input
  GPIO_PinModeSet(gpioPortC, 8, gpioModeInput, 0);

  // Configure Push Button 0 to create PRS interrupt signals only
  GPIO_ExtIntConfig(gpioPortC, 8, 8, false, false, false);

  // Use GPIO PB0 as async PRS to trigger ADC in EM2
  CMU_ClockEnable(cmuClock_PRS, true);

  PRS_SourceAsyncSignalSet(0, PRS_CH_CTRL_SOURCESEL_GPIOH, 0);
}

/***************************************************************************//**
 * Wrapper function to access array
 ******************************************************************************/
uint32_t *get_adc_data1(void)
{
  return (&adcBuffer1[0]);
}

/***************************************************************************//**
 * Wrapper function to access array
 ******************************************************************************/
uint32_t *get_adc_data0(void)
{
  return (&adcBuffer0[0]);
}
