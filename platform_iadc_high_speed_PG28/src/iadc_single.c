/***************************************************************************//**
 * @file iadc_single.c
 *
 * @brief Uses the IADC to take repeated, non-blocking measurements on
 * the precision analog input   The LDMA saves results to RAM each time
 * the FIFO fills up.
 *******************************************************************************
 * # License
 * <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 *******************************************************************************
 * # Evaluation Quality
 * This code has been minimally tested to ensure that it builds and is suitable
 * as a demonstration for evaluation purposes only. This code will be maintained
 * at the sole discretion of Silicon Labs.
 ******************************************************************************/

#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_iadc.h"
#include "em_ldma.h"
#include "em_gpio.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

// How many samples to capture
#define NUM_SAMPLES               1024

// Set CLK_ADC to 10 MHz
#define CLK_SRC_ADC_FREQ          39000000 // CLK_SRC_ADC
#define CLK_ADC_FREQ              19500000 // CLK_ADC - 20 MHz max in high speed
                                           //   mode

#define IADC_INPUT_0_PORT_PIN     iadcPosInputPadAna0;
#define IADC_INPUT_1_PORT_PIN     iadcPosInputDvdd;

// GPIO output toggle to notify LDMA transfer complete
#define GPIO_OUTPUT_0_PORT        gpioPortB
#define GPIO_OUTPUT_0_PIN         1

// Use specified LDMA channel
#define IADC_LDMA_CH              0

/*******************************************************************************
 ***************************   GLOBAL VARIABLES
 ***************************     *******************************
 ******************************************************************************/

/// Globally declared LDMA link descriptor
LDMA_Descriptor_t descriptor;

// Buffer to store IADC samples
uint32_t singleBuffer[NUM_SAMPLES];

/**************************************************************************//**
 * @brief  CMU initialization
 *****************************************************************************/
void initCMU(void)
{
  CMU_ClockSelectSet(cmuClock_EM01GRPACLK, cmuSelect_HFXO);
  CMU_ClockSelectSet(cmuClock_IADCCLK, cmuSelect_EM01GRPACLK);
  CMU_ClockEnable(cmuClock_IADC0, true);
}

/**************************************************************************//**
 * @brief  GPIO initialization
 *****************************************************************************/
void initGPIO(void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Output toggled upon completing each LDMA transfer sequence
  GPIO_PinModeSet(GPIO_OUTPUT_0_PORT, GPIO_OUTPUT_0_PIN, gpioModePushPull, 0);
}

/**************************************************************************//**
 * @brief  IADC initialization
 *****************************************************************************/
void initIADC(void)
{
  // Declare initialization structures
  IADC_Init_t init = IADC_INIT_DEFAULT;
  IADC_AllConfigs_t initAllConfigs = IADC_ALLCONFIGS_DEFAULT;
  IADC_InitSingle_t initSingle = IADC_INITSINGLE_DEFAULT;
  IADC_SingleInput_t singleInput = IADC_SINGLEINPUT_DEFAULT;

  // Shutdown between conversions to reduce current
  init.warmup = iadcWarmupKeepWarm;

  // Set the HFSCLK prescale value here
  init.srcClkPrescale = IADC_calcSrcClkPrescale(IADC0, CLK_SRC_ADC_FREQ, 0);

  /*
   * Configuration 0 is used by both scan and single conversions by
   * default.  Use Vdd as the reference and specify the
   * reference voltage in mV.
   *
   * Resolution is not configurable directly but is based on the
   * selected oversampling ratio (osrHighSpeed), which defaults to
   * 2x and generates 12-bit results.
   */
  initAllConfigs.configs[0].reference = iadcCfgReferenceVddx;
  initAllConfigs.configs[0].vRef = 3300;
  initAllConfigs.configs[0].adcMode = iadcCfgModeHighSpeed;
  initAllConfigs.configs[0].osrHighSpeed = iadcCfgOsrHighSpeed2x;
  initAllConfigs.configs[0].analogGain = iadcCfgAnalogGain1x;

  /*
   * CLK_SRC_ADC must be prescaled by some value greater than 1 to
   * derive the intended CLK_ADC frequency.
   *
   * Based on the default 2x oversampling rate (OSRHS)...
   *
   * conversion time = ((4 * OSRHS) + 2) / fCLK_ADC
   *
   * ...which results in a maximum sampling rate of 1.95 Msps when using
   * the 39 MHz HFXO
   */
  initAllConfigs.configs[0].adcClkPrescale = IADC_calcAdcClkPrescale(IADC0,
                                                                     CLK_ADC_FREQ,
                                                                     0,
                                                                     iadcCfgModeHighSpeed,
                                                                     init.srcClkPrescale);

  /*
   * Trigger continuously once scan is started.  Note that
   * initSingle.start = false by default, so conversions must be started
   * manually with IADC_command(IADC0, iadcCmdStartSingle).
   *
   * Enable DMA wake-up to save the results when the specified FIFO
   * level is hit.
   */
  initSingle.triggerAction = iadcTriggerActionContinuous;
  initSingle.dataValidLevel = iadcFifoCfgDvl2;
  initSingle.fifoDmaWakeup = true;

  // Configure entry of the single input.
  singleInput.posInput = IADC_INPUT_0_PORT_PIN;
  singleInput.negInput = iadcNegInputGnd;

  // Initialize IADC
  IADC_init(IADC0, &init, &initAllConfigs);

  // Initialize Single
  IADC_initSingle(IADC0, &initSingle, &singleInput);

  /*
   * This is a workaround to manually set the prescalers.
   * EMLIB currently does not calculate the correct prescalers
   * for high speed mode.
   */
  while ((IADC0->STATUS & IADC_STATUS_SYNCBUSY) != 0U) {}
  IADC0->EN_CLR = IADC_EN_EN;
  while (IADC0->EN & _IADC_EN_DISABLING_MASK) {}
  IADC0->CTRL_CLR = 0x30000000; // Set HSCLKRATE to 0
  IADC0->CFG[0].SCHED = 1;      // set PRESSCALE to 1
  IADC0->EN_SET = IADC_EN_EN;
}

