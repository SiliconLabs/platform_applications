/***************************************************************************//**
 * @file lcd.c
 * @brief LCD functions
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

#include <stdio.h>
#include <string.h>

#include "sl_board_control.h"
#include "em_assert.h"
#include "glib.h"
#include "dmd.h"

#include "lcd.h"

enum lcd_line{
  Line_0,
  Line_1,
  Line_2,
  Line_3,
  Line_4,
  Line_5,
  Line_6,
  Line_7,
  Line_8,
  Line_9
};

static GLIB_Context_t glibContext;

/**************************************************************************//**
 * @brief LCD initialization
 *****************************************************************************/
void init_lcd(void)
{
  uint32_t status;

  /* Enable the memory lcd */
  status = sl_board_enable_display();
  EFM_ASSERT(status == SL_STATUS_OK);

  /* Initialize the DMD support for memory lcd display */
  status = DMD_init(0);
  EFM_ASSERT(status == DMD_OK);

  /* Initialize the glib context */
  status = GLIB_contextInit(&glibContext);
  EFM_ASSERT(status == GLIB_OK);

  glibContext.backgroundColor = White;
  glibContext.foregroundColor = Black;

  /* Fill lcd with background color */
  GLIB_clear(&glibContext);

  /* Use Narrow font */
  GLIB_setFont(&glibContext, (GLIB_Font_t *) &GLIB_FontNarrow6x8);

  /* Draw text on the memory lcd display*/
  GLIB_drawStringOnLine(&glibContext,
                        "LCD clock display",
                        Line_0,
                        GLIB_ALIGN_CENTER,
                        5,
                        5,
                        true);
  GLIB_drawStringOnLine(&glibContext,
                        " Date",
                        Line_1,
                        GLIB_ALIGN_CENTER,
                        5,
                        5,
                        true);
  GLIB_drawStringOnLine(&glibContext,
                        " Time",
                        Line_3,
                        GLIB_ALIGN_CENTER,
                        5,
                        5,
                        true);

  DMD_updateDisplay();
}

/**************************************************************************//**
 * @brief Update Date on LCD
 *****************************************************************************/
void update_lcd_date(uint8_t year, uint8_t month, uint8_t day)
{
  /* Convert integer values of date to string */
  char date_str[9];

  snprintf(date_str, sizeof(date_str), "%.2d/%2.d/%.2d", month, day, year);

  /* Display string on LCD */
  GLIB_drawStringOnLine(&glibContext,
                        &date_str[0],
                        Line_2,
                        GLIB_ALIGN_CENTER,
                        5,
                        5,
                        true);

  /*Update LCD */
  DMD_updateDisplay();
}

/**************************************************************************//**
 * @brief Update Time on LCD
 *****************************************************************************/
void update_lcd_time(uint8_t hour, uint8_t min, uint8_t sec)
{
  /* Convert integer values of time to string */
  char time_str[9];

  snprintf(time_str, sizeof(time_str), "%.2d:%2.d:%.2d", hour, min, sec);

  /* Display string on LCD */
  GLIB_drawStringOnLine(&glibContext,
                        &time_str[0],
                        Line_4,
                        GLIB_ALIGN_CENTER,
                        5,
                        5,
                        true);

  /*Update LCD */
  DMD_updateDisplay();
}

/**************************************************************************//**
 * @brief Display WDOG timeout Warning on LCD
 *****************************************************************************/
void update_lcd_warning()
{
  GLIB_drawStringOnLine(&glibContext,
                        "Warning!",
                        Line_5,
                        GLIB_ALIGN_CENTER,
                        5,
                        5,
                        true);

  /*Update LCD */
  DMD_updateDisplay();
}

/**************************************************************************//**
 * @brief Indicate WDOG overflow/ system reset on LCD
 *****************************************************************************/
void update_lcd_reset()
{
  GLIB_drawStringOnLine(&glibContext,
                        "System Reset!",
                        Line_9,
                        GLIB_ALIGN_CENTER,
                        5,
                        5,
                        true);

  /*Update LCD */
  DMD_updateDisplay();
}

/**************************************************************************//**
 * @brief Clear the LCD
 *****************************************************************************/
void clear_lcd()
{
  GLIB_drawStringOnLine(&glibContext,
                        "        ",
                        Line_5,
                        GLIB_ALIGN_CENTER,
                        5,
                        5,
                        true);

  GLIB_drawStringOnLine(&glibContext,
                        "             ",
                        Line_9,
                        GLIB_ALIGN_CENTER,
                        5,
                        5,
                        true);

  DMD_updateDisplay();
}
