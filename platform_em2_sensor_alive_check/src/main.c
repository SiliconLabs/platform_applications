/**************************************************************************//**
 * @file
 * @brief This project demonstrates pulse width modulation using the LETIMER.
 * @version 0.0.1
 ******************************************************************************
 * @section License
 * <b>Copyright 2018 Silicon Labs, Inc. http://www.silabs.com</b>
 *******************************************************************************
 *
 * This file is licensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 *
 ******************************************************************************/

#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_cryotimer.h"
#include "em_timer.h"
#include "em_letimer.h"
#include "em_rtcc.h"
#include "em_wdog.h"
#include "em_prs.h"
#include "bsp.h"

#define D_LETIMER	1
#define D_WDOG		1
#define D_PRS		1

#define BSP_GPIO_POLL_STATUS_PORT	gpioPortC
#define BSP_GPIO_POLL_STATUS_PIN	10

#define BSP_GPIO_STATUS_PORT        gpioPortC
#define BSP_GPIO_STATUS_PIN         11


#if D_LETIMER
// Desired frequency in Hz
#define OUT_FREQ 2

// Duty cycle percentage
#define DUTY_CYCLE 25

#define PORTIO_LETIMER0_OUT0_PIN              (9U)
#define PORTIO_LETIMER0_OUT0_PORT             (gpioPortC)
#define PORTIO_LETIMER0_OUT0_LOC              (14U)

/**************************************************************************//**
 * @brief TIMER initialization
 *****************************************************************************/
void initLETIMER(void)
{
  LETIMER_Init_TypeDef letimerInit = LETIMER_INIT_DEFAULT;

  // Enable clock to the LE modules interface
  CMU_ClockEnable(cmuClock_HFLE, true);

  // Select LFXO for the LETIMER
  CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);
  CMU_ClockEnable(cmuClock_LETIMER0, true);

  // Reload COMP0 on underflow, pulse output, and run in repeat mode
  letimerInit.comp0Top = true;
  letimerInit.out0Pol = true;
  letimerInit.ufoa0 = letimerUFOAPwm;
  letimerInit.repMode = letimerRepeatFree;

  // Need REP0 != 0 to run PWM
  LETIMER_RepeatSet(LETIMER0, 0, 1);
  //LETIMER_RepeatSet(LETIMER0, 1, 1);

  /* Route LETIMER to location 0 (PD9 and PD10) and enable outputs */
  LETIMER0->ROUTELOC0 = (LETIMER0->ROUTELOC0
		& ~_LETIMER_ROUTELOC0_OUT0LOC_MASK)
		| PORTIO_LETIMER0_OUT0_LOC;
  LETIMER0->ROUTEPEN |= LETIMER_ROUTEPEN_OUT0PEN;

//  LETIMER0->ROUTELOC0 = (LETIMER0->ROUTELOC0
//		& ~_LETIMER_ROUTELOC0_OUT1LOC_MASK)
//		| PORTIO_LETIMER0_OUT1_LOC;
//  LETIMER0->ROUTEPEN |= LETIMER_ROUTEPEN_OUT1PEN;

  // Initialize and enable LETIMER
  LETIMER_Init(LETIMER0, &letimerInit );

  // Set COMP0 to desired PWM frequency
   LETIMER_CompareSet(LETIMER0, 0,
        CMU_ClockFreqGet(cmuClock_LETIMER0) / OUT_FREQ);

   // Set COMP1 to control duty cycle
   LETIMER_CompareSet(LETIMER0, 1,
        CMU_ClockFreqGet(cmuClock_LETIMER0) * DUTY_CYCLE / (OUT_FREQ * 100));
}
#endif

#if D_WDOG
/* Defining the watchdog initialization data */
WDOG_Init_TypeDef init =
{
  .enable     = true,                 /* Start watchdog when init done */
  .debugRun   = false,                /* WDOG not counting during debug halt */
  .em2Run     = true,                 /* WDOG counting when in EM2 */
  .em3Run     = true,                 /* WDOG counting when in EM3 */
  .em4Block   = false,                /* EM4 can be entered */
  .swoscBlock = false,                /* Do not block disabling LFRCO/LFXO in CMU */
  .lock       = false,                /* Do not lock WDOG configuration (if locked, reset needed to unlock) */
  .clkSel     = wdogClkSelLFXO,     /* Select 1kHZ WDOG oscillator */
  .perSel     = wdogPeriod_32k,        /* Set the watchdog period to 2049 clock periods (ie ~2 seconds) */
};
/* ISR */
/******************************************************************************
 * @brief WDOG Interrupt Handler. Clears interrupt flag.
 *        The interrupt table is in assembly startup file startup_efm32.s
 *
 *****************************************************************************/
