/**************************************************************************//**
 * @file pwm.c
 * @brief Configures PWM signals for high-side transitors with optional
 *        complementary PWM for low-side.
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
#include "em_gpio.h"
#include "config.h"
#include "adc.h"
#include "logging.h"
#include "motor.h"
#include "pwm.h"

/* This mask is used to change only the low
 * side transitor pins */
#define LOW_SIDE_PIN_MASK (0xFFFF           \
                           &  ~((1 <<       \
                                 PWM0B_PIN) \
                                | (1 << PWM1B_PIN) | (1 << PWM2B_PIN)))

/* The current commutation state [0-5] */
volatile int pwmCurState = 0;

/* Contains the values to write to TIMER0_ROUTE and
 * port GPIO_PORT_A_DOUT register for each commutation step */
PwmState states[] =
{
  { 0x01, 1 << PWM1B_PIN }, // C - AH, BL
  { 0x01, 1 << PWM2B_PIN }, // B - AH, CL
  { 0x02, 1 << PWM2B_PIN }, // A - BH, CL
  { 0x02, 1 << PWM0B_PIN }, // C - BH, AL
  { 0x04, 1 << PWM0B_PIN }, // B - CH, AL
  { 0x04, 1 << PWM1B_PIN }  // A - CH, BL
};

/**********************************************************
 * Sets a new commutation state.
 *
 * @param state
 *   The new state [0-5]
 **********************************************************/
void pwmSetState(int state)
{
  uint32_t dout;

  /* Write to the TIMER0_ROUTE register to enable/disable the PWM
   * waveforms. This controls the top side transistors.  */
  GPIO->TIMERROUTE[0].ROUTEEN = states[state].pwm | (states[state].pwm << 3);
  TIMER0->DTOGEN = states[state].pwm | (states[state].pwm << 3);

  /* Do a read-modify-write of DOUT register. This controls the
   * low side transistors. Only change pins PD10 - PD12 */
  dout = GPIO->P[PWM_PORT].DOUT & LOW_SIDE_PIN_MASK;
  GPIO->P[PWM_PORT].DOUT = dout | states[state].high;

  /* Save the current state */
  pwmCurState = state;
}

/**********************************************************
 * Go to the next commutation state.
 *********************************************************/
void pwmNextState(void)
{
  if (getDirection()) {
    pwmSetState((pwmCurState + 1) % 6);
  } else {
    pwmSetState((pwmCurState + 6) % 7);
  }
}

/**********************************************************
 * Turn of all the transistors.
 *********************************************************/
void pwmOff(void)
{
  /* Disable all PWM pins */
  GPIO->TIMERROUTE[0].ROUTEEN = 0;

  /* Clear low-side pins */
  GPIO->P[PWM_PORT].DOUT = GPIO->P[PWM_PORT].DOUT & LOW_SIDE_PIN_MASK;
}

/**********************************************************
 * Initialize the PWM waveforms that drive the
 * inverter transistors.
 *********************************************************/
