/***************************************************************************//**
* @file  main.c
* @brief Main source file of the Guitar and ukulele tuner project
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

#include <stdlib.h>
#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_prs.h"
#include "../Microphone/Microphone_driver.h"
#include "../Audio DSP/audio_dsp.h"
#include "../LCD display/lcd.h"
#include "../Tuned algorithm/tuned_algorithm.h"

/********************************//**
 * Application buffers
 ********************************/
// Dual buffer for microphone data
static volatile uint32_t Micbuffer1[FFT_SIZE];
static volatile uint32_t Micbuffer2[FFT_SIZE];

// Buffer for floating point converted microphone data (time domain)
static float timeDomBuffer[FFT_SIZE];

// Buffer for frequency content data (frequency domain)
static float freqDomBuffer[FFT_SIZE/2];

/********************************//**
 * Application type definitions
 ********************************/
// app_status flags
typedef struct {
  bool buffer_select;        //Indicates which buffer to process (Micbuffer1 = true, Micbuffer2 = false)
  bool buffer_ready;         //Indicates if at least a buffer had been filled with data
  bool instrument_toggle; //Indicates the target instrument to tune (guitar = true, ukulele = false)
} app_status_t;

/********************************//**
 * Global variables
 ********************************/
static volatile app_status_t app_status;
tuner_display_t tuner_UI_data;

/********************************//**
 * Board define macros
 ********************************/
// Push button
#define PB0_PORT gpioPortC
#define PB0_PIN  8

#define PB1_PORT gpioPortC
#define PB1_PIN  9

/********************************//**
 * Function prototypes
 ********************************/
void LDMA_IRQHandler(void);
void GPIO_ODD_IRQHandler(void);
static void initCMU(void);
static void initGPIO(void);
static void initPRS(void);
static void initNVIC(void);

/***************************************************************************//**
 * @name: LDMA_IRQHandler
 *
 * @brief: Handler that triggers on each complete LDMA transfer and sets the
 *         corresponding flag
 *
 * @param[in]: none
 *
 * @return: None
 ******************************************************************************/
void LDMA_IRQHandler(void)
{
  // Clear all LDMA interrupt flags
  LDMA->IFC |= 0xFFFFFF;

  // Switch the buffer to store data
  app_status.buffer_select = !app_status.buffer_select;

  app_status.buffer_ready = true;
}

/***************************************************************************//**
 * @name: GPIO_ODD_IRQHandler
 *
 * @brief: Handler that triggers when PB1 is pressed. Toggles the instrument
 *
 * @param[in]: none
 *
 * @return: none
 ******************************************************************************/
void GPIO_ODD_IRQHandler(void)
{
  // Clear all odd pin interrupt flags
  GPIO_IntClear(0xAAAA);

  // Toggle the instrument to tune
  app_status.instrument_toggle = !app_status.instrument_toggle;
}

/***************************************************************************//**
 * @name: initCMU
 *
 * @brief: Initializes the HF clock speed
 *
 * @param[in]: none
 *
 * @return: none
 ******************************************************************************/
static void initCMU()
{
  // Set HF clock to 72MHz
  CMU_HFRCOBandSet(cmuHFRCOFreq_72M0Hz);
}

/***************************************************************************//**
 * @name: initPRS
 *
 * @brief: Initializes the PRS peripheral having PB0 as the source for channel 0
 *
 * @param[in]: none
 *
 * @return: None
 ******************************************************************************/
static void initPRS(void)
{
  // Enable the PRS clock
  CMU_ClockEnable(cmuClock_PRS, true);

  // Select GPIO as PRS source and push button as signal for PRS channel - On positive edge (Matching the pushbutton interrupt)
  if (PB0_PIN > 7){
    PRS_SourceSignalSet(0, PRS_CH_CTRL_SOURCESEL_GPIOH, (uint32_t)(PB0_PIN - 8), prsEdgePos);
  } else {
    PRS_SourceSignalSet(0, PRS_CH_CTRL_SOURCESEL_GPIOL, PB0_PIN, prsEdgePos);
  }
}

