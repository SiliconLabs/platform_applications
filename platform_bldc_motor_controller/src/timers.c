/**************************************************************************//**
 * @file timers.c
 * @brief Enables TIMERS and PWM
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
#include "em_timer.h"
#include "em_cmu.h"
#include "em_prs.h"
#include "config.h"
#include "pwm.h"
#include "acmp.h"
#include "logging.h"
#include "motor.h"
#include "pid.h"
#include "timers.h"

/* Keeps track of overflows on TIMER0. Used to
 * invoke the PID regulator regularly */
volatile int timer0OverflowCounter = 0;

/* Keeps track of overflows on TIMER1. Used to
 * calculate the current speed and detect stall
 * conditions. */
volatile int timer1OverflowCounter = 0;

/* Tells if the motor is running */
extern bool isRunning;

/* Tells if the PID regulater is enabled */
extern bool pidActive;

/**********************************************************
 * Initialize TIMER0. This timer is used for PWM output.
 *********************************************************/
void timer0Init(void)
{
  timer0OverflowCounter = 0;

  CMU_ClockEnable(cmuClock_TIMER0, true);

  /* Initialize PWM on TIMER0 */
  pwmInit();

  /* Enable overflow interrupt */
  TIMER0->IEN = TIMER_IEN_OF;
  NVIC_EnableIRQ(TIMER0_IRQn);

  TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;

#if PRESCALER_TIMER0 == 1
  timerInit.prescale = timerPrescale1;
#elif PRESCALER_TIMER0 == 2
  timerInit.prescale = timerPrescale2;
#elif PRESCALER_TIMER0 == 4
  timerInit.prescale = timerPrescale4;
#elif PRESCALER_TIMER0 == 8
  timerInit.prescale = timerPrescale8;
#elif PRESCALER_TIMER0 == 16
  timerInit.prescale = timerPrescale16;
#elif PRESCALER_TIMER0 == 32
  timerInit.prescale = timerPrescale32;
#elif PRESCALER_TIMER0 == 64
  timerInit.prescale = timerPrescale64;
#elif PRESCALER_TIMER0 == 128
  timerInit.prescale = timerPrescale128;
#endif

  TIMER_Init(TIMER0, &timerInit);
}

/**********************************************************
 * Stops TIMER0.
 *********************************************************/
void timer0Stop(void)
{
  NVIC_DisableIRQ(TIMER0_IRQn);
  NVIC_ClearPendingIRQ(TIMER0_IRQn);
  TIMER0->CMD = TIMER_CMD_STOP;
  TIMER_Reset(TIMER0);
  pwmOff();
  CMU_ClockEnable(cmuClock_TIMER0, false);
}

/**********************************************************
 * Called at the end of each PWM period. Used to
 * invoke the PID regulator at fixed intervals.
 * Also sends real-time logging data if enabled.
 *********************************************************/
void TIMER0_IRQHandler(void)
{
  TIMER0->IF_CLR = TIMER_IF_OF;

  timer0OverflowCounter = (timer0OverflowCounter + 1) % PID_PRESCALER;

  if (timer0OverflowCounter == 0) {
    if (pidActive) {
      pidRegulate();
    }
    LOG_SEND();
  }
}

/**********************************************************
 * Initialize TIMER1. This timer is used to keep track
 * of the current speed.
 *********************************************************/
void timer1Init(void)
{
  timer1OverflowCounter = 0;

  CMU_ClockEnable(cmuClock_TIMER1, true);

  /* Configure TIMER1 compare channel to trigger commutations */
  TIMER_InitCC_TypeDef initCc0 = TIMER_INITCC_DEFAULT;
  initCc0.mode = timerCCModeCompare;
  TIMER_InitCC(TIMER1, 0, &initCc0);

  TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;

#if PRESCALER_TIMER1 == 1
  timerInit.prescale = timerPrescale1;
#elif PRESCALER_TIMER1 == 2
  timerInit.prescale = timerPrescale2;
#elif PRESCALER_TIMER1 == 4
  timerInit.prescale = timerPrescale4;
#elif PRESCALER_TIMER1 == 8
  timerInit.prescale = timerPrescale8;
#elif PRESCALER_TIMER1 == 16
  timerInit.prescale = timerPrescale16;
#elif PRESCALER_TIMER1 == 32
  timerInit.prescale = timerPrescale32;
#elif PRESCALER_TIMER1 == 64
  timerInit.prescale = timerPrescale64;
#elif PRESCALER_TIMER1 == 128
  timerInit.prescale = timerPrescale128;
#endif

  /* Enable TIMER interrupt */
  TIMER1->IF_CLR = TIMER_IF_OF;
  NVIC_ClearPendingIRQ(TIMER1_IRQn);
  TIMER1->IEN = TIMER_IEN_OF;
  NVIC_EnableIRQ(TIMER1_IRQn);

  TIMER_Init(TIMER1, &timerInit);
}

