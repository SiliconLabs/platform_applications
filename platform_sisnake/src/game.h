/***************************************************************************//**
 * @file
 * @brief Ingame related functions.
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

#ifndef GAME_H_
#define GAME_H_

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

uint8_t get_difficulty(void);

void set_difficulty(uint8_t game_difficulty);

void set_maze(const enum maze_type_t game_maze);

/***************************************************************************//**
 * Sets up a new game by initializing the game_state struct.
 ******************************************************************************/
void init_game(void);

/***************************************************************************//**
 * Make a game turn.
 ******************************************************************************/
enum move_snake_return_t game_turn(const enum relative_direction_t direction);

/***************************************************************************//**
 * Make a game over turn.
 ******************************************************************************/
void game_over_tick (const enum event_t touch_slider_state);

/***************************************************************************//**
 * Gets the neighbor field coordinate determined by the given direction.
 ******************************************************************************/
map_coords_t get_neighbor_field_coords(map_coords_t field_coords,
                                       const enum direction_t next_field_direction);

#ifdef __cplusplus
}
#endif

#endif /* GAME_H_ */
