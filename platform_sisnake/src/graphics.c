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
 ******************************************************************************
 *
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <float.h>
#include <stdio.h>

#include "sl_board_control.h"
#include "sl_memlcd_display.h"
#include "em_assert.h"
#include "glib.h"
#include "dmd.h"

#include "types.h"
#include "graphics.h"
#include "graphics_bitmaps.h"
#include "game.h"

/*******************************************************************************
 ******************************   DEFINES   ************************************
 ******************************************************************************/

#define MENU_HEADER_DELIMITER 23
#define INGAME_HEADER_DELIMITER 10
#define SCREEN_BOTTOM_DELIMITER 108

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/
static GLIB_Context_t glibContext;
static bool menu_flag = false;

/*******************************************************************************
 **************************   LOCAL FUNCTIONS   ********************************
 ******************************************************************************/

/***************************************************************************//**
 * Clears a rectangle shaped area of the screen.
 ******************************************************************************/
void clear_screen_area(GLIB_Context_t *pContext,
                       const uint8_t xMin,
                       const uint8_t xMax,
                       const uint8_t yMin,
                       const uint8_t yMax)
{
  GLIB_Rectangle_t clipping_area;
  clipping_area.xMin = xMin;
  clipping_area.xMax = xMax;
  clipping_area.yMin = yMin;
  clipping_area.yMax = yMax;

  GLIB_setClippingRegion(pContext, &clipping_area);
  GLIB_clearRegion(pContext);

  //reset the clipping region to default
  GLIB_resetClippingRegion(pContext);
  //apply if for the app
  GLIB_applyClippingRegion(pContext);
}

/***************************************************************************//**
 * Prints menu header to the screen.
 ******************************************************************************/
void print_menu_header(const char *title)
{
  clear_screen_area(&glibContext, 0,
                    SL_MEMLCD_DISPLAY_WIDTH - 1,
                    0,
                    MENU_HEADER_DELIMITER);

  GLIB_drawLineH(&glibContext,
                 0,
                 MENU_HEADER_DELIMITER,
                 SL_MEMLCD_DISPLAY_WIDTH - 1);

  GLIB_setFont(&glibContext, (GLIB_Font_t *) &GLIB_FontNormal8x8);
  GLIB_drawStringOnLine(&glibContext,
                        title,
                        1,
                        GLIB_ALIGN_CENTER,
                        0,
                        0,
                        true);

  DMD_updateDisplay();
}

/***************************************************************************//**
 * Prints the bottom part of the screen.
 ******************************************************************************/
void print_screen_bottom(char *upper_hint_text,
                  char *lower_hint_text,
                  char *button_text,
                  bool is_button_selected)
{
  clear_screen_area(&glibContext,
                    0,
                    SL_MEMLCD_DISPLAY_WIDTH - 1,
                    SCREEN_BOTTOM_DELIMITER,
                    SL_MEMLCD_DISPLAY_WIDTH - 1);

  // draw button edges
  GLIB_drawLineH(&glibContext,
                 0,
                 SCREEN_BOTTOM_DELIMITER,
                 SL_MEMLCD_DISPLAY_WIDTH - 1);
  GLIB_drawLineV(&glibContext,
                 SL_MEMLCD_DISPLAY_WIDTH / 2 - 1,
                 SCREEN_BOTTOM_DELIMITER + 1,
                 SL_MEMLCD_DISPLAY_HEIGHT - 1);

  // draw button
  if (is_button_selected) {
      GLIB_Rectangle_t rectangle = {66,
                                    111,
                                    SL_MEMLCD_DISPLAY_WIDTH - 2,
                                    SL_MEMLCD_DISPLAY_HEIGHT - 2};
      GLIB_drawRectFilled(&glibContext, &rectangle);
      glibContext.backgroundColor = Black;
      glibContext.foregroundColor = White;
  }
  GLIB_setFont(&glibContext, (GLIB_Font_t *) &GLIB_FontNarrow6x8);
  GLIB_drawStringOnLine(&glibContext,
                        button_text,
                        12,
                        GLIB_ALIGN_CENTER,
                        SL_MEMLCD_DISPLAY_WIDTH / 4,
                        -5,
                        true);

  if (is_button_selected) {
      glibContext.backgroundColor = White;
      glibContext.foregroundColor = Black;
  }

  // draw hint texts
  GLIB_drawStringOnLine(&glibContext,
                        upper_hint_text,
                        11,
                        GLIB_ALIGN_LEFT,
                        1,
                        0,
                        true);


  GLIB_drawStringOnLine(&glibContext,
                        lower_hint_text,
                        12,
                        GLIB_ALIGN_LEFT,
                        1,
                        0,
                        true);

  DMD_updateDisplay();
}

