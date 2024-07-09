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
#include <stdio.h>

#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_prs.h"
#include "em_adc.h"
#include "em_ldma.h"
#include "em_letimer.h"

#include "graph.h"

// Uncomment for selecting the triggering source
// #define GPIO_TO_ADC_PRS_MODE
#define LETIMER_TO_ADC_PRS_MODE

#define BUTTON0_PORT  gpioPortC
#define BUTTON0_PIN   8

#define LDMA_CHANNEL  0

#define ADC_DVL       1  // Number of samples to get at once
#define BUFFER_SIZE   1  // For now, buffer size is 1 just for immediate testing

static uint32_t ADCBuffer[BUFFER_SIZE];

LDMA_TransferCfg_t ldmaTxCfg = LDMA_TRANSFER_CFG_PERIPHERAL(
  ldmaPeripheralSignal_ADC0_SINGLE);
LDMA_Descriptor_t ldmaDescr = LDMA_DESCRIPTOR_SINGLE_P2M_BYTE(
  &(ADC0->SINGLEDATA), ADCBuffer, BUFFER_SIZE);

LDMA_TransferCfg_t leuartXfer = LDMA_TRANSFER_CFG_PERIPHERAL(
  ldmaPeripheralSignal_LEUART0_TXEMPTY);
LDMA_Descriptor_t leuartDescr =
  LDMA_DESCRIPTOR_SINGLE_M2P_BYTE(ADCBuffer, &(LEUART0->TXDATA), BUFFER_SIZE);

#if defined(GPIO_TO_ADC_PRS_MODE)
static void GPIOtoADC_PRS_Setup(void);
static void GPIO_setup(void);

#endif

#if defined(LETIMER_TO_ADC_PRS_MODE)
static void LETIMER_setup(void);
static void LETIMERtoADC_PRS_Setup(void);

#endif

static void ADC_setup(void);
static void LDMA_setup(void);

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  Graph_Init();

#if defined(GPIO_TO_ADC_PRS_MODE)
  GPIOtoADC_PRS_Setup();
#elif defined(LETIMER_TO_ADC_PRS_MODE)
  LETIMERtoADC_PRS_Setup();
#endif

  LDMA_setup();

  // clear buffer
  for (int i = 0; i < BUFFER_SIZE; i++) {
    ADCBuffer[i] = 0;
  }
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
  static uint32_t x = 0;
  EMU_EnterEM2(true);

  Graph_Plot(x, ADCBuffer[0], Red);

  ADCBuffer[0] = 0;

  // To make point visible, each point is 2x2 pixels. The + 2 is to keep
  //   everything even.
  x = (x + 2) % GRAPH_WIDTH;
}

#if defined(GPIO_TO_ADC_PRS_MODE)
static void GPIOtoADC_PRS_Setup(void)
{
  /* Producer like normal : GPIO */
  GPIO_setup();

  CMU_ClockEnable(cmuClock_PRS, true);
  PRS_SourceAsyncSignalSet(0, PRS_CH_CTRL_SOURCESEL_GPIOH,
                           PRS_CH_CTRL_SIGSEL_GPIOPIN8);

  ADC_setup();
}

static void GPIO_setup(void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);
  GPIO_PinModeSet(BUTTON0_PORT, BUTTON0_PIN, gpioModeInput, 1);
  GPIO_ExtIntConfig(BUTTON0_PORT,
                    BUTTON0_PIN,
                    BUTTON0_PIN, true, false, false);
}

#endif

#if defined(LETIMER_TO_ADC_PRS_MODE)
static void LETIMERtoADC_PRS_Setup(void)
{
  LETIMER_setup();
  CMU_ClockEnable(cmuClock_PRS, true);
  PRS_SourceAsyncSignalSet(0,
                           PRS_CH_CTRL_SOURCESEL_LETIMER0,
                           PRS_CH_CTRL_SIGSEL_LETIMER0CH0);
  ADC_setup();
}

static void LETIMER_setup(void)
{
  CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);

  CMU_ClockEnable(cmuClock_HFLE, true);

  // Enable clock for LETIMER0
  CMU_ClockEnable(cmuClock_LETIMER0, true);

  LETIMER_Init_TypeDef letimerInit = LETIMER_INIT_DEFAULT;
  letimerInit.comp0Top = true;
  letimerInit.ufoa0 = letimerUFOAPulse;
  LETIMER_Init(LETIMER0, &letimerInit);

  LETIMER_CompareSet(LETIMER0, 0, CMU_ClockFreqGet(cmuClock_LETIMER0) / 480);

  // Need REP0 != 0 to pulse on underflow
  LETIMER_RepeatSet(LETIMER0, 0, 1);

  NVIC_ClearPendingIRQ(LETIMER0_IRQn);
}

#endif

static void ADC_setup(void)
{
  /*  Configure for ADC conversion to be triggered whenever GPIO is pressed.
   *    The following code was referenced off of the peripheral examples github
   *   repo.
   *    Found actual correct documentation to initialize ADC clocks to handle
   *   EM2 in correct
   *     rm. (GG11 rm, not GG)
   *    Enable clocks to perform in energy mode 2 */
  CMU_ClockEnable(cmuClock_HFPER, true);
  CMU_ClockEnable(cmuClock_ADC0, true);

  /* Select AUXHFRCO for ADC ASYNC mode so it can run in EM2 */
  CMU->ADCCTRL = CMU_ADCCTRL_ADC0CLKSEL_AUXHFRCO;

  ADC_Init_TypeDef init = ADC_INIT_DEFAULT;
  CMU_AUXHFRCOBandSet(cmuAUXHFRCOFreq_16M0Hz);
  init.em2ClockConfig = adcEm2ClockOnDemand;
  init.timebase = ADC_TimebaseCalc(CMU_AUXHFRCOBandGet());
  init.prescale = ADC_PrescaleCalc(16000000, CMU_AUXHFRCOBandGet());

  ADC_InitSingle_TypeDef initSingle = ADC_INITSINGLE_DEFAULT;
  initSingle.singleDmaEm2Wu = true;
  initSingle.reference = adcRefVDD;
  initSingle.posSel = adcPosSelAPORT4XCH11;
  initSingle.prsSel = adcPRSSELCh0;
  initSingle.prsEnable = true;
  ADC_Init(ADC0, &init);
  ADC_InitSingle(ADC0, &initSingle);
}

static void LDMA_setup(void)
{
  CMU_ClockEnable(cmuClock_LDMA, true);
  LDMA_Init_t ldmaInit = LDMA_INIT_DEFAULT;
  LDMA_Init(&ldmaInit);

  ldmaDescr.xfer.decLoopCnt = true;
  ldmaDescr.xfer.doneIfs = true;
  ldmaDescr.xfer.blockSize = ADC_DVL - 1;
  ldmaDescr.xfer.ignoreSrec = true;
  ldmaDescr.xfer.size = ldmaCtrlSizeWord;

  LDMA_StartTransfer(LDMA_CHANNEL, &ldmaTxCfg, &ldmaDescr);

  // Interrupt whenever dma completed the transfer
  LDMA_IntEnable(LDMA_IF_DONE_DEFAULT);

  NVIC_ClearPendingIRQ(LDMA_IRQn);
  NVIC_EnableIRQ(LDMA_IRQn);
}

void LDMA_IRQHandler(void)
{
  LDMA_IntClear(LDMA_IntGet());
  LDMA_StartTransfer(0, &ldmaTxCfg, &ldmaDescr);
}
