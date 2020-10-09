/**************************************************************************//**
 * @main_s0.c
 * @brief This project uses BGX's to communicate between the computer and
 * the EFM32 starter kit via Bluetooth.
 * @version 0.0.1
 ******************************************************************************
 * @section License
 * <b>Copyright 2018 Silicon Labs, Inc. http://www.silabs.com</b>
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
 ******************************************************************************/
#include "em_device.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_usart.h"
#include "em_chip.h"
#include "em_timer.h"
#include "sl_status.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define BUFFER_SIZE 80
#define TIMER_TOP 100
#define DUTY_CYCLE 1
#define TIMER_CHANNEL 2
#define ONE_MILLISECOND_BASE_VALUE_COUNT 7
#define MILLISECOND_DIVISOR 1000
#define CONNECTION_ACTIVE_PORT gpioPortD
#define CONNECTION_ACTIVE_PIN 3
#define STREAM_ACTIVE_PORT gpioPortD
#define STREAM_ACTIVE_PIN 2

volatile uint32_t base_value = 0;
volatile bool timer3_overflow = false;
volatile uint32_t msTicks = 0;
volatile uint32_t rx_data_ready = 0;
volatile uint32_t tx_data_ready;

char rx_buffer[BUFFER_SIZE];
char tx_buffer[BUFFER_SIZE];
char string_get_time_in_ms[BUFFER_SIZE];
char rx_temp_buffer[BUFFER_SIZE];
char correct_command[] = "This command exists!";
char incorrect_command[] = "This command doesn't exist!";
char time[BUFFER_SIZE] = "The program has been running for ";
char time_unit[BUFFER_SIZE] = " milliseconds.";
volatile char last_character = '\0';
volatile uint32_t rx_counter = 0;
volatile uint32_t tx_counter = 0;
volatile bool stream_mode = false;

void TIMER3_IRQHandler(void)
{
  timer3_overflow = true;
  base_value += ONE_MILLISECOND_BASE_VALUE_COUNT;
  TIMER_IntClear(TIMER3, TIMER_IF_OF);
}

/**************************************************************************//**
 * @brief USART1 RX interrupt service routine
 *****************************************************************************/
void USART1_RX_IRQHandler(void)
{
  uint32_t flags;
  flags = USART_IntGet(USART1);
  USART_IntClear(USART1, flags);

  /* Store incoming data into rx_buffer, set rx_data_ready when a full
  * line has been received
  */
  rx_buffer[rx_counter++] = USART_Rx(USART1);

  if (stream_mode)
  {
    last_character = rx_buffer[rx_counter - 1];
    USART_IntSet(USART1, USART_IFS_TXC);
  }

  if (rx_buffer[rx_counter - 1] == '\n')
  {
    rx_data_ready = 1;
    rx_buffer[rx_counter - 2] = '\0'; // Overwrite CR or LF character
    rx_counter = 0;
  }

  if ( rx_counter >= BUFFER_SIZE - 2 )
  {
    rx_data_ready = 1;
    rx_buffer[rx_counter] = '\0'; // Do not overwrite last character
    rx_counter = 0;
  }
}

/**************************************************************************//**
 * @brief USART1 TX interrupt service routine
 *****************************************************************************/
void USART1_TX_IRQHandler(void)
{
  uint32_t flags;
  flags = USART_IntGet(USART1);
  USART_IntClear(USART1, flags);

  if (flags & USART_IF_TXC)
  {
    if (last_character != '\0' && stream_mode)
    {
      USART_Tx(USART1, last_character);
      last_character = '\0';
    }
    else if (tx_counter < BUFFER_SIZE && tx_data_ready == 1)
    {
      if (tx_buffer[tx_counter] != '\0')
      {
        USART_Tx(USART1, tx_buffer[tx_counter++]); // Transmit byte
      }
      else
      {
        tx_counter = 0; // No more data to send
        tx_data_ready = 0;
      }
    }
  }
}

uint32_t get_time_in_ms()
{
  // Clear our overflow indicator
  timer3_overflow = false;

  // Get the current count
  uint16_t count = TIMER3->CNT;

  // Get a copy of the base value
  uint32_t copy_of_base_value = base_value;

  // Now check to make sure that the ISR didn't fire while in here
  // If it did, then grab the values again
  if (timer3_overflow)
  {
    count = TIMER3->CNT;
    copy_of_base_value = base_value;
  }

  // Now calculate the number of milliseconds the program has run
  return copy_of_base_value + count / MILLISECOND_DIVISOR;
}

