/***************************************************************************//**
 * @file
 * @brief Top level application functions
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

#include "em_cmu.h"
#include "em_gpio.h"
#include "em_iadc.h"
#include "em_prs.h"

#include "stdio.h"

#include "app.h"

#include "sl_simple_button.h"
#include "sl_simple_button_instances.h"

/*******************************************************************************
 ***************************   GLOBAL VARIABLES  *******************************
 ******************************************************************************/

static volatile double scanResult[NUM_INPUTS] = { 0 };
static volatile double singleResult = 0;

// if has a new ADC measurement printout
static volatile uint8_t newvalue = false;

/*******************************************************************************
 *********************   STATIC FUNCTION DEFINATION ****************************
 ******************************************************************************/

static void app_initIADC(void);
static void app_initIADCScan(void);
static void app_initIADCSingle(void);

static void app_debugSignalSetup(void);

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  printf("\rn------ IADC Tailgating conversation --------\r\n");

  // init prs signal for debugging
  app_debugSignalSetup();
  // init IADC
  app_initIADC();

  // Start scan
  IADC_command(IADC0, iadcCmdStartScan);
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
  if (newvalue) {
    // printout the latest ADC values from the register
    printf("ch0: %f\t ch1: %f\t single: %f\r\n",
           scanResult[0],
           scanResult[1],
           singleResult);
    newvalue = false;
  }
}

/**************************************************************************//**
 * @brief  IADC SCAN mode initialization
 *****************************************************************************/
static void app_initIADCScan(void)
{
  // Declare initialization structures
  IADC_InitScan_t initScan = IADC_INITSCAN_DEFAULT;

  // Scan table structure
  IADC_ScanTable_t scanTable = IADC_SCANTABLE_DEFAULT;

  /*
   * The IADC local timer triggers conversions.
   *
   * Set the SCANFIFODVL flag when scan FIFO holds 2 entries.  In this
   * example, the interrupt associated with the SCANFIFODVL flag in
   * the IADC_IF register is not used.
   *
   * Tag each FIFO entry with scan table entry ID.
   */
  initScan.triggerSelect = iadcTriggerSelTimer;
  initScan.dataValidLevel = iadcFifoCfgDvl2;
  initScan.showId = true;

  /*
   * Configure entries in the scan table.  CH0 is single-ended from
   * input 0; CH1 is single-ended from input 1.
   */
  scanTable.entries[0].posInput = IADC_INPUT_0_PORT_PIN;
  scanTable.entries[0].negInput = iadcNegInputGnd;
  scanTable.entries[0].includeInScan = true;

  scanTable.entries[1].posInput = IADC_INPUT_1_PORT_PIN;
  scanTable.entries[1].negInput = iadcNegInputGnd;
  scanTable.entries[1].includeInScan = true;

  // Initialize scan
  IADC_initScan(IADC0, &initScan, &scanTable);
}

/**************************************************************************//**
 * @brief  IADC SINGLE mode initialization
 *****************************************************************************/
static void app_initIADCSingle(void)
{
  // Declare initialization structures
  IADC_InitSingle_t initSingle = IADC_INITSINGLE_DEFAULT;

  // Single input structure
  IADC_SingleInput_t singleInput = IADC_SINGLEINPUT_DEFAULT;

  /*
   * Enable tailgating function
   */
  initSingle.singleTailgate = true;

  /*
   * Specify the input channel.  When negInput = iadcNegInputGnd, the
   * conversion is single-ended.
   */
  singleInput.posInput = IADC_INPUT_2_PORT_PIN;
  singleInput.negInput = iadcNegInputGnd;

  // Initialize a single-channel conversion
  IADC_initSingle(IADC0, &initSingle, &singleInput);
}

/**************************************************************************//**
 * @brief  IADC initialization
 *****************************************************************************/
