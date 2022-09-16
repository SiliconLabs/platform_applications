/***************************************************************************//**
 * @file
 * @brief Top level application functions
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
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/
#include "em_acmp.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_device.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_lesense.h"
#include "em_letimer.h"
#include "em_pcnt.h"
#include "em_prs.h"
#include "em_vdac.h"

#include "sl_segmentlcd.h"

#include "app.h"

volatile uint32_t counter = 0;     // metal detected counter
uint32_t no_metal[16];             // store calibration value for no metal
uint32_t pcnt_top[2] = { 0, 4 };   // PCNT top value for two modes
uint32_t update_mode = 0;          // flag for operation mode update
uint32_t update_counter = 0;       // flag for metal detection counter update
uint32_t mode = 0;  // Mode 0 updates segment LCD on every metal detection
                    // Mode 1 updates segment LCD on every 5 metal detection

/***************************************************************************//**
 * LESENSE interrupt handler
 ******************************************************************************/
void LESENSE_IRQHandler(void)
{
  // Clear interrupt
  uint32_t flag = LESENSE_IntGet();
  LESENSE_IntClear(flag);

  // Read from RESFIFO
  for (int i = 0; i < 16; i++) {
    no_metal[i] = LESENSE->RESFIFO;
  }

  // Set channel threshold to no metal count - 1
  LESENSE_ChannelThresSet(0, 0, no_metal[6] - 1);

  // Disable LESENSE interrupt as it is no longer needed
  LESENSE_IntDisable(_LESENSE_IEN_MASK);
  LESENSE_ScanStart();
}

/***************************************************************************//**
 * GPIO_ODD interrupt handler
 ******************************************************************************/
void GPIO_ODD_IRQHandler()
{
  uint32_t flag = GPIO_IntGet();
  GPIO_IntClear(flag);

  // update operation mode
  update_mode = 1;
}

/***************************************************************************//**
 * PCNT interrupt handler
 ******************************************************************************/
void PCNT0_IRQHandler()
{
  uint32_t flag = PCNT_IntGet(PCNT0);
  PCNT_IntClear(PCNT0, flag);

  // update segment counter
  update_counter = 1;
}

/***************************************************************************//**
 * Initialize ACMP
 ******************************************************************************/
void initACMP(void)
{
  // Reset ACMP
  ACMP_Reset(ACMP0);

  // Configure ACMP
  ACMP_Init_TypeDef initACMP = ACMP_INIT_DEFAULT;
  initACMP.vrefDiv = 43;  // set reference input divider
                          // VREFOUT = 2.5 * (43/63) ~= 1.7V
  initACMP.accuracy = acmpAccuracyHigh;
  initACMP.biasProg = 0x7;
  initACMP.hysteresisLevel = acmpHysteresis30Sym;

  ACMP_Init(ACMP0, &initACMP);

  // Allocate BODD0 to ACMP0 to be able to use the input
  GPIO->BBUSALLOC_SET = GPIO_BBUSALLOC_BODD0_ACMP0;

  // Select 2.5V as the reference voltage for ACMP negative input
  ACMP0->INPUTCTRL_SET = ACMP_INPUTCTRL_NEGSEL_VREFDIV2V5;
  ACMP0->INPUTCTRL_SET = ACMP_INPUTCTRL_POSSEL_EXTPB;  // LESENSE port B

  // Enable ACMP
  ACMP_Enable(ACMP0);

  // Wait for warm-up
  while (!(ACMP0->STATUS && ACMP_IF_ACMPRDY)) {}
}

/***************************************************************************//**
 * Initialize GPIO
 ******************************************************************************/
