/***************************************************************************//**
 * @file
 * @brief Top level application functions
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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
#include "keyscan_driver.h"
#include "keyscan_driver_config.h"

#include "glib.h"
#include "dmd.h"

#include "em_assert.h"
#include "sl_board_control.h"
#include "sl_power_manager.h"

/*******************************************************************************
 ******************************  DEFINES **************************************
 ******************************************************************************/
#define FIFO_BUFFER_SIZE 24

/*******************************************************************************
 **************************  GLOBAL VARIABLES   ********************************
 ******************************************************************************/

// Keyboard Mapping
static const char keypad[SL_KEYSCAN_DRIVER_ROW_NUMBER][
  SL_KEYSCAN_DRIVER_COLUMN_NUMBER] = {
  { '1', '4', '7', '*' },
  { '2', '5', '8', '0' },
  { '3', '6', '9', '#' },
};

// Store the pressed keys in a FIFO buffer
static char pressed_key[FIFO_BUFFER_SIZE] = { 0 };
// buffer index
static uint8_t write_index = 0;
static uint8_t read_index = 0;

// Variables for the visualisation:
static GLIB_Context_t glibContext;

// flag to enable the sleep mode
static volatile bool ok_to_sleep = true;

/*******************************************************************************
 *********************   STATIC FUNCTION DEFINITION ****************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief Called when a key on the keypad is pressed.
 ******************************************************************************/
static void on_event(uint8_t *p_keyscan_matrix,
                     sl_keyscan_driver_status_t status);

/***************************************************************************//**
 * @brief Map the pushed key and load in to the buffer
 ******************************************************************************/
static void map_key(uint8_t *scan_result);

/***************************************************************************//**
 * @brief GLIB and DMD initialization
 ******************************************************************************/
static void init_display(void);

/***************************************************************************//**
 * @brief Update the display with the pressed keys
 ******************************************************************************/
static void update_display(char key);

/***************************************************************************//**
 * @brief PowerManager initialization
 ******************************************************************************/
static void init_powermanager(void);

/***************************************************************************//**
 * @brief Sleep mode  transition handling.
 ******************************************************************************/
static void sleep_mode_transition(sl_power_manager_em_t from,
                                  sl_power_manager_em_t to);

// Register the power manager event
sl_power_manager_em_transition_event_handle_t event_handle;
sl_power_manager_em_transition_event_info_t event_info = {
  .event_mask = SL_POWER_MANAGER_EVENT_TRANSITION_ENTERING_EM0
                | SL_POWER_MANAGER_EVENT_TRANSITION_LEAVING_EM0,
  .on_event = sleep_mode_transition
};

/*******************************************************************************
 *********************  GLOBAL FUNCTION DECLARATION ****************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief Initialize application.
 ******************************************************************************/
void app_init(void)
{
  // Initialize the display
  init_display();

  // Register the keyscan handle.
  static sl_keyscan_driver_process_keyscan_event_handle_t handle =
  {
    .on_event = on_event,
  };

  sl_keyscan_driver_subscribe_event(&handle);

  // Start Scannig the keypad
  sl_keyscan_driver_start_scan();

  // Initialize power manager
  init_powermanager();
}

/***************************************************************************//**
 * @brief App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
  static uint8_t performed_writing = false;

  // If the two index is equal, then the buffer is empty
  if (write_index != read_index) {
    // Update the display with the pressed key
    update_display(pressed_key[read_index]);
    read_index++;
    if (read_index >= FIFO_BUFFER_SIZE) {
      read_index = 0;
    }
    // The key was written out so I could go to sleep.
    performed_writing = true;
  }
  if (performed_writing) {
    // All the keys I pressed were written out so I could go to sleep.
    ok_to_sleep = true;
    performed_writing = false;
  }
}

/***************************************************************************//**
 * @brief indicates to the power manager that the app is ready to sleep
 ******************************************************************************/
bool app_is_ok_to_sleep(void)
{
  return ok_to_sleep;
}

/*******************************************************************************
 *********************  STATIC FUNCTION DECLARATION ****************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief Called when a key on the keypad is pressed.
 ******************************************************************************/
static void on_event(uint8_t *p_keyscan_matrix,
                     sl_keyscan_driver_status_t status)
{
#if (SL_KEYSCAN_DRIVER_SINGLEPRESS) // When the SinglePress feature is enabled
  static uint8_t press = false;
  // If a key is pressed and not being pressed, then map the key and set the
  //   press flag to true.
  if ((status == SL_KEYSCAN_STATUS_KEYPRESS_VALID) && (!press)) {
    map_key(p_keyscan_matrix);
    press = true;
  }
  // If a key is released and being pressed, then reset the press flag to false.
  if ((status == SL_KEYSCAN_STATUS_KEYPRESS_RELEASED) && (press)) {
    press = false;
    // The key released so I can go back to sleep.
    ok_to_sleep = true;
  }
#else
  if (status == SL_KEYSCAN_STATUS_KEYPRESS_RELEASED) {
    map_key(p_keyscan_matrix);
  }
#endif
}

