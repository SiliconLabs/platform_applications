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

#include <stdlib.h>
#include <string.h>
#include "em_device.h"

#include "em_cmu.h"
#include "em_ldma.h"
#include "em_usart.h"
#include "em_gpio.h"

#include "bsp.h"
#include "microphone.h"
#include "platform_dma.h"

#if defined(BSP_STK_BRD2204A)
    #include "config/bsp_microphone_stk3701a.h"
#elif defined(BSP_TBSENSE2)
    #include "config/bsp_microphone_brd4166a.h"
#else
    #error Unsupported board
#endif

#ifndef CHECK_FAILED
#define CHECK_FAILED(res, func) ((res = func) != 0)
#endif

#ifndef DIV_ROUND_UP
#define DIV_ROUND_UP(m, n)    (((m) + (n) - 1) / (n))
#endif /* ifndef DIV_ROUND_UP */

#define VERIFY_CONTEXT_INITIALIZED() \
if(microphone_context.buffer.base == NULL) \
{ \
    return -1; \
}

#define MAX_DMA_LENGTH ((_LDMA_CH_CTRL_XFERCNT_MASK >> _LDMA_CH_CTRL_XFERCNT_SHIFT)+1)

typedef enum
{
    CHANNEL_STATUS_NONE         = 0,
    CHANNEL_STATUS_LEFT_READY  = (1 << 0),
    CHANNEL_STATUS_RIGHT_READY = (1 << 1),
} channel_status_t;

typedef struct
{
    uint8_t *base;
    uint8_t *start;
    uint8_t *end;
    uint8_t *current;
    uint32_t offset;
    LDMA_Descriptor_t *dma_desc;
    uint8_t dma_ch;
} audio_buffer_t;

typedef struct
{
    uint32_t                    sample_rate;
    uint32_t                    sample_size;
    uint32_t                    sample_count;
    microphone_buffer_callback_t callback;
    void*                       callback_arg;
    audio_buffer_t              buffer;

    struct
    {
        LDMA_Descriptor_t dma_desc;
        uint8_t buffer;
        uint8_t dma_ch;
    } __ALIGNED(4) drop_channel;

    microphone_resolution_t     resolution;
    microphone_flag_t           flags;
} microphone_context_t;

static void microphone_dma_irq_callback(void *arg);
static void drop_dma_irq_callback(void *arg){};
static void dma_desc_init(uint32_t sample_count);
static int  validate_config(const microphone_config_t *config);

static microphone_context_t microphone_context;