/***************************************************************************//**
 * @name: initGPIO
 *
 * @brief: Initializes PB0 and PB1 as inputs as well as their corresponding
 *         interrupts PB0 interrupt is configured but disabled (serves as
 *         source for PRS) PB1 interrupt is enabled
 *
 * @param[in]: none
 *
 * @return: None
 ******************************************************************************/
static void initGPIO(void)
{
  // Enable GPIO clock
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Configure push button 0 & 1 as input with filter and pull-up
  GPIO_PinModeSet(PB0_PORT, PB0_PIN, gpioModeInputPullFilter, 1);
  GPIO_PinModeSet(PB1_PORT, PB1_PIN, gpioModeInputPullFilter, 1);

  // Configure interrupt on push button 0 for rising edge but not enabled - PRS sensing instead
  GPIO_IntConfig(PB0_PORT, PB0_PIN, true, false, false);

  // Configure interrupt on push button 1 for rising edge
  GPIO_IntConfig(PB1_PORT, PB1_PIN, true, false, true);

  // Enable IRQ for odd numbered GPIO pins
  NVIC_EnableIRQ(GPIO_ODD_IRQn);
}

/***************************************************************************//**
 * @name: initNVIC
 *
 * @brief: Sets up the interrupt priority levels. This is to ensure that the LDMA
 *         Interrupt is handled higher than the push button
 *
 * @param[in]: none
 *
 * @return: None
 ******************************************************************************/
static void initNVIC(void)
{
	__NVIC_SetPriority(LDMA_IRQn, 0);
	__NVIC_SetPriority(GPIO_ODD_IRQn, 1);
}

/**************************************************************************//**
 * @brief Main Function
 *****************************************************************************/
int main(void)
{
  // Main loop variables
  uint16_t counter = 0;
  float frequency = 0;
  float fft_resolution = ((float)MIC_SAMPLING_FREQ_HZ/(float)FFT_SIZE);

  // Board and peripheral initialization
  CHIP_Init();        // Chip errata
  initCMU();          // Set up HF clock
  initGPIO();         // Initialize push buttons
  initPRS();          // Initialize PRS for PB0
  initNVIC();         // Set up interrupt priorities
  LCD_Init();         // Initialize the DMD module for the LCD display

  InitLDMA_MIC((uint32_t *)Micbuffer1, (uint32_t *)Micbuffer2, FFT_SIZE);
  Init_MIC();
  Init_hanning_coef();  // Initialize the Hanning window coefficients
  DSP_InitFFT_fast();   // Configure and initialize CMSIS FFT

  // Set control flags
  app_status.buffer_select        = false;
  app_status.buffer_ready         = false;
  app_status.instrument_toggle    = true;

  // Main loop
  while (1) {
    // Enter energy mode 1 until and interrupt occurs
    //   Source 1: Data is available for processing
    //   Source 2: PB1 is pressed to switch instrument
    EMU_EnterEM1();

    if (app_status.buffer_ready) {
      // Pointer to the buffer to be processed
      uint32_t *processBuffer;

      // Process the buffer not being written to by the LDMA
      if (app_status.buffer_select) {
        processBuffer = (uint32_t *)Micbuffer1;
      } else {
        processBuffer = (uint32_t *)Micbuffer2;
      }

      // Raw data processing
      for (counter = 0; counter < FFT_SIZE; counter++) {
        // Arrange microphone sample bytes as 20 bit signed data in Big endian format
        timeDomBuffer[counter] = (float) (((int32_t) __REV(processBuffer[counter])) >> 12);
      }

      // Get the frequency of the input signal
      frequency = DSP_AnalyzeData(timeDomBuffer, freqDomBuffer, fft_resolution);

      //Determine the LCD contents based on the instrument type and detected frequency
      if (app_status.instrument_toggle) {
        tuner_UI_data = getGuitarNote(frequency, fft_resolution);
      } else {
        tuner_UI_data = getUkuleleNote(frequency, fft_resolution);
      }

      LCD_update_UI_feedback(&tuner_UI_data, frequency);

      // Indicate the system is ready to process a new buffer
      app_status.buffer_ready = false;
    }
  }
}
