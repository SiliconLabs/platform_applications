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

#include "sine_null_detector.h"
#include "sine_null_detector_config.h"

#include "em_acmp.h"
#include "em_cmu.h"
#include "em_prs.h"
#include "em_emu.h"
#include "em_letimer.h"

#include "sl_power_manager.h"
#include "sl_simple_button_instances.h"

#include "app_log.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/
#ifndef BUTTON_INSTANCE_0
#define BUTTON_INSTANCE_0             sl_button_btn0    // BTN0 instance
#endif

#ifndef BUTTON_INSTANCE_1
#define BUTTON_INSTANCE_1             sl_button_btn1    // BTN1 instance
#endif

#define ACMP_COMPARATOR_VOLTAGE       1.65    // Reference voltage for null detection
#define ACMP_MAX_DIVIDER              63      // Max divider value of ACMP
#define ACMP_REFERENCE_VOLTAGE        2.5     // ACMP input reference voltage

#define OUT_FREQ                      100     // Desired frequency in Hz
#define MAX_DUTY_CYCLE                100     // Duty cycle percentage
#define DUTY_CYCLE_STEP               5       // Step limit for increase/decrease

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/
// Flag to indicate to power manager if application can sleep
// The application will start in EM0
static volatile bool ok_to_sleep = false;

// Flag to indicate to power manager if the application should return to sleep
// after an interrupt
static volatile sl_power_manager_on_isr_exit_t isr_ok_to_sleep =
  SL_POWER_MANAGER_IGNORE;

// Target energy mode
static volatile sl_power_manager_em_t em_mode = SL_POWER_MANAGER_EM0;

// Duty cycle of PWM
static volatile uint32_t duty_cycle = 0;

// Top value of LETIMER, which generates the PWM
static uint32_t topValue = 0;

// State of the main state machine
typedef enum status{
  DUTY_CYCLE_CHANGE,
  IDLE
}status_t;

// Default state of state machine
static volatile status_t status = IDLE;

// Input reference voltage divider for defining the reference voltage
// of null point to ACMP input
static uint32_t input_reference_voltage_div = 0;

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * Hook for power manager
 ******************************************************************************/
bool app_is_ok_to_sleep(void)
{
  // return false to prevent sleep mode and force EM0
  // return true to indicate to power manager that application can sleep
  return ok_to_sleep;
}

/***************************************************************************//**
 * Hook for power manager
 ******************************************************************************/
sl_power_manager_on_isr_exit_t app_sleep_on_isr_exit(void)
{
  // flag used by power manager to determine if device can return to sleep
  // following interrupt
  return isr_ok_to_sleep;
}

/***************************************************************************//**
 * GPIO initialization
 ******************************************************************************/
