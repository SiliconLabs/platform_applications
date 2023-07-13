/***************************************************************************//**
 * @file
 * @brief IADC example functions
 *******************************************************************************
 * # License
 * <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
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
#include "em_ldma.h"
#include "em_gpio.h"
#include "em_prs.h"
#include "iadc_single.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

// How many samples to capture
#define NUM_SAMPLES                1024

#define IADC_PRS_CH                6 // using PortC for GPIO output; ASYNCH6 is
                                     // first available PRS_CH w/PortC access
                                     // See datasheet for more information.

// Set CLK_ADC to 5MHz
#define CLK_SRC_ADC_FREQ           10000000 // CLK_SRC_ADC
#define CLK_ADC_FREQ               5000000 // CLK_ADC - 5MHz max in hiacc mode
// This corresponds to a sample rate of ~3.8kSps with OSR = 256 and DIGAVG = 2

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
//// Use dedicated analog inputs in the IADC initialization
// #define IADC_INPUT_0_PORT_PIN     iadcPosInputPortAPin5;
//
// #define IADC_INPUT_0_BUS          ABUSALLOC
// #define IADC_INPUT_0_BUSALLOC     GPIO_ABUSALLOC_AODD0_ADC0

// GPIO output toggle to notify IADC conversion complete
#define GPIO_OUTPUT_0_PORT        gpioPortC
#define GPIO_OUTPUT_0_PIN         10

#define GPIO_OUTPUT_1_PORT        gpioPortC
#define GPIO_OUTPUT_1_PIN         11

#define ADC_REF_ENABLE_PORT       gpioPortD
#define ADC_REF_ENABLE_PIN        15

/*******************************************************************************
 ***************************   GLOBAL VARIABLES   ******************************
 ******************************************************************************/

// Descriptor linked list for LDMA transfer
LDMA_Descriptor_t descLink[2];

// buffer to store IADC samples
uint32_t singleBuffer1[NUM_SAMPLES];
uint32_t singleBuffer2[NUM_SAMPLES];

// used to toggle which buffer to perform statistical analysis;
uint32_t *dataBuffer = singleBuffer2;

double meanV = 0.0;

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/

bool ldma_done = false;

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

  // Configure GPIO as output, will indicate when conversions are being
  //   performed
  GPIO_PinModeSet(GPIO_OUTPUT_1_PORT, GPIO_OUTPUT_1_PIN, gpioModePushPull, 0);

  // Configure GPIO as output, enables ADR1581 1.25V analog reference
  GPIO_PinModeSet(ADC_REF_ENABLE_PORT, ADC_REF_ENABLE_PIN, gpioModePushPull, 1);
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

  // Select clock for IADC
  CMU_ClockSelectSet(cmuClock_IADCCLK, cmuSelect_FSRCO);

  // Modify init structs and initialize
  init.warmup = iadcWarmupKeepWarm;

  // Set the HFSCLK prescale value here
  init.srcClkPrescale = IADC_calcSrcClkPrescale(IADC0, CLK_SRC_ADC_FREQ, 0);

  // Configuration 0 is used by both scan and single conversions by default
  // Use unbuffered external 1.25V reference
  initAllConfigs.configs[0].reference = iadcCfgReferenceExt1V25;
  initAllConfigs.configs[0].vRef = 1250;
  initAllConfigs.configs[0].analogGain = iadcCfgAnalogGain0P5x;

  // Divides CLK_SRC_ADC to set the CLK_ADC frequency for desired sample rate
  initAllConfigs.configs[0].adcClkPrescale = IADC_calcAdcClkPrescale(IADC0,
                                                                     CLK_ADC_FREQ,
                                                                     0,
                                                                     iadcCfgModeNormal,
                                                                     init.srcClkPrescale);

  // Set oversampling rate to 256x; digital averaging to 1x
  // resolution formula res = 12 + log2(oversampling * digital averaging)
  // in this case res = 12 + log2(256 * 1) = 20
  initAllConfigs.configs[0].adcMode = iadcCfgModeHighAccuracy;
  initAllConfigs.configs[0].osrHighAccuracy = iadcCfgOsrHighAccuracy256x;
  initAllConfigs.configs[0].digAvg = iadcDigitalAverage1;
  // Conversion Time = ((5 * (OSR * digavg)) + 7) / fCLK_ADC
  //                 = 1287 / 5000000
  //                 = 257 us -> 3.8 kSps

  // Single initialization
  initSingle.dataValidLevel = _IADC_SINGLEFIFOCFG_DVL_VALID1;

  // Set conversions to run continuously
  initSingle.triggerAction = iadcTriggerActionContinuous;

  // Set alignment to right justified with 20 bits for data field
  initSingle.alignment = iadcAlignRight20;

  // Configure Input sources for single ended conversion
  initSingleInput.posInput = iadcPosInputPadAna0;
  initSingleInput.negInput = iadcNegInputPadAna1 | 1;

  // Initialize IADC
  // Note oversampling and digital averaging will affect the offset correction
  // This is taken care of in the IADC_init() function in the emlib
  IADC_init(IADC0, &init, &initAllConfigs);

  // Initialize Single
  IADC_initSingle(IADC0, &initSingle, &initSingleInput);
}

