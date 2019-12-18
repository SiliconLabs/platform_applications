/***************************************************************************//**
EM3 sleep with Micrium example
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement.
 * The software is governed by the sections of the MSLA applicable to Micrium
 * Software.
 *
 ******************************************************************************/


/* ...
*
* EXPERIMENTAL QUALITY
* This code has not been formally tested and is provided as-is.  It is not suitable for production environments.
* This code will not be maintained.
*
... */

/*
*********************************************************************************************************
*
*                                             EXAMPLE MAIN
*
* File : ex_main.c
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <bsp_os.h>
#include  "bsp.h"

#include  <cpu/include/cpu.h>
#include  <kernel/include/os.h>
#include  <kernel/include/os_trace.h>
#include  <common/include/common.h>
#include  <common/include/lib_def.h>
#include  <common/include/rtos_utils.h>
#include  <common/include/toolchains.h>
/**********************************USER CODE**********************************/
#include "em_cmu.h"
#include "em_cryotimer.h"
#include "em_emu.h"
/********************************END USER CODE********************************/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                             LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  EX_MAIN_START_TASK_PRIO              21u
#define  EX_MAIN_START_TASK_STK_SIZE         512u
/**********************************USER CODE**********************************/
#define  USER_IDLE_TASK_PRIO				 OS_CFG_PRIO_MAX-2
#define  USER_IDLE_TASK_STK_SIZE             128u
/********************************END USER CODE********************************/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                        LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

                                                                /* Start Task Stack.                                    */
static  CPU_STK  Ex_MainStartTaskStk[EX_MAIN_START_TASK_STK_SIZE];
                                                                /* Start Task TCB.                                      */
static  OS_TCB   Ex_MainStartTaskTCB;

/**********************************USER CODE**********************************/
static  CPU_STK  UserIdleStk[USER_IDLE_TASK_STK_SIZE];

static  OS_TCB   UserIdleTCB;

uint32_t         idleCount;
/********************************END USER CODE********************************/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  void  Ex_MainStartTask (void  *p_arg);
/**********************************USER CODE**********************************/
static  void  UserIdle  (void *p_arg);
/********************************END USER CODE********************************/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                                main()
*
* Description : This is the standard entry point for C applications. It is assumed that your code will
*               call main() once you have performed all necessary initialization.
*
* Argument(s) : None.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

int  main (void)
{
    RTOS_ERR  err;

    BSP_SystemInit();                                           /* Initialize System.                                   */
    CPU_Init();                                                 /* Initialize CPU.                                      */

    OS_TRACE_INIT();                                            /* Initialize trace if enabled                          */
    OSInit(&err);                                               /* Initialize the Kernel.                               */
                                                                /*   Check error code.                                  */
    APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);


    OSTaskCreate(&Ex_MainStartTaskTCB,                          /* Create the Start Task.                               */
                 "Ex Main Start Task",
                  Ex_MainStartTask,
                  DEF_NULL,
                  EX_MAIN_START_TASK_PRIO,
                 &Ex_MainStartTaskStk[0],
                 (EX_MAIN_START_TASK_STK_SIZE / 10u),
                  EX_MAIN_START_TASK_STK_SIZE,
                  0u,
                  0u,
                  DEF_NULL,
                 (OS_OPT_TASK_STK_CLR),
                 &err);
                                                                /*   Check error code.                                  */
    APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);



/**********************************USER CODE**********************************/
    OSTaskCreate(&UserIdleTCB,
    			 "User Implemented Idle",
				 UserIdle,
				 DEF_NULL,
				 USER_IDLE_TASK_PRIO,
				 &UserIdleStk[0],
				 (USER_IDLE_TASK_STK_SIZE / 10u),
				 USER_IDLE_TASK_STK_SIZE,
				 0u,
				 0u,
				 DEF_NULL,
				 (OS_OPT_TASK_STK_CLR | OS_OPT_TASK_NO_TLS),
				 &err);

    APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

/********************************END USER CODE********************************/

    OSStart(&err);                                              /* Start the kernel.                                    */
                                                                /*   Check error code.                                  */
    APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);


    return (1);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                          Ex_MainStartTask()
*
* Description : This is the task that will be called by the Startup when all services are initializes
*               successfully.
*
* Argument(s) : p_arg   Argument passed from task creation. Unused, in this case.
*
* Return(s)   : None.
*
* Notes       : None.
*********************************************************************************************************
*/

static  void  Ex_MainStartTask (void  *p_arg)
{
    RTOS_ERR  err;


    PP_UNUSED_PARAM(p_arg);                                     /* Prevent compiler warning.                            */


    BSP_TickInit();                                             /* Initialize Kernel tick source.                       */
    BSP_OS_Init();                                              /* Initialize the BSP. It is expected that the BSP ...  */
                                                                /* ... will register all the hardware controller to ... */
                                                                /* ... the platform manager at this moment.             */

#if (OS_CFG_STAT_TASK_EN == DEF_ENABLED)
    OSStatTaskCPUUsageInit(&err);                               /* Initialize CPU Usage.                                */
                                                                /*   Check error code.                                  */
    APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), ;);
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
    CPU_IntDisMeasMaxCurReset();                                /* Initialize interrupts disabled measurement.          */
#endif

    Common_Init(&err);                                          /* Call common module initialization example.           */
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);


    while (DEF_ON) {
        BSP_LedToggle(0);
                                                                /* Delay Start Task execution for                       */
        OSTimeDly( 1000,                                        /*   1000 OS Ticks                                      */
                   OS_OPT_TIME_DLY,                             /*   from now.                                          */
                  &err);
                                                                /*   Check error code.                                  */
        APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), ;);
    }
}



/*
*********************************************************************************************************
*                                          Ex_MainStartTask()
*
* Description : This is the task that will be called by the Startup when all services are initializes
*               successfully.
*
* Argument(s) : p_arg   Argument passed from task creation. Unused, in this case.
*
* Return(s)   : None.
*
* Notes       : None.
*********************************************************************************************************
*/

static void UserIdle (void *p_arg)
{
	RTOS_ERR  err;


    while(DEF_ON) {
        EMU_EnterEM3(true);
    }
}
/********************************END USER CODE********************************/
