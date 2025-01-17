#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "em_acmp.h"
#include "em_cmu.h"
#include "em_gpio.h"

#define BLDC_FW_VER 0x03

#if EFM32GG990F1024
#include "config_gg.h"
#elif EFM32HG322F64
#include "config_hg.h"
#elif EFR32MG24B210F1536IM48
#include "config_mg24.h"
#elif EFR32ZG28B322F1024IM68
#include "config_zg28.h"
#endif

#define DEBUG_GPIO

/* Overflow value of TIMERs */
#define TIMER_MAX                      65536

/* Enum values for COMMUTATION_METHOD */
#define COMMUTATION_HALL               1
#define COMMUTATION_SENSORLESS         2

/**********************************************************
 *
 * MOTOR CONFIGURATION
 *
 * Change these values to match the motor setup
 *
 *********************************************************/

/* Must match the number of motor pole pairs */
#define MOTOR_POLE_PAIRS               3

/* This parameter controls whether Hall or Sensorless
 * commutation is used */
#define COMMUTATION_METHOD             COMMUTATION_SENSORLESS

/**********************************************************
 *
 * SPEED SETPOINT
 *
 * Set various properities for the RPM setpoint
 *
 **********************************************************/

/* Default speed setpoint in RPM */
#define DEFAULT_SETPOINT_RPM           2800

/* Minimum value for the speed setpoint (in RPM) */
#define SETPOINT_MIN_RPM               500

/* Maximum value for the speed setpoint (in RPM) */
#define SETPOINT_MAX_RPM               35000

/* The step size for each speed increment/decrement (in RPM) */
#define SPEED_INCREMENT_RPM            800

/**********************************************************
 *
 * PULSE WIDTH MODULATION
 *
 * Control the behavior of the PWM waveform
 *
 **********************************************************/

/* The PWM period in us. */
#if defined (EFM32GG990F1024)
#define PWM_PERIOD_US                  40
#elif defined (EFM32HG322F64)
#define PWM_PERIOD_US                  120
#elif EFR32MG24B210F1536IM48
#define PWM_PERIOD_US                  80
#elif EFR32ZG28B322F1024IM68
#define PWM_PERIOD_US                  80
#endif

/* The minimum PWM duty cycle in percent */
#define PWM_MIN_PERCENT                1.0

/* The maximum PWM duty cycle in percent */
#define PWM_MAX_PERCENT                90.0

/* The initial PWM duty cycle. If the motor has
 * trouble to begin spinning during sensorless
 * startup, increase this parameter */
#define PWM_DEFAULT_DUTY_CYCLE_PERCENT 8

/**********************************************************
 *
 * DEAD TIME INSERTION
 *
 * Controls the amount of dead time inserted
 *
 **********************************************************/

/* Uncomment this to use complementary PWM with Dead Time Insertion */
#define USE_COMPLEMENTARY_PWM

/* Set the prescaler for the Dead Time Generator. See the reference
 * manual for possible values. Note that this prescales the HFPERCLK
 * and is independent of the prescaler used for the TIMER itself. */
#define DEAD_TIME_PRESCALER                15

/* The number of (prescaled) Dead Time cycles for a rising edge of the PWM. */
#define DEAD_TIME_CYCLES_RISING_EDGE       4

/* The number of (prescaled) Dead Time cycles for a falling edge of the PWM. */
#define DEAD_TIME_CYCLES_FALLING_EDGE      4

/**********************************************************
 *
 * TIMING PARAMETERS
 *
 * Various timing parameters and clock frequencies
 *
 **********************************************************/

/* The HF clock frequency. If using a different crystal or HFRCO band
 * change this to match the nominal frequency. */
#if defined (EFM32GG990F1024)
#define CORE_FREQUENCY                     48000000
#elif defined (EFM32HG322F64)
#define CORE_FREQUENCY                     24000000
#elif EFR32MG24B210F1536IM48
#define CORE_FREQUENCY                     39000000
#elif EFR32ZG28B322F1024IM68
#define CORE_FREQUENCY                     39000000
#endif

/* Prescaler used for TIMER0 (PWM generation). Should not need to change this */
#define PRESCALER_TIMER0                   1

/* Prescaler used for TIMER1, used for timing commutation. Should normally
 * not need to change this. */
#define PRESCALER_TIMER1                   32

/* The PID controller period. The controller algorithm is invoked
 * every PID_PERIOD_MS milliseconds. */
#if defined (EFM32GG990F1024)
#define PID_PERIOD_MS                      6
#elif defined (EFM32HG322F64)
#define PID_PERIOD_MS                      24
#elif EFR32MG24B210F1536IM48
#define PID_PERIOD_MS                      24
#elif EFR32ZG28B322F1024IM68
#define PID_PERIOD_MS                      24
#endif

