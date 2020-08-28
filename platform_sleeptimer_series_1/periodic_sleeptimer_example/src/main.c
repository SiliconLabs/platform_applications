/***************************************************************************//**
 * @file main.c
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
#define PERIODIC_TIMEOUT 15000

// LED to turn on
#define	LED0_NUM	0


/**************************************************************************//**
 * @brief
 *   CMU setup function. Default clock used for Series 1 devices is RTCC.
 *****************************************************************************/
static void CMU_setup(void)
{
	// HFLE needed for low frequency peripherals
	CMU_ClockEnable(cmuClock_HFLE, true);
	CMU_ClockSelectSet(cmuClock_LFE, cmuSelect_LFXO);
	CMU_ClockEnable(cmuClock_RTCC, true);
}

/**************************************************************************//**
 * @brief
 *   Sleeptimer callback function. Each time sleeptimer reaches timeout value,
 *   this callback is executed.
 *****************************************************************************/
void sleeptimer_cb(sl_sleeptimer_timer_handle_t *handle, void *data)
{
	// Turn off LED0
	BSP_LedToggle(LED0_NUM);
}

/**************************************************************************//**
 * @brief
 *   Main Function.
 *****************************************************************************/
int main(void)
{
	// Sleeptimer return status
	sl_status_t status;

	// Chip errata
	CHIP_Init();

	CMU_setup();

	// Initialize the BSP LEDs; also enables GPIO clock
	BSP_LedsInit();

	// Initialize sleeptimer driver and setup underlying hardware timer
	status = sl_sleeptimer_init();

	// Stop if initialization error occurs
	if (status != SL_STATUS_OK)
		__BKPT(2);

	/**************************************************************************//**
	 *   Starts a periodic sleeptimer to a callback function. The arguments
	 *   passed are as follows: (sleeptimer handle, timeout value, callback
	 *   function, user data (in this case, null), priority value, option flag)
	 *   The priority value is useful in the case of multiple timers being used;
	 *   0 denotes highest priority. The option flag is a bit array of option flags
	 *   for the timer. A 0 option flag denotes no flags, and bit-wise OR denotes
	 *   SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG.
	 *****************************************************************************/
	status = sl_sleeptimer_start_periodic_timer(&my_sleeptimer_handle, PERIODIC_TIMEOUT, sleeptimer_cb, (void *)NULL,0,0);

	// Stop if a start error occurs
	if (status != SL_STATUS_OK)
		__BKPT(1);

	// Loops indefinitely
	while (1)
		EMU_EnterEM2(false);

	// Program should not reach this breakpoint
	__BKPT(0);
}
