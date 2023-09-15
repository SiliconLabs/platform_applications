/***************************************************************************//**
 * @file dali_define.h
 * @brief Header file for DALI defines.
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

#ifndef DALI_DEFINE_H
#define DALI_DEFINE_H

#define GLUE_DEF(x, y, z)       x ## y ## z
#define GLUE(x, y, z)           GLUE_DEF(x, y, z)

#if defined(_SILICON_LABS_32B_SERIES_2)
// TX PRS
#define TX_PRS_SRC      GLUE(PRS_ASYNC_CH_CTRL_SOURCESEL_USART, SPI_USART_NUM, )
#define TX_PRS_SIG      GLUE(PRS_ASYNC_CH_CTRL_SIGSEL_USART, SPI_USART_NUM, TXC)

// RX pin PRS
#define PIN_PRS_SRC     PRS_ASYNC_CH_CTRL_SOURCESEL_GPIO
#define PIN_PRS_SIG     GLUE(PRS_ASYNC_CH_CTRL_SIGSEL_GPIOPIN, DALI_RX_PIN, )

// RX TIMER PRS
#define TIMER_PRS_SRC   GLUE(PRS_ASYNC_CH_CTRL_SOURCESEL_TIMER, DALI_TIMER_NUM, \
                             )
#define TIMER_PRS_SIG   GLUE(PRS_ASYNC_CH_CTRL_SIGSEL_TIMER, DALI_TIMER_NUM, OF)

// RX DMA source
#define PRS_REQ_SRC     GLUE(LDMAXBAR_CH_REQSEL_SIGSEL_LDMAXBARPRSREQ, \
                             DMAREQ_NUM,                               \
                             ) | LDMAXBAR_CH_REQSEL_SOURCESEL_LDMAXBAR
#define PRS_DMA_REQ     GLUE(prsConsumerLDMA_REQUEST, DMAREQ_NUM, )
#else
// TX PRS
#define TX_PRS_SRC      GLUE(PRS_CH_CTRL_SOURCESEL_USART, SPI_USART_NUM, )
#define TX_PRS_SIG      GLUE(PRS_CH_CTRL_SIGSEL_USART, SPI_USART_NUM, TXC)

// RX pin PRS
#if (DALI_RX_PIN > 8)
#define PIN_PRS_SRC     PRS_CH_CTRL_SOURCESEL_GPIOH
#else
#define PIN_PRS_SRC     PRS_CH_CTRL_SOURCESEL_GPIOL
#endif
#define PIN_PRS_SIG     GLUE(PRS_CH_CTRL_SIGSEL_GPIOPIN, DALI_RX_PIN, )

// RX TIMER PRS
#define TIMER_PRS_SRC   GLUE(PRS_CH_CTRL_SOURCESEL_TIMER, DALI_TIMER_NUM, )
#define TIMER_PRS_SIG   GLUE(PRS_CH_CTRL_SIGSEL_TIMER, DALI_TIMER_NUM, OF)

// RX DMA source
#define PRS_REQ_SRC     GLUE(ldmaPeripheralSignal_PRS_REQ, DMAREQ_NUM, )
#define PRS_DMA_REQ     GLUE(PRS_DMAREQ, DMAREQ_NUM, _PRSSEL_PRSCH)
#define PRS_REQ_REG     GLUE(DMAREQ, DMAREQ_NUM, )
#define PRS_DMA_CH      GLUE(PRS_DMA_REQ, TIMER_PRS_CH, )
#endif

#if !defined(DALI_SECONDARY)
// DALI status
typedef enum {
  DALI_IDLE,
  DALI_FORWARD_TX,
  DALI_DATA_RX_TIMEOUT,
  DALI_BACKWARD_RX,
  DALI_BACKWARD_RX_WAIT,
  DALI_BACKWARD_RX_TIMEOUT
} DaliStatus_t;

// TX DMA descriptor and buffer size
#define TX_DESC_SIZE    1
#define TX_BUFFER_SIZE  8

// RX buffer size
#define RX_BUFFER_SIZE  22

// RX pin DMA descriptor size
#if defined(_SILICON_LABS_32B_SERIES_2)
#if (IDLE_LEVEL == 0)
#define PIN_DESC_SIZE   8
#else
#define PIN_DESC_SIZE   10
#endif
#else
#if (IDLE_LEVEL == 0)
#define PIN_DESC_SIZE   7
#else
#define PIN_DESC_SIZE   9
#endif
#endif

// Start and stop location of stop bit
#define RX_BIT_START    (16 + RX_START)
#define RX_BIT_STOP     (16 + RX_START_STOP)
#else
// DALI status
typedef enum {
  DALI_IDLE,
  DALI_FORWARD_RX_WAIT,
  DALI_FORWARD_RX,
  DALI_DATA_RX_TIMEOUT,
  DALI_BACKWARD_TX_WAIT,
  DALI_BACKWARD_TX_READY,
  DALI_BACKWARD_TX,
  DALI_BACKWARD_TX_DONE
} DaliStatus_t;

// TX DMA descriptor and buffer size
#define TX_DESC_SIZE    3
#define TX_BUFFER_SIZE  3

// RX buffer size
#define RX_BUFFER_SIZE  38

// RX pin DMA descriptor size
#if defined(_SILICON_LABS_32B_SERIES_2)
#define PIN_DESC_SIZE   5
#else
#define PIN_DESC_SIZE   4
#endif

// Start and stop location of stop bit
#define RX_BIT_START    (32 + RX_START)
#define RX_BIT_STOP     (32 + RX_START_STOP)
#endif

#if (IDLE_LEVEL == 0)
// TX idle state level
#define TX_IDLE_OUTPUT  0x00

// TX constants for start and stop bit
#define START_NIBBLE    0x20
#define STOP_NIBBLE     0x00

// RX constants for decode
#define HIGH_MSB        1
#define HIGH_LSB        0
#define STOP_LEVEL      0
#else
// TX idle state level
#define TX_IDLE_OUTPUT  0xff

// TX constants for start and stop bit
#define START_NIBBLE    0xd0
#define STOP_NIBBLE     0x0f

// RX constants for decode
#define HIGH_MSB        0
#define HIGH_LSB        1
#define STOP_LEVEL      1
#endif

// TX parameters
#define SPI_BAUDRATE    1200
#define SPI_USART       GLUE(USART, SPI_USART_NUM, )
#define SPI_USART_CLK   GLUE(cmuClock_USART, SPI_USART_NUM, )

// TX DMA source
#define SPI_DMA_TXREQ   GLUE(ldmaPeripheralSignal_USART, SPI_USART_NUM, _TXBL)

// TX constants for right and left shift
#define RIGHT_SHIFT0    12
#define RIGHT_SHIFT1    4
#define LEFT_SHIFT0     4

// RX pin register
#define DALI_RX_DIN     GPIO->P[DALI_RX_PORT].DIN

// RX TIMER, 2T = 1/1200
#define DALI_TIMER      GLUE(TIMER, DALI_TIMER_NUM, )
#define DALI_TIMER_CLK  GLUE(cmuClock_TIMER, DALI_TIMER_NUM, )

// RX timeout TIMER
#define TO_TIMER        GLUE(TIMER, TO_TIMER_NUM, )
#define TO_TIMER_CLK    GLUE(cmuClock_TIMER, TO_TIMER_NUM, )
#define TO_TIMER_ISR    GLUE(TIMER, TO_TIMER_NUM, _IRQHandler)
#define TO_TIMER_IRQn   GLUE(TIMER, TO_TIMER_NUM, _IRQn)

#define DALI_ONE_T      (DALI_HALF_T * 2) + 1

// RX DMA SYNC trigger values
#define SYNC_RXPIN      (1 << PIN_PRS_CH)
#define SYNC_TXC        (1 << TX_PRS_CH)

// RX TIMER DMA descriptor size
#define TMR_DESC_SIZE   2

// Number of start and stop bits
#define RX_START        2
#define RX_START_STOP   6

// Function prototypes
void initDali(void);
void startDaliTxDma(uint8_t addr, uint8_t data);

void initDaliRxPrs(void);
void initDaliRxTimer(void);
void startDaliRxDma(void);
bool decodeDaliRx(uint8_t *addr, uint8_t *data);

// External variable and constants
extern DaliStatus_t daliStatus;
extern const uint16_t manchesterEncodeTable[256];
extern const uint8_t manchesterDecodeTable[256];
#endif // DALI_DEFINE_H
