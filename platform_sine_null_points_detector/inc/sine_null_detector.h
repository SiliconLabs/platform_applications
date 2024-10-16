/***************************************************************************//**
 * @file
 * @brief Simple button baremetal examples functions
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

#ifndef SIMPLE_BUTTON_BAREMETAL_H
#define SIMPLE_BUTTON_BAREMETAL_H

#ifndef DEBUG_MODE_ENABLE
#define DEBUG_MODE_ENABLE             0    // Debug mode enable
#endif

/***************************************************************************//**
 * GPIO initialization
 ******************************************************************************/
void initGPIO(void);

/***************************************************************************//**
 * ACMP0 initialization for reference point detection
 *
 * In this example, the internal 2.5 V reference is used and this is divided
 * to get 1.65 V reference voltage
 * so that when the input is lower than 1.65 V, the ACMP output is 0,
 * and when it's higher than 1.65 V, the ACMP output is 1.
 *
 * Here the input signal is on the POSITIVE pin of ACMP
 ******************************************************************************/
void initACMP0(void);

/***************************************************************************//**
 * ACMP1 initialization for reference point detection
 *
 * In this example, the internal 2.5 V reference is used and this is divided
 * to get 1.65 V reference voltage
 * so that when the input is lower than 1.65 V, the ACMP output is 0,
 * and when it's higher than 1.65 V, the ACMP output is 1.
 *
 * Here the input signal is on the NEGATIVE pin of ACMP
 ******************************************************************************/
void initACMP1(void);

/***************************************************************************//**
 * PRS initialization
 ******************************************************************************/
void initPrs(void);

/**************************************************************************//**
 * LETIMER initialization for PWM generation
 *****************************************************************************/
void initLetimer0(void);

/***************************************************************************//**
 * Initialize example
 ******************************************************************************/
void sine_null_detector_init(void);

/***************************************************************************//**
 * App ticking function
 ******************************************************************************/
void sine_null_detector_process_action(void);

#endif // SIMPLE_BUTTON_BAREMETAL_H
