/***************************************************************************//**
 * @file
 * @brief Top level application functions
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
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
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "glib.h"
#include "dmd.h"
#include "gpiointerrupt.h"
#include "squash_config.h"
#include "app.h"
#if (LED_DEMO == 1)
#include "sl_simple_led_instances.h"
#endif

// -----------------------------------------------------------------------------
//                                Global Variables
// -----------------------------------------------------------------------------

/*  Greeting text  */
static const char *msg = "Hello SQUASH!";
#define GTEXT_L 14

/*  LCD context variable  */
static GLIB_Context_t g_context;

/*  Racket entity structure  */
static GLIB_Rectangle_t g_racket = { 10, 10, 13, 29 };

/*  Ball entity structure  */
static GLIB_Rectangle_t g_ball = { 50, 50, 51, 51 };

/*  Task creation returns  */
BaseType_t rtask;

/*  Handler declarations for each task  */
TaskHandle_t taskBlinkh = NULL;
TaskHandle_t taskRDownh = NULL;
TaskHandle_t taskRUph = NULL;
TaskHandle_t taskBallh = NULL;

/*  Semaphore declarations for button tasks  */
xSemaphoreHandle xBSemaphore;
xSemaphoreHandle xBSemaphoreUp;
xSemaphoreHandle xMutex;

/*  Save colors  */
uint32_t fg_color;
uint32_t bg_color;

#if (LED_DEMO == 1)
void vBlink(void *pvParameters);

#endif
void vBall(void *pvParameters);
void vRacket_up(void *pvParameters);
void vRacket_down(void *pvParameters);
static void app_gpio_button0_int_cb(uint8_t intNo);
static void app_gpio_button1_int_cb(uint8_t intNo);

