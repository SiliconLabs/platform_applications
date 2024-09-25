/***************************************************************************//**
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
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

#include "em_chip.h"
#include "em_cmu.h"
#include "em_device.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_prs.h"
#include "em_timer.h"

#include "sl_emlib_gpio_simple_init.h"
#include "sl_emlib_gpio_init_timer0_config.h"
#include "sl_emlib_gpio_init_timer1_config.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

#define PRS_MODE                  1

#define TIMER0_OUT_FREQ           1

#define TIMER1_TOP                2

#if (PRS_MODE)
#define TIMER0_PRS_CHANNEL        0
#define TIMER1_PRS_CHANNEL        1
#endif

/**************************************************************************//**
 * @brief CMU initialization
 *****************************************************************************/
static void initCMU(void)
{
  // Enable peripheral clocks
#if (PRS_MODE)
  CMU_ClockEnable(cmuClock_PRS, true);
#endif

  CMU_ClockEnable(cmuClock_TIMER0, true);

  CMU_ClockEnable(cmuClock_TIMER1, true);
}

#if (PRS_MODE)

/**************************************************************************//**
 * @brief PRS initialization
 *****************************************************************************/
static void initPRS(void)
{
  // Connect TIMER0's output to a PRS channel
  PRS_SourceAsyncSignalSet(TIMER0_PRS_CHANNEL,
                           PRS_ASYNC_CH_CTRL_SOURCESEL_TIMER0,
                           _PRS_ASYNC_CH_CTRL_SIGSEL_TIMER0CC0);

  // Connect TIMER1's output to a PRS channel
  PRS_SourceAsyncSignalSet(TIMER1_PRS_CHANNEL,
                           PRS_ASYNC_CH_CTRL_SOURCESEL_TIMER1,
                           _PRS_ASYNC_CH_CTRL_SIGSEL_TIMER1CC0);
}

#endif

/**************************************************************************//**
 * @brief TIMER0 initialization
 *****************************************************************************/
static void initTIMER0(void)
{
  uint32_t timerFreq, topValue;
  TIMER_Init_TypeDef timer0_Init = TIMER_INIT_DEFAULT;
  TIMER_InitCC_TypeDef timer0_CCInit = TIMER_INITCC_DEFAULT;

  // Don't start counter on initialization
  timer0_Init.enable = false;

  // Configure capture/compare channel for output compare
  timer0_CCInit.mode = timerCCModeCompare;
  timer0_CCInit.cofoa = timerOutputActionToggle;

#if (PRS_MODE)
  // Configure the output to create PRS pulses
  timer0_CCInit.prsOutput = timerPrsOutputPulse;
#endif

  // Timer initialization
  TIMER_Init(TIMER0, &timer0_Init);

  // Route CC0 output to a GPIO
  GPIO->TIMERROUTE[0].ROUTEEN = GPIO_TIMER_ROUTEEN_CC0PEN;
  GPIO->TIMERROUTE[0].CC0ROUTE =
    (SL_EMLIB_GPIO_INIT_TIMER0_PORT << _GPIO_TIMER_CC0ROUTE_PORT_SHIFT)
    | (SL_EMLIB_GPIO_INIT_TIMER0_PIN <<
       _GPIO_TIMER_CC0ROUTE_PIN_SHIFT);

  // Timer Compare/Capture channel 0 initialization
  TIMER_InitCC(TIMER0, 0, &timer0_CCInit);

  /*
   * Set the TOP register value.  Each time the counter overflows TOP
   * is one half of the signal period.
   */
  timerFreq = CMU_ClockFreqGet(cmuClock_TIMER0) / (timer0_Init.prescale + 1);
  topValue = timerFreq / (2 * TIMER0_OUT_FREQ) - 1;
  TIMER_TopSet(TIMER0, topValue);

  // Enable TIMER0 interrupts
  TIMER_IntEnable(TIMER0, TIMER_IEN_CC0 | TIMER_IEN_OF);
  NVIC_ClearPendingIRQ(TIMER0_IRQn);
  NVIC_EnableIRQ(TIMER0_IRQn);

  // Now start the TIMER
  TIMER_Enable(TIMER0, true);
}

