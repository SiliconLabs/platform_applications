/***************************************************************************//**
 * @file
 * @brief Header file for bootloader I2C communication queue.
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
 *******************************************************************************
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/

#ifndef _BTL_I2C_QUEUE_H
#define _BTL_I2C_QUEUE_H
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define QUEUE_OK              (0)
#define QUEUE_EOF             (256)

#define QUEUE_BUFFER_SIZE     (256)

#define QUEUE_STATUS_UNDERRUN (1)
#define QUEUE_STATUS_OVERRUN  (2)

typedef struct {
  volatile uint16_t head;                    //**< queue's head index*/
  volatile uint16_t tail;                    //**< queue's tail index*/
  volatile uint8_t data[QUEUE_BUFFER_SIZE]; //**< queue's data*/
} queue_t;

void queue_init(queue_t *queue);
uint16_t queue_push(queue_t *queue,  uint8_t c);
uint16_t queue_pop(queue_t *queue);
uint16_t queue_popb(queue_t *queue);
uint16_t queue_peek(queue_t *queue);
uint16_t queue_size(queue_t *queue);
bool queue_is_empty(queue_t *queue);
bool queue_is_full(queue_t *queue);

#ifdef __cplusplus
} // extern "C"
#endif

#endif  /* _BTL_I2C_QUEUE_H */
