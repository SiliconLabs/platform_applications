/***************************************************************************//**
 * @file rtcc.h
 * @brief RTCC functions
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

#ifndef RTCC_H_
#define RTCC_H_

#include <stdint.h>
#include <stdbool.h>

/* HH:MM:SS in 24hr format*/
#define TIME_RTCC (0x122000)

/* YY:MM:DD*/
#define DATE_RTCC (0x210610)

struct time_s
{
  uint8_t year;
  uint8_t month;
  uint8_t day_of_month;

  uint8_t hour;
  uint8_t min;
  uint8_t sec;
};

extern struct time_s g_time_info;

/**************************************************************************//**
 * @brief RTCC initialization
 *****************************************************************************/
void init_rtcc(void);

/**************************************************************************//**
 * @brief Read Date from RTCC registers
 *****************************************************************************/
void get_date(struct time_s *);

/**************************************************************************//**
 * @brief Read Date from RTCC registers
 *****************************************************************************/
void get_time(struct time_s *);

/**************************************************************************//**
 * @brief Wrapper function to return flag
 *****************************************************************************/
uint32_t get_flag_time(void);

/**************************************************************************//**
 * @brief Wrapper function to reset flag
 *****************************************************************************/
void reset_flag_time(void);

#endif /* RTCC_H_ */