/***************************************************************************//**
 * Prints menu items to the screen.
 ******************************************************************************/
void print_menu_items(const menu_t menu)
{
  clear_screen_area(&glibContext,
                    0,
                    SL_MEMLCD_DISPLAY_WIDTH - 1,
                    MENU_HEADER_DELIMITER + 1,
                    SCREEN_BOTTOM_DELIMITER -1);

  uint8_t item_offset = 3;

  // Draw element size sensitive selection box,
  // to make the selection a bit more attractive.
  GLIB_Rectangle_t item_pointer_rectangle;
  uint8_t choosen_element_size = strlen(menu.menu_elements[menu.choosen_menu_element_index]);
  item_pointer_rectangle.xMin = ((SL_MEMLCD_DISPLAY_WIDTH/2) -
                                (10 * (choosen_element_size)) / 2) +
                                choosen_element_size;
  item_pointer_rectangle.xMax = ((SL_MEMLCD_DISPLAY_WIDTH/2) +
                                (10 * (choosen_element_size)) / 2) -
                                choosen_element_size;
  item_pointer_rectangle.yMin = (item_offset + menu.choosen_menu_element_index) * 10 - 1;
  item_pointer_rectangle.yMax = item_pointer_rectangle.yMin + 9;
  GLIB_drawRect(&glibContext, &item_pointer_rectangle);

  // draw menu elements
  GLIB_setFont(&glibContext, (GLIB_Font_t *) &GLIB_FontNormal8x8);
  for (uint8_t i = 0; i<menu.menu_element_count; i++)
    if (i != menu.choosen_menu_element_index)
      GLIB_drawStringOnLine(&glibContext,
                            menu.menu_elements[i],
                            i+item_offset,
                            GLIB_ALIGN_CENTER,
                            0,
                            0,
                            true);
    else {
      glibContext.backgroundColor = Black;
      glibContext.foregroundColor = White;
      GLIB_drawStringOnLine(&glibContext,
                            menu.menu_elements[i],
                            i+item_offset,
                            GLIB_ALIGN_CENTER,
                            0,
                            0,
                            true);
      glibContext.backgroundColor = White;
      glibContext.foregroundColor = Black;
    }



  DMD_updateDisplay();
}

/***************************************************************************//**
 * Prints ingame header to the screen.
 ******************************************************************************/
void print_ingame_header(uint32_t score, bool game_over)
{
    clear_screen_area(&glibContext,
                      0,
                      SL_MEMLCD_DISPLAY_WIDTH - 1,
                      0,
                      INGAME_HEADER_DELIMITER);

    // print header delimiter
    GLIB_drawLineH(&glibContext, 0,
                   INGAME_HEADER_DELIMITER - 1,
                   SL_MEMLCD_DISPLAY_WIDTH - 1);

    // print game over if present
    if (game_over) {
      GLIB_setFont(&glibContext, (GLIB_Font_t *) &GLIB_FontNormal8x8);
      GLIB_drawStringOnLine(&glibContext,
                            "Game Over",
                            0,
                            GLIB_ALIGN_LEFT,
                            1,
                            1,
                            true);
      GLIB_setFont(&glibContext, (GLIB_Font_t *) &GLIB_FontNarrow6x8);
    }

    // print score
    char score_str[sizeof(uint32_t)*8+1];
    sprintf(score_str, "%d", (int)score);
    GLIB_drawStringOnLine(&glibContext,
                          score_str,
                          0,
                          GLIB_ALIGN_RIGHT,
                          -1,
                          1,
                          true);

    DMD_updateDisplay();
}

/***************************************************************************//**
 * Checks if the given neighbor field is food.
 *
 *@note The purpose of this function is determine if the snake mouth is closed,
 *      or open.
 ******************************************************************************/