/* Number of milliseconds without a commutation event that should
 * trigger a 'Motor Stall' condition and shut off the motor */
#define STALL_TIMEOUT_MS                   500

/* Number of triggered commutations to do
 * after manual startup is complete, but before
 * before starting PID regulator. This helps the
 * measured speed to settle and avoids bumps when
 * switching to closed loop regulation */
#define PID_STARTUP_COMMUTATIONS           (30)

/**********************************************************
 *
 * HALL STARTUP
 *
 **********************************************************/

/* This parameter sets the number of manual commutations
 * before starting the Hall commutations. The startup
 * commutations are perfomed so that the motor has
 * some speed before the Hall GPIO interrupts are enabled. */
#define STARTUP_COMMUTATIONS_HALL          100

/**********************************************************
 *
 * SENSORLESS STARTUP
 *
 * These parameters control the motor startup sequence
 * in sensorless (back-emf) driven mode. See also
 * PWM_DEFAULT_DUTY_CYCLE_PERCENT.
 *
 **********************************************************/

/* The initial commutation delay. The commutation delay will
 * be decreased by a function that keeps a constant angular
 * acceleration. If the startup speed is too fast for the
 * motor to keep up, this parameter can be increased */
/* This value also determines the minimum TIMER1 prescale
 * value during startup. Since Series 2 devices can't change
 * the prescaler while the timer is enabled, the initial
 * startup period must be short enough so the timer doesn't
 * overflow, or the or PRESCALER_TIMER1 must be set higher. */
#define STARTUP_INITIAL_PERIOD_MS          50

/* The threshold for when the controller should switch from
 * the 'blindly' driven startup mode to back-emf driven
 * commutation. If the motor is able to follow the startup
 * timing, but stops when switching to back-emf, this parameter
 * can be increased.*/
#define STARTUP_FINAL_SPEED_RPM_SENSORLESS 2800

/**********************************************************
 *
 * CURRENT/OVERCURRENT MEASUREMENT
 *
 * These parameters control the current measurement which
 * is used for logging and overcurrent protection.
 *
 **********************************************************/

/* Uncomment this to enable current measurement */
#define CURRENT_MEASUREMENT_ENABLED

/* The maximum current before overcurrent protection
 * will shut off the motor */
#define MAX_CURRENT_MA   10000

/* The size of the current measurement resistor (in ohms) */
#define CURRENT_RESISTOR 0.05f

/* Select differential ADC channel used to measure motor current */
// #define ADC_CHANNEL adcSingleInpCh6Ch7

/**********************************************************
 *
 * DATA LOGGING
 *
 * These parameters control data logging. Real-time information
 * about the motor speed, current and input can be plotted
 * on a PC with the supplied efm32_bldc.exe
 *
 **********************************************************/

/* Uncomment this to enable logging to PC. When this is set
 * the firmware will report real-time data over UART */
#define BLDC_LOGGING_ENABLED

/* Set the UART baud rate. This must match the configuration
 * for efm32_bldc.exe */
#define UART_BAUDRATE          115200

/**********************************************************
 *
 * CALCULATED VALUES
 * (do not modify)
 *
 *********************************************************/

#define PWM_TOP                ((((CORE_FREQUENCY / PRESCALER_TIMER0) / 1000) \
                                 * PWM_PERIOD_US) / 1000)

#define PWM_DEFAULT_DUTY_CYCLE ((((CORE_FREQUENCY / PRESCALER_TIMER0) / 1000) \
                                 * (PWM_PERIOD_US                             \
                                    * PWM_DEFAULT_DUTY_CYCLE_PERCENT))        \
                                / (100 * 1000))

#define PWM_MIN                ((int)(((((CORE_FREQUENCY / PRESCALER_TIMER0) \
                                         / 1000)                             \
                                        * (PWM_PERIOD_US * PWM_MIN_PERCENT)) \
                                       / (100 * 1000))))

#define PWM_MAX                ((int)(((((CORE_FREQUENCY / PRESCALER_TIMER0) \
                                         / 1000)                             \
                                        * (PWM_PERIOD_US * PWM_MAX_PERCENT)) \
                                       / (100 * 1000))))

#define COUNT_TO_RPM(x) (((60 * (CORE_FREQUENCY / PRESCALER_TIMER1)) \
                          / MOTOR_POLE_PAIRS) / (x))

#define RPM_TO_COUNT(x) (((60 * (CORE_FREQUENCY / PRESCALER_TIMER1)) \
                          / MOTOR_POLE_PAIRS) / (x))

#define PID_PRESCALER          ((PID_PERIOD_MS * 1000) / PWM_PERIOD_US)

#define STALL_TIMEOUT_OF       ((STALL_TIMEOUT_MS * (CORE_FREQUENCY / 1000)) \
                                / (TIMER_MAX * PRESCALER_TIMER1))

#endif
