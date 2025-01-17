/**************************************************************************//**
 * @file usart.c
 * @brief Functions to send and receive data over USART
 * @author Silicon Labs
 * @version x.xx
 ******************************************************************************
 * @section License
 * <b>(C) Copyright 2014 Silicon Labs, http://www.silabs.com</b>
 *******************************************************************************
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Labs has no
 * obligation to support this Software. Silicon Labs is providing the
 * Software "AS IS", with no express or implied warranties of any kind,
 * including, but not limited to, any implied warranties of merchantability
 * or fitness for any particular purpose or warranties against infringement
 * of any proprietary rights of a third party.
 *
 * Silicon Labs will not be liable for any consequential, incidental, or
 * special damages, or any other relief, or for any claim by any third party,
 * arising from your use of this Software.
 *
 ******************************************************************************/
#include "em_device.h"
#include "em_chip.h"
#include "em_usart.h"
#include "em_cmu.h"
#include "em_core.h"
#include "em_gpio.h"
#include "config.h"
#include "logging.h"
#include "motor.h"
#include "pid.h"
#include "uart.h"
#include "em_gpio.h"
#define TX_BUFFER_SIZE 100

/* TX ring buffer */
static uint8_t txBuffer[TX_BUFFER_SIZE];
static int start = 0;
static int end = 0;

/**********************************************************
 * Push a byte onto the TX buffer.
 * This is used by the application to schedule data
 * to be sent over USART.
 **********************************************************/
void push(uint8_t b)
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();

  /* Avoid overflow */
  if ((end - start) % TX_BUFFER_SIZE == (TX_BUFFER_SIZE - 1)) {
    return;
  }

  txBuffer[end] = b;
  if (++end >= TX_BUFFER_SIZE) {
    end = 0;
  }

  /* Turn on TXBL interrupt to start sending data */
  USART0->IEN |= USART_IEN_TXBL;
  CORE_EXIT_CRITICAL();
}

/**********************************************************
 * Returns and removes the first byte in the TX buffer.
 * This is used by the USART TX interrupt handler to fetch
 * bytes to send.
 **********************************************************/
bool pop(uint8_t *b)
{
  if (end - start == 0) {
    return false;
  }

  *b = txBuffer[start];

  if (++start >= TX_BUFFER_SIZE) {
    start = 0;
  }

  return true;
}

/************************************************
 * Initializes USART. Enables clocks, configures
 * baudrate and pins and enables interrupts.
 ************************************************/
void uartInit(void)
{
  CMU_ClockEnable(cmuClock_USART0, true);
  CMU_ClockEnable(cmuClock_GPIO, true);

#ifdef VCOM_ENABLE
  GPIO_PinModeSet(VCOM_ENABLE_PORT, VCOM_ENABLE_PIN, gpioModePushPull, 1);
#endif
  GPIO_PinModeSet(UART_TX_PORT, UART_TX_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(UART_RX_PORT, UART_RX_PIN, gpioModeInput, 0);

  GPIO->USARTROUTE[0].TXROUTE =
    (UART_TX_PORT <<
      _GPIO_USART_TXROUTE_PORT_SHIFT)
    | (UART_TX_PIN << _GPIO_USART_TXROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[0].RXROUTE =
    (UART_RX_PORT <<
      _GPIO_USART_RXROUTE_PORT_SHIFT)
    | (UART_RX_PIN << _GPIO_USART_RXROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[0].ROUTEEN = GPIO_USART_ROUTEEN_RXPEN
                                | GPIO_USART_ROUTEEN_TXPEN;

  USART_InitAsync_TypeDef usartInit = USART_INITASYNC_DEFAULT;
  usartInit.baudrate = UART_BAUDRATE;
  USART_InitAsync(USART0, &usartInit);

  USART0->IEN = USART_IEN_RXDATAV;
  NVIC_EnableIRQ(USART0_RX_IRQn);
  NVIC_EnableIRQ(USART0_TX_IRQn);

  USART0->CMD = USART_CMD_CLEARTX;
}

/************************************************
 * Command handler for input from PC tool.
 * This function receives commands from the
 * PC tool over USART and executes them.
 *
 * @param b
 *    The last byte received over USART
 *
 ************************************************/
void cmdHandler(uint8_t b)
{
  static int curCmd = 0;
  static uint8_t curData[12];
  static int curIndex = 0;

  if (curCmd == 0) {
    switch (b) {
      /* Handle commands with no data */
      case CMD_START:
        /*'A'*/
        startMotor();
        break;
      case CMD_STOP:
        /*'B'*/
        stopMotor();
        break;
      case CMD_GET_VER:
        /*'D'*/
        sendVersion();
        break;
      case CMD_CHANGE_DIR:
        /*'F'*/
        setDirection(!getDirection());
        break;

      /* Commands with data payload needs to
       * receive the entire payload before they
       * can be executed */
      case CMD_SET_SETPOINT:
      case CMD_SET_PID:
        curCmd = b;
        curIndex = 0;
        break;
    }
  } else {
    switch (curCmd) {
      /* Handle commands with a data payload.
       * Save each byte until the entire payload is
       * received, then execute the command */
      case CMD_SET_SETPOINT:
        curData[curIndex++] = b;
        if (curIndex == 4) {
          uint32_t *p = (uint32_t *)&curData;
          setSpeed(*p);
          curCmd = 0;
        }
        break;
      case CMD_SET_PID:
        curData[curIndex++] = b;
        if (curIndex == 12) {
          float *pKp = (float *)curData;
          float *pKi = (float *)(curData + 4);
          float *pKd = (float *)(curData + 8);

          pidSetCoefficients(*pKp, *pKi, *pKd);
          curCmd = 0;
        }
        break;
    }
  }
}

/************************************************
 * Sends the current firmware version.
 ************************************************/
void sendVersion(void)
{
  uartSendByte('a');
  uartSendByte('b');
  uartSendByte('d');
  uartSendByte('E');
  pidSendLog();
}

/************************************************
 * USART TX handler
 ************************************************/
void USART0_TX_IRQHandler(void)
{
  uint8_t b;

  if (pop(&b)) {
    USART0->TXDATA = b;
  } else {
    /* Buffer is empty. Turn off TXBL */
    USART0->IEN &= ~USART_IEN_TXBL;
  }
}

/************************************************
 * USART RX handler
 ************************************************/
void USART0_RX_IRQHandler(void)
{
  cmdHandler(USART0->RXDATA);
}

/************************************************
 * Send a byte over USART
 ************************************************/
void uartSendByte(uint8_t byte)
{
  push(byte);
}

/************************************************
 * Send a 16-bit integer, least significant byte
 * first.
 ************************************************/
void uartSendNumber(int16_t n)
{
  push(n & 0x00FF);
  push((n >> 8) & 0x00FF);
}
