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

#include <stdio.h>
#include <stdbool.h>
#include "dali_config.h"
#include "dali_define.h"
#include "dali_macro.h"
#include "em_cmu.h"
#include "em_eusart.h"
#include "em_gpio.h"
#include "em_emu.h"
#include "dmadrv.h"
#include "sl_sleeptimer.h"

// DALI status
SL_ALIGN(4) DaliStatus_t daliStatus;
uint8_t idleLevel;
int8_t c;
uint8_t fwdAddr;              // Forward frame address
uint8_t fwdData;              // Forward frame data
uint8_t bwdData;              // Backward frame data
uint16_t fwdAddrData;         // Forward frame address & data
unsigned int dmaTxChannel;
unsigned int dmaRxChannel;
DaliStatus_t state;           // DALI status

#if !defined(DALI_SECONDARY)
sl_sleeptimer_timer_handle_t te_7_sleeptimer;
sl_sleeptimer_timer_handle_t te_22_sleeptimer;
#else
sl_sleeptimer_timer_handle_t te_10_sleeptimer;
#endif

bool dmaRxCallback(unsigned int channel,
                   unsigned int sequenceNo,
                   void *userParam)
{
  (void) channel;
  (void) sequenceNo;
  (void) userParam;

#if !defined(DALI_SECONDARY)
  setDaliStatus(DALI_BACKWARD_RX);
#else
  setDaliStatus(DALI_BACKWARD_TX_WAIT);
#endif

  return true;
}

void EUSART1_TX_IRQHandler(void)
{
  uint32_t flags = EUSART_IntGet(EUSART1);
  EUSART_IntClear(EUSART1, flags);

  if (flags & EUSART_IF_TXC) {
#if !defined(DALI_SECONDARY)
    setDaliStatus(DALI_FORWARD_TX);
#else
    setDaliStatus(DALI_BACKWARD_TX_DONE);
#endif
  }
}

#if !defined(DALI_SECONDARY)
void GPIO_EVEN_IRQHandler(void)
{
  uint32_t flags = GPIO_IntGet();

  // Start bit of backward frame was detected, so stop 22TE timeout timer
  if (flags & (1 << DALI_RX_PIN)) {
    sl_sleeptimer_stop_timer(&te_22_sleeptimer);

    GPIO_ExtIntConfig(DALI_RX_PORT, DALI_RX_PIN, DALI_RX_PIN, false, true,
                      false);
    NVIC_DisableIRQ(GPIO_EVEN_IRQn);

    setDaliStatus(DALI_BACKWARD_RX_WAIT);
  }

  GPIO_IntClear(flags);
}

void TE_7_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void) handle;
  (void) data;

  setDaliStatus(DALI_DATA_RX_TIMEOUT);
}

void TE_22_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void) handle;
  (void) data;

  setDaliStatus(DALI_BACKWARD_RX_TIMEOUT);
}

#else
void TE_10_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void) handle;
  (void) data;

  setDaliStatus(DALI_BACKWARD_TX_READY);
}

#endif

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
#if (IDLE_LEVEL == 0)           // Get idle level
  idleLevel = 0;
#else
  idleLevel = 1;
#endif

  // Initialize EUSART to use HF DALI mode
  CMU_ClockEnable(cmuClock_EUSART1, true);
  EUSART_AdvancedInit_TypeDef advInit = EUSART_ADVANCED_DALI_INIT_DEFAULT;
  EUSART_DaliInit_TypeDef daliInit = EUSART_DALI_INIT_DEFAULT_HF;
#if defined(DALI_SECONDARY)
  daliInit.TXdatabits = eusartDaliTxDataBits8;
  daliInit.RXdatabits = eusartDaliRxDataBits16;
#endif
  daliInit.init.advancedSettings = &advInit;
  EUSART_DaliInit(EUSART1, &daliInit);

  // Enable EUSART interrupts
  EUSART_IntEnable(EUSART1, EUSART_IEN_TXC);
  EUSART_IntClear(EUSART1, EUSART_IF_TXC);
  NVIC_ClearPendingIRQ(EUSART1_TX_IRQn);
  NVIC_EnableIRQ(EUSART1_TX_IRQn);

  // Configure DALI pins
  CMU_ClockEnable(cmuClock_GPIO, true);
  GPIO_PinModeSet(DALI_TX_PORT, DALI_TX_PIN, gpioModePushPull, 1);
  GPIO_PinModeSet(DALI_RX_PORT, DALI_RX_PIN, gpioModeInput, 0);
  GPIO->EUSARTROUTE[1].TXROUTE =
    (DALI_TX_PORT <<
      _GPIO_EUSART_TXROUTE_PORT_SHIFT)
    | (DALI_TX_PIN << _GPIO_EUSART_TXROUTE_PIN_SHIFT);
  GPIO->EUSARTROUTE[1].RXROUTE =
    (DALI_RX_PORT <<
      _GPIO_EUSART_RXROUTE_PORT_SHIFT)
    | (DALI_RX_PIN << _GPIO_EUSART_RXROUTE_PIN_SHIFT);
  GPIO->EUSARTROUTE[1].ROUTEEN = GPIO_EUSART_ROUTEEN_RXPEN
                                 | GPIO_EUSART_ROUTEEN_TXPEN;

  // Press 1 to start
