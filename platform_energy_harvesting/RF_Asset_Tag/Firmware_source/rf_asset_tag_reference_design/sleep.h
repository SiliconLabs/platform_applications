/***************************************************************************//**
 * @file sleep.h
 * @brief Low power sleep including EM4 wakeup
 *******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

#ifndef SLEEP__H
#define SLEEP__H

void set_wakeup(unsigned int time_ms);
void clear_wakeup(void);
void sleep(unsigned int time_ms);

#endif // SLEEP__H
