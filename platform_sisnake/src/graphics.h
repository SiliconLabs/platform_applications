/***************************************************************************//**
 * @file
 * @brief Graphics functions.
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

#ifndef GRAPHICS_H_
#define GRAPHICS_H_

#include "types.h"
#include "sl_string.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * Init graphics.
 ******************************************************************************/
void init_graphics(void);

/***************************************************************************//**
 * Print error message to the screen, and stops.
 ******************************************************************************/
void print_error_msg(const char *msg, const int error_code);

/***************************************************************************//**
 * Print a menu to the screen.
 ******************************************************************************/
void print_menu(const menu_t menu);

/***************************************************************************//**
 * Prints the in-game tail to the screen.
 ******************************************************************************/
void print_ingame_tail(const enum event_t touch_slider_state);

/***************************************************************************//**
 * Prints the current snapshot of the game to the screen.
 ******************************************************************************/
void print_game(const game_state_t *game_state);

/***************************************************************************//**
 * Prints the game over screen to the screen.
 ******************************************************************************/
void print_game_over(const uint32_t score,
                     const enum event_t touch_slider_state);

#ifdef __cplusplus
}
#endif

#endif /* GRAPHICS_H_ */
