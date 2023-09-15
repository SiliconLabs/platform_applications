/***************************************************************************//**
 * @file dali_tx.c
 * @brief DALI encoding and transmission.
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
#include "em_usart.h"
#include "dali_config.h"
#include "dali_define.h"
#include "dali_macro.h"
#if defined(DALI_USE_DMADRV)
#include "dmadrv.h"

// DMA channel for DALI TX
static unsigned int daliTxCh;
#endif

// DALI status
SL_ALIGN(4) DaliStatus_t daliStatus;

// TX buffer and descriptor linked list
static uint8_t txData[TX_BUFFER_SIZE];
SL_ALIGN(4) static LDMA_Descriptor_t descTx[TX_DESC_SIZE];

// Constant structures for DMA transfer
static const LDMA_TransferCfg_t xferTx =
  LDMA_TRANSFER_CFG_PERIPHERAL(SPI_DMA_TXREQ);
#if !defined(DALI_SECONDARY)
static const LDMA_Descriptor_t m2pFwdBwdTx =
  LDMA_DESCRIPTOR_SINGLE_M2P_BYTE(txData, &SPI_USART->TXDATA, TX_BUFFER_SIZE);
#else
static const LDMA_Descriptor_t m2pFwdBwdTx =
  LDMA_DESCRIPTOR_LINKREL_M2P_BYTE(txData, &SPI_USART->TXDATA,
                                   TX_BUFFER_SIZE, 1UL);
static const LDMA_Descriptor_t syncLinkBwdTxc =
  LDMA_DESCRIPTOR_LINKREL_SYNC(0, SYNC_TXC, SYNC_TXC, SYNC_TXC,
                               1UL);
static const LDMA_Descriptor_t wriSingleTx =
  LDMA_DESCRIPTOR_SINGLE_WRITE(DALI_BACKWARD_TX_DONE, &daliStatus);
#endif

/***************************************************************************//**
 * @brief
 *   Initialize LDMA.
 ******************************************************************************/
static void initDaliLdma(void)
{
#if defined(DALI_USE_DMADRV)
  // Initialize DMA driver
  DMADRV_Init();
  // Enable PRS inputs to set respective bits of LDMA SYNC register
  setLdmaSyncPrs();
#else
  LDMA_Init_t init = LDMA_INIT_DEFAULT;

  // Enable PRS inputs to set respective bits of LDMA SYNC register
  init.ldmaInitCtrlSyncPrsSetEn = SYNC_RXPIN + SYNC_TXC;
  // Initialize DMA
  LDMA_Init(&init);
#endif
}

/***************************************************************************//**
 * @brief
 *   Initialize USART for DALI TX.
 ******************************************************************************/
static void initDaliTx(void)
{
  USART_InitSync_TypeDef usartInit = USART_INITSYNC_DEFAULT;
  LDMA_Descriptor_t *desc = &descTx[0];

  // Initialize USART for SPI TX only
  usartInit.enable = usartEnableTx;
  usartInit.baudrate = SPI_BAUDRATE * 2;
  usartInit.msbf = true;
  USART_InitSync(SPI_USART, &usartInit);

  // Configure GPIO pin for SPI TX (MOSI)
  GPIO_PinModeSet(SPI_MOSI_PORT, SPI_MOSI_PIN, gpioModePushPull, 1);

  // Send dummy data to set TX pin idle level
  USART_SpiTransfer(SPI_USART, TX_IDLE_OUTPUT);

  // Enable routing for SPI TX pin (USART to GPIO), USART TXC as PRS source
  setTxGpioPrs();

  // Request a DMA channel without callback function if using DMADRV
  allocateTxDmaCh();

  // Setup LMDA descriptor(s) for DALI TX
  *desc = m2pFwdBwdTx;
  desc->xfer.doneIfs = 0;
#if defined(DALI_SECONDARY)
  desc++;
  *desc = syncLinkBwdTxc;

  desc++;
  *desc = wriSingleTx;
#endif
}

/***************************************************************************//**
 * @brief
 *   Initialize DALI interface.
 ******************************************************************************/
void initDali(void)
{
  // Initialize LDMA
  initDaliLdma();

  // Configure HFXO as TIMER clock source (EM01GRPACLK) for EFR32 S2
  setTimerClk();

  // Enable peripheral clocks
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(cmuClock_PRS, true);
  CMU_ClockEnable(DALI_TIMER_CLK, true);
  CMU_ClockEnable(TO_TIMER_CLK, true);
  CMU_ClockEnable(SPI_USART_CLK, true);

  // Initialize USART for DALI TX
  initDaliTx();

  // Initialize TIMER and PRS for DALI RX
  initDaliRxTimer();
  initDaliRxPrs();
}

/***************************************************************************//**
 * @brief
 *   Encode and transmit the DALI forward frame or backward frame.
 *
 * @param[in] addr
 *   The address byte of DALI forward frame.
 *
 * @param[in] data
 *   The data byte of DALI forward frame or backward frame.
 ******************************************************************************/
void startDaliTxDma(uint8_t addr, uint8_t data)
{
#if !defined(DALI_SECONDARY)
  uint16_t addrCode;
  uint16_t dataCode;

  // Set forward TX in progress
  setDaliStatus(DALI_FORWARD_TX);

  // Get the encoded data and address from table
  dataCode = manchesterEncodeTable[data];
  addrCode = manchesterEncodeTable[addr];

  // Pack the address and data with start and stop bit for forward frame
  txData[0] = TX_IDLE_OUTPUT;
  txData[1] = TX_IDLE_OUTPUT;
  txData[2] = TX_IDLE_OUTPUT;
  txData[3] = (uint8_t)(START_NIBBLE | (addrCode >> RIGHT_SHIFT0));
  txData[4] = (uint8_t)(addrCode >> RIGHT_SHIFT1);
  txData[5] = (uint8_t)((addrCode << LEFT_SHIFT0)
                        | (dataCode >> RIGHT_SHIFT0));
  txData[6] = (uint8_t)(dataCode >> RIGHT_SHIFT1);
  txData[7] = (uint8_t)((dataCode << LEFT_SHIFT0) | STOP_NIBBLE);
#else
  uint16_t dataCode;

  // Set backward TX in progress
  setDaliStatus(DALI_BACKWARD_TX);

  // Get the encoded data from table
  dataCode = manchesterEncodeTable[data];

  // Pack the data with start and stop bit for backward frame
  txData[0] = (uint8_t)(START_NIBBLE | (dataCode >> RIGHT_SHIFT0));
  txData[1] = (uint8_t)(dataCode >> RIGHT_SHIFT1);
  txData[2] = (uint8_t)((dataCode << LEFT_SHIFT0) | STOP_NIBBLE);
#endif

  // Activate forward or backward frame DMA TX
  startTxTransfer();

#if !defined(DALI_SECONDARY)
  // No interrupt for TX
  diableTxDmaInt();

  // Setup DMA to receive backward frame
  startDaliRxDma();
#endif
}
