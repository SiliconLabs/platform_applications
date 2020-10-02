/**************************************************************************//**
 * @file main.c
 * @brief This project demonstrates the the IADC with peripheral-to-memory LDMA
 * data transfer using multiple descriptors and ping-pong buffers.
 * @version 0.0.1
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

#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_rtcc.h"
#include "em_iadc.h"
#include "em_ldma.h"
#include "em_gpio.h"
#include "em_prs.h"
#include "bsp.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

// How many samples to capture
#define NUM_SAMPLES               1024

// Set HFRCOEM23 to lowest frequency (1MHz)
#define HFRCOEM23_FREQ            cmuHFRCOEM23Freq_1M0Hz

// Set CLK_ADC to 1 MHz (set to max for shortest IADC conversion/power-up time)
#define CLK_SRC_ADC_FREQ          1000000 // CLK_SRC_ADC
#define CLK_ADC_FREQ              1000000 // CLK_ADC

// Set IADC timer cycles
#define TIMER_CYCLES              1000   // 1000  => 1000  samples/second

#define IADC_INPUT_0_PORT_PIN     iadcPosInputPortCPin4;

// When changing GPIO port/pins above, make sure to change xBUSALLOC macro's
// accordingly.
#define IADC_INPUT_0_BUS          CDBUSALLOC
#define IADC_INPUT_0_BUSALLOC     GPIO_CDBUSALLOC_CDEVEN0_ADC0

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

/**************************************************************************//**
 * @brief  GPIO Initializer
 *****************************************************************************/
void initGPIO (void)
{
  // Enable GPIO clock branch
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Configure LED0 as output, will toggle when LDMA transfers complete
  GPIO_PinModeSet(BSP_GPIO_LED0_PORT, BSP_GPIO_LED0_PIN, gpioModePushPull, 0);

  // Configure LED1 as output, will toggle when statistics complete
  GPIO_PinModeSet(BSP_GPIO_LED1_PORT, BSP_GPIO_LED1_PIN, gpioModePushPull, 0);
}

/**************************************************************************//**
 * @brief  IADC Initializer
 *****************************************************************************/
void initIADC (void)
{
  // Declare init structs
  IADC_Init_t init = IADC_INIT_DEFAULT;
  IADC_AllConfigs_t initAllConfigs = IADC_ALLCONFIGS_DEFAULT;
  IADC_InitSingle_t initSingle = IADC_INITSINGLE_DEFAULT;
  IADC_SingleInput_t initSingleInput = IADC_SINGLEINPUT_DEFAULT;

  // Reset IADC to reset configuration in case it has been modified
  IADC_reset(IADC0);

  // Configure IADC clock source for use while in EM2
  CMU_ClockSelectSet(cmuClock_IADCCLK, cmuSelect_HFRCOEM23); // 1MHz

  // Modify init structs and initialize
  init.warmup = iadcWarmupNormal;

  // Set the HFSCLK prescale value here
  init.srcClkPrescale = IADC_calcSrcClkPrescale(IADC0, CLK_SRC_ADC_FREQ, 0);

  // Set timer cycles to configure sampling rate
  init.timerCycles = TIMER_CYCLES;

  // Configuration 0 is used by both scan and single conversions by default
  // Use unbuffered AVDD as reference
  initAllConfigs.configs[0].reference = iadcCfgReferenceVddx;

  // Divides CLK_SRC_ADC to set the CLK_ADC frequency
  // Default oversampling (OSR) is 2x, 
  //  and Conversion Time = ((4 * OSR) + 2) / fCLK_ADC
  initAllConfigs.configs[0].adcClkPrescale = IADC_calcAdcClkPrescale(IADC0,
                                                          CLK_ADC_FREQ,
                                                          0,
                                                          iadcCfgModeNormal,
                                                          init.srcClkPrescale);

  // Single initialization
  initSingle.triggerSelect = iadcTriggerSelTimer;
  initSingle.dataValidLevel = _IADC_SINGLEFIFOCFG_DVL_VALID1;

  // Enable triggering of single conversion
  initSingle.start = true;

  // Set to run in EM2
  initSingle.fifoDmaWakeup = true;

  // Configure Input sources for single ended conversion
  initSingleInput.posInput = IADC_INPUT_0_PORT_PIN;
  initSingleInput.negInput = iadcNegInputGnd;

  // Initialize IADC
  IADC_init(IADC0, &init, &initAllConfigs);

  // Initialize Single
  IADC_initSingle(IADC0, &initSingle, &initSingleInput);

  // Allocate the analog bus for ADC0 inputs
  GPIO->IADC_INPUT_0_BUS |= IADC_INPUT_0_BUSALLOC;
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
   LDMA_DESCRIPTOR_LINKREL_P2M_BYTE(&IADC0->SINGLEFIFODATA, buffer1, size, 1);
  descLink[1] = (LDMA_Descriptor_t) \
   LDMA_DESCRIPTOR_LINKREL_P2M_BYTE(&IADC0->SINGLEFIFODATA, buffer2, size, -1);

  // Modify descriptor for NUM_SAMPLES sized transfer from iadc to buffer
  // Transfer 32 bits per unit, increment by 32 bits
  descLink[0].xfer.size = ldmaCtrlSizeWord;
  descLink[1].xfer.size = ldmaCtrlSizeWord;
  descLink[0].xfer.blockSize = ldmaCtrlBlockSizeUnit1;
  descLink[1].xfer.blockSize = ldmaCtrlBlockSizeUnit1;

  // Set descriptor to loop NUM_SAMPLES times and then complete
  descLink[0].xfer.decLoopCnt = 0; //this is the default when 
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
  LDMA_StartTransfer(0, (void*)&transferCfg, (void*)&descLink[0]);
}

