/***************************************************************************//**
 * @file main_pcnt_prs_input.c
 *
 * @brief This example demonstrates the single input externally clocked
 * mode but with the input coming via the PRS such that each GPIO
 * rising or falling edge (provided by button 0) generates an event.
 * In this way, the PCNT counts edges instead of pulses.  An interrupt
 * is requested whenever the count increases above the threshold
 * specified by PCNT_EDGE_COUNT and LED0 is toggled.
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

#include "em_chip.h"
#include "em_cmu.h"
#include "em_device.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_pcnt.h"
#include "em_prs.h"

#include "bsp.h"

#include <stdint.h>
#include <stdbool.h>

// Number of pulses (edges) to count before overflowing
#define PCNT_EDGE_COUNT   5

// PRS channel to route the input events to the PCNT
#define PCNT_PRS_CH       4

/***************************************************************************//**
 * @brief PCNT0 interrupt handler
 *        This function acknowledges the interrupt and toggles LED0
 ******************************************************************************/
void PCNT0_IRQHandler(void)
{
  // Clear overflow interrupt
  PCNT_IntClear(PCNT0, PCNT_IFC_OF);

  // Toggle LED0
  GPIO_PinOutToggle(BSP_GPIO_LED0_PORT, BSP_GPIO_LED0_PIN);
}

/***************************************************************************//**
 * @brief PCNT0 setup
 *        This function sets up PCNT0 with oversampling single mode
 *        Top value default to 5
 *        Event triggering when counting up
 *******************************************************************************/
static void initPCNT(void)
{
  PCNT_Init_TypeDef pcntInit = PCNT_INIT_DEFAULT;

  CMU_ClockEnable(cmuClock_PCNT0, true);

  /*
   * The externally clocked single input mode is used here.  The PCNT
   * glitch filter cannot be used in this mode, so it is necessary to
   * rely upon the GPIO analog glitch filter instead.
   *
   * It is not possible to use the single input oversampling mode to do
   * this because the maximum PCNT input frequency is limited to 8 kHz
   * in this case, and the PRS pulses are too short to be detected.
   *
   * In addition to selecting the specified PRS channel as its input,
   * the PCNT_S1IN is ignored such that counting is always in the up
   * direction.
   *
   * IMPORTANT NOTE: Because S0IN clocks the PCNT in externally clocked
   * mode the first three pulses are effectively ignored because they
   * serve to synchronize the registers with the HFCLKLE domain.  The
   * counter will not update until the fourth pulse has been received.
   */
  pcntInit.mode     = pcntModeExtSingle;
  pcntInit.top      = PCNT_EDGE_COUNT;
  pcntInit.s1CntDir = false;
  pcntInit.s0PRS    = PCNT_PRS_CH;
  pcntInit.filter   = false;

  PCNT_Init(PCNT0, &pcntInit);

  // Enable the PCNT0_S0IN PRS input
  PCNT_PRSInputEnable(PCNT0, pcntPRSInputS0, true);

  // Clear pending interrupts and enable overflow interrupt
  PCNT_IntClear(PCNT0, _PCNT_IF_MASK);
  PCNT_IntEnable(PCNT0, PCNT_IEN_OF);

  // Clear NVIC sources and enable
  NVIC_ClearPendingIRQ(PCNT0_IRQn);
  NVIC_EnableIRQ(PCNT0_IRQn);
}

static void initPRS(void)
{
  uint32_t source, signal;

  CMU_ClockEnable(cmuClock_PRS, true);

  // Select the PRS source/signal depending on the button 0 port/pin
  if (BSP_GPIO_PB0_PIN >= 8)
  {
    source = PRS_CH_CTRL_SOURCESEL_GPIOH;
    signal = (uint32_t)(BSP_GPIO_PB0_PIN - 8);
  }
  else
  {
    source = PRS_CH_CTRL_SOURCESEL_GPIOL;
    signal = BSP_GPIO_PB0_PIN;
  }

  // Select GPIO as PRS source and signal for both falling edges
  PRS_SourceSignalSet(PCNT_PRS_CH, source, signal, prsEdgeBoth);

  // Enable pulse stretching between domains
  PRS->CH[PCNT_PRS_CH].CTRL |= PRS_CH_CTRL_STRETCH;
}

static void initGPIO(void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Enable LED0 pin as an output initially off
  GPIO_PinModeSet(BSP_GPIO_LED0_PORT, BSP_GPIO_LED0_PIN, gpioModePushPull, 0);

  // Enable button 0 pin as an input with pull-up and filter
  GPIO_PinModeSet(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, gpioModeInputPullFilter, 1);

  /*
   * Configure rising/falling GPIO edge detection on button 0 pin.
   * Note that this call to GPIO_ExtIntConfig() almost looks the same
   * as enabling a GPIO interrupt on the given pin, except that this
   * case only the edge detection logic is enabled, not the interrupt
   * itself.  This allows rising and falling edges on the pin to
   * produce PRS events.
   */
  GPIO_ExtIntConfig(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, BSP_GPIO_PB0_PIN, true, true, false);
}

int main(void)
{
  uint32_t i;

  EMU_DCDCInit_TypeDef dcdcInit = EMU_DCDCINIT_DEFAULT;

  CHIP_Init();

  // Initialize DCDC with kit specific parameters
  EMU_DCDCInit(&dcdcInit);

  // Initialize and switch to the HFXO for the HFCLK
  CMU_HFXOInit_TypeDef hfxoInit = CMU_HFXOINIT_DEFAULT;
  CMU_HFXOInit(&hfxoInit);
  CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);

  // Enable the HFCLKLE for LF peripheral bus access
  CMU_ClockEnable(cmuClock_HFLE, true);

  /*
   * Use the LFRCO as the LFACLK.  Even though the PCNT is configured
   * to operate in externally clocked single input mode, it is still
   * necessary for the LFACLK to be set to some on-chip source in order
   * for register writes to propagate through to the module logic.
   */
  CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFRCO);

  initGPIO();

  initPRS();

  initPCNT();

  /*
   * As noted above, when the PCNT operates in externally clocked
   * mode, the first three pulse it receives are effectively ignored
   * because they are needed to synchronize the PCNT registers with the
   * HFCLKLE domain.
   *
   * To get around this such that each button push is recognized, the
   * PRS software pulse triggering mechanism can be used to generate
   * those first three pulses.  This allows the first button press (and
   * the two edge events it creates) to be counted.
   */
  for (i = 0; i < 3; i++)
    PRS_PulseTrigger(1 << PCNT_PRS_CH);

  while (true)
    EMU_EnterEM1();
}
