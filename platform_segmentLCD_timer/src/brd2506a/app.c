/***************************************************************************//**
 * @file
 * @brief Top level application functions
 *******************************************************************************
 * # License
 * <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
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

#define BTN0_GPIO_PORT  gpioPortB
#define BTN0_GPIO_PIN   1
#define BTN1_GPIO_PORT  gpioPortB
#define BTN1_GPIO_PIN   6

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
  if (compare_match) {
    counter = (counter + 1) % 2;
  }
  // If no compare match, continue counting up counter
  else {
    counter += 1;
    // hold_timer used to determine if compare mode should
    // be entered. This is triggered by holding down PB1 for
    // 2 or more seconds
    hold_timer += 1;
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

  // Interrupt triggered by PB0 (PB1) falling edge
  if (flags & ((1 << BTN0_GPIO_PIN) << _GPIO_IF_EXTIF0_SHIFT)) {
    // If in compare mode, then each press of PB0 will increment
    // compare_value by 1;
    if (compare_mode) {
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
      letimer_enable = (letimer_enable + 1) % 2;
      LETIMER_Enable(LETIMER0, letimer_enable);
    }
  }
}

/***************************************************************************//**
 * GPIO Even Interrupt Handler
 ******************************************************************************/
void GPIO_EVEN_IRQHandler(void)
{
  // Clear all interrupt flags
  uint32_t flags = GPIO_IntGet();
  GPIO_IntClear(flags);

  // Interrupt triggered by PB1 (PB4) falling edge
  if (flags & ((1 << BTN1_GPIO_PIN) << _GPIO_IF_EXTIF0_SHIFT)) {
    LETIMER_Enable(LETIMER0, true);  // Need LETIMER running to detect compare
                                     // mode
    // If in compare mode set up
    if (compare_mode) {
      compare_start = 1;  // start compare mode
      compare_mode = 0;   // reset compare_mode variable
      letimer_enable = 0; // reset LETIMER flag
      LETIMER_Enable(LETIMER0, false);  // If in compare mode, disable LETIMER
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
void initGPIO(void)
{
  // Configure PB1 (Push Button 0) as input with filter enabled
  // Enable PB1 interrupt on falling edge
  GPIO_PinModeSet(BTN0_GPIO_PORT, BTN0_GPIO_PIN, gpioModeInputPullFilter, 1);
  GPIO_ExtIntConfig(BTN0_GPIO_PORT, BTN0_GPIO_PIN, BTN0_GPIO_PIN, false, true,
                    true);

  // Configure PB6 (Push Button 1) as input with filter enabled
  // Enable PB6 interrupt on falling edge
  GPIO_PinModeSet(BTN1_GPIO_PORT, BTN1_GPIO_PIN, gpioModeInputPullFilter, 1);
  GPIO_ExtIntConfig(BTN1_GPIO_PORT, BTN1_GPIO_PIN, BTN1_GPIO_PIN, false, true,
                    true);

  // Enable GPIO odd IRQHandler
  NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
  NVIC_EnableIRQ(GPIO_ODD_IRQn);

  // Enable GPIO even IRQHandler
  NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
  NVIC_EnableIRQ(GPIO_EVEN_IRQn);
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
* Disable Unused LCD Segments
*******************************************************************************/
void disableUnusedLCDSeg(void)
{
/***************************************************************************//**
* The LCD driver enables all segments, even those that are not mapped to
* segments on the pro kit board. These are disabled below in order to
* minimize current consumption.
*******************************************************************************/
  LCD_SegmentEnable(SL_SEGMENT_LCD_SEG_S00, false);
  LCD_SegmentEnable(SL_SEGMENT_LCD_SEG_S01, false);
  LCD_SegmentEnable(SL_SEGMENT_LCD_SEG_S02, false);
  LCD_SegmentEnable(SL_SEGMENT_LCD_SEG_S03, false);
  LCD_SegmentEnable(SL_SEGMENT_LCD_SEG_S04, false);
  LCD_SegmentEnable(SL_SEGMENT_LCD_SEG_S05, false);
  LCD_SegmentEnable(SL_SEGMENT_LCD_SEG_S06, false);
  LCD_SegmentEnable(SL_SEGMENT_LCD_SEG_S07, false);
  LCD_SegmentEnable(SL_SEGMENT_LCD_SEG_S08, false);
  LCD_SegmentEnable(SL_SEGMENT_LCD_SEG_S09, false);
  LCD_SegmentEnable(SL_SEGMENT_LCD_SEG_S10, false);
  LCD_SegmentEnable(SL_SEGMENT_LCD_SEG_S11, false);
  LCD_SegmentEnable(SL_SEGMENT_LCD_SEG_S12, false);
  LCD_SegmentEnable(SL_SEGMENT_LCD_SEG_S13, false);
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

  // Configure LCD to use step down mode and disable unused segments
  // Default display value 0
  sl_segment_lcd_init(false);
  LCD->BIASCTRL_SET = LCD_BIASCTRL_VDDXSEL_AVDD;
  disableUnusedLCDSeg();
  sl_segment_lcd_number(0);
}

/***************************************************************************//**
 * compare mode setup
 ******************************************************************************/
void counter_compare(void)
{
  compare_mode = 1;  // Enter compare mode
  compare_value = 0; // reset compare_value
  counter = 0;       // reset timer counter value
  sl_segment_lcd_number(0);                    // reset display value
  sl_segment_lcd_symbol(SL_LCD_SYMBOL_DEGC, true);  // display compare symbol
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
  // compare mode
  if (compare_mode) {
    // Display compare value and compare symbol
    sl_segment_lcd_number(compare_value);
    sl_segment_lcd_symbol(SL_LCD_SYMBOL_DEGC, true);
  }
  // compare mode started
  else if (compare_start) {
    sl_segment_lcd_number(counter);  // display timer counter
    // Check if compare match happened
    if (counter == compare_value) {
      compare_start = 0;  // disable compare start
      compare_match = 1;  // compare match
    }
  }
  // check if compare mode set up should be entered
  else if (GPIO_PinInGet(BTN1_GPIO_PORT, BTN1_GPIO_PIN) == 0) {
    sl_segment_lcd_number(0);
    hold_timer = 0;
    while (GPIO_PinInGet(BTN1_GPIO_PORT, BTN1_GPIO_PIN) == 0) {}
    if (hold_timer > 1) {
      counter_compare();
    }
    // Once completed, disable LETIMER to allow reset
    letimer_enable = 0;
    counter = 0;
    LETIMER_Enable(LETIMER0, letimer_enable);
  }
  // compare match
  else if (compare_match) {
    // reset compare mode related variables
    compare_start = 0;
    compare_mode = 0;
    // counter now behaves as a blinking flag, the LCD display will blink every
    // second to indicate that a compare match happened.
    if (counter) {
      sl_segment_lcd_all_off();
    } else {
      sl_segment_lcd_number(compare_value);
    }
  }
  // regular timer mode
  else {
    sl_segment_lcd_number(counter);
  }
}
