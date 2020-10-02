/***************************************************************************//**
* @file  tuned_algorithm.h
* @brief Header file used to specify the typical frequencies of the Ukulele and Guitar strings
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

#ifndef TUNED_ALGORITHM_H_
#define TUNED_ALGORITHM_H_

#include <stdbool.h>

// Ukulele fundamental notes:
//   -- G4, C4, E4, A4

// Guitar fundamental notes:
//   -- E2, A2, D3, G3, B3, E4

/********************************//**
 * String define macros - Ukulele
 ********************************/
// Fundamental notes (Adjusted to the closest integer value for a resolution of 3.91 Hz for the FFT)

#define UK_G4 390  // Real frequency: 392.00
#define UK_C4 261  // Real frequency: 261.63
#define UK_E4 328  // Real frequency: 329.63
#define UK_A4 441  // Real frequency: 440.00

/********************************//**
 * String define macros - Guitar
 ********************************/
// Fundamental notes (Adjusted to the closest integer value for a resolution of 3.91 Hz for the FFT)
#define GT_E2 82   // Real frequency: 82.41
#define GT_A2 109  // Real frequency: 110
#define GT_D3 148  // Real frequency: 146.83
#define GT_G3 195  // Real frequency: 196
#define GT_B3 246  // Real frequency: 246.94
#define GT_E4 328  // Real frequency: 329.63

/********************************//**
 * Type definitions
 ********************************/
typedef enum {
  red,
  yellow,
  green
} color_display_t;

typedef enum {
  up,
  down,
  both,
  none
} arrow_display_t;

typedef struct {
  bool             instrument;   // Indicates the type of instrument being tuned (true: guitar, false: ukulele)
  arrow_display_t  arrow_dir;    // Indicates the direction of the arrow/s
  bool             double_text;  // Indicates if double text is required (false: single, true: double)
  char             text1[3];     // Holds the text to be displayed
  char             text2[3];     // Holds the text to be displayed (Valid for double_text)
  color_display_t  color;        // Indicates the color to be displayed
} tuner_display_t;


/********************************//**
 * Function prototypes
 ********************************/
tuner_display_t getUkuleleNote(float frequency, float resolution);

tuner_display_t getGuitarNote(float frequency, float resolution);

#endif /* TUNED_ALGORITHM_H_ */
