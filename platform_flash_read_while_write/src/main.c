/***************************************************************************//**
* @file main.c
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
* This software is provided \'as-is\', without any express or implied
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

// CMSIS
#include "em_device.h"

// emlib
#include "em_chip.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_msc.h"

// counts 1ms timeTicks
volatile uint32_t msTicks;

// The flash page size in a Giant Gecko device is 4096 bytes
uint32_t GG_S0_PAGE_SIZE = 0x1000;

// The write will be performed to the lower half of flash. Hence the start and
// end addresses of the lower half are defined here.
#define WRITE_ADDRESS_START     ((uint32_t*)0x00080000)
#define WRITE_ADDRESS_END       ((uint32_t*)0x00100000)

/***************************************************************************//**
 * SysTick_Handler
 * Interrupt Service Routine for system tick counter
 ******************************************************************************/
void SysTick_Handler(void)
{
  msTicks++;       /* increment counter necessary in Delay()*/
}

/***************************************************************************//**
 * Delays number of msTick Systicks (typically 1 ms)
 * Input Parameter - dlyTicks Number of ticks to delay
 ******************************************************************************/
void Delay(uint32_t dlyTicks)
{
  uint32_t curTicks;

  curTicks = msTicks;
  while ((msTicks - curTicks) < dlyTicks) ;
}

/**************************************************************************//**
 * Programs a single word into flash. In general, the write data must be
 * aligned to words and contain a number of bytes that is divisible by four.
 *
 * It is recommended to erase the flash page before performing a write. A
 * function has been custom written instead of using MSC_WriteWord since most
 * em_msc functions are executed from RAM which is necessary when the flash
 * region is not split to read and write sections.
 *
 *****************************************************************************/
void FLASH_writeWord(uint32_t *adr, uint32_t data)
{
  // Enable Read-While-Write feature (RWWEN) and write and erase feature(WREN)
  MSC->WRITECTRL |= MSC_WRITECTRL_RWWEN | MSC_WRITECTRL_WREN;

  // Load page address for the write operation
  MSC->ADDRB    = (uint32_t)adr;
  MSC->WRITECMD = MSC_WRITECMD_LADDRIM;

  // The WDATA register must be written when the WDATAREADY bit is set
  while (!(MSC->STATUS & MSC_STATUS_WDATAREADY));

  // Load data to be written to the address in MSC_ADDR
  MSC->WDATA = data;

  // Starts write of the first word written to MSC_WDATA
  MSC->WRITECMD = MSC_WRITECMD_WRITEONCE;

  /* Poll till the write is complete */
  while (MSC->STATUS & MSC_STATUS_BUSY){
    // Add a GPIO toggle within the wait function. Observe the GPIO in an
    // o-scope to see the device writing when it is reading
    GPIO_PinOutToggle(gpioPortD, 6);
  }
}

/***************************************************************************//**
 * Main function
 ******************************************************************************/
int main(void)
{
  CHIP_Init();

  uint32_t address;

  // Setup SysTick Timer for 1 msec interrupts
  if (SysTick_Config(CMU_ClockFreqGet(cmuClock_CORE) / 1000)) {
    while (1) ;
  }

  // Enable high frequency and GPIO clocks
  CMU_ClockEnable(cmuClock_HFPER, true);
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Configure the GPIO to push-pull
  GPIO_PinModeSet(gpioPortD, 6, gpioModePushPull, 0);

  // Set the start address for page erase
  address = (uint32_t)WRITE_ADDRESS_START;

  // Erase the lower half of flash
  while (address < (uint32_t)WRITE_ADDRESS_END){
    MSC_ErasePage((uint32_t*)address);
    // Since flash erases the entire page, increment by a page for the next
    // erase
    address = address + GG_S0_PAGE_SIZE;
  }

  // Write a pattern across the entire lower half of flash
  for (address = (uint32_t)WRITE_ADDRESS_START; \
  address < (uint32_t)WRITE_ADDRESS_END;){

    FLASH_writeWord ((uint32_t*)address, 0xA5A5A5A5);
    // Increment by a word
    address = address+4;
  }

  // Disable writing to the MSC module.
  MSC->WRITECTRL &= ~MSC_WRITECTRL_WREN;

  // Infinite blink loop
  while (1);
}
