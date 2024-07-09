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

#include "em_cmu.h"
#include "em_gpio.h"
#include "em_msc.h"

// The write will be performed to the lower half of flash. Hence the start and
// end addresses of the lower half are defined here.
#define WRITE_ADDRESS_START     ((uint32_t *)0x00080000)
#define WRITE_ADDRESS_END       ((uint32_t *)0x00100000)

// counts 1ms timeTicks
volatile uint32_t msTicks;

// The flash page size in a Giant Gecko device is 4096 bytes
uint32_t GG_S0_PAGE_SIZE = 0x1000;

static void FLASH_writeWord(uint32_t *adr, uint32_t data);

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  uint32_t address;

  // Enable high frequency and GPIO clocks
  CMU_ClockEnable(cmuClock_HFPER, true);
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Configure the GPIO to push-pull
  GPIO_PinModeSet(gpioPortD, 6, gpioModePushPull, 0);

  // Set the start address for page erase
  address = (uint32_t)WRITE_ADDRESS_START;

  // Erase the lower half of flash
  while (address < (uint32_t)WRITE_ADDRESS_END) {
    MSC_ErasePage((uint32_t *)address);
    // Since flash erases the entire page, increment by a page for the next
    // erase
    address = address + GG_S0_PAGE_SIZE;
  }

  // Write a pattern across the entire lower half of flash
  for (address = (uint32_t)WRITE_ADDRESS_START; \
       address < (uint32_t)WRITE_ADDRESS_END;) {
    FLASH_writeWord((uint32_t *)address, 0xA5A5A5A5);
    // Increment by a word
    address = address + 4;
  }

  // Disable writing to the MSC module.
  MSC->WRITECTRL &= ~MSC_WRITECTRL_WREN;
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
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
static void FLASH_writeWord(uint32_t *adr, uint32_t data)
{
  // Enable Read-While-Write feature (RWWEN) and write and erase feature(WREN)
  MSC->WRITECTRL |= MSC_WRITECTRL_RWWEN | MSC_WRITECTRL_WREN;

  // Load page address for the write operation
  MSC->ADDRB = (uint32_t)adr;
  MSC->WRITECMD = MSC_WRITECMD_LADDRIM;

  // The WDATA register must be written when the WDATAREADY bit is set
  while (!(MSC->STATUS & MSC_STATUS_WDATAREADY)) {}

  // Load data to be written to the address in MSC_ADDR
  MSC->WDATA = data;

  // Starts write of the first word written to MSC_WDATA
  MSC->WRITECMD = MSC_WRITECMD_WRITEONCE;

  /* Poll till the write is complete */
  while (MSC->STATUS & MSC_STATUS_BUSY) {
    // Add a GPIO toggle within the wait function. Observe the GPIO in an
    // o-scope to see the device writing when it is reading
    GPIO_PinOutToggle(gpioPortD, 6);
  }
}
