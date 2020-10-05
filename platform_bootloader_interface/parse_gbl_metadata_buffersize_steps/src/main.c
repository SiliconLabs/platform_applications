/***************************************************************************//**
 * @file main.c
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
 * # Evaluation Quality
 * This code has been minimally tested to ensure that it builds and is suitable
 * as a demonstration for evaluation purposes only. This code will be maintained
 * at the sole discretion of Silicon Labs.
 ******************************************************************************/

#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "stdio.h"
#include "retargetserial.h"
#include "inttypes.h"
#include "btl_parse.h"
#include "btl_interface.h"
#include "btl_interface_parser.h"
#include "application_properties.h"

// Start address of OTA slot
static const uint32_t OTA_slot0_address =0x44000;

// Size of OTA slot
static const uint32_t OTA_slot0_size = 196608;

// BufferSize for the parser function
static const uint32_t bufferSize = 128U;

// Enable debug print
#define HAL_VCOM_ENABLE        1

// Define application properties
/// Version number for this application (uint32_t)
#define APP_PROPERTIES_VERSION 1

/// Unique ID for the product this application is built for (uint8_t[16])
#define APP_PROPERTIES_ID    { 0 }

#define KEEP_SYMBOL           __attribute__((used))
KEEP_SYMBOL const ApplicationProperties_t sl_app_properties = {

  /// Magic value indicating that this is an ApplicationProperties_t struct.
  .magic = APPLICATION_PROPERTIES_MAGIC,

  /// Version number of this struct
  .structVersion = APPLICATION_PROPERTIES_VERSION,

  /// Type of signature this application is signed with
  .signatureType = APPLICATION_SIGNATURE_ECDSA_P256,

  /// Location of the signature. Typically a pointer to the end of the app
  .signatureLocation = 0,

  /// Information about the application
  .app = {

    /// Bitfield representing type of application
    .type = APPLICATION_TYPE_MCU,

    /// Version number for this application
    .version = APP_PROPERTIES_VERSION,

    /// Capabilities of this application
    .capabilities = 0,

    /// Unique ID for the product this application is built for
    .productId = APP_PROPERTIES_ID,
  },
};

// Metadata parse function
void metadata_parsing_function(void);

// Parse callback function
void callback(uint32_t , uint8_t *, size_t , void *);

/**************************************************************************//**
 * @brief GPIO Odd IRQ for pushbuttons on odd-numbered pins and to start
 * OTA GBL image verification and parsing for metadata
 *****************************************************************************/
void GPIO_ODD_IRQHandler(void) 
{
  bool faulted = false;

  // Clear all odd pin interrupt flags
  GPIO_IntClear(0xAAAA);

  // Toggle LED1
  GPIO_PinOutToggle(BSP_GPIO_LED1_PORT, BSP_GPIO_LED1_PIN);

  // Signature verification of GBL image in Slot0
  int32_t ret = bootloader_verifyImage(0,NULL);

  if (ret == BOOTLOADER_OK && !faulted)
  {
    printf("GBL signature verification: SUCCESS \n\r");

    // Parsing the metadata in the slot0
    metadata_parsing_function();
  }
  else
  {
    printf("GBL signature verification failed, ERROR: 0x%08" PRIx32 "\n\r", ret);
  }

}

/**************************************************************************//**
 * @brief GPIO initialization
 *****************************************************************************/
void initGPIO(void) 
{
  // Enable GPIO clock
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Configure PB1 as input with glitch filter enabled
  GPIO_PinModeSet(BSP_GPIO_PB1_PORT, BSP_GPIO_PB1_PIN, gpioModeInputPullFilter, 1);

  // Configure LED1 as output
  GPIO_PinModeSet(BSP_GPIO_LED1_PORT, BSP_GPIO_LED1_PIN, gpioModePushPull, 0);

  // Enable IRQ for odd numbered GPIO pins
  NVIC_EnableIRQ(GPIO_ODD_IRQn);

  // Enable falling-edge interrupts for PB1 pin
  GPIO_ExtIntConfig(BSP_GPIO_PB1_PORT, BSP_GPIO_PB1_PIN, BSP_GPIO_PB1_PIN, 0, 1, true);
}

/**************************************************************************//**
 * @brief Parser Callback Function
 *****************************************************************************/
void callback(uint32_t offset, uint8_t *buffer, size_t length, void *ctx)
{
  (void) offset;
  (void) ctx;
  // Buffer contains the raw data parsed. offset does not really matter here.
  for (uint32_t i = 0; i < length; i++)
  {
     if (i%16==0)
     {
       printf("\r");
     }
     printf("%02X ", buffer[i]);
  }
}

/**************************************************************************//**
 * @brief Metadata GBL Parsing Function
 *****************************************************************************/
void metadata_parsing_function(void)
{
  size_t offset = 0U;
  int32_t retval = 0;
  uint32_t OTA_slot0_address_new = OTA_slot0_address;
  uint8_t ctx[BOOTLOADER_STORAGE_VERIFICATION_CONTEXT_SIZE];
  BootloaderParserCallbacks_t callbacks = { 0 };
  callbacks.metadataCallback = callback;

  // Initialize components of the bootloader so the app can use the interface
  bootloader_init();

  // Application interface image parser for decrypting and parsing GBL image
  bootloader_initParser((BootloaderParserContext_t *)ctx,
                        BOOTLOADER_STORAGE_VERIFICATION_CONTEXT_SIZE);

  while (offset < OTA_slot0_size)
  {
    if (offset < OTA_slot0_size - bufferSize)
    {
       // Parse the complete GBL file and send the raw data to callback function
       retval = bootloader_parseBuffer((BootloaderParserContext_t *)ctx,
                       &callbacks, (uint8_t*)OTA_slot0_address_new, bufferSize);

       // Update offset and address for parsing
       offset += bufferSize / 1;
       OTA_slot0_address_new = OTA_slot0_address_new+bufferSize;
    }
    else
    {
       // Parse the complete GBL file and send the raw data to callback function
       retval = bootloader_parseBuffer((BootloaderParserContext_t *)ctx,
                                &callbacks, (uint8_t*)OTA_slot0_address_new, 4);

       // Update offset and address for parsing
       offset += 4 / 1;
       OTA_slot0_address_new = OTA_slot0_address_new+bufferSize;
    }

    if (retval == BOOTLOADER_ERROR_PARSE_SUCCESS)
    {
      printf("GBL metadata parse: SUCCESS\n\r");
	  break;
    }
    else if (retval != BOOTLOADER_ERROR_PARSE_CONTINUE)
    {
      // Parsing the GBL failed
      printf("GBL metadata parse failed, ERROR: 0x%08" PRIx32 "\n\r", retval);
	  break;
    }
  }
}

/**************************************************************************//**
 * @brief  Main function
 *****************************************************************************/
int main(void)
{
  // Chip errata
  CHIP_Init();

  // Initializations
  initGPIO();

  RETARGET_SerialInit();

  printf("Welcome\n\r");
  printf("OTA/flash the metadata GBL file at the beginning of solt0\n\r");
  printf("When ready press PB1 for GBL Signature verification and Parsing\n\r");

  while (1){
    // Enter Low Energy Mode until an interrupt occurs
    EMU_EnterEM3(false);
  }
}
