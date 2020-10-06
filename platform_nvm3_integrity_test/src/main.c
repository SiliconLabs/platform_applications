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

#include "em_device.h"
#include "em_chip.h"
#include "nvm3.h"
#include "em_cmu.h"
#include "em_gpcrc.h"
#include "em_gpio.h"
#include "nvm3_hal_flash.h"

uint32_t keys[4] = { 0xAAAA1111, 0x55552222, 0x33336666, 0xCCCC9999 };
volatile uint32_t msTicks; /* counts 1ms timeTicks */

void Delay(uint32_t dlyTicks);

/***************************************************************************//**
 * @brief SysTick_Handler
 * Interrupt Service Routine for system tick counter
 ******************************************************************************/
void SysTick_Handler(void)
{
  msTicks++;       /* increment counter necessary in Delay()*/
}

void Delay(uint32_t dlyTicks)
{
  uint32_t curTicks;

  curTicks = msTicks;
  while ((msTicks - curTicks) < dlyTicks) ;
}

void fail(uint32_t code)
{
  uint32_t i;
  while (1)
  {
    for(i = 0; i < code ; i ++ )
    {
      GPIO_PinModeSet(gpioPortF, 4, gpioModePushPull, 1);
      Delay(200);
      GPIO_PinModeSet(gpioPortF, 4, gpioModePushPull, 0);
      Delay(200);
    }
    Delay(1000);
  }
}
extern uint8_t       *nvm3Address;
// Create a NVM area of 24kB (size must equal N * FLASH_PAGE_SIZE, N is integer). Create a cache of 10 entries.
NVM3_DEFINE_SECTION_STATIC_DATA(nvm3Data1, 4*2048, 10);
// This macro creates the following:
// 1. An array to hold NVM data named nvm3Data1_nvm
// 2. A section called nvm3Data1_section containing nvm3Data1_nvm. The application linker script must place this section correctly in memory.
// 3. A cache array: nvm3Data1_cache
void nvm3_init(uint32_t crc)
{
  // Declare a nvm3_Init_t struct of name nvm3Data1 with initialization data. This is passed to nvm3_open() below.
  NVM3_DEFINE_SECTION_INIT_DATA(nvm3Data1, &nvm3_halFlashHandle);
  static nvm3_Handle_t handle;
  Ecode_t status;
  size_t numberOfObjects;
  uint32_t objectType;
  size_t dataLen1;
  size_t dataLen2;
  status = nvm3_open(&handle, &nvm3Data1);
  if (status != ECODE_NVM3_OK) {
    fail(1);
  }
  // Get the number of valid keys already in NVM3
  numberOfObjects = nvm3_countObjects(&handle);
  // Skip if we have initial keys. If not, generate objects and store
  // persistently in NVM3 before proceeding.
  if (numberOfObjects < 3)
  {
    GPIO_PinModeSet(gpioPortF,6,gpioModeInputPull, 1);
    while (GPIO_PinInGet(gpioPortF,6) != 0) //Wait for PB0 press
    {
      GPIO_PinModeSet(gpioPortF, 4, gpioModePushPull, 1);
      Delay(50);
      GPIO_PinModeSet(gpioPortF, 4, gpioModePushPull, 0);
      Delay(50);
    }
    // Erase all objects and write initial data to NVM3
    status = nvm3_eraseAll(&handle);
    status = nvm3_writeData(&handle, 1, &keys[0], sizeof(uint32_t));
    status = nvm3_writeData(&handle, 2, &keys[1], sizeof(uint32_t));
    status = nvm3_writeData(&handle, 3, &crc, sizeof(uint32_t));
  }
}

