#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_ldma.h"
#include "em_usart.h"

#include "bsp.h"
#include "retargetserial.h"

#include "systick.h"

#include <stdio.h>

// USART2 SPI pins
#define SPIPORT   gpioPortC
#define MOSIPIN   0
#define MISOPIN   1
#define SCLKPIN   2
#define CSPORT    gpioPortC
#define CSPIN     3

// LDMA channel assignments
#define SPIRX_CHAN  0
#define SPITX_CHAN  1

// LDMA SPI RX descriptor and transfer configuration structures
LDMA_Descriptor_t ldmaRXdesc;
LDMA_TransferCfg_t ldmaRXcfg;

// LDMA SPI TX descriptor and transfer configuration structures
LDMA_Descriptor_t ldmaTXdesc;
LDMA_TransferCfg_t ldmaTXcfg;

// SPI flash command
#define M25X_READ 0x3

// Transfer count
#define BYTECOUNT 1024
#define LOOPCOUNT 1024

// Block receive completion flag
bool rxDone = false;

// Dummy buffers for SPI transmit/receive
uint8_t dummyOut[1] = { 0x0 };
uint8_t dummyIn[1] = { 0x0 };

// Globals for tick count
volatile uint32_t msTicks;
uint32_t msTicks_start, msTicks_end;

void initCMU(void)
{
  // Initialize and enable the HFXO for the crystal on the BRD4180A radio board
  CMU_HFXOInit_TypeDef hfxoInit = CMU_HFXOINIT_DEFAULT;
  CMU_HFXOInit(&hfxoInit);
  CMU_OscillatorEnable(cmuOsc_HFXO, true, true);

  // Select the DPLL as the SYSCLK
  //CMU_ClockSelectSet(cmuClock_SYSCLK, cmuSelect_HFRCODPLL);

  /*
   * Initialize the DPLL to run at 50 MHz from the 38.4 MHz HFXO
   * reference.  The HFXO is designed to provide an accurate reference
   * to the radio vs. the LFXO which is designed for low-power.
   *
   * fDPLL = fREF * (N + 1) / (M + 1), where N > 300
   *
   * In this case:
   *
   * fDPLL = 38,400,000 * (3749 + 1) / (2879 + 1) = 50,000,000 MHz
   */
  /*
  CMU_DPLLInit_TypeDef dpllInit = CMU_DPLLINIT_DEFAULT;
  dpllInit.frequency = 50000000;
  dpllInit.n = (3750 - 1);
  dpllInit.m = (2880 - 1);
  dpllInit.refClk = cmuSelect_HFXO;
  */
  /*
  CMU_DPLLInit_TypeDef dpllInit = CMU_DPLLINIT_DEFAULT;
  dpllInit.frequency = 40000000;
  dpllInit.n = (3750 - 1);
  dpllInit.m = (3600 - 1);
  dpllInit.refClk = cmuSelect_HFXO;
  */
  // Set DPLL to 80 MHz (40 MHz PCLK)
  CMU_DPLLInit_TypeDef dpllInit = CMU_DPLL_HFXO_TO_80MHZ;

  // Attempt DPLL lock; halt on failure
  if (CMU_DPLLLock(&dpllInit) == false)
    __BKPT(0);
}

// LDMA channel configuration
void initLDMA(void)
{
  // LDMA module initialization
  LDMA_Init_t ldmaInit = LDMA_INIT_DEFAULT;
  LDMA_Init(&ldmaInit);

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

  // Transmit a byte when there's free space in the USART TX buffer
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

  // Transfer a byte on receive data valid
  ldmaRXcfg = (LDMA_TransferCfg_t)LDMA_TRANSFER_CFG_PERIPHERAL(ldmaPeripheralSignal_USART2_RXDATAV);
}

/*
 * The LDMA interrupt handler is called upon completion of all LDMA
 * transfers unless specifically disabled.  This has to be customized
 * to handle whatever channel post-completion logic is needed in a
 * given application.
 */
