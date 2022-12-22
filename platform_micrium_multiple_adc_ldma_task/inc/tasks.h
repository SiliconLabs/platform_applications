/***************************************************************************//**
 * @file tasks.h
 * @brief task implementations
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

#ifndef TASKS_H_
#define TASKS_H_

/***************************************************************************//**
 * Initialize task 1 and other RTOS objects such as semaphores, mutex and
 * event flags.
 ******************************************************************************/
void task_init(void);

/***************************************************************************//**
 * Task 1 function
 *
 * It waits for the individual buffers to be filled with samples. Once that is
 * done, the function converts the samples to voltage values and stores them in
 * a different set of buffers.
 *
 * The GPIO triggers ADC0 and ADC1. Once all the data samples are written to
 * individual buffers by LDMA, the LDMA interrupt occurs and triggers task 1.
 ******************************************************************************/
void consumer_task_1(void *p_arg);

/***************************************************************************//**
 * Task 2 function
 *
 * It waits for task 1 to be completed before executing. Once all the samples
 * are converted into voltage by task 1, they will be printed on the serial
 * terminal or device console in this function.
 ******************************************************************************/
void consumer_task_2(void *p_arg);

#endif /* TASKS_H_ */
