/***************************************************************************//**
 * @file main_xg21.c
 * @version 1.0.1
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
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

// emlib
#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpcrc.h"
#include "em_ldma.h"
#include "em_timer.h"

// BSP
#include "bsp.h"

// STDIO to serial
#include "retargetserial.h"

#include <stdio.h>

// IEEE 802.3 CRC of blank 8 KB flash page
#define BLANK_FLASH_CRC_8KB (0xb4293435UL)

// LDMA GPCRC channel assignment
#define LDMA_GPCRC_CHAN  0

// LDMA GPCRC descriptor and transfer configuration structures
LDMA_Descriptor_t ldmaCrcDesc[2];
LDMA_TransferCfg_t ldmaCrcXferCfg;

void initCMU(void)
{
  // Initialize and enable the HFXO for the crystal on the (W)STK
  CMU_HFXOInit_TypeDef hfxoInit = CMU_HFXOINIT_DEFAULT;
  CMU_HFXOInit(&hfxoInit);
  CMU_OscillatorEnable(cmuOsc_HFXO, true, true);

  // Set DPLL to 80 MHz (40 MHz PCLK)
  CMU_DPLLInit_TypeDef dpllInit = CMU_DPLL_HFXO_TO_80MHZ;

  // Attempt DPLL lock; halt on failure
  if (CMU_DPLLLock(&dpllInit) == false)
    __BKPT(0);
}

void initTIMER0(void)
{
  // Initialize but don't start TIMER0
  TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
  timerInit.enable = false;

  TIMER_Init(TIMER0, &timerInit);
}

void initLDMA(void)
{
  /*
   * LDMA module initialization.  Note that the LDMA_IF_ERROR interrupt
   * is also enabled by LDMA_Init(), so it's effectively mandatory to
   * have LDMA_IRQHandler() because of this just to catch the error
   * interrupt, if it happens.
   */
  LDMA_Init_t ldmaInit = LDMA_INIT_DEFAULT;
  LDMA_Init(&ldmaInit);
}

void LDMA_IRQHandler(void)
{
  // Shouldn't ever get in here, so take a breakpoint
  __BKPT(1);
}

void initGPCRC(void)
{
  // GPCRC module initialization for IEEE 802.3 polynomial
  GPCRC_Init_TypeDef initCrc = GPCRC_INIT_DEFAULT;
  initCrc.initValue = 0xFFFFFFFF;
  GPCRC_Init(GPCRC, &initCrc);
}

bool crcBlankCheckFlashPageStart(uint32_t baseAddr)
{
  bool result = true;
  uint32_t alignedBaseAddr;

  // Make sure the address is in flash
  if ((baseAddr >= FLASH_MEM_BASE) &&
      (baseAddr <= FLASH_MEM_END))
  {
    // Align the base address to its page boundary as a precaution
    alignedBaseAddr = baseAddr & ~(FLASH_PAGE_SIZE - 1);

    // Prep the GPCRC
    GPCRC_Start(GPCRC);

    // Use the generic memory-to-memory transfer configuration; halt during debug
    ldmaCrcXferCfg = (LDMA_TransferCfg_t)LDMA_TRANSFER_CFG_MEMORY();
    ldmaCrcXferCfg.ldmaDbgHalt = true;

    /*
     * Even though the GPCRC is a peripheral, it's necessary to use the
     * M2M descriptor because there is no DMA request (such as RXDATAV
     * upon USART reception).  Instead, the LDMA simply needs to pump
     * the page of flash through the GPCRC, word by word, as fast as it
     * can manage to do so.
     *
     * One minor annoyance here is that even when the descriptor's
     * doneIfs field is set to 0 in order to avoid an interrupt upon
     * channel completion, the LDMA always sets the corresponding
     * LDMA_IF flag after executing the last descriptor.
     *
     * Setting the LDMA_IF flag in and of itself isn't a problem save
     * for the fact that LDMA_StartTransfer() enables a channel's
     * interrupt no matter what.
     *
     * Because the CRC blank checking operation is intended to run in
     * this example until software checks to see if it is actually
     * done, a little bit of trickery is used to disable the interrupt
     * right after LDMA_StartTransfer() enables it.
     *
     * Instead of using a single descriptor to execute the GPCRC
     * operation, a two element linked list is used.  The first entry
     * in the list is a WRI descriptor that writes the bit mask for the
     * GPCRC LDMA channel to the LDMA_IEN register in the peripheral
     * bit clear aliasing region (register address + 0x04000000).  The
     * next descriptor is the M2M transfer that feeds the flash page
     * contents into the GPCRC_INPUTDATA register word by word.
     */
    ldmaCrcDesc[0] = (LDMA_Descriptor_t)LDMA_DESCRIPTOR_LINKREL_WRITE((1 << LDMA_GPCRC_CHAN), &(LDMA->IEN_CLR), 1);
    ldmaCrcDesc[1] = (LDMA_Descriptor_t)LDMA_DESCRIPTOR_SINGLE_M2M_WORD(alignedBaseAddr, &(GPCRC->INPUTDATA), (FLASH_PAGE_SIZE >> 2));
    ldmaCrcDesc[1].xfer.dstInc = ldmaCtrlDstIncNone;

    // This starts the CRC calculation.
    LDMA_StartTransfer(LDMA_GPCRC_CHAN, (void*)&ldmaCrcXferCfg, (void*)&ldmaCrcDesc);
  }
  else result = false;

  return result;
}

