/***************************************************************************//**
 * @file
 * @brief Main app logic.
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

#include <menu.h>
#include <time.h>
#include <stdlib.h>

#include "em_rtcc.h"
#include "sl_sleeptimer.h"
#include "sl_simple_button_instances.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

#include "types.h"
#include "app_csen.h"
#include "game.h"
#include "graphics.h"
#include "menu.h"

/*******************************************************************************
 ***************************  GLOBAL VARIABLES   *******************************
 ******************************************************************************/

enum event_t irq_type = UNDETERMINED;

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/

sl_sleeptimer_timer_handle_t turn_timer;
enum state_t app_state;

/*******************************************************************************
 **************************   LOCAL FUNCTIONS   ********************************
 ******************************************************************************/

/***************************************************************************//**
 * Callback on button change.
 *
 * This function overrides a weak implementation defined in the simple_button
 * module. It is triggered when the user activates one of the buttons.
 *
 ******************************************************************************/
void sl_button_on_change(const sl_button_t *handle)
{
  if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_PRESSED) {
    if (&sl_button_btn0 == handle) {
      irq_type = BTN0;
    } else if (&sl_button_btn1 == handle) {
      irq_type = BTN1;
    }
  }
}

/***************************************************************************//**
 * Callback on periodic timeout.
 ******************************************************************************/
static void turn_callback(sl_sleeptimer_timer_handle_t *handle,
                          void *data)
{
  (void)&handle;
  (void)&data;
  irq_type = TURN;
}

/***************************************************************************//**
 * Generate a random uint with mbedtls.
 ******************************************************************************/
unsigned int generate_random_number(void)
{
  static bool mbedtlsInit = false;
  static mbedtls_entropy_context entropyCtx;
  static mbedtls_ctr_drbg_context drbgCtx;
  static const char pers[] = "enter_the_snatrix"; // It's bad, I know.
  int mbedtlsStatus;

  // Initialize mbedTLS RNG module for random number gen
  if (!mbedtlsInit) {
    mbedtls_ctr_drbg_init(&drbgCtx);
    mbedtls_entropy_init(&entropyCtx);
    mbedtlsStatus = mbedtls_ctr_drbg_seed(&drbgCtx,
                                          mbedtls_entropy_func,
                                          &entropyCtx,
                                          (const unsigned char *) pers,
                                          sizeof pers);
    if (mbedtlsStatus != 0) {
      print_error_msg("mbedtls_seed", mbedtlsStatus);
    }
    mbedtlsInit = true;
  }

  // generate a random number
  unsigned int random_number = 0;
  unsigned char random[sizeof(unsigned int)];
  if (mbedtlsInit) {
    mbedtlsStatus = mbedtls_ctr_drbg_random(&drbgCtx,
                                            random,
                                            sizeof(unsigned int));
    if (mbedtlsStatus != 0) {
      print_error_msg("mbedtls_random", mbedtlsStatus);
    }
    for (unsigned int i = 0; i < sizeof(unsigned int); i++ ) {
      random_number = ((random_number << 8) | random[i]);
    }
  }

  return random_number;
}

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * Initialize Sisnake.
 ******************************************************************************/
void sisnake_init(void)
{
  init_graphics();
  srand(generate_random_number());
  setup_CSEN(); // setup touch slider
  app_state = MENUS;
  init_main_menu();
}

/***************************************************************************//**
 * Sisnake main mechanics state machine.
 *
 * The game updates only when user input occur, or the next turn of the game is
 * initiated by the periodic sleep timer in case of game state.
 *
 ******************************************************************************/
#pragma GCC push_options
#pragma GCC optimize (0) // GNU ARM v7.2.1 optimization bug override

void sisnake_process_action(void)
{
  // 2 elements for register double push (for quick turns)
  static enum relative_direction_t next_direction[2] = { R_FORWARD, R_FORWARD };

  enum move_snake_return_t turn_result;

  // set the event
  enum event_t event = UNDETERMINED;
  if (irq_type != UNDETERMINED) {
    event = irq_type;
    irq_type = UNDETERMINED;
  }

  switch (app_state)
  {
    case MENUS:
      if ((event == BTN0) || (event == BTN1)) {
        menu_tick(event);
      } else if ((event == TOUCH_SLIDER_RIGHT_PUSH)
                 || (event == TOUCH_SLIDER_RIGHT_RELEASE)
                 || (event == TOUCH_SLIDER_RIGHT_CANCEL)) {
        enum menu_return_t menu_return = menu_tick(event);
        if (menu_return == START_NEW_GAME) {
          sl_sleeptimer_restart_periodic_timer_ms(&turn_timer,
                                                  800 / (get_difficulty() + 1),
                                                  turn_callback,
                                                  NULL,
                                                  0,
                                                  SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
          init_game();
          app_state = GAME;
        } else if (menu_return == CONTINUE) {
          app_state = GAME;
        }
      }
      break;

    case GAME:
      if (event == TURN) {
        if (next_direction[1] != R_FORWARD) {
          turn_result = game_turn(next_direction[1]);
          next_direction[1] = R_FORWARD;
        } else {
          turn_result = game_turn(next_direction[0]);
          next_direction[0] = R_FORWARD;
        }
        if (turn_result == CRASH) {
          sl_sleeptimer_stop_timer(&turn_timer);
          game_over_tick(UNDETERMINED);
          app_state = GAME_OVER;
        }
      } else if (event == BTN0) {
        if (next_direction[0] == R_FORWARD) {
          next_direction[0] = R_RIGHT;
        } else if (next_direction[1] == R_FORWARD) {
          next_direction[1] = R_RIGHT;
        }
      } else if (event == BTN1) {
        if (next_direction[0] == R_FORWARD) {
          next_direction[0] = R_LEFT;
        } else if (next_direction[1] == R_FORWARD) {
          next_direction[1] = R_LEFT;
        }
      } else if ((event == TOUCH_SLIDER_RIGHT_PUSH)
                 || (event == TOUCH_SLIDER_RIGHT_CANCEL)) {
        print_ingame_tail(event);
      } else if (event == TOUCH_SLIDER_RIGHT_RELEASE) {
        app_state = MENUS;
        init_pause();
      }
      break;

    case GAME_OVER:
      if (event == TOUCH_SLIDER_RIGHT_RELEASE) {
        app_state = MENUS;
        init_main_menu();
      } else if ((event == TOUCH_SLIDER_RIGHT_PUSH)
                 || (event == TOUCH_SLIDER_RIGHT_CANCEL)) {
        game_over_tick(event);
      }
      break;
  }
}

#pragma GCC pop_options
