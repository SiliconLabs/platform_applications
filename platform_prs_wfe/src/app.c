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

#include "em_cmu.h"
#include "em_timer.h"
#include "em_prs.h"

#include "sl_led.h"
#include "sl_simple_led_instances.h"
#include "sl_simple_button_instances.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

#ifndef BUTTON_INSTANCE_0
#define BUTTON_INSTANCE_0   sl_button_btn0
#endif

#ifndef LED_INSTANCE_0
#define LED_INSTANCE_0      sl_led_led0
#endif

// Necessary define for SiSDK v2024.12.0 and earlier
#define prsConsumerCORE_M33RXEV (PRS_Consumer_t) offsetof(PRS_TypeDef, \
                                                          CONSUMER_CORE_M33RXEV)

#define PRS_CHANNEL             0

// Change this to modify the time between CPU wakes
#define NUM_SEC_DELAY           1

/*******************************************************************************
 *****************************   VARIABLES   ***********************************
 ******************************************************************************/

// topValue for TIMER
static uint32_t topValue;

static volatile bool waitForInterrupt = false;

/**************************************************************************//**
 * @brief Interrupt handler for TIMER0
 *****************************************************************************/
void TIMER0_IRQHandler(void)
{
  // UNUSED
}

void sl_button_on_change(const sl_button_t *handle)
{
  if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_PRESSED) {
    if (&BUTTON_INSTANCE_0 == handle) {
      waitForInterrupt = !waitForInterrupt;
    }
  }
}

/**************************************************************************//**
 * @brief CMU initialization
 *****************************************************************************/
void initCMU(void)
{
  CMU_ClockEnable(cmuClock_TIMER0, true);
  CMU_ClockEnable(cmuClock_PRS, true);
}

/**************************************************************************//**
 * @brief PRS initialization
 *****************************************************************************/
void initPRS(void)
{
  // Route TIMER0 Overflow to PRS Channel 0
  PRS_SourceAsyncSignalSet(PRS_CHANNEL,
                           PRS_ASYNC_CH_CTRL_SOURCESEL_TIMER0,
                           PRS_ASYNC_TIMER0_OF);

  // Enable the M33 wake event as a consumer of PRS Channel 0
  PRS_ConnectConsumer(PRS_CHANNEL, prsTypeAsync,
                      prsConsumerCORE_M33RXEV);
}

/**************************************************************************//**
 * @brief TIMER initialization
 *****************************************************************************/
void initTIMER(void)
{
  TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;

  // Do not start counter upon initialization
  timerInit.enable = false;

  TIMER_Init(TIMER0, &timerInit);

  // Set the top value
  topValue = CMU_ClockFreqGet(cmuClock_TIMER0) * NUM_SEC_DELAY;
  TIMER_TopSet(TIMER0, topValue);

  // Now start the TIMER
  TIMER_Enable(TIMER0, true);
}

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  initCMU();
  initTIMER();
  initPRS();
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
  if (waitForInterrupt) {
    __WFI();
  } else {
    __WFE();
  }

  sl_led_toggle(&LED_INSTANCE_0);
}