/**************************************************************************//**
 * @brief TIMER1 initialization
 *****************************************************************************/
static void initTIMER1(void)
{
  TIMER_Init_TypeDef timer1_Init = TIMER_INIT_DEFAULT;
  TIMER_InitCC_TypeDef timer1_CC0Init = TIMER_INITCC_DEFAULT;

  // Don't start counter on initialization
  timer1_Init.enable = false;
#if (PRS_MODE)
  // Use CC1 as a PRS input
  TIMER_InitCC_TypeDef timer1_CC1Init = TIMER_INITCC_DEFAULT;

  // Configure capture/compare channel for input capture
  timer1_Init.clkSel = timerClkSelCC1;
  timer1_CC1Init.mode = timerCCModeCapture;

  // Select the TIMER0's PRS signal
  timer1_CC1Init.prsSel = TIMER0_PRS_CHANNEL;
  timer1_CC1Init.prsInput = true;
  timer1_CC1Init.prsInputType = timerPrsInputAsyncPulse;
#else
  // Select the previous timer's (TIMER0) output as a clock source
  timer1_Init.clkSel = timerClkSelCascade;
#endif
  // Configure capture/compare channel for output compare
  timer1_CC0Init.mode = timerCCModeCompare;
  timer1_CC0Init.cofoa = timerOutputActionToggle;

  // Timer initialization
  TIMER_Init(TIMER1, &timer1_Init);

  // Route CC0 output to a GPIO
  GPIO->TIMERROUTE[1].ROUTEEN = GPIO_TIMER_ROUTEEN_CC0PEN;
  GPIO->TIMERROUTE[1].CC0ROUTE =
    (SL_EMLIB_GPIO_INIT_TIMER1_PORT << _GPIO_TIMER_CC0ROUTE_PORT_SHIFT)
    | (SL_EMLIB_GPIO_INIT_TIMER1_PIN <<
       _GPIO_TIMER_CC0ROUTE_PIN_SHIFT);

#if (PRS_MODE)
  // Timer Compare/Capture channel 1 initialization
  TIMER_InitCC(TIMER1, 1, &timer1_CC1Init);
#endif

  // Timer Compare/Capture channel 0 initialization
  TIMER_InitCC(TIMER1, 0, &timer1_CC0Init);

  // Set the TOP register value.

  TIMER_TopSet(TIMER1, TIMER1_TOP - 1);

  // Enable TIMER1 interrupts
  TIMER_IntEnable(TIMER1, TIMER_IEN_CC0 | TIMER_IEN_CC1 | TIMER_IEN_OF);
  NVIC_ClearPendingIRQ(TIMER1_IRQn);
  NVIC_EnableIRQ(TIMER1_IRQn);

  // Now start the TIMER
  TIMER_Enable(TIMER1, true);
}

void TIMER0_IRQHandler(void)
{
  // Acknowledge the interrupt
  uint32_t flags = TIMER_IntGet(TIMER0);

  // Clear interrupt flags
  TIMER_IntClear(TIMER0, flags);
}

void TIMER1_IRQHandler(void)
{
  // Acknowledge the interrupt
  uint32_t flags = TIMER_IntGet(TIMER1);

  // Clear interrupt flags
  TIMER_IntClear(TIMER1, flags);
}

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * Initialize example.
 ******************************************************************************/
void app_init(void)
{
  // Enable clock sources for all of the used peripherals
  initCMU();
#if (PRS_MODE)
  // Initialize the PRS peripheral
  initPRS();
#endif
  // Initialize the TIMERs
  initTIMER0();
  initTIMER1();
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
  // Nothing
}
