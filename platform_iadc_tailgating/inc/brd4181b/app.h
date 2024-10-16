/***************************************************************************//**
 * @file
 * @brief Top level application functions
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

#ifndef APP_H
#define APP_H

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

// Set CLK_ADC to 40 MHz - this will be adjusted to HFXO frequency in the
// initialization process
// (The CLK_ADC clock possible dived values are 1,2,3,4)
#define CLK_SRC_ADC_FREQ          HFXO_FREQ / 4 // CLK_SRC_ADC - 40 MHz max
#define CLK_ADC_FREQ              100000 // ADC_CLK - 10 MHz max in normal mode

// Desired conversation interval in Hz
// min 150 Hz if the CLK_ADC is 9.75MHz
#define CLK_TIMER                 200

// Number of scan channels
#define NUM_INPUTS                2

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
#define IADC_INPUT_0_PORT_PIN     iadcPosInputPortCPin3;
#define IADC_INPUT_1_PORT_PIN     iadcPosInputPortCPin0;
#define IADC_INPUT_2_PORT_PIN     iadcPosInputPortBPin1;

#define IADC_INPUT_0_BUS          CDBUSALLOC
#define IADC_INPUT_0_BUSALLOC     GPIO_CDBUSALLOC_CDODD0_ADC0

#define IADC_INPUT_1_BUS          CDBUSALLOC
#define IADC_INPUT_1_BUSALLOC     GPIO_CDBUSALLOC_CDEVEN0_ADC0

#define IADC_INPUT_2_BUS          BBUSALLOC
#define IADC_INPUT_2_BUSALLOC     GPIO_BBUSALLOC_BODD0_ADC0

// PRS Channel 0-5 can be routed to port A/B and Channel 6-11 to port C/D
#define SCANENTRYDONE_PRS_CHANNEL 6
#define SCANENTRYDONE_PORT        gpioPortD
#define SCANENTRYDONE_PIN         3

// PRS Channel 0-5 can be routed to port A/B and Channel 6-11 to port C/D
#define SINGLEDONE_PRS_CHANNEL    7
#define SINGLEDONE_PORT           gpioPortD
#define SINGLEDONE_PIN            2

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void);

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void);

#endif // APP_H