void pwmInit(void)
{
  /* Reset state */
  pwmCurState = 0;

  /* Enable the driving pins  */
  CMU_ClockEnable(cmuClock_GPIO, true);
  GPIO_PinModeSet(PWM_PORT, PWM0A_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(PWM_PORT, PWM1A_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(PWM_PORT, PWM2A_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(PWM_PORT, PWM0B_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(PWM_PORT, PWM1B_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(PWM_PORT, PWM2B_PIN, gpioModePushPull, 0);

#ifdef USE_COMPLEMENTARY_PWM
  pwmEnableComplementaryPwm(true);
#endif

  /* Enable PWM on all three compare/capture channels */
  TIMER_InitCC_TypeDef initCc = TIMER_INITCC_DEFAULT;
  initCc.mode = timerCCModePWM;
  TIMER_InitCC(TIMER0, 0, &initCc);
  TIMER_InitCC(TIMER0, 1, &initCc);
  TIMER_InitCC(TIMER0, 2, &initCc);

  /* Set the PWM period and initial duty cycle */
  TIMER_TopSet(TIMER0, PWM_TOP);
  TIMER0->CC[0].OC = PWM_DEFAULT_DUTY_CYCLE;
  TIMER0->CC[1].OC = PWM_DEFAULT_DUTY_CYCLE;
  TIMER0->CC[2].OC = PWM_DEFAULT_DUTY_CYCLE;

  /* Set location */
  GPIO->TIMERROUTE[0].CC0ROUTE =
    (PWM_PORT <<
      _GPIO_TIMER_CC0ROUTE_PORT_SHIFT)
    | (PWM0A_PIN << _GPIO_TIMER_CC0ROUTE_PIN_SHIFT);
  GPIO->TIMERROUTE[0].CC1ROUTE =
    (PWM_PORT <<
      _GPIO_TIMER_CC1ROUTE_PORT_SHIFT)
    | (PWM1A_PIN << _GPIO_TIMER_CC1ROUTE_PIN_SHIFT);
  GPIO->TIMERROUTE[0].CC2ROUTE =
    (PWM_PORT <<
      _GPIO_TIMER_CC2ROUTE_PORT_SHIFT)
    | (PWM2A_PIN << _GPIO_TIMER_CC2ROUTE_PIN_SHIFT);
  GPIO->TIMERROUTE[0].CDTI0ROUTE =
    (PWM_PORT <<
      _GPIO_TIMER_CDTI0ROUTE_PORT_SHIFT)
    | (PWM0B_PIN << _GPIO_TIMER_CDTI0ROUTE_PIN_SHIFT);
  GPIO->TIMERROUTE[0].CDTI1ROUTE =
    (PWM_PORT <<
      _GPIO_TIMER_CDTI1ROUTE_PORT_SHIFT)
    | (PWM1B_PIN << _GPIO_TIMER_CDTI1ROUTE_PIN_SHIFT);
  GPIO->TIMERROUTE[0].CDTI2ROUTE =
    (PWM_PORT <<
      _GPIO_TIMER_CDTI2ROUTE_PORT_SHIFT)
    | (PWM2B_PIN << _GPIO_TIMER_CDTI2ROUTE_PIN_SHIFT);
}

/**********************************************************
 * Enables/disables complemtentary PWM output.
 *********************************************************/
void pwmEnableComplementaryPwm(bool enable)
{
  if (enable) {
    TIMER_EnableDTI(TIMER0, false);
    TIMER0->EN_CLR = TIMER_EN_EN;

    /* Configure dead time for rising and falling edges of the PWM waveform. */
    TIMER0->DTTIMECFG =
      ((DEAD_TIME_CYCLES_RISING_EDGE - 1) << _TIMER_DTTIMECFG_DTRISET_SHIFT)
      | ((DEAD_TIME_CYCLES_FALLING_EDGE - 1) <<
         _TIMER_DTTIMECFG_DTFALLT_SHIFT)
      | DEAD_TIME_PRESCALER;

    TIMER0->EN_SET = TIMER_EN_EN;

    /* Enable dead time output generation for all CCx and DTIx pins */
    TIMER0->DTOGEN = TIMER_DTOGEN_DTOGCC0EN
                     | TIMER_DTOGEN_DTOGCC1EN
                     | TIMER_DTOGEN_DTOGCC2EN
                     | TIMER_DTOGEN_DTOGCDTI0EN
                     | TIMER_DTOGEN_DTOGCDTI1EN
                     | TIMER_DTOGEN_DTOGCDTI2EN;

    /* Enable complementary output */
    TIMER_EnableDTI(TIMER0, true);
  } else {
    TIMER0->DTCTRL = 0;
  }
}

/**********************************************************
 * Sets the a new PWM duty cycle.
 *********************************************************/
void pwmSetDutyCycle(int pwm)
{
  TIMER0->CC[0].OCB = pwm;
  TIMER0->CC[1].OCB = pwm;
  TIMER0->CC[2].OCB = pwm;

/* If current measurement is enabled set the
 * measurement point to be in the middle of PWM period */
#ifdef CURRENT_MEASUREMENT_ENABLED
  adcSetMeasurementPoint();
#endif

  /* Log the new pwm value */
  LOG_SET_PWM(pwm);
}
