
/***************************************************************************//**
 * @file
 * @brief Use the ADC to take repeated non-blocking measurements on
 * multiple inputs.
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
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
#include "em_gpio.h"

#include "sl_sleeptimer.h"
#include "app.h"
#include "printf.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/
// Sleeptimer period
#define TIMER_TIMEOUT             5000

// Set CLK_ADC to 10MHz
#define CLK_SRC_ADC_FREQ          20000000 // CLK_SRC_ADC
#define CLK_ADC_FREQ              10000000 // CLK_ADC - 10 MHz max in normal mode

// Number of scan channels
#define NUM_INPUTS                3

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
// Allocate port A and B (Pins of port A and port B can be used for measurement)
#define IADC_INPUT_0_BUS          BBUSALLOC
#define IADC_INPUT_0_BUSALLOC     GPIO_BBUSALLOC_BEVEN0_ADC0
#define IADC_INPUT_1_BUS          BBUSALLOC
#define IADC_INPUT_1_BUSALLOC     GPIO_BBUSALLOC_BODD0_ADC0
#define IADC_INPUT_2_BUS          ABUSALLOC
#define IADC_INPUT_2_BUSALLOC     GPIO_ABUSALLOC_AEVEN0_ADC0
#define IADC_INPUT_3_BUS          ABUSALLOC
#define IADC_INPUT_3_BUSALLOC     GPIO_ABUSALLOC_AODD0_ADC0

/*******************************************************************************
 ***************************   GLOBAL VARIABLES   ******************************
 ******************************************************************************/

sl_sleeptimer_timer_handle_t my_timer;  // Sleeptimer
volatile bool timer_expired = false; // Sleeptimer flag
static volatile double scanResult[NUM_INPUTS];  // Volts

// Sleeptimer callback
void my_timer_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void) handle;
  (void) data;
  timer_expired = true;
}

/**************************************************************************//**
 * @brief  IADC Initializer
 *****************************************************************************/