bool check_food_at_neighbor(map_coords_t current_field_coords,
                   const enum direction_t neighbor_field_direction,
                   const game_state_t *game_state)
{
  map_coords_t next_field_coords = get_neighbor_field_coords(current_field_coords,
                                                             neighbor_field_direction);
  if (game_state->map[next_field_coords.x][next_field_coords.y].state == FOOD_1 ||
      game_state->map[next_field_coords.x][next_field_coords.y].state == FOOD_2 ||
      game_state->map[next_field_coords.x][next_field_coords.y].state == FOOD_3)
    return true;
  else
    return false;
}

/***************************************************************************//**
 * Prints snake head to the given coodrinates of the map.
 ******************************************************************************/
void print_snake_head(const uint8_t x,
                      const uint8_t y,
                      const game_state_t *game_state)
{
  const uint8_t *selected_bitmap = NULL;

  switch (game_state->
          map[game_state->snake_head.x][game_state->snake_head.y].next_segment){
    case UP:
      if (check_food_at_neighbor(game_state->snake_head, DOWN, game_state))
        selected_bitmap = open_mouth_downBitmap;
      else
        selected_bitmap = head_downBitmap;
      break;

    case DOWN:
      if (check_food_at_neighbor(game_state->snake_head, UP, game_state))
        selected_bitmap = open_mouth_upBitmap;
      else
        selected_bitmap = head_upBitmap;
      break;

    case LEFT:
      if (check_food_at_neighbor(game_state->snake_head, RIGHT, game_state))
        selected_bitmap = open_mouth_rightBitmap;
      else
        selected_bitmap = head_rightBitmap;
      break;
    case RIGHT:
      if (check_food_at_neighbor(game_state->snake_head, LEFT, game_state))
        selected_bitmap = open_mouth_leftBitmap;
      else
        selected_bitmap = head_leftBitmap;
      break;

    default:
      return;
  }
  GLIB_drawBitmap(&glibContext,
                   x,
                   y,
                   FIELD_BITMAP_SIZE,
                   FIELD_BITMAP_SIZE,
                   selected_bitmap);
}

/***************************************************************************//**
 * Prints snake body to the given coodrinates of the map.
 ******************************************************************************/
void print_snake_body(const uint8_t x,
                      const uint8_t y,
                      const enum direction_t next_segment,
                      const enum direction_t previous_segment)
{
  const uint8_t *selected_bitmap = NULL;

  if ((next_segment == LEFT && previous_segment == RIGHT) ||
      (next_segment == RIGHT && previous_segment == LEFT))
    selected_bitmap = body_horizontalBitmap;

  else if ((next_segment == UP && previous_segment == DOWN) ||
           (next_segment == DOWN && previous_segment == UP))
    selected_bitmap = body_verticalBitmap;

  else if ((next_segment == LEFT && previous_segment == UP) ||
           (next_segment == UP && previous_segment == LEFT))
    selected_bitmap = body_up_leftBitmap;

  else if ((next_segment == RIGHT && previous_segment == UP) ||
           (next_segment == UP && previous_segment == RIGHT))
    selected_bitmap = body_up_rightBitmap;

  else if ((next_segment == LEFT && previous_segment == DOWN) ||
           (next_segment == DOWN && previous_segment == LEFT))
    selected_bitmap = body_down_leftBitmap;

  else if ((next_segment == RIGHT && previous_segment == DOWN) ||
           (next_segment == DOWN && previous_segment == RIGHT))
    selected_bitmap = body_down_rightBitmap;

  else {
      return;
  }
  GLIB_drawBitmap(&glibContext,
                   x,
                   y,
                   FIELD_BITMAP_SIZE,
                   FIELD_BITMAP_SIZE,
                   selected_bitmap);
}

/***************************************************************************//**
 * Prints snake tail to the given coodrinates of the map.
 ******************************************************************************/
