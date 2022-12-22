/***************************************************************************//**
 * @file tasks.c
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

#include "tasks.h"
#include "peripherals.h"
#include "os.h"
#include <stdio.h>
#include "em_ldma.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

#ifndef TASK_STACK_SIZE
#define TASK_STACK_SIZE      (256)
#endif

#ifndef TASK_PRIO
#define TASK_PRIO            (20)
#endif

#define LDMA_CH0_INT         (0x0001)
#define LDMA_CH1_INT         (0x0002)

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/

// Create Task related variables
static OS_TCB tcb;
static OS_TCB tcb_task2;

static CPU_STK stack[TASK_STACK_SIZE];
static CPU_STK stack_task2[TASK_STACK_SIZE];

// Create Mutex, Semaphores, event flags to use in tasks.
static OS_MUTEX mutex_vol_val;
static OS_SEM sem_adc0;
static OS_SEM sem_adc1;
static OS_FLAG_GRP array_full_flag;

// Arrays to store voltage values.
uint32_t arr_voltage0[4];
uint32_t arr_voltage1[4];
int val = 0;

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * Initialize task 1 and other RTOS objects such as semaphores, mutex and
 * event flags.
 ******************************************************************************/
void task_init(void)
{
  RTOS_ERR err;

  // Create Task
  OSTaskCreate(&tcb,
               "Task 1",
               consumer_task_1,
               DEF_NULL,
               TASK_PRIO,
               &stack[0],
               (TASK_STACK_SIZE / 10u),
               TASK_STACK_SIZE,
               0u,
               0u,
               DEF_NULL,
               (OS_OPT_TASK_STK_CLR),
               &err);
  EFM_ASSERT((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE));

  // Create Mutex
  OSMutexCreate(&mutex_vol_val,
                "MutexVolVal",
                &err);
  EFM_ASSERT((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE));

  // Create Semaphore
  OSSemCreate(&sem_adc0,
              "SemADC0",
              0,
              &err);
  EFM_ASSERT((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE));

  // Create Semaphore
  OSSemCreate(&sem_adc1,
              "SemADC1",
              0,
              &err);
  EFM_ASSERT((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE));

  // Create Event Flag
  OSFlagCreate(&array_full_flag,
               "RawValArr_Full",
               0,
               &err);
  EFM_ASSERT((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE));
}

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
void consumer_task_1(void *p_arg)
{
  (void)p_arg;

  RTOS_ERR err;

  // Create Task
  OSTaskCreate(&tcb_task2,
               "Task 2",
               consumer_task_2,
               DEF_NULL,
               TASK_PRIO,
               &stack_task2[0],
               (TASK_STACK_SIZE / 10u),
               TASK_STACK_SIZE,
               0u,
               0u,
               DEF_NULL,
               (OS_OPT_TASK_STK_CLR),
               &err);
  EFM_ASSERT((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE));

  // Initialize peripherals
  adc_init();
  ldma_init();
  gpio_init();

  uint32_t *data1 = NULL;
  uint32_t *data0 = NULL;
  data1 = get_adc_data1();
  data0 = get_adc_data0();

  while (1)
  {
    /*Wait for Semaphores to pend. Until then the task is blocked.
     * Sempahores are posted when enough values are sampled from the ADC
     */
    OSSemPend(&sem_adc0,
              0,
              OS_OPT_PEND_BLOCKING,
              (CPU_TS *)0,
              &err);
    EFM_ASSERT((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE));

    OSSemPend(&sem_adc1,
              0,
              OS_OPT_PEND_BLOCKING,
              (CPU_TS *)0,
              &err);
    EFM_ASSERT((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE));

    // Pend mutex to lock access to arr_voltage arrays
    OSMutexPend(&mutex_vol_val,
                0,
                OS_OPT_PEND_BLOCKING,
                (CPU_TS *)0,
                &err);
    EFM_ASSERT((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE));

    for (uint8_t i = 0; i < 4; i++)
    {
      arr_voltage0[i] = ((data0[i] * 2500) / 4095);

      arr_voltage1[i] = ((data1[i] * 2500) / 4095);
    }

    LDMA_Tx();

    // Post flag indicating that voltages have been calculated
    OSFlagPost(&array_full_flag,
               0x01,
               OS_OPT_POST_FLAG_SET,
               &err);
    EFM_ASSERT((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE));

    // post mutex to unlock access to arr_voltage array
    OSMutexPost(&mutex_vol_val,
                OS_OPT_POST_NONE,
                &err);
    EFM_ASSERT((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE));
  }
}

/***************************************************************************//**
 * Task 2 function
 *
 * It waits for task 1 to be completed before executing. Once all the samples
 * are converted into voltage by task 1, they will be printed on the serial
 * terminal or device console in this function.
 ******************************************************************************/
void consumer_task_2(void *p_arg)
{
  (void)p_arg;

  RTOS_ERR err;

  while (1)
  {
    if (OSFlagPend(&array_full_flag,
                   0x01,
                   0,
                   OS_OPT_PEND_FLAG_SET_ANY,
                   (CPU_TS *)0,
                   &err) == 0x01) {
      // Clear event flag
      OSFlagPost(&array_full_flag,
                 0x01,
                 OS_OPT_POST_FLAG_CLR,
                 &err);
      EFM_ASSERT((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE));

      OSMutexPend(&mutex_vol_val,
                  0,
                  OS_OPT_PEND_BLOCKING,
                  (CPU_TS *)0,
                  &err);
      EFM_ASSERT((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE));

      // print voltages
      printf(" Voltages\n\r");
      printf("Ch10    Ch11 \n\r");

      for (uint8_t i = 0; i < 4; i++)
      {
        printf("%lumV    %lumV\n\r", arr_voltage0[i], arr_voltage1[i]);
      }

      OSMutexPost(&mutex_vol_val,
                  OS_OPT_POST_NONE,
                  &err);
      EFM_ASSERT((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE));
    }
  }
}

/***************************************************************************//**
 * LDMA Interrupt Handler
 ******************************************************************************/
void LDMA_IRQHandler(void)
{
  RTOS_ERR err;
  OSIntEnter();
  uint32_t flags = LDMA_IntGet();

  /* When transfer is completed, the bit corresponding to the channel is set */
  if ((flags & LDMA_CH0_INT) != 0x0000) { // channel 0
    LDMA_IntClear(flags);
    OSSemPost(&sem_adc0,
              OS_OPT_POST_ALL,
              &err);
    EFM_ASSERT((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE));
  }

  if ((flags & LDMA_CH1_INT) != 0x0000) { // channel 1
    LDMA_IntClear(flags);

    OSSemPost(&sem_adc1,
              OS_OPT_POST_ALL,
              &err);
    EFM_ASSERT((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE));
  }
  OSIntExit();
}
