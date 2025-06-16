/***************************************************************************//**
 * @file
 * @brief LDMA examples function
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

#ifndef APP_H
#define APP_H

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

// Enable Synchronous mode
#define SYNC_MODE                  1

// Route the TIMER0_CC0 signal
#define TIMER_DEBUG                1

// Timer frequency in Hz
#define TIMER_FREQ                 1

// How many samples to capture
#define NUM_SAMPLES                10

// Configured PRS channel
#define PRS_CHANNEL                0

// Set CLK_ADC to 10MHz
#define CLK_SRC_ADC_FREQ           20000000 // CLK_SRC_ADC
#define CLK_ADC_FREQ               10000000 // CLK_ADC - 10 MHz max

#define IADC_INPUT_0_PORT_PIN      iadcPosInputPortAPin6 // EXP_14
#define IADC_VREF                  3300

/*
 * Specify the IADC input using the IADC_PosInput_t typedef.  This
 * must be paired with a corresponding macro definition that allocates
 * the corresponding ABUS to the IADC.  These are...
 *
 * GPIO->ABUSALLOC |= GPIO_ABUSALLOC_AEVEN0_ADC0
 * GPIO->ABUSALLOC |= GPIO_ABUSALLOC_AODD0_ADC0
 * GPIO->BBUSALLOC |= GPIO_BBUSALLOC_BEVEN0_ADC0
 * GPIO->BBUSALLOC |= GPIO_BBUSALLOC_BODD0_ADC0
 * GPIO->CDBUSALLOC |= GPIO_CDBUSALLOC_CDEVEN0_ADC0
 * GPIO->CDBUSALLOC |= GPIO_CDBUSALLOC_CDODD0_ADC0
 *
 * ...for port A, port B, and port C/D pins, even and odd, respectively.
 */
// Allocate port A and B (Pins of port A and port B can be used for measurement)
#define IADC_INPUT_0_BUS           ABUSALLOC
#define IADC_INPUT_0_BUSALLOC      GPIO_ABUSALLOC_AEVEN0_ADC0

#define LDMA_GPIO_LED1_PORT        gpioPortD
#define LDMA_GPIO_LED1_PIN         3

/***************************************************************************//**
 * Initialize application
 ******************************************************************************/
void app_init(void);

/***************************************************************************//**
 * App ticking function
 ******************************************************************************/
void app_process_action(void);

#endif // APP_H
