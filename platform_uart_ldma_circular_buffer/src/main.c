/***************************************************************************//**
 * @file main.c
 * @brief GG11 UART RX with ciruclar buffer using LDMA
 * @version 1.0.0
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

#include <stdint.h>
#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_ldma.h"
#include "em_usart.h"

#define RX_PORT gpioPortC
#define RX_PIN  1
#define TX_PORT gpioPortC
#define TX_PIN  0

#define BUFFER_SIZE 20

struct circular_buffer
{
  uint8_t buffer[BUFFER_SIZE];
  int start_index;
  int stop_index;
} rx_buffer;

LDMA_Descriptor_t desc_link[2];
int link_desc_num;

/***************************************************************************//**
 * @brief  Init UART
 ******************************************************************************/
void initUART(void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(cmuClock_USART0, true);

  GPIO_PinModeSet(RX_PORT, RX_PIN, gpioModeInput, 0);
  GPIO_PinModeSet(TX_PORT, TX_PIN, gpioModePushPull, 1);

  USART_InitAsync_TypeDef init = USART_INITASYNC_DEFAULT;
  USART_InitAsync(USART0, &init);

  USART0->ROUTELOC0 = USART_ROUTELOC0_RXLOC_LOC5 | USART_ROUTELOC0_TXLOC_LOC5;
  USART0->ROUTEPEN |= USART_ROUTEPEN_TXPEN | USART_ROUTEPEN_RXPEN;

  USART_Enable(USART0, usartEnable);
}

/***************************************************************************//**
 * @brief  LDMA IRQ Handler moves the circular buffer stop index
 ******************************************************************************/
void LDMA_IRQHandler(void)
{
  uint32_t pending = LDMA_IntGet();

  LDMA_IntClear(pending);
  if(pending & LDMA_IF_ERROR) {
    while(1);
  }

  if(link_desc_num == 0)
  {
    link_desc_num = 1;
    rx_buffer.stop_index = BUFFER_SIZE/2-1;
  }
  else
  {
    link_desc_num = 0;
    rx_buffer.stop_index = BUFFER_SIZE-1;
  }
}

/***************************************************************************//**
 * @brief  Init LDMA
 ******************************************************************************/
void initLDMA(void)
{
  CMU_ClockEnable(cmuClock_LDMA, true);

  LDMA_Init_t init = LDMA_INIT_DEFAULT;
  LDMA_Init(&init);

  LDMA_TransferCfg_t config = LDMA_TRANSFER_CFG_PERIPHERAL(ldmaPeripheralSignal_USART0_RXDATAV);

  desc_link[0] = (LDMA_Descriptor_t)LDMA_DESCRIPTOR_LINKREL_P2M_BYTE(
      &(USART0->RXDATA),
      rx_buffer.buffer,
      BUFFER_SIZE/2,
      1);
  desc_link[1] = (LDMA_Descriptor_t)LDMA_DESCRIPTOR_LINKREL_P2M_BYTE(
      &(USART0->RXDATA),
      rx_buffer.buffer + (BUFFER_SIZE/2),
      BUFFER_SIZE/2,
      -1);

  desc_link[0].xfer.doneIfs = true;
  desc_link[1].xfer.doneIfs = true;

  LDMA_StartTransfer(0, &config, &(desc_link[0]));
}

/***************************************************************************//**
 * @brief  Main function
 ******************************************************************************/
int main(void)
{
  CHIP_Init();

  link_desc_num = 0;
  rx_buffer.start_index = 0;
  rx_buffer.stop_index = 0;

  initUART();
  initLDMA();

  while (true) {
    EMU_EnterEM1();
  }
}
