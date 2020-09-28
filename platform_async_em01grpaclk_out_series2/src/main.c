/**************************************************************************//**
 * @file main.c
 *
 * @version 1.0.2
 *
 * @brief This example drives a 50% duty cycle 6 MHz clock generated
 * by TIMER0 on PA6 (expansion header pin 14).  The DPLL is set to run
 * at 36 MHz for this purpose.  By default, the EM01GRPACLK is sourced
 * from the DPLL and certain peripherals also either run from or default
 * to this clock: TIMERs, EUART, and IADC.
 *
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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
 *******************************************************************************
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/

#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_timer.h"

#include "bsp.h"

// Clock state
typedef struct {
  bool      running;    // Is the clock currently running
  bool      offstate;   // Is the clock low or high when stopped
  uint32_t  frequency;  // Desired output frequency
  uint32_t  top;        // TIMER_TOP register value
  uint32_t  duty;       // TIMER_CC_OC register value
} OutputClock_TypeDef;

// Initialzizer for 6 MHz output
#define OUTPUTCLOCK_6MHZ                                             \
{                                                                        \
  true,     /* clock is initially running */  \
  false,    /* clock off state is low     */  \
  6000000,  /* 6 MHz target frequency     */  \
  0,        /* set during initialization  */  \
  0,        /* set during initialization  */  \
}

// Track the output clock state in a global variable
OutputClock_TypeDef clockState = OUTPUTCLOCK_6MHZ;

/**************************************************************************//**
 * @brief
 *    CMU initialization
 *****************************************************************************/
void initClocks(void)
{
  // Initialize and enable the HFXO for the crystal on the BRD4182A radio board
  CMU_HFXOInit_TypeDef hfxoInit = CMU_HFXOINIT_DEFAULT;
  CMU_HFXOInit(&hfxoInit);
  CMU_OscillatorEnable(cmuOsc_HFXO, true, true);

  // Select the HFXO as the SYSCLK
  CMU_ClockSelectSet(cmuClock_SYSCLK, cmuSelect_HFXO);

  /*
   * Initialize the DPLL to run at 36 MHz from the 38.4 MHz HFXO
   * reference.  The HFXO is designed to provide an accurate reference
   * to the radio vs. the LFXO which is designed for low-power.
   *
   * fDPLL = fREF * (N + 1) / (M + 1), where N > 300
   *
   * In this case:
   *
   * fDPLL = 38,400,000 * (3599 + 1) / (3839 + 1) = 36,000,000 MHz
   */
  CMU_DPLLInit_TypeDef dpllInit = CMU_DPLLINIT_DEFAULT;
  dpllInit.frequency = 36000000;
  dpllInit.n = (3600 - 1);
  dpllInit.m = (3840 - 1);
  dpllInit.refClk = cmuSelect_HFXO;

  // Attempt DPLL lock; halt on failure
  if (CMU_DPLLLock(&dpllInit) == false)
    __BKPT(0);

  /*
   * Note that TIMERs, the EUART, and the IADC either run from the
   * EM01GRPACLK or default to it, and the EM01GRPACLK defaults to the
   * DPLL as its clock source.  This means that the clock for these
   * peripherals is now 36 MHz unless another source is selected for
   * those that allow it.
   *
   * Other peripherals, including the GPIO block, are clocked by the
   * peripheral bus clock (PCLK), which is derived from the SYSCLK and
   * run at 38.4 MHz in this example.
   */
  CMU_ClockEnable(cmuClock_TIMER0, true);
  CMU_ClockEnable(cmuClock_GPIO, true);
}

/**************************************************************************//**
 * @brief
 *    TIMER initialization
 *****************************************************************************/