/*************************************************************************************************/
int microphone_init(const microphone_config_t *config)
{
    int result = 0;
    uint8_t *ptr = NULL;

    if(validate_config(config) != 0)
    {
        goto exit;
    }

    const bool use_stereo = (config->flags & MICROPHONE_FLAG_STEREO);
    const bool drop_channel = (config->flags & MICROPHONE_FLAG_MONO_DROP_OTHER_CHANNEL);
    const bool alloc_audio_buffer = (config->buffer == NULL);
    const uint32_t sample_length_bytes = (config->sample_size * (config->resolution / 8));
    uint32_t buffer_length_bytes = alloc_audio_buffer ? (sample_length_bytes * 2) : config->buffer_length_bytes;

    if(alloc_audio_buffer)
    {
        if(use_stereo)
        {
            buffer_length_bytes *= 2;
        }
    }

    const uint32_t sample_count = buffer_length_bytes / sample_length_bytes;
    const uint32_t desc_per_sample = DIV_ROUND_UP(sample_length_bytes, MAX_DMA_LENGTH);
    const uint8_t dma_desc_count = desc_per_sample * sample_count;

    // Copy configuration
    microphone_context.flags           = config->flags;
    microphone_context.sample_rate     = config->sample_rate;
    microphone_context.resolution      = config->resolution;
    microphone_context.sample_size     = config->sample_size;
    microphone_context.sample_count    = sample_count;
    microphone_context.callback        = config->buffer_callback;
    microphone_context.callback_arg    = config->buffer_callback_arg;
    microphone_context.drop_channel.dma_ch = UINT8_MAX;
    microphone_context.buffer.dma_ch   = UINT8_MAX;
    microphone_context.buffer.base     = NULL;

    uint32_t alloc_size  = (sizeof(LDMA_Descriptor_t) * dma_desc_count);
    // If an audio buffer is not being provided to use
    // then we need to allocate that as well
    if(alloc_audio_buffer)
    {
        alloc_size += buffer_length_bytes;
    }

    // Allocate sample buffers
    microphone_context.buffer.base = malloc(alloc_size);
    if(microphone_context.buffer.base == NULL)
    {
        goto exit;
    }
    else if(CHECK_FAILED(result, platform_dma_alloc(microphone_dma_irq_callback,
                                             NULL,
                                             &microphone_context.buffer.dma_ch)))
    {
        goto exit;
    }
    else if(drop_channel &&
            CHECK_FAILED(result, platform_dma_alloc(drop_dma_irq_callback,
                                             NULL,
                                             &microphone_context.drop_channel.dma_ch)))
    {
        goto exit;
    }

    ptr = microphone_context.buffer.base;

    microphone_context.buffer.dma_desc = (LDMA_Descriptor_t*)ptr;
    ptr += (sizeof(LDMA_Descriptor_t) * dma_desc_count);

    ptr = alloc_audio_buffer ? ptr : config->buffer;
    memset(ptr, 0xAA, buffer_length_bytes);

    microphone_context.buffer.start = ptr;
    microphone_context.buffer.current = ptr;
    microphone_context.buffer.end = ptr + buffer_length_bytes;
    microphone_context.buffer.offset = sample_length_bytes;
    dma_desc_init(sample_count);

    // Enable clocks
    CMU_ClockEnable(cmuClock_GPIO, true);
    CMU_ClockEnable(BSP_MICROPHONE_USART_CLOCK, true);

    // Setup GPIO pins
#ifdef BSP_MICROPHONE_ENABLE_PIN
    GPIO_PinModeSet(BSP_MICROPHONE_ENABLE_PIN, gpioModePushPull, 1); // power-supply enable
#endif
    GPIO_PinModeSet(BSP_MICROPHONE_USART_CS_PIN, gpioModePushPull, 1);
    GPIO_PinModeSet(BSP_MICROPHONE_USART_CLK_PIN, gpioModePushPull, 1);
    GPIO_PinModeSet(BSP_MICROPHONE_USART_RX_PIN, gpioModeInputPullFilter, 0);

    // Initialize USART to receive data from microphones synchronously
    USART_InitI2s_TypeDef def = USART_INITI2S_DEFAULT;

    def.sync.databits   = usartDatabits16;
    def.sync.enable     = usartDisable;
    def.sync.autoTx     = true;
    def.dmaSplit        = false;
    def.mono            = (use_stereo || drop_channel) ? false : true;
    def.format          = usartI2sFormatW32D16;

    // Set baud rate to achieve desired sample frequency
    def.sync.baudrate   = microphone_context.sample_rate * 32; // I2S always uses 32bit transfers

    if(!def.mono)
    {
        def.sync.baudrate *= 2;
    }

    if(drop_channel)
    {
        def.dmaSplit = true;
    }

    USART_InitI2s(BSP_MICROPHONE_USART, &def);

    // Connect USART to IO pins
    BSP_MICROPHONE_USART->ROUTELOC0 = BSP_MICROPHONE_USART_ROUTELOC0;
    BSP_MICROPHONE_USART->ROUTEPEN = USART_ROUTEPEN_RXPEN | USART_ROUTEPEN_CLKPEN | USART_ROUTEPEN_CSPEN;

    // Stop the clock until it's needed
    CMU_ClockEnable(BSP_MICROPHONE_USART_CLOCK, false);

exit:
    if(result != 0)
    {
        microphone_deinit();
    }

    return result;
}

