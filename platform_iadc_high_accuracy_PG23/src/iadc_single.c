/***************************************************************************//**
 * @file
 * @brief IADC example functions
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

#include <stdio.h>
#include "em_cmu.h"
#include "em_emu.h"
#include "em_iadc.h"
#include "em_gpio.h"
#include "em_prs.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

// Use specified PRS channel
#define IADC_PRS_CH               0

// Set HFRCOEM23 to lowest frequency (1MHz)
#define HFRCOEM23_FREQ            cmuHFRCOEM23Freq_1M0Hz

// Set CLK_ADC to 10MHz
#define CLK_SRC_ADC_FREQ          1000000 // CLK_SRC_ADC
#define CLK_ADC_FREQ              1000000 // CLK_ADC - 5MHz max in hiacc mode
// This corresponds to a sample rate of ~388Sps with OSR = 256 and DIGAVG = 2

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
#define IADC_INPUT_0_PORT_PIN     iadcPosInputPortAPin5;

#define IADC_INPUT_0_BUS          ABUSALLOC
#define IADC_INPUT_0_BUSALLOC     GPIO_ABUSALLOC_AODD0_ADC0

// GPIO output toggle to notify IADC conversion complete
#define GPIO_OUTPUT_0_PORT        gpioPortB
#define GPIO_OUTPUT_0_PIN         4

/*******************************************************************************
 ***************************   GLOBAL VARIABLES   ******************************
 ******************************************************************************/

// Stores latest ADC sample and converts to volts
static volatile IADC_Result_t sample;
static volatile double singleResult;

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/

bool iadc_done = false;

/**************************************************************************//**
 * @brief  GPIO Initializer
 *****************************************************************************/
void initGPIO(void)
{
  // Enable GPIO clock branch
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Configure GPIO as output, will indicate when conversions are being
  //   performed
  GPIO_PinModeSet(GPIO_OUTPUT_0_PORT, GPIO_OUTPUT_0_PIN, gpioModePushPull, 0);
}

/**************************************************************************//**
 * @brief  PRS Initializer
 *****************************************************************************/
void initPRS(void)
{
  // Enable PRS clock
  CMU_ClockEnable(cmuClock_PRS, true);

  // Connect PRS Async channel to ADC single complete signal
  PRS_SourceAsyncSignalSet(IADC_PRS_CH, PRS_ASYNC_CH_CTRL_SOURCESEL_IADC0,
                           PRS_ASYNC_CH_CTRL_SIGSEL_IADC0SINGLEDONE);

  // Route PRS channel to GPIO output to indicate a conversion complete
  PRS_PinOutput(IADC_PRS_CH, prsTypeAsync, GPIO_OUTPUT_0_PORT,
                GPIO_OUTPUT_0_PIN);
}

/**************************************************************************//**
 * @brief  IADC Initializer
 *****************************************************************************/
void initIADC(void)
{
  // Declare init structs
  IADC_Init_t init = IADC_INIT_DEFAULT;
  IADC_AllConfigs_t initAllConfigs = IADC_ALLCONFIGS_DEFAULT;
  IADC_InitSingle_t initSingle = IADC_INITSINGLE_DEFAULT;
  IADC_SingleInput_t initSingleInput = IADC_SINGLEINPUT_DEFAULT;

  // Enable IADC clock
  CMU_ClockEnable(cmuClock_IADC0, true);

  // Reset IADC to reset configuration in case it has been modified
  IADC_reset(IADC0);

  // Configure IADC clock source
  CMU_ClockSelectSet(cmuClock_IADCCLK, cmuSelect_HFRCOEM23);

  // Modify init structs and initialize
  init.warmup = iadcWarmupNormal;

  // Set the HFSCLK prescale value here
  init.srcClkPrescale = IADC_calcSrcClkPrescale(IADC0, CLK_SRC_ADC_FREQ, 0);

  // Configuration 0 is used by both scan and single conversions by default
  // Use unbuffered AVDD as reference
  initAllConfigs.configs[0].reference = iadcCfgReferenceVddx;

  // Divides CLK_SRC_ADC to set the CLK_ADC frequency for desired sample rate
  initAllConfigs.configs[0].adcClkPrescale = IADC_calcAdcClkPrescale(IADC0,
                                                                     CLK_ADC_FREQ,
                                                                     0,
                                                                     iadcCfgModeNormal,
                                                                     init.srcClkPrescale);

  initAllConfigs.configs[0].adcMode = iadcCfgModeHighAccuracy;

  // Set oversampling rate to 256x; digital averaging to 2x
  // resolution formula res = 11 + log2(oversampling * digital averaging)
  // in this case res = 11 + log2(256 * 2) = 20
  initAllConfigs.configs[0].osrHighAccuracy = iadcCfgOsrHighAccuracy256x;
  initAllConfigs.configs[0].digAvg = iadcDigitalAverage2;

  // Single initialization
  initSingle.dataValidLevel = _IADC_SINGLEFIFOCFG_DVL_VALID1;

  // Set conversions to run continuously
  initSingle.triggerAction = iadcTriggerActionContinuous;

  // Set alignment to right justified with 20 bits for data field
  initSingle.alignment = iadcAlignRight20;

  // Configure Input sources for single ended conversion
  initSingleInput.posInput = IADC_INPUT_0_PORT_PIN;
  initSingleInput.negInput = iadcNegInputGnd;

  // Initialize IADC
  // Note oversampling and digital averaging will affect the offset correction
  // This is taken care of in the IADC_init() function in the emlib
  IADC_init(IADC0, &init, &initAllConfigs);

  // Initialize Scan
  IADC_initSingle(IADC0, &initSingle, &initSingleInput);

  // Allocate the analog bus for ADC0 inputs
  GPIO->IADC_INPUT_0_BUS |= IADC_INPUT_0_BUSALLOC;

  // Enable interrupts on data valid level
  IADC_enableInt(IADC0, IADC_IEN_SINGLEDONE);

  // Enable ADC interrupts
  NVIC_ClearPendingIRQ(IADC_IRQn);
  NVIC_EnableIRQ(IADC_IRQn);
}

/**************************************************************************//**
 * @brief  ADC Handler
 *****************************************************************************/
void IADC_IRQHandler(void)
{
  IADC_clearInt(IADC0, IADC_IF_SINGLEDONE);

  iadc_done = true;
}

/***************************************************************************//**
 * Initialize IADC for single 20-bit high accuracy conversion.
 ******************************************************************************/
void iadc_single_init(void)
{
  // Switch to running from the FSRCO
  CMU_ClockSelectSet(cmuClock_SYSCLK, cmuSelect_FSRCO);

  // Set clock frequency to defined value
  CMU_HFRCOEM23BandSet(HFRCOEM23_FREQ);

  // Initialize GPIO
  initGPIO();

  // Initialize PRS
  initPRS();

  // Initialize the IADC
  initIADC();

  // Start single conversion
  IADC_command(IADC0, iadcCmdStartSingle);
}

/***************************************************************************//**
 * IADC conversion complete process action.
 ******************************************************************************/
void iadc_single_process_action(void)
{
  if (iadc_done == true) {
    // Read most recent single conversion result
    sample = IADC_readSingleResult(IADC0);

    // For single-ended the result range is 0 to +Vref, i.e., 20 bits for the
    // conversion value.
    singleResult = sample.data * 3.3 / 0xFFFFF;

    // Start new single conversion
    IADC_command(IADC0, iadcCmdStartSingle);

    iadc_done = false;
  }
}