/**************************************************************************//**
 * @brief
 *   LDMA initialization
 *
 * @param[in] buffer
 *   pointer to the array where ADC results will be stored.
 * @param[in] size
 *   size of the array
 *****************************************************************************/
void initLDMA(uint32_t *buffer, uint32_t size)
{
  // Declare LDMA init structs
  LDMA_Init_t init = LDMA_INIT_DEFAULT;

  // Initialize LDMA with default configuration
  LDMA_Init(&init);

  // Trigger LDMA transfer on IADC scan completion
  LDMA_TransferCfg_t transferCfg =
    LDMA_TRANSFER_CFG_PERIPHERAL(ldmaPeripheralSignal_IADC0_IADC_SINGLE);

  /*
   * Set up a linked descriptor to save scan results to the
   * user-specified buffer.  By linking the descriptor to itself
   * (the last argument is the relative jump in terms of the number of
   * descriptors), transfers will run continuously until firmware
   * otherwise stops them.
   */
  descriptor =
    (LDMA_Descriptor_t)LDMA_DESCRIPTOR_LINKREL_P2M_WORD(
      &(IADC0->SINGLEFIFODATA),
      buffer,
      size,
      0);

  /*
   * Start the transfer.  The LDMA request and interrupt after saving
   * the specified number of IADC conversion results.
   */
  LDMA_StartTransfer(IADC_LDMA_CH, (void *)&transferCfg, (void *)&descriptor);
}

/**************************************************************************//**
 * @brief  LDMA Handler
 *****************************************************************************/
void LDMA_IRQHandler(void)
{
  // Clear interrupt flags
  LDMA_IntClear(1 << IADC_LDMA_CH);

  /*
   * Toggle GPIO to signal LDMA transfer is complete.  The low/high
   * time will be NUM_SAMPLES divided by the sampling rate, the
   * calculations for which are explained above.  For the example
   * defaults (1024 samples and a sampling rate of 833 ksps), the
   * low/high time will be around 1.23 ms, subject to FSRCO tuning
   * accuracy.
   */
  GPIO_PinOutToggle(GPIO_OUTPUT_0_PORT, GPIO_OUTPUT_0_PIN);
}

/***************************************************************************//**
 * Initialize IADC for single 20-bit high accuracy conversion.
 ******************************************************************************/
void iadc_single_init(void)
{
  // Initialize Oscillators
  initCMU();

  // Initialize the GPOI
  initGPIO();

  // Initialize the IADC
  initIADC();

  // Initialize the LDMA
  initLDMA(singleBuffer, NUM_SAMPLES);

  // Start single conversion, IADC converts continuously after
  IADC_command(IADC0, iadcCmdStartSingle);
}
