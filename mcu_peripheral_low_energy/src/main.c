#include <stdint.h>
#include <stdio.h>
#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_prs.h"
#include "em_adc.h"
#include "em_ldma.h"
#include "em_lcd.h"
#include "em_leuart.h"
#include "em_csen.h"
#include "em_letimer.h"

#include "graph.h"
#include "led.h"

#define BUTTON0_PORT	gpioPortC
#define BUTTON0_PIN		8

#define LDMA_CHANNEL	0

#define ADC_DVL			1		// Number of samples to get at once
#define BUFFER_SIZE		1		// For now, buffer size is 1 just for immediate testing

uint32_t ADCBuffer[BUFFER_SIZE];

LDMA_TransferCfg_t ldmaTxCfg = LDMA_TRANSFER_CFG_PERIPHERAL(ldmaPeripheralSignal_ADC0_SINGLE);;
LDMA_Descriptor_t ldmaDescr = LDMA_DESCRIPTOR_SINGLE_P2M_BYTE(&(ADC0->SINGLEDATA),
							      ADCBuffer,
							      BUFFER_SIZE);

LDMA_TransferCfg_t leuartXfer = LDMA_TRANSFER_CFG_PERIPHERAL(ldmaPeripheralSignal_LEUART0_TXEMPTY);
LDMA_Descriptor_t leuartDescr = LDMA_DESCRIPTOR_SINGLE_M2P_BYTE(ADCBuffer, &(LEUART0->TXDATA), BUFFER_SIZE);

void GPIO_Open(void);
void ADC0_Open(void);
void LETimer_Open(void);
void GPIOtoADC_PRS_Setup(void);
void LETimertoADC_PRS_Setup(void);
void LDMA_Setup(void);

int main(void)
{
  /* Chip errata */
  CHIP_Init();

  EMU_DCDCInit_TypeDef dcdcInit = EMU_DCDCINIT_DEFAULT;
  EMU_DCDCInit(&dcdcInit);

  EMU_EM23Init_TypeDef em23Init = EMU_EM23INIT_DEFAULT;
  em23Init.vScaleEM23Voltage = emuVScaleEM23_LowPower;
  EMU_EM23Init(&em23Init);

  DebugLED_Init();

  Graph_Init();

//GPIOtoADC_PRS_Setup();
  LETimertoADC_PRS_Setup();

  LDMA_Setup();

  // clear buffer
  for(int i = 0; i < BUFFER_SIZE; i++){
    ADCBuffer[i] = 0;
  }

  uint32_t x = 0;

  /* Infinite loop */
  while (1) {
    EMU_EnterEM2(true);

    Graph_Plot(x, ADCBuffer[0], Red);

    ADCBuffer[0] = 0;

    // To make point visible, each point is 2x2 pixels. The + 2 is to keep everything even.
    x = (x + 2) % GRAPH_WIDTH;

//    DebugLED_Toggle();

  }
}

void GPIOtoADC_PRS_Setup(void)
{
  /* Producer like normal : GPIO */
  GPIO_Open();

  CMU_ClockEnable(cmuClock_PRS, true);
  PRS_SourceAsyncSignalSet(0, PRS_CH_CTRL_SOURCESEL_GPIOH,
		  PRS_CH_CTRL_SIGSEL_GPIOPIN8);

  ADC0_Open();

}

void LETimertoADC_PRS_Setup(void)
{
  LETimer_Open();
  CMU_ClockEnable(cmuClock_PRS, true);
  PRS_SourceAsyncSignalSet(0,
			  PRS_CH_CTRL_SOURCESEL_LETIMER0,
			  PRS_CH_CTRL_SIGSEL_LETIMER0CH0);
  ADC0_Open();
}

void GPIO_Open(void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);
  GPIO_PinModeSet(BUTTON0_PORT, BUTTON0_PIN, gpioModeInput, 1);
  GPIO_IntConfig(BUTTON0_PORT, BUTTON0_PIN, false, false, false);
}

void ADC0_Open(void)
{
  /*	Configure for ADC conversion to be triggered whenever GPIO is pressed.
	  The following code was referenced off of the peripheral examples github repo.
	  Found actual correct documentation to initialize ADC clocks to handle EM2 in correct
		  rm. (GG11 rm, not GG)
	  Enable clocks to perform in energy mode 2 */
  CMU_ClockEnable(cmuClock_HFPER, true);
  CMU_ClockEnable(cmuClock_ADC0, true);

  /* Select AUXHFRCO for ADC ASYNC mode so it can run in EM2 */
  CMU->ADCCTRL = CMU_ADCCTRL_ADC0CLKSEL_AUXHFRCO;

  ADC_Init_TypeDef adcInit = ADC_INIT_DEFAULT;
  adcInit.em2ClockConfig = adcEm2ClockOnDemand;
  adcInit.timebase = ADC_TimebaseCalc(CMU_AUXHFRCOBandGet());
  adcInit.prescale = ADC_PrescaleCalc(16000000, CMU_AUXHFRCOBandGet());

  ADC_InitSingle_TypeDef adcSingleInit = ADC_INITSINGLE_DEFAULT;
  adcSingleInit.singleDmaEm2Wu = true;
  adcSingleInit.reference = adcRefVDD;
  //adcSingleInit.posSel = adcPosSelAPORT3YCH9;
  adcSingleInit.posSel = adcPosSelAPORT4XCH11;
  adcSingleInit.prsSel = adcPRSSELCh0;
  adcSingleInit.prsEnable = true;
  ADC_Init(ADC0, &adcInit);
  ADC_InitSingle(ADC0, &adcSingleInit);
}

void LDMA_Setup(void)
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

void LETimer_Open(void)
{
  CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);

  CMU_ClockEnable(cmuClock_HFLE, true);

  // Enable clock for LETIMER0
  CMU_ClockEnable(cmuClock_LETIMER0, true);

  LETIMER_Init_TypeDef letimerInit = LETIMER_INIT_DEFAULT;
  letimerInit.comp0Top = true;
  letimerInit.ufoa0 = letimerUFOAPulse;
  LETIMER_Init(LETIMER0, &letimerInit);

  //LETIMER_IntEnable(LETIMER0, LETIMER_IF_COMP0);

  LETIMER_CompareSet(LETIMER0, 0, CMU_ClockFreqGet(cmuClock_LETIMER0) / 480);

  // Need REP0 != 0 to pulse on underflow
  LETIMER_RepeatSet(LETIMER0, 0, 1);

  NVIC_ClearPendingIRQ(LETIMER0_IRQn);
  //NVIC_EnableIRQ(LETIMER0_IRQn);	// No interrupt
}

// Not used
void LETIMER0_IRQHandler(void)
{
  LETIMER_IntClear(LETIMER0, LETIMER_IF_COMP0);
}

void LDMA_IRQHandler(void)
{
  LDMA_IntClear(LDMA_IntGet());
  //DebugLED_Toggle();
  LDMA_StartTransfer(0, &ldmaTxCfg, &ldmaDescr);
}


