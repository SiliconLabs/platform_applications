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
#include "dmadrv.h"
#include "em_ldma.h"
#include "em_usart.h"
#include "stddef.h"
#include "sl_sleeptimer.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "app_log.h"

#define SPIPORT gpioPortC
#define MOSIPIN   0
#define MISOPIN   1
#define SCLKPIN   2
#define CSPORT    gpioPortC
#define CSPIN     3

// SPI flash command
#define M25X_READ 0x03

// Transfer count
#define BYTECOUNT 1024
#define LOOPCOUNT 1024

static unsigned int spiRxChan;
static unsigned int spiTxChan;

// Block receive completion flag
static volatile bool rxDone = false;
static volatile bool txDone = false;

// Dummy buffers for SPI transmit/receive
static uint8_t dummyOut[1] = { 0x0 };
static uint8_t dummyIn[BYTECOUNT] = { 0x0 };

// Globals for tick count
static volatile uint32_t msTicks;
static uint32_t msTicks_start, msTicks_end;

static sl_sleeptimer_timer_handle_t app_timer;

static LDMA_Descriptor_t ldmaTXdesc;
static LDMA_TransferCfg_t ldmaTXcfg;

static LDMA_Descriptor_t ldmaRXdesc;
static LDMA_TransferCfg_t ldmaRXcfg;


static void app_timer_callback(sl_sleeptimer_timer_handle_t *timer, void* data);
static bool dma_rx_callback(unsigned int channel,
                              unsigned int sequenceNo,
                              void *userParam);
static bool dma_tx_callback(unsigned int channel,
                              unsigned int sequenceNo,
                              void *userParam);

/***************************************************************************//**
 * DMA RX complete function.
 ******************************************************************************/
bool dma_rx_callback(unsigned int channel,
                              unsigned int sequenceNo,
                              void *userParam)
{
  (void)channel;
  (void)sequenceNo;
  (void)userParam;

  rxDone = true;

  return true;
}

/***************************************************************************//**
 * DMA TX complete callback function.
 ******************************************************************************/
bool dma_tx_callback(unsigned int channel,
                              unsigned int sequenceNo,
                              void *userParam)
{
  (void)channel;
  (void)sequenceNo;
  (void)userParam;

  txDone = true;

  return true;
}

/***************************************************************************//**
 * App timer callback function.
 ******************************************************************************/
static void app_timer_callback(sl_sleeptimer_timer_handle_t *timer, void* data)
{
  (void)timer;
  (void)data;

  msTicks++;  // Increment milliseconds tick counter

}

/***************************************************************************//**
 * Set start point function.
 ******************************************************************************/
void app_time_sync(void)
{
  msTicks_end = msTicks;

  while(msTicks_end == msTicks);    //msTicks Sync

  msTicks_start = msTicks;
}

/***************************************************************************//**
 * App DMA initialization function.
 ******************************************************************************/