/**********************************************************
 * Stops TIMER1.
 *********************************************************/
void timer1Stop(void)
{
  NVIC_DisableIRQ(TIMER1_IRQn);
  NVIC_ClearPendingIRQ(TIMER1_IRQn);
  TIMER1->CMD = TIMER_CMD_STOP;
  TIMER_Reset(TIMER1);
  CMU_ClockEnable(cmuClock_TIMER1, false);
}

/**********************************************************
 * Keeps track of TIMER1 overflows.
 * The timer is reset for each 6th commutation event
 * and the speed is calculated from the counter value
 * at that point. The number of overflows is tracked
 * in order to increase the dynamic range of the timer,
 * but if the timer overflows too many times it
 * is an indication of a motor stall. In case of a
 * stall detect we stop driving the motor.
 *********************************************************/
void TIMER1_IRQHandler(void)
{
  uint32_t flags = TIMER1->IF;
  TIMER1->IF_CLR = flags;
  if (isRunning && (flags & TIMER_IF_OF)) {
    timer1OverflowCounter++;
    if (timer1OverflowCounter > STALL_TIMEOUT_OF) {
      /* Stall condition. Stop driving the motor. */
      stopMotor();
    }
  }
}

/**********************************************************
 * Initialize TIMER2 to trigger at a certain point in
 * the PWM waveform.
 *********************************************************/
void timer2Init(void)
{
  CMU_ClockEnable(cmuClock_TIMER2, true);
  CMU_ClockEnable(cmuClock_PRS, true);

#if COMMUTATION_METHOD == COMMUTATION_SENSORLESS

  /* Initialize ACMP trigger. The ACMP uses compare channel 1 */
  acmpInitTrigger();
#endif

  /* Enable PRS connection from TIMER0 overflow. */
  PRS_SourceAsyncSignalSet(0,
                           PRS_ASYNC_CH_CTRL_SOURCESEL_TIMER0,
                           PRS_ASYNC_CH_CTRL_SIGSEL_TIMER0OF);

  /* Configure TIMER2 to listen on PRS channel 0 */
  TIMER_InitCC_TypeDef initCc = TIMER_INITCC_DEFAULT;
  initCc.prsInputType = timerPrsInputAsyncPulse;
  initCc.prsInput = true;
  initCc.prsSel = timerPRSSELCh0;
  initCc.mode = timerCCModeCapture;
  TIMER_InitCC(TIMER2, 0, &initCc);

  /* Configure TIMER2 to start counting on rising input edge (from PRS)
   * and in one-shot mode. */
  TIMER_Init_TypeDef initTimer2 = TIMER_INIT_DEFAULT;
  initTimer2.oneShot = true;
  initTimer2.riseAction = timerInputActionReloadStart;
  initTimer2.enable = false;
  TIMER_Init(TIMER2, &initTimer2);
}

/**********************************************************
 * Stop TIMER2.
 *********************************************************/
void timer2Stop(void)
{
  NVIC_DisableIRQ(TIMER2_IRQn);
  NVIC_ClearPendingIRQ(TIMER2_IRQn);
  TIMER2->CMD = TIMER_CMD_STOP;
  TIMER_Reset(TIMER2);
  CMU_ClockEnable(cmuClock_TIMER2, false);
}

/**********************************************************
 * TIMER2 IRQ Handler. Triggers a new ACMP zero
 * crossing measurement at the same point
 * in the PWM waveform. The ADC current measuremnt
 * is also started from TIMER2, but this is triggered
 * via PRS and the ADC interrupt is called when the
 * conversion is complete.
 *********************************************************/
void TIMER2_IRQHandler(void)
{
  uint32_t flags = TIMER_IntGetEnabled(TIMER2);
  TIMER_IntClear(TIMER2, flags);

#if COMMUTATION_METHOD == COMMUTATION_SENSORLESS

  /* Capture channel 1 is used by ACMP. On interrupt,
   * measure zero crossing of back emf. */
  if ((flags & TIMER_IF_CC1) && isRunning) {
    acmpDetectZeroCrossing();
  }
#endif
}

/**********************************************************
 * Initialize all timers.
 *********************************************************/
void timersInit(void)
{
  timer0Init();
  timer1Init();
  timer2Init();
}

/**********************************************************
 * Stop all timers.
 *********************************************************/
void timersStop(void)
{
  timer0Stop();
  timer1Stop();
  timer2Stop();
}
