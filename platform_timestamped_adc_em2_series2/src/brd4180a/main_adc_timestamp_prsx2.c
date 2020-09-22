/***************************************************************************//**
 * @file main_adc_timestamp_prsx2.c
 * @version 1.0.0
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/

// CMSIS
#include "em_device.h"

// emlib
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_iadc.h"
#include "em_ldma.h"
#include "em_prs.h"
#include "em_rtcc.h"

// (W)STK BSP
#include "bsp.h"

/*
 * Use EXP header pin 12 (PA5) for the trigger input.
 *
 * Note that not all pins on a given port are available as PRS
 * producers on that port.  For example, while PA5 can be the GPIO pin
 * 5 producer on port A, pin 6 cannot (port C or D would have to be
 * used instead).
 */
#define TRIG_PORT     gpioPortA
#define TRIG_PIN      5

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
#define IADC_INPUT      iadcPosInputPortBPin0   // EXP header pin 11
#define ALLOCATE_ABUS   (GPIO->BBUSALLOC |= GPIO_BBUSALLOC_BEVEN0_ADC0)

/*
 * The HFRCOEM23 is the oscillator that is enabled by the IADC when a
 * trigger wakes it from EM2.  The frequency of the HFRCOEM23
 * determines the IADC clock and, consequently, both the IADC current
 * draw and the conversion timing.  Use the lowest defined calibrated
 * frequency, 1 MHz, to minimize current draw.
 */
#define HFRCOEM23_BAND  cmuHFRCOEM23Freq_1M0Hz

// Number of samples/timestamps to capture
#define NUM_SAMPLES   256

// Sample and timestamp buffers
uint32_t valbuf[NUM_SAMPLES];
uint32_t timebuf[NUM_SAMPLES];

/*
 * PRS and LDMA channel assignments.
 *
 * This code works on the premise that the timestamp transfer can be
 * processed before the conversion result so long as the read of the
 * RTCC_CNT register is assigned to a higher priority LDMA channel.
 *
 * To make this relationship clear, channel 0 of both the PRS and the
 * LDMA is used for the RTCC transfer because lower-numbered LDMA
 * channels have higher priority if more than one channel needs to be
 * serviced at the same time.
 */
#define LDMA_RTCC_CH  0
#define PRS_RTCC_CH   0
#define LDMA_IADC_CH  1
#define PRS_IADC_CH   1

// Global LDMA structures
LDMA_TransferCfg_t iadcXferCfg;
LDMA_Descriptor_t  iadcXferDesc;
LDMA_TransferCfg_t rtccXferCfg;
LDMA_Descriptor_t  rtccXferDesc;

/*
 * These are the frequencies of the two IADC clocks related to
 * conversion timing.
 *
 * CLK_SRC_ADC is derived from the incoming IADC module clock
 * (CLK_CMU_ADC) and is prescaled by the value specified in
 * IADC_CTRL_HSCLKRATE.  This is the clock used for warm-up and
 * start-up timing and is the clock used by the IADC's own timer.
 *
 * ADC_CLK is derived from CLK_SRC_ADC and is further scaled by the
 * IADC_SCHEDx_PRESCALE register bit field.  This is the clock that
 * times the ADC conversion cycle.
 *
 * CLK_SRC_ADC and ADC_CLK can only be the same frequency when
 * IADC_CTRL_HSCLKRATE = 0.  In this example, CLK_CMU_ADC is 1 MHz
 * (because it is sourced from the HFRCOEM23 which has been switched
 * to the 1 MHz frequency band), so both CLK_SRC_ADC and ADC_CLK can
 * also be 1 MHz.
 */
#define CLK_SRC_ADC_FREQ    1000000
#define ADC_CLK_FREQ        1000000

// Configure the conversion trigger input
void triggerInit(void)
{
  GPIO_PinModeSet(TRIG_PORT, TRIG_PIN, gpioModeInput, 0);

  GPIO_ExtIntConfig(TRIG_PORT, TRIG_PIN, TRIG_PIN, true, false, true);

  // Assign the trigger input pin to the PRS channel selected for the RTCC
  PRS_SourceAsyncSignalSet(PRS_RTCC_CH, PRS_ASYNC_CH_CTRL_SOURCESEL_GPIO, TRIG_PIN);
  PRS_ConnectConsumer(PRS_RTCC_CH, prsTypeAsync, prsConsumerLDMA_REQUEST0);

  // Do the same but for the IADC
  PRS_SourceAsyncSignalSet(PRS_IADC_CH, PRS_ASYNC_CH_CTRL_SOURCESEL_GPIO, TRIG_PIN);
  PRS_ConnectConsumer(PRS_IADC_CH, prsTypeAsync, prsConsumerIADC0_SINGLETRIGGER);
}

/*
 * Debug escape hatch mechanism.  Check for button 0 pressed out of
 * reset.  If not, disable PB0 pin to minimize energy use.
 */
bool haltOnButton0(void)
{
  // Configure button 0 pin as an input
  GPIO_PinModeSet(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, gpioModeInputPullFilter, 1);

  // Is button 0 pressed? If so, turn on LED0 and return true
  if (GPIO_PinInGet(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN) == 0)
  {
    GPIO_PinModeSet(BSP_GPIO_LED0_PORT, BSP_GPIO_LED0_PIN, gpioModePushPull, 1);

    return true;
  }
  // Otherwise, disable button 0 pin and return false
  else
  {
    GPIO_PinModeSet(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, gpioModeDisabled, 0);

    return false;
  }
}