void delay_ms(uint32_t milliseconds)
{
  int32_t time_in_ms2 = get_time_in_ms();
  uint32_t trigger_time = time_in_ms2 + milliseconds;
  while (get_time_in_ms() < trigger_time);
}

int32_t set_ms_timeout(int32_t timeout_ms)
{
  return get_time_in_ms() + timeout_ms;
}

bool expired_ms(int32_t timeout_ms)
{
  int32_t time_in_ms = get_time_in_ms();
  if (timeout_ms < time_in_ms)
  {
    return true;
  }
  return false;
}

void timer_init(void)
{
  CMU_ClockEnable(cmuClock_TIMER3, true);

  // Create the timer count control object initializer
  TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;
  timerCCInit.mode = timerCCModePWM;
  timerCCInit.cmoa = timerOutputActionToggle;

  // Configure CC channel 2
  TIMER_InitCC(TIMER3, TIMER_CHANNEL, &timerCCInit);

  // Route CC2 to location 1 (PE3) and enable pin for cc2
  TIMER3->ROUTE |= (TIMER_ROUTE_CC2PEN | TIMER_ROUTE_LOCATION_LOC1);

  // Set Top Value
  TIMER_TopSet(TIMER3, TIMER_TOP);

  // Set the PWM duty cycle here!
  TIMER_CompareBufSet(TIMER3, TIMER_CHANNEL, DUTY_CYCLE);

  // Create a timerInit object, based on the API default
  TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
  timerInit.prescale = timerPrescale1024;

  TIMER_Init(TIMER3, &timerInit);

  TIMER_IntEnable(TIMER3, TIMER_IEN_OF);
  NVIC_EnableIRQ(TIMER3_IRQn);
}

void uart_init(void)
{
  USART_InitAsync_TypeDef init = USART_INITASYNC_DEFAULT;

  // Enable oscillator to GPIO and USART1 modules
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(cmuClock_USART1, true);

  // set pin modes for USART TX and RX pins
  GPIO_PinModeSet(gpioPortD, 1, gpioModeInput, 0);
  GPIO_PinModeSet(gpioPortD, 0, gpioModePushPull, 1);

  // Initialize USART asynchronous mode and route pins
  USART_InitAsync(USART1, &init);
  USART1->ROUTE |= USART_ROUTE_TXPEN | USART_ROUTE_RXPEN | USART_ROUTE_LOCATION_LOC1;

  // Initialize USART Interrupts
  USART_IntEnable(USART1, USART_IEN_RXDATAV);
  USART_IntEnable(USART1, USART_IEN_TXC);

  // Enabling USART Interrupts
  NVIC_EnableIRQ(USART1_RX_IRQn);
  NVIC_EnableIRQ(USART1_TX_IRQn);

  GPIO_PinModeSet(CONNECTION_ACTIVE_PORT, CONNECTION_ACTIVE_PIN, gpioModeInput, 1);
  GPIO_PinModeSet(STREAM_ACTIVE_PORT, STREAM_ACTIVE_PIN, gpioModeInput, 1);
}

void bgx_breakout_sequence(char breakout_string[], bool next_line)
{
  while (tx_data_ready == 1);
  int i = 0;
  delay_ms(500);
  USART_IntDisable(USART1, USART_IEN_RXDATAV);
  USART_IntDisable(USART1, USART_IEN_TXC);

  for (i = 0; breakout_string[i] != 0 && i < BUFFER_SIZE-3; i++)
  {
    tx_buffer[i] = breakout_string[i];
  }

  tx_buffer[i] = '\0';
  tx_data_ready = 1;

  USART_IntEnable(USART1, USART_IEN_RXDATAV);
  USART_IntEnable(USART1, USART_IEN_TXC);
  USART_IntSet(USART1, USART_IFS_TXC);
  delay_ms(500);
}

