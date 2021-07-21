/***************************************************************************//**
 * @file
 * @brief Voice transmission
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
#include <app_voice.h>
#include <stdio.h>
#include <string.h>
#include "em_common.h"
#include "sl_board_control.h"
#include "sl_app_assert.h"
#include "circular_buff.h"
#include "filter.h"
#include "sl_mic.h"
#include "sl_sleeptimer.h"
#include "sl_iostream.h"

// -----------------------------------------------------------------------------
// Private macros

#define VOICE_SAMPLE_RATE_DEFAULT sr_16k
#define VOICE_CHANNELS_DEFAULT    1
#define VOICE_FILTER_DEFAULT      true
#define VOICE_ENCODE_DEFAULT      false

#define MIC_CHANNELS_MAX        2
#define MIC_SAMPLE_SIZE         2
#define MIC_SAMPLE_BUFFER_SIZE  112
#define MIC_SEND_BUFFER_SIZE    (MIC_SAMPLE_BUFFER_SIZE * MIC_SAMPLE_SIZE)
#define CIRCULAR_BUFFER_SIZE    (MIC_SAMPLE_BUFFER_SIZE * 10)

#define SR2FS(sr)               ((sr) * 1000)

/** Time (in ms) between periodic JSON template messages. */
#define JSON_TEMPLATE_INTERVAL_MS      1000

// -----------------------------------------------------------------------------
// Private type definitions

typedef struct {
  sample_rate_t sampleRate;
  uint8_t channels;
  bool filter_enabled;
} voice_config_t;

// -----------------------------------------------------------------------------
// Private variables

static biquad_t biquads[MIC_CHANNELS_MAX];
static filter_context_t filter = { 0, biquads };
static bool voice_running = false;
static int16_t mic_buffer[2 * MIC_SAMPLE_BUFFER_SIZE];
static voice_config_t voice_config;
static circular_buffer_t circular_buffer;
static const int16_t *sample_buffer;
static uint32_t frames;
static bool event_process = false;
static bool event_send = false;

static bool send_config_flag = true;
sl_sleeptimer_timer_handle_t send_config_timer;

// -----------------------------------------------------------------------------
// Private function declarations

/***************************************************************************//**
 * Process data coming from microphone.
 *
 * Depending on the configuration settings data are filtered, encoded and added
 * to a circular buffer.
 ******************************************************************************/
static void voice_process_data(void);

/***************************************************************************//**
 * Send audio data from circular buffer.
 *
 * Data are sent in packages of MIC_SEND_BUFFER_SIZE size.
 * If there is less then MIC_SEND_BUFFER_SIZE in circular buffer data will be
 * sent after next DMA readout.
 ******************************************************************************/
static void voice_send_data(void);

/***************************************************************************//**
 * Transmit voice buffer.
 ******************************************************************************/
static void voice_transmit(uint8_t *buffer, uint32_t size);

/***************************************************************************//**
 * DMA callback indicating that the buffer is ready.
 *
 * @param buffer Microphone buffer to be processed.
 ******************************************************************************/
static void mic_buffer_ready(const void *buffer, uint32_t n_frames);

/***************************************************************************//**
 * JSON send configuration timeout callback.
 ******************************************************************************/
static void send_config_callback(sl_sleeptimer_timer_handle_t *handle, void *data);

/***************************************************************************//**
 * Sends JSON configuration over iostream.
 ******************************************************************************/
static void send_json_config(void);

// -----------------------------------------------------------------------------
// Public function definitions

/***************************************************************************//**
 * Setup periodic timer for sending configuration messages.
 ******************************************************************************/
void app_config_mic(void)
{
  /* Set up periodic JSON configuration timer. */
  sl_sleeptimer_start_periodic_timer_ms(&send_config_timer, JSON_TEMPLATE_INTERVAL_MS, send_config_callback, NULL, 0, 0);

  // Send initial JSON config message
  send_json_config();
}

/***************************************************************************//**
 * JSON configuration message ticking function.
 ******************************************************************************/
void app_config_process_action(void)
{
  if (send_config_flag == true) {
    send_json_config();
    send_config_flag = false;
  }
}

/***************************************************************************//**
 * Initialize internal variables.
 ******************************************************************************/
void app_voice_init(void)
{
  cb_err_code_t err;
  voice_config.sampleRate = VOICE_SAMPLE_RATE_DEFAULT;
  voice_config.channels = VOICE_CHANNELS_DEFAULT;
  voice_config.filter_enabled = VOICE_FILTER_DEFAULT;
  err = cb_init(&circular_buffer, CIRCULAR_BUFFER_SIZE, sizeof(uint8_t));
  sl_app_assert(err == cb_err_ok,
                "[E: 0x%04x] Circular buffer init failed\n",
                (int)err);
}

/***************************************************************************//**
 * Start voice transmission.
 ******************************************************************************/
