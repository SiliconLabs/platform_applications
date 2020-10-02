/***************************************************************************//**
* @file  tuned_algorithm.c
* @brief Algorithm functions used to determine if the measured frequency is in tune or not
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

#include "tuned_algorithm.h"
#include "em_core.h"
#include <string.h>

static tuner_display_t display_config;

/***************************************************************************//**
 * @name: getUkuleleNote
 *
 * @brief: Heuristic algorithm to determine the closest note for the standard ukulele
 *         tuning. The output of this function is used to define the information to be
 *         displayed in the LCD screen of the SLSTK
 *
 * @param[in]:
 * 		frequency: float value representing the frequency of the input signal
 * 		resolution: tuner resolution
 *
 * @return:
 * 		UI_struct: tuner_display_t structure
 ******************************************************************************/
tuner_display_t getUkuleleNote(float frequency, float resolution)
{
  display_config.instrument = false;

  // Heuristic resolution tree
  //   Corner cases
  if (frequency < (UK_C4-(resolution*2))){
    display_config.arrow_dir   = up;
    display_config.double_text = false;
    display_config.color       = red;
    strcpy(display_config.text1, "C4");

    return display_config;
  }
  if (frequency > (UK_A4+(resolution*2))){
    display_config.arrow_dir   = down;
    display_config.double_text = false;
    display_config.color       = red;
    strcpy(display_config.text1, "A4");

    return display_config;
  }

  //   Second level (C4, E4, G4, A4)
  if (frequency < (UK_C4+(resolution*3))) {       // C4 note
    if (frequency < UK_C4){
      display_config.arrow_dir   = up;
      display_config.double_text = false;
      display_config.color       = yellow;
      strcpy(display_config.text1, "C4");
    } else if ((uint32_t)frequency == UK_C4) {
      display_config.arrow_dir   = none;
      display_config.double_text = false;
      display_config.color       = green;
      strcpy(display_config.text1, "C4");
    } else {
      display_config.arrow_dir   = down;
      display_config.double_text = false;
      display_config.color       = yellow;
      strcpy(display_config.text1, "C4");
    }
  } else if (frequency < (UK_E4+(resolution*3))) {  // E4 note
      if (frequency > (UK_E4-(resolution*3))) {
        if (frequency < UK_E4) {
          display_config.arrow_dir   = up;
          display_config.double_text = false;
          display_config.color       = yellow;
          strcpy(display_config.text1, "E4");
        } else if ((uint32_t)frequency == UK_E4) {
          display_config.arrow_dir   = none;
          display_config.double_text = false;
          display_config.color       = green;
          strcpy(display_config.text1, "E4");
        } else {
          display_config.arrow_dir   = down;
          display_config.double_text = false;
          display_config.color       = yellow;
          strcpy(display_config.text1, "E4");
        }
      } else {
        display_config.arrow_dir   = both;
        display_config.double_text = true;
        display_config.color       = red;
        strcpy(display_config.text1, "C4");
        strcpy(display_config.text2, "E4");
      }
  } else if (frequency < (UK_G4+(resolution*3))) { // G4 note
      if (frequency > (UK_G4-(resolution*3))) {
        if (frequency < UK_G4) {
          display_config.arrow_dir   = up;
          display_config.double_text = false;
          display_config.color       = yellow;
          strcpy(display_config.text1, "G4");
        } else if ((uint32_t)frequency == UK_G4) {
          display_config.arrow_dir   = none;
          display_config.double_text = false;
          display_config.color       = green;
          strcpy(display_config.text1, "G4");
        } else {
          display_config.arrow_dir   = down;
          display_config.double_text = false;
          display_config.color       = yellow;
          strcpy(display_config.text1, "G4");
        }
      } else {
        display_config.arrow_dir   = both;
        display_config.double_text = true;
        display_config.color       = red;
        strcpy(display_config.text1, "E4");
        strcpy(display_config.text2, "G4");
      }
  } else {                                      // A4 note
      if (frequency > (UK_A4-(resolution*3))) {
        if (frequency < UK_A4) {
          display_config.arrow_dir   = up;
          display_config.double_text = false;
          display_config.color       = yellow;
          strcpy(display_config.text1, "A4");
        } else if ((uint32_t)frequency == UK_A4) {
          display_config.arrow_dir   = none;
          display_config.double_text = false;
          display_config.color       = green;
          strcpy(display_config.text1, "A4");
        } else {
          display_config.arrow_dir   = down;
          display_config.double_text = false;
          display_config.color       = yellow;
          strcpy(display_config.text1, "A4");
        }
      } else {
        display_config.arrow_dir   = both;
        display_config.double_text = true;
        display_config.color       = red;
        strcpy(display_config.text1, "G4");
        strcpy(display_config.text2, "A4");
      }
  }

  return display_config;
}

/***************************************************************************//**
 * @name: getUkuleleNote
 *
 * @brief: Heuristic algorithm to determine the closest note for the standard ukulele
 *         tuning. The output of this function is used to define the information to be
 *         displayed in the LCD screen of the SLSTK
 *
 * @param[in]:
 * 		frequency: float value representing the frequency of the input signal
 * 		resolution: tuner resolution
 *
 * @return:
 * 		UI_struct: tuner_display_t structure
 ******************************************************************************/
