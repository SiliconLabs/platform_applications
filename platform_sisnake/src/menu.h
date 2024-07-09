/***************************************************************************//**
 * @file
 * @brief Menu functions.
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *******************************************************************************
 *
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 *
 ******************************************************************************/

#ifndef MENU_H_
#define MENU_H_

#include "types.h"
#include "sl_string.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * Init main menu.
 ******************************************************************************/
void init_main_menu(void);

/***************************************************************************//**
 * Init pause menu.
 ******************************************************************************/
void init_pause(void);

/***************************************************************************//**
 * Make a menu tick.
 ******************************************************************************/
bool menu_tick(const enum event_t irq);

#ifdef __cplusplus
}
#endif

#endif /* MENU_H_ */
