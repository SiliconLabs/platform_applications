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
	//CMU setup
	CMU_setup();

	// Initialize the BSP LEDs; also enables GPIO clock
	BSP_LedsInit();

	// Sleeptimer return status
	sl_status_t status;

	//Initialize sleep timer
	status = sl_sleeptimer_init();
	//Check for initialization error
	if(status != SL_STATUS_OK) {
		__BKPT(1);
	}

	status = sl_sleeptimer_start_periodic_timer(&my_sleeptimer_handle, PERIODIC_TIMEOUT, sleeptimer_cb, (void *)NULL,0,0);

	// Stop if a start error occurs
	if (status != SL_STATUS_OK)
		__BKPT(1);

	// Loops indefinitely
	while (1)
		EMU_EnterEM1();

		// Program should not reach this breakpoint
		__BKPT(0);
}
