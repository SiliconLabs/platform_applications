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

#include <time.h>
#include <stdlib.h>
#include <stdbool.h>

#include "types.h"
#include "graphics.h"

#include "game.h"

/*******************************************************************************
 ******************************   DEFINES   ************************************
 ******************************************************************************/

#define CROSS_SIZE 6

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/

static uint8_t difficulty = 0;
enum maze_type_t maze = NONE;
static game_state_t game_state;

/*******************************************************************************
 **************************   LOCAL FUNCTIONS   ********************************
 ******************************************************************************/

/***************************************************************************//**
 * Generates a random food type to a random empty field.
 *
 * @note game_state.empty_fields_count must be initialized for every new game
 ******************************************************************************/
void generate_food(void)
{
  uint16_t random_field = rand() % game_state.empty_fields_count;

  uint16_t empty_field_counter = 0;
  for (uint16_t x = 0; x < MAP_SIZE_X; x++)
    for (uint16_t y = 0; y < MAP_SIZE_Y; y++) {
      if (game_state.map[x][y].state == EMPTY) {
        if (empty_field_counter != random_field)
          empty_field_counter++;
        else {
            switch (rand() % 3) {
              case 0:
                game_state.map[x][y].state = FOOD_1;
                break;

              case 1:
                game_state.map[x][y].state = FOOD_2;
                break;

              case 2:
                game_state.map[x][y].state = FOOD_3;
                break;

            }
            game_state.empty_fields_count--;
            return;
        }
      }
    }
}


/***************************************************************************//**
 * Gets the left neighbor coordinates of the given field coordinates.
 *
 * @param field_coords coordinates of the given field
 * @returns coordinates of the left field relative to the given field
 ******************************************************************************/
map_coords_t get_left_field_coords(const map_coords_t field_coords)
{
  map_coords_t left_field_coords = {0, field_coords.y};

  if (field_coords.x - 1 >= 0)
    left_field_coords.x = field_coords.x - 1;
  else
    left_field_coords.x = MAP_SIZE_X - 1;

  return left_field_coords;
}

/***************************************************************************//**
 * Gets the right neighbor coordinates of the given field coordinates.
 *
 * @param field_coords coordinates of the given field
 * @returns coordinates of the right field relative to the given field
 ******************************************************************************/
map_coords_t get_right_field_coords(const map_coords_t field_coords)
{
  map_coords_t right_field_coords = {0, field_coords.y};

  if (field_coords.x + 1 != MAP_SIZE_X)
    right_field_coords.x = field_coords.x + 1;
  else
    right_field_coords.x = 0;

  return right_field_coords;
}

/***************************************************************************//**
 * Gets the above neighbor coordinates of the given field coordinates.
 *
 * @param field_coords coordinates of the given field
 * @returns coordinates of the above field relative to the given field
 ******************************************************************************/
map_coords_t get_above_field_coords(const map_coords_t field_coords)
{
  map_coords_t above_field_coords = {field_coords.x, 0};

  if (field_coords.y - 1 >= 0)
    above_field_coords.y = field_coords.y - 1;
  else
    above_field_coords.y = MAP_SIZE_Y - 1;

  return above_field_coords;
}

/***************************************************************************//**
 * Gets the below neighbor coordinates of the given field coordinates.
 *
 * @returns coordinates of the below field relative to the given field
 ******************************************************************************/
map_coords_t get_below_field_coords(const map_coords_t field_coords)
{
  map_coords_t above_field_coords = {field_coords.x, 0};

  if (field_coords.y + 1 != MAP_SIZE_Y)
    above_field_coords.y = field_coords.y + 1;
  else
    above_field_coords.y = 0;

  return above_field_coords;
}

/***************************************************************************//**
 * Converts the head relative next movement direction to the direction system of
 * the map.
 *
 * @param r_direction next direction relative to the head
 * @returns direction in the direction system of the map
 ******************************************************************************/
