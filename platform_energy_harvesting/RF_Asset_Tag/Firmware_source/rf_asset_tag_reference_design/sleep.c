/***************************************************************************//**
 * @file sleep.c
 * @brief Low power sleep including EM4 wakeup
 *******************************************************************************
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

#include "sleep.h"

#include <em_cmu.h>
#include <em_burtc.h>
#include <em_letimer.h>
#include <em_ramfunc.h>

// #define ULFRCO_FOR_EM4
// #define ULFRCO_FOR_EM2

#if defined(ULFRCO_FOR_EM4)
#define EM4_TICKS_PER_SEC                       1000
#else
#define EM4_TICKS_PER_SEC                       32768
#endif

#if defined(ULFRCO_FOR_EM2)
#define EM2_TICKS_PER_SEC                       1000
#else
#define EM2_TICKS_PER_SEC                       32768
#endif

void set_wakeup(unsigned int time_ms)
{
  BURTC_Init_TypeDef burtcInit = {
    .start = false,
    .debugRun = false,
    .clkDiv = 1,
    .compare0Top = false,
    .em4comp = true,
    .em4overflow = false,
  };

  CMU_ClockEnable(cmuClock_BURTC, false);
#if defined(ULFRCO_FOR_EM2)
  CMU_ClockSelectSet(cmuClock_BURTC, cmuSelect_ULFRCO);
#else
  CMU_ClockSelectSet(cmuClock_BURTC, cmuSelect_LFRCO);
#endif
  CMU_ClockEnable(cmuClock_BURTC, true);

  BURTC_Reset();
  BURTC_Init(&burtcInit);
  // Float is used to avoid overflow when time * EM4_TICKS_PER_SEC is larger than 2^32-1
  // uint64_t might be used as well, but might take longer for the division.
  float time = ((float) time_ms * EM4_TICKS_PER_SEC + 999) / 1000;
  BURTC_CompareSet(0, (int) time);
  BURTC_IntClear(_BURTC_IF_MASK);
  BURTC_IntEnable(BURTC_IEN_OF);
  BURTC_Start();
}

void clear_wakeup(void)
{
  CMU_ClockEnable(cmuClock_BURTC, true);
  BURTC_Reset();
  CMU_ClockEnable(cmuClock_BURTC, false);
}

#if defined(_SILICON_LABS_GECKO_INTERNAL_SDID_205)
SL_RAMFUNC_DECLARATOR static void __attribute__ ((noinline)) ramWFE(void);
SL_RAMFUNC_DEFINITION_BEGIN
static void __attribute__ ((noinline)) ramWFE(void)
{
  __WFE();                      // Enter EM2 or EM3
  if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) {
    for (volatile int i = 0; i < 6; i++) {
    }                           // Dummy wait loop ...
  }
}
SL_RAMFUNC_DEFINITION_END
#endif

// running from HFRCO, DPLL is not enabled, so this is fine...
void sleep(unsigned int time_ms)
{
  LETIMER_Init_TypeDef init = LETIMER_INIT_DEFAULT;
  CORE_DECLARE_IRQ_STATE;

  init.enable = false;
  init.comp0Top = true;
  init.topValue = (time_ms * EM2_TICKS_PER_SEC + 999) / 1000;

  CMU_ClockEnable(cmuClock_LETIMER0, true);
  LETIMER_Init(LETIMER0, &init);
  LETIMER_IntClear(LETIMER0, _LETIMER_IF_MASK);
  LETIMER_IntEnable(LETIMER0, LETIMER_IEN_UF);
  GPIO_IntClear(_GPIO_IF_EM4WU_MASK);
  LETIMER_Enable(LETIMER0, true);

  CORE_ENTER_CRITICAL();
  while ((LETIMER0->IF & LETIMER_IF_UF) == 0)
  {
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

#if defined(_SILICON_LABS_GECKO_INTERNAL_SDID_205)
    ramWFE();
#else
    __WFE();
#endif

    CORE_YIELD_CRITICAL();
  }
  CORE_EXIT_CRITICAL();

  LETIMER_Enable(LETIMER0, false);
  LETIMER_IntDisable(LETIMER0, _TIMER_IEN_MASK);
  LETIMER_IntClear(LETIMER0, _TIMER_IEN_MASK);
  NVIC_ClearPendingIRQ(LETIMER0_IRQn);
  CMU_ClockEnable(cmuClock_LETIMER0, false);
}