void initIADC(void)
{
  // Declare init structs
  IADC_Init_t init = IADC_INIT_DEFAULT;
  IADC_AllConfigs_t initAllConfigs = IADC_ALLCONFIGS_DEFAULT;
  IADC_InitScan_t initScan = IADC_INITSCAN_DEFAULT;
  IADC_ScanTable_t initScanTable = IADC_SCANTABLE_DEFAULT;  // Scan Table

  /*
   * Enable IADC0 and GPIO clock branches.
   *
   * Note: On EFR32xG21 devices, CMU_ClockEnable() calls have no effect
   * as clocks are enabled/disabled on-demand in response to peripheral
   * requests.  Deleting such lines is safe on xG21 devices and will
   * reduce provide a small reduction in code size.
   */
  CMU_ClockEnable(cmuClock_IADC0, true);
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Select clock for IADC
  CMU_ClockSelectSet(cmuClock_IADCCLK, cmuSelect_FSRCO);

  // Modify init structures and initialize
  init.warmup = iadcWarmupKeepWarm;

  // Set the HFSCLK prescale value here
  init.srcClkPrescale = IADC_calcSrcClkPrescale(IADC0, CLK_SRC_ADC_FREQ, 0);

  /*
   * Configuration 0 is used by both scan and single conversions by
   * default.  Use internal bandgap as the reference and specify the
   * reference voltage in mV.
   *
   * Resolution is not configurable directly but is based on the
   * selected oversampling ratio (osrHighSpeed), which defaults to
   * 2x and generates 12-bit results.
   */
  initAllConfigs.configs[0].reference = iadcCfgReferenceVddx;
  initAllConfigs.configs[0].vRef = 3300;
  initAllConfigs.configs[0].osrHighSpeed = iadcCfgOsrHighSpeed2x;
  initAllConfigs.configs[0].analogGain = iadcCfgAnalogGain1x;

  // Divide CLK_SRC_ADC to set the CLK_ADC frequency
  initAllConfigs.configs[0].adcClkPrescale = IADC_calcAdcClkPrescale(IADC0,
                                                                     CLK_ADC_FREQ,
                                                                     0,
                                                                     iadcCfgModeNormal,
                                                                     init.srcClkPrescale);

  /*
   * Similarly, the fifoDmaWakeup member of the initScan structure
   * is left at its default setting of false, so LDMA service is not
   * requested when the FIFO holds the specified number of samples.
   */
  initScan.dataValidLevel = _IADC_SCANFIFOCFG_DVL_VALID3;

  // Tag FIFO entry with scan table entry id.
  initScan.showId = true;

  /*
   * Configure entries in scan table.  All channels are measured
   * as single-ended.
   */
  initScanTable.entries[0].posInput = IADC_INPUT_0_PORT_PIN;
  initScanTable.entries[0].negInput = iadcNegInputGnd;
  initScanTable.entries[0].includeInScan = true;

  initScanTable.entries[1].posInput = IADC_INPUT_1_PORT_PIN;
  initScanTable.entries[1].negInput = iadcNegInputGnd;
  initScanTable.entries[1].includeInScan = true;

  initScanTable.entries[2].posInput = IADC_INPUT_2_PORT_PIN;
  initScanTable.entries[2].negInput = iadcNegInputGnd;
  initScanTable.entries[2].includeInScan = true;

  // Initialize IADC
  IADC_init(IADC0, &init, &initAllConfigs);

  // Initialize scan
  IADC_initScan(IADC0, &initScan, &initScanTable);

  // Allocate the analog bus for ADC0 inputs
  GPIO->IADC_INPUT_0_BUS |= IADC_INPUT_0_BUSALLOC;
  GPIO->IADC_INPUT_1_BUS |= IADC_INPUT_1_BUSALLOC;
  GPIO->IADC_INPUT_2_BUS |= IADC_INPUT_2_BUSALLOC;
  GPIO->IADC_INPUT_3_BUS |= IADC_INPUT_3_BUSALLOC;

  // Clear any previous interrupt flags
  IADC_clearInt(IADC0, _IADC_IF_MASK);

  // Enable Scan interrupts
  IADC_enableInt(IADC0, IADC_IEN_SCANTABLEDONE);

  // Enable ADC interrupts
  NVIC_ClearPendingIRQ(IADC_IRQn);
  NVIC_EnableIRQ(IADC_IRQn);
}

/**************************************************************************//**
 * @brief  IADC interrupt handler
 *****************************************************************************/
void IADC_IRQHandler(void)
{
  IADC_Result_t result = { 0, 0 };

  // While the FIFO count is non-zero...
  while (IADC_getScanFifoCnt(IADC0))
  {
    // Pull a scan result from the FIFO
    result = IADC_pullScanFifoResult(IADC0);

    /*
     * Calculate the voltage converted as follows:
     *
     * For single-ended conversions, the result can range from 0 to
     * +Vref, i.e., for Vref = VBGR = 3.3V, and with analog gain = 1
     * 0xFFF represents the full scale value of 3.3V.
     */
    scanResult[result.id] = result.data * 3.3 / 0xFFF;
  }

  /*
   * Clear the scan table complete interrupt.  Reading FIFO results
   * does not do this automatically.
   */
  IADC_clearInt(IADC0, IADC_IF_SCANTABLEDONE);
}

void app_init(void)
{
  // Starting the sleeptimer periodically
  sl_sleeptimer_start_periodic_timer_ms(&my_timer,
                                        TIMER_TIMEOUT,
                                        my_timer_callback,
                                        (void *)NULL,
                                        0,
                                        0);

  // IADC initialization
  initIADC();
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
  if (timer_expired == true) {
    timer_expired = false;
    IADC_command(IADC0, iadcCmdStartScan);
    // delay for the command
    for (volatile uint32_t i = 0; i < 100000; i++) {}
    printf("\r\nData1: %f \r\n", scanResult[0]);
    printf("Data2: %f \r\n", scanResult[1]);
    printf("Data3: %f \r\n", scanResult[2]);
  }
}
