/***************************************************************************//**
 * @file colors.h
 * @brief Header file for colors.c
 * @version v1.0
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
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

#ifndef COLORS_H_
#define COLORS_H_

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rgb_t{
  uint8_t G, R, B;
}rgb_t;

static const rgb_t red = { 0x00, 0xFF, 0x00 };
static const rgb_t green = { 0xFF, 0x00, 0x00 };
static const rgb_t blue = { 0x00, 0x00, 0xFF };
static const rgb_t yellow = { 0xFF, 0xFF, 0x00 };
static const rgb_t magenta = { 0x00, 0xFF, 0xFF };
static const rgb_t cyan = { 0xFF, 0x00, 0xFF };
static const rgb_t white = { 0xFF, 0xFF, 0xFF };
static const rgb_t black = { 0x00, 0x00, 0x00 };

rgb_t reduce_color_brightness(rgb_t color, uint8_t intensity_percentage);

#ifdef __cplusplus
}
#endif

#endif /* COLORS_H_ */