void initGPIO(void)
{
  // Enable routing for LESENSE channel 0
  // Configure PB3 to be LESENSE channel 0 pin
  GPIO->LESENSEROUTE_SET.ROUTEEN |= GPIO_LESENSE_ROUTEEN_CH0OUTPEN;
  GPIO->LESENSEROUTE_SET.CH0OUTROUTE = (gpioPortB
                                        << _GPIO_LESENSE_CH0OUTROUTE_PORT_SHIFT)
                                       | (3
                                          << _GPIO_LESENSE_CH0OUTROUTE_PIN_SHIFT);

  // LESENSE pin PB3 set to push pull alternate for excitation
  GPIO_PinModeSet(gpioPortB, 3, gpioModePushPullAlternate, 0);

  // Disable port B alternate input to achieve lowest current consumption
  // Need to use alternate functionality for port B since the push button also
  // uses port B and its input should not be disabled.
  GPIO->P_SET[gpioPortB].CTRL = GPIO_P_CTRL_DINDISALT;

  // PB1 - push button 0 configured as input mode with filter enabled
  // Enable interrupt on PB1
  GPIO_PinModeSet(gpioPortB, 1, gpioModeInput, 1);
  GPIO_ExtIntConfig(gpioPortB, 1, 1, false, true, true);
  NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
  NVIC_EnableIRQ(GPIO_ODD_IRQn);
}

/***************************************************************************//**
 * Initialize VDAC
 ******************************************************************************/
void initVDAC(void)
{
  VDAC_Init_TypeDef initVdac = VDAC_INIT_DEFAULT;
  VDAC_InitChannel_TypeDef initChannel = VDAC_INITCHANNEL_DEFAULT;

  initChannel.trigMode = vdacTrigModeLesense;     // Triggered by LESENSE
  initChannel.sampleOffMode = true;               // Sample off mode
  initChannel.powerMode = vdacPowerModeLowPower;  // Low power mode
  initChannel.holdOutTime = 10;

  initVdac.reference = vdacRef2V5;               // Internal 2.5V reference
  initVdac.onDemandClk = false;
  initVdac.prescaler = VDAC_PrescaleCalc(VDAC0, 500000);

  // Configure VDAC
  VDAC_Init(VDAC0, &initVdac);
  VDAC_InitChannel(VDAC0, &initChannel, 0);

  // Enable VDAC
  VDAC_Enable(VDAC0, 0, true);

  // Set channel 0 output, 3000 / 4095 * 2.5 ~= 1.83V
  VDAC_Channel0OutputSet(VDAC0, 3000);
}

/***************************************************************************//**
 * Initialize PRS
 ******************************************************************************/
void initPrs(void)
{
  // PRS channel 0 connect to LESENSE decoder output 0
  PRS_SourceAsyncSignalSet(0, PRS_ASYNC_CH_CTRL_SOURCESEL_LESENSE,
                           prsSignalLESENSE_DECOUT0);

  // PRS channel 0 connect to LESENSE decoder output 1
  PRS_SourceAsyncSignalSet(1, PRS_ASYNC_CH_CTRL_SOURCESEL_LESENSE,
                           prsSignalLESENSE_DECOUT1);
}

/***************************************************************************//**
 * Initialize LESENSE
 ******************************************************************************/
