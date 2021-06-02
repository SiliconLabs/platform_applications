/***************************************************************************//**
 * @file
 * @brief Bootloader I2C communication queue.
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
#include "btl_i2c_queue.h"
#include <stdbool.h>

/**
 * @brief Initializes the queue
 * @param queue The queue

 */
void queue_init(queue_t *queue)
{
  queue->head = 0;
  queue->tail = 0;
}

/**
 * @brief push a byte into the queue
 * @param queue the queue where the byte pushed into
 * @param c     the byte to push
 * @return QUEUE_EOF when queue is full,
 *         QUEUE_OK when byte is pushed
 */
uint16_t queue_push(queue_t *queue, uint8_t c)
{
  if (queue_is_full(queue)) {
    return QUEUE_EOF;
  }
  queue->data[queue->head++] = c;
  if (queue->head == QUEUE_BUFFER_SIZE)
    queue->head = 0;
  return QUEUE_OK;
}

/**
 * @brief pop a byte from queue
 * @param queue The queue that holds the byte
 * @return QUEUE_EOF when queue is empty,
 *         QUEUE_OK | byte on success.
 */
uint16_t queue_pop(queue_t *queue)
{
  uint16_t result;
  if (queue_is_empty(queue)) {
    result = QUEUE_EOF;
  } else {
    result = QUEUE_OK | queue->data[queue->tail++];
    if (queue->tail == QUEUE_BUFFER_SIZE)
      queue->tail = 0;
  }
  return result;
}

/**
 * @brief returns with the queue's first byte without setting the queue.
 * @param queue The queue that holds the byte
 * @return QUEUE_EOF when queue is empty,
 *         QUEUE_OK | byte on success.
 */
uint16_t queue_peek(queue_t *queue)
{
  uint16_t result;
  if (queue_is_empty(queue)) {
    result = QUEUE_EOF;
  } else {
    result = QUEUE_OK | queue->data[queue->tail];
  }
  return result;
}

/**
 * @brief checks queue's emptiness
 * @param queue the queue
 * @return True if queue is empty
 */
bool queue_is_empty(queue_t *queue)
{
  return queue->head == queue->tail;
}

/**
 * @brief checks queue's fullness
 * @param queue the queue
 * @return True if queue is full
 */
bool queue_is_full(queue_t *queue)
{
  return queue_size(queue) == QUEUE_BUFFER_SIZE;
}

/**
 * @brief get the queue storing bytes size.
 * @param queue the queue
 * @return amount of stored bytes
 */
uint16_t queue_size(queue_t *queue)
{
  if (queue->head < queue-> tail) {
    return (queue->head + QUEUE_BUFFER_SIZE - queue->tail) ;
  } else {
    return (queue->head-queue->tail);
  }
}