#if !defined(DALI_SECONDARY)
  fwdAddr = 0xff;       // Broadcast address
  fwdData = 0x90;       // Query status command
  fwdAddrData = 0xff90;
  printf("\nPress 1: DALI Main (Idle Level %d) - Forward Frame TX\n",
         idleLevel);
#else
  bwdData = 0x64;       // Return status
  printf("\nPress 1: DALI Secondary (Idle Level %d) - Forward Frame RX\n",
         idleLevel);
#endif

  // Allocate DMA channels for DALI TX and RX
  DMADRV_AllocateChannel(&dmaTxChannel, NULL);
  DMADRV_AllocateChannel(&dmaRxChannel, NULL);
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
  state = getDaliStatus();

#if !defined(DALI_SECONDARY)
  // Handle different status of DALI main
  switch (state) {
    case DALI_IDLE:
      c = getchar();
      if (c == '1') {
        c = 0;
        printf("Sending DALI Main Forward Frame\n");
        // Transmit forward frame
        DMADRV_MemoryPeripheral(dmaTxChannel,
                                dmadrvPeripheralSignal_EUSART1_TXBL,
                                (void *)&(EUSART1->TXDATA),
                                &fwdAddrData,
                                false,
                                1,
                                dmadrvDataSize2,
                                NULL,
                                NULL);
      }
      break;

    case DALI_FORWARD_TX:
      // Start 7TE and 22TE timeout timers
      sl_sleeptimer_start_timer(&te_7_sleeptimer,
                                95,
                                TE_7_callback,
                                (void *)NULL,
                                0,
                                0);
      sl_sleeptimer_start_timer(&te_22_sleeptimer,
                                300,
                                TE_22_callback,
                                (void *)NULL,
                                0,
                                0);
      break;

    case DALI_DATA_RX_TIMEOUT:
      // Ready to receive backward frame after 7TE
      DMADRV_PeripheralMemory(dmaRxChannel,
                              dmadrvPeripheralSignal_EUSART1_RXDATAV,
                              &bwdData,
                              (void *)&(EUSART1->RXDATA),
                              false,
                              1,
                              dmadrvDataSize1,
                              (DMADRV_Callback_t)dmaRxCallback,
                              NULL);

      // Enable GPIO interrupt on RX pin to detect start bit
      GPIO_ExtIntConfig(DALI_RX_PORT,
                        DALI_RX_PIN,
                        DALI_RX_PIN,
                        false,
                        true,
                        true);
      NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
      NVIC_EnableIRQ(GPIO_EVEN_IRQn);
      break;

    case DALI_BACKWARD_RX:
      setDaliStatus(DALI_IDLE);
      printf("FWD TX - Address: %3d Data: %3d\n", fwdAddr, fwdData);
      printf("BWD RX - Data: %3d\n", bwdData);
      printf("\nPress 1: DALI Main (Idle Level %d) - Forward Frame TX\n",
             idleLevel);
      break;

    case DALI_BACKWARD_RX_TIMEOUT:
      printf("Backward RX Timeout\n");
      setDaliStatus(DALI_IDLE);
      printf("\nPress 1: DALI Main (Idle Level %d) - Forward Frame TX\n",
             idleLevel);
      break;

    default:
      break;
  }
#else
  // Handle different status of DALI secondary
  switch (state) {
    case DALI_IDLE:
      c = getchar();
      if (c == '1') {
        c = 0;
        printf("Waiting DALI Main Forward Frame\n");
        // Ready to receive forward frame
        setDaliStatus(DALI_FORWARD_RX_WAIT);
        DMADRV_PeripheralMemory(dmaRxChannel,
                                dmadrvPeripheralSignal_EUSART1_RXDATAV,
                                &fwdAddrData,
                                (void *)&(EUSART1->RXDATA),
                                false,
                                1,
                                dmadrvDataSize2,
                                (DMADRV_Callback_t)dmaRxCallback,
                                NULL);
      }
      break;

    case DALI_BACKWARD_TX_WAIT:
      // Wait about 10TE (between 7TE and 22TE) before sending backward frame
      sl_sleeptimer_start_timer(&te_10_sleeptimer,
                                136,
                                TE_10_callback,
                                (void *)NULL,
                                0,
                                0);
      break;

    case DALI_BACKWARD_TX_READY:
      // Send backward frame
      DMADRV_MemoryPeripheral(dmaTxChannel,
                              dmadrvPeripheralSignal_EUSART1_TXBL,
                              (void *)&(EUSART1->TXDATA),
                              &bwdData,
                              false,
                              1,
                              dmadrvDataSize1,
                              NULL,
                              NULL);
      break;

    case DALI_BACKWARD_TX_DONE:
      setDaliStatus(DALI_IDLE);
      fwdAddr = (fwdAddrData & 0xFF00) >> 8;
      fwdData = fwdAddrData & 0x00FF;
      printf("FWD RX - Address: %3d Data: %3d\n", fwdAddr, fwdData);
      printf("BWD TX - Data: %3d\n", bwdData);
      printf("\nPress 1: DALI Secondary (Idle Level %d) - Forward Frame RX\n",
             idleLevel);
      break;

    default:
      break;
  }
#endif
}