// Setup the RTCC for use as the IADC timestamp
void rtccInit()
{
  // Enable the LFRCO and select as the RTCC counter clock
  CMU_ClockSelectSet(cmuClock_RTCCCLK, cmuSelect_LFRCO);

  /*
   * Set count prescaler to 1 such that each tick is 1/32768 for
   * highest resolution timestamp.
   */
  RTCC_Init_TypeDef init = RTCC_INIT_DEFAULT;
  init.presc = rtccCntPresc_1;
  RTCC_Init(&init);
}

void ldmaInit()
{
  LDMA_Init_t init = LDMA_INIT_DEFAULT;

  // Setup the RTCC transfer configuration and descriptor
  LDMA_TransferCfg_t rtccXferCfg = (LDMA_TransferCfg_t)LDMA_TRANSFER_CFG_PERIPHERAL(ldmaPeripheralSignal_LDMAXBAR_PRSREQ0);

  rtccXferDesc = (LDMA_Descriptor_t)LDMA_DESCRIPTOR_LINKREL_P2M_WORD(
      &(RTCC->CNT),               // source
      timebuf,                    // destination
      NUM_SAMPLES,                // data transfer size
      0);                       // link relative offset (links to self)

  // Setup the IADC transfer configuration and descriptor
  LDMA_TransferCfg_t iadcXferCfg = (LDMA_TransferCfg_t)LDMA_TRANSFER_CFG_PERIPHERAL(ldmaPeripheralSignal_IADC0_IADC_SINGLE);

  iadcXferDesc = (LDMA_Descriptor_t)LDMA_DESCRIPTOR_LINKREL_P2M_WORD(
      &(IADC0->SINGLEFIFODATA),   // source
      valbuf,                     // destination
      NUM_SAMPLES,                // data transfer size
      0);                         // link relative offset (links to self)

  // Initialize LDMA with default configuration
  LDMA_Init(&init);

  // Arm the LDMA to transfer on a single ADC conversion complete
  LDMA_StartTransfer(LDMA_RTCC_CH, &rtccXferCfg, &rtccXferDesc);
  LDMA_StartTransfer(LDMA_IADC_CH, &iadcXferCfg, &iadcXferDesc);
}

void LDMA_IRQHandler(void)
{
  /*
   * Clear interrupt flags, stop the IADC, and halt in order to view
   * the results in the debugger.
   */
  LDMA_IntClear(1 << LDMA_IADC_CH);
  IADC_command(IADC0, iadcCmdStopSingle);
__BKPT(0);
}

void iadcInit(void)
{
  // Use the HFRCOEM23 as the ADC clock
  CMU_ClockSelectSet(cmuClock_IADCCLK, cmuSelect_HFRCOEM23);

  // Modify IADC initialization structure
  IADC_Init_t init = IADC_INIT_DEFAULT;

  init.warmup = iadcWarmupNormal;

  // Set the prescaler based on the incoming IADC module clock
  init.srcClkPrescale = IADC_calcSrcClkPrescale(IADC0, CLK_SRC_ADC_FREQ, 0);

  // Modify configuration 0
  IADC_AllConfigs_t allConfigs = IADC_ALLCONFIGS_DEFAULT;

  allConfigs.configs[0].reference = iadcCfgReferenceVddx;

  /*
   * Determine the prescale value for CLK_SRC_ADC to set the ADC_CLK
   * frequency.  This will be 1 since the HFRCOEM23 provides the IADC
   * module clock (CLK_CMU_ADC), and it has been switched to operate
   * in the 1 MHz tuning band.
   *
   * In this example, at the default oversampling rate (OSR) of 2x....
   *
   * Conversion Time = ((4 * OSR) + 2) / fCLK_ADC = ((4 * 2) + 2) / 1 MHz = 10 µs
   */
  allConfigs.configs[0].adcClkPrescale = IADC_calcAdcClkPrescale(IADC0,
                                                                 ADC_CLK_FREQ,
                                                                 0,
                                                                 iadcCfgModeNormal,
                                                                 init.srcClkPrescale);

  // Modify the default single conversion settings
  IADC_InitSingle_t single = IADC_INITSINGLE_DEFAULT;

  single.triggerSelect = iadcTriggerSelPrs0PosEdge;
  single.triggerAction = iadcTriggerActionOnce;
  single.dataValidLevel = _IADC_SINGLEFIFOCFG_DVL_VALID1;   // Request LDMA service when one entry is in the FIFO
  single.start = true;
  single.fifoDmaWakeup = true;    // Run in EM2

  // Modify the default single-ended input configuration
  IADC_SingleInput_t seInput= IADC_SINGLEINPUT_DEFAULT;

  seInput.posInput = IADC_INPUT;
  seInput.negInput = iadcNegInputGnd;

  // Initialize IADC
  IADC_init(IADC0, &init, &allConfigs);

  // Allocate the corresponding ABUS to the IADC
  ALLOCATE_ABUS;

  /*
   * Initialize a single channel conversion.  Note that no conversion
   * can happen until the trigger input pin is initialized and routed
   * to the IADC via the PRS.
   */
  IADC_initSingle(IADC0, &single, &seInput);
}

int main(void)
{
  CHIP_Init();

  // Halt on button 0 pressed for debug access
  if (haltOnButton0() == true)
    __BKPT(0);

  // Initialize EM2/EM3 with default parameters
  EMU_EM23Init_TypeDef em23Init = EMU_EM23INIT_DEFAULT;
  EMU_EM23Init(&em23Init);

  // Use the specified HFRCOEM23 tuning band
  CMU_HFRCOEM23BandSet(HFRCOEM23_BAND);

  // Start the RTCC for timestamping
  rtccInit();

  iadcInit();

  ldmaInit();

  triggerInit();

  // Infinite loop
  while(1)
    EMU_EnterEM2(true);
}