enum direction_t r_direction2direction(const enum relative_direction_t r_direction)
{
  switch (game_state.map[game_state.snake_head.x][game_state.snake_head.y].next_segment) {
    case RIGHT:
      if ( r_direction == R_LEFT)
        return DOWN;
      else if (r_direction == R_FORWARD)
        return LEFT;
      else if (r_direction == R_RIGHT)
        return UP;
      break;
    case DOWN:
      if ( r_direction == R_LEFT)
        return LEFT;
      else if (r_direction == R_FORWARD)
        return UP;
      else if (r_direction == R_RIGHT)
        return RIGHT;
      break;
    case LEFT:
      if ( r_direction == R_LEFT)
        return UP;
      else if (r_direction == R_FORWARD)
        return RIGHT;
      else if (r_direction == R_RIGHT)
        return DOWN;
      break;
    case UP:
      if ( r_direction == R_LEFT)
        return RIGHT;
      else if (r_direction == R_FORWARD)
        return DOWN;
      else if (r_direction == R_RIGHT)
        return LEFT;
      break;
  }

  return UP;
}

/***************************************************************************//**
 * Snake mover function.
 *
 * @param next_field_direction field direction where the snake tries to move to
 * @returns the result of the move attempt
 *
 * Tries to move the snake towards the given direction,
 * and returns the result of it.
 ******************************************************************************/
enum move_snake_return_t move_snake(const enum direction_t next_field_direction)
{
  static bool just_ate = false;
  map_field_t *current_field = &game_state.map[game_state.snake_head.x][game_state.snake_head.y];
  map_coords_t next_field_coords = get_neighbor_field_coords(game_state.snake_head,
                                                             next_field_direction);
  map_field_t *next_field = &game_state.map[next_field_coords.x][next_field_coords.y];

  // check if the next move would be crash
  if (next_field->state == SNAKE_BODY ||
      next_field->state == SNAKE_FULL_BELLY ||
      next_field->state == SNAKE_TAIL ||
      next_field->state == WALL)
    return CRASH;

  // check if food ahead
  bool food = false;
  if (next_field->state == FOOD_1 ||
      next_field->state == FOOD_2 ||
      next_field->state == FOOD_3)
    food = true;

  // make the move
  if (just_ate) {
    current_field->state = SNAKE_FULL_BELLY;
    just_ate = false;
  }
  else
    current_field->state = SNAKE_BODY;

  if (next_field_direction == RIGHT) {
    next_field->next_segment = LEFT;
    current_field->previous_segment = RIGHT;
  }
  else if (next_field_direction == LEFT) {
    next_field->next_segment = RIGHT;
    current_field->previous_segment = LEFT;
  }
  else if (next_field_direction == DOWN) {
    next_field->next_segment = UP;
    current_field->previous_segment = DOWN;
  }
  else if (next_field_direction == UP) {
    next_field->next_segment = DOWN;
    current_field->previous_segment = UP;
  }

  next_field->state = SNAKE_HEAD;
  game_state.snake_head.x = next_field_coords.x;
  game_state.snake_head.y = next_field_coords.y;

  if (!food) {
      map_coords_t before_tail_segment_coords = get_neighbor_field_coords(game_state.snake_tail,
                                                                      game_state.map[game_state.snake_tail.x][game_state.snake_tail.y].previous_segment);
      map_field_t *tail = &game_state.map[game_state.snake_tail.x][game_state.snake_tail.y];
      tail->state = EMPTY;
      game_state.map[before_tail_segment_coords.x][before_tail_segment_coords.y].state = SNAKE_TAIL;
      game_state.snake_tail.x = before_tail_segment_coords.x;
      game_state.snake_tail.y = before_tail_segment_coords.y;

      return CLEAN_MOVE;
  } else {
      just_ate = true;
      return FOOD;
  }

}

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * Gets the difficulty of the current game.
 *
 * @returns the difficulty of the current game
 *
 * @note 0 is the easiest. Bigger number means harder game
 ******************************************************************************/
uint8_t get_difficulty(void)
{
  return difficulty;
}

void set_difficulty(uint8_t game_difficulty)
{
  difficulty = game_difficulty;
}

void set_maze(const enum maze_type_t game_maze)
{
  maze = game_maze;
}

/***************************************************************************//**
 * Sets up a new game by initializing the game_state struct.
 ******************************************************************************/