void nvm3_check(uint32_t crc)
{
  // Declare a nvm3_Init_t struct of name nvm3Data1 with initialization data. This is passed to nvm3_open() below.
  NVM3_DEFINE_SECTION_INIT_DATA(nvm3Data1, &nvm3_halFlashHandle);
  static nvm3_Handle_t handle;
  Ecode_t status;
  size_t numberOfObjects;
  uint32_t objectType;
  uint32_t data1;
  uint32_t data2;
  uint32_t stored_crc;
  size_t dataLen1;
  size_t dataLen2;
  status = nvm3_open(&handle, &nvm3Data1);
  if (status != ECODE_NVM3_OK) {
    fail(1);
  }
  // Get the number of valid keys already in NVM3
  numberOfObjects = nvm3_countObjects(&handle);
  // Skip if we have initial keys. If not, generate objects and store
  // persistently in NVM3 before proceeding.
  if (numberOfObjects < 3) {
    fail(1);
  }
  status = nvm3_getObjectInfo(&handle, 1, &objectType, &dataLen1);
  if (objectType == NVM3_OBJECTTYPE_DATA)
  {
    status = nvm3_readData(&handle, 1, &data1, dataLen1);
  }
  else
  {
    fail(2);
  }
  status = nvm3_getObjectInfo(&handle, 2, &objectType, &dataLen2);
  if (objectType == NVM3_OBJECTTYPE_DATA)
  {
    status = nvm3_readData(&handle, 2, &data2, dataLen2);
  }
  else
  {
    fail(2);
  }
  if (data1 == keys[0] )
    data1 = keys[2];
  else if (data1 == keys[2] )
    data1 = keys[0];
  else
    fail(3);
  if (data2 == keys[1] )
    data2 = keys[3];
  else if (data2 == keys[3] )
    data2 = keys[1];
  else
    fail(3);

  status = nvm3_getObjectInfo(&handle, 3, &objectType, &dataLen2);
  if (objectType == NVM3_OBJECTTYPE_DATA)
  {
    status = nvm3_readData(&handle, 3, &stored_crc, dataLen2);
  }
  else
  {
    fail(2);
  }
  if (stored_crc != crc)
  {
    fail(4);
  }

  GPIO_PinModeSet(gpioPortF, 5, gpioModePushPull, 1);
  status = nvm3_writeData(&handle, 1, &data1, sizeof(uint32_t));
  status = nvm3_writeData(&handle, 2, &data2, sizeof(uint32_t));
  // Do repacking if needed
  if (nvm3_repackNeeded(&handle))
  {
    status = nvm3_repack(&handle);
    if (status != ECODE_NVM3_OK)
    {
      fail(5);
    }
  }
  GPIO_PinModeSet(gpioPortF, 5, gpioModePushPull, 0);
}
void initGpcrc (void)
{
  // Enable clocks required
  CMU_ClockEnable(cmuClock_GPCRC, true);

  // Declare init structs
  GPCRC_Init_TypeDef init = GPCRC_INIT_DEFAULT;

  init.initValue = 0xFFFFFFFF;  // Starting value in GPCRC_DATA
  init.autoInit = true;     // Reset GPCRC_DATA to 0xFFFF_FFFF after every read
  init.reverseBits = true;  // Reverse all bits of the incoming message

  // Initialize GPCRC
  GPCRC_Init(GPCRC, &init);

  // Prepare GPCRC_DATA for inputs
  GPCRC_Start(GPCRC);
}

int main(void)
{
  /* Chip errata */
  CHIP_Init();
  uint32_t *p;
  uint32_t results;
  uint32_t crc;

  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(cmuClock_GPCRC, true);

  initGpcrc();
  /* Setup SysTick Timer for 1 msec interrupts  */
  if (SysTick_Config(CMU_ClockFreqGet(cmuClock_CORE) / 1000)) {
    while (1) ;
  }
  for (p = 0; p < 0x3E000; p++)
  {
    GPCRC_InputU32(GPCRC, *p);
  }
  crc = GPCRC_DataReadBitReversed(GPCRC);
  nvm3_init(crc);
  for (;;)
  {
    for (p = 0; p < 0x3E000; p++)
    {
      GPCRC_InputU32(GPCRC, *p);
    }
    crc = GPCRC_DataReadBitReversed(GPCRC);
    nvm3_check(crc);
    Delay(100);
  }
}