void bgx_transmit_data(char temp_buffer[], bool next_line)
{

  while (tx_data_ready == 1);
  int i = 0;
  USART_IntDisable(USART1, USART_IEN_RXDATAV);
  USART_IntDisable(USART1, USART_IEN_TXC);

  for (i = 0; temp_buffer[i] != 0 && i < BUFFER_SIZE-3; i++)
  {
    tx_buffer[i] = temp_buffer[i];
  }

  if (next_line == true)
  {
    tx_buffer[i++] = '\r';
    tx_buffer[i++] = '\n';
    tx_buffer[i] = '\0';
    tx_data_ready = 1;
  }
  else if (next_line == false)
  {
    tx_buffer[i] = '\0';
    tx_data_ready = 1;
  }

  USART_IntEnable(USART1, USART_IEN_RXDATAV);
  USART_IntEnable(USART1, USART_IEN_TXC);
  USART_IntSet(USART1, USART_IFS_TXC);
}

sl_status_t bgx_transmit_command(char cmd_buffer[], bool next_line, bool connected)
{
  if(GPIO_PinInGet(CONNECTION_ACTIVE_PORT, CONNECTION_ACTIVE_PIN) == !connected)
  {
    return SL_STATUS_FAIL;
  }

  if(GPIO_PinInGet(STREAM_ACTIVE_PORT, STREAM_ACTIVE_PIN))
  {
    return SL_STATUS_FAIL;
  }

  while (tx_data_ready == 1);
  int i = 0;
  rx_buffer[i] = '\0';
  USART_IntDisable(USART1, USART_IEN_RXDATAV);
  USART_IntDisable(USART1, USART_IEN_TXC);

  for (i = 0; cmd_buffer[i] != 0 && i < BUFFER_SIZE-3; i++)
  {
    tx_buffer[i] = cmd_buffer[i];
  }

  if (next_line == true)
  {
    tx_buffer[i++] = '\r';
    tx_buffer[i++] = '\n';
    tx_buffer[i] = '\0';
    tx_data_ready = 1;
  }
  else if (next_line == false)
  {
    tx_buffer[i] = '\0';
    tx_data_ready = 1;
  }

  USART_IntEnable(USART1, USART_IEN_RXDATAV);
  USART_IntEnable(USART1, USART_IEN_TXC);
  USART_IntSet(USART1, USART_IFS_TXC);

  uint32_t time = set_ms_timeout(10000);
  while (expired_ms(time) == false)
  {
    delay_ms(20);
    if(strcmp("Success", rx_buffer) == 0)
    {
      return SL_STATUS_OK;
    }
  }
  return SL_STATUS_FAIL;
}


/**************************************************************************//**
 * @brief Main function
 *****************************************************************************/
int main(void)
{
  sl_status_t status;

  // Chip errata
  CHIP_Init();
  timer_init();
  uart_init();

  tx_data_ready = 0;

  status = bgx_transmit_command("scan", true, false);
  // Wait for the BGX connection to finalize
  delay_ms(20);
  status = bgx_transmit_command("con 1", true, false);
  delay_ms(20);
  stream_mode = true;

  while(1)
  {
    if (rx_data_ready == 1 && stream_mode)
      {
        USART_IntDisable(USART1, USART_IEN_RXDATAV);
        USART_IntDisable(USART1, USART_IEN_TXC);

        strcpy(rx_temp_buffer, rx_buffer);

        if (strcmp("get time", rx_temp_buffer) == 0)
        {
          __itoa(get_time_in_ms(), string_get_time_in_ms, 10);
          bgx_transmit_data(time, false);
          bgx_transmit_data(string_get_time_in_ms, false);
          bgx_transmit_data(time_unit, false);
          bgx_transmit_data("", true);
        }
        else if (strcmp("turn led off", rx_temp_buffer) == 0)
        {
          stream_mode = false;
          bgx_breakout_sequence("$$$", true);
          bgx_transmit_data("", true);
          status = bgx_transmit_command("gse 1 1", true, true);
          bgx_transmit_data("str", true);
          stream_mode = true;
        }
        else if (strcmp("turn led on", rx_temp_buffer) == 0)
        {
          stream_mode = false;
          bgx_breakout_sequence("$$$", true);
          bgx_transmit_data("", true);
          status = bgx_transmit_command("gse 1 0", true, true);
          bgx_transmit_data("str", true);
          stream_mode = true;
        }
        else
        {
          bgx_transmit_data(incorrect_command, true);
        }

        rx_data_ready = 0;

        USART_IntEnable(USART1, USART_IEN_RXDATAV);
        USART_IntEnable(USART1, USART_IEN_TXC);
        USART_IntSet(USART1, USART_IFS_TXC);
      }
  }
}

