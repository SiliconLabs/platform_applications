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

#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "uart.h"
#include "motor.h"
#include "logging.h"
#include "pid.h"
#include "config.h"
#include "button.h"
#include "kit.h"
#include "debug.h"

extern bool isRunning;
volatile uint32_t msTicks = 0;

uint16_t buttons;
uint16_t lastButtons = 0xFFFF;
uint16_t validButtons;
bool released = false;

void SysTick_Handler(void)
{
  msTicks++;       /* increment counter necessary in Delay()*/
}

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  if (SysTick_Config(CMU_ClockFreqGet(cmuClock_CORE) / 1000)) {
    while (1) {}
  }

  kitInit();
  uartInit();
  LOG_INIT();
  buttonInit();
  debugInit();

  msTicks = 0;
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
  /* Check if any of the push buttons are pressed */

  buttons = buttonDetect();
  if (buttons != lastButtons) {
    if (buttons != 0x0) {
      validButtons = buttons;
      msTicks = 0;
      released = 0;
    } else {
      released = 1;
    }
    lastButtons = buttons;

    if (released == 1) {
      if (validButtons & 0x1) {
        if (isRunning) {
          stopMotor();
        } else {
          setDirection(!getDirection());
        }
      } else if (validButtons & 0x2) {
        if (isRunning) {
          if (msTicks > 500) {
            speedDecrease();
          } else {
            speedIncrease();
          }
        } else {
          startMotor();
        }
      }
      msTicks = 0;
    }
  }
}
