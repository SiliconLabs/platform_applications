#ifndef _CONFIG_MG24_H_
#define _CONFIG_MG24_H_

#include "em_acmp.h"
#include "em_gpio.h"

// ACMP config
#define ACMP_VMY_PORT        gpioPortC
#define ACMP_VMY_PIN         0
#define ACMP_VMA_PORT        gpioPortB
#define ACMP_VMA_PIN         5
#define ACMP_VMB_PORT        gpioPortC
#define ACMP_VMB_PIN         5
#define ACMP_VMC_PORT        gpioPortC
#define ACMP_VMC_PIN         7
#define ACMP_ABUS_ALLOC      0
#define ACMP_BBUS_ALLOC      GPIO_BBUSALLOC_BODD0_ACMP0
#define ACMP_CDBUS_ALLOC     GPIO_CDBUSALLOC_CDEVEN0_ACMP0 \
  | GPIO_CDBUSALLOC_CDODD0_ACMP0
#define acmpVMN              acmpInputPC0
#define acmpVMA              acmpInputPB5
#define acmpVMB              acmpInputPC5
#define acmpVMC              acmpInputPC7

// ADC config
#define ADC_IM_0P_PORT       gpioPortD
#define ADC_IM_0P_PIN        4
#define ADC_IM_0N_PORT       gpioPortD
#define ADC_IM_0N_PIN        5
#define ADC_ABUS_ALLOC       0
#define ADC_BBUS_ALLOC       0
#define ADC_CDBUS_ALLOC      GPIO_CDBUSALLOC_CDEVEN1_ADC0 \
  | GPIO_CDBUSALLOC_CDODD1_ADC0
#define adcPosInput          iadcPosInputPortDPin4
#define adcNegInput          iadcNegInputPortDPin5

// PWM config
#define PWM_PORT             gpioPortA
#define PWM0A_PIN            0 // PWM A high
#define PWM1A_PIN            5 // PWM B high
#define PWM2A_PIN            6 // PWM C high
#define PWM0B_PIN            7 // PWM A low
#define PWM1B_PIN            8 // PWM B low
#define PWM2B_PIN            9 // PWM C low

// Push button config
#define BUTTON0_PORT         gpioPortB
#define BUTTON0_PIN          1
#define BUTTON1_PORT         gpioPortB
#define BUTTON1_PIN          3

// UART config
#define UART_TX_PORT         gpioPortC
#define UART_TX_PIN          1
#define UART_RX_PORT         gpioPortC
#define UART_RX_PIN          2

// Debug GPIO config
#define DBG_GPIO_PORT0       gpioPortD
#define DBG_GPIO_PIN0        2
#define DBG_GPIO_PORT1       gpioPortD
#define DBG_GPIO_PIN1        2

#define VCOM_DISABLE
#define VCOM_DISABLE_PORT    gpioPortB
#define VCOM_DISABLE_PIN     0

#define SENSOR_DISABLE
#define SENSOR_DISABLE_PORT  gpioPortC
#define SENSOR_DISABLE_PIN   9

#define DISPLAY_DISABLE
#define DISPLAY_DISABLE_PORT gpioPortD
#define DISPLAY_DISABLE_PIN  3

#endif
