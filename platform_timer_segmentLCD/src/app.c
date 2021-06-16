/***************************************************************************//**
 * @file
 * @brief Top level application functions
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
#include "app.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_device.h"
#include "em_emu.h"
#include "em_letimer.h"
#include "sl_segmentlcd.h"

uint32_t counter = 0;          // general time counter
uint32_t compare_mode = 0;     // enter compare mode set up
uint32_t compare_value = 0;    // compare value for compare mode
uint32_t compare_start = 0;    // start compare mode
uint32_t compare_match = 0;    // Flag triggered if counter = compare value
uint32_t letimer_enable = 0;   // LETIMER enable flag
volatile uint32_t hold_timer;  // Timer to counter hold time elapsed on PB1

/***************************************************************************//**
 * LETIEMR Interrupt Handler
 ******************************************************************************/
void LETIMER0_IRQHandler(void)
{
  // Clear all interrupt flags
  uint32_t flags = LETIMER_IntGet(LETIMER0);
  LETIMER_IntClear(LETIMER0, flags);

  // If compare match, re-configure counter as blinking flag
  if(compare_match) {
    counter = (counter+1)%2;
  }
  // If no compare match, continue counting up counter
  else {
    counter+=1;
    // hold_timer used to determine if compare mode should
    // be entered. This is triggered by holding down PB1 for
    // 2 or more seconds
    hold_timer+=1;
  }
}

/***************************************************************************//**
 * GPIO Odd Interrupt Handler
 ******************************************************************************/
void GPIO_ODD_IRQHandler(void)
{
  // Clear all interrupt flags
  uint32_t flags = GPIO_IntGet();
  GPIO_IntClear(flags);

  // Interrupt triggered by PB0 falling edge
  if(flags & GPIO_IF_EXTIF1) {
    // If in compare mode, then each press of PB0 will increment
    // compare_value by 1;
    if(compare_mode) {
      compare_value++;
    }
    // If a compare_match happened, that is counter == compare_value
    // then reset compare_match variable and counter variable
    else if (compare_match) {
      compare_match = 0;
      counter = 0;
    }
    // For all other cases, PB0 is used to start/stop the LETIMER
    else {
      letimer_enable = (letimer_enable+1)%2;
      LETIMER_Enable(LETIMER0, letimer_enable);
    }
  }

  // Interrupt triggered by PB1 falling edge
  else if(flags & GPIO_IF_EXTIF5) {
    LETIMER_Enable(LETIMER0, true);  // Need LETIMER running to detect compare
                                     // mode
    // If in compare mode set up
    if(compare_mode) {
      compare_start = 1;  // start compare mode
      compare_mode = 0;   // reset compare_mode variable
      letimer_enable = 0; // reset LETIMER flag
      LETIMER_Enable(LETIMER0, false);  // If in compare mode, disable LETIEMR
    }
    // For all other cases, PB1 will act as a timer reset
    else {
      compare_match = 0;
      letimer_enable = 0;
    }
    counter = 0;
  }
}

/***************************************************************************//**
 * Initialize CMU
 ******************************************************************************/
void initCMU(void)
{
  CMU_ClockEnable(cmuClock_LETIMER0, true);
  CMU_ClockEnable(cmuClock_GPIO, true);
}

/***************************************************************************//**
 * Initialize GPIO
 ******************************************************************************/
void initGPIO(void){
  // Configure PB1(Push Button 0) as input with filter enabled
  // Enable PB1 interrupt on falling edge
  GPIO_PinModeSet(gpioPortB, 1, gpioModeInput, 1);
  GPIO_ExtIntConfig(gpioPortB, 1, 1, false, true, true);

  // Configure PA5(Push Button 1) as input with filter enabled
  // Enable PA5 interrupt on falling edge
  GPIO_PinModeSet(gpioPortA, 5, gpioModeInput, 1);
  GPIO_ExtIntConfig(gpioPortA, 5, 5, false, true, true);

  // Enable GPIO odd IRQHandler
  NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
  NVIC_EnableIRQ(GPIO_ODD_IRQn);
}

/***************************************************************************//**
 * Initialize LETIMER.
 ******************************************************************************/
void initLetimer(void)
{
  // LETIMER initialization
  // Top value = LF Clock frequency, compare match frequency = 1 HZ
  LETIMER_Init_TypeDef initLetimer = LETIMER_INIT_DEFAULT;
  initLetimer.enable = false;  // Do not enable LETIMER when initializing
  initLetimer.topValue = 32768;
  LETIMER_Init(LETIMER0, &initLetimer);

  LETIMER_IntDisable(LETIMER0, _LETIMER_IEN_MASK);  // Disable all interrupts
  LETIMER_IntEnable(LETIMER0, LETIMER_IEN_COMP0);  // Enable compare match 0

  NVIC_ClearPendingIRQ(LETIMER0_IRQn);
  NVIC_EnableIRQ(LETIMER0_IRQn);
}
/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  // Initialize peripheral clocks
  initCMU();

  // GPIO configuration
  initGPIO();

  // LETIMER configuration
  initLetimer();

  // Configure LCD
  // Default display value 0
  SegmentLCD_Init(true);
  SegmentLCD_Number(0);
}

/***************************************************************************//**
 * compare mode setup
 ******************************************************************************/
void counter_compare(void)
{
  compare_mode = 1;  // Enter compare mode
  compare_value = 0; // reset compare_value
  counter = 0;       // reset timer counter value
  SegmentLCD_Number(0);                    // reset display value
  SegmentLCD_Symbol(LCD_SYMBOL_P5, true);  // display compare symbol
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
  // Enter EM2 to save power
  EMU_EnterEM2(true);

  // compare mode
  if(compare_mode) {
    // Display compare value and compare symbol
    SegmentLCD_Number(compare_value);
    SegmentLCD_Symbol(LCD_SYMBOL_P5, true);
  }
  // compare mode started
  else if(compare_start) {
    SegmentLCD_Number(counter);  // display timer counter
    // Check if compare match happened
    if(counter == compare_value) {
      compare_start = 0;  // disable compare start
      compare_match = 1;  // compare match
    }
  }
  // check if compare mode set up should be entered
  else if(GPIO_PinInGet(gpioPortA, 5) == 0) {
    SegmentLCD_Number(0);
    hold_timer = 0;
    while(GPIO_PinInGet(gpioPortA, 5) == 0){}
    if(hold_timer > 1) {
      counter_compare();
    }
    // Once completed, disable LETIMER to allow reset
    letimer_enable = 0;
    counter = 0;
    LETIMER_Enable(LETIMER0, letimer_enable);
  }
  // compare match
  else if(compare_match) {
    // reset compare mode related variables
    compare_start = 0;
    compare_mode = 0;
    // counter now behaves as a blinking flag, the LCD display will blink every
    // second to indicate that a compare match happened.
    if(counter) {
      SegmentLCD_AllOff();
    }
    else {
      SegmentLCD_Number(compare_value);
    }
  }
  // regular timer mode
  else {
      SegmentLCD_Number(counter);
  }
}