/*************************************************************************************************/
void microphone_deinit(void)
{
    if(microphone_context.buffer.base != NULL)
    {
        microphone_stop();

        USART_Reset(BSP_MICROPHONE_USART);

        platform_dma_free(microphone_context.buffer.dma_ch);
        microphone_context.buffer.dma_ch = UINT8_MAX;

        platform_dma_free(microphone_context.drop_channel.dma_ch);
        microphone_context.drop_channel.dma_ch = UINT8_MAX;

        free(microphone_context.buffer.base);
        microphone_context.buffer.base = NULL;
    }
}

/*************************************************************************************************/
int microphone_start(void)
{
    VERIFY_CONTEXT_INITIALIZED();
    const LDMA_TransferCfg_t dma_config  = LDMA_TRANSFER_CFG_PERIPHERAL(BSP_MICROPHONE_USART_DMA_LEFT_SIGNAL);

    CMU_ClockEnable(BSP_MICROPHONE_USART_CLOCK, true);
    USART_Enable(BSP_MICROPHONE_USART, usartEnable);

    microphone_context.buffer.current =  microphone_context.buffer.start;

    LDMA_StartTransfer(microphone_context.buffer.dma_ch, &dma_config, microphone_context.buffer.dma_desc);

    if(microphone_context.flags & MICROPHONE_FLAG_MONO_DROP_OTHER_CHANNEL)
    {
        const LDMA_TransferCfg_t drop_dma_config  = LDMA_TRANSFER_CFG_PERIPHERAL(BSP_MICROPHONE_USART_DMA_RIGHT_SIGNAL);
        LDMA_StartTransfer(microphone_context.drop_channel.dma_ch, &drop_dma_config, &microphone_context.drop_channel.dma_desc);
    }

    return 0;
}

/*************************************************************************************************/
int microphone_stop(void)
{
    VERIFY_CONTEXT_INITIALIZED();

    USART_Enable(BSP_MICROPHONE_USART, usartDisable);
    CMU_ClockEnable(BSP_MICROPHONE_USART_CLOCK, false);

    LDMA_StopTransfer(microphone_context.buffer.dma_ch);

    if(microphone_context.flags & MICROPHONE_FLAG_MONO_DROP_OTHER_CHANNEL)
    {
        LDMA_StopTransfer(microphone_context.drop_channel.dma_ch);
    }

    return 0;
}

/*************************************************************************************************/
int microphone_set_callback_enabled(bool enabled)
{
    VERIFY_CONTEXT_INITIALIZED();

    return platform_dma_set_interrupt_enabled(microphone_context.buffer.dma_ch, enabled);
}

/** --------------------------------------------------------------------------------------------
 *  Internal functions
 * -------------------------------------------------------------------------------------------- **/

/*************************************************************************************************/
static void microphone_dma_irq_callback(void *arg)
{
    microphone_context.callback(microphone_context.buffer.current, microphone_context.buffer.offset, microphone_context.callback_arg);
    microphone_context.buffer.current += microphone_context.buffer.offset;
    if(microphone_context.buffer.current ==  microphone_context.buffer.end)
    {
        microphone_context.buffer.current = microphone_context.buffer.start;
    }
}

