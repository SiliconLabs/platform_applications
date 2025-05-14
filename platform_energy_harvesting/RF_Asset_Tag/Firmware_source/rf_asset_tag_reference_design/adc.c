/***************************************************************************//**
 * @file adc.c
 * @brief ADC driver
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

#include "adc.h"

#include <em_cmu.h>
#include <em_iadc.h>

#include "pin_config.h"

#define ADC_SCAN

static void routeABUS(GPIO_Port_TypeDef port, uint8_t pin, uint8_t bus, uint8_t what)
{
  volatile uint32_t *alloc_rd;
  volatile uint32_t *alloc_set;

  if (port == gpioPortA)
  {
    alloc_rd = &GPIO->ABUSALLOC;
    alloc_set = &GPIO->ABUSALLOC_SET;
  }
  else if (port == gpioPortB)
  {
    alloc_rd = &GPIO->BBUSALLOC;
    alloc_set = &GPIO->BBUSALLOC_SET;
  }
  else
  {
    alloc_rd = &GPIO->CDBUSALLOC;
    alloc_set = &GPIO->CDBUSALLOC_SET;
  }

  if (pin & 1)
  {
    uint32_t shift = (bus) ? _GPIO_ABUSALLOC_AODD1_SHIFT : _GPIO_ABUSALLOC_AODD0_SHIFT;
    uint32_t mask = (bus) ? _GPIO_ABUSALLOC_AODD1_MASK : _GPIO_ABUSALLOC_AODD0_MASK;
    uint32_t oddn = (*alloc_rd & mask) >> shift;

    // block if ODDn routed to other than TRISTATE or the requested peripheral
    EFM_ASSERT(oddn == _GPIO_ABUSALLOC_AODD0_TRISTATE || oddn == what);
    *alloc_set = (what << shift) & mask;
  }
  else
  {
    uint32_t shift = (bus) ? _GPIO_ABUSALLOC_AEVEN1_SHIFT : _GPIO_ABUSALLOC_AEVEN0_SHIFT;
    uint32_t mask = (bus) ? _GPIO_ABUSALLOC_AEVEN1_MASK : _GPIO_ABUSALLOC_AEVEN0_MASK;
    uint32_t evenn = (*alloc_rd & mask) >> shift;

    // block if EVEN0 routed to other than TRISTATE or the requested peripheral
    EFM_ASSERT(evenn == _GPIO_ABUSALLOC_AEVEN0_TRISTATE || evenn == what);
    *alloc_set = (what << shift) & mask;
  }
}

static void unrouteABUS(GPIO_Port_TypeDef port, uint8_t pin, uint8_t bus)
{
  volatile uint32_t *alloc_clr;
  uint32_t mask;

  if (port == gpioPortA)
  {
    alloc_clr = &GPIO->ABUSALLOC_CLR;
  }
  else if (port == gpioPortB)
  {
    alloc_clr = &GPIO->BBUSALLOC_CLR;
  }
  else
  {
    alloc_clr = &GPIO->CDBUSALLOC_CLR;
  }

  if (pin & 1)
  {
    mask = (bus) ? _GPIO_ABUSALLOC_AODD1_MASK : _GPIO_ABUSALLOC_AODD0_MASK;
  }
  else
  {
    mask = (bus) ? _GPIO_ABUSALLOC_AEVEN1_MASK : _GPIO_ABUSALLOC_AEVEN0_MASK;
  }

  *alloc_clr = mask;
}

// Set CLK_ADC to 10MHz (this corresponds to a sample rate of 77K with OSR = 32)
// CLK_SRC_ADC; largest division is by 4
#define CLK_SRC_ADC_FREQ        20000000

// CLK_ADC; IADC_SCHEDx PRESCALE has 10 valid bits
#define CLK_ADC_FREQ            10000000

// can be 1..4, do not set this to 0.5 (or fix code)
#define ADC_GAIN                2

#define VREF                    1210

void measure_voltages(sl_harvester_voltages_t *hv)
{
  CORE_DECLARE_IRQ_STATE;

  IADC_Init_t init = IADC_INIT_DEFAULT;
  IADC_AllConfigs_t initAllConfigs = IADC_ALLCONFIGS_DEFAULT;
#if defined(ADC_SCAN)
  IADC_InitScan_t initScan = IADC_INITSCAN_DEFAULT;
  IADC_ScanTable_t scanTable = IADC_SCANTABLE_DEFAULT;
#else
  IADC_InitSingle_t initSingle = IADC_INITSINGLE_DEFAULT;
  IADC_SingleInput_t input_Vopv = IADC_SINGLEINPUT_DEFAULT;
  IADC_SingleInput_t input_Vsto = IADC_SINGLEINPUT_DEFAULT;
#endif
  IADC_Result_t sample;

  init.iadcClkSuspend0 = true;
  init.iadcClkSuspend1 = true;
  init.warmup = iadcWarmupNormal;
  init.srcClkPrescale = IADC_calcSrcClkPrescale(IADC0, CLK_SRC_ADC_FREQ, 0);

  initAllConfigs.configs[0].analogGain = ADC_GAIN + 1;
  initAllConfigs.configs[0].reference = iadcCfgReferenceInt1V2;
  initAllConfigs.configs[0].vRef = VREF / ADC_GAIN;

  // Divides CLK_SRC_ADC to set the CLK_ADC frequency for desired sample rate
  initAllConfigs.configs[0].adcClkPrescale = IADC_calcAdcClkPrescale(IADC0,
                                                                    CLK_ADC_FREQ / ADC_GAIN,
                                                                    0,
                                                                    iadcCfgModeNormal,
                                                                    init.srcClkPrescale);

  initAllConfigs.configs[0].osrHighSpeed = iadcCfgOsrHighSpeed4x;
  initAllConfigs.configs[0].digAvg = iadcDigitalAverage2;

#if defined(ADC_SCAN)
  initScan.alignment = iadcAlignRight16;
  initScan.dataValidLevel = iadcFifoCfgDvl2;

  scanTable.entries[0].posInput = IADC_portPinToPosInput(IADC0_SCAN0POS_PORT, IADC0_SCAN0POS_PIN);
  scanTable.entries[0].includeInScan = true;

  scanTable.entries[1].posInput = IADC_portPinToPosInput(IADC0_SCAN1POS_PORT, IADC0_SCAN1POS_PIN);
  scanTable.entries[1].includeInScan = true;
#else
  initSingle.alignment = iadcAlignRight16;
  initSingle.dataValidLevel = iadcFifoCfgDvl1;

  input_Vopv.posInput = IADC_portPinToPosInput(IADC0_SCAN0POS_PORT, IADC0_SCAN0POS_PIN);
  input_Vsto.posInput = IADC_portPinToPosInput(IADC0_SCAN1POS_PORT, IADC0_SCAN1POS_PIN);
#endif

  CMU_ClockEnable(cmuClock_IADC0, false);
  CMU_ClockSelectSet(cmuClock_IADCCLK, cmuSelect_FSRCO);
  CMU_ClockEnable(cmuClock_IADC0, true);

  IADC_init(IADC0, &init, &initAllConfigs);

#if defined(ADC_SCAN)
  IADC_clearInt(IADC0, _IADC_IF_MASK);
  IADC_initScan(IADC0, &initScan, &scanTable);
  IADC_enableInt(IADC0, IADC_IF_SCANTABLEDONE);

  routeABUS(IADC0_SCAN0POS_PORT, IADC0_SCAN0POS_PIN, 0, _GPIO_ABUSALLOC_AODD0_ADC0);
  routeABUS(IADC0_SCAN1POS_PORT, IADC0_SCAN1POS_PIN, 0, _GPIO_ABUSALLOC_AODD0_ADC0);
  GPIO_PinModeSet(ENABLE_VOPV_PORT, ENABLE_VOPV_PIN, gpioModePushPull, 1);
  GPIO_PinModeSet(ENABLE_VSTO_PORT, ENABLE_VSTO_PIN, gpioModePushPull, 1);
  IADC_command(IADC0, iadcCmdStartScan);

  CORE_ENTER_CRITICAL();
  while ((IADC0->IF & IADC_IF_SCANTABLEDONE) == 0)
  {
    __WFE();
    CORE_YIELD_CRITICAL();
  }
  CORE_EXIT_CRITICAL();

  IADC_clearInt(IADC0, _IADC_IF_MASK);
  NVIC_ClearPendingIRQ(IADC_IRQn);
  GPIO_PinModeSet(ENABLE_VOPV_PORT, ENABLE_VOPV_PIN, gpioModeDisabled, 0);
  GPIO_PinModeSet(ENABLE_VSTO_PORT, ENABLE_VSTO_PIN, gpioModeDisabled, 0);
  unrouteABUS(IADC0_SCAN0POS_PORT, IADC0_SCAN0POS_PIN, 0);
  unrouteABUS(IADC0_SCAN1POS_PORT, IADC0_SCAN1POS_PIN, 0);

  sample = IADC_pullScanFifoResult(IADC0);
  hv->source_voltage_millivolts = (sample.data * VREF * RESISTOR_DIVIDER) / (65536 * ADC_GAIN);

  sample = IADC_pullScanFifoResult(IADC0);
  hv->storage_voltage_millivolts = (sample.data * VREF * RESISTOR_DIVIDER) / (65536 * ADC_GAIN);
#else
  routeABUS(IADC0_SCAN0POS_PORT, IADC0_SCAN0POS_PIN, 0, _GPIO_ABUSALLOC_AODD0_ADC0);
  GPIO_PinModeSet(ENABLE_VOPV_PORT, ENABLE_VOPV_PIN, gpioModePushPull, 1);
  IADC_initSingle(IADC0, &initSingle, &input_Vopv);
  IADC_clearInt(IADC0, _IADC_IF_MASK);
  IADC_enableInt(IADC0, IADC_IF_SINGLEDONE);
  IADC_command(IADC0, iadcCmdStartSingle);

  CORE_ENTER_CRITICAL();
  while ((IADC0->IF & IADC_IF_SINGLEDONE) == 0)
  {
    __WFE();
    CORE_YIELD_CRITICAL();
  }
  CORE_EXIT_CRITICAL();

  IADC_clearInt(IADC0, _IADC_IF_MASK);
  NVIC_ClearPendingIRQ(IADC_IRQn);
  GPIO_PinModeSet(ENABLE_VOPV_PORT, ENABLE_VOPV_PIN, gpioModeDisabled, 0);
  unrouteABUS(IADC0_SCAN0POS_PORT, IADC0_SCAN0POS_PIN, 0);

  sample = IADC_readSingleResult(IADC0);
  hv->source_voltage_millivolts  = (sample.data * VREF * RESISTOR_DIVIDER) / (65536 * ADC_GAIN);

  routeABUS(IADC0_SCAN1POS_PORT, IADC0_SCAN1POS_PIN, 0, _GPIO_ABUSALLOC_AODD0_ADC0);
  GPIO_PinModeSet(ENABLE_VSTO_PORT, ENABLE_VSTO_PIN, gpioModePushPull, 1);
  IADC_updateSingleInput(IADC0, &input_Vsto);
  IADC_command(IADC0, iadcCmdStartSingle);

  CORE_ENTER_CRITICAL();
  while ((IADC0->IF & IADC_IF_SINGLEDONE) == 0)
  {
    __WFE();
    CORE_YIELD_CRITICAL();
  }
  CORE_EXIT_CRITICAL();

  IADC_clearInt(IADC0, _IADC_IF_MASK);
  NVIC_ClearPendingIRQ(IADC_IRQn);
  GPIO_PinModeSet(ENABLE_VSTO_PORT, ENABLE_VSTO_PIN, gpioModeDisabled, 0);
  unrouteABUS(IADC0_SCAN1POS_PORT, IADC0_SCAN1POS_PIN, 0);

  sample = IADC_readSingleResult(IADC0);
  hv->storage_voltage_millivolts = (sample.data * VREF * RESISTOR_DIVIDER) / (65536 * ADC_GAIN);
#endif

  IADC_reset(IADC0);
  CMU_ClockEnable(cmuClock_IADC0, false);
}