void initTIMER0(OutputClock_TypeDef *init)
{
  uint32_t timerFreq;

  /*
   * Configure, but do not start TIMER0.  Do not prescale the TIMER
   * clock (stick with divide-by-1) in order to have the highest
   * resolution for the most accurate output.
   */
  TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
  timerInit.prescale = timerPrescale1;
  timerInit.enable = init->offstate;

  TIMER_Init(TIMER0, &timerInit);

  // Configure TIMER0 channel 0 for PWM, output initially low
  TIMER_InitCC_TypeDef cc0init = TIMER_INITCC_DEFAULT;
  cc0init.mode = timerCCModePWM;
  cc0init.coist = false;

  TIMER_InitCC(TIMER0, 0, &cc0init);

  // Output the clock on PA6 (expansion board pin 14)
  GPIO_PinModeSet(gpioPortA, 6, gpioModePushPull, 0);

  // Route TIMER0 CC0 output to PA6
  GPIO->TIMERROUTE[0].ROUTEEN  = GPIO_TIMER_ROUTEEN_CC0PEN;
  GPIO->TIMERROUTE[0].CC0ROUTE = (gpioPortA << _GPIO_TIMER_CC0ROUTE_PORT_SHIFT)
                    | (6 << _GPIO_TIMER_CC0ROUTE_PIN_SHIFT);

  /*
   * Set TIMER0 counter TOP value.  Output toggle occurs on overflow;
   * each overflow is 1/2 the signal period.  Calculate the TOP value
   * based on the frequency of the TIMER0 clock source and the desired
   * output frequency.
   *
   * Note that no bounds-checking is performed on the desired output
   * frequency.  There are limitations based on the maximum peripheral
   * clock frequency and the width of the timer used (32- vs. 16-bit).
   */
  timerFreq = CMU_ClockFreqGet(cmuClock_TIMER0);
  init->top = (timerFreq/ init->frequency) - 1;
  TIMER_TopSet (TIMER0, init->top);

  init->duty = (init->top + 1) / 2;
  TIMER_CompareSet(TIMER0, 0, init->duty);

  // Start the timer
  TIMER_Enable(TIMER0, true);
}

/**************************************************************************//**
 * @brief
 *    Setup GPIO interrupt for push button 0.
 *****************************************************************************/
void initPB0(void)
{
  // Use button 0 as input; configure for rising edge interrupt
  GPIO_PinModeSet(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, gpioModeInputPull, 1);

  GPIO_ExtIntConfig(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, BSP_GPIO_PB0_PIN, true, false, true);

  // Enable appropriate odd/even interrupt source depending on the pin number
#if (BSP_GPIO_PB0_PIN & 1)
  NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
  NVIC_EnableIRQ(GPIO_ODD_IRQn);
#else
  NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
  NVIC_EnableIRQ(GPIO_EVEN_IRQn);
#endif  // (BSP_GPIO_PB0_PIN & 1)
}

/**************************************************************************//**
 * @brief
 *    GPIO interrupt handler for push button 0.
 *****************************************************************************/
#if (BSP_GPIO_PB0_PIN & 1)
void GPIO_ODD_IRQHandler(void)
#else
void GPIO_EVEN_IRQHandler(void)
#endif  // (BSP_GPIO_PB0_PIN & 1)
{
  if (clockState.running)
  {
    // Entering EM2, so stop the clock in the off state
    clockState.running = false;
    TIMER_CompareBufSet(TIMER0, 0, 0);
  }
  else
  {
    // Exiting EM2, so restart the clock
    clockState.running = true;
    TIMER_CompareBufSet(TIMER0, 0, clockState.duty);
  }
  // Clear interrupt flag
  GPIO_IntClear(1 << BSP_GPIO_PB0_PIN);
}

/**************************************************************************//**
 * @brief  Main function
 *****************************************************************************/
int main(void)
{
  // Chip errata
  CHIP_Init();

  initClocks();

  // Keep debug active in EM2
  EMU->CTRL |= EMU_CTRL_EM2DBGEN;

  initPB0();

  initTIMER0(&clockState);

  EMU_EM23Init_TypeDef emu23init = EMU_EM23INIT_DEFAULT;
  EMU_EM23Init(&emu23init);

  while (1)
  {
    // Wait in EM1; clock output is enabled
    EMU_EnterEM1();

    // Wait in EM2; clock output is disabled
    EMU_EnterEM2(true);
  }
}
