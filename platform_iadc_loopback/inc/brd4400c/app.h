/***************************************************************************//**
 * @file
 * @brief IADC Loopback examples function
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
// VDAC-related macro definitions
#define VDAC_CLK_FREQ             1000000
#define VDAC_CHANNEL_NUM          0
#define VDAC0_PORT                vdacChPortB
#define VDAC0_PIN                 4
#define VDAC_0_BUSALLOC           BBUSALLOC
#define VDAC0_BUS_REGISTER        GPIO_BBUSALLOC_BEVEN0_VDAC0CH0

// IADC-related macro defintions
#define IADC_CLK_SRC_FREQ         40000000   // CLK_SRC_ADC - 40 MHz max
#define IADC_CLK_FREQ             10000000   // CLK_ADC - 10 MHz max in normal mode
#define IADC0_POS_INPUT           iadcPosInputPortBPin5
#define IADC0_NEG_INPUT           iadcNegInputGnd
#define IADC0_BUSALLOC            BBUSALLOC
#define IADC0_BUS_REGISTER        GPIO_BBUSALLOC_BODD1_ADC0

// Timer frequency in Hertz
#define TIMER_FREQ                5

// Initialize application
void app_init(void);

// App ticking function
void app_process_action(void);

#endif // APP_H
