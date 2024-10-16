/*
 * sine_null_detector_config.h
 *
 *  Created on: Oct 2, 2024
 *      Author: leorosz
 */

#ifndef SINE_NULL_DETECTOR_CONFIG_H_
#define SINE_NULL_DETECTOR_CONFIG_H_

#define ACMP0_OUTPUT_PORT             gpioPortA       // ACMP0 output port
#define ACMP0_OUTPUT_PIN              5               // ACMP0 output pin
#define ACMP0_INPUT_PORT              gpioPortC       // ACMP0 input port
#define ACMP0_INPUT_PIN               0               // ACMP0 input pin
#define ACMP0_INPUT_PORT_PIN          acmpInputPC0
#define ACMP0_BUSALLOC                GPIO_CDBUSALLOC_CDEVEN0_ACMP0    // Bus allocation for ACMP0
#define ACMP0_BUS                     CDBUSALLOC      // GPIO Bus for ACMP0

#define ACMP1_OUTPUT_PORT             gpioPortA       // ACMP1 output port
#define ACMP1_OUTPUT_PIN              7               // ACMP1 output pin
#define ACMP1_INPUT_PORT              gpioPortC       // ACMP1 input port D
#define ACMP1_INPUT_PIN               7               // ACMP1 input pin 2
#define ACMP1_INPUT_PORT_PIN          acmpInputPC7
#define ACMP1_BUSALLOC                GPIO_CDBUSALLOC_CDODD1_ACMP1     // Bus allocation for ACMP1
#define ACMP1_BUS                     CDBUSALLOC      // GPIO Bus for ACMP1

#define NULL_POINT_DETECTOR_PORT      gpioPortA       // PRS output port
#define NULL_POINT_DETECTOR_PIN       6               // PRS output pin

#define PWM_PORT                      gpioPortB       // LED 0 port
#define PWM_PIN                       2               // LED 0 pin

#define PRS_CH_ACMP0                  3       // PRS channel for ACMP0 output
#define PRS_CH_ACMP1                  4       // PRS channel for ACMP1 output
#define PRS_CH_NULL_DETECTOR          1       // PRS channel for null detector
#define PRS_CH_PWM_OUTPUT             2       // PRS channel for PWM

#endif /* SINE_NULL_DETECTOR_CONFIG_H_ */
