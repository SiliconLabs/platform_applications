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
#include "em_vdac.h"
#include "em_timer.h"
#include "em_ldma.h"
#include "em_prs.h"
#include "em_gpio.h"

#include "sl_emlib_gpio_simple_init.h"
#include "sl_emlib_gpio_init_timer_config.h"

/*******************************************************************************
 *******************************   Local variables   ***************************
 ******************************************************************************/

// 32 point sine table
static const uint16_t sineTable[SINE_TABLE_SIZE] = {
  2048, 2447, 2831, 3185, 3495, 3750, 3939, 4056,
  4095, 4056, 3939, 3750, 3495, 3185, 2831, 2447,
  2048, 1648, 1264, 910, 600, 345, 156, 39,
  0, 39, 156, 345, 600, 910, 1264, 1648,
};

/**************************************************************************//**
 * @brief CMU initialization
 *****************************************************************************/
static void initCMU(void)
{
  // Enable peripheral clocks
#if (PRS_MODE)
  CMU_ClockEnable(cmuClock_PRS, true);
#endif

  CMU_ClockEnable(cmuClock_TIMER0, true);

  // The EM01GRPACLK is chosen as VDAC clock source since the VDAC will be
  // operating in EM1
  CMU_ClockSelectSet(cmuClock_VDAC0, cmuSelect_EM01GRPACLK);
  // Enable the VDAC clocks
  CMU_ClockEnable(cmuClock_VDAC0, true);
}

/**************************************************************************//**
 * @brief
 *    VDAC initialization
 *****************************************************************************/
void initVdac(void)
{
  // Use default settings
  VDAC_Init_TypeDef        init = VDAC_INIT_DEFAULT;
  VDAC_InitChannel_TypeDef initChannel = VDAC_INITCHANNEL_DEFAULT;

  // Calculate the VDAC clock prescaler value resulting in a 1 MHz VDAC clock.
  init.prescaler = VDAC_PrescaleCalc(VDAC0, (uint32_t)CLK_VDAC_FREQ);

  // Set reference to internal 1.25V low noise reference
  init.reference = vdacRef1V25;

  // Since the minimum load requirement for high capacitance mode is 25 nF, turn
  // this mode off
  initChannel.highCapLoadEnable = false;

  // Initialize the VDAC and VDAC channel
  VDAC_Init(VDAC0, &init);
  VDAC_InitChannel(VDAC0, &initChannel, CHANNEL_NUM);

  // Enable the VDAC
  VDAC_Enable(VDAC0, CHANNEL_NUM, true);
}

/**************************************************************************//**
 * @brief
 *    Timer initialization
 *****************************************************************************/
void initTimer(void)
{
  uint32_t timerFreq, compareValue;

  // Initialize TIMER0
  TIMER_Init_TypeDef init = TIMER_INIT_DEFAULT;
  TIMER_InitCC_TypeDef timer_CCInit = TIMER_INITCC_DEFAULT;

  init.dmaClrAct = true;
  init.enable = false;

  // Configure capture/compare channel for output compare
  timer_CCInit.mode = timerCCModeCompare;
  timer_CCInit.cofoa = timerOutputActionToggle;

#if (PRS_MODE)
  // Configure the output to create PRS pulses
  timer_CCInit.prsOutput = timerPrsOutputPulse;
#endif

  TIMER_Init(TIMER0, &init);

#if TIMER_DEBUG
  // Route CC0 output to a GPIO
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

  compareValue = timerFreq / TIMER0_FREQ;

  // Set top value to overflow at the desired TIMER0_FREQ frequency
  TIMER_CompareSet(TIMER0, 0, compareValue);

  // Enable TIMER0
  TIMER_Enable(TIMER0, true);
}

/**************************************************************************//**
* @brief
*    Initialize the LDMA module
*
* @details
*    Configure the channel descriptor to use the default memory to
*    peripheral transfer descriptor. Modify it to not generate an interrupt
*    upon transfer completion (we don't need it for this example).
*    Also make the descriptor link to itself so that the descriptor runs
*    continuously.
*
* @note
*    The descriptor object needs to at least have static scope persistence so
*    that the reference to the object is valid beyond its first use in
*    initialization. This is because this code loops back to the same
*    descriptor after every dma transfer. If the reference isn't valid anymore,
*    then all dma transfers after the first one will fail.
******************************************************************************/
void initLdma(void)
{
  // Descriptor loops through the sine table and outputs its values to the VDAC
  static LDMA_Descriptor_t loopDescriptor =
    LDMA_DESCRIPTOR_LINKREL_M2P_BYTE(&sineTable[0], // Memory source address
                                     &VDAC0->CH0F,    // Peripheral destination
                                                      //   address
                                     SINE_TABLE_SIZE, // Number of halfwords per
                                                      //   transfer
                                     0);              // Link to same descriptor

  // Don't trigger interrupt when transfer is done
  loopDescriptor.xfer.doneIfs = 0;

  // Transfer halfwords (VDAC data register is 12 bits)
  loopDescriptor.xfer.size = ldmaCtrlSizeHalf;

  // LDMA initialization
  LDMA_Init_t init = LDMA_INIT_DEFAULT;
  LDMA_Init(&init);

  // Transfer configuration and trigger selection
  // Trigger on TIMER0 compare and set loop count to size of the sine table
  // minus one
#if PRS_MODE
  LDMA_TransferCfg_t transferConfig =
    LDMA_TRANSFER_CFG_PERIPHERAL(ldmaPeripheralSignal_LDMAXBAR_PRSREQ0);
#else
  LDMA_TransferCfg_t transferConfig =
    LDMA_TRANSFER_CFG_PERIPHERAL(ldmaPeripheralSignal_TIMER0_CC0);
#endif

  // Start the transfer
  uint32_t channelNum = 0;
  LDMA_StartTransfer(channelNum, &transferConfig, &loopDescriptor);
}

#if (PRS_MODE)

/**************************************************************************//**
 * @brief PRS initialization
 *****************************************************************************/
static void initPRS(void)
{
  // Connect TIMER0's output to a PRS channel
  PRS_SourceAsyncSignalSet(TIMER0_PRS_CHANNEL,
                           PRS_ASYNC_CH_CTRL_SOURCESEL_TIMER0,
                           _PRS_ASYNC_CH_CTRL_SIGSEL_TIMER0CC0);

  PRS_ConnectConsumer(TIMER0_PRS_CHANNEL, prsTypeAsync,
                      prsConsumerLDMA_REQUEST0);
}

#endif

void app_init(void)
{
  EMU_DCDCInit_TypeDef dcdcInit = EMU_DCDCINIT_DEFAULT;
  // Enable DC-DC converter
  EMU_DCDCInit(&dcdcInit);

  // Initialize clocks
  initCMU();

  // Initialize the VDAC, LDMA and Timer
  initVdac();
  initLdma();
  initTimer();

#if PRS_MODE
  initPRS();
#endif
}

void app_process_action(void)
{
  // Enter Sleep mode
  EMU_EnterEM1();
}
