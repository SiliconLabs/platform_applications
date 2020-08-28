/***************************************************************************//**
 * @file bsp_tick_rtcc.c
 * @brief BSP Dynamic Tick using cryotimer
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc.  Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement.  This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

/********************************************************************************************************
 ********************************************************************************************************
 *                                            INCLUDE FILES
 ********************************************************************************************************
 *******************************************************************************************************/

/* ...
*
* EXPERIMENTAL QUALITY
* This code has not been formally tested and is provided as-is.  It is not suitable for production environments.
* This code will not be maintained.
*
... */


#include "../include/bsp_tick_rtcc.h"
#include <kernel/include/os.h>
#include <common/include/rtos_utils.h>
#include <common/include/toolchains.h>

#if (OS_CFG_DYN_TICK_EN == DEF_ENABLED)

#include "em_cmu.h"
#include "em_cryotimer.h"
#include "em_emu.h"


/********************************************************************************************************
 ********************************************************************************************************
 *                                            LOCAL DEFINES
 ********************************************************************************************************
 *******************************************************************************************************/

#define  CRYO_OSCILLATOR_FREQ                     (1024u)
#define  CRYO_PRESCALER                           (1u)
#define  BSP_OS_CRYO_TICK_RATE_HZ                 (CRYO_OSCILLATOR_FREQ / CRYO_PRESCALER)
#define  BSP_OS_CRYOTICK_TO_OSTICK(rtcctick)      (((rtcctick) * OSCfg_TickRate_Hz) / BSP_OS_RTCC_TICK_RATE_HZ)
#define  BSP_OS_OSTICK_TO_CRYOTICK(ostick)        (((ostick) * BSP_OS_CRYO_TICK_RATE_HZ) / OSCfg_TickRate_Hz)


/********************************************************************************************************
 ********************************************************************************************************
 *                                       LOCAL GLOBAL VARIABLES
 ********************************************************************************************************
 *******************************************************************************************************/

// The number of OS ticks that have passed at the last time
// we updated the OS time. This is stored in OS Tick units
static OS_TICK BSP_OS_LastTick;



/********************************************************************************************************
 ********************************************************************************************************
 *                                      LOCAL FUNCTION PROTOTYPES
 ********************************************************************************************************
 *******************************************************************************************************/

static void CRYO_UpdateTicks(uint32_t tickElapsed);



/********************************************************************************************************
 ********************************************************************************************************
 *                                          GLOBAL FUNCTIONS
 ********************************************************************************************************
 *******************************************************************************************************/

/****************************************************************************************************//**
 *                                          BSP_RTCC_TickInit()
 *
 * @brief    Initialize the RTCC module to enable dynamic ticking.
 *
 * @note     (1) This function is called at the start of the Application Start Task, and so other
 *               tasks with more priority could be executed before this Initialization. Therefore, this
 *               function can be use for a lazy Initialization by being called when a RTC timer is needed.
 *******************************************************************************************************/
void BSP_RTCC_TickInit(void)
{
    EMU_EM23Init_TypeDef em23Init;
    CRYOTIMER_Init_TypeDef cryoInit;


    CMU_ClockEnable(cmuClock_CRYOTIMER, true);                  // Initialize the cryotimer to wake the device from EM3.

    cryoInit.enable    = false;                                 // Initialize the cryo timer
    cryoInit.debugRun  = false;
    cryoInit.em4Wakeup = false;
    cryoInit.osc       = cryotimerOscULFRCO;
    cryoInit.presc     = cryotimerPresc_1;

    CRYOTIMER_Init(&cryoInit);
    CRYOTIMER_IntEnable(CRYOTIMER_IEN_PERIOD);                  // Enable cryotimer interrupts
    NVIC_EnableIRQ(CRYOTIMER_IRQn);

    em23Init.em23VregFullEn    = false;
    em23Init.vScaleEM23Voltage = emuVScaleEM23_FastWakeup;
    EMU_EM23Init(&em23Init);
}


/****************************************************************************************************//**
 *                                           BSP_OS_TickGet()
 *
 * @brief    Get the OS Tick Counter as if it was running continuously.
 *
 * @return   The effective OS Tick Count.
 *******************************************************************************************************/
