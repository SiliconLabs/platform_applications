/***************************************************************************//**
 * @file
 * @brief Top level application functions
 *******************************************************************************
 * # License
 * <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
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
#define FRAME_RATE_DIV      68
#define VLCD                2.50
#define ALL_SEGMENTS        1 // Set to zero to only enable one segment

/***************************************************************************//**
 * @brief  Initializes the LCD
 ******************************************************************************/
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

    // The LCD has 8 COMs and is driven with 1/4 bias (5 voltage levels)
    .mux = lcdMuxOctaplex,
    .bias = lcdBiasOneFourth,

    // In order to minimize power consumption, the low power waveform is used
    .wave = lcdWaveLowPower,

    // Step down mode selected because VDD = 3.3V and the desired VLCD is lower
    .mode = lcdModeStepDown,

    // Four Cycle charge redistribution gives the best power consumption for
    // these frame rate divider and prescaler settings
    .chargeRedistribution = lcdChargeRedistributionTwoCycle,

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

#if ALL_SEGMENTS
  // Enable common (COM0-COM7) and segment (SEG0-SEG8, SEG10-SEG13,
  // SEG16-SEG22) lines
  LCD_ComEnable(0, true);
  LCD_ComEnable(1, true);
  LCD_ComEnable(2, true);
  LCD_ComEnable(3, true);
  LCD_ComEnable(4, true);
  LCD_ComEnable(5, true);
  LCD_ComEnable(6, true);
  LCD_ComEnable(7, true);
  LCD_SegmentEnable(0, true);
  LCD_SegmentEnable(1, true);
  LCD_SegmentEnable(2, true);
  LCD_SegmentEnable(3, true);
  LCD_SegmentEnable(4, true);
  LCD_SegmentEnable(5, true);
  LCD_SegmentEnable(6, true);
  LCD_SegmentEnable(7, true);
  LCD_SegmentEnable(8, true);
  LCD_SegmentEnable(10, true);
  LCD_SegmentEnable(11, true);
  LCD_SegmentEnable(12, true);
  LCD_SegmentEnable(13, true);
  LCD_SegmentEnable(16, true);
  LCD_SegmentEnable(17, true);
  LCD_SegmentEnable(18, true);
  LCD_SegmentEnable(19, true);
  LCD_SegmentEnable(20, true);
  LCD_SegmentEnable(21, true);
  LCD_SegmentEnable(22, true);

  // Turn on all segments; demonstrates worst case current consumption
  LCD_SegmentSetLow(0, _LCD_SEGD0_MASK, 0x7F3DFF);
  LCD_SegmentSetLow(1, _LCD_SEGD1_MASK, 0x7F3DFF);
  LCD_SegmentSetLow(2, _LCD_SEGD2_MASK, 0x7F3DFF);
  LCD_SegmentSetLow(3, _LCD_SEGD3_MASK, 0x7F3DFF);
  LCD_SegmentSetLow(4, _LCD_SEGD4_MASK, 0x7F3DFF);
  LCD_SegmentSetLow(5, _LCD_SEGD5_MASK, 0x7F3DFF);
  LCD_SegmentSetLow(6, _LCD_SEGD6_MASK, 0x7F3DFF);
  LCD_SegmentSetLow(7, _LCD_SEGD7_MASK, 0x7F3DFF);
#else
  // Enable common (COM0) and segment (SEG0) lines
  LCD_ComEnable(0, true);
  LCD_SegmentEnable(0, true);

  // Turn on only one segment; best case current consumption
  LCD_SegmentSet(0, 0, true);
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