void WDOG0_IRQHandler(void)
{
  WDOGn_IntClear(DEFAULT_WDOG, WDOG_IEN_TOUT);
  GPIO_PinOutToggle(BSP_GPIO_LED1_PORT, BSP_GPIO_LED1_PIN);
}

void initWDOG(void)
{
  init.enable = true;
  init.resetDisable = true;         /* Disable watchdog reset the device when fired */
  WDOG_Init(&init);

  /* Enable watchdog timeout interrupt */
  WDOGn_IntEnable(DEFAULT_WDOG, WDOG_IEN_TOUT);
  NVIC_EnableIRQ(WDOG0_IRQn);

  /* PRS rising edge as clear source */
  /* PRS rising edge of channel 0 as WDOG PCH[0] PRS source */
  WDOG0->PCH[0].PRSCTRL = WDOG_PCH_PRSCTRL_PRSSEL_PRSCH0;
  while(DEFAULT_WDOG->SYNCBUSY & WDOG_SYNCBUSY_PCH0_PRSCTRL) ;

  /* Enable PRS clear for watchdog */
  DEFAULT_WDOG->CTRL |= WDOG_CTRL_CLRSRC;
}

#endif

#if D_PRS
#define PRS_BASE_CH   0
#define USE_AND       true
/**************************************************************************//**
 * @brief PRS initialization
 *****************************************************************************/
void initPrs(void)
{
  // Enable PRS clock
  CMU_ClockEnable(cmuClock_PRS, true);

  PRS_SourceSignalSet(PRS_BASE_CH, PRS_CH_CTRL_SOURCESEL_PRSL, PRS_CH_CTRL_SIGSEL_PRSCH0, prsEdgePos);

  PRS_SourceAsyncSignalSet(PRS_BASE_CH, PRS_CH_CTRL_SOURCESEL_LETIMER0, PRS_CH_CTRL_SIGSEL_LETIMER0CH0);	//POLL
  PRS_SourceAsyncSignalSet(PRS_BASE_CH+1, PRS_CH_CTRL_SOURCESEL_GPIOH, BSP_GPIO_STATUS_PIN - 8);			//STATUS
  // Configure PRS Logic
  PRS->CH[PRS_BASE_CH].CTRL |= PRS_CH_CTRL_ANDNEXT; // Channel 0 will AND with Channel 1
  // Route PRS output to PC10,check the logic AND
  PRS_GpioOutputLocation(PRS_BASE_CH, 12); //STATUS&POLL
}
#endif

/**************************************************************************//**
 * @brief GPIO initialization
 *****************************************************************************/
void initGPIO(void)
{
  // Enable GPIO and clock
  CMU_ClockEnable(cmuClock_GPIO, true);
  // Set Push Buttons to input
  GPIO_PinModeSet(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, gpioModeInputPullFilter, 1);
  GPIO_PinModeSet(BSP_GPIO_PB1_PORT, BSP_GPIO_PB1_PIN, gpioModeInputPullFilter, 1);

  // Set LEDs to output
  GPIO_PinModeSet(BSP_GPIO_LED0_PORT, BSP_GPIO_LED0_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(BSP_GPIO_LED1_PORT, BSP_GPIO_LED1_PIN, gpioModePushPull, 0);

  // Configure Push Buttons to create interrupt signals
  //GPIO_ExtIntConfig(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, BSP_GPIO_PB0_PIN, 0, 0, false);
  //GPIO_ExtIntConfig(BSP_GPIO_PB1_PORT, BSP_GPIO_PB1_PIN, BSP_GPIO_PB1_PIN, 0, 0, false);

  // Configure LETIMER PWM output
  GPIO_PinModeSet(PORTIO_LETIMER0_OUT0_PORT, PORTIO_LETIMER0_OUT0_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(BSP_GPIO_POLL_STATUS_PORT, BSP_GPIO_POLL_STATUS_PIN, gpioModePushPull, 0);

  GPIO_PinModeSet(BSP_GPIO_STATUS_PORT, BSP_GPIO_STATUS_PIN, gpioModeInput, 1);//gpioModeInputPullFilter
  GPIO_ExtIntConfig(BSP_GPIO_STATUS_PORT, BSP_GPIO_STATUS_PIN, BSP_GPIO_STATUS_PIN, 0, 0, false);
}

/**************************************************************************//**
 * @brief  Main function
 *****************************************************************************/
int main(void)
{
  // Chip errata
  CHIP_Init();

  // Initializations
  initGPIO();
  #if D_LETIMER
  initLETIMER();
  #endif
  #if D_WDOG
  initWDOG();
  #endif
  #if D_PRS
  initPrs();
  #endif
  // Enter low energy state, PWM will continue
  // To change duty cycle, briefly wake from EM2 and change COMP1
  while (1)
  {
	  EMU_EnterEM2(true);
  }
}
