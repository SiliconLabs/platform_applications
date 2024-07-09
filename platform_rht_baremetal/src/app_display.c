/***************************************************************************//**
 * @file app_display.c
 * @brief RHT Si7021 baremetal example - display.
 * @version 1.0.0
 *******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
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
 *
 * EVALUATION QUALITY
 * This code has been minimally tested to ensure that it builds with
 * the specified dependency versions and is suitable as a demonstration for
 * evaluation purposes only.
 * This code will be maintained at the sole discretion of Silicon Labs.
 *
 ******************************************************************************/
#include <sl_string.h>
#include <string.h>
#include <stdio.h>
#include "app_log.h"
#include "app_display.h"
#include "rht_lcd_bitmap.h"

// -----------------------------------------------------------------------------
//                              Macros and Typedefs
// -----------------------------------------------------------------------------
#define LCD_CENTER_X    (glib_context.pDisplayGeometry->xSize / 2)
#define LCD_CENTER_Y    (glib_context.pDisplayGeometry->ySize / 2)

#define LCD_MAX_X       (glib_context.pDisplayGeometry->xSize - 1)
#define LCD_MAX_Y       (glib_context.pDisplayGeometry->ySize - 1)

#define LCD_FONT_HEIGHT (glib_context.font.fontHeight)
#define LCD_FONT_WIDTH  (glib_context.font.fontWidth + \
                         glib_context.font.charSpacing)

// -----------------------------------------------------------------------------
//                                Static Variables
// -----------------------------------------------------------------------------
static GLIB_Context_t glib_context; // The GLIB context

// -----------------------------------------------------------------------------
//                          Public Function Definitions
// -----------------------------------------------------------------------------

/***************************************************************************//**
 * @brief
 *     Display context initialization
 ******************************************************************************/
void display_init(void)
{
  EMSTATUS status = DMD_OK;

  // Initialize the DMD module for the DISPLAY device driver
  status = DMD_init(0);
  if (status != DMD_OK) {
    app_log("[DMD] - Error 0x%lx initializing DMD driver!", status);
    while (1) {
    }
  }

  // Initialize the GLIB context
  status = GLIB_contextInit(&glib_context);
  if (status != GLIB_OK) {
    app_log("[GLIB] - Error 0x%lx initializing GLIB context!", status);
    while (1) {
    }
  }
}

/***************************************************************************//**
 * @brief
 *     Create the example main screen
 ******************************************************************************/
void display_rht_screen(void)
{
  char str[LCD_MAX_STR];
  EMSTATUS status = GLIB_OK;
  char title[] = "RHT MCU example";

  glib_context.backgroundColor = White;
  glib_context.foregroundColor = Black;

  // Clear display
  status = GLIB_clear(&glib_context);
  if (status != GLIB_OK) {
    app_log("[GLIB] - Error 0x%lx clearing display!", status);
    while (1) {
    }
  }

  // Print the heading
  glib_context.font = GLIB_FontNormal8x8;

  memcpy(str, title, sizeof(title));
  status = GLIB_drawString(&glib_context,
                           str,
                           sl_strlen(str),
                           LCD_CENTER_X - ((LCD_FONT_WIDTH * sl_strlen(
                                              str)) / 2),
                           4,
                           true);

  // Draw the RHT bitmap
  status = GLIB_drawBitmap(&glib_context,
                           ((LCD_MAX_X + 1 - RHT_BITMAP_WIDTH) / 2),
                           4 + LCD_FONT_HEIGHT,
                           RHT_BITMAP_WIDTH,
                           RHT_BITMAP_HEIGHT,
                           rht_bitmap);

  // Modify font size and draw temperature and RH data
  glib_context.font = GLIB_FontNarrow6x8;
  snprintf(str, sizeof(str), "T:%2.1f C", 0.0);
  status = GLIB_drawString(&glib_context,
                           str,
                           sl_strlen(str),
                           ((LCD_CENTER_X / 2)
                            - ((LCD_FONT_WIDTH * sl_strlen(str)) / 2)),
                           (LCD_MAX_Y - 5 - (LCD_FONT_HEIGHT / 2)),
                           true);

  snprintf(str, sizeof(str), "RH:%2.1f %%", 0.0);
  status = GLIB_drawString(&glib_context,
                           str,
                           sl_strlen(str),
                           (LCD_CENTER_X + (LCD_CENTER_X / 2) \
                            - ((LCD_FONT_WIDTH * sl_strlen(str)) / 2)),
                           (LCD_MAX_Y - 5 - (LCD_FONT_HEIGHT / 2)),
                           true);

  if (status != GLIB_OK) {
    app_log("[GLIB] - Error 0x%lx drawing to display!", status);
    while (1) {
    }
  }

  // Refresh the display
  status = DMD_updateDisplay();
  if (status != DMD_OK) {
    app_log("[DMD] - Error 0x%lx updating display!", status);
    while (1) {
    }
  }
}