void initLesense(void)
{
  LESENSE_Init_TypeDef initLESENSE = LESENSE_INIT_DEFAULT;
  LESENSE_DecCtrlDesc_TypeDef initDecode = LESENSE_DECCTRL_DESC_DEFAULT;

  initDecode.prsCount = true;  // Enable count mode on PRS 0 and 1
  initDecode.hystPRS0 = false; // Do not enable hysteresis in decoder for PRS0
  initDecode.hystPRS1 = false; // Do not enable hysteresis in decoder for PRS1
  initDecode.intMap = false;   // Do not set interrupt upon state transition

  // LESENSE only controls ACMP mux
  initLESENSE.perCtrl.acmp0Mode = lesenseACMPModeMux;

  initLESENSE.decCtrl = initDecode;
  initLESENSE.coreCtrl.scanStart = lesenseScanStartPeriodic;  // periodic scan

  // Reset and configure LESENSE module
  LESENSE_Init(&initLESENSE, true);

  /****************************************************************************
   * @description: Configure LESENSE decoder states
   * Since only one sensor is scanned and the only purpose of the scan is to
   * detect metal presence, only two LESENSE state(with/without metal) need to
   * be used. The next state will either be the current state if metal presence
   * does not change, or switch to the other state if the metal presence changes
   * Note that the PRS signal is only generated upon the following conditions:
   * 1. Current state = Decode state
   * 2. Sensor state = COMP
   ***************************************************************************/
  LESENSE_DecStAll_TypeDef initStAll = LESENSE_DECODER_CONF_DEFAULT;

  initStAll.St[0].curState = 0;     // Make 0 as the defined state (no metal)
  initStAll.St[0].nextState = 1;     // Set next state to with metal
  initStAll.St[0].compMask = 0xE;   // Only enable channel 0 as decoder input
  initStAll.St[0].compVal = 0x01;  // compare match on channel 0 sensorstate
  initStAll.St[0].prsAct = lesenseTransActUp;
  initStAll.St[0].setInt = true;

  initStAll.St[1].curState = 0;     // Make 0 as the defined state (no metal)
  initStAll.St[1].nextState = 0;     // Set next state to no metal
  initStAll.St[1].compMask = 0xE;
  initStAll.St[1].compVal = 0x00;

  initStAll.St[2].curState = 1;     // Make 1 as the defined state (with metal)
  initStAll.St[2].nextState = 0;     // Set next state to no metal
  initStAll.St[2].compMask = 0xE;
  initStAll.St[2].compVal = 0x00;

  initStAll.St[3].curState = 1;     // Make 1 as the defined state (with metal)
  initStAll.St[3].nextState = 1;     // Set next state to with metal
  initStAll.St[3].compMask = 0xE;
  initStAll.St[3].compVal = 0x01;

  LESENSE_DecoderStateAllConfig(&initStAll); // Initialize decoder

  LESENSE_ChAll_TypeDef confChAll = LESENSE_SCAN_CONF_DEFAULT;
  LESENSE_ChDesc_TypeDef confCh = LESENSE_LCSENSE_SENSOR_CH_CONF;
  confChAll.Ch[0] = confCh;
  LESENSE_ChannelAllConfig(&confChAll);  // Configure LESENSE channel 0

  // Wait for SYNCBUSY clear before configuring
  while (LESENSE->SYNCBUSY) {}

  // Disable LESENSE module
  LESENSE->EN_CLR = LESENSE_EN_EN;
  while (LESENSE->EN & _LESENSE_EN_DISABLING_MASK) {
    /* Wait for disabling to finish */
  }
  LESENSE->TIMCTRL_SET = (LESENSE_TIMCTRL_PCPRESC_DIV32
                          | (32 << _LESENSE_TIMCTRL_PCTOP_SHIFT));  // about 8HZ

  // LESENSE offset 3 = ACMP PB + 3 = ACMP input PB3
  LESENSE->CH_SET[0].INTERACT = (3 << _LESENSE_CH_INTERACT_OFFSET_SHIFT);
  LESENSE->EN_SET = LESENSE_EN_EN;  // Enable LESENSE
  while (LESENSE->SYNCBUSY) {}         // SYNCBUSY check

  // Enable LESENSE interrupt to configure the compare threshold
  LESENSE_IntEnable(LESENSE_IEN_RESOF);
  NVIC_ClearPendingIRQ(LESENSE_IRQn);
  NVIC_EnableIRQ(LESENSE_IRQn);

  // LESENSE scan start
  LESENSE_ScanStart();
}

/***************************************************************************//**
 * Initialize PCNT
 ******************************************************************************/
