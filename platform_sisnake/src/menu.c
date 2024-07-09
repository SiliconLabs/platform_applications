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

#include <stdbool.h>
#include <string.h>

#include "graphics.h"
#include "types.h"
#include "app_csen.h"
#include "game.h"
#include "types.h"

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/

static menu_t menu;

/*******************************************************************************
 **************************   LOCAL FUNCTIONS   ********************************
 ******************************************************************************/

/***************************************************************************//**
 * Sets the main menu.
 ******************************************************************************/
void set_main_menu(menu_t *menu)
{
  sl_strcpy_s(menu->title, sl_strlen("Sisnake\n"), "Sisnake");
  menu->menu_element_count = 3;
  sl_strcpy_s(menu->menu_elements[0], sl_strlen("new game\n"), "new game");
  sl_strcpy_s(menu->menu_elements[1], sl_strlen("difficulty\n"), "difficulty");
  sl_strcpy_s(menu->menu_elements[2], sl_strlen("maze\n"), "maze");
  menu->choosen_menu_element_index = 0;
  menu->is_ok_selected = false;
}

/***************************************************************************//**
 * Sets the difficulty menu.
 ******************************************************************************/
void set_difficulty_menu(menu_t *menu)
{
  sl_strcpy_s(menu->title, sl_strlen("Select diff\n"), "Select diff");
  menu->menu_element_count = 7;
  sl_strcpy_s(menu->menu_elements[0], sl_strlen("level 1\n"), "level 1");
  sl_strcpy_s(menu->menu_elements[1], sl_strlen("level 2\n"), "level 2");
  sl_strcpy_s(menu->menu_elements[2], sl_strlen("level 3\n"), "level 3");
  sl_strcpy_s(menu->menu_elements[3], sl_strlen("level 4\n"), "level 4");
  sl_strcpy_s(menu->menu_elements[4], sl_strlen("level 5\n"), "level 5");
  sl_strcpy_s(menu->menu_elements[5], sl_strlen("level 6\n"), "level 6");
  sl_strcpy_s(menu->menu_elements[6], sl_strlen("level 7\n"), "level 7");
  menu->choosen_menu_element_index = 0;
  menu->is_ok_selected = false;
}

/***************************************************************************//**
 * Sets the maze menu.
 ******************************************************************************/
void set_maze_menu(menu_t *menu)
{
  sl_strcpy_s(menu->title, sl_strlen("Select maze\n"), "Select maze");
  menu->menu_element_count = 3;
  sl_strcpy_s(menu->menu_elements[0], sl_strlen("none\n"), "none");
  sl_strcpy_s(menu->menu_elements[1], sl_strlen("border\n"), "border");
  sl_strcpy_s(menu->menu_elements[2], sl_strlen("cross\n"), "cross");
  menu->choosen_menu_element_index = 0;
  menu->is_ok_selected = false;
}

/***************************************************************************//**
 * Sets the pause menu.
 ******************************************************************************/
void set_pause_menu(menu_t *menu)
{
  sl_strcpy_s(menu->title, sl_strlen("Paused\n"), "Paused");
  menu->menu_element_count = 3;
  sl_strcpy_s(menu->menu_elements[0], sl_strlen("continue\n"), "continue");
  sl_strcpy_s(menu->menu_elements[1], sl_strlen("new game\n"), "new game");
  sl_strcpy_s(menu->menu_elements[2], sl_strlen("main menu\n"), "main menu");
  menu->choosen_menu_element_index = 0;
  menu->is_ok_selected = false;
}

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * Init main menu.
 ******************************************************************************/
void init_main_menu(void)
{
  set_main_menu(&menu);
  print_menu(menu);
}

/***************************************************************************//**
 * Init pause menu.
 ******************************************************************************/
void init_pause(void)
{
  set_pause_menu(&menu);
  print_menu(menu);
}

/***************************************************************************//**
 * Make a menu tick.
 *
 * Make changes in the current menu driven by a user input.
 * It the current menu session should be ended,
 * it returns with the related order.
 * Or else with no_action.
 *
 ******************************************************************************/
enum menu_return_t menu_tick(const enum event_t event)
{
  switch (event) {
    case BTN0:
      if (menu.choosen_menu_element_index == menu.menu_element_count - 1) {
        menu.choosen_menu_element_index = 0;
      } else {
        menu.choosen_menu_element_index++;
      }
      break;

    case BTN1:
      if (menu.choosen_menu_element_index == 0) {
        menu.choosen_menu_element_index = menu.menu_element_count - 1;
      } else {
        menu.choosen_menu_element_index--;
      }
      break;

    case TOUCH_SLIDER_RIGHT_PUSH:
      menu.is_ok_selected = true;
      break;

    case TOUCH_SLIDER_RIGHT_RELEASE:
      menu.is_ok_selected = false;
      if (!strcmp(menu.title, "Sisnake")) {
        switch (menu.choosen_menu_element_index) {
          case 0:
            return START_NEW_GAME;

          case 1:
            set_difficulty_menu(&menu);
            break;

          case 2:
            set_maze_menu(&menu);
            break;
        }
      } else if (!strcmp(menu.title, "Select diff")) {
        set_difficulty(menu.choosen_menu_element_index);
        set_main_menu(&menu);
      } else if (!strcmp(menu.title, "Select maze")) {
        switch (menu.choosen_menu_element_index) {
          case 0:
            set_maze(NONE);
            break;

          case 1:
            set_maze(BORDER);
            break;

          case 2:
            set_maze(CROSS);
            break;
        }
        set_main_menu(&menu);
      } else if (!strcmp(menu.title, "Paused")) {
        switch (menu.choosen_menu_element_index) {
          case 0:
            return CONTINUE;

          case 1:
            return START_NEW_GAME;

          case 2:
            set_main_menu(&menu);
            break;
        }
      }
      break;

    case TOUCH_SLIDER_RIGHT_CANCEL:
      menu.is_ok_selected = false;
      break;

    default:
      break;
  }

  print_menu(menu);

  return NO_ACTION;
}