/*************************************************************************************************/
static void dma_desc_init(uint32_t sample_count)
{
    LDMA_CH_TypeDef *desc = NULL;
    uint32_t desc_ptr = (uint32_t)microphone_context.buffer.dma_desc;
    uint8_t *buffer_ptr = microphone_context.buffer.start;
    const uint32_t sample_length = microphone_context.buffer.offset;

    // For each sample buffer
    for(uint32_t sample_id = 0; sample_id < sample_count; ++sample_id)
    {
        // Populate the descriptors for the current sample buffer
        for(uint32_t length_remaining = sample_length; length_remaining > 0;)
        {
            const uint16_t chunk_length = (length_remaining > MAX_DMA_LENGTH) ?
                                          MAX_DMA_LENGTH : length_remaining;
            desc = (LDMA_CH_TypeDef*)(desc_ptr - offsetof(LDMA_CH_TypeDef, CTRL));

            // Update the counters and pointers
            desc_ptr += sizeof(LDMA_Descriptor_t);
            length_remaining -= chunk_length;

            // The source of the DMA is the I2S RXDATA
            desc->SRC = (uint32_t)&BSP_MICROPHONE_USART->RXDATA;

            // The destination is the current buffer pointer
            desc->DST = (uint32_t)buffer_ptr;
            buffer_ptr += chunk_length;

            // Use block transfer and generate an interrupt on the last DMA descriptor for this buffer
            desc->CTRL = (LDMA_CH_CTRL_STRUCTTYPE_TRANSFER |
                          LDMA_CH_CTRL_BLOCKSIZE_UNIT1     |
                          LDMA_CH_CTRL_SIZE_BYTE           |
                          LDMA_CH_CTRL_REQMODE_BLOCK       |
                          LDMA_CH_CTRL_DSTINC_ONE          |
                          LDMA_CH_CTRL_SRCINC_NONE         |
                          ((length_remaining > 0) ? 0 : LDMA_CH_CTRL_DONEIFSEN) |
                          ((chunk_length-1) << _LDMA_CH_CTRL_XFERCNT_SHIFT) );

            // This descriptor points to the next descriptor
            desc->LINK = (desc_ptr | LDMA_CH_LINK_LINKMODE_ABSOLUTE | LDMA_CH_LINK_LINK);
        }
    }

    // The last DMA descriptor points to the first descriptor
    // to create a ring between the sample buffers:
    // buffer1 -> buffer2 -> buffer3 -> buffer1 -> ...
    desc->LINK = (((uint32_t)microphone_context.buffer.dma_desc) | LDMA_CH_LINK_LINKMODE_ABSOLUTE | LDMA_CH_LINK_LINK);

    if(microphone_context.flags & MICROPHONE_FLAG_MONO_DROP_OTHER_CHANNEL)
    {
        desc_ptr = (uint32_t)&microphone_context.drop_channel.dma_desc;
        desc = (LDMA_CH_TypeDef*)(desc_ptr - offsetof(LDMA_CH_TypeDef, CTRL));
        desc->SRC = (uint32_t)&BSP_MICROPHONE_USART->RXDATA;
        desc->DST = (uint32_t)&microphone_context.drop_channel.buffer;
        desc->CTRL = (LDMA_CH_CTRL_STRUCTTYPE_TRANSFER |
                      LDMA_CH_CTRL_BLOCKSIZE_UNIT1     |
                      LDMA_CH_CTRL_SIZE_BYTE           |
                      LDMA_CH_CTRL_REQMODE_BLOCK       |
                      LDMA_CH_CTRL_DSTINC_NONE         |
                      LDMA_CH_CTRL_SRCINC_NONE         |
                      ((MAX_DMA_LENGTH-1) << _LDMA_CH_CTRL_XFERCNT_SHIFT) );

        // This descriptor points back to itself
        desc->LINK = (desc_ptr | LDMA_CH_LINK_LINKMODE_ABSOLUTE | LDMA_CH_LINK_LINK);
    }
}

/*************************************************************************************************/
static int validate_config(const microphone_config_t *config)
{
    int result;


    if(!((config->resolution == MICROPHONE_RESOLUTION_16BITS) || (config->resolution == MICROPHONE_RESOLUTION_24BITS)))
    {
        result = -1;
    }
    else if((config->buffer_callback == NULL))
    {
        result = -1;
    }
    else if(config->sample_size < 256)
    {
        result = -1;
    }
    else if((config->buffer == NULL) ^ (config->buffer_length_bytes == 0))
    {
        result = -1;
    }
    else if((config->buffer != NULL) &&
            (config->buffer_length_bytes % (config->sample_size * (config->resolution/8)) != 0))
    {
        result = -1;
    }
    else
    {
        result = 0;
    }

    return result;
}

