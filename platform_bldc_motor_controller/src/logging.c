/**************************************************************************//**
 * @file logging.c
 * @brief Logging routines that sends data to PC
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
#include "config.h"
#include "uart.h"
#include "logging.h"

static int16_t logSpeed;
static int16_t logPwm;
static int16_t logMotorCurrent;

/**********************************************************
 * Sets the current speed. The value will be sent over
 * UART when logSend() is called.
 **********************************************************/
void logSetCurrentSpeed(int16_t x)
{
  logSpeed = x;
}

/**********************************************************
 * Sets the current PWM duty cycle. The value will be sent over
 * UART when logSend() is called.
 **********************************************************/
void logSetCurrentPwm(int16_t x)
{
  logPwm = x;
}

/**********************************************************
 * Sets the current motor current. The value will be sent over
 * UART when logSend() is called.
 **********************************************************/
void logSetMotorCurrent(int16_t x)
{
  logMotorCurrent = x;
}

/**********************************************************
 * Send real-time logging data over UART. This function
 * should be called with a fixed frequency and ouputs
 * the current:
 *     * Speed in RPM
 *     * PWM duty cycle
 *     * Motor current
 *
 **********************************************************/
void logSend(void)
{
  uartSendByte(HEADER_REALTIME);
  uartSendNumber(logSpeed);
  uartSendNumber(logPwm);
  uartSendNumber(logMotorCurrent);
}

/**********************************************************
 * Send a scalar value over UART. This is used to log
 * parameters that does not change very frequent
 * (such as the setpoint)
 *
 * @param param
 *    The parameter ID
 *
 * @param value
 *    The current value of the parameter
 *
 **********************************************************/
void logSendScalar(uint8_t param, int16_t value)
{
  uartSendByte(HEADER_SCALAR);
  uartSendByte(param);
  uartSendNumber(value);
}

/**********************************************************
 * Send a float value over UART. This is used to log
 * parameters that does not change very frequent
 * (such as the setpoint)
 *
 * @param param
 *    The parameter ID
 *
 * @param value
 *    The current value of the parameter
 *
 **********************************************************/
void logSendFloat(uint8_t param, float value)
{
  uartSendByte(HEADER_FLOAT);
  uartSendByte(param);

  uint32_t *p = (uint32_t *)&value;

  int i;
  for (i = 0; i < 4; i++) {
    uartSendByte((*p >> (8 * i)) & 0xFF);
  }
}
