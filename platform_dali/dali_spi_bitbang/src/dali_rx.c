/***************************************************************************//**
 * @file dail_rx.c
 * @brief DALI reception and decoding.
 * @version 0.01
 *******************************************************************************
 * # License
 * <b>Copyright 2019 Silicon Laboratories Inc. www.silabs.com</b>
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
#include "em_ldma.h"
#include "em_prs.h"
#include "em_timer.h"
#include "dali_config.h"
#include "dali_define.h"
#include "dali_macro.h"
#if defined(DALI_USE_DMADRV)
#include "dmadrv.h"

// DMA channels for DALI RX
static unsigned int daliRxPinCh;
static unsigned int daliRxTmrCh;
#endif

// RX buffer and descriptor linked lists
static uint16_t rxData[RX_BUFFER_SIZE];
SL_ALIGN(4) static LDMA_Descriptor_t descRxPin[PIN_DESC_SIZE];
SL_ALIGN(4) static LDMA_Descriptor_t descRxTimer[TMR_DESC_SIZE];

// Constant structures for DMA transfer
static const LDMA_TransferCfg_t xferRxPin = LDMA_TRANSFER_CFG_MEMORY();
static const LDMA_TransferCfg_t xferRxTimer =
  LDMA_TRANSFER_CFG_PERIPHERAL_LOOP(PRS_REQ_SRC, RX_BUFFER_SIZE - 2);
static const LDMA_Descriptor_t m2mLinkRx =
  LDMA_DESCRIPTOR_LINKREL_M2M_HALF(&DALI_RX_DIN, &rxData, 1UL, 1UL);
static const LDMA_Descriptor_t wriLinkRx =
  LDMA_DESCRIPTOR_LINKREL_WRITE(DALI_HALF_T, &DALI_TIMER->TOP, 1UL);
static const LDMA_Descriptor_t syncLinkRxPin =
  LDMA_DESCRIPTOR_LINKREL_SYNC(0, SYNC_RXPIN, SYNC_RXPIN, SYNC_RXPIN, 1UL);
#if !defined(DALI_SECONDARY)
static const LDMA_Descriptor_t syncLinkTxc =
  LDMA_DESCRIPTOR_LINKREL_SYNC(0, SYNC_TXC, SYNC_TXC, SYNC_TXC, 1UL);
#endif

#if defined(DALI_USE_DMADRV)

/***************************************************************************//**
 * @brief
 *   LDMA DALI RX complete callback function
 ******************************************************************************/
bool completeDaliRx(unsigned int channel, unsigned int primary, void *user)
{
  // Ignore unused parameters
  (void) channel;
  (void) primary;
  (void) user;

  // End of RX, stop RX pin DMA, disable RX pin PRS source and stop timers
  stopRxPinDma();
  resetRxPinPrs();
  DALI_TIMER->CMD = TIMER_CMD_STOP;
  TO_TIMER->CMD = TIMER_CMD_STOP;

#if !defined(DALI_SECONDARY)
  // Receive backward message, ready for decode
  setDaliStatus(DALI_BACKWARD_RX);
  return true;
#else
  // Receive forward message, ready for decode
  setDaliStatus(DALI_FORWARD_RX);

  // Set settling time for backward TX and start
  TIMER_CounterSet(TO_TIMER, 0);
  TIMER_TopSet(TO_TIMER, TX_BWARD_WAIT);
  TO_TIMER->CMD = TIMER_CMD_START;
  return true;
#endif
}

#else

/***************************************************************************//**
 * @brief
 *   LDMA Interrupt handler
 ******************************************************************************/
void LDMA_IRQHandler(void)
{
  uint32_t pending;

  // Read and clear the interrupt source
  pending = LDMA_IntGet();
  LDMA_IntClear(pending);

  if (pending & (1 << DMA_CH_RX_TMR)) {
    // End of RX, stop RX pin DMA, disable RX pin PRS source and stop timers
    stopRxPinDma();
    resetRxPinPrs();
    DALI_TIMER->CMD = TIMER_CMD_STOP;
    TO_TIMER->CMD = TIMER_CMD_STOP;

#if !defined(DALI_SECONDARY)
    // Receive backward message, ready for decode
    setDaliStatus(DALI_BACKWARD_RX);
#else
    // Receive forward message, ready for decode
    setDaliStatus(DALI_FORWARD_RX);

    // Set settling time for backward TX and start
    TIMER_CounterSet(TO_TIMER, 0);
    TIMER_TopSet(TO_TIMER, TX_BWARD_WAIT);
    TO_TIMER->CMD = TIMER_CMD_START;
#endif
  } else if (pending & LDMA_IF_ERROR) {
    while (1) {
    }
  }
}

#endif

/**************************************************************************//**
 * @brief
 *   Timeout TIMER Interrupt handler
 *****************************************************************************/
