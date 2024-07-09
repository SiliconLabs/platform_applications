/***************************************************************************//**
 * @file rtcc.c
 * @brief RTCC functions
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

#include "em_cmu.h"
#include "em_rtcc.h"
#include "rtcc.h"

volatile uint32_t g_flag_time = 0;
struct time_s g_time_info;

/**************************************************************************//**
 * @brief RTCC initialization
 *****************************************************************************/
void init_rtcc(void)
{
  /* Disable LFXO oscillator */
  CMU_OscillatorEnable(cmuOsc_LFXO, false, true);

  /* Disable RTCC clock */
  CMU_ClockEnable(cmuClock_RTCC, false);

  /* Set osc mode */
  CMU_LFXOInit_TypeDef config_LFXO;
  config_LFXO.mode = cmuOscMode_Crystal;
  CMU_LFXOInit(&config_LFXO);

  // Configure HFCLK because bus interface to Low Energy E peripherals is
  //   clocked by HFCLK(LE)
  CMU_ClockEnable(cmuClock_HFLE, true);

  /* Set reference osc used for clock branch */
  CMU_ClockSelectSet(cmuClock_LFE, cmuSelect_LFXO);

  /* Enable LFXO oscillator */
  CMU_OscillatorEnable(cmuOsc_LFXO, true, true);

  /* Enable RTCC clock */
  CMU_ClockEnable(cmuClock_RTCC, true);

  /* Configure and initialize the peripheral */
  RTCC_Init_TypeDef config_RTCC;
  config_RTCC.enable = false;
  config_RTCC.debugRun = false;
  config_RTCC.cntMode = rtccCntModeCalendar;
  config_RTCC.presc = rtccCntPresc_32768;
  config_RTCC.prescMode = rtccCntTickPresc;

  RTCC_Init(&config_RTCC);

  /* Set time and date to begin with */
  RTCC_TimeSet(TIME_RTCC);
  RTCC_DateSet(DATE_RTCC);

  /* Enabling interrupts*/
  RTCC_IntEnable(RTCC_IEN_CNTTICK); // not set
  RTCC_IntClear(RTCC_IEN_CNTTICK);
  __NVIC_EnableIRQ(RTCC_IRQn);

  /* Enable RTCC peripheral */
  RTCC_Enable(true);// not set
}

/**************************************************************************//**
 * @brief RTCC IRQ Handler
 *****************************************************************************/
void RTCC_IRQHandler(void)
{
  uint32_t int_flag;
  int_flag = RTCC_IntGet();

  if (int_flag & RTCC_IF_CNTTICK) {
    RTCC_IntClear(int_flag);

    /*set flag to print the timestamp.*/
    g_flag_time = 1;
  }
}

/**************************************************************************//**
 * @brief Read Date from RTCC registers
 *****************************************************************************/
void get_date(struct time_s *time_data)
{
  uint32_t date = RTCC_DateGet();
  time_data->day_of_month = (date & _RTCC_DATE_DAYOMU_MASK)
                            + (((date & _RTCC_DATE_DAYOMT_MASK) >>
                                _RTCC_DATE_DAYOMT_SHIFT) * 10);

  time_data->month =
    ((date & _RTCC_DATE_MONTHU_MASK) >> _RTCC_DATE_MONTHU_SHIFT)
    + (((date & _RTCC_DATE_MONTHT_MASK) >>
        _RTCC_DATE_MONTHT_SHIFT) * 10);

  time_data->year = ((date & _RTCC_DATE_YEARU_MASK) >> _RTCC_DATE_YEARU_SHIFT)
                    + (((date & _RTCC_DATE_YEART_MASK) >>
                        _RTCC_DATE_YEART_SHIFT) * 10);
}

/**************************************************************************//**
 * @brief read Time from RTCC registers
 *****************************************************************************/
void get_time(struct time_s *time_data)
{
  uint32_t time = RTCC_TimeGet();
  time_data->sec = (time & _RTCC_TIME_SECU_MASK)
                   + (((time & _RTCC_TIME_SECT_MASK) >>
                       _RTCC_TIME_SECT_SHIFT) * 10);

  time_data->min = ((time & _RTCC_TIME_MINU_MASK) >> _RTCC_TIME_MINU_SHIFT)
                   + (((time & _RTCC_TIME_MINT_MASK) >>
                       _RTCC_TIME_MINT_SHIFT) * 10);

  time_data->hour = ((time & _RTCC_TIME_HOURU_MASK) >> _RTCC_TIME_HOURU_SHIFT)
                    + (((time & _RTCC_TIME_HOURT_MASK) >>
                        _RTCC_TIME_HOURT_SHIFT) * 10);
}

/**************************************************************************//**
 * @brief Wrapper function to return flag
 *****************************************************************************/
uint32_t get_flag_time(void)
{
  return g_flag_time;
}

/**************************************************************************//**
 * @brief Wrapper function to reset flag
 *****************************************************************************/
void reset_flag_time(void)
{
  g_flag_time = 0;
}
