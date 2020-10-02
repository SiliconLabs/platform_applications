/***************************************************************************//**
* @file  lcd.h
* @brief Header and configuration file for the LCD display in the SLSTK3701A
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

#ifndef LCD_H_
#define LCD_H_

#include "../Tuned algorithm/tuned_algorithm.h"

/********************************//**
 * LCD basic coordinates
 ********************************/
#define LCD_CENTER_X    (glibContext.pDisplayGeometry->xSize / 2)
#define LCD_CENTER_Y    (glibContext.pDisplayGeometry->ySize / 2)

#define LCD_MAX_X       (glibContext.pDisplayGeometry->xSize - 1)
#define LCD_MAX_Y       (glibContext.pDisplayGeometry->ySize - 1)
#define LCD_MIN_X       0
#define LCD_MIN_Y       0

#define LCD_FONT_WIDTH  (glibContext.font.fontWidth + glibContext.font.charSpacing)
#define LCD_FONT_HEIGHT (glibContext.font.fontHeight)

/********************************//**
 * LCD shape defines
 ********************************/
#define CIRCLE_RADIUS  39

#define NOTE_X_OFFSET  4
#define NOTE_Y_OFFSET  32

#define TOP_ARROW          \
  {                        \
	8,60,8,18,3,18,12,7,   \
	21,18,16,18,16,60,8,60 \
  }

#define BOTTOM_ARROW         \
  {                          \
	8,68,8,111,3,111,12,122, \
	21,111,16,111,16,68,8,68 \
  }

/********************************//**
 * LCD string size
 ********************************/
#define LCD_MAX_STR 50

/********************************//**
 * Function prototypes
 ********************************/
void LCD_Init(void);

void LCD_update_UI_feedback(tuner_display_t *display_data, float frequency);

#endif /* LCD_H_ */