void initPCNT(void)
{
  PCNT_Init_TypeDef initPcnt = PCNT_INIT_DEFAULT;
  initPcnt.mode = pcntModeOvsSingle;     // oversampling single mode
  initPcnt.counter = 0;                  // default value = 0
  initPcnt.top = 0;                      // overflow on every count
  initPcnt.countDown = false;            // direction controled by S1
  initPcnt.filter = false;               // disable filter
  initPcnt.cntEvent = pcntCntEventBoth;  // count both events
  initPcnt.s1CntDir = true;              // S1 input controls direction
  initPcnt.debugHalt = false;            // enable debug run
  initPcnt.s0PRS = 0;                    // S0 input from PRS0
  initPcnt.s1PRS = 1;                    // S1 input from PRS1

  PCNT_Init(PCNT0, &initPcnt);           // Init PCNT

  PCNT_PRSInputEnable(PCNT0, pcntPRSInputS0, true);  // Enable PRS0 input
  PCNT_PRSInputEnable(PCNT0, pcntPRSInputS1, true);  // Enable PRS1 input
  PCNT_Enable(PCNT0, pcntModeOvsSingle);             // Enable PCNT

  PCNT_IntClear(PCNT0, _PCNT_IEN_MASK);  // Clear all PCNT interrupt enable
  PCNT_IntEnable(PCNT0, PCNT_IEN_OF);    // Enable PCNT overflow interrupt
  NVIC_ClearPendingIRQ(PCNT0_IRQn);
  NVIC_EnableIRQ(PCNT0_IRQn);
}

/***************************************************************************//**
 * Initialize CMU
 ******************************************************************************/
void initCMU(void)
{
  // Enable clock for ACMP0
  CMU_ClockEnable(cmuClock_ACMP0, true);

  // Enable clock for GPIO
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Enable clock for PRS
  CMU_ClockEnable(cmuClock_PRS, true);

  // Set LFRCO as EFM32GRPACLK clock source
  // Enable clock for LESENSE
  // Set LESENSE High Frequency Clock to FSRCO for EM2 operation
  CMU_ClockSelectSet(cmuClock_EM23GRPACLK, cmuSelect_LFRCO);
  CMU_ClockEnable(cmuClock_LESENSE, true);
  CMU_ClockSelectSet(cmuClock_LESENSEHFCLK, cmuSelect_HFRCOEM23);
  CMU_ClockEnable(cmuClock_HFRCOEM23, true);
  HFRCOEM23->CTRL_SET = HFRCO_CTRL_EM23ONDEMAND; 

  // Enable clock for VDAC
  CMU_ClockSelectSet(cmuClock_VDAC0, cmuSelect_HFRCOEM23);
  CMU_ClockEnable(cmuClock_VDAC0, true);

  // Enable clock for PCNT
  CMU_ClockEnable(cmuClock_PCNT0, true);
}

/***************************************************************************//**
* Disable Unused LCD Segments
*******************************************************************************/
void disableUnusedLCDSeg(void)
{
/***************************************************************************//**
* The LCD driver enables all segments, even those that are not mapped to
* segments on the dev kit board. These are disabled below in order to
* minimize current consumption.
*******************************************************************************/
  LCD_SegmentEnable(2, false);
  LCD_SegmentEnable(3, false);
  LCD_SegmentEnable(9, false);
  LCD_SegmentEnable(11, false);
  LCD_SegmentEnable(12, false);
  LCD_SegmentEnable(13, false);
  LCD_SegmentEnable(14, false);
  LCD_SegmentEnable(15, false);
  LCD_SegmentEnable(16, false);
  LCD_SegmentEnable(17, false);
}

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  // CMU Initialization
  initCMU();

  // GPIO Initialization
  initGPIO();

  // VDAC Initialization
  initVDAC();

  // ACMP Initialization
  initACMP();

  // PRS Initialization
  initPrs();

  // PCNT Initialization
  initPCNT();

  // LESENSE Initialization
  initLesense();

  // Segment LCD Initialization
  // Default display 0
  SegmentLCD_Init(false);
  LCD->BIASCTRL_SET = LCD_BIASCTRL_VDDXSEL_AVDD;
  disableUnusedLCDSeg();
  SegmentLCD_Number(counter);
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
  // If mode update flag set
  if (update_mode) {
    counter = 0;                        // clear counnter
    mode = (mode + 1) % 2;              // update mode
    update_mode = 0;                    // clear mode update flag
    SegmentLCD_Number(counter);         // update segment LCD
    PCNT_CounterReset(PCNT0);           // reset PCNT counter
    PCNT_TopSet(PCNT0, pcnt_top[mode]); // update PCNT top value
  }
  // If update counter flag set
  if (update_counter) {
    update_counter = 0;            // clear flag
    SegmentLCD_Number(++counter);  // update counter
  }
}
