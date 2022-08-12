/***************************************************************************//**
* @file lcd.h
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

#ifndef LCD_H_
#define LCD_H_

/**************************************************************************//**
 * @brief LCD initialization
 *****************************************************************************/
void init_lcd(void);

/**************************************************************************//**
 * @brief Update Date on LCD
 *****************************************************************************/
void update_lcd_date(uint8_t year, uint8_t month, uint8_t day);

/**************************************************************************//**
 * @brief Update Time on LCD
 *****************************************************************************/
void update_lcd_time(uint8_t hour, uint8_t min, uint8_t sec);

/**************************************************************************//**
 * @brief Display WDOG timeout Warning on LCD
 *****************************************************************************/
void update_lcd_warning(void);

/**************************************************************************//**
 * @brief Indicate WDOG overflow/ system reset on LCD
 *****************************************************************************/
void update_lcd_reset(void);

/**************************************************************************//**
 * @brief Clear the LCD
 *****************************************************************************/
void clear_lcd(void);

#endif /* LCD_H_ */