/***************************************************************************//**
 * @brief Map the pushed key and load in to the buffer
 ******************************************************************************/
static void map_key(uint8_t *scan_result)
{
  uint8_t offset_fix = 0;
  uint8_t i = 0;
  uint8_t j = 0;
  while (i < SL_KEYSCAN_DRIVER_COLUMN_NUMBER) {
    while (j < SL_KEYSCAN_DRIVER_ROW_NUMBER)
    {
      if (scan_result[i] & (1 << j)) {
#if (SL_KEYSCAN_DRIVER_SINGLEPRESS) // When the SinglePress feature is enabled
        offset_fix = i;
#else
        // The returned column number contains a fix offset error.
        // So I use the offset_fix to fix the column number error.
        offset_fix = i - 1;
        // I checked for the 0xFF because I used the uint_8 type.
        if (offset_fix == 0xFF) {
          offset_fix = SL_KEYSCAN_DRIVER_COLUMN_NUMBER - 1;
        }
#endif
        // Load the pressed key in to the buffer and increment the write index.
        // The buffer size is 24, so if there are more than 24 pressed keys, the
        //   oldest key will be overwritten.
        pressed_key[write_index] = keypad[j][offset_fix];
        write_index++;
        if (write_index >= FIFO_BUFFER_SIZE) {
          write_index = 0;
        }
// The single press feature is enabled, so after saving a key, it will return
//   immediately.
// If the SinglePress feature is disabled, it will write all the keys and then
//   return.
#if (SL_KEYSCAN_DRIVER_SINGLEPRESS)
        return;
#endif
      }
      j++;
    }
    j = 0;
    i++;
  }
}

/***************************************************************************//**
 * @brief GLIB and DMD initialization
 ******************************************************************************/
static void init_display(void)
{
  uint32_t status;
  static int currentLine = 0;

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

  /* Use Narrow font */
  GLIB_setFont(&glibContext, (GLIB_Font_t *) &GLIB_FontNarrow6x8);

  /*Draw the start screen on to the Display*/
  GLIB_drawLineH(&glibContext, 0, 2, 128);
  GLIB_drawStringOnLine(&glibContext,
                        "Keyscan Sample App",
                        currentLine++,
                        GLIB_ALIGN_CENTER,
                        0,
                        7,
                        true);

  GLIB_drawLineH(&glibContext, 0, 17, 128);

  GLIB_drawStringOnLine(&glibContext,
                        "Pressed Key: ",
                        currentLine++,
                        GLIB_ALIGN_LEFT,
                        10,
                        11,
                        true);

  GLIB_Rectangle_t rect = { 0, 40, 115, 47 };
  GLIB_drawRectFilled(&glibContext, &rect);

  DMD_updateDisplay();
}

/***************************************************************************//**
 * @brief Update the display with the pressed keys
 ******************************************************************************/
static void update_display(char key)
{
  static int32_t x = 6;
  static int32_t y = 40;
  // Write out a space to the screen
  GLIB_drawChar(&glibContext, ' ', x, y, true);
  // Write out the pressed key to the screen
  GLIB_drawChar(&glibContext, key, x + 6, y, true);
  // Move the pointer to the next position in the screen
  x += 12;
  if (x > 110) {
    // Remove the cursor from the actual line
    GLIB_drawChar(&glibContext, ' ', 0, y, true);
    GLIB_drawChar(&glibContext, ' ', x, y, true);
    // Move the pointers to the next position in the screen
    y += 15;
    x = 5;
    if (y > 120) {
      y = 40;
    }
    // Move the cursor line to the next line in the screen
    GLIB_Rectangle_t rect = { 0, y, 115, y + 7 };
    GLIB_drawRectFilled(&glibContext, &rect);
  }
  DMD_updateDisplay();
}

/***************************************************************************//**
 * @brief PowerManager initialization
 ******************************************************************************/
static void init_powermanager(void)
{
  // Subscribe to EM transition event
  sl_power_manager_subscribe_em_transition_event(&event_handle, &event_info);
  // Add required EM mode
  sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM2);
}

/***************************************************************************//**
 * @brief Sleep mode  transition handling.
 ******************************************************************************/
void sleep_mode_transition(sl_power_manager_em_t from,
                           sl_power_manager_em_t to)
{
  if (from == SL_POWER_MANAGER_EM0) {
    GLIB_displaySleep();
  }
  if (to == SL_POWER_MANAGER_EM0) {
    sl_board_enable_display();
    GLIB_displayWakeUp();
    // I don't go back to sleep, while the button pressed is not processed
    ok_to_sleep = false;
  }
}