static void app_initIADC(void)
{
  // Declare initialization structures
  IADC_Init_t init = IADC_INIT_DEFAULT;
  IADC_AllConfigs_t initAllConfigs = IADC_ALLCONFIGS_DEFAULT;

  CMU_ClockEnable(cmuClock_IADC0, true);

  // Select HFXO as the EM01GRPA clock
  CMU_ClockSelectSet(cmuClock_EM01GRPACLK, cmuSelect_HFXO);
  // Use the EM01GRPACLK as the IADC clock
  CMU_ClockSelectSet(cmuClock_IADCCLK, cmuSelect_EM01GRPACLK);

  // Shutdown between conversions to reduce current
  init.warmup = iadcWarmupNormal;

  // Set the HFSCLK prescale value here
  // 1st prescaler  settings
  init.srcClkPrescale = IADC_calcSrcClkPrescale(IADC0, CLK_SRC_ADC_FREQ, 0);

  /*
   * The IADC local timer runs at CLK_SRC_ADC_FREQ, which is at least
   * 2x CLK_ADC_FREQ. CLK_SRC_ADC_FREQ in this example is equal to the
   * HFXO frequency. Dividing the frequency of the HFXO by 1000 will give
   * the tick count for 1 ms trigger rate.
   * For example - if HFXO freq = 38.4 MHz,
   *
   * ticks for 1 ms trigger = 38400000 / 1000
   * ticks =  38400
   */
  init.timerCycles = (uint16_t)(CLK_SRC_ADC_FREQ / CLK_TIMER);

  /*
   * Configuration 0 is used by both scan and single conversions by
   * default.  Use internal bandgap as the reference and specify the
   * reference voltage in mV.
   *
   * Resolution is not configurable directly but is based on the
   * selected oversampling ratio (osrHighSpeed), which defaults to
   * 2x and generates 12-bit results.
   */
  initAllConfigs.configs[0].reference = iadcCfgReferenceInt1V2;
  initAllConfigs.configs[0].vRef = 1210;
  initAllConfigs.configs[0].osrHighSpeed = iadcCfgOsrHighSpeed2x;
  initAllConfigs.configs[0].analogGain = iadcCfgAnalogGain0P5x;

  /*
   * CLK_SRC_ADC must be prescaled by some value greater than 1 to
   * derive the intended CLK_ADC frequency.
   *
   * Based on the default 2x oversampling rate (OSRHS)...
   *
   * conversion time = ((4 * OSRHS) + 2) / fCLK_ADC
   *
   * ...which results in a maximum sampling rate of 833 ksps with the
   * 2-clock input multiplexer switching time is included.
   */
  initAllConfigs.configs[0].adcClkPrescale = IADC_calcAdcClkPrescale(IADC0,
                                                                     CLK_ADC_FREQ,
                                                                     0,
                                                                     iadcCfgModeNormal,
                                                                     init.
                                                                     srcClkPrescale);

  // Initialize IADC
  IADC_init(IADC0, &init, &initAllConfigs);

  // Initialize scan
  app_initIADCScan();

  // Initialize a single-channel conversion
  app_initIADCSingle();

  // Enable the IADC timer (must be after the IADC is initialized)
  IADC_command(IADC0, iadcCmdEnableTimer);

  // Allocate the analog bus for ADC0 inputs
#ifndef EFM32PG23B310F512IM48
  GPIO->IADC_INPUT_0_BUS |= IADC_INPUT_0_BUSALLOC;
#endif
  GPIO->IADC_INPUT_1_BUS |= IADC_INPUT_1_BUSALLOC;
  GPIO->IADC_INPUT_2_BUS |= IADC_INPUT_2_BUSALLOC;
  // Clear any previous interrupt flags
  IADC_clearInt(IADC0, _IADC_IF_MASK);

  // Enable scan interrupts
  IADC_enableInt(IADC0, IADC_IEN_SCANFIFODVL);

  // Enable single-channel done interrupts
  IADC_enableInt(IADC0, IADC_IEN_SINGLEDONE);

  // Enable ADC interrupts
  NVIC_ClearPendingIRQ(IADC_IRQn);
  NVIC_EnableIRQ(IADC_IRQn);
}

/**************************************************************************//**
 * @brief  Debug Signal initialization
 *****************************************************************************/
static void app_debugSignalSetup(void)
{
  CMU_ClockEnable(cmuClock_PRS, true);

  // route out the SCANENTRYDONE IADC event to an GPIO for debugging
  PRS_ConnectSignal(SCANENTRYDONE_PRS_CHANNEL,
                    prsTypeAsync,
                    PRS_IADC0_SCANENTRYDONE);
  PRS_PinOutput(SCANENTRYDONE_PRS_CHANNEL,
                prsTypeAsync,
                SCANENTRYDONE_PORT,
                SCANENTRYDONE_PIN);
  GPIO_PinModeSet(SCANENTRYDONE_PORT,
                  SCANENTRYDONE_PIN,
                  gpioModePushPullAlternate,
                  0);

  // route out the SINGLEDONE IADC event to an GPIO for debugging
  PRS_ConnectSignal(SINGLEDONE_PRS_CHANNEL, prsTypeAsync, PRS_IADC0_SINGLEDONE);
  PRS_PinOutput(SINGLEDONE_PRS_CHANNEL,
                prsTypeAsync,
                SINGLEDONE_PORT,
                SINGLEDONE_PIN);
  GPIO_PinModeSet(SINGLEDONE_PORT, SINGLEDONE_PIN, gpioModePushPullAlternate,
                  0);
}

/**************************************************************************//**
 * @brief  IADC interrupt handler
 *****************************************************************************/
void IADC_IRQHandler(void)
{
  IADC_Result_t sample;
  uint32_t event = IADC_getInt(IADC0);

  if (event & IADC_IF_SCANFIFODVL) {
    // While the FIFO count is non-zero...
    while (IADC_getScanFifoCnt(IADC0))
    {
      // Pull a scan result from the FIFO
      sample = IADC_pullScanFifoResult(IADC0);

      /*
       * Calculate the voltage converted as follows:
       *
       * For single-ended conversions, the result can range from 0 to
       * +Vref, i.e., for Vref = VBGR = 1.21V, and with analog gain = 0.5,
       * 0xFFF represents the full scale value of 2.42V.
       */
      scanResult[sample.id] = sample.data * 2.42 / 0xFFF;
    }
    newvalue = true;

    /*
     * Clear the scan table complete interrupt.  Reading from the FIFO
     * does not do this automatically.
     */
    IADC_clearInt(IADC0, IADC_IF_SCANFIFODVL);
  }
  if (event & IADC_IF_SINGLEDONE) {
    sample = IADC_pullSingleFifoResult(IADC0);

    /*
     * Calculate the voltage converted as follows:
     *
     * For single-ended conversions, the result can range from 0 to
     * +Vref, i.e., for Vref = VBGR = 1.21V, and with analog gain = 0.5
     * 0xFFF represents the full scale value of 2.42V.
     */
    singleResult = sample.data * 2.42 / 0xFFF;

    /*
     * Clear the single conversion complete interrupt.  Reading FIFO
     * results does not do this automatically.
     */
    IADC_clearInt(IADC0, IADC_IF_SINGLEDONE);
    newvalue = true;
  }
}

void sl_button_on_change(const sl_button_t *handle)
{
  (void)handle;

  if (sl_button_get_state(&sl_button_btn0) == SL_SIMPLE_BUTTON_PRESSED) {
    // Start Single conversion on button press
    IADC_command(IADC0, iadcCmdStartSingle);
  }
}
