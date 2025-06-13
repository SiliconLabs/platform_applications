/***************************************************************************//**
 * @file app.c
 * @brief Example that illustrates how to determine the amount of time
 *        spent in EM4 using the BURTC
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
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

#include "string.h"

#include "sl_board_control.h"
#include "sl_simple_button_instances.h"
#include "em_assert.h"
#include "glib.h"
#include "dmd.h"

#include "em_burtc.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_rmu.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

#ifndef BUTTON_INSTANCE_0
#define BUTTON_INSTANCE_0   sl_button_btn0
#endif

#ifndef LCD_MAX_LINES
#define LCD_MAX_LINES       11
#endif

/*******************************************************************************
 **************************   GLOBAL VARIABLES   *******************************
 ******************************************************************************/
extern uint32_t em4Cnt_value;

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/
static GLIB_Context_t glibContext;
static int currentLine = 0;

/**************************************************************************//**
 * @brief  BURTC initialization to track EM4 duration
 *
 * @note It would be customary to call CMU_ClockSelectSet() to specify
 * the oscillator for the EM4GRPACLK, which clocks the BURTC counter,
 * before configuring the BURTC.
 *
 * However, this example uses the memory LCD, the driver for which
 * select the LFXO as the source of the EM4GRPACLK before this function
 * is called.
 *****************************************************************************/
void burtc_init()
{
  CMU_ClockEnable(cmuClock_BURTC, true);

  /*
   * Set prescale to 32768 so each tick of CNT is 1 second, but DO NOT
   * start counting after initialization.
   */
  BURTC_Init_TypeDef burtcInit = BURTC_INIT_DEFAULT;
  burtcInit.clkDiv = 32768;
  burtcInit.start = false;
  BURTC_Init(&burtcInit);
}

/**************************************************************************//**
 * @brief  Configure BURTC to count using the LFRCO in EM4
 *****************************************************************************/
void enable_em4_gpio_wakeup(void)
{
  // Configure Button PB1 as input and EM4 wake-up source
  GPIO_PinModeSet(GPIO_EM4WU4_PORT, GPIO_EM4WU4_PIN, gpioModeInputPullFilter,
                  1);

  // Enable GPIO pin wake-up from EM4; PB1 is EM4WUEN pin 4
  GPIO_EM4EnablePinWakeup(GPIO_IEN_EM4WUIEN4, 0);
}

/**************************************************************************//**
 * @brief  Quick and dirty integer to ASCII string
 *****************************************************************************/
char *itoa(uint32_t val, uint32_t base)
{
  static char buf[32] = { 0 };

  int i = 30;

  for (; val && i ; --i, val /= base) {
    buf[i] = "0123456789abcdef"[val % base];
  }

  return &buf[i + 1];
}

/***************************************************************************//**
 * Print start-up message
 ******************************************************************************/
void app_init()
{
  uint32_t status;

  burtc_init();

  // Enable memory LCD
  status = sl_board_enable_display();
  EFM_ASSERT(status == SL_STATUS_OK);

  // Initialize DMD support for memory LCD
  status = DMD_init(0);
  EFM_ASSERT(status == DMD_OK);

  // Initialize GLIB context
  status = GLIB_contextInit(&glibContext);
  EFM_ASSERT(status == GLIB_OK);

  glibContext.backgroundColor = White;
  glibContext.foregroundColor = Black;

  // Fill LCD with background color
  GLIB_clear(&glibContext);
  DMD_updateDisplay();

  // Use narrow font
  GLIB_setFont(&glibContext, (GLIB_Font_t *) &GLIB_FontNarrow6x8);

  // If last reset was wake from EM4, print the duration
  uint32_t rstCause = RMU_ResetCauseGet();
  RMU_ResetCauseClear();

  if ((rstCause & EMU_RSTCAUSE_EM4) != 0) {
    char *em4Cnt_text = itoa(em4Cnt_value, 10);

    GLIB_drawStringOnLine(&glibContext, "Time spent in EM4:",
                          currentLine++, GLIB_ALIGN_LEFT,
                          5, 5, true);

    GLIB_drawStringOnLine(&glibContext, strcat(em4Cnt_text, " seconds"),
                          currentLine++, GLIB_ALIGN_LEFT,
                          5, 5, true);
  }

  GLIB_drawStringOnLine(&glibContext, "Enter EM4 with BTN0",
                        currentLine++, GLIB_ALIGN_LEFT,
                        5, 5, true);

  GLIB_drawStringOnLine(&glibContext, "BTN1 for EM4 wake-up",
                        currentLine++, GLIB_ALIGN_LEFT,
                        5, 5, true);

  DMD_updateDisplay();
}

/***************************************************************************//**
 * Ticking function.  Not used in this example.
 ******************************************************************************/
void app_process_action(void)
{
  return;
}

/***************************************************************************//**
 * Callback on button change.
 *
 * This function overrides a weak implementation defined in the simple_button
 * module. It is triggered when the user presses BTN0 to enter EM4.
 *
 ******************************************************************************/
void sl_button_on_change(const sl_button_t *handle)
{
  if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_PRESSED) {
    if (&BUTTON_INSTANCE_0 == handle) {
      GLIB_drawStringOnLine(&glibContext, "Entering EM4",
                            currentLine++, GLIB_ALIGN_LEFT,
                            5, 5, true);
      DMD_updateDisplay();

      // Disable PB0 and enable EM4 wake-up on PB1
      sl_button_disable(handle);
      enable_em4_gpio_wakeup();

      // Set CNT to 0 and start counting
      BURTC_CounterReset();

      // Enter EM4; retain GPIOs so screen does not blank
      EMU_EM4Init_TypeDef initEm4 = EMU_EM4INIT_DEFAULT;
      initEm4.pinRetentionMode = emuPinRetentionLatch;
      EMU_EM4Init(&initEm4);
      EMU_EnterEM4();
    }
  }
}
