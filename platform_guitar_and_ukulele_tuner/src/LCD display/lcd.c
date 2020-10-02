/***************************************************************************//**
* @file  lcd.c
* @brief Source file for the LCD display in the SLSTK3701A
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

#include <string.h>
#include <stdio.h>
#include "lcd.h"
#include "display.h"
#include "dmd.h"
#include "glib.h"
#include "bspconfig.h"

/********************************//**
 * Global variables
 ********************************/
// glib context configuration
static GLIB_Context_t glibContext;
// Status variable for debugging purposes
static EMSTATUS status;

/********************************//**
 * Static function prototypes
 ********************************/
static void LCD_buildHomeScreen(void);

/**************************************************************************//**
 * @name: LCD_Init
 *
 * @brief: Initializes the LCD screen and displays the home screen
 *
 * @param[in]: none
 *
 * @return: none
 *****************************************************************************/
void LCD_Init(void)
{
  // Initialize the DMD module for the DISPLAY device driver.
  status = DMD_init(0);
  if (DMD_OK != status) {
    while (1) {
    };
  }

  // Initialize GLIB
  status = GLIB_contextInit(&glibContext);
  if (GLIB_OK != status) {
    while (1) {
	};
  }

  LCD_buildHomeScreen();
}

/**************************************************************************//**
 * @name: LCD_buildHomeScreen
 *
 * @brief: Generates and update sthe LCD screen to display the home screen
 *         of the Guitar Tuner application
 *
 * @param[in]: none
 *
 * @return: none
 *****************************************************************************/
static void LCD_buildHomeScreen(void)
{
  char str[LCD_MAX_STR];

  // Set default colors
  glibContext.backgroundColor = Black;
  glibContext.foregroundColor = White;

  // Clear display
  GLIB_clear(&glibContext);

  // Display start screen, set font to narrow after title
  glibContext.font = GLIB_FontNormal8x8;
  GLIB_drawString(&glibContext,
                  "Guitar",
                  strlen("Guitar"),
                  LCD_CENTER_X - ((LCD_FONT_WIDTH * strlen("Guitar")) / 2),
                  10,
                  true);
  GLIB_drawString(&glibContext,
	              "&",
                  strlen("&"),
                  LCD_CENTER_X - ((LCD_FONT_WIDTH * strlen("&")) / 2),
                  10+LCD_FONT_HEIGHT,
                  true);
  GLIB_drawString(&glibContext,
                  "Ukulele tuner",
                  strlen("Ukulele tuner"),
                  LCD_CENTER_X - ((LCD_FONT_WIDTH * strlen("Ukulele tuner")) / 2),
                  10+LCD_FONT_HEIGHT*2,
                  true);

  glibContext.font = GLIB_FontNarrow6x8;
  strncpy(str, "BTN0 to start\nBTN1 to switch", sizeof("BTN0 to start\nBTN1 to switch"));
  GLIB_drawString(&glibContext,
                  str,
                  strlen(str),
                  12,
                  108,
                  true);

  DMD_updateDisplay();
}

/***************************************************************************//**
 * @name: LCD_update_UI_feedback
 *
 * @brief: Update the frequency value displayed in the LCD screen
 *
 * @param[in]:
 * 		frequency: Float value representing the frequency of the input signal
 *
 * @return: None
 ******************************************************************************/
void LCD_update_UI_feedback(tuner_display_t *display_data, float frequency)
{
  // String buffers
  char str[LCD_MAX_STR];

  // Arrow buffer
  int32_t arrow_top[16] = TOP_ARROW;
  int32_t arrow_bottom[16] = BOTTOM_ARROW;

  glibContext.backgroundColor = White;
  glibContext.foregroundColor = Black;

  // Clear display
  GLIB_clear(&glibContext);

  // Draw the corresponding circle
  switch (display_data->color) {
    case red:
      glibContext.foregroundColor = Red;
      break;

    case yellow:
      glibContext.foregroundColor = Yellow;
      break;

    case green:
      glibContext.foregroundColor = Green;
      break;

    default:
      break;
  }
  GLIB_drawCircleFilled(&glibContext,
                        LCD_CENTER_X,
                        LCD_CENTER_Y,
                        CIRCLE_RADIUS);

  // Draw the corresponding arrows
  glibContext.foregroundColor = Blue;

  switch(display_data->arrow_dir) {
    case up:
      GLIB_drawPolygonFilled(&glibContext, 8, arrow_top);
      break;

    case down:
      GLIB_drawPolygonFilled(&glibContext, 8, arrow_bottom);
      break;

    case both:
      GLIB_drawPolygonFilled(&glibContext, 8, arrow_top);
      GLIB_drawPolygonFilled(&glibContext, 8, arrow_bottom);
      break;

    case none:
      break;
  }

  // Print the corresponding text
  glibContext.font = GLIB_FontNormal8x8;
  glibContext.foregroundColor = Black;

  // Tune type
  if (display_data->instrument) {
    strncpy(str, "Guitar tuner", sizeof("Guitar tuner"));
  }	else {
    strncpy(str, "Ukulele tuner", sizeof("Ukulele tuner"));
  }

  GLIB_drawString(&glibContext,
                  str,
                  strlen(str),
                  8 + LCD_CENTER_X - ((LCD_FONT_WIDTH * strlen(str)) / 2),
                  5,
                  true);

  // Tuning note - Double text case
  if (display_data->double_text) {
    strncpy(str, display_data->text1, sizeof(display_data->text1));
    GLIB_drawString(&glibContext,
                    str,
                    strlen(str),
                    LCD_CENTER_X + CIRCLE_RADIUS + NOTE_X_OFFSET,
                    LCD_CENTER_Y + NOTE_Y_OFFSET - (LCD_FONT_HEIGHT/2),
                    true);

    strncpy(str, display_data->text2, sizeof(display_data->text2));
    GLIB_drawString(&glibContext,
                    str,
                    strlen(str),
                    LCD_CENTER_X + CIRCLE_RADIUS + NOTE_X_OFFSET,
                    LCD_CENTER_Y - NOTE_Y_OFFSET - (LCD_FONT_HEIGHT/2),
                    true);
  }	else { // Tuning note - Single text
    strncpy(str, display_data->text1, sizeof(display_data->text1));
    GLIB_drawString(&glibContext,
                    str,
                    strlen(str),
                    LCD_CENTER_X + CIRCLE_RADIUS + NOTE_X_OFFSET,
                    LCD_CENTER_Y - (LCD_FONT_HEIGHT/2),
                    true);
  }

  // Print the measured frequency
  sprintf(str, "%.2f Hz", frequency);
  GLIB_drawString(&glibContext,
                  str,
                  strlen(str),
                  LCD_CENTER_X - ((LCD_FONT_WIDTH * strlen(str)) / 2),
                  115,
                  true);

  // Refresh the display
  DMD_updateDisplay();
}