/**************************************************************************//**
 * @brief
 *   IADC Initializer
 *
 * @param[in] buffer
 *   pointer to the array where ADC data will be stored.
 * @param[in] size
 *   size of the array
 *****************************************************************************/
void initLDMA(uint32_t *buffer1, uint32_t *buffer2, uint32_t size)
{
  // Declare LDMA init structs
  LDMA_Init_t init = LDMA_INIT_DEFAULT;

  // Configure LDMA for transfer from IADC to memory
  // LDMA will loop continuously
  LDMA_TransferCfg_t transferCfg =
    LDMA_TRANSFER_CFG_PERIPHERAL(ldmaPeripheralSignal_IADC0_IADC_SINGLE);

  // Set up descriptors for peripheral to memory transfers that ping-pong
  //  transfers between to 2 buffers
  descLink[0] = (LDMA_Descriptor_t) \
                LDMA_DESCRIPTOR_LINKREL_P2M_BYTE(&IADC0->SINGLEFIFODATA,
                                                 buffer1,
                                                 size,
                                                 1);
  descLink[1] = (LDMA_Descriptor_t) \
                LDMA_DESCRIPTOR_LINKREL_P2M_BYTE(&IADC0->SINGLEFIFODATA,
                                                 buffer2,
                                                 size,
                                                 -1);

  // Modify descriptor for NUM_SAMPLES sized transfer from iadc to buffer
  // Transfer 32 bits per unit, increment by 32 bits
  descLink[0].xfer.size = ldmaCtrlSizeWord;
  descLink[1].xfer.size = ldmaCtrlSizeWord;
  descLink[0].xfer.blockSize = ldmaCtrlBlockSizeUnit1;
  descLink[1].xfer.blockSize = ldmaCtrlBlockSizeUnit1;

  // Set descriptor to loop NUM_SAMPLES times and then complete
  descLink[0].xfer.decLoopCnt = 0; // this is the default when
                                   // calling LDMA_DESCRIPTOR_LINKREL_P2M_BYTE;
                                   // disables loop counting; runs continuously
  descLink[0].xfer.xferCnt = size - 1;
  descLink[1].xfer.decLoopCnt = 0;
  descLink[1].xfer.xferCnt = size - 1;

  // Interrupt after each descriptor's transfer is complete
  descLink[0].xfer.doneIfs = 1;
  descLink[1].xfer.doneIfs = 1;

  // Initialize LDMA with default configuration
  LDMA_Init(&init);

  // Start transfer, LDMA/IADC samples NUM_SAMPLES and then LDMA interrupts
  LDMA_StartTransfer(0, (void *)&transferCfg, (void *)&descLink[0]);
}

/**************************************************************************//**
 * @brief  LDMA Handler
 *****************************************************************************/
void LDMA_IRQHandler(void)
{
  // Clear interrupt flags
  LDMA_IntClear(LDMA_IF_DONE0);

  if (dataBuffer == singleBuffer2) {
    dataBuffer = singleBuffer1;
  } else {
    dataBuffer = singleBuffer2;
  }

  ldma_done = true;

  // Toggle GPIO to notify that transfer is complete
  GPIO_PinOutToggle(GPIO_OUTPUT_1_PORT, GPIO_OUTPUT_1_PIN);
}

/***************************************************************************//**
 * @brief   Computes mean and variance of data set using Welford's algorithm
 ******************************************************************************/
void statsWelford(uint32_t *buffer, uint32_t size, double *mean, double *var)
{
  uint32_t cnt;
  double M, M2, delta1, delta2;

  M = 0;
  M2 = 0;
  for (cnt = 1; cnt <= size; cnt++) {
    delta1 = buffer[cnt - 1] - M;
    M += delta1 / cnt;
    delta2 = buffer[cnt - 1] - M;
    M2 += delta1 * delta2;
  }
  *mean = M;
  *var = M2 / (size - 1);
}

/***************************************************************************//**
 * Initialize IADC for single 20-bit high accuracy conversion.
 ******************************************************************************/
void iadc_single_init(void)
{
  // Switch to running from the FSRCO
  CMU_ClockSelectSet(cmuClock_SYSCLK, cmuSelect_FSRCO);

  // Initialize GPIO
  initGPIO();

  // Initialize PRS
  initPRS();

  // Initialize the IADC
  initIADC();

  // Initialize LDMA
  initLDMA(singleBuffer1, singleBuffer2, NUM_SAMPLES);

  // Start single conversion
  IADC_command(IADC0, iadcCmdStartSingle);
}

/***************************************************************************//**
 * IADC conversion complete process action.
 ******************************************************************************/
void iadc_single_process_action(void)
{
  double mean, variance;

  if (ldma_done == true) {
    // Process most recent buffer
    statsWelford(dataBuffer, NUM_SAMPLES, &mean, &variance);

    // Calculate input voltage:
    // For differential inputs, the resultant range is from -Vref to +Vref,
    //   i.e.,
    // with analog gain = 0.5 and Vref = 1.25V, 20 bits represents
    // 5.0V full scale IADC range (-2.5 <-> +2.5).
    meanV = mean * 5 / 1048576;

    ldma_done = false;
  }
}
