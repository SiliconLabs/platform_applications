/***************************************************************************//**
 * @file xg22_main.c
 * @brief SLEEPTIMER configuration. 
 * @version 1.0.1
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

// emlib
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"

// Service
#include "sl_sleeptimer.h"

// BSP
#include "bsp.h"

// Handle for sleeptimer
sl_sleeptimer_timer_handle_t my_sleeptimer_handle;

// Timeout value in ticks of chosen LF clock source
#define	ONESHOT_TIMEOUT	15000

// LED to turn on
#define	LEDNUM	0

/**************************************************************************//**
 * @brief
 *   GPIO setup function. Enables interrupts for
 *   even or odd pins for PB0 across different devices.
 *****************************************************************************/
static void GPIO_setup(void)
{
	// Configure the PB0 pin as an input with pull-up enabled
	GPIO_PinModeSet(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, gpioModeInputPullFilter, 1);

	// Enable even or odd IRQs based on which pin PB0 is on
#if (BSP_GPIO_PB0_PIN & 1)
	NVIC_EnableIRQ(GPIO_ODD_IRQn);
#else
	NVIC_EnableIRQ(GPIO_EVEN_IRQn);
#endif // (BSP_GPIO_PB0_PIN & 1)

	// Enable GPIO rising edge interrupts from PB0
	GPIO_ExtIntConfig(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, BSP_GPIO_PB0_PIN, true, false, true);
}

/**************************************************************************//**
 * @brief
 *   CMU setup function. Default clock used for Series 1 devices is RTCC.
 *****************************************************************************/
static void CMU_setup(void)
{
	// Initialize LFXO with specific parameters
	CMU_LFXOInit_TypeDef lfxoInit = CMU_LFXOINIT_DEFAULT;
	CMU_LFXOInit(&lfxoInit);

	// Setting RTCC clock source
	CMU_ClockSelectSet(cmuClock_RTCCCLK, cmuSelect_LFXO);

	// Enable RTCC bus clock
	CMU_ClockEnable(cmuClock_RTCC, true);
}

/**************************************************************************//**
 * @brief
 *   Sleeptimer callback function. Each time sleeptimer reaches timeout value,
 *   this callback is executed. In this case, LED0 turns off.
 *****************************************************************************/
void sleeptimer_cb(sl_sleeptimer_timer_handle_t *handle, void *data)
{
	// Turn off LED0
	BSP_LedClear(LEDNUM);
}

/**************************************************************************//**
 * @brief
 *   GPIO Interrupt Handler. This handles interrupt requests received on
 *   GPIO pins. The logic at the beginning helps determine if the interrupt will
 *   be received on an even or odd pin, which will determine if the even or odd
 *   IRQ handler will be used. Once inside the interrupt, the LED will turn on,
 *   the interrupt flag is cleared, and a one-shot sleeptimer will be started.
 *****************************************************************************/
#if (BSP_GPIO_PB0_PIN & 1)
void  GPIO_ODD_IRQHandler(void)
#else
void  GPIO_EVEN_IRQHandler(void)
#endif // (BSP_GPIO_PB0_PIN & 1)
{
	// Sleeptimer return status
	sl_status_t status;

	// Turn on LED0
	BSP_LedSet(LEDNUM);

	// Clear interrupt flag
	GPIO_IntClear(1 << BSP_GPIO_PB0_PIN);

	/**************************************************************************//**
	 *   Starts a one shot sleeptimer to a callback function. The arguments
	 *   passed are as follows: (sleeptimer handle, timeout value, callback
	 *   function, user data (in this case, null), priority value, option flags)
	 *   The priority value is useful in the case of multiple timers being used;
	 *   0 denotes highest priority. The option flag is a bit array of option flags
	 *   for the timer. A 0 option flag denotes no flags, and bit-wise OR denotes
	 *   SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG.
	 *****************************************************************************/
	status = sl_sleeptimer_start_timer(&my_sleeptimer_handle, ONESHOT_TIMEOUT, sleeptimer_cb,(void *)NULL, 0, 0);

	// Stop if a start error occurs
	if (status != SL_STATUS_OK)
		__BKPT(1);
}

int main(void)
{
	// Sleeptimer return status
	sl_status_t status;

	// Chip errata
	CHIP_Init();

	CMU_setup();

	// Initialize the BSP LEDs; also enables GPIO clock
	BSP_LedsInit();

	GPIO_setup();

	// Initialize sleeptimer driver and setup underlying hardware timer
	status = sl_sleeptimer_init();

	// Stop if initialization error occurs
	if (status != SL_STATUS_OK)
		__BKPT(3);

	// Loops indefinitely
	while (1)
		EMU_EnterEM1();

	// Program should not reach this breakpoint
	__BKPT(0);
}
