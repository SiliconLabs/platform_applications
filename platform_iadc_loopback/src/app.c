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

#include "app.h"

#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_timer.h"
#include "em_vdac.h"
#include "em_gpio.h"
#include "em_iadc.h"
#include "app_log.h"

#include "stdbool.h"

// Local variables

// Enable the IADCs measurement logging
static volatile bool IADC_measurement_completed = false;

// Raw IADC conversion result
static volatile IADC_Result_t sample;

// Result converted to volts
static volatile float single_result;

// CMU initialization
static void initCMU(void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);
  // The EM01GRPACLK is chosen as VDAC clock source since the VDAC will be
  // operating in EM1
  CMU_ClockSelectSet(cmuClock_VDAC0, cmuSelect_EM01GRPACLK);
  // Enable the VDAC clocks
  CMU_ClockEnable(cmuClock_VDAC0, true);
  CMU_ClockEnable(cmuClock_TIMER0, true);

  CMU_ClockSelectSet(cmuClock_IADCCLK, cmuSelect_EM01GRPACLK);
  CMU_ClockEnable(cmuClock_IADC0, true);
}

// VDAC initialization
void initVDAC(void)
{
  // Use default settings
  VDAC_Init_TypeDef        init = VDAC_INIT_DEFAULT;
  VDAC_InitChannel_TypeDef initChannel = VDAC_INITCHANNEL_DEFAULT;

  // Calculate the VDAC clock prescaler value resulting in a 1 MHz VDAC clock.
  init.prescaler = VDAC_PrescaleCalc(VDAC0, (uint32_t)VDAC_CLK_FREQ);

  // Set reference to internal 1.25V low noise reference
  init.reference = vdacRef1V25;

  // Enable sine mode
  init.sineEnable = true;

  // Set the output mode to continuous as required for the sine mode
  initChannel.sampleOffMode = false;

  // Since the minimum load requirement for high capacitance mode is 25 nF, turn
  // this mode off
  initChannel.highCapLoadEnable = false;

  // Main Output disable
  initChannel.mainOutEnable = false;
  // Alternative output enable
  initChannel.auxOutEnable = true;

  initChannel.port = VDAC0_PORT;
  initChannel.pin = VDAC0_PIN;
  GPIO->VDAC_0_BUSALLOC |= VDAC0_BUS_REGISTER;

  // Trigger mode and refresh source should be programmed to None for the sine
  // mode to avoid interference in sine output generation from other triggers
  initChannel.trigMode = vdacTrigModeNone;
  initChannel.chRefreshSource = vdacRefreshSrcNone;

  // Initialize the VDAC and VDAC channel
  VDAC_Init(VDAC0, &init);
  VDAC_InitChannel(VDAC0, &initChannel, VDAC_CHANNEL_NUM);

  // Enable the VDAC
  VDAC_Enable(VDAC0, VDAC_CHANNEL_NUM, true);
}

// TIMER0 initialization
static void initTIMER(void)
{
  uint32_t timerFreq;
  uint32_t topValue;

  // Initialize TIMER0
  TIMER_Init_TypeDef init = TIMER_INIT_DEFAULT;
  TIMER_InitCC_TypeDef timer_CCInit = TIMER_INITCC_DEFAULT;

  init.enable = false;

  // Configure capture/compare channel for output compare
  timer_CCInit.mode = timerCCModeCompare;
  timer_CCInit.cofoa = timerOutputActionToggle;

  TIMER_Init(TIMER0, &init);

  // Timer Compare/Capture channel 0 initialization
  TIMER_InitCC(TIMER0, 0, &timer_CCInit);

  // Set compare (reload) value for the timer
  // Note: the timer runs off of the EM01GRPACLK clock
  timerFreq = CMU_ClockFreqGet(cmuClock_TIMER0) / (init.prescale + 1);

  topValue = timerFreq / (2 * TIMER_FREQ) - 1;
  TIMER_TopSet(TIMER0, topValue);

  // Enable TIMER0
  TIMER_Enable(TIMER0, true);
}

