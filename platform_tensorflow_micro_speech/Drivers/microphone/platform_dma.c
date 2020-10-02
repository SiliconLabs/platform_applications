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
#include <assert.h>
#include <stdlib.h>


#include "em_device.h"
#include "em_ldma.h"
#include "em_cmu.h"
#include "em_bus.h"
#include "em_core.h"

#include "platform_dma.h"

#define UINT32_MAX_TRANSFER_LENGTH (PLATFORM_DMA_MAX_TRANSFER_LENGTH/sizeof(uint32_t))
#define NULL_IRQ_CALLBACK ((platform_dma_callback_t)0x01)

typedef struct
{
    void *arg;
    platform_dma_callback_t callback;
    volatile bool irq_pending;
} dma_channel_irq_t;


typedef struct
{
    dma_channel_irq_t channels[DMA_CHAN_COUNT];
    uint32_t rx_dummy;
    uint32_t tx_dummy;
    bool initialized;
} dma_context_t;

static void dma_init(void);

static dma_context_t platform_dma_context;

/*************************************************************************************************/
int platform_dma_alloc(platform_dma_callback_t callback, void *arg, platform_dma_channel_t *channel_ptr)
{
    CORE_DECLARE_IRQ_STATE;
    int result = -1;

    dma_init();

    callback = (callback == NULL) ? NULL_IRQ_CALLBACK : callback;
    *channel_ptr = PLATFORM_DMA_CHANNEL_INVALID;

    CORE_ENTER_CRITICAL();

    for(uint8_t channel = 0; channel < DMA_CHAN_COUNT; channel++)
    {
        if(platform_dma_context.channels[channel].callback == NULL)
        {
            platform_dma_context.channels[channel].callback = callback;
            platform_dma_context.channels[channel].arg = arg;
            platform_dma_context.channels[channel].irq_pending = false;
            *channel_ptr = channel;
            result = 0;
            break;
        }
    }

    CORE_EXIT_CRITICAL();

    return result;
}

/*************************************************************************************************/
int platform_dma_free(platform_dma_channel_t channel)
{
    CORE_DECLARE_IRQ_STATE;
    int result;


    CORE_ENTER_CRITICAL();

    if(channel >= DMA_CHAN_COUNT)
    {
        result = -1;
    }
    else if(platform_dma_context.channels[channel].callback == NULL)
    {
        result = -1;
    }
    else
    {
        platform_dma_stop(channel);
        platform_dma_context.channels[channel].callback = NULL;
        platform_dma_context.channels[channel].arg = NULL;
        platform_dma_context.channels[channel].irq_pending = false;

        result = 0;
    }


    CORE_EXIT_CRITICAL();

    return result;
}

/*************************************************************************************************/
int platform_dma_stop(platform_dma_channel_t channel)
{
    CORE_DECLARE_IRQ_STATE;
    int result;


    CORE_ENTER_CRITICAL();

    if(channel >= DMA_CHAN_COUNT)
    {
        result = -1;
    }
    else
    {
        const uint32_t channel_bit = (1 << channel);

        LDMA->IEN &= ~channel_bit;
        BUS_RegMaskedClear(&LDMA->CHEN, channel_bit);

        result = 0;
    }

    CORE_EXIT_CRITICAL();

    return result;
}

/*************************************************************************************************/
int platform_dma_set_interrupt_enabled(platform_dma_channel_t channel, bool enabled)
{
    CORE_DECLARE_IRQ_STATE;
    int result;

    CORE_ENTER_CRITICAL();

    if(channel >= DMA_CHAN_COUNT)
    {
        result = -1;
    }
    else
    {
        dma_channel_irq_t *channel_irq = &platform_dma_context.channels[channel];

        BUS_RegBitWrite(&LDMA->IEN, channel, enabled);
        if(enabled && channel_irq->irq_pending)
        {
            BUS_RegBitWrite(&LDMA->IFS, channel, 1);
        }
        result = 0;
    }

    CORE_EXIT_CRITICAL();

    return result;
}

/** --------------------------------------------------------------------------------------------
 *  Internal functions
 * -------------------------------------------------------------------------------------------- **/

/*************************************************************************************************/
static void dma_init(void)
{
    if(!platform_dma_context.initialized)
    {
        platform_dma_context.initialized = true;

        /* Make sure DMA clock is enabled prior to accessing DMA module */
        CMU_ClockEnable(cmuClock_LDMA, true);

        /* Make sure DMA controller is set to a known reset state */
        LDMA->IEN  = 0;
        LDMA->CHEN = 0;

        LDMA->CTRL = ((0 << _LDMA_CTRL_NUMFIXED_SHIFT)     |
                      (0 << _LDMA_CTRL_SYNCPRSCLREN_SHIFT) |
                      (0 << _LDMA_CTRL_SYNCPRSSETEN_SHIFT));

        LDMA->DBGHALT = 0;
        LDMA->REQDIS  = 0;

        LDMA->IFC = 0xFFFFFFFF;
        LDMA->IEN = LDMA_IEN_ERROR;

        /* Clear/enable DMA interrupts */
        NVIC_ClearPendingIRQ(LDMA_IRQn);
        NVIC_EnableIRQ(LDMA_IRQn);

        platform_dma_context.tx_dummy = UINT32_MAX;
    }
}

/*************************************************************************************************/
void LDMA_IRQHandler(void)
{
    uint32_t pending_mask;

    /* Get all pending and enabled interrupts */
    pending_mask  = LDMA->IF;

    if(pending_mask & LDMA_IF_ERROR)
    {
        const uint32_t error_status = (LDMA->STATUS & _LDMA_STATUS_CHERROR_MASK) >> _LDMA_STATUS_CHERROR_SHIFT;
        (void)error_status;
        pending_mask &= ~LDMA_IF_ERROR;
    }

    LDMA->IFC = pending_mask;
    const uint32_t enabled_mask = LDMA->IEN;


    for(uint32_t channel = 0; pending_mask != 0; ++channel, pending_mask >>= 1)
    {
        if(pending_mask & 0x01)
        {
            dma_channel_irq_t *channel_irq = &platform_dma_context.channels[channel];
            const platform_dma_callback_t callback = channel_irq->callback;

            if(callback != NULL)
            {
                const uint32_t channel_mask = (1 << channel);

                if(channel_mask & enabled_mask)
                {
                    channel_irq->irq_pending = false;
                    if(callback != NULL_IRQ_CALLBACK)
                    {
                        callback(channel_irq->arg);
                    }
                }
                else
                {
                    channel_irq->irq_pending = true;
                }
            }
        }
    }
}