void init_game(void)
{
  game_state.score = 0;

  //clear the map
  for (uint16_t x = 0; x < MAP_SIZE_X; x++)
    for (uint16_t y = 0; y < MAP_SIZE_Y; y++)
        game_state.map[x][y].state = EMPTY;

  // make the snake
  // head
  map_coords_t map_pointer;
  map_pointer.x = MAP_SIZE_X/3*2+3;
  map_pointer.y = MAP_SIZE_Y/3;
  game_state.map[map_pointer.x][map_pointer.y].state = SNAKE_HEAD;
  game_state.map[map_pointer.x][map_pointer.y].next_segment = RIGHT;
  game_state.snake_head.x = map_pointer.x;
  game_state.snake_head.y = map_pointer.y;

  // body
  map_pointer.x++;
  game_state.map[map_pointer.x][map_pointer.y].state = SNAKE_BODY;
  game_state.map[map_pointer.x][map_pointer.y].previous_segment = LEFT;
  game_state.map[map_pointer.x][map_pointer.y].next_segment = RIGHT;

  // tail
  map_pointer.x++;
  game_state.map[map_pointer.x][map_pointer.y].state = SNAKE_TAIL;
  game_state.map[map_pointer.x][map_pointer.y].previous_segment = LEFT;
  game_state.snake_tail.x = map_pointer.x;
  game_state.snake_tail.y = map_pointer.y;

  // generate a maze, if needed
  if ( maze == BORDER) {
    for (uint16_t x = 0; x < MAP_SIZE_X; x++) {
        game_state.map[x][0].state = WALL;
        game_state.map[x][MAP_SIZE_Y - 1].state = WALL;
    }
    for (uint16_t y = 0; y < MAP_SIZE_Y; y++) {
        game_state.map[0][y].state = WALL;
        game_state.map[MAP_SIZE_X - 1][y].state = WALL;
    }
  } else if ( maze == CROSS ) {
      for (uint16_t x = MAP_SIZE_X/2 - CROSS_SIZE + 1; x < (MAP_SIZE_X/2 + CROSS_SIZE); x++)
        game_state.map[x][MAP_SIZE_Y/2].state = WALL;
      for (uint16_t y = MAP_SIZE_Y/2 - CROSS_SIZE + 1; y < (MAP_SIZE_Y/2 + CROSS_SIZE); y++)
        game_state.map[MAP_SIZE_X/2][y].state = WALL;
  }

  // count the initial empty field count for the generate_food function
  game_state.empty_fields_count = 0;
  for (uint16_t x = 0; x < MAP_SIZE_X; x++)
    for (uint16_t y = 0; y < MAP_SIZE_Y; y++)
      if (game_state.map[x][y].state == EMPTY)
        game_state.empty_fields_count++;

  generate_food();
}

/***************************************************************************//**
 * Make a game turn.
 *
 * @param r_direction move direction relative to the snake head
 * @returns the result of the move attempt
 ******************************************************************************/
enum move_snake_return_t game_turn(const enum relative_direction_t r_direction)
{
  enum direction_t next_field_direction = r_direction2direction(r_direction);

  enum move_snake_return_t move_result = move_snake(next_field_direction);
  if (move_result == FOOD) {
      generate_food();
      game_state.score = game_state.score + 1 + (difficulty * 0.5);
      if (maze != NONE)
        game_state.score++;
  }

  print_game(&game_state);

  return move_result;
}

/***************************************************************************//**
 * Make a game over tick.
 *
 * @param touch_slider_state the state of the touch slider to be printed
 ******************************************************************************/
void game_over_tick (const enum event_t touch_slider_state)
{
  print_game_over((uint32_t)game_state.score, touch_slider_state);
}

/***************************************************************************//**
 * Gets the neighbor field coordinate determined by the given direction.
 *
 * @param field_coords coordinates of the current field
 * @param neighbor_field_direction determines which neighbor coordinates needed
 *        of the current field
 * @returns the coordinates of the selected neighbor
 ******************************************************************************/
map_coords_t get_neighbor_field_coords(map_coords_t field_coords,
                                const enum direction_t neighbor_field_direction)
{
  map_coords_t next_field_coords = {0, 0};

  switch (neighbor_field_direction) {
    case RIGHT:
      next_field_coords = get_right_field_coords(field_coords);
      break;

    case LEFT:
      next_field_coords = get_left_field_coords(field_coords);
      break;

    case DOWN:
      next_field_coords = get_below_field_coords(field_coords);
      break;

    case UP:
      next_field_coords = get_above_field_coords(field_coords);
      break;

    default:
      break;
  }

  return next_field_coords;
}