/***************************************************************************//**
 * @brief
 *     Update the example main screen
 *
 * @param[in] rh_data
 *     Float value holding the relative humidity data
 *
 * @param[in] temp_data
 *     Float value holding the temperature data
 ******************************************************************************/
void display_rht_screen_update(float rh_data, float temp_data)
{
  char str[LCD_MAX_STR];
  EMSTATUS status = GLIB_OK;

  // Set a clipping region beneath the bitmap and clear it
  GLIB_Rectangle_t rect_clip_region = { 0,
                                        LCD_MAX_Y - 5 - LCD_FONT_HEIGHT,
                                        LCD_MAX_X,
                                        LCD_MAX_Y };

  status = GLIB_setClippingRegion(&glib_context, &rect_clip_region);
  if (status != GLIB_OK) {
    app_log("[GLIB] - Error 0x%lx setting clipping region!", status);
    while (1) {
    }
  }

  status = GLIB_clearRegion(&glib_context);
  if (status != GLIB_OK) {
    app_log("[GLIB] - Error 0x%lx clearing region!", status);
    while (1) {
    }
  }

  // Reset clipping region
  status = GLIB_resetClippingRegion(&glib_context);
  if (status != GLIB_OK) {
    app_log("[GLIB] - Error 0x%lx resetting clipping region!", status);
    while (1) {
    }
  }

  status = GLIB_applyClippingRegion(&glib_context);
  if (status != GLIB_OK) {
    app_log("[GLIB] - Error 0x%lx applying clipping region!", status);
    while (1) {
    }
  }

  // Modify font size and draw temperature and RH data
  glib_context.font = GLIB_FontNarrow6x8;
  snprintf(str, sizeof(str), "T:%2.1f C", temp_data);
  status = GLIB_drawString(&glib_context,
                           str,
                           sl_strlen(str),
                           ((LCD_CENTER_X / 2)
                            - ((LCD_FONT_WIDTH * sl_strlen(str)) / 2)),
                           (LCD_MAX_Y - 5 - (LCD_FONT_HEIGHT / 2)),
                           true);

  snprintf(str, sizeof(str), "RH:%2.1f %%", rh_data);
  status = GLIB_drawString(&glib_context,
                           str,
                           sl_strlen(str),
                           (LCD_CENTER_X + (LCD_CENTER_X / 2) \
                            - ((LCD_FONT_WIDTH * sl_strlen(str)) / 2)),
                           (LCD_MAX_Y - 5 - (LCD_FONT_HEIGHT / 2)),
                           true);

  if (status != GLIB_OK) {
    app_log("[GLIB] - Error 0x%lx drawing to display!", status);
    while (1) {
    }
  }

  // Refresh the display
  status = DMD_updateDisplay();
  if (status != DMD_OK) {
    app_log("[DMD] - Error 0x%lx updating display!", status);
    while (1) {
    }
  }
}
