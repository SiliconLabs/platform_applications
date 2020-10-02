/**************************************************************************//**
 * @main.c
 * @brief This project uses a timer to periodically read the EMU temperature
 * sensor and transmit the temperature in degrees C over UART/VCOM on a WSTK.
 * @version 0.0.1
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

#include "stdio.h"
#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_timer.h"
#include "retargetserial.h"

#include "tempdrv.h"

/* Version tracking which gets printed over UART */
#define VERSION 1

// Desired frequency in Hz
// EMU temperature measurement is taken once every second.
#define OUT_FREQ 1

volatile uint32_t measureEMUTemp = false;

/**************************************************************************//**
 * @brief GPIO initialization
 *****************************************************************************/
void initGPIO(void)
{
  // Configure PD2 as output; toggles LED
  GPIO_PinModeSet(gpioPortD, 2, gpioModePushPull, 0);

  // VCOM enable
  GPIO_PinModeSet(gpioPortB, 4, gpioModePushPull, 1);
}

/**************************************************************************//**
 * @brief
 *    CMU initialization
 *****************************************************************************/
void initCmu(void)
{
  // Enable clock to GPIO and TIMER0
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(cmuClock_TIMER0, true);
}

/**************************************************************************//**
 * @brief TIMER initialization
 *****************************************************************************/
void initTIMER(void)
{
  uint32_t timerFreq = 0;

  // Initialize the timer
  TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;

  // Configure TIMER0 Compare/Capture for output compare
  TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;

  timerInit.prescale = timerPrescale1;
  timerInit.enable = false;
  timerCCInit.mode = timerCCModeCompare;
  timerCCInit.cmoa = timerOutputActionToggle; //timerOutputActionNone;

  // configure, but do not start timer
  TIMER_Init(TIMER0, &timerInit);

  // Route Timer0 CC0 output to PD2
  GPIO->TIMERROUTE[0].ROUTEEN  = GPIO_TIMER_ROUTEEN_CC0PEN;
  GPIO->TIMERROUTE[0].CC0ROUTE = (gpioPortD << _GPIO_TIMER_CC0ROUTE_PORT_SHIFT)
  								  | (2 << _GPIO_TIMER_CC0ROUTE_PIN_SHIFT);

  TIMER_InitCC(TIMER0, 0, &timerCCInit);

  // Set Top value
  timerFreq = CMU_ClockFreqGet(cmuClock_TIMER0)/(timerInit.prescale + 1);
  int topValue = timerFreq / OUT_FREQ - 1;
  TIMER_TopSet (TIMER0, topValue);

  // Enable TIMER0 interrupts
  TIMER_IntEnable(TIMER0, TIMER_IEN_CC0);
  NVIC_EnableIRQ(TIMER0_IRQn);

  /* Start the timer */
  TIMER_Enable(TIMER0, true);
}

/**************************************************************************//**
 * @brief
 *    TIMER0 handler
 *****************************************************************************/
void TIMER0_IRQHandler(void)
{
  // Acknowledge the interrupt
  uint32_t flags = TIMER_IntGet(TIMER0);
  TIMER_IntClear(TIMER0, flags);

  // Flag main to get EMU temperature and send value over UART
  measureEMUTemp = true;
}

/**************************************************************************//**
 * @brief  Main function
 *****************************************************************************/
int main(void)
{
  float EMUTempC = 0.0;
  int n, d;

  // Chip errata
  CHIP_Init();

  // Initializations
  initCmu();
  initGPIO();
  initTIMER();

  /* Initialize LEUART/USART and map LF to CRLF */
  RETARGET_SerialInit();
  RETARGET_SerialCrLf(1);

  printf("EMU_Temp example v0.0%u\n", VERSION);

  TEMPDRV_Init();

  while (1)
  {
    if (measureEMUTemp == true)
    {
      //measure EMU temperature
      EMUTempC = EMU_TemperatureGet();
      n = EMUTempC / 1.0;
      d = (EMUTempC - n) * 100;

      //output via serial UART
      printf("  EMU_Temp\t%d.%d\n", n, d);

	  //clear flag
	  measureEMUTemp = false;
	}
    EMU_EnterEM1();
  }
}
