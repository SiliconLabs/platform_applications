/***************************************************************************//**
 * @file
 * @brief Squash demo application
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
 ******************************************************************************
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/

#ifndef SQUASH_H
#define SQUASH_H

// -----------------------------------------------------------------------------
//                                 Includes
// -----------------------------------------------------------------------------

#include "glib.h"


// -----------------------------------------------------------------------------
//                                  Macros
// -----------------------------------------------------------------------------

/*  Greeting text length  */
#define GTEXT_L 14


// -----------------------------------------------------------------------------
//                               Static Variables
// -----------------------------------------------------------------------------

/*  Greeting text  */
static const char *msg = "Hello SQUASH!";


/*  LCD context variable  */
static GLIB_Context_t g_context;

/*  Racket entity structure  */

static GLIB_Rectangle_t g_racket = { 10, 10, 13, 29 };


/*  Ball entity structure  */

static GLIB_Rectangle_t g_ball = { 50, 50, 51, 51 };

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
//                          Public Function Declaration
// -----------------------------------------------------------------------------

void sl_squash_init(void);


// -----------------------------------------------------------------------------
//                          Function Declaration
// -----------------------------------------------------------------------------

void vBlink(void *pvParameters);
void vRacket_down(void *pvParameters);
void vRacket_up(void *pvParameters);
void vBall(void *pvParameters);
void button_cb(void);

#ifdef __cplusplus
}
#endif

#endif // SQUASH_H