void initGPIO(void)
{
  // Enable GPIO clock
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Configure ACMP0, ACMP1, PRS (output of null point detector) and PWM GPIO output pins
#if DEBUG_MODE_ENABLE == 1
  GPIO_PinModeSet(ACMP0_OUTPUT_PORT, ACMP0_OUTPUT_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(ACMP1_OUTPUT_PORT, ACMP1_OUTPUT_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(NULL_POINT_DETECTOR_PORT,
                  NULL_POINT_DETECTOR_PIN,
                  gpioModePushPull,
                  0);
#endif
  GPIO_PinModeSet(PWM_PORT, PWM_PIN, gpioModePushPull, 0);
}

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
void initACMP0(void)
{
  // Enable ACMP0 clock
  CMU_ClockEnable(cmuClock_ACMP0, true);

  /*
   * The reference voltage for comparator is 2.5V and this value will divide
   * with this divider value so that the real reference voltage will be 1.65 V
   * (which is the null point of 3.3 Vpp and +1.65 Voff sine wave).
   *
   * The new reference voltage divider:
   * MaxDivider * (Vreal_reference / Vref) = 63 * (1.65 V / 2.5 V) = 42
   */

  input_reference_voltage_div = (uint32_t) ACMP_MAX_DIVIDER
                                * (ACMP_COMPARATOR_VOLTAGE
                                   / ACMP_REFERENCE_VOLTAGE);

  // Configure ACMP0 settings
  ACMP_Init_TypeDef init = ACMP_INIT_DEFAULT;
  init.vrefDiv = input_reference_voltage_div;
  init.accuracy = acmpAccuracyHigh;
  init.hysteresisLevel = acmpHysteresis30Sym; // For clearing transients

  // ACMP0 initialization
  ACMP_Init(ACMP0, &init);

  // Allocate CDEVEN0 to ACMP0 to be able to use the input
  GPIO->ACMP0_BUS |= ACMP0_BUSALLOC;

  /*
   * Configure ACMP0 to compare the specified input pin against the
   * selected and divided reference.
   */
  ACMP_ChannelSet(ACMP0, acmpInputVREFDIV2V5, ACMP0_INPUT_PORT_PIN);

  // Wait for warm-up
  while (!(ACMP0->IF & ACMP_IF_ACMPRDY)) {
    // Empty while cycle for waiting warm-up
  }
}

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
void initACMP1(void)
{
  // Enable ACMP1 clock
  CMU_ClockEnable(cmuClock_ACMP1, true);

  /*
   * The reference voltage for comparator is 2.5V and this value will divide
   * with this divider value so that the real reference voltage will be 1.65 V
   * (which is the null point of 3.3 Vpp and +1.65 Voff sine wave).
   *
   * The new reference voltage divider:
   * MaxDivider * (Vreal_reference / Vref) = 63 * (1.65 V / 2.5 V) = 42
   */

  input_reference_voltage_div = (uint32_t) ACMP_MAX_DIVIDER
                                * (ACMP_COMPARATOR_VOLTAGE
                                   / ACMP_REFERENCE_VOLTAGE);

  // Configure ACMP1 settings
  ACMP_Init_TypeDef init = ACMP_INIT_DEFAULT;
  init.vrefDiv = input_reference_voltage_div;
  init.accuracy = acmpAccuracyHigh;
  init.hysteresisLevel = acmpHysteresis30Sym; // For clearing transients

  // ACMP0 initialization
  ACMP_Init(ACMP1, &init);

  // Allocate CDODD1 to ACMP1 to be able to use the input
  GPIO->ACMP1_BUS |= ACMP1_BUSALLOC;

  /*
   * Configure ACMP1 to compare the specified input pin against the
   * selected and divided reference.
   */
  ACMP_ChannelSet(ACMP1, ACMP1_INPUT_PORT_PIN, acmpInputVREFDIV2V5);

  // Wait for warm-up
  while (!(ACMP1->IF & ACMP_IF_ACMPRDY)) {
    // Empty while cycle for waiting warm-up
  }
}

/***************************************************************************//**
 * PRS initialization
 ******************************************************************************/
void initPrs(void)
{
  // Enable PRS clock
  CMU_ClockEnable(cmuClock_PRS, true);

  // Connect the output of ACMP0 to PRS_CH_ACMP0 channel as producer
  PRS_SourceAsyncSignalSet(PRS_CH_ACMP0,
                           PRS_ASYNC_CH_CTRL_SOURCESEL_ACMP0,
                           PRS_ASYNC_CH_CTRL_SIGSEL_ACMP0OUT);

#if DEBUG_MODE_ENABLE == 1
  // Connect the PRS_CH_ACMP0 PRS channel output to GPIO pins
  PRS_PinOutput(PRS_CH_ACMP0, prsTypeAsync, ACMP0_OUTPUT_PORT,
                ACMP0_OUTPUT_PIN);
#endif

  // Connect the output of ACMP1 to PRS_CH_ACMP1 channel as producer
  PRS_SourceAsyncSignalSet(PRS_CH_ACMP1,
                           PRS_ASYNC_CH_CTRL_SOURCESEL_ACMP1,
                           PRS_ASYNC_CH_CTRL_SIGSEL_ACMP1OUT);

#if DEBUG_MODE_ENABLE == 1
  // Connect the PRS_CH_ACMP1 PRS channel outputs to GPIO pins
  PRS_PinOutput(PRS_CH_ACMP1, prsTypeAsync, ACMP1_OUTPUT_PORT,
                ACMP1_OUTPUT_PIN);
#endif

  // Connect the output of ACMP0 to PRS_CH_NULL_DETECTOR channel as producer
  PRS_SourceAsyncSignalSet(PRS_CH_NULL_DETECTOR,
                           PRS_ASYNC_CH_CTRL_SOURCESEL_ACMP0,
                           PRS_ASYNC_CH_CTRL_SIGSEL_ACMP0OUT);

  /*
   * Use PRS Logic to get impulse in null point of sine wave
   * (null point is when the values of two ACMP is the same
   * and this is the XNOR logic)
   */
  PRS_Combine(PRS_CH_NULL_DETECTOR, PRS_CH_ACMP1, prsLogic_A_XNOR_B);

#if DEBUG_MODE_ENABLE == 1
  // Connect the PRS_CH_NULL_DETECTOR PRS channel outputs to GPIO pins
  PRS_PinOutput(PRS_CH_NULL_DETECTOR,
                prsTypeAsync,
                NULL_POINT_DETECTOR_PORT,
                NULL_POINT_DETECTOR_PIN);
#endif

  // Connect the specified PRS channel to the START of LETIMER0 as the consumer
  PRS_ConnectConsumer(PRS_CH_NULL_DETECTOR,
                      prsTypeAsync,
                      prsConsumerLETIMER0_START);

  // Connect the specified PRS channel to the CLEAR of LETIMER0 as the consumer
  PRS_ConnectConsumer(PRS_CH_NULL_DETECTOR,
                      prsTypeAsync,
                      prsConsumerLETIMER0_CLEAR);

  // Connect the output of LETIMER0 to PRS_CH_PWM_OUTPUT channel as producer
  PRS_SourceAsyncSignalSet(PRS_CH_PWM_OUTPUT,
                           PRS_ASYNC_CH_CTRL_SOURCESEL_LETIMER0,
                           PRS_ASYNC_CH_CTRL_SIGSEL_LETIMER0CH0);

  // Connect the PRS_CH_PWM_OUTPUT PRS channel outputs to GPIO pins
  PRS_PinOutput(PRS_CH_PWM_OUTPUT, prsTypeAsync, PWM_PORT, PWM_PIN);
}

/**************************************************************************//**
 * LETIMER initialization for PWM generation
 *****************************************************************************/
void initLetimer0(void)
{
  // Enable LETIMER0 clock
  CMU_ClockEnable(cmuClock_LETIMER0, true);

  // LETIMER0 default initialization
  LETIMER_Init_TypeDef letimerInit = LETIMER_INIT_DEFAULT;

  // Calculate the top value (frequency) based on clock source
  topValue = CMU_ClockFreqGet(cmuClock_LETIMER0) / OUT_FREQ;

  // Reload top on underflow, PWM output, and run in free mode
  letimerInit.comp0Top = true;
  letimerInit.topValue = topValue * duty_cycle / 100;
  letimerInit.ufoa0 = letimerUFOAPwm;
  letimerInit.repMode = letimerRepeatFree;
  letimerInit.enable = false;

  // Enable the LETIMER0 PRS input mode (Start for falling edge, clear for rising edge)
  LETIMER0->PRSMODE = LETIMER_PRSMODE_PRSSTARTMODE_FALLING \
                      | LETIMER_PRSMODE_PRSCLEARMODE_RISING;

  // Initialize LETIMER0
  LETIMER_Init(LETIMER0, &letimerInit);
}

/***************************************************************************//**
 * Initialize example.
 ******************************************************************************/
void sine_null_detector_init(void)
{
  app_log_debug("Program Starts \r\n");
  app_log_debug("PWM duty cycle is %lu \r\n", duty_cycle);

  // Initializing peripherals
  initGPIO();
  initPrs();
  initLetimer0();
  initACMP0();
  initACMP1();

  // Setting energy mode requirement to EM2
  isr_ok_to_sleep = SL_POWER_MANAGER_SLEEP;
  ok_to_sleep = true;
  em_mode = SL_POWER_MANAGER_EM2;
}

/***************************************************************************//**
 * Ticking function.
 * DUTY_CYCLE_CHANGE : If the value of duty cycle is changed, then this state
 *                     will set the new topValue of LETIMER.
 * IDLE : Default state of state machine. This is an empty state.
 ******************************************************************************/
void sine_null_detector_process_action(void)
{
  switch (status) {
    case DUTY_CYCLE_CHANGE:
      app_log_debug("PWM duty cycle is %lu \r\n", duty_cycle);
      LETIMER_TopSet(LETIMER0, topValue * duty_cycle / 100);
      status = IDLE;
      isr_ok_to_sleep = SL_POWER_MANAGER_SLEEP;
      ok_to_sleep = true;
      em_mode = SL_POWER_MANAGER_EM2;
      break;
    case IDLE:
      break;
    default:
      EFM_ASSERT(false);
      break;
  }
}

/***************************************************************************//**
 * Button interrupt.
 * BTN0 : Increase the duty cycle of PWM
 * BTN1 : Decrease the duty cycle of PWM
 ******************************************************************************/
void sl_button_on_change(const sl_button_t *handle)
{
  if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_PRESSED) {
    isr_ok_to_sleep = SL_POWER_MANAGER_WAKEUP;
    ok_to_sleep = false;
    em_mode = SL_POWER_MANAGER_EM0;
    if (&BUTTON_INSTANCE_0 == handle) {
      duty_cycle += DUTY_CYCLE_STEP;
      if (duty_cycle >= MAX_DUTY_CYCLE) {
        duty_cycle = MAX_DUTY_CYCLE;
      }
    }
    if (&BUTTON_INSTANCE_1 == handle) {
      if (duty_cycle <= DUTY_CYCLE_STEP) {
        duty_cycle = 0;
      } else {
        duty_cycle -= DUTY_CYCLE_STEP;
      }
    }
    status = DUTY_CYCLE_CHANGE;
  }
}