OS_TICK BSP_OS_TickGet(void)
{
    uint32_t tickElapsed;

    tickElapsed = BSP_OS_LastTick + CRYOTIMER_CounterGet();

    return (tickElapsed);
}

/****************************************************************************************************//**
 *                                         BSP_OS_TickNextSet()
 *
 * @brief    Set the number of OS Ticks to wait before calling OSTimeDynTick.
 *
 * @param    ticks   Number of OS Ticks to wait.
 *
 * @return   Number of effective OS Ticks until next OSTimeDynTick.
 *******************************************************************************************************/
OS_TICK BSP_OS_TickNextSet(OS_TICK ticks)
{
    OS_TICK tick_rate = 0;
    OS_TICK tick_shift;
    OS_TICK period = 0;
    CPU_INT32U counter;
    CPU_SR_ALLOC();


    if ((ticks != (OS_TICK) -1) && (ticks != 0)) {

        CRYOTIMER_IntDisable(CRYOTIMER_IEN_PERIOD);             // Disable the period interrupt

        CPU_CRITICAL_ENTER();                                   // Do not count pending ticks twice
        ticks -= OSTickCtrPend;
        CPU_CRITICAL_EXIT();

        counter = CRYOTIMER_CounterGet();                       // Check if there are any ticks unaccounted for

        if(counter > 0u) {                                      // If there are ticks unaccoutned for, update the tick counter
            CPU_CRITICAL_ENTER();
            CRYO_UpdateTicks(counter);                          // Update the OS with actual time passed based on the RTCC.
            CPU_CRITICAL_EXIT();
        }

        CRYOTIMER_Enable(false);                                // Clear the Cryo Timer config

        for(tick_rate = ticks; tick_rate >= 1; tick_rate--)     // Since we can only interrupt on powers of 2, find the
        {                                                       // next closest power of 2 to delay for.
            if ((tick_rate & (tick_rate - 1)) == 0)
            {
                tick_shift = tick_rate;
                break;
            }
        }

        while(tick_shift) {                                     // Count the bits, off by one to match the enum
            tick_shift >>= 1;
            if(!tick_shift) {
                break;
            }
            period++;
        }

        CRYOTIMER_PeriodSet(period);                            // Set the new period for the cryo timer
        CRYOTIMER_IntEnable(CRYOTIMER_IEN_PERIOD);              // Re-enable the period interrupt
        CRYOTIMER_Enable(true);                                 // Clear the Cryo Timer config
    }

    return tick_rate;
}



/********************************************************************************************************
 ********************************************************************************************************
 *                                           LOCAL FUNCTIONS
 ********************************************************************************************************
 *******************************************************************************************************/


/*********************************************************************************************************
 *                                          CRYO_UpdateTicks()
 *
 * @brief   Updates the number of pending OS Ticks and the internal time reference.
 *
 ********************************************************************************************************/

static void CRYO_UpdateTicks(uint32_t tickElapsed)
{
    if (tickElapsed > 0u) {
        BSP_OS_LastTick += tickElapsed;                             // Increment last tick
        OSTimeDynTick(tickElapsed);                                 // Signal the number of ticks elapsed
    }
}


/****************************************************************************************************//**
 *                                          CRYOTIMER_IRQHandler()
 *
 * @brief    Callback for cryo timer.
 *
 *******************************************************************************************************/

void  CRYOTIMER_IRQHandler (void)
{
    uint32_t period;
    uint32_t tickElapsed;
    uint32_t flags;


    flags = CRYOTIMER_IntGet();                                 // Acknowledge the interrupt
    CRYOTIMER_IntClear(flags);

    __DSB();                                                    // Put a barrier here to ensure interrupts are not retriggered.

    period = CRYOTIMER_PeriodGet();                             // Get the period for the interrupt
    tickElapsed = (1 << period);

    CRYOTIMER_Enable(false);                                    // Disable the cryo timer until the next tick set

    CRYO_UpdateTicks(tickElapsed);                              // Update dynamic tick count

}

#endif // (OS_CFG_DYN_TICK_EN == DEF_ENABLED)
