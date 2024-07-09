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
#include "em_emu.h"
#include "em_gpio.h"
#include "em_timer.h"

#ifndef GPIO_PB0_PORT             
#define GPIO_PB0_PORT              gpioPortB
#endif
#ifndef GPIO_PB0_PIN              
#define GPIO_PB0_PIN               0
#endif

// Clock state
typedef struct {
  bool      running;    // Is the clock currently running
  bool      offstate;   // Is the clock low or high when stopped
  uint32_t  frequency;  // Desired output frequency
  uint32_t  top;        // TIMER_TOP register value
  uint32_t  duty;       // TIMER_CC_OC register value
} OutputClock_TypeDef;

// Track the output clock state in a global variable
// Initializer for 6 MHz output
OutputClock_TypeDef clockState = {             \
  true,       /* clock is initially running */ \
  false,      /* clock off state is low     */ \
  6000000,    /* 6 MHz target frequency     */ \
  0,          /* set during initialization  */ \
  0,          /* set during initialization  */ \
};

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
  if (CMU_DPLLLock(&dpllInit) == false) {
    __BKPT(0);
  }

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
  GPIO->TIMERROUTE[0].ROUTEEN = GPIO_TIMER_ROUTEEN_CC0PEN;
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
  init->top = (timerFreq / init->frequency) - 1;
  TIMER_TopSet(TIMER0, init->top);

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
  GPIO_PinModeSet(GPIO_PB0_PORT,
                  GPIO_PB0_PIN,
                  gpioModeInputPull,
                  1);

  GPIO_ExtIntConfig(GPIO_PB0_PORT,
                    GPIO_PB0_PIN,
                    GPIO_PB0_PIN,
                    true,
                    false,
                    true);

  // Enable appropriate odd/even interrupt source depending on the pin number
#if (GPIO_PB0_PIN & 1)
  NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
  NVIC_EnableIRQ(GPIO_ODD_IRQn);
#else
  NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
  NVIC_EnableIRQ(GPIO_EVEN_IRQn);
#endif // (GPIO_PB0_PIN & 1)
}

/**************************************************************************//**
 * @brief
 *    GPIO interrupt handler for push button 0.
 *****************************************************************************/
#if (GPIO_PB0_PIN & 1)
void GPIO_ODD_IRQHandler(void)
#else
void GPIO_EVEN_IRQHandler(void)
#endif // (GPIO_PB0_PIN & 1)
{
  if (clockState.running) {
    // Entering EM2, so stop the clock in the off state
    clockState.running = false;
    TIMER_CompareBufSet(TIMER0, 0, 0);
  } else {
    // Exiting EM2, so restart the clock
    clockState.running = true;
    TIMER_CompareBufSet(TIMER0, 0, clockState.duty);
  }
  // Clear interrupt flag
  GPIO_IntClear(1 << GPIO_PB0_PIN);
}

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  initClocks();

  // Keep debug active in EM2
  EMU->CTRL |= EMU_CTRL_EM2DBGEN;

  initPB0();

  initTIMER0(&clockState);

  EMU_EM23Init_TypeDef emu23init = EMU_EM23INIT_DEFAULT;
  EMU_EM23Init(&emu23init);
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
  // Wait in EM1; clock output is enabled
  EMU_EnterEM1();

  // Wait in EM2; clock output is disabled
  EMU_EnterEM2(true);
}