void app_init(void)
{
  EMSTATUS gstatus = GLIB_OK;

  /*  Creating semaphores for button tasks  */
  vSemaphoreCreateBinary(xBSemaphore);
  vSemaphoreCreateBinary(xBSemaphoreUp);
  xMutex = xSemaphoreCreateMutex();

  /* Creating tasks */
  rtask = xTaskCreate(vRacket_down,
                      "Racket down task",
                      STACK_ROCKET_DOWN_SIZE,
                      NULL,
                      TASK_ROCKET_DOWN_PRIORITY,
                      &taskRDownh);
  if (rtask == pdTRUE) {
    rtask = xTaskCreate(vRacket_up,
                        "Racket up task",
                        STACK_UP_DOWN_SIZE,
                        NULL,
                        TASK_ROCKET_UP_PRIORITY,
                        &taskRUph);
  }
  if (rtask == pdTRUE) {
    rtask = xTaskCreate(vBall,
                        "Ball task",
                        STACK_BALL_SIZE, NULL,
                        TASK_BALL_PRIORITY,
                        &taskBallh);
  }
#if (LED_DEMO == 1)
  if (rtask == pdTRUE) {
    rtask = xTaskCreate(vBlink,
                        "Blink task",
                        STACK_BLINK_SIZE,
                        NULL,
                        TASK_BLINK_PRIORITY,
                        &taskBlinkh);
  }
#endif

  /*  Halt program if any of the task and Mutex creation functions failed  */
  if ((rtask == pdFAIL) || (xMutex == NULL)) {
    while (1) {
      /* Error here */
    }
  }

  /*  Initializing Dot Matrix Display and outputting greeting text  */
  DMD_init(0);
  gstatus = GLIB_contextInit(&g_context);

  if (GLIB_OK != gstatus) {
    while (1) {
      /* Error here */
    }
  }

  GLIB_drawString(&g_context, msg, GTEXT_L, GTEXT_X, GTEXT_Y, true);
  GLIB_drawRectFilled(&g_context, &g_racket);
  DMD_updateDisplay();

  fg_color = g_context.foregroundColor;
  bg_color = g_context.backgroundColor;

  /*  Initializing button Ports and PINs  */
  /* GPIO Button 0*/
  GPIO_PinModeSet(BUTTON_0_PORT,
                  BUTTON_0_PIN,
                  gpioModeInput,
                  0);

  GPIO_ExtIntConfig(BUTTON_0_PORT,
                    BUTTON_0_PIN,
                    BUTTON_0_INT,
                    0,
                    true,
                    true);

  GPIOINT_CallbackRegister(BUTTON_0_PIN, app_gpio_button0_int_cb);
  GPIO_IntEnable(BUTTON_0_PIN);

  /* GPIO Button 1*/
  GPIO_PinModeSet(BUTTON_1_PORT,
                  BUTTON_1_PIN,
                  gpioModeInput,
                  0);

  GPIO_ExtIntConfig(BUTTON_1_PORT,
                    BUTTON_1_PIN,
                    BUTTON_1_INT,
                    0,
                    true,
                    true);

  GPIOINT_CallbackRegister(BUTTON_1_PIN, app_gpio_button1_int_cb);
  GPIO_IntEnable(BUTTON_1_PIN);

  /*  Setting GPIO ( button )interrupt levels  */

  NVIC_SetPriority(BUTTON0_INT_NUM, BUTTON0_INT_PRIO);
  NVIC_SetPriority(BUTTON1_INT_NUM, BUTTON1_INT_PRIO);
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void
app_process_action(void)
{
  /* App ticking function */
}

#if (LED_DEMO == 1)

/*****************************************************************************
 * @brief
 *   Blink task toggles LED0 in a configurable way.
 *
 * @param[in] Not used
 *
 ******************************************************************************/
void vBlink(void *pvParameters)
{
  (void)&pvParameters;

  while (1) {
    sl_led_toggle(&sl_led_led0);
    vTaskDelay(BLINK_TASK_DLY);
  }
}

#endif

/*****************************************************************************
 * @brief
 *   Task execution is triggered by BUTTON0 press and move down the racket.
 *
 * @param[in] Not used
 *
 ******************************************************************************/
void vRacket_down(void *pvParameters)
{
  (void)&pvParameters;

  xSemaphoreTake(xMutex, portMAX_DELAY);
  GLIB_drawRectFilled(&g_context, &g_racket);
  DMD_updateDisplay();
  xSemaphoreGive(xMutex);

  while (1) {
    xSemaphoreTake(xBSemaphore, portMAX_DELAY);

    if ((g_racket.yMax + MOV_STEP_Y) < LCD_HEIGHT) {
      xSemaphoreTake(xMutex, portMAX_DELAY);
      g_context.foregroundColor = bg_color;

      /*  Remove Racket from the old position  */
      GLIB_drawRectFilled(&g_context, &g_racket);
      g_racket.yMin += MOV_STEP_Y;
      g_racket.yMax += MOV_STEP_Y;
      g_context.foregroundColor = fg_color;
      GLIB_drawRectFilled(&g_context, &g_racket);
      DMD_updateDisplay();
      xSemaphoreGive(xMutex);
    }
  }
}

/*****************************************************************************
 * @brief
 *   Task execution is triggered by BUTTON1 press and move up the racket.
 *
 * @param[in] Not used
 *
 ******************************************************************************/
void vRacket_up(void *pvParameters)
{
  (void)&pvParameters;

  while (1) {
    xSemaphoreTake(xBSemaphoreUp, portMAX_DELAY);

    if (g_racket.yMin - MOV_STEP_Y >= 0) {
      xSemaphoreTake(xMutex, portMAX_DELAY);
      g_context.foregroundColor = bg_color;

      /*  Remove Racket from the old position  */
      GLIB_drawRectFilled(&g_context, &g_racket);
      g_racket.yMin -= MOV_STEP_Y;
      g_racket.yMax -= MOV_STEP_Y;
      g_context.foregroundColor = fg_color;
      GLIB_drawRectFilled(&g_context, &g_racket);
      DMD_updateDisplay();
      xSemaphoreGive(xMutex);
    }
  }
}

/*****************************************************************************
 * @brief
 *   Task moves the ball on the screen, delay is used to allow other tasks
 *   to run and make ball visibility smooth.
 *
 * @param[in] Not used
 *
 ******************************************************************************/
void vBall(void *pvParameters)
{
  (void)&pvParameters;

  /*  Racket right edge contacting the Ball  */
  int32_t lbarrier = g_racket.xMax + 1;
  int8_t movballx = MOV_BALL_X;
  int8_t movbally = MOV_BALL_Y;

  /*  Initial Ball position  */
  xSemaphoreTake(xMutex, portMAX_DELAY);
  GLIB_drawRectFilled(&g_context, &g_ball);
  DMD_updateDisplay();
  xSemaphoreGive(xMutex);

  while (1) {
    vTaskDelay(50);

    xSemaphoreTake(xMutex, portMAX_DELAY);

    /*  Removing Ball in previous position   */
    g_context.foregroundColor = bg_color;
    GLIB_drawRectFilled(&g_context, &g_ball);
    DMD_updateDisplay();
    g_context.foregroundColor = fg_color;
    xSemaphoreGive(xMutex);

    /*  Check whether Ball's right edge exceeds the end of the screen */
    if ((g_ball.xMax) >= (LCD_WIDTH - 1)) {
      movballx *= -1;

      /*  Check whether the ball touches the Racket  */
    } else if ((g_ball.xMin <= lbarrier) && ((g_racket.yMin <= g_ball.yMin)
                                             && (g_racket.yMax
                                                 >= g_ball.yMax))) {
      movballx *= -1;

      /*  If Ball misses the Racket  start from position (50,50) */
    } else if (g_ball.xMin < lbarrier) {
      g_ball.xMin = 50;
      g_ball.xMax = 51;
      g_ball.yMin = 50;
      g_ball.yMax = 51;
    }

    /*  Check whether Ball exceeds the top or the bottom of the screen  */
    if ((g_ball.yMax >= (LCD_HEIGHT - 1)) || (g_ball.yMin <= 0)) {
      movbally *= -1;
    }

    /*  Refresh Ball coordinates */
    g_ball.xMin += movballx;
    g_ball.xMax += movballx;
    g_ball.yMin += movbally;
    g_ball.yMax += movbally;

    xSemaphoreTake(xMutex, portMAX_DELAY);
    GLIB_drawRectFilled(&g_context, &g_ball);
    DMD_updateDisplay();
    xSemaphoreGive(xMutex);
  }
}

static void app_gpio_button0_int_cb(uint8_t intNo)
{
  (void) intNo;

  portBASE_TYPE xHigherPTWoken = 0;
  xSemaphoreGiveFromISR(xBSemaphore, &xHigherPTWoken);
  portYIELD_FROM_ISR(xHigherPTWoken);
}

static void app_gpio_button1_int_cb(uint8_t intNo)
{
  (void) intNo;

  portBASE_TYPE xHigherPTWoken = 0;
  xSemaphoreGiveFromISR(xBSemaphoreUp, &xHigherPTWoken);
  portYIELD_FROM_ISR(xHigherPTWoken);
}
