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

#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_lcd.h"

#define LCD_PRESC           1
#define FRAME_RATE_DIV      136
#define VLCD                2.75
#define ALL_SEGMENTS        1 // Set to zero to only enable one segment

/***************************************************************************//**
* @brief  Initializes the LCD
*******************************************************************************/
void initLCD(void)
{
  // Enable clocks required for LCD
  CMU_ClockEnable(cmuClock_LFRCO, true);
  CMU_ClockEnable(cmuClock_LCD, true);
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Disable all RAM blocks except block 0
  EMU_RamPowerDown(SRAM_BASE, 0);

  // Initialize the LCD
  LCD_Init_TypeDef lcd_init = {
    // Don't enable LCD driver after init to continue configuring the LCD
    .enable = false,

    // The LCD has 4 COMs and is driven with 1/3 bias (4 voltage levels)
    .mux = lcdMuxQuadruplex,
    .bias = lcdBiasOneThird,

    // In order to minimize power consumption, the low power waveform is used
    .wave = lcdWaveLowPower,

    // Step down mode selected because VDD = 3.3V and the desired VLCD is lower
    .mode = lcdModeStepDown,

    // Four Cycle charge redistribution gives the best power consumption for
    // these frame rate divider and prescaler settings
    .chargeRedistribution = lcdChargeRedistributionFourCycle,

    // Frame rate = LCD_CLK / (8 * FRAME_RATE_DIV)
    .frameRateDivider = FRAME_RATE_DIV - 1,

    // Contrast level = (VLCD - 2.25V) / 0.05V
    .contrastLevel = (int)((VLCD - 2.25) / 0.05)
  };

  LCD_Init(&lcd_init);

  // LCD_CLK = LFRCOfreq / LCD_PRESC
  LCD->CTRL = (LCD->CTRL & (~_LCD_CTRL_PRESCALE_MASK))
              | ((LCD_PRESC - 1) << _LCD_CTRL_PRESCALE_SHIFT);

  // Select LCD VDDX to AVDD to enable step down mode
  LCD->BIASCTRL_SET = LCD_BIASCTRL_VDDXSEL_AVDD;

  // Enable common (COM0-COM3) and segment (SEG0-SEG1, SEG4-SEG7,
  // SEG18, SEG19, SEG8, SEG10) lines
  LCD_ComEnable(0, true);
  LCD_SegmentEnable(0, true);
#if ALL_SEGMENTS
  LCD_ComEnable(1, true);
  LCD_ComEnable(2, true);
  LCD_ComEnable(3, true);

  LCD_SegmentEnable(1, true);
  LCD_SegmentEnable(4, true);
  LCD_SegmentEnable(5, true);
  LCD_SegmentEnable(6, true);
  LCD_SegmentEnable(7, true);
  LCD_SegmentEnable(18, true);
  LCD_SegmentEnable(19, true);
  LCD_SegmentEnable(8, true);
  LCD_SegmentEnable(10, true);
#endif

  LCD_SegmentSet(0, 0, true);
#if ALL_SEGMENTS
  LCD_SegmentSet(0, 1, true);
  LCD_SegmentSet(0, 4, true);
  LCD_SegmentSet(0, 5, true);
  LCD_SegmentSet(0, 6, true);
  LCD_SegmentSet(0, 7, true);
  LCD_SegmentSet(0, 18, true);
  LCD_SegmentSet(0, 19, true);
  LCD_SegmentSet(0, 8, true);
  LCD_SegmentSet(0, 10, true);

  LCD_SegmentSet(1, 0, true);
  LCD_SegmentSet(1, 1, true);
  LCD_SegmentSet(1, 4, true);
  LCD_SegmentSet(1, 5, true);
  LCD_SegmentSet(1, 6, true);
  LCD_SegmentSet(1, 7, true);
  LCD_SegmentSet(1, 18, true);
  LCD_SegmentSet(1, 19, true);
  LCD_SegmentSet(1, 8, true);
  LCD_SegmentSet(1, 10, true);

  LCD_SegmentSet(2, 0, true);
  LCD_SegmentSet(2, 1, true);
  LCD_SegmentSet(2, 4, true);
  LCD_SegmentSet(2, 5, true);
  LCD_SegmentSet(2, 6, true);
  LCD_SegmentSet(2, 7, true);
  LCD_SegmentSet(2, 18, true);
  LCD_SegmentSet(2, 19, true);
  LCD_SegmentSet(2, 8, true);
  LCD_SegmentSet(2, 10, true);

  LCD_SegmentSet(3, 0, true);
  LCD_SegmentSet(3, 1, true);
  LCD_SegmentSet(3, 4, true);
  LCD_SegmentSet(3, 5, true);
  LCD_SegmentSet(3, 6, true);
  LCD_SegmentSet(3, 7, true);
  LCD_SegmentSet(3, 18, true);
  LCD_SegmentSet(3, 19, true);
  LCD_SegmentSet(3, 8, true);
  LCD_SegmentSet(3, 10, true);
#endif

  LCD_Enable(true);
}

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  initLCD();
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
}
