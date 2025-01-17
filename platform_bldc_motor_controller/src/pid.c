/**************************************************************************//**
 * @file pid.c
 * @brief PID regulator
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
#include "config.h"
#include "pwm.h"
#include "logging.h"
#include "motor.h"
#include "pid.h"

/* PID controller coefficients */
/* These work well for the Silabs BLCD Kit Motor */
static float Kp = -0.006;
static float Ki = -0.000012;
static float Kd = -0.020;

/* Control variables */
volatile int setpoint = DEFAULT_SETPOINT_RPM;

/* The current PWM duty cycle (in timer counts) */
volatile int currentPwm = PWM_DEFAULT_DUTY_CYCLE;

/* State variables */
static float prevError;
static float integralError;

/* The current period. Used to calculate the speed */
extern int currentPeriod;

/* Status flag. Indicates if PID controller is running */
volatile bool pidActive = false;

/**********************************************************
 * Initializes the PID control algorithm. One timer is
 * required to keep track of the current speed.
 *********************************************************/
void pidInit(void)
{
  /* Reset control variables */
  currentPwm = PWM_DEFAULT_DUTY_CYCLE;
  prevError = 0.0;
  integralError = 0.0;

  LOG_SET_PWM(currentPwm);
}

/**********************************************************
 * Stops PID regulator
 *********************************************************/
void pidStop(void)
{
  pidActive = false;
}

/**********************************************************
 * PID regulator. This function is called periodically
 * with the frequency defined in config.h. It calculates
 * the current error relative to the setpoint and
 * updates the PWM period.
 *********************************************************/
void pidRegulate(void)
{
  float error, derivativeError;
  int correction;

  /* Get current speed */
  int speed = COUNT_TO_RPM(currentPeriod);

  /* PID algorithm */
  error = (speed - setpoint);
  integralError += error;
  derivativeError = error - prevError;
  correction = (int)(Kp * error + Ki * integralError + Kd * derivativeError);
  prevError = error;

  /* Obey min/max limits on PWM duty cycle */
  if (currentPwm + correction < PWM_MIN) {
    currentPwm = PWM_MIN;
  } else if (currentPwm + correction > PWM_MAX) {
    currentPwm = PWM_MAX;
  } else {
    currentPwm += correction;
  }

  /* Set the new PWM duty cycle */
  pwmSetDutyCycle(currentPwm);
}

/**********************************************************
 * Set a new target speed (setpoint)
 *
 * @param rpm
 *    The new speed in RPM
 *********************************************************/
void setSpeed(int rpm)
{
  if (rpm > SETPOINT_MAX_RPM) {
    setpoint = SETPOINT_MAX_RPM;
  } else if (rpm < SETPOINT_MIN_RPM) {
    setpoint = SETPOINT_MIN_RPM;
  } else {
    setpoint = rpm;
  }

  LOG_SEND_SCALAR(PARAM_SETPOINT, setpoint);
}

/**********************************************************
 * Increase the speed of the motor. The speed (RPM) is
 * incremented by SPEED_INCREMENT_RPM defined in config.h.
 *********************************************************/
void speedIncrease(void)
{
  setSpeed(setpoint + SPEED_INCREMENT_RPM);
}

/**********************************************************
 * Decrease the speed of the motor. The speed (RPM) is
 * decreased by SPEED_INCREMENT_RPM defined in config.h.
 *********************************************************/
void speedDecrease(void)
{
  setSpeed(setpoint - SPEED_INCREMENT_RPM);
}

/**********************************************************
 * Sets the PID coefficents Kp, Ki and Kd
 **********************************************************/
void pidSetCoefficients(float kp, float ki, float kd)
{
  Kp = kp;
  Ki = ki;
  Kd = kd;
  pidSendLog();
}

/**********************************************************
 * Outputs the current PID parameters, coefficients and
 * setpoint over UART.
 **********************************************************/
void pidSendLog(void)
{
  LOG_SEND_SCALAR(PARAM_SETPOINT, setpoint);
  LOG_SEND_FLOAT(PARAM_KP, Kp);
  LOG_SEND_FLOAT(PARAM_KI, Ki);
  LOG_SEND_FLOAT(PARAM_KD, Kd);
  LOG_SEND_SCALAR(PARAM_DIR, (int16_t)getDirection());
}