bool crcBlankCheckFlashPageBusy(void)
{
  if (LDMA_TransferDone(LDMA_GPCRC_CHAN) == true)
    return false;
  else
    return true;
}

bool crcBlankCheckFlashPageResult(void)
{
  // Invert the data register output to get the IEEE 802.3 result
  uint32_t crcResult = ~GPCRC_DataRead(GPCRC);

  if (crcResult == BLANK_FLASH_CRC_8KB)
    return true;
  else
    return false;
}

/*
 * This CPU-driven blank check expects that *(uint64_t *)(baseAddr + i)
 * is going to compile to a single LDRD instruction.  For this to be
 * the case, (baseAddr + i) must be double word-aligned, which is
 * guaranteed for a flash page on any EFM32/EFR32 device, assuming that
 * 'i' is always a multiple of 8.  GCC 7.2.1 will do this in either
 * Simplicity Studio's Debug (-O0) or Release (-O2) configurations.
 */
bool cpuBlankCheckFlashPage(uint32_t baseAddr)
{
  bool result = true;
  uint32_t alignedBaseAddr, i = 0;

  // Make sure the address is in flash
  if ((baseAddr >= FLASH_MEM_BASE) &&
      (baseAddr <= FLASH_MEM_END))
  {
    // Align the base address to its page boundary as a precaution
    alignedBaseAddr = baseAddr & ~(FLASH_PAGE_SIZE - 1);

    while (i < FLASH_PAGE_SIZE)
    {
      /*
      // Check using 32-bit read
      if ((*(uint32_t *)(alignedBaseAddr + i)) == 0xFFFFFFFF)
        i += 4;
      */
      // Check using a 64-bit read
      if ((*(uint64_t *)(alignedBaseAddr + i)) == 0xFFFFFFFFFFFFFFFF)
        i += 8;
      else
      {
        result = false;
        i = FLASH_PAGE_SIZE;
      }
    }
  }
  else result = false;

  return result;
}

/*
 * Use the CPU to feed the flash page to the GPCRC.  This might be a
 * useful as a way to perform flash word programming of one page in
 * parallel with blank checking of the next sequential page (e.g. the
 * same loop index would be used for both).
 */
uint32_t crcManualBlankCheckPage(uint32_t baseAddr)
{
  bool result = false;
  uint32_t alignedBaseAddr, i = 0;

  // Make sure the address is in flash
  if ((baseAddr >= FLASH_MEM_BASE) &&
      (baseAddr <= FLASH_MEM_END))
  {
    // Align the base address to its page boundary as a precaution
    alignedBaseAddr = baseAddr & ~(FLASH_PAGE_SIZE - 1);

    GPCRC_Start(GPCRC);

    do
    {
      GPCRC_InputU32(GPCRC, (*(uint32_t *)(alignedBaseAddr + i)));
      i += 4;
    }
    while (i < FLASH_PAGE_SIZE);

    if (GPCRC_DataRead(GPCRC) == ~BLANK_FLASH_CRC_8KB)
      result = true;
  }

  return result;
}