/**************************************************************************//**
 * @brief  LDMA Handler
 *****************************************************************************/
void LDMA_IRQHandler(void)
{
  // Clear interrupt flags
  LDMA_IntClear(LDMA_IF_DONE0);

  if (dataBuffer == singleBuffer2)
      dataBuffer = singleBuffer1;
  else
      dataBuffer = singleBuffer2;

  // Toggle GPIO to notify that transfer is complete
  GPIO_PinOutToggle(BSP_GPIO_LED0_PORT, BSP_GPIO_LED0_PIN);
}

/**************************************************************************//**
 * @brief   Disable high frequency clocks
 *****************************************************************************/
static void disableHFClocks(void)
{
  // Make sure all high frequency peripherals are disabled
  USART0->EN_CLR = 0x1;
  USART1->EN_CLR = 0x1;
  USART2->EN_CLR = 0x1;
  TIMER0->EN_CLR = 0x1;
  TIMER1->EN_CLR = 0x1;
  TIMER2->EN_CLR = 0x1;
  TIMER3->EN_CLR = 0x1;
  ACMP0->EN_CLR = 0x1;
  ACMP1->EN_CLR = 0x1;
  I2C0->EN_CLR = 0x1;
  I2C1->EN_CLR = 0x1;
  GPCRC->EN_CLR = 0x1;

  CMU_ClockSelectSet(cmuClock_SYSCLK, cmuSelect_FSRCO);

  // Check that HFRCODPLL and HFXO are not requested
  while (((HFRCO0->STATUS & _HFRCO_STATUS_ENS_MASK) != 0U)
         || ((HFXO0->STATUS & _HFXO_STATUS_ENS_MASK) != 0U));
}

/***************************************************************************//**
 * @brief   Disable low frequency clocks
 ******************************************************************************/
static void disableLFClocks(void)
{
  // Make sure all low frequency peripherals are disabled
  RTCC->EN_CLR = 0x1;
  WDOG0->EN_CLR = 0x1;
  WDOG1->EN_CLR = 0x1;
  LETIMER0->EN_CLR = 0x1;
  BURTC->EN_CLR = 0x1;

  // Check that all low frequency oscillators are stopped
  while ((LFRCO->STATUS != 0U) && (LFXO->STATUS != 0U));
}

