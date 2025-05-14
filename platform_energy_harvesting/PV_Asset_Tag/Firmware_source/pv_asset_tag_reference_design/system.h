/***************************************************************************//**
 * @file system.h
 * @brief System init and control
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

#ifndef SYSTEM__H
#define SYSTEM__H

void system_init(void);
void setup_modem_signals(void);
void prepare_hfxo(void);
void select_hfxo(void);

#endif // SYSTEM_H
