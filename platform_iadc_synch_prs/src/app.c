/***************************************************************************//**
 * @file
 * @brief LDMA examples function
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
#include "em_prs.h"
#include "em_iadc.h"
#include "em_gpio.h"
#include "em_ldma.h"
#include "dmadrv.h"

#include "sl_emlib_gpio_simple_init.h"
#include "sl_emlib_gpio_init_timer_config.h"

/*******************************************************************************
 *******************************   Local variables   ***************************
 ******************************************************************************/
// Globally declared LDMA link descriptor
LDMA_Descriptor_t descriptor;

// Buffer for IADC samples
uint32_t singleBuffer[NUM_SAMPLES];

static unsigned int iadc_channel;

// Callback triggered when DMA transfer on reception channel is complete
static bool dma_transfer_finished_cb(unsigned int channel,
                                     unsigned int sequence_no,
                                     void *user_param)
{
  (void)channel;
  (void)sequence_no;
  (void)&user_param;

  // Toggle LED1 to notify that transfers are complete
  GPIO_PinOutToggle(LDMA_GPIO_LED1_PORT, LDMA_GPIO_LED1_PIN);

  // return value is not used for simple (non ping-pong) transfers
  return true;
}

/**************************************************************************//**
 * @brief CMU initialization
 *****************************************************************************/
static void initCMU(void)
{
  // Enable PRS clock
  CMU_ClockEnable(cmuClock_PRS, true);

  // Enable GPIO clock
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Enable IADC clock
  CMU_ClockEnable(cmuClock_IADC0, true);

  // Enable TIMER0 clock
  CMU_ClockEnable(cmuClock_TIMER0, true);
}

/**************************************************************************//**
 * @brief   Timer initialization
 *****************************************************************************/
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

  // Configure the output to create PRS pulses
  timer_CCInit.prsOutput = timerPrsOutputPulse;

  TIMER_Init(TIMER0, &init);

#if TIMER_DEBUG
  // Route CC0 output to LED0
  GPIO->TIMERROUTE[0].ROUTEEN = GPIO_TIMER_ROUTEEN_CC0PEN;
  GPIO->TIMERROUTE[0].CC0ROUTE =
    (SL_EMLIB_GPIO_INIT_TIMER_PORT << _GPIO_TIMER_CC0ROUTE_PORT_SHIFT)
    | (SL_EMLIB_GPIO_INIT_TIMER_PIN << _GPIO_TIMER_CC0ROUTE_PIN_SHIFT);
#endif

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

/**************************************************************************//**
 * @brief  IADC Initialization
 *****************************************************************************/
static void initIADC(void)
{
  // Declare initialization structures
  IADC_Init_t init = IADC_INIT_DEFAULT;
  IADC_AllConfigs_t initAllConfigs = IADC_ALLCONFIGS_DEFAULT;
  IADC_InitSingle_t initSingle = IADC_INITSINGLE_DEFAULT;

  // Single input structure
  IADC_SingleInput_t singleInput = IADC_SINGLEINPUT_DEFAULT;

  // Select clock for the IADC
  CMU_ClockSelectSet(cmuClock_IADCCLK, cmuSelect_EM01GRPACLK);

  // Set the HFSCLK prescale value here
  init.srcClkPrescale = IADC_calcSrcClkPrescale(IADC0, CLK_SRC_ADC_FREQ, 0);

  init.warmup = iadcWarmupNormal;
  init.iadcClkSuspend1 = true;

  /*
   * Configuration 0 is used by both scan and single conversions by default.
   * Resolution is not configurable directly but is based on the
   * selected oversampling ratio (osrHighSpeed), which defaults to
   * 2x and generates 12-bit results.
   */
  initAllConfigs.configs[0].reference = iadcCfgReferenceVddx;
  initAllConfigs.configs[0].vRef = IADC_VREF;
  initAllConfigs.configs[0].osrHighSpeed = iadcCfgOsrHighSpeed2x;
  initAllConfigs.configs[0].analogGain = iadcCfgAnalogGain1x;

  // Divide CLK_SRC_ADC to set the CLK_ADC frequency
  initAllConfigs.configs[0].adcClkPrescale = IADC_calcAdcClkPrescale(IADC0,
                                                                     CLK_ADC_FREQ,
                                                                     0,
                                                                     iadcCfgModeNormal,
                                                                     init.srcClkPrescale);

#if SYNC_MODE
  initSingle.triggerSelect = iadcTriggerSelPrs0SameClk;
#else
  initSingle.triggerSelect = iadcTriggerSelPrs0PosEdge;
#endif
  initSingle.dataValidLevel = iadcFifoCfgDvl2;
  initSingle.fifoDmaWakeup = true;
  initSingle.start = true;

  // Configure the IADC input pin
  singleInput.posInput = IADC_INPUT_0_PORT_PIN;
  singleInput.negInput = iadcNegInputGnd;

  // Allocate the analog bus for ADC0 inputs
  GPIO->IADC_INPUT_0_BUS |= IADC_INPUT_0_BUSALLOC;

  // Initialize IADC
  IADC_init(IADC0, &init, &initAllConfigs);

  // Initialize single conversion
  IADC_initSingle(IADC0, &initSingle, &singleInput);
}