/***************************************************************************//**
 * @brief   Disable all clocks to achieve lowest current consumption numbers.
 ******************************************************************************/
static void disableClocks(void)
{
  // Disable High Frequency Clocks
  disableHFClocks();

  // Disable Low Frequency Clocks
  disableLFClocks();
}

/***************************************************************************//**
 * @brief
 *   Enter EM2 with RTCC running on a low frequency oscillator.
 *
 * @param[in] osc
 *   Oscillator to run RTCC from (LFXO or LFRCO).
 * @param[in] powerdownRam
 *   Power down all RAM except the first 16 kB block or retain full RAM.
 *
 * @details
 *   Parameter:
 *     EM2. Deep Sleep Mode.@n
 *   Condition:
 *     RTCC, 32.768 kHz LFXO or LFRCO.@n
 *
 * @note
 *   To better understand disabling clocks and oscillators for specific modes,
 *   see Reference Manual section EMU-Energy Management Unit and Table 9.2.
 ******************************************************************************/
void em_EM2_RTCC(CMU_Select_TypeDef osc, bool powerdownRam)
{
  // Make sure clocks are disabled.
  disableClocks();

  // Route desired oscillator to RTCC clock tree.
  CMU_ClockSelectSet(cmuClock_RTCCCLK, osc);

  // Setup RTC parameters
  RTCC_Init_TypeDef rtccInit = RTCC_INIT_DEFAULT;
  rtccInit.presc = rtccCntPresc_256;
  rtccInit.cntWrapOnCCV1 = true;

  // Initialize RTCC. Configure RTCC with prescaler 256.
  RTCC_Init(&rtccInit);

  // Power down all RAM blocks except block 1
  if (powerdownRam)
    EMU_RamPowerDown(SRAM_BASE, 0);

  // Enter EM2.
  EMU_EnterEM2(true);
}

/***************************************************************************//**
 * @brief   Computes mean and variance of data set using Welford's algorithm
 ******************************************************************************/
void statsWelford(uint32_t *buffer, uint32_t size, double *mean, double *var)
{
  int32_t cnt;
  double M, M2, delta1, delta2;

  M = 0;
  M2 = 0;
  for (cnt = 1; cnt <= size; cnt++) {
    delta1 = buffer[cnt - 1] - M;
    M += delta1 / cnt;
    delta2 = buffer[cnt -1] - M;
    M2 += delta1 * delta2;
  }
  *mean = M;
  *var = M2 / (size - 1);
}

/**************************************************************************//**
 * @brief  Main function
 *****************************************************************************/
int main(void)
{
  double mean, variance;

  // Use default settings for EM23 and HFXO
  EMU_EM23Init_TypeDef em23Init = EMU_EM23INIT_DEFAULT;
  CMU_HFXOInit_TypeDef hfxoInit = CMU_HFXOINIT_WSTK_DEFAULT;

  CHIP_Init();

  initGPIO();

  // Initialize EM2/EM3/EM4 with default parameters
  EMU_EM23Init(&em23Init);
  CMU_HFXOInit(&hfxoInit);

  // Set clock frequency to defined value
  CMU_HFRCOEM23BandSet(HFRCOEM23_FREQ);

  // Initialize the IADC
  initIADC();

  // Initialize LDMA
  initLDMA(singleBuffer1, singleBuffer2, NUM_SAMPLES);

  // IADC single already enabled; must enable timer block in order to trigger
  IADC_command(IADC0, iadcCmdEnableTimer);

  // Infinite loop
  while(1) {
    // Sleep CPU until LDMA transfer completes
    // EM2 with RTCC running off LFRCO is a documented current mode in the DS
    em_EM2_RTCC(cmuSelect_LFRCO, false);

    // Process most recent buffer
    statsWelford(dataBuffer, NUM_SAMPLES, &mean, &variance);

    // Toggle GPIO to notify that stats are complete
    GPIO_PinOutToggle(BSP_GPIO_LED1_PORT, BSP_GPIO_LED1_PIN);
  }
}