void LDMA_IRQHandler()
{
  uint32_t flags = LDMA_IntGet();

  // Clear the SPI TX channel flag on completion
  if (flags & (1 << SPITX_CHAN))
    LDMA_IntClear(1 << SPITX_CHAN);

  /*
   * Clear the SPI RX channel flag on completion and set the receive
   * state to done.
   */
  if (flags & (1 << SPIRX_CHAN))
  {
    LDMA_IntClear(1 << SPIRX_CHAN);
    rxDone = true;
  }

  // Stop in case there was an error
  if (flags & LDMA_IF_ERROR)
    __BKPT(0);
}

// Using USART2 because USART1 is not available on the EXP header port C pins
void initSPI2(void)
{
  // SPI initialization; go for max baud rate (PCLK / 2)
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

void tickerSync()
{
  msTicks_end = msTicks;

  while(msTicks_end == msTicks);    //msTicks Sync

  msTicks_start = msTicks;
}

int main(void)
{
  int blkCount, rate, flashAddr = 0;

  CHIP_Init();

  initCMU();

  // Setup SysTick for 1 msec interval
  SysTick_Start();
  SysTick_SetPeriod(CMU_ClockFreqGet(cmuClock_CORE) / 1000);

  // Retarget STDIO to USART0
  RETARGET_SerialInit();
  RETARGET_SerialCrLf(1);

  printf("Retarget is working!\n");

  initSPI2();

  initLDMA();

  // Start timing
  SysTick_IntEnable();
  tickerSync();

  /*
   * Read 1 MB from SPI flash using x8 transfers
   *
   * 1. Assert CS
   * 2. Send READ command (0x3)
   * 3. Send 24-bit address
   * 4. Clock in 1 MB of data by sending 1048576 dummy bytes
   * 5. De-assert CS
   */
  GPIO_PinOutClear(CSPORT, CSPIN);

  USART_Tx(USART2, M25X_READ);

  /*
   * Send bytes 1, 2, and 3 of 32-bit address.  Byte 0 should always
   * be 0 unless the device in question has a 32-bit address space, in
   * which case byte 0 needs to be sent, too.
   */
  USART_Tx(USART2, *((uint8_t *)&flashAddr + 1));
  USART_Tx(USART2, *((uint8_t *)&flashAddr + 2));
  USART_Tx(USART2, *((uint8_t *)&flashAddr + 3));

  /*
   * Perform the 1 MB read as a series of 1024 SW-initiated 1 KB
   * transfers.  LDMA loop code could be used to automate this entire
   * process, but no EFR32 device has enough RAM to store 1 MB of data,
   * so handling the transfers in blocks is a more realistic use case.
   *
   * The LDMA can perform 16-bit writes to the TX data register, and
   * the transfer count can be a maximum of 2048, so a block of 4096
   * bytes can be transferred each time a channel is started.
   */
  for (blkCount = LOOPCOUNT; blkCount > 0; blkCount--)
  {
    // Set the receive state to not done
    rxDone = false;

    // Start both channels
    LDMA_StartTransfer(SPIRX_CHAN, &ldmaRXcfg, &ldmaRXdesc);
    LDMA_StartTransfer(SPITX_CHAN, &ldmaTXcfg, &ldmaTXdesc);

    // Wait in EM1 until all data is received
    while (!rxDone)
      EMU_EnterEM1();
  }

  GPIO_PinOutSet(CSPORT, CSPIN);

  // End timing
  msTicks_end = msTicks;
  SysTick_IntDisable();

  // Display transfer rate
  rate = ((LOOPCOUNT * BYTECOUNT) / (msTicks_end - msTicks_start));
  printf("Done: %u bytes in %u ms = %u KB/sec\n", (LOOPCOUNT * BYTECOUNT), (unsigned int)(msTicks_end - msTicks_start), rate);

  printf("LDMA clock = %u MHz\n", (unsigned int)(CMU_ClockFreqGet(cmuClock_LDMA) / 1000000));
  __BKPT(0);
}

// SysTick ISR
void SysTick_Handler(void)
{
  msTicks++;  // Increment milliseconds tick counter
}