int main(void)
{
  unsigned int timerCount;
  float tickLength;
  bool pageBlank;

  CHIP_Init();

  initCMU();

  initTIMER0();

  initLDMA();

  initGPCRC();

  // Retarget STDIO to USART0
  RETARGET_SerialInit();
  RETARGET_SerialCrLf(1);

  // ANSI/POSIX clear screen; should work in most terminal programs
  printf("\e[1;1H\e[2J");

  printf("Retarget is working!\n\n");

  // TIMER tick length
  tickLength = (float)(1000000000 / CMU_ClockFreqGet(cmuClock_TIMER0));

  // Start counting
  TIMER0->CMD = TIMER_CMD_START;

  // Check to see if last page in flash is blank
  pageBlank = cpuBlankCheckFlashPage(FLASH_BASE + FLASH_SIZE - FLASH_PAGE_SIZE);

  // Stop counting
  TIMER0->CMD = TIMER_CMD_STOP;

  // Get TIMER0 timerCount;
  timerCount = TIMER_CounterGet(TIMER0);

  // CPU clock frequency
  printf("CPU blank check at %u MHz ", (unsigned int)(CMU_ClockFreqGet(cmuClock_CORE) / 1000000));

  // CPU blank check execution time in microseconds
  printf("took %u microseconds.\n", (unsigned int)((tickLength * (float)timerCount) / 1000));

  printf("Flash page at %#010x is ", (unsigned int)(FLASH_BASE + FLASH_SIZE - FLASH_PAGE_SIZE));

  // Print blank state
  if (pageBlank == true)
    printf("blank.");
  else
    printf("not blank.");

  // Reset TIMER0 counter
  TIMER_CounterSet(TIMER0, _TIMER_CNT_RESETVALUE);

  // Start counting
  TIMER0->CMD = TIMER_CMD_START;

  /*
   * Use the GPCRC to blank check the last page of flash.  If the CPU
   * stops on the BKPT, it means the address is out of range or not
   * word-aligned.  In a normal use case, software should otherwise
   * handle this error condition.
   */
  if (crcBlankCheckFlashPageStart(FLASH_BASE + FLASH_SIZE - FLASH_PAGE_SIZE) == false)
    __BKPT(0);

  // Wait while CRC blank check is in progress
  while (crcBlankCheckFlashPageBusy() == true);

  // Get the blank state
  pageBlank = crcBlankCheckFlashPageResult();

  // Stop counting
  TIMER0->CMD = TIMER_CMD_STOP;

  // Get TIMER0 timerCount;
  timerCount = TIMER_CounterGet(TIMER0);

  // LDMA/GPCRC clock frequency
  printf("\n\nLDMA/GPCRC blank check at %u MHz ", (unsigned int)(CMU_ClockFreqGet(cmuClock_GPCRC) / 1000000));

  // LDMA/GPCRC blank check execution time in microseconds
  printf("took %u microseconds.\n", (unsigned int)((tickLength * (float)timerCount) / 1000));

  printf("Flash page at %#010x is ", (unsigned int)(FLASH_BASE + FLASH_SIZE - FLASH_PAGE_SIZE));

  // Print blank state
  if (pageBlank == true)
    printf("blank.");
  else
    printf("not blank.");

  // Reset TIMER0 counter
  TIMER_CounterSet(TIMER0, _TIMER_CNT_RESETVALUE);

  // Start counting
  TIMER0->CMD = TIMER_CMD_START;

  // Blank check using CPU/GPCRC state
  pageBlank = crcManualBlankCheckPage(FLASH_BASE + FLASH_SIZE - FLASH_PAGE_SIZE);

  // Stop counting
  TIMER0->CMD = TIMER_CMD_STOP;

  // Get TIMER0 timerCount;
  timerCount = TIMER_CounterGet(TIMER0);

  // CPU/GPCRC blank check execution time in microseconds
  printf("\n\nCPU/GPCRC blank check at %u MHz/%u MHz ", (unsigned int)(CMU_ClockFreqGet(cmuClock_CORE) / 1000000),
                                                        (unsigned int)(CMU_ClockFreqGet(cmuClock_GPCRC) / 1000000));

  // CPU/GPCRC blank check execution time in microseconds
  printf("took %u microseconds.\n", (unsigned int)((tickLength * (float)timerCount) / 1000));

  printf("Flash page at %#010x is ", (unsigned int)(FLASH_BASE + FLASH_SIZE - FLASH_PAGE_SIZE));

  // Print blank state
  if (pageBlank == true)
    printf("blank.");
  else
    printf("not blank.");

  __BKPT(2);
}
