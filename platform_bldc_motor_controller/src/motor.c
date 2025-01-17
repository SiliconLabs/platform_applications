/**************************************************************************//**
 * @file motor.c
 * @brief Functions to control the motor
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
#include <stdbool.h>
#include "em_device.h"
#include "em_core.h"
#include "config.h"
#include "timers.h"
#include "adc.h"
#include "acmp.h"
#include "pid.h"
#include "sensorless_motor.h"
#include "pwm.h"
#include "logging.h"
#include "motor.h"

/* Motor state */
volatile bool isRunning = false;

/* Spin direction */
static volatile bool cwDir = true;

/* Used to count the number of commutations before starting PID regulator */
static int startupCounter;

/* Used to keep track of the period of commutations */
static int commutationCounter;

/* The current period of one electrical (6 commutations) rotation.
 * Measured in counts of TIMER1 */
int currentPeriod;

/* The current commutation phase */
extern int pwmCurState;

/* Tells whether the PID controller is enabled */
extern bool pidActive;

/* Used to keep track of overflows on TIMER1 to calculate the correct speed */
extern int timer1OverflowCounter;

/**********************************************************
 * Start the motor. This method is called both for
 * sensorless and hall sensor motors. The configuration
 * macro COMMUTATION_METHOD determines the type of commutation.
 *********************************************************/
void startMotor(void)
{
  if (isRunning) {
    return;
  }
  commutationCounter = 0;
  startupCounter = 0;

  timersInit();
  pidInit();

#ifdef CURRENT_MEASUREMENT_ENABLED
  adcInit();
#endif

#if COMMUTATION_METHOD == COMMUTATION_SENSORLESS
  sensorlessStartMotor();
#else
  hallStartMotor();
#endif

  isRunning = true;
}

/**********************************************************
 * Stops the motor. This function disables the timers,
 * ACMP and ADC.
 *********************************************************/
void stopMotor(void)
{
#if COMMUTATION_METHOD == COMMUTATION_HALL
  hallStop();
#endif
  pidStop();
  timersStop();
#ifdef CURRENT_MEASUREMENT_ENABLED
  adcStop();
#endif
#if COMMUTATION_METHOD == COMMUTATION_SENSORLESS
  acmpStop();
#endif
  setSpeed(DEFAULT_SETPOINT_RPM);
  isRunning = false;
  pidActive = false;
}

/**********************************************************
 * Saves the current speed.
 *********************************************************/
void saveSpeed(void)
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();

  /* Save current speed and reset timer.
   * The speed is saved as the current electrical
   * period (time it takes to perform 6 commutations)
   * in terms of TIMER1 counts. Use the COUNT_TO_RPM()
   * macro to get the RPM value. */
  currentPeriod = TIMER1->CNT;
  TIMER1->CNT = 0;
  if (timer1OverflowCounter > 0) {
    currentPeriod += timer1OverflowCounter * TIMER_MAX;
    timer1OverflowCounter = 0;
  }

  CORE_EXIT_CRITICAL();
  LOG_SET_SPEED(COUNT_TO_RPM(currentPeriod));
}

/**********************************************************
 * Perform commutation of the motor.
 *********************************************************/
void commutate(void)
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();

  /* Go to the next driving state */
  pwmNextState();

#if COMMUTATION_METHOD == COMMUTATION_SENSORLESS

  /* In sensorless mode, change the ACMP input based on the
   * current commutation state. This is so the ACMP always
   * measures the undriven terminal of the motor. */
  if (getDirection()) {
    acmpSetInput(pwmCurState);
  } else {
    int nextState = (pwmCurState - 1);
    if (nextState < 0) {
      nextState = (nextState + 6);
    }
    acmpSetInput(nextState);
  }
#endif

  /* Wait for a few commutations at startup before
   * enabling the PID regulator. This helps the
   * measured speed to settle and avoids bumps when
   * switching to closed loop regulation */
  if (startupCounter <= PID_STARTUP_COMMUTATIONS) {
    if (startupCounter == PID_STARTUP_COMMUTATIONS) {
      pidActive = true;
    }
    startupCounter++;
  }

  /* Save the speed for every 6th commutation step */
  commutationCounter = (commutationCounter + 1) % 6;

  if (commutationCounter == 0) {
    saveSpeed();
  }

  CORE_EXIT_CRITICAL();
}

/**********************************************************
 * Sets the motor direction. The direction can only
 * be changed when the motor is at rest.
 *
 * @param spinCw
 *   If true, the motor will spin clockwise. If false
 *   it will spin counter-clockwise.
 *
 *********************************************************/
void setDirection(bool spinCw)
{
  if (isRunning) {
    return;
  }

  cwDir = spinCw;

  LOG_SEND_SCALAR(PARAM_DIR, (int16_t)cwDir);
}

/**********************************************************
 * Returns the motor direction. If the return value is
 * true the motor will spin clockwise. If false it
 * will spin counter-clockwise.
 *********************************************************/
bool getDirection(void)
{
  return cwDir;
}