void print_snake_tail(const uint8_t x,
                      const uint8_t y,
                      const enum direction_t previous_segment)
{

  const uint8_t *selected_bitmap = NULL;

  switch (previous_segment) {
    case UP:
      selected_bitmap = tail_upBitmap;
      break;

    case DOWN:
      selected_bitmap = tail_downBitmap;
      break;

    case LEFT:
      selected_bitmap = tail_leftBitmap;
      break;

    case RIGHT:
      selected_bitmap = tail_rightBitmap;
      break;

    default:
      return;
  }
  GLIB_drawBitmap(&glibContext,
                   x,
                   y,
                   FIELD_BITMAP_SIZE,
                   FIELD_BITMAP_SIZE,
                   selected_bitmap);
}

/***************************************************************************//**
 * Prints the map to the screen.
 *
 * @param x_coord x coordinate of the upper left corner of the map
 * @param y_coord y coordinate of the upper left corner of the map
 ******************************************************************************/
void print_map(const game_state_t *game_state, uint8_t x_coord, uint8_t y_coord)
{
  clear_screen_area(&glibContext,
                    0,
                    SL_MEMLCD_DISPLAY_WIDTH - 1,
                    y_coord, y_coord + (MAP_SIZE_Y * FIELD_BITMAP_SIZE));

  for (uint8_t x = 0; x < MAP_SIZE_X; x++)
    for (uint8_t y = 0; y < MAP_SIZE_Y; y++) {
        map_field_t selected_field = game_state->map[x][y];
        switch (selected_field.state) {
          case EMPTY:
            break;

          case SNAKE_HEAD:
            print_snake_head(x_coord + x*FIELD_BITMAP_SIZE,
                             y_coord + y*FIELD_BITMAP_SIZE,
                             game_state);
            break;

          case SNAKE_BODY:
            print_snake_body(x_coord + x*FIELD_BITMAP_SIZE,
                             y_coord + y*FIELD_BITMAP_SIZE,
                             selected_field.next_segment,
                             selected_field.previous_segment);
            break;
          case SNAKE_FULL_BELLY:
            GLIB_drawBitmap(&glibContext,
                            x_coord + x*FIELD_BITMAP_SIZE,
                            y_coord + y*FIELD_BITMAP_SIZE,
                            FIELD_BITMAP_SIZE,
                            FIELD_BITMAP_SIZE,
                            full_bellyBitmap);
            break;

          case SNAKE_TAIL:
            print_snake_tail(x_coord + x*FIELD_BITMAP_SIZE,
                             y_coord + y*FIELD_BITMAP_SIZE,
                             selected_field.previous_segment);
            break;

          case FOOD_1:
            GLIB_drawBitmap(&glibContext,
                            x_coord + x*FIELD_BITMAP_SIZE,
                            y_coord + y*FIELD_BITMAP_SIZE,
                            FIELD_BITMAP_SIZE,
                            FIELD_BITMAP_SIZE,
                            food_1Bitmap);
            break;

          case FOOD_2:
            GLIB_drawBitmap(&glibContext,
                            x_coord + x*FIELD_BITMAP_SIZE,
                            y_coord + y*FIELD_BITMAP_SIZE,
                            FIELD_BITMAP_SIZE,
                            FIELD_BITMAP_SIZE,
                            food_2Bitmap);
            break;

          case FOOD_3:
            GLIB_drawBitmap(&glibContext,
                            x_coord + x*FIELD_BITMAP_SIZE,
                            y_coord + y*FIELD_BITMAP_SIZE,
                            FIELD_BITMAP_SIZE,
                            FIELD_BITMAP_SIZE,
                            food_3Bitmap);
            break;

          case WALL:
            GLIB_drawBitmap(&glibContext,
                            x_coord + x*FIELD_BITMAP_SIZE,
                            y_coord + y*FIELD_BITMAP_SIZE,
                            FIELD_BITMAP_SIZE,
                            FIELD_BITMAP_SIZE,
                            wallBitmap);
            break;

          default:
            break;

        }
    }

  DMD_updateDisplay();

}

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * Init graphics.
 ******************************************************************************/
void init_graphics(void)
{
  uint32_t status;

  /* Enable the memory lcd */
  status = sl_board_enable_display();
  EFM_ASSERT(status == SL_STATUS_OK);

  /* Initialize the DMD support for memory lcd display */
  status = DMD_init(0);
  EFM_ASSERT(status == DMD_OK);

  /* Initialize the glib context */
  status = GLIB_contextInit(&glibContext);
  EFM_ASSERT(status == GLIB_OK);

  glibContext.backgroundColor = White;
  glibContext.foregroundColor = Black;

  /* Fill lcd with background color */
  GLIB_clear(&glibContext);
}