// IADC initialization
void initIADC(void)
{
  // Declare initialization structures
  IADC_Init_t init = IADC_INIT_DEFAULT;
  IADC_AllConfigs_t initAllConfigs = IADC_ALLCONFIGS_DEFAULT;
  IADC_InitSingle_t initSingle = IADC_INITSINGLE_DEFAULT;

  // Single input structure
  IADC_SingleInput_t singleInput = IADC_SINGLEINPUT_DEFAULT;

  // Set the prescaler needed for the intended IADC clock frequency
  init.srcClkPrescale = IADC_calcSrcClkPrescale(IADC0, IADC_CLK_SRC_FREQ, 0);

  // Shutdown between conversions to reduce current
  init.warmup = iadcWarmupNormal;

  // Configuration 0 is used by both scan and single conversions by
  // default.  Use internal bandgap as the reference and specify the
  // reference voltage in mV.

  // Resolution is not configurable directly but is based on the
  // selected oversampling ratio (osrHighSpeed), which defaults to
  // 2x and generates 12-bit results.

  initAllConfigs.configs[0].reference = iadcCfgReferenceInt1V2;
  initAllConfigs.configs[0].vRef = 1210;
  initAllConfigs.configs[0].osrHighSpeed = iadcCfgOsrHighSpeed2x;
  initAllConfigs.configs[0].analogGain = iadcCfgAnalogGain0P5x;

  // CLK_SRC_ADC must be prescaled by some value greater than 1 to
  // derive the intended CLK_ADC frequency.

  // Based on the default 2x oversampling rate (OSRHS)...
  // conversion time = ((4 * OSRHS) + 2) / fCLK_ADC
  // ...which results in a maximum sampling rate of 833 ksps with the
  // 2-clock input multiplexer switching time is included.

  initAllConfigs.configs[0].adcClkPrescale = IADC_calcAdcClkPrescale(IADC0,
                                                                     IADC_CLK_FREQ,
                                                                     0,
                                                                     iadcCfgModeNormal,
                                                                     init.srcClkPrescale);

  // Specify the input channel.  When negInput = iadcNegInputGnd, the
  // conversion is single-ended.

  singleInput.posInput = IADC0_POS_INPUT;
  singleInput.negInput = IADC0_NEG_INPUT;

  // Allocate the analog bus for ADC0 inputs
  GPIO->IADC0_BUSALLOC |= IADC0_BUS_REGISTER;

  // Initialize IADC
  IADC_init(IADC0, &init, &initAllConfigs);

  // Initialize a single-channel conversion
  IADC_initSingle(IADC0, &initSingle, &singleInput);

  // Clear any previous interrupt flags
  IADC_clearInt(IADC0, _IADC_IF_MASK);

  // Enable single-channel done interrupts
  IADC_enableInt(IADC0, IADC_IEN_SINGLEDONE);

  // Enable IADC interrupts
  NVIC_ClearPendingIRQ(IADC_IRQn);
  NVIC_EnableIRQ(IADC_IRQn);
}

// Initializing the application
void app_init(void)
{
  EMU_DCDCInit_TypeDef dcdcInit = EMU_DCDCINIT_DEFAULT;
  // Enable DC-DC converter
  EMU_DCDCInit(&dcdcInit);

  // Initialize clocks
  initCMU();

  // Initialize Timer
  initTIMER();

  // Initialize VDAC
  initVDAC();

  // Initialize IADC
  initIADC();

  // Start Sine Wave generation
  VDAC_SineModeStart(VDAC0, true);
  IADC_command(IADC0, iadcCmdStartSingle);
}

// App ticking function
void app_process_action(void)
{
  if (IADC_measurement_completed) {
    app_log_info("IADC0's measured value is %f\n", single_result);
    IADC_measurement_completed = false;
    IADC_command(IADC0, iadcCmdStartSingle);
  }
  // Enter Sleep mode
  EMU_EnterEM1();
}

// IADC interrupt handler
// It will negate a flag that will enable the logging
// in the app_process_action()
void IADC_IRQHandler(void)
{
  // Read a result from the FIFO
  sample = IADC_pullSingleFifoResult(IADC0);

  // Calculate the voltage converted as follows:
  // For single-ended conversions, the result can range from 0 to
  // +Vref, i.e., for Vref = VBGR = 1.21V, and with analog gain = 0.5
  // 0xFFF represents the full scale value of 2.42V.
  single_result = sample.data * 2.42 / 0xFFF;
  IADC_measurement_completed = true;

  // Clear the single conversion complete interrupt.  Reading FIFO
  // results does not do this automatically.

  IADC_clearInt(IADC0, IADC_IF_SINGLEDONE);
}