tuner_display_t getGuitarNote(float frequency, float resolution)
{
  display_config.instrument = true;

  // Heuristic resolution tree
  //   Corner cases
  if (frequency < (GT_E2 - resolution)) {
    display_config.arrow_dir   = up;
    display_config.double_text = false;
    display_config.color       = red;
    strcpy(display_config.text1, "E2");

    return display_config;
  }
  if (frequency > (GT_E4+(resolution*2))) {
    display_config.arrow_dir   = down;
    display_config.double_text = false;
    display_config.color       = red;
    strcpy(display_config.text1, "E4");

    return display_config;
  }

  //  Second level (E2, A2, D3, G3, B3, E4)
  if (frequency < (GT_E2+(resolution*2))) {         // E2 note
    if (frequency < GT_E2) {
      display_config.arrow_dir   = up;
      display_config.double_text = false;
      display_config.color       = yellow;
      strcpy(display_config.text1, "E2");
    } else if ((uint32_t)frequency == GT_E2) {
      display_config.arrow_dir   = none;
      display_config.double_text = false;
      display_config.color       = green;
      strcpy(display_config.text1, "E2");
    } else {
      display_config.arrow_dir   = down;
      display_config.double_text = false;
      display_config.color       = yellow;
      strcpy(display_config.text1, "E2");
    }
  }	else if (frequency < (GT_A2+(resolution*3))) {  // A2 note
      if (frequency > (GT_A2-(resolution*3))) {
        if (frequency < GT_A2) {
          display_config.arrow_dir   = up;
          display_config.double_text = false;
          display_config.color       = yellow;
          strcpy(display_config.text1, "A2");
        } else if ((uint32_t)frequency == GT_A2) {
          display_config.arrow_dir   = none;
          display_config.double_text = false;
          display_config.color       = green;
          strcpy(display_config.text1, "A2");
        } else {
          display_config.arrow_dir   = down;
          display_config.double_text = false;
          display_config.color       = yellow;
          strcpy(display_config.text1, "A2");
        }
      } else {
        display_config.arrow_dir   = both;
        display_config.double_text = true;
        display_config.color       = red;
        strcpy(display_config.text1, "E2");
        strcpy(display_config.text2, "A2");
      }
  } else if (frequency < (GT_D3+(resolution*3))) {  // D3 note
      if (frequency > (GT_D3-(resolution*3))) {
        if (frequency < GT_D3) {
          display_config.arrow_dir   = up;
          display_config.double_text = false;
          display_config.color       = yellow;
          strcpy(display_config.text1, "D3");
        } else if ((uint32_t)frequency == GT_D3) {
          display_config.arrow_dir   = none;
          display_config.double_text = false;
          display_config.color       = green;
          strcpy(display_config.text1, "D3");
        } else {
          display_config.arrow_dir   = down;
          display_config.double_text = false;
          display_config.color       = yellow;
          strcpy(display_config.text1, "D3");
        }
      } else {
        display_config.arrow_dir   = both;
        display_config.double_text = true;
        display_config.color       = red;
        strcpy(display_config.text1, "A2");
        strcpy(display_config.text2, "D3");
      }
  } else if (frequency < (GT_G3+(resolution*3))) {  // G3 note
      if (frequency > (GT_G3-(resolution*3))) {
        if (frequency < GT_G3) {
          display_config.arrow_dir   = up;
          display_config.double_text = false;
          display_config.color       = yellow;
          strcpy(display_config.text1, "G3");
        } else if ((uint32_t)frequency == GT_G3) {
          display_config.arrow_dir   = none;
          display_config.double_text = false;
          display_config.color       = green;
          strcpy(display_config.text1, "G3");
        } else {
          display_config.arrow_dir   = down;
          display_config.double_text = false;
          display_config.color       = yellow;
          strcpy(display_config.text1, "G3");
        }
      } else {
        display_config.arrow_dir   = both;
        display_config.double_text = true;
        display_config.color       = red;
        strcpy(display_config.text1, "D3");
        strcpy(display_config.text2, "G3");
      }
  } else if (frequency < (GT_B3+(resolution*3))) {  // B3 note
      if (frequency > (GT_B3-(resolution*3))) {
        if (frequency < GT_B3) {
          display_config.arrow_dir   = up;
          display_config.double_text = false;
          display_config.color       = yellow;
          strcpy(display_config.text1, "B3");
        } else if ((uint32_t)frequency == GT_B3) {
          display_config.arrow_dir   = none;
          display_config.double_text = false;
          display_config.color       = green;
          strcpy(display_config.text1, "B3");
        } else {
          display_config.arrow_dir   = down;
          display_config.double_text = false;
          display_config.color       = yellow;
          strcpy(display_config.text1, "B3");
        }
      } else {
        display_config.arrow_dir   = both;
        display_config.double_text = true;
        display_config.color       = red;
        strcpy(display_config.text1, "G3");
        strcpy(display_config.text2, "B3");
      }
  } else {                                        // E4 note
      if (frequency > (GT_E4-(resolution*3))) {
        if (frequency < GT_E4) {
          display_config.arrow_dir   = up;
          display_config.double_text = false;
          display_config.color       = yellow;
          strcpy(display_config.text1, "E4");
        } else if ((uint32_t)frequency == GT_E4) {
          display_config.arrow_dir   = none;
          display_config.double_text = false;
          display_config.color       = green;
          strcpy(display_config.text1, "E4");
        } else {
          display_config.arrow_dir   = down;
          display_config.double_text = false;
          display_config.color       = yellow;
          strcpy(display_config.text1, "E4");
        }
      } else {
        display_config.arrow_dir   = both;
        display_config.double_text = true;
        display_config.color       = red;
        strcpy(display_config.text1, "B3");
        strcpy(display_config.text2, "E4");
      }
  }

  return display_config;
}