void app_voice_start(void)
{
  sl_status_t sc;
  // Check if transfer is running
  if (voice_running) {
    return;
  }
  // Power up microphone
  sc = sl_board_enable_sensor(SL_BOARD_SENSOR_MICROPHONE);
  if ( sc != SL_STATUS_OK ) {
    return;
  }
  // Microphone initialization
  sc = sl_mic_init(SR2FS(voice_config.sampleRate), voice_config.channels);
  if ( sc != SL_STATUS_OK ) {
    return;
  }
  // Start microphone sampling
  sc = sl_mic_start_streaming(mic_buffer, MIC_SAMPLE_BUFFER_SIZE / voice_config.channels, mic_buffer_ready);
  if ( sc != SL_STATUS_OK ) {
    return;
  }
  // Filter initialization
  if (voice_config.filter_enabled) {
    filter_parameters_t fp = DEFAULT_FILTER;
    fp.srate = SR2FS(voice_config.sampleRate);
    filter.ch_count = voice_config.channels;
    fil_init(&filter, &fp);
  }

  // Audio transfer started
  voice_running = true;
}

/***************************************************************************//**
 * Stop voice transmission.
 ******************************************************************************/
void app_voice_stop(void)
{
  // Check if transfer is running
  if (!voice_running) {
    return;
  }

  // Microphone deinitialization
  sl_mic_deinit();

  // Power down microphone
  sl_board_disable_sensor(SL_BOARD_SENSOR_MICROPHONE);

  // Audio transfer stopped
  voice_running = false;
}

/***************************************************************************//**
 * Voice event handler.
 ******************************************************************************/
void app_voice_process_action(void)
{
  if (event_process) {
    event_process = false;
    voice_process_data();
  }
  if (event_send) {
    event_send = false;
    voice_send_data();
  }
}

/***************************************************************************//**
 * Setter for configuration setting sample rate.
 ******************************************************************************/
void app_voice_set_sample_rate(sample_rate_t sample_rate)
{
  if ((sample_rate == sr_16k) || (sample_rate == sr_8k)) {
    voice_config.sampleRate = sample_rate;
  }
  // Otherwise leave the sample rate unchanged.
}

/***************************************************************************//**
 * Setter for configuration setting channels.
 ******************************************************************************/
void app_voice_set_channels(uint8_t channels)
{
  if ((channels > 0) && (channels <= MIC_CHANNELS_MAX)) {
    voice_config.channels = channels;
  }
  // Otherwise leave the parameter unchanged.
}

/***************************************************************************//**
 * Setter for configuration setting filter status.
 ******************************************************************************/
void app_voice_set_filter_enable(bool status)
{
  voice_config.filter_enabled = status;
}

// -----------------------------------------------------------------------------
// Private function definitions

static void voice_process_data(void)
{
  cb_err_code_t err;
  int16_t buffer[MIC_SAMPLE_BUFFER_SIZE];
  uint32_t sample_count = frames * voice_config.channels;

  // Move DMA samples to local buffer.
  for (uint32_t i = 0; i < sample_count; i++ ) {
    buffer[i] = sample_buffer[i];
  }

  if (voice_config.filter_enabled) {
    // Filter samples.
    fil_filter(&filter, buffer, buffer, frames);
  }

  err = cb_push_buff(&circular_buffer, buffer, sample_count * MIC_SAMPLE_SIZE);

  sl_app_assert(err == cb_err_ok,
                "[E: 0x%04x] Circular buffer push failed\n",
                (int)err);

  event_send = true;
}

static void voice_send_data(void)
{
  cb_err_code_t cb_error;
  uint8_t buffer[MIC_SEND_BUFFER_SIZE];

  cb_error = cb_pop_buff(&circular_buffer, buffer, MIC_SEND_BUFFER_SIZE);

  if ( cb_error == cb_err_ok ) {
    voice_transmit(buffer, MIC_SEND_BUFFER_SIZE);
    event_send = true;
  }
}

#include "ssi_comms.h"
static void voice_transmit(uint8_t *buffer, uint32_t size)
{
  // Send data using SSI v2 on default Channel
  ssiv2_publish_sensor_data(SSI_CHANNEL_DEFAULT, buffer, size);
}

static void mic_buffer_ready(const void *buffer, uint32_t n_frames)
{
  sample_buffer = (int16_t *)buffer;
  frames = n_frames;
  event_process = true;
}

static void send_config_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void)handle;
  (void)data;
  send_config_flag = true;
}

static void send_json_config()
{
#if (SSI_JSON_CONFIG_VERSION == 1)
  printf("{\"sample_rate\":%d,"
      "\"column_location\":{"
      "\"Microphone\":0},"
      "\"samples_per_packet\":%d}\n", SR2FS(VOICE_SAMPLE_RATE_DEFAULT), MIC_SAMPLE_BUFFER_SIZE);
#elif (SSI_JSON_CONFIG_VERSION == 2)
  printf("{\"version\":%d, \"sample_rate\":%d,"
      "\"column_location\":{"
      "\"Microphone\":0},"
      "\"samples_per_packet\":%d}\n", SSI_JSON_CONFIG_VERSION, SR2FS(VOICE_SAMPLE_RATE_DEFAULT), MIC_SAMPLE_BUFFER_SIZE);
#else
#error "Unknown SSI_JSON_CONFIG_VERSION"
#endif
}

