/**************************************************************************//**
 * @file acmp.c
 * @brief Functions to control the ACMP during sensorless commutation
 * @author Silicon Labs
 * @version x.xx (leave as is with x.xx, Correct version is automatically inserted by auto-generation)
 ******************************************************************************
 * @section License
 * <b>(C) Copyright 2014 Silicon Labs, http://www.silabs.com</b>
 *******************************************************************************
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Labs has no
 * obligation to support this Software. Silicon Labs is providing the
 * Software "AS IS", with no express or implied warranties of any kind,
 * including, but not limited to, any implied warranties of merchantability
 * or fitness for any particular purpose or warranties against infringement
 * of any proprietary rights of a third party.
 *
 * Silicon Labs will not be liable for any consequential, incidental, or
 * special damages, or any other relief, or for any claim by any third party,
 * arising from your use of this Software.
 *
 ******************************************************************************/
#include "em_device.h"
#include "em_cmu.h"
#include "em_timer.h"
#include "em_acmp.h"
#include "config.h"
#include "motor.h"
#include "acmp.h"

/* The current commutation state. This variable is used to
 * determine if we are waiting for a rising or falling
 * flank for the zero crossing. */
extern int pwmCurState;

/* This flag is true between a zero crossing and the next commutation */
bool commutationPending;

/* The current electrical period (number of TIMER1 counts per
 * 6 electrical commutations. */
extern int currentPeriod;

/* Local counter. The counter is increased every time the
 * zero crossing routine is called. It is used to calculate
 * the correct time for commutations. */
int acmpCnt = 0;

/**********************************************************
 * Initializes the timer trigger that will start an
 * ACMP measurement. The ACMP uses TIMER2 CC1.
 *********************************************************/
void acmpInitTrigger(void)
{
  TIMER_InitCC_TypeDef initCc = TIMER_INITCC_DEFAULT;
  initCc.prsInput = false;
  initCc.mode = timerCCModeCompare;
  TIMER_InitCC(TIMER2, 1, &initCc);

  TIMER2->IEN = TIMER_IEN_CC1;
  NVIC_EnableIRQ(TIMER2_IRQn);
  TIMER_CompareSet(TIMER2, 1, (PWM_MAX * 85) / 100);
}

/**********************************************************
 * Detects zero crossings of the back-emf from the motor.
 * When a zero crossing is detected a commutation event
 * is scheduled. The commutation should be 30 electrical
 * degrees after the zero crossing. This function
 * is called once per PWM cycle.
 *********************************************************/
void acmpDetectZeroCrossing(void)
{
  if (commutationPending) {
    /* Calculate the commutation point based on the current speed.
     * The value calculated is number of PWM cycles per 30
     * electrical degrees at the current speed. */
    int commutationPoint =
      (currentPeriod * (PRESCALER_TIMER1 / PRESCALER_TIMER0)) / (12 * PWM_TOP);

    /* If the number of times this function is called after the last
     * previous zero crossing is equal to the commutation point
     * we should perform the commutation */
    if (++acmpCnt >= commutationPoint) {
      commutate();
      commutationPending = false;
      acmpCnt = 0;
    }
  } else {
    /* Skip the very first PWM cycle after a commutation
     * has taken place. This avoids erronuous commutations
     * from voltage spikes that can occur just around the
     * commutation. */
    if (acmpCnt >= 1) {
      /* Measure zero crossing. The back-emf alternates
       * between rising and falling flanks, thus we
       * switch between checking for a high and low
       * output of the ACMP. */
      if (pwmCurState % 2 == 0) {
        if (!(ACMP0->STATUS & ACMP_STATUS_ACMPOUT)) {
          commutationPending = true;
          acmpCnt = 0;
        }
      } else {
        if (ACMP0->STATUS & ACMP_STATUS_ACMPOUT) {
          commutationPending = true;
          acmpCnt = 0;
        }
      }
    }
    acmpCnt++;
  }
}

/**********************************************************
 * Initialize the ACMP to detect zero crossings
 * of the motor back-emf.
 *********************************************************/
void acmpInit(void)
{
  /* Initialize state variables */
  commutationPending = false;
  acmpCnt = 0;

  CMU_ClockEnable(cmuClock_ACMP0, true);

  ACMP_Init_TypeDef acmpInit = ACMP_INIT_DEFAULT;
  ACMP_Init(ACMP0, &acmpInit);

  GPIO_PinModeSet(ACMP_VMY_PORT, ACMP_VMY_PIN, gpioModeDisabled, 0);
  GPIO_PinModeSet(ACMP_VMA_PORT, ACMP_VMA_PIN, gpioModeDisabled, 0);
  GPIO_PinModeSet(ACMP_VMB_PORT, ACMP_VMB_PIN, gpioModeDisabled, 0);
  GPIO_PinModeSet(ACMP_VMC_PORT, ACMP_VMC_PIN, gpioModeDisabled, 0);

  GPIO->ABUSALLOC |= ACMP_ABUS_ALLOC;
  GPIO->BBUSALLOC |= ACMP_BBUS_ALLOC;
  GPIO->CDBUSALLOC |= ACMP_CDBUS_ALLOC;
}

/**********************************************************
 * Disable ACMP.
 *********************************************************/
void acmpStop(void)
{
  CMU_ClockEnable(cmuClock_ACMP0, false);
}

/**********************************************************
 * Selects inputs to the ACMP based on the current
 * commutation state.
 *********************************************************/
void acmpSetInput(int state)
{
  switch (state) {
    case 0:
    case 3:
      ACMP_ChannelSet(ACMP0, acmpVMN, acmpVMC);
      break;
    case 1:
    case 4:
      ACMP_ChannelSet(ACMP0, acmpVMN, acmpVMB);
      break;
    case 2:
    case 5:
      ACMP_ChannelSet(ACMP0, acmpVMN, acmpVMA);
      break;
  }
}
