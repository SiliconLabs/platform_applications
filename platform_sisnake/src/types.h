/***************************************************************************//**
 * @file
 * @brief App wide types.
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

#ifndef TYPES_H_
#define TYPES_H_

#include <stdint.h>
#include <stdbool.h>
#include "sl_memlcd_display.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

#define MAP_SIZE_X 21
#define MAP_SIZE_Y 16
#define MAX_STRING_LENGTH (SL_MEMLCD_DISPLAY_WIDTH / 8)
#define MAX_MENU_ELEMENT_COUNT 7

/*******************************************************************************
 ********************************   TYPES   ************************************
 ******************************************************************************/

enum state_t{MENUS,
             GAME,
             GAME_OVER};

enum menu_return_t {NO_ACTION,
                    START_NEW_GAME,
                    CONTINUE};

enum event_t{BTN0,
             BTN1,
             TOUCH_SLIDER_RIGHT_PUSH,
             TOUCH_SLIDER_RIGHT_RELEASE,
             TOUCH_SLIDER_RIGHT_CANCEL,
             TURN,
             UNDETERMINED};


enum relative_direction_t{R_FORWARD,
                          R_LEFT,
                          R_RIGHT};

enum maze_type_t{NONE,
                 BORDER,
                 CROSS};

enum map_field_state_t{SNAKE_HEAD,
                       SNAKE_BODY,
                       SNAKE_FULL_BELLY,
                       SNAKE_TAIL,
                       FOOD_1,
                       FOOD_2,
                       FOOD_3,
                       WALL,
                       EMPTY};

enum direction_t{UP,
                 DOWN,
                 LEFT,
                 RIGHT};

enum move_snake_return_t {CLEAN_MOVE,
                          FOOD,
                          CRASH};

typedef struct {
  enum map_field_state_t state;
  enum direction_t next_segment;
  enum direction_t previous_segment;
} map_field_t;

typedef struct {
  uint8_t x;
  uint8_t y;
} map_coords_t;

typedef struct {
  float score;
  map_field_t map[MAP_SIZE_X][MAP_SIZE_Y];
  uint16_t empty_fields_count;
  map_coords_t snake_head;
  map_coords_t snake_tail;
} game_state_t;

typedef struct {
  char title[MAX_STRING_LENGTH];
  uint8_t menu_element_count;
  uint8_t choosen_menu_element_index;
  char menu_elements[MAX_MENU_ELEMENT_COUNT][MAX_STRING_LENGTH];
  bool is_ok_selected;
} menu_t;

#endif /* TYPES_H_ */
