/******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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
 *******************************************************************************
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Microphone Flags
 *
 */
typedef enum
{
    MICROPHONE_FLAG_NONE                        = 0,            //!< No flags
    MICROPHONE_FLAG_MONO                        = (0 << 0),     //!< Use mono (i.e. single channel)
    MICROPHONE_FLAG_STEREO                      = (1 << 0),     //!< Use stereo (i.e. dual channel, left/right)
    MICROPHONE_FLAG_MONO_DROP_OTHER_CHANNEL     = (1 << 1),     //!< DMA stereo data, but drop all data with the `dma_drop_signal` field of @ref microphone_usart_config_t
                                                                //!< This effectively creates mono data
                                                                //!< This is required by the 'Hurricane' development board
                                                                //!< The microphone always transfers stereo data but there's only one
                                                                //!< microphone so one channel is always 0.
} microphone_flag_t;

/**
 * @brief Microphone resolution
 *
 * The number of bits per sample
 */
typedef enum
{
    MICROPHONE_RESOLUTION_UNKNOWN,              //!< Unknown resolution
    MICROPHONE_RESOLUTION_16BITS    = 16,       //!< 16 bits per sample
    MICROPHONE_RESOLUTION_24BITS    = 24,       //!< 24 bits per sample
} microphone_resolution_t;

/**
 * @brief Buffer callback
 *
 * This callback is invoked in the DMA interrupt routine when audio data
 * has been DMA'd into a buffer. The `buffer` argument points to the audio data
 * and the `buffer_length` contains the number of bytes in teh buffer.
 *
 * The application should immediately copy or process the buffer as
 * the buffer will be overwritten the next time the callback is invoked.
 *
 * @note This executes in the DMA interrupt routine. Minimal processing
 *       should be done in this callback. Heavy processing should be deferred
 *       to a thread (e.g. User Thread).
 *
 * For a `mono` configuration, the buffer contains the following format:
 * @verbatim
 * <channel0><channel0><channel0>...
 * @endverbatim
 *
 * For a `stereo` configuration, the buffer contains the following format:
 * @verbatim
 * <channel0><channel1><channel0><channel1>...
 * @endverbatim
 *
 * @param[in] buffer Buffer containing audio data
 * @param[in] buffer_length Length in bytes of the supplied buffer
 */
typedef void (*microphone_buffer_callback_t)(const void* buffer, uint32_t buffer_length_bytes, void *arg);

/**
 * @brief Microphone configuration
 */
typedef struct
{
    uint32_t sample_rate;               ///< The sample or data rate of the microphone in Hertz, e.g. 16000
    uint32_t sample_size;              ///< The number of samples to collect before invoking the @ref microphone_buffer_callback_t
                                        ///< Microphone audio is continuously captured and DMA'd into two buffers
                                        ///< that 'ping-pong' back-and-forth. The `sample_count` indicates how large each buffer will be.
                                        ///< This value must be large enough such that an audio buffer can be completely processed before
                                        ///< the next buffer is filled.
    microphone_resolution_t resolution; ///< The number of bits per sample, see @ref microphone_resolution_t
    microphone_flag_t flags;            ///< Configuration flags, see @ref microphone_flag_t
    microphone_buffer_callback_t buffer_callback; ///< Callback to be invoked when an audio buffer is full, see @ref microphone_buffer_callback_t
    void *buffer_callback_arg;

    uint32_t buffer_length_bytes;
    void *buffer;
} microphone_config_t;

/**
 * @brief Initialize microphone component and I2S peripheral
 *
 * Initialize microphone component and I2S peripheral based on the
 * provided @ref microphone_config_t configuration.
 *
 * After initialization, call @ref microphone_start() to start
 * audio capture.
 *
 * Use @ref microphone_deinit() to cleanup the context.
 *
 * @param[in] config Microphone configuration.
 * @return 0 success, else failure
 */
int microphone_init(const microphone_config_t *config);

/**
 * @brief De-initialize microphone component and I2S peripheral
 *
 * Cleanup microphone component context and shutdown I2S peripheral
 */
void microphone_deinit(void);

/**
 * @brief Start audio capture
 *
 * The starts audio capture based on the provided configuration
 * to @ref microphone_init()
 *
 * Once started, audio is continuously transferred via I2S and DMA'd into a RAM buffer.
 * The RAM buffer is comprised of two equal-length buffers.
 * After one buffer is full, the configured @ref microphone_buffer_callback_t is invoked
 * in the DMA IRQ handler. While the application processes the audio buffer the
 * other buffer is filled with captured audio in the background.
 *
 * In this way, audio is continuously transferred, e.g:
 * @verbatim
 * Fill Buffer1 -> Callback
 *                 Fill Buffer2  -> Callback
 *                                  Fill Buffer1 -> Callback
 *                                                  Fill Buffer2 -> Callback
 * @endverbatim
 *
 *
 * @return 0 success, else failure
 */
int microphone_start(void);

/**
 * @brief Stop audio capture
 *
 * Stop capturing of audio via I2S
 *
 * @return 0 success, else failure
 */
int microphone_stop(void);

int microphone_set_callback_enabled(bool enabled);

#ifdef __cplusplus
}
#endif

