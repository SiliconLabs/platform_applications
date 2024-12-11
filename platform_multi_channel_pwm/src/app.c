/***************************************************************************//**
 * @file
 * @brief app.c
 *******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/

// -----------------------------------------------------------------------------
//                               Includes
// -----------------------------------------------------------------------------
#include "em_gpio.h"
#include "em_timer.h"
#include "em_cmu.h"
#include "sl_udelay.h"

// -----------------------------------------------------------------------------
//                               Macros
// -----------------------------------------------------------------------------
#define PWM_FREQ                  1000

#define PWM_CHANNEL0_PORT         gpioPortD
#define PWM_CHANNEL0_PIN          3

#define PWM_CHANNEL1_PORT         gpioPortD
#define PWM_CHANNEL1_PIN          2

#define PWM_CHANNEL2_PORT         gpioPortB
#define PWM_CHANNEL2_PIN          1

// -----------------------------------------------------------------------------
//                       Local Variables
// -----------------------------------------------------------------------------
static const uint32_t dutyCycle[] = {
  0, 1, 1, 1, 2, 2, 2, 2, 2, 2,
  2, 3, 3, 3, 3, 3, 4, 4, 4, 4,
  5, 5, 5, 5, 6, 6, 6, 7, 7, 7,
  8, 8, 8, 9, 9, 10, 10, 10, 11, 11,
  12, 12, 13, 13, 14, 15, 15, 16, 17, 17,
  18, 19, 19, 20, 21, 22, 23, 23, 24, 25,
  26, 27, 28, 29, 30, 31, 32, 34, 35, 36,
  37, 39, 40, 41, 43, 44, 46, 48, 49, 51,
  53, 54, 56, 58, 60, 62, 64, 66, 68, 71,
  73, 75, 78, 80, 83, 85, 88, 91, 94, 97,
  100,
};

/**************************************************************************//**
 * @brief
 *    GPIO initialization
 *****************************************************************************/
void initGPIO(void)
{
  GPIO_PinModeSet(PWM_CHANNEL0_PORT, PWM_CHANNEL0_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(PWM_CHANNEL1_PORT, PWM_CHANNEL1_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(PWM_CHANNEL2_PORT, PWM_CHANNEL2_PIN, gpioModePushPull, 0);
}

/**************************************************************************//**
 * @brief
 *    CMU initialization
 *****************************************************************************/
void initCMU(void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(cmuClock_TIMER0, true);
}

/**************************************************************************//**
 * @brief
 *    TIMER initialization
 *****************************************************************************/
void initTIMER(void)
{
  uint32_t timerFreq, topValue;
  TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
  TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;

  // Don't start counter on initialization
  timerInit.enable = false;

  // PWM mode sets/clears the output on compare/overflow events
  timerCCInit.mode = timerCCModePWM;

  TIMER_Init(TIMER0, &timerInit);

  // Route CC0 output to PA6
  GPIO->TIMERROUTE[0].ROUTEEN = GPIO_TIMER_ROUTEEN_CC0PEN
                                | GPIO_TIMER_ROUTEEN_CC1PEN
                                | GPIO_TIMER_ROUTEEN_CC2PEN;

  GPIO->TIMERROUTE[0].CC0ROUTE =
    (PWM_CHANNEL0_PORT << _GPIO_TIMER_CC0ROUTE_PORT_SHIFT)
    | (PWM_CHANNEL0_PIN <<
       _GPIO_TIMER_CC0ROUTE_PIN_SHIFT);

  GPIO->TIMERROUTE[0].CC1ROUTE =
    (PWM_CHANNEL1_PORT << _GPIO_TIMER_CC0ROUTE_PORT_SHIFT)
    | (PWM_CHANNEL1_PIN <<
       _GPIO_TIMER_CC0ROUTE_PIN_SHIFT);

  GPIO->TIMERROUTE[0].CC2ROUTE =
    (PWM_CHANNEL2_PORT << _GPIO_TIMER_CC0ROUTE_PORT_SHIFT)
    | (PWM_CHANNEL2_PIN <<
       _GPIO_TIMER_CC0ROUTE_PIN_SHIFT);

  TIMER_InitCC(TIMER0, 0, &timerCCInit);
  TIMER_InitCC(TIMER0, 1, &timerCCInit);
  TIMER_InitCC(TIMER0, 2, &timerCCInit);

  // Set top value to overflow at the desired PWM_FREQ frequency
  timerFreq = CMU_ClockFreqGet(cmuClock_TIMER0) / (timerInit.prescale + 1);
  topValue = (timerFreq / PWM_FREQ);
  TIMER_TopSet(TIMER0, topValue);

  // Now start the TIMER
  TIMER_Enable(TIMER0, true);

  // Trigger DMA on compare event to set CCVB to update duty cycle on next
  //   period
  TIMER_IntEnable(TIMER0, TIMER_IEN_CC0);
}

/**************************************************************************//**
 * @brief
 *    Set PWM Duty Cycle
 *****************************************************************************/
void PWMsetDutyCycle(uint8_t chan0,
                     uint8_t chan1,
                     uint8_t chan2)
{
  uint32_t top = TIMER_TopGet(TIMER0);

  // Set compare value
  TIMER_CompareBufSet(TIMER0, 0, (top * chan0) / 100);
  TIMER_CompareBufSet(TIMER0, 1, (top * chan1) / 100);
  TIMER_CompareBufSet(TIMER0, 2, (top * chan2) / 100);
}

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  initCMU();
  initGPIO();
  initTIMER();
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
  for (uint8_t i = 0; i < 100; i++) {
    PWMsetDutyCycle(dutyCycle[i],
                    dutyCycle[100 - i],
                    50);
    sl_udelay_wait(10000);
    if (i == 0) {
      sl_udelay_wait(500000);
    }
  }
}