void TO_TIMER_ISR(void)
{
  // Clear flag for TIMER overflow interrupt
  TIMER_IntClear(TO_TIMER, TIMER_IF_OF);

  // RX timeout, stop RX pin DMA, disable RX pin PRS source and stop timers
  stopRxPinDma();
  resetRxPinPrs();
  DALI_TIMER->CMD = TIMER_CMD_STOP;
  TO_TIMER->CMD = TIMER_CMD_STOP;

#if !defined(DALI_SECONDARY)
  // Check RX data or RX backward timeout
  if (TO_TIMER->TOP == RX_EDGE_TO) {
    setDaliStatus(DALI_DATA_RX_TIMEOUT);
  } else {
    setDaliStatus(DALI_BACKWARD_RX_TIMEOUT);
  }
#else
  // Check RX data or TX backward timeout
  if (TO_TIMER->TOP == RX_EDGE_TO) {
    setDaliStatus(DALI_DATA_RX_TIMEOUT);
  } else {
    setDaliStatus(DALI_BACKWARD_TX_READY);
  }
#endif
}

/***************************************************************************//**
 * @brief
 *   Setup DALI RX input as PRS source.
 ******************************************************************************/
void initDaliRxPrs(void)
{
  LDMA_Descriptor_t *descPin = &descRxPin[0];

  // Request a DMA channel without callback function if using DMADRV
  allocatePinDmaCh();

  // Configure GPIO toggle as PRS source
  GPIO_PinModeSet(DALI_RX_PORT, DALI_RX_PIN, gpioModeInput, 1);
  GPIO_ExtIntConfig(DALI_RX_PORT, DALI_RX_PIN, DALI_RX_PIN, false, false,
                    false);
  initPinPrs();

#if !defined(DALI_SECONDARY)
  // Setup RX pin LDMA descriptor, wait TX complete to start
  *descPin = syncLinkTxc;

  // Start timeout timer
  descPin++;
  *descPin = wriLinkRx;
  descPin->wri.immVal = TIMER_CMD_START;
  descPin->wri.dstAddr = (uint32_t)(&TO_TIMER->CMD);

  // PRS input from RX pin is selected for RX and timeout timers
  descPin++;
  *descPin = wriLinkRx;
  setRxPinPrs();

  // Re-enable rising edge reload for idle level high
  setTimerRisingEdge();

  descPin++;
#endif

  // Wait RX pin edges to start, infinite loop
  *descPin = syncLinkRxPin;

  // Toggle edge detection for next PRS trigger if EFR32 S2
  syncEdgeToggle();

  // Refresh TOP and TOPB value when edge is detected
  descPin++;
  *descPin = wriLinkRx;

  descPin++;
  *descPin = wriLinkRx;
  descPin->wri.immVal = DALI_ONE_T;
  descPin->wri.dstAddr = (uint32_t)(&DALI_TIMER->TOPB);

  // Set timeout time when edge is detected (actually once is enough)
  descPin++;
  *descPin = wriLinkRx;
  descPin->wri.immVal = RX_EDGE_TO;
  descPin->wri.dstAddr = (uint32_t)(&TO_TIMER->TOP);
  loopBackDesc();               // Wait next RX pin edge
}

/***************************************************************************//**
 * @brief
 *   Setup TIMER OF as PRS source.
 ******************************************************************************/
void initDaliRxTimer(void)
{
  TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
  TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;
  LDMA_Descriptor_t *descTmr = &descRxTimer[0];

  // Request a DMA channel with callback function if using DMADRV
  allocateTmrDmaCh();

  // Configure CC channel, must use CC0 for reload and start
  timerCCInit.prsInput = true;
  timerCCInit.prsSel = PIN_PRS_CH;
  setPrsInputType();                    // For EFR32 S2
  TIMER_InitCC(TO_TIMER, 0, &timerCCInit);
  TIMER_InitCC(DALI_TIMER, 0, &timerCCInit);

  // Configure RX TIMER
  timerInit.enable = false;
  timerInit.riseAction = timerInputActionReloadStart;
  setTimerFallingEdge();                // For EFR32 S2
  TIMER_Init(DALI_TIMER, &timerInit);

  // Configure timeout TIMER, prescale for longer interval
  timerInit.prescale = timerPrescale16;
  TIMER_Init(TO_TIMER, &timerInit);

  // Enable timeout TIMER overflow interrupt
  TIMER_IntEnable(TO_TIMER, TIMER_IEN_OF);
  TIMER_IntClear(TO_TIMER, _TIMER_IF_MASK);
  NVIC_EnableIRQ(TO_TIMER_IRQn);
  NVIC_ClearPendingIRQ(TO_TIMER_IRQn);

  // Configure RX TIMER OF as PRS source
  setRxTimerPrs();

  // Select PRS channel for DMA request
  setDmaReqPrsCh();

  // Setup RX TIMER LDMA descriptor
  *descTmr = m2mLinkRx;
  descTmr->xfer.structReq = 0;
  descTmr->xfer.srcInc = ldmaCtrlSrcIncNone;

  descTmr++;
  *descTmr = m2mLinkRx;
  descTmr->xfer.structReq = 0;
  descTmr->xfer.srcInc = ldmaCtrlSrcIncNone;

  descTmr->xfer.dstAddr = 0;
  descTmr->xfer.decLoopCnt = 1;
  descTmr->xfer.dstAddrMode = ldmaCtrlDstAddrModeRel;
  descTmr->xfer.link = 0;
  descTmr->wri.linkAddr = 0;
}

