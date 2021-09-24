/***************************************************************************//**
 * @file
 * @brief Segment LCD Config for the EFR32FG23B_BRD2600A starter kit
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
 ******************************************************************************/

#ifndef SL_SEGMENTLCD_CONFIG_H
#define SL_SEGMENTLCD_CONFIG_H

#include "em_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

// Define the LCD module type
#define LCD_MODULE_CE322_1002

// LCD Controller Prescaler (divide LCDCLK / 64)
// LCDCLK_pre = 512 Hz
// Set FDIV=1, means 512/2 = 256 Hz
// With quadruplex mode, 256/8 => 32 Hz Frame Rate
#define LCD_CLK_PRE         64
#define LCD_FRAME_RATE_DIV   1

#define LCD_BOOST_CONTRAST      0x1D

// LCD initialization structure
#define LCD_INIT_DEF                \
  { true,                           \
    lcdMuxQuadruplex,               \
    lcdBiasOneThird,                \
    lcdWaveLowPower,                \
    lcdModeChargePump,              \
    lcdChargeRedistributionDisable, \
    LCD_FRAME_RATE_DIV,             \
    LCD_DEFAULT_CONTRAST,           \
    LCD_CLK_PRE                     \
  }

// Range of symbols available on display
typedef enum {
  LCD_SYMBOL_P2, //!< LCD_SYMBOL_P2
  LCD_SYMBOL_P3, //!< LCD_SYMBOL_P3
  LCD_SYMBOL_P4, //!< LCD_SYMBOL_P4
  LCD_SYMBOL_P5, //!< LCD_SYMBOL_P5
  LCD_SYMBOL_DEGC//!< LCD_SYMBOL_DEGC
} lcdSymbol;

#define LCD_SYMBOL_P2_COM  0
#define LCD_SYMBOL_P2_SEG  5
#define LCD_SYMBOL_P3_COM  0
#define LCD_SYMBOL_P3_SEG  7
#define LCD_SYMBOL_P4_COM  0
#define LCD_SYMBOL_P4_SEG  8
#define LCD_SYMBOL_P5_COM  3
#define LCD_SYMBOL_P5_SEG  19
#define LCD_SYMBOL_DEGC_COM  3
#define LCD_SYMBOL_DEGC_SEG  19

// Multiplexing table: See board schematic for more details
#define EFM_DISPLAY_DEF {                                         \
    .Number      = {                                              \
      { /* #5 location : Segments 5A/5B/5C/5D/5E/5F/5G */         \
        .com[0] = 3, .com[1] = 2, .com[2] = 1, .com[3] = 0,       \
        .bit[0] = 10, .bit[1] = 10, .bit[2] = 10, .bit[3] = 10,   \
        .com[4] = 1, .com[5] = 3, .com[6] = 2,                    \
        .bit[4] = 8, .bit[5] = 8, .bit[6] = 8,                    \
      },                                                          \
      { /* #4 location : Segments 4A/4B/4C/4D/4E/4F/4G */         \
        .com[0] = 3, .com[1] = 2, .com[2] = 0, .com[3] = 0,       \
        .bit[0] = 18, .bit[1] = 19, .bit[2] = 19, .bit[3] = 18,   \
        .com[4] = 1, .com[5] = 2, .com[6] = 1,                    \
        .bit[4] = 18, .bit[5] = 18, .bit[6] = 19,                 \
      },                                                          \
      { /* #3 location : Segments 3A/3B/3C/3D/3E/3F/3G */         \
        .com[0] = 3, .com[1] = 3, .com[2] = 1, .com[3] = 0,       \
        .bit[0] = 6, .bit[1] = 7, .bit[2] = 7, .bit[3] = 6,       \
        .com[4] = 1, .com[5] = 2, .com[6] = 2,                    \
        .bit[4] = 6, .bit[5] = 6, .bit[6] = 7,                    \
      },                                                          \
      { /* #2 location : Segments 2A/2B/2C/2D/2E/2F/2G */         \
        .com[0] = 3, .com[1] = 3, .com[2] = 1, .com[3] = 0,       \
        .bit[0] = 4, .bit[1] = 5, .bit[2] = 5, .bit[3] = 4,       \
        .com[4] = 1, .com[5] = 2, .com[6] = 2,                    \
        .bit[4] = 4, .bit[5] = 4, .bit[6] = 5,                    \
      },                                                          \
      { /* #1 location : Segments 1A/1B/1C/1D/1E/1F/1G */         \
        .com[0] = 3, .com[1] = 2, .com[2] = 0, .com[3] = 0,       \
        .bit[0] = 0, .bit[1] = 1, .bit[2] = 1, .bit[3] = 0,       \
        .com[4] = 1, .com[5] = 2, .com[6] = 1,                    \
        .bit[4] = 0, .bit[5] = 0, .bit[6] = 1,                    \
      }                                                           \
    }                                                             \
}

// Utility Macros
#define LCD_NUMBER_OFF()                  \
  do {                                    \
    LCD_SegmentSetLow(0, 0xFFFFF, 0x0000); \
    LCD_SegmentSetLow(1, 0xFFFFF, 0x0000); \
    LCD_SegmentSetLow(2, 0xFFFFF, 0x0000); \
    LCD_SegmentSetLow(3, 0xFFFFF, 0x0000); \
  } while (0)

#define LCD_ALL_SEGMENTS_OFF()            \
  do {                                    \
    LCD_SegmentSetLow(0, 0xFFFFF, 0x0000); \
    LCD_SegmentSetLow(1, 0xFFFFF, 0x0000); \
    LCD_SegmentSetLow(2, 0xFFFFF, 0x0000); \
    LCD_SegmentSetLow(3, 0xFFFFF, 0x0000); \
  } while (0)

#define LCD_ALL_SEGMENTS_ON()                 \
    do {                                      \
      LCD_SegmentSetLow(0, 0xFFFFF, 0xFFFFF); \
      LCD_SegmentSetLow(1, 0xFFFFF, 0xFFFFF); \
      LCD_SegmentSetLow(2, 0xFFFFF, 0xFFFFF); \
      LCD_SegmentSetLow(3, 0xFFFFF, 0xFFFFF); \
    } while (0)

#define LCD_SEGMENTS_ENABLE() \
  do {                        \
    ;                         \
  } while (0)

#define LCD_DISPLAY_ENABLE() \
  do {                       \
    ;                        \
  } while (0)


#ifdef __cplusplus
}
#endif

#endif