/***************************************************************************//**
 * Prints an error message, and an error code.
 *
 * This function never returns.
 * The msg length should be less than the MAX_STRING_LENGTH of the screen.
 *
 ******************************************************************************/
void print_error_msg(const char *msg, const int error_code)
{
  glibContext.backgroundColor = Black;
  glibContext.foregroundColor = White;
  /* Fill lcd with background color */
  GLIB_clear(&glibContext);

  GLIB_setFont(&glibContext, (GLIB_Font_t *) &GLIB_FontNormal8x8);
  if (strlen(msg) > MAX_STRING_LENGTH) {
    GLIB_drawStringOnLine(&glibContext,
                          "Too long err msg",
                          5,
                          GLIB_ALIGN_CENTER,
                          0,
                          0,
                          true);
  }
  else {
    GLIB_drawStringOnLine(&glibContext,
                          msg,
                          5,
                          GLIB_ALIGN_CENTER,
                          0,
                          0,
                          true);
    char return_code_text[MAX_STRING_LENGTH + 1];
    sprintf(return_code_text, "Error code: %d", error_code);
    GLIB_drawStringOnLine(&glibContext,
                          return_code_text,
                          6,
                          GLIB_ALIGN_CENTER,
                          0,
                          0,
                          true);
  }

  DMD_updateDisplay();

  while (1)
    ;
}



/***************************************************************************//**
 * Print a menu to the screen.
 ******************************************************************************/
void print_menu(const menu_t menu)
{
  // for display performance optimization
  static menu_t last_menu_state;


  if (strcmp(menu.title, last_menu_state.title) || menu_flag == false ) {

    strcpy(last_menu_state.title, menu.title);
    print_menu_header(menu.title);

  }

  if (menu_flag == false || (menu.is_ok_selected != last_menu_state.is_ok_selected)) {

    menu_flag = true;
    print_screen_bottom("BTN1: up",
                        "BTN0: down",
                        "OK",
                        menu.is_ok_selected);

  }

  last_menu_state = menu;

  print_menu_items(menu);
}

/***************************************************************************//**
 * Prints the in-game tail to the screen.
 ******************************************************************************/
void print_ingame_tail(const enum event_t touch_slider_state)
{
  if (touch_slider_state == TOUCH_SLIDER_RIGHT_PUSH)
    print_screen_bottom("BTN1:left",
                      "BTN0:right",
                      "pause",
                      true);
  else if (touch_slider_state == TOUCH_SLIDER_RIGHT_CANCEL)
    print_screen_bottom("BTN1:left",
                      "BTN0:right",
                      "pause",
                      false);
}

/***************************************************************************//**
 * Prints the current snapshot of the game to the screen.
 ******************************************************************************/
void print_game(const game_state_t *game_state)
{
  static float last_score = FLT_MAX;
  uint32_t current_score = (uint32_t)game_state->score;

  if (last_score != current_score || menu_flag == true ) {
    print_ingame_header(game_state->score, false);
    last_score = (uint32_t)game_state->score;
  }

  if (menu_flag == true) {
      print_screen_bottom("BTN1:left",
                        "BTN0:right",
                        "menu",
                        false);
      menu_flag = false;
  }

  print_map(game_state, 1, 11);
}

/***************************************************************************//**
 * Prints the game over screen to the screen.
 ******************************************************************************/
void print_game_over(const uint32_t score, const enum event_t touch_slider_state)
{
  //used for the first call
  if (touch_slider_state == UNDETERMINED)
    print_ingame_header(score, true);


  if (touch_slider_state == TOUCH_SLIDER_RIGHT_PUSH)
    print_screen_bottom("",
                        "",
                        "main menu",
                        true);
  else if (touch_slider_state == UNDETERMINED ||
           touch_slider_state == TOUCH_SLIDER_RIGHT_CANCEL)
    print_screen_bottom("",
                        "",
                        "main menu",
                        false);
}

