/***************************************************************************//**
 * @file dali_macro.h
 * @brief Header file for DALI macros.
 * @version 0.00
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

#ifndef DALI_MACRO_H
#define DALI_MACRO_H

#if defined(_SILICON_LABS_32B_SERIES_2)
// Set LDMA SYNC register for PRS trigger
#define setLdmaSyncPrs()                                           \
  do {                                                             \
    LDMA->SYNCHWEN |=                                              \
      ((SYNC_RXPIN + SYNC_TXC) << _LDMA_SYNCHWEN_SYNCSETEN_SHIFT); \
  } while (0)

// Set TX GPIO and PRS
#define setTxGpioPrs()                                           \
  do {                                                           \
    GPIO->USARTROUTE[SPI_USART_NUM].ROUTEEN =                    \
      GPIO_USART_ROUTEEN_TXPEN;                                  \
    GPIO->USARTROUTE[SPI_USART_NUM].TXROUTE =                    \
      (SPI_MOSI_PORT << _GPIO_USART_TXROUTE_PORT_SHIFT)          \
      | (SPI_MOSI_PIN << _GPIO_USART_TXROUTE_PIN_SHIFT);         \
    PRS_SourceAsyncSignalSet(TX_PRS_CH, TX_PRS_SRC, TX_PRS_SIG); \
  } while (0)

// Set RX pin PRS
#define initPinPrs()                                                         \
  do {                                                                       \
    PRS_SourceAsyncSignalSet(PIN_PRS_CH,                                     \
                             PRS_ASYNC_CH_CTRL_SOURCESEL_NONE, PIN_PRS_SIG); \
  } while (0)

// Set RX pin PRS through DMA WRI
#define setRxPinPrs()                                    \
  do {                                                   \
    descPin->wri.immVal = PRS->ASYNC_CH[PIN_PRS_CH].CTRL \
                          | PIN_PRS_SRC;                 \
    descPin->wri.dstAddr =                               \
      (uint32_t)(&PRS->ASYNC_CH[PIN_PRS_CH].CTRL);       \
  } while (0)

// Set RX pin PRS source
#define setPinPrs()                                \
  do {                                             \
    PRS->ASYNC_CH[PIN_PRS_CH].CTRL |= PIN_PRS_SRC; \
  } while (0)

// Stop RX pin DMA
#if defined(DALI_USE_DMADRV)
#define stopRxPinDma()                \
  do {                                \
    LDMA->CHDIS = (1 << daliRxPinCh); \
  } while (0)
#else
#define stopRxPinDma()                  \
  do {                                  \
    LDMA->CHDIS = (1 << DMA_CH_RX_PIN); \
  } while (0)
#endif

// Reset RX pin PRS source
#define resetRxPinPrs()                   \
  do {                                    \
    PRS->ASYNC_CH[PIN_PRS_CH].CTRL &=     \
      ~_PRS_ASYNC_CH_CTRL_SOURCESEL_MASK; \
  } while (0)

// Descriptor back count
#define loopBackDesc()  do { descPin->wri.linkAddr = -16; } while (0)

// TIMER clock source
#define setTimerClk()                                         \
  do {                                                        \
    CMU_ClockSelectSet(cmuClock_EM01GRPACLK, cmuSelect_HFXO); \
  } while (0)

// Set RX TIMER PRS
#define setRxTimerPrs()                                   \
  do {                                                    \
    PRS_SourceAsyncSignalSet(TIMER_PRS_CH, TIMER_PRS_SRC, \
                             TIMER_PRS_SIG);              \
  } while (0)

// RX TIMER PRS input type
#define setPrsInputType()                               \
  do {                                                  \
    timerCCInit.prsInputType = timerPrsInputAsyncLevel; \
  } while (0)

// RX TIMER PRS falling edge action
#define setTimerFallingEdge()                           \
  do {                                                  \
    timerInit.fallAction = timerInputActionReloadStart; \
  } while (0)

// Set DMA request PRS channel
#define setDmaReqPrsCh()                                          \
  do {                                                            \
    PRS_ConnectConsumer(TIMER_PRS_CH, prsTypeAsync, PRS_DMA_REQ); \
  } while (0)

// Toggle edge detection for next SYNC trigger through DMA WRI
#define syncEdgeToggle()                                        \
  do {                                                          \
    descPin++;                                                  \
    *descPin = wriLinkRx;                                       \
    descPin->wri.immVal = SYNC_RXPIN                            \
                          << _LDMA_SYNCHWSEL_SYNCSETEDGE_SHIFT; \
    descPin->wri.dstAddr = (uint32_t)(&LDMA->SYNCHWSEL_TGL);    \
  } while (0)
#else
// Set LDMA SYNC register for PRS trigger
#define setLdmaSyncPrs()                                          \
  do {                                                            \
    LDMA->CTRL |=                                                 \
      ((SYNC_RXPIN + SYNC_TXC) << _LDMA_CTRL_SYNCPRSSETEN_SHIFT); \
  } while (0)

// Set TX GPIO and PRS
#define setTxGpioPrs()                                     \
  do {                                                     \
    SPI_USART->ROUTEPEN = USART_ROUTEPEN_TXPEN;            \
    SPI_USART->ROUTELOC0 = SPI_TX_LOC;                     \
    PRS_SourceSignalSet(TX_PRS_CH, TX_PRS_SRC, TX_PRS_SIG, \
                        prsEdgeOff);                       \
  } while (0)

// Set RX pin PRS
#define initPinPrs()                                            \
  do {                                                          \
    PRS_SourceSignalSet(PIN_PRS_CH, PRS_CH_CTRL_SOURCESEL_NONE, \
                        PIN_PRS_SIG, prsEdgeBoth);              \
  } while (0)

// Set RX pin PRS through DMA WRI
#define setRxPinPrs()                                             \
  do {                                                            \
    descPin->wri.immVal = PRS->CH[PIN_PRS_CH].CTRL | PIN_PRS_SRC; \
    descPin->wri.dstAddr = (uint32_t)(&PRS->CH[PIN_PRS_CH].CTRL); \
  } while (0)

// Set RX pin PRS source
#define setPinPrs()                          \
  do {                                       \
    PRS->CH[PIN_PRS_CH].CTRL |= PIN_PRS_SRC; \
  } while (0)

// Stop RX pin DMA
#if defined(DALI_USE_DMADRV)
#define stopRxPinDma()                                   \
  do {                                                   \
    BUS_RegMaskedClear(&LDMA->CHEN, (1 << daliRxPinCh)); \
  } while (0)
#else
#define stopRxPinDma()                                     \
  do {                                                     \
    BUS_RegMaskedClear(&LDMA->CHEN, (1 << DMA_CH_RX_PIN)); \
  } while (0)
#endif

// Reset RX pin PRS source
#define resetRxPinPrs()             \
  do {                              \
    PRS->CH[PIN_PRS_CH].CTRL &=     \
      ~_PRS_CH_CTRL_SOURCESEL_MASK; \
  } while (0)

// Descriptor back count
#define loopBackDesc()          do { descPin->wri.linkAddr = -12; } while (0)

// Do nothing for EFR32 S1
#define setTimerClk()           (void)0

// Set RX TIMER PRS
#define setRxTimerPrs()                                             \
  do {                                                              \
    PRS_SourceSignalSet(TIMER_PRS_CH, TIMER_PRS_SRC, TIMER_PRS_SIG, \
                        prsEdgeOff);                                \
  } while (0)

// Do nothing for EFR32 S1
#define setPrsInputType()       (void)0

// Do nothing for EFR32 S1
#define setTimerFallingEdge()   (void)0

// Set DMA request PRS channel
#define setDmaReqPrsCh()           \
  do {                             \
    PRS->PRS_REQ_REG = PRS_DMA_CH; \
  } while (0)

// Do nothing for EFR32 S1
#define syncEdgeToggle()        (void)0
#endif

#if (IDLE_LEVEL == 0)
// Do nothing for idle level low
#define setRiseReload()         (void)0
#define resetRiseReload()       (void)0

// Set DMA SYNC trigger edge
#if defined(_SILICON_LABS_32B_SERIES_2)
#define setDmaStartEdge()                                       \
  do {                                                          \
    LDMA->SYNCHWSEL_CLR = SYNC_RXPIN                            \
                          << _LDMA_SYNCHWSEL_SYNCSETEDGE_SHIFT; \
  } while (0)
#else
// Do nothing for EFR32 S1
#define setDmaStartEdge()       (void)0
#endif

// Do nothing for idle level low
#define setTimerRisingEdge()    (void)0
#else
// Set and reset TIMER rising edge reload and start
#define setRiseReload()                         \
  do {                                          \
    DALI_TIMER->CTRL |= _TIMER_CTRL_RISEA_MASK; \
    TO_TIMER->CTRL |= _TIMER_CTRL_RISEA_MASK;   \
  } while (0)

#define resetRiseReload()                        \
  do {                                           \
    DALI_TIMER->CTRL &= ~_TIMER_CTRL_RISEA_MASK; \
    TO_TIMER->CTRL &= ~_TIMER_CTRL_RISEA_MASK;   \
  } while (0)

// Set DMA SYNC trigger edge
#if defined(_SILICON_LABS_32B_SERIES_2)
#define setDmaStartEdge()                                       \
  do {                                                          \
    LDMA->SYNCHWSEL_SET = SYNC_RXPIN                            \
                          << _LDMA_SYNCHWSEL_SYNCSETEDGE_SHIFT; \
  } while (0)
#else
// Do nothing for EFR32 S1
#define setDmaStartEdge()       (void)0
#endif

// Set TIMER rising edge reload and start through DMA WRI
#define setTimerRisingEdge()                                         \
  do {                                                               \
    descPin++;                                                       \
    *descPin = wriLinkRx;                                            \
    descPin->wri.immVal = DALI_TIMER->CTRL | _TIMER_CTRL_RISEA_MASK; \
    descPin->wri.dstAddr = (uint32_t)(&DALI_TIMER->CTRL);            \
    descPin++;                                                       \
    *descPin = wriLinkRx;                                            \
    descPin->wri.immVal = TO_TIMER->CTRL | _TIMER_CTRL_RISEA_MASK;   \
    descPin->wri.dstAddr = (uint32_t)(&TO_TIMER->CTRL);              \
  } while (0)
#endif

#if defined(DALI_USE_DMADRV)
// Allocate TX DMA channel without callback function
#define allocateTxDmaCh()                    \
  do {                                       \
    DMADRV_AllocateChannel(&daliTxCh, NULL); \
  } while (0)

// Start TX DMA transfer
#define startTxTransfer()                                           \
  do {                                                              \
    LDMA_StartTransfer(daliTxCh, (void *)&xferTx, (void *)&descTx); \
  } while (0)

// Disable TX DMA interupt
#define diableTxDmaInt()            \
  do {                              \
    LDMA_IntDisable(1 << daliTxCh); \
  } while (0)

// Allocate RX pin DMA channel without callback function
#define allocatePinDmaCh()                      \
  do {                                          \
    DMADRV_AllocateChannel(&daliRxPinCh, NULL); \
  } while (0)

// Allocate TIMER DMA channel, set callback function then stop DMA transfer
#if defined(_SILICON_LABS_32B_SERIES_2)
#define allocateTmrDmaCh()                               \
  do {                                                   \
    DMADRV_AllocateChannel(&daliRxTmrCh, NULL);          \
    DMADRV_MemoryPeripheral(daliRxTmrCh,                 \
                            dmadrvPeripheralSignal_NONE, \
                            rxData,                      \
                            rxData,                      \
                            false,                       \
                            1,                           \
                            dmadrvDataSize1,             \
                            completeDaliRx,              \
                            NULL);                       \
    LDMA->CHDIS = 1 << daliRxTmrCh;                      \
  } while (0)
#else
#define allocateTmrDmaCh()                               \
  do {                                                   \
    DMADRV_AllocateChannel(&daliRxTmrCh, NULL);          \
    DMADRV_MemoryPeripheral(daliRxTmrCh,                 \
                            dmadrvPeripheralSignal_NONE, \
                            rxData,                      \
                            rxData,                      \
                            false,                       \
                            1,                           \
                            dmadrvDataSize1,             \
                            completeDaliRx,              \
                            NULL);                       \
    BUS_RegMaskedClear(&LDMA->CHEN, (1 << daliRxTmrCh)); \
  } while (0)
#endif

// Start RX pin and TIMER DMA transfer
#define startPinTmrTransfer()                             \
  do {                                                    \
    LDMA_StartTransfer(daliRxPinCh, (void *)&xferRxPin,   \
                       (void *)&descRxPin);               \
    LDMA_StartTransfer(daliRxTmrCh, (void *)&xferRxTimer, \
                       (void *)&descRxTimer);             \
    LDMA_IntDisable(1 << daliRxPinCh);                    \
  } while (0)
#else
// Do nothing if not using DMADRV
#define allocateTxDmaCh()       (void)0

// Start TX DMA transfer
#define startTxTransfer()                                                \
  do {                                                                   \
    LDMA_StartTransfer(DMA_CH_SPI_TX, (void *)&xferTx, (void *)&descTx); \
  } while (0)

// Disable TX DMA interupt
#define diableTxDmaInt()                 \
  do {                                   \
    LDMA_IntDisable(1 << DMA_CH_SPI_TX); \
  } while (0)

// Do nothing if not using DMADRV
#define allocatePinDmaCh()      (void)0

// Do nothing if not using DMADRV
#define allocateTmrDmaCh()      (void)0

// Start RX pin and TIMER DMA transfer
#define startPinTmrTransfer()                               \
  do {                                                      \
    LDMA_StartTransfer(DMA_CH_RX_PIN, (void *)&xferRxPin,   \
                       (void *)&descRxPin);                 \
    LDMA_StartTransfer(DMA_CH_RX_TMR, (void *)&xferRxTimer, \
                       (void *)&descRxTimer);               \
    LDMA_IntDisable(1 << DMA_CH_RX_PIN);                    \
  } while (0)
#endif

/***************************************************************************//**
 * @brief
 *   Get DALI status.
 *
 * @return
 *   Current DALI status.
 ******************************************************************************/
__STATIC_INLINE DaliStatus_t getDaliStatus(void)
{
  return daliStatus;
}

/***************************************************************************//**
 * @brief
 *   Set DALI status.
 *
 * @param[in] status
 *   DALI status to be set.
 ******************************************************************************/
__STATIC_INLINE void setDaliStatus(DaliStatus_t status)
{
  daliStatus = status;
}

#endif // DALI_MACRO_H
