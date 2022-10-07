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

#define LCD_PRESC           16
#define FRAME_RATE_DIV      8
#define VLCD                3.0
#define FRAME_COUNTER_TOP   16

#define BLINK_ENABLE        0

/***************************************************************************//**
 * @brief  Initializes the LCD to enable 1 COM x 8 SEG, set the framerate to
 *         32Hz, and set the frame counter event to trigger at 1Hz.
 ******************************************************************************/
void initLCD(void)
{
  // Enable clocks required for LCD
  CMU_ClockEnable(cmuClock_LFRCO, true);
  CMU_ClockEnable(cmuClock_LCD, true);
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Initialize the LCD
  LCD_Init_TypeDef lcd_init = {
    // Don't enable LCD driver after init to continue configuring the LCD
    .enable = false,

    // The LCD has 4 COMs and is driven with 1/3 bias (4 voltage levels)
    .mux = lcdMuxQuadruplex,
    .bias = lcdBiasOneThird,

    // In order to minimize power consumption, the low power waveform is used
    .wave = lcdWaveLowPower,

    // Step down mode selected because VDD = 3.3V and the desired VLCD = 3.0V
    .mode = lcdModeStepDown,

    // Table 28.1 in the reference manual shows 0.0% charge redistribution with
    // currently configured mux and prescaler values, so just keep it disabled
    .chargeRedistribution = lcdChargeRedistributionDisable,

    // Frame rate = LCD_CLK / (8 * FRAME_RATE_DIV)
    //            =  2048Hz / (8 * 8)
    //            =  32Hz
    .frameRateDivider = FRAME_RATE_DIV - 1,

    // Contrast level = (VLCD - 2.25V) / 0.05V
    //                = (3.0V - 2.25V) / 0.05V
    //                = 15
    .contrastLevel = (int)((VLCD - 2.25) / 0.05)
  };

  LCD_Init(&lcd_init);

  // LCD_CLK = LFRCOfreq / LCD_PRESC
  //         =   32768Hz / 16
  //         =    2048Hz
  LCD->CTRL = (LCD->CTRL & (~_LCD_CTRL_PRESCALE_MASK))
              | ((LCD_PRESC - 1) << _LCD_CTRL_PRESCALE_SHIFT);

  // Select LCD VDDX to AVDD to enable step down mode
  LCD->BIASCTRL_SET = LCD_BIASCTRL_VDDXSEL_AVDD;

  // Enable common (COM0) and SEG0-7 lines
  LCD_ComEnable(0, true);
  LCD_SegmentEnable(0, true);
  LCD_SegmentEnable(1, true);
  LCD_SegmentEnable(2, true);
  LCD_SegmentEnable(3, true);
  LCD_SegmentEnable(4, true);
  LCD_SegmentEnable(5, true);
  LCD_SegmentEnable(6, true);
  LCD_SegmentEnable(7, true);

  // Animation state 0 has segment 0 on
  // Odd state will shift AReg to the left by 1 bit
  // AReg initial value is 1000 0000, BReg initial value is 0000 0000
  LCD_AnimInit_TypeDef anim_init = {
    .enable = true,
    .AReg = 0x80,
    .AShift = lcdAnimShiftLeft,
    .BReg = 0x00,
    .BShift = lcdAnimShiftNone,
    .animLogic = lcdAnimLogicOr,
    .startSeg = lcdAnimLocSeg0To7
  };

  LCD_AnimInit(&anim_init);

#if BLINK_ENABLE
  // Set BLINK_ENABLE define to 1 to enable blinking. Blinking will turn off
  // all segments every other frame counter event.
  LCD_BlinkEnable(true);
#endif

  // Frame count event frequency = frame rate / (FRAME_COUNTER_TOP * prescale)
  //                             = 32Hz       / (16                * 1)
  //                             = 2Hz
  LCD_FrameCountInit_TypeDef fc_init = {
    .enable = true,
    .top = FRAME_COUNTER_TOP,
    .prescale = lcdFCPrescDiv1
  };

  LCD_FrameCountInit(&fc_init);
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