void app_dma_init(void)
{
  Ecode_t ecode;

  ecode = DMADRV_Init();
  if ((ecode != ECODE_OK)
      && (ecode != ECODE_EMDRV_DMADRV_ALREADY_INITIALIZED)) {
      app_log("DMA initalized failed!\n");
    return;
  }
  /*
   * If transmitting an actual block, dummyOut would be replaced with
   * an array with actual transmit data.  Further, there would be no
   * need to change the transfer source increment to none.  We'd want
   * the LDMA to increment the source pointer by one with each byte,
   * which is the default when the LDMA_DESCRIPTOR_SINGLE_M2P_BYTE()
   * initializer is used.
   *
   * In the case of reading from a SPI flash, ldmaCtrlSrcIncNone is
   * useful because we just want to pump dummy data into the TX buffer
   * to clock in the READ data.  Sending the READ command and the
   * address are done separately.
   *
   * It's probably not worth using the LDMA to transfer just those four
   * bytes, although it can be done with its own descriptor and then
   * linked to the descriptor that generates the dummy transfers as a
   * means of using LDMA for the entire READ operation.
   */
  ldmaTXdesc = (LDMA_Descriptor_t)LDMA_DESCRIPTOR_SINGLE_M2P_BYTE(dummyOut, &(USART2->TXDATA), BYTECOUNT);
  ldmaTXdesc.xfer.srcInc = ldmaCtrlSrcIncNone;
  ldmaTXcfg = (LDMA_TransferCfg_t)LDMA_TRANSFER_CFG_PERIPHERAL(ldmaPeripheralSignal_USART2_TXBL);

  /*
   * This descriptor setup is the reverse of what was used above.
   * Of course, when reading actual data from the SPI flash, the input
   * buffer would have some pre-allocated size, and there would be no
   * reason to change the destination increment to none because
   * LDMA_DESCRIPTOR_SINGLE_P2M_BYTE() defaults to increment by one for
   * peripheral to memory byte transfers.
   */
  ldmaRXdesc = (LDMA_Descriptor_t)LDMA_DESCRIPTOR_SINGLE_P2M_BYTE(&(USART2->RXDATA), dummyIn, BYTECOUNT);
  ldmaRXdesc.xfer.dstInc = ldmaCtrlDstIncNone;
  ldmaRXcfg = (LDMA_TransferCfg_t)LDMA_TRANSFER_CFG_PERIPHERAL(ldmaPeripheralSignal_USART2_RXDATAV);

  // Allocate DMA channel for TX
  ecode = DMADRV_AllocateChannel(&spiTxChan, NULL);
  if (ecode != ECODE_EMDRV_DMADRV_OK) {
    return;
  }

  // Allocate DMA channel for RX
  ecode = DMADRV_AllocateChannel(&spiRxChan, NULL);
  if (ecode != ECODE_EMDRV_DMADRV_OK) {
    return;
  }
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
  GPIO_PinModeSet (SPIPORT, MOSIPIN, gpioModePushPull, 0);   // MOSI/TX: PC0, EXP header pin 4
  GPIO_PinModeSet (SPIPORT, MISOPIN, gpioModeInput, 0);      // MISO/RX: PC1, EXP header pin 6
  GPIO_PinModeSet (SPIPORT, SCLKPIN, gpioModePushPull, 0);   // CLK: PC2, EXP header pin 8
  GPIO_PinModeSet (CSPORT, CSPIN, gpioModePushPull, 1);       // CS: PC3, EXP header pin 10

  // Route MOSI, MISO, and CLK signals to port C pins; CS remains a discrete
  GPIO->USARTROUTE[2].TXROUTE = (SPIPORT << _GPIO_USART_TXROUTE_PORT_SHIFT)
      | (MOSIPIN << _GPIO_USART_TXROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[2].RXROUTE = (SPIPORT << _GPIO_USART_RXROUTE_PORT_SHIFT)
      | (MISOPIN << _GPIO_USART_RXROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[2].CLKROUTE = (SPIPORT << _GPIO_USART_CLKROUTE_PORT_SHIFT)
      | (SCLKPIN << _GPIO_USART_CLKROUTE_PIN_SHIFT);

  // Enable USART2 signals at the pins
  GPIO->USARTROUTE[2].ROUTEEN = GPIO_USART_ROUTEEN_TXPEN |    // MOSI
                                GPIO_USART_ROUTEEN_RXPEN |    // MISO
                                GPIO_USART_ROUTEEN_CLKPEN;
}

/***************************************************************************//**
 * App main task function.
 ******************************************************************************/
void app_main_task(void)
{
  int flashAddr = 0, rate;
  int blkCount = 0;

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

  for (blkCount = LOOPCOUNT; blkCount > 0; blkCount--)
  {
    // Set the receive state to not done
    rxDone = false;
    txDone = false;

    // Start both channels
    DMADRV_LdmaStartTransfer(spiTxChan, &ldmaTXcfg, &ldmaTXdesc, dma_tx_callback, NULL);
    DMADRV_LdmaStartTransfer(spiRxChan, &ldmaRXcfg, &ldmaRXdesc, dma_rx_callback, NULL);

    // Wait in EM1 until all data is received
    while (!rxDone && !txDone) {
            EMU_EnterEM1();
    }
  }

  // End timing
  msTicks_end = msTicks;
  sl_sleeptimer_stop_timer(&app_timer);

  // Display transfer rate
  rate = ((LOOPCOUNT * BYTECOUNT) / (msTicks_end - msTicks_start));

  app_log("Done: %u bytes in %u ms = %u KB/sec\n", (LOOPCOUNT * BYTECOUNT), (unsigned int)(msTicks_end - msTicks_start), rate);
  app_log("USART2 clock = %u MHz\n", (unsigned int)(CMU_ClockFreqGet(cmuClock_USART2) / 1000000));
  app_log("LDMA clock = %u MHz\n", (unsigned int)(CMU_ClockFreqGet(cmuClock_LDMA) / 1000000));

  __BKPT(0);
}

/***************************************************************************//**
 * App init function.
 ******************************************************************************/
void app_init(void)
{
  app_log("\nPlatform - SPI LDMA Throughput Example\n\n");

  app_spi_init();
  app_dma_init();
  app_main_task();
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
  // Nothing
}
