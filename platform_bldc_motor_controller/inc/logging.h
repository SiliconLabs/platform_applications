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
#ifndef _LOGGING_H_
#define _LOGGING_H_

void logInit(void);
void logSend(void);
void logSendScalar(uint8_t param, int16_t value);
void logSendFloat(uint8_t param, float value);
void logSetCurrentSpeed(int16_t);
void logSetCurrentPwm(int16_t);
void logSetMotorCurrent(int16_t);

#ifdef BLDC_LOGGING_ENABLED

#define LOG_INIT()
#define LOG_SEND()               logSend()
#define LOG_SEND_SCALAR(x, y)    logSendScalar(x, y)
#define LOG_SEND_FLOAT(x, y)     logSendFloat(x, y)
#define LOG_SET_SPEED(x)         logSetCurrentSpeed(x)
#define LOG_SET_PWM(x)           logSetCurrentPwm(x)
#define LOG_SET_MOTOR_CURRENT(x) logSetMotorCurrent(x)

#else

#define LOG_INIT()
#define LOG_SEND()
#define LOG_SEND_SCALAR(x, y)
#define LOG_SEND_FLOAT(x, y)
#define LOG_SET_SPEED(x)
#define LOG_SET_PWM(x)
#define LOG_SET_MOTOR_CURRENT(x)

#endif

#define PARAM_SPEED             1
#define PARAM_SETPOINT          2
#define PARAM_COMMUTATION_DELAY 3
#define PARAM_CURRENT           4
#define PARAM_DEBUG             5
#define PARAM_KP                6
#define PARAM_KI                7
#define PARAM_KD                8
#define PARAM_DIR               9

#define HEADER_REALTIME         0x32
#define HEADER_SCALAR           0xA6
#define HEADER_FLOAT            0xA7
#define HEADER_VERSION          0xA8

#define CMD_START               0x41
#define CMD_STOP                0x42
#define CMD_SET_SETPOINT        0x43
#define CMD_GET_VER             0x44
#define CMD_SET_PID             0x45
#define CMD_CHANGE_DIR          0x46

#endif