/**************************************************************************//**
 * @brief  GPIO initialization
 *****************************************************************************/
static void initGPIO(void)
{
  // Show sample completion status on LED0
  GPIO_PinModeSet(LDMA_GPIO_LED1_PORT, LDMA_GPIO_LED1_PIN, gpioModePushPull, 0);
}

/**************************************************************************//**
 * @brief PRS initialization
 *****************************************************************************/
static void initPRS(void)
{
  // Set up PRS to connect the TIMER0 CC0 to IADC single trigger
#if SYNC_MODE
  PRS_SourceSignalSet(PRS_CHANNEL,
                      PRS_SYNC_CH_CTRL_SOURCESEL_TIMER0,
                      PRS_SYNC_TIMER0_CC0,
                      prsEdgePos);
  PRS_ConnectConsumer(PRS_CHANNEL, prsTypeSync,
                      prsConsumerIADC0_SINGLETRIGGER);
#else
  PRS_SourceAsyncSignalSet(PRS_CHANNEL,
                           PRS_ASYNC_CH_CTRL_SOURCESEL_TIMER0,
                           PRS_TIMER0_CC0);
  PRS_ConnectConsumer(PRS_CHANNEL,
                      prsTypeAsync,
                      prsConsumerIADC0_SINGLETRIGGER);
#endif
}

/**************************************************************************//**
 * @brief  LDMA initialization
 *
 * @param[in] buffer - Pointer to the array where ADC results will be stored.
 * @param[in] size - Size of the array
 *****************************************************************************/
static void initLDMA(uint32_t *buffer, uint32_t size)
{
  sl_status_t status;

  // Initialize DMA with default parameters
  DMADRV_Init();

  // Allocate DMA channel
  status = DMADRV_AllocateChannel(&iadc_channel, NULL);
  EFM_ASSERT(status == ECODE_EMDRV_DMADRV_OK);

  // Trigger LDMA transfer on IADC scan completion
  LDMA_TransferCfg_t transferCfg = LDMA_TRANSFER_CFG_PERIPHERAL(
    ldmaPeripheralSignal_IADC0_IADC_SINGLE);

  /*
   * Set up a linked descriptor to save scan results to the
   * user-specified buffer. By linking the descriptor to itself
   * (the last argument is the relative jump in terms of the number of
   * descriptors), transfers will run continuously.
   */
  descriptor = (LDMA_Descriptor_t)LDMA_DESCRIPTOR_LINKREL_P2M_WORD(
    &(IADC0->SINGLEFIFODATA),
    buffer,
    size,
    0);

  DMADRV_LdmaStartTransfer(iadc_channel,
                           &transferCfg,
                           &descriptor,
                           dma_transfer_finished_cb,
                           NULL);
}

void app_init(void)
{
  // Initialize clocks
  initCMU();

  // Initialize GPIO
  initGPIO();

  // Initialize PRS
  initPRS();

  // Initialize IADC
  initIADC();

  // Initialize LDMA
  initLDMA(singleBuffer, NUM_SAMPLES);

  // Initialize TIMER
  initTIMER();
}

void app_process_action(void)
{
  // Enter EM1 sleep
  EMU_EnterEM1();
}
