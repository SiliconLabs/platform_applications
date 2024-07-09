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

#include "em_cmu.h"
#include "em_gpcrc.h"

#include "nvm3.h"
#include "nvm3_hal_flash.h"
#include "nvm3_default.h"
#include "nvm3_default_config.h"

#include "sl_component_catalog.h"
#include "sl_sleeptimer.h"
#include "sl_simple_led_instances.h"
#include "sl_simple_led.h"
#include "sl_simple_button_instances.h"
#include "sl_simple_button.h"

#define led0_on()  sl_simple_led_turn_on(sl_led_led0.context)
#define led0_off() sl_simple_led_turn_off(sl_led_led0.context)
#ifdef SL_CATALOG_SIMPLE_LED_LED1_PRESENT
#define led1_on()  sl_simple_led_turn_on(sl_led_led1.context)
#define led1_off() sl_simple_led_turn_off(sl_led_led1.context)
#else
#define led1_on()
#define led1_off()
#endif

// Use default nvm3 area
#if (NVM3_DEFAULT_CACHE_SIZE != 0)
static nvm3_CacheEntry_t nvm3_cache[NVM3_DEFAULT_CACHE_SIZE];
#endif

// Get main flash space info from linker symbol
extern char linker_nvm_begin;
extern char linker_vectors_begin;
#define MAIN_FLASH_BASE (&linker_vectors_begin)
#define MAIN_FLASH_END  (&linker_nvm_begin)
#define NVM3_BASE       MAIN_FLASH_END
nvm3_Init_t nvm3_init_data =
{
  (nvm3_HalPtr_t)NVM3_BASE,
  NVM3_DEFAULT_NVM_SIZE,
#if (NVM3_DEFAULT_CACHE_SIZE != 0)
  nvm3_cache,
#else
  NULL,
#endif
  NVM3_DEFAULT_CACHE_SIZE,
  NVM3_DEFAULT_MAX_OBJECT_SIZE,
  NVM3_DEFAULT_REPACK_HEADROOM,
  &nvm3_halFlashHandle,
};
static nvm3_Handle_t nvm3_handle;

uint32_t keys[4] = { 0xAAAA1111, 0x55552222, 0x33336666, 0xCCCC9999 };
volatile uint32_t msTicks; /* counts 1ms timeTicks */

void fail(uint32_t code)
{
  while (1)
  {
    for (uint32_t i = 0; i < code ; i++ )
    {
      led0_on();
      sl_sleeptimer_delay_millisecond(200);
      led0_off();
      sl_sleeptimer_delay_millisecond(200);
    }
    sl_sleeptimer_delay_millisecond(1000);
  }
}

void nvm3_init(uint32_t crc)
{
  Ecode_t status;
  size_t numberOfObjects;

  status = nvm3_open(&nvm3_handle, &nvm3_init_data);
  if (status != ECODE_NVM3_OK) {
    fail(1);
  }
  // Get the number of valid keys already in NVM3
  numberOfObjects = nvm3_countObjects(&nvm3_handle);
  // Skip if we have initial keys. If not, generate objects and store
  // persistently in NVM3 before proceeding.
  if (numberOfObjects < 3) {
    // Wait for PB0 press
    while (sl_simple_button_get_state(&sl_button_btn0)
           == SL_SIMPLE_BUTTON_RELEASED)
    {
      led0_on();
      sl_sleeptimer_delay_millisecond(50);
      led0_off();
      sl_sleeptimer_delay_millisecond(50);
    }
    // Erase all objects and write initial data to NVM3
    nvm3_eraseAll(&nvm3_handle);
    nvm3_writeData(&nvm3_handle, 1, &keys[0], sizeof(uint32_t));
    nvm3_writeData(&nvm3_handle, 2, &keys[1], sizeof(uint32_t));
    nvm3_writeData(&nvm3_handle, 3, &crc, sizeof(uint32_t));
  }
}

void nvm3_check(uint32_t crc)
{
  Ecode_t status;
  size_t numberOfObjects;
  uint32_t objectType;
  uint32_t data1;
  uint32_t data2;
  uint32_t stored_crc;
  size_t dataLen1;
  size_t dataLen2;
  status = nvm3_open(&nvm3_handle, &nvm3_init_data);
  if (status != ECODE_NVM3_OK) {
    fail(1);
  }
  // Get the number of valid keys already in NVM3
  numberOfObjects = nvm3_countObjects(&nvm3_handle);
  // Skip if we have initial keys. If not, generate objects and store
  // persistently in NVM3 before proceeding.
  if (numberOfObjects < 3) {
    fail(1);
  }

  nvm3_getObjectInfo(&nvm3_handle, 1, &objectType, &dataLen1);
  if (objectType == NVM3_OBJECTTYPE_DATA) {
    nvm3_readData(&nvm3_handle, 1, &data1, dataLen1);
  } else {
    fail(2);
  }

  nvm3_getObjectInfo(&nvm3_handle, 2, &objectType, &dataLen2);
  if (objectType == NVM3_OBJECTTYPE_DATA) {
    nvm3_readData(&nvm3_handle, 2, &data2, dataLen2);
  } else {
    fail(2);
  }
  if (data1 == keys[0]) {
    data1 = keys[2];
  } else if (data1 == keys[2]) {
    data1 = keys[0];
  } else {
    fail(3);
  }
  if (data2 == keys[1]) {
    data2 = keys[3];
  } else if (data2 == keys[3]) {
    data2 = keys[1];
  } else {
    fail(3);
  }

  nvm3_getObjectInfo(&nvm3_handle, 3, &objectType, &dataLen2);
  if (objectType == NVM3_OBJECTTYPE_DATA) {
    nvm3_readData(&nvm3_handle, 3, &stored_crc, dataLen2);
  } else {
    fail(2);
  }
  if (stored_crc != crc) {
    fail(4);
  }

  led1_on();
  nvm3_writeData(&nvm3_handle, 1, &data1, sizeof(uint32_t));
  nvm3_writeData(&nvm3_handle, 2, &data2, sizeof(uint32_t));
  // Do repacking if needed
  if (nvm3_repackNeeded(&nvm3_handle)) {
    status = nvm3_repack(&nvm3_handle);
    if (status != ECODE_NVM3_OK) {
      fail(5);
    }
  }
  led1_off();
}

void initGpcrc(void)
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

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  uint32_t *p_start = (uint32_t *)MAIN_FLASH_BASE;
  uint32_t *p_end = (uint32_t *)MAIN_FLASH_END;
  uint32_t crc;

  initGpcrc();

  for (uint32_t *i = p_start; i < p_end; i++)
  {
    GPCRC_InputU32(GPCRC, *i);
  }
  crc = GPCRC_DataReadBitReversed(GPCRC);
  nvm3_init(crc);
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
  uint32_t crc;
  uint32_t *p_start = (uint32_t *)MAIN_FLASH_BASE;
  uint32_t *p_end = (uint32_t *)MAIN_FLASH_END;

  for (uint32_t *i = p_start; i < p_end; i++)
  {
    GPCRC_InputU32(GPCRC, *i);
  }
  crc = GPCRC_DataReadBitReversed(GPCRC);
  nvm3_check(crc);
  sl_sleeptimer_delay_millisecond(100);
}
