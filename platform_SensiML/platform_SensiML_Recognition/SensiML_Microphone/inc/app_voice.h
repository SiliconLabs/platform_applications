/***************************************************************************//**
 * @file
 * @brief Voice transmission header
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
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

#ifndef APP_VOICE_H
#define APP_VOICE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  sr_8k = 8,
  sr_16k = 16,
} sample_rate_t;

/***************************************************************************//**
 * Setup periodic timer for sending configuration messages.
 ******************************************************************************/
void app_config_mic(void);

/***************************************************************************//**
 * JSON configuration message ticking function.
 ******************************************************************************/
void app_config_process_action(void);

/***************************************************************************//**
 * Initialize internal variables.
 ******************************************************************************/
void app_voice_init(void);

/***************************************************************************//**
 * Start voice transmission.
 ******************************************************************************/
void app_voice_start(void);

/***************************************************************************//**
 * Stop voice transmission.
 ******************************************************************************/
void app_voice_stop(void);

/***************************************************************************//**
 * Voice event handler.
 ******************************************************************************/
void app_voice_process_action(void);

/***************************************************************************//**
 * Setter for configuration setting sample rate.
 *
 * @param[in] sample_rate Sample rate to be used, see \ref sample_rate_t.
 ******************************************************************************/
void app_voice_set_sample_rate(sample_rate_t sample_rate);

/***************************************************************************//**
 * Setter for configuration setting channels.
 *
 * @param[in] channels Number of audio channels to use.
 ******************************************************************************/
void app_voice_set_channels(uint8_t channels);

/***************************************************************************//**
 * Setter for configuration setting filter status.
 *
 * @param[in] status Enable (true) or disable (false) the biquad filter.
 ******************************************************************************/
void app_voice_set_filter_enable(bool status);

#endif // APP_VOICE_H
