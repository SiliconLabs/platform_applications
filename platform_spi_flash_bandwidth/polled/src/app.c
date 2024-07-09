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

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
#include "em_cmu.h"
#include "sl_sleeptimer.h"
#include "app_log.h"
#include "stdint.h"
#include "em_usart.h"
#include "em_gpio.h"

#define SPIPORT   gpioPortC
#define MOSIPIN   0
#define MISOPIN   1
#define SCLKPIN   2
#define CSPORT    gpioPortC
#define CSPIN     3

// SPI flash commands
#define M25X_READ 0x3

// Transfer count
#define BYTECOUNT 1048576

// Globals for tick count
static volatile uint32_t msTicks;
static uint32_t msTicks_start, msTicks_end;

static sl_sleeptimer_timer_handle_t app_timer;
static void app_timer_callback(sl_sleeptimer_timer_handle_t *timer, void *data);

/***************************************************************************//**
 * App timer callback function.
 ******************************************************************************/
static void app_timer_callback(sl_sleeptimer_timer_handle_t *timer, void *data)
{
  (void)timer;
  (void)data;

  msTicks++;  // Increment milliseconds tick counter
}

/***************************************************************************//**
 * App capture start time function.
 ******************************************************************************/
void app_time_sync(void)
{
  msTicks_end = msTicks;

  while (msTicks_end == msTicks) {  // msTicks Sync
  }
  msTicks_start = msTicks;
}

/***************************************************************************//**
 * App SPI intialization function.
 ******************************************************************************/
void app_spi_init(void)
{
  USART_InitSync_TypeDef initSpi = USART_INITSYNC_DEFAULT;
  initSpi.baudrate = CMU_ClockFreqGet(cmuClock_USART2) / 2;
  initSpi.msbf = true;
  USART_InitSync(USART2, &initSpi);

  /*
   * Enable synchronous master sample delay in order to sample data on
   * the next setup edge as this is required to meet timing both on the
   * EFR32 side of the system (delays due to pin muxing) and on the SPI
   * flash, too (long output valid, tV or tCLQV, times on some devices,
   * at least at higher clock rates).
   */
  USART2->CTRL_SET = USART_CTRL_SMSDELAY;

  /*
   * Use fastest slew rate on port C pins to get cleaner edges; keep
   * alternate slew rate at port default.
   */
  GPIO_SlewrateSet(SPIPORT, 7, 4);

  // Configure pins for SPI master mode
  GPIO_PinModeSet(SPIPORT, MOSIPIN, gpioModePushPull, 0);    // MOSI/TX: PC0,
                                                             //   EXP header pin
                                                             //   4
  GPIO_PinModeSet(SPIPORT, MISOPIN, gpioModeInput, 0);       // MISO/RX: PC1,
                                                             //   EXP header pin
                                                             //   6
  GPIO_PinModeSet(SPIPORT, SCLKPIN, gpioModePushPull, 0);    // CLK: PC2, EXP
                                                             //   header pin 8
  GPIO_PinModeSet(CSPORT, CSPIN, gpioModePushPull, 1);        // CS: PC3, EXP
                                                              //   header pin 10

  // Route MOSI, MISO, and CLK signals to port C pins; CS remains a discrete
  GPIO->USARTROUTE[2].TXROUTE = (SPIPORT << _GPIO_USART_TXROUTE_PORT_SHIFT)
                                | (MOSIPIN << _GPIO_USART_TXROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[2].RXROUTE = (SPIPORT << _GPIO_USART_RXROUTE_PORT_SHIFT)
                                | (MISOPIN << _GPIO_USART_RXROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[2].CLKROUTE = (SPIPORT << _GPIO_USART_CLKROUTE_PORT_SHIFT)
                                 | (SCLKPIN << _GPIO_USART_CLKROUTE_PIN_SHIFT);

  // Enable USART2 signals at the pins
  GPIO->USARTROUTE[2].ROUTEEN = GPIO_USART_ROUTEEN_TXPEN      // MOSI
                                | GPIO_USART_ROUTEEN_RXPEN    // MISO
                                | GPIO_USART_ROUTEEN_CLKPEN;
}

/***************************************************************************//**
 * App main task function.
 ******************************************************************************/
void app_main_task(void)
{
  int readCount, flashAddr = 0, rate;
  uint32_t data;

  app_log("Start transfer data ... \n");
  sl_sleeptimer_start_periodic_timer_ms(&app_timer,
                                        1,
                                        app_timer_callback,
                                        NULL,
                                        0,
                                        0);
  app_time_sync();
  USART_Tx(USART2, M25X_READ);

  /*
   * Send bytes 1, 2, and 3 of 32-bit address.  Byte 0 should always
   * be 0 unless the device in question has a 32-bit address space, in
   * which case byte 0 needs to be sent, too.
   */
  USART_Tx(USART2, *((uint8_t *)&flashAddr + 1));
  USART_Tx(USART2, *((uint8_t *)&flashAddr + 2));
  USART_Tx(USART2, *((uint8_t *)&flashAddr + 3));

  for (readCount = BYTECOUNT; readCount > 0; readCount--) {
    data = USART_SpiTransfer(USART2, 0x0);
  }

  (void)data;

  // End timing
  msTicks_end = msTicks;
  sl_sleeptimer_stop_timer(&app_timer);

  // Display transfer rate
  rate = (BYTECOUNT / (msTicks_end - msTicks_start));
  app_log("USART2 clock = %u MHz\n",
          (unsigned int)(CMU_ClockFreqGet(cmuClock_USART2) / 1000000));
  app_log("Done: %u bytes in %u ms = %u KB/sec\n", BYTECOUNT,
          (unsigned int)(msTicks_end - msTicks_start), rate);
}

/***************************************************************************//**
 * App init function.
 ******************************************************************************/
void app_init(void)
{
  app_log("\nPlatform - EFR32xG21 Polled SPI Throughput Example\n\n");

  app_spi_init();
  app_main_task();
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
    // Nothing
}
