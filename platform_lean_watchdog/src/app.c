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

#include <stdbool.h>
#include "em_burtc.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_rmu.h"
#include "em_wdog.h"
#include "sl_power_manager.h"
#include "sl_sleeptimer.h"

#define GPIO_LED0_PORT  gpioPortB
#define GPIO_LED0_PIN   2
#define GPIO_LED1_PORT  gpioPortD
#define GPIO_LED1_PIN   3
#define GPIO_PB0_PORT   gpioPortB
#define GPIO_PB0_PIN    1
#define GPIO_PB1_PORT   gpioPortB
#define GPIO_PB1_PIN    3

volatile bool inEM2;
sl_sleeptimer_timer_handle_t watchdog_feed_timer;

void checkResetReason(void)
{
  // Get and clear reset cause
  uint32_t resetCause = RMU_ResetCauseGet();
  RMU_ResetCauseClear();

  // Turn on LED0 if a Watchdog reset caused the latest reset
  if (resetCause & EMU_RSTCAUSE_WDOG0) {
    CMU_ClockEnable(cmuClock_GPIO, true);
    GPIO_PinModeSet(GPIO_LED0_PORT, GPIO_LED0_PIN, gpioModePushPull, 1);
    while (1) {}
  }
  // Turn on LED1 if a System reset caused the latest reset
  else if (resetCause & EMU_RSTCAUSE_SYSREQ) {
    CMU_ClockEnable(cmuClock_GPIO, true);
    GPIO_PinModeSet(GPIO_LED1_PORT, GPIO_LED1_PIN, gpioModePushPull, 1);
    while (1) {}
  }
}

void initGPIO(void)
{
  // Enable GPIO clock
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Configure LED0 and LED1
  GPIO_PinModeSet(GPIO_LED0_PORT, GPIO_LED0_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(GPIO_LED1_PORT, GPIO_LED1_PIN, gpioModePushPull, 0);

  // Configure PB1
  GPIO_PinModeSet(GPIO_PB1_PORT, GPIO_PB1_PIN, gpioModeInputPull, 1);

  // Configure PB0 and enable GPIO falling edge interrupts
  GPIO_PinModeSet(GPIO_PB0_PORT, GPIO_PB0_PIN, gpioModeInputPull, 1);
  GPIO_ExtIntConfig(GPIO_PB0_PORT, GPIO_PB0_PIN, GPIO_PB0_PIN, false, true,
                    true);
  NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
  NVIC_EnableIRQ(GPIO_ODD_IRQn);
}

void initLFXO(void)
{
  // Enable LFXO with failure detection
  CMU_LFXOInit_TypeDef lfxoInit = CMU_LFXOINIT_DEFAULT;
  lfxoInit.failDetEn = true;
  CMU_LFXOInit(&lfxoInit);

  // Enable LFXO FAIL interrupts
  LFXO->IF_CLR = LFXO_IF_FAIL;
  LFXO->IEN_SET = LFXO_IEN_FAIL;
  NVIC_ClearPendingIRQ(LFXO_IRQn);
  NVIC_EnableIRQ(LFXO_IRQn);
}

void initWDOG(void)
{
  // Select LFXO as WDOG's clock source
  CMU_ClockSelectSet(cmuClock_WDOG0CLK, cmuSelect_LFXO);
  CMU_ClockEnable(cmuClock_WDOG0, true);

  // Configure WDOG0 to run in EM0/1
  WDOG_Init_TypeDef wdogInit = WDOG_INIT_DEFAULT;
  wdogInit.em1Run = true;
  wdogInit.perSel = wdogPeriod_64k;
  WDOGn_Init(WDOG0, &wdogInit);
}

void initBURTC(void)
{
  // Select LFXO as BURTC's clock source
  CMU_ClockSelectSet(cmuClock_EM4GRPACLK, cmuSelect_LFXO);
  CMU_ClockEnable(cmuClock_BURTC, true);

  // Configure BURTC to compare match every 2 seconds
  BURTC_Init_TypeDef burtcInit = BURTC_INIT_DEFAULT;
  burtcInit.start = false;
  burtcInit.compare0Top = true;
  BURTC_Reset();
  BURTC_Init(&burtcInit);
  BURTC_CompareSet(0, (32768 * 2));

  // Enable BURTC COMP interrupts
  BURTC_IntClear(BURTC_IEN_COMP);
  BURTC_IntEnable(BURTC_IEN_COMP);
  NVIC_ClearPendingIRQ(BURTC_IRQn);
  NVIC_EnableIRQ(BURTC_IRQn);
}

void GPIO_ODD_IRQHandler(void)
{
  uint32_t flags = GPIO_IntGet();
  GPIO_IntClear(flags);

  // Toggle between EM1 and EM2 on PB0 button press
  if (flags & (1 << GPIO_PB0_PIN)) {
    // Lowest energy mode to enter is EM1, stop BURTC and start WDOG0
    if (inEM2) {
      inEM2 = false;
      sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
      BURTC_Stop();
      WDOGn_Enable(WDOG0, true);
    }
    // Device can enter EM2, stop WDOG0 and start BURTC
    else {
      inEM2 = true;
      sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
      WDOGn_Enable(WDOG0, false);
      BURTC_CounterReset();
      BURTC_Start();
    }
  }
}

void BURTC_IRQHandler(void)
{
  uint32_t flags = BURTC_IntGet();
  BURTC_IntClear(flags);

  if (flags & BURTC_IF_COMP) {
    // Handle BURTC failure here
    NVIC_SystemReset();
  }
}

void LFXO_IRQHandler(void)
{
  uint32_t flags = LFXO->IF;
  LFXO->IF_CLR = flags;

  if (flags & LFXO_IF_FAIL) {
    // Handle LFXO failure here
    NVIC_SystemReset();
  }
}

void BURTC_Feed(void)
{
  BURTC->CNT = 0;
}

void watchdog_feed_timer_callback(sl_sleeptimer_timer_handle_t *handle,
                                  void *data)
{
  (void) handle;
  (void) data;

  // Feed the watchdog if PB1 is not pressed
  if (GPIO_PinInGet(GPIO_PB1_PORT, GPIO_PB1_PIN)) {
    if (inEM2) {
      BURTC_Feed();
    } else {
      WDOGn_Feed(WDOG0);
    }
  }
}

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  // Check the latest reset reason
  checkResetReason();

  // Configure lowest energy mode to EM1
  inEM2 = false;
  sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);

  // Initialize peripherals
  initGPIO();
  initLFXO();
  initWDOG();
  initBURTC();

  // Feed the watchdog every 1 second
  sl_sleeptimer_start_periodic_timer(&watchdog_feed_timer,
                                     32768,
                                     watchdog_feed_timer_callback,
                                     (void *)NULL,
                                     0,
                                     0);
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
}