/***************************************************************************//**
 * @brief
 *   Setup DMA for DALI RX.
 ******************************************************************************/
void startDaliRxDma(void)
{
  // Set RX TIMER TOP and TOPB
  TIMER_TopSet(DALI_TIMER, DALI_HALF_T);
  TIMER_TopBufSet(DALI_TIMER, DALI_ONE_T);

  // Idle level high, disable rising edge reload to avoid false trigger
  resetRiseReload();
  // Set first trigger edge (rising or falling) if EFR32 S2
  setDmaStartEdge();

#if !defined(DALI_SECONDARY)
  // Clear timeout timer, set timeout interval
  TIMER_CounterSet(TO_TIMER, 0);
  TIMER_TopSet(TO_TIMER, RX_BWARD_TO);
#else
  setPinPrs();
  // Idle level high, re-enable rising edge reload
  setRiseReload();
  // Clear timeout timer, set timeout interval
  TIMER_CounterSet(TO_TIMER, 0);
  TIMER_TopSet(TO_TIMER, RX_EDGE_TO);
  setDaliStatus(DALI_FORWARD_RX_WAIT);
#endif

  // Activate DMA RX, no interrupt for pin DMA (infinite loop)
  startPinTmrTransfer();
}

/***************************************************************************//**
 * @brief
 *   Decode received DALI bit stream.
 *
 * @param[in] addr
 *   Pointer of address byte of DALI forward frame.
 *
 * @param[in] data
 *   Pointer of data byte of DALI forward or backward frame.
 *
 * @return
 *   True if succeed, false if fail.
 ******************************************************************************/
bool decodeDaliRx(uint8_t *addr, uint8_t *data)
{
  uint8_t i, j;

  // Check start bit
  if ((rxData[0] & (1 << DALI_RX_PIN)) >> DALI_RX_PIN != HIGH_MSB) {
    setDaliStatus(DALI_IDLE);
    TO_TIMER->CMD = TIMER_CMD_STOP;     // Stop secondary backward TX timeout
    return false;
  }

  if ((rxData[1] & (1 << DALI_RX_PIN)) >> DALI_RX_PIN != HIGH_LSB) {
    setDaliStatus(DALI_IDLE);
    TO_TIMER->CMD = TIMER_CMD_STOP;     // Stop secondary backward TX timeout
    return false;
  }

  // Check stop bit
  for (i = RX_BIT_START; i < RX_BIT_STOP; i++) {
    if ((rxData[i] & (1 << DALI_RX_PIN)) >> DALI_RX_PIN != STOP_LEVEL) {
      setDaliStatus(DALI_IDLE);
      TO_TIMER->CMD = TIMER_CMD_STOP;   // Stop secondary backward TX timeout
      return false;
    }
  }

  // Get first encoded half word
  rxData[0] = 0;
  for (i = RX_START; i < (16 + RX_START); i++) {
    rxData[0] <<= 1;
    rxData[0] |= (rxData[i] & (1 << DALI_RX_PIN)) >> DALI_RX_PIN;
  }

  // Decode first encoded half word to byte
  i = rxData[0] >> 8;
  j = rxData[0] & 0xff;
  *data = (manchesterDecodeTable[i] << 4) | manchesterDecodeTable[j];

#if !defined(DALI_SECONDARY)
  (void)addr;
  (void)data;
  // Idle if backward message receive
  setDaliStatus(DALI_IDLE);
  return true;
#else
  // Get second encoded half word if forward frame
  *addr = *data;
  rxData[1] = 0;
  for (i = (16 + RX_START); i < (32 + RX_START); i++) {
    rxData[1] <<= 1;
    rxData[1] |= (rxData[i] & (1 << DALI_RX_PIN)) >> DALI_RX_PIN;
  }

  // Decode second encoded half word to byte
  i = rxData[1] >> 8;
  j = rxData[1] & 0xff;
  *data = (manchesterDecodeTable[i] << 4) | manchesterDecodeTable[j];

  // Wait timeout to TX backward if forward message receive
  setDaliStatus(DALI_BACKWARD_TX_WAIT);
  return true;
#endif
}
