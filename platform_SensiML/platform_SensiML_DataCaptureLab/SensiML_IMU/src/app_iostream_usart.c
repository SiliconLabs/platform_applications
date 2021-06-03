/***************************************************************************//**
 * @file
 * @brief iostream usart examples functions
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided \'as-is\', without any express or implied
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

#include <stdio.h>
#include <string.h>
#include "em_chip.h"
#include "sl_iostream.h"
#include "sl_iostream_init_instances.h"
#include "sl_iostream_handles.h"

#include "sl_sleeptimer.h"

#include "app_sensor_imu.h"
#include "app_led.h"

#include "ssi_comms.h"
extern volatile bool config_received;
extern sl_sleeptimer_timer_handle_t send_config_timer;

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

#ifndef BUFSIZE
#define BUFSIZE    80
#endif

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/

/* Input buffer */
static char buffer[BUFSIZE];

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * Initialize example.
 ******************************************************************************/
void app_iostream_usart_init(void)
{
  /* Prevent buffering of output/input.*/
#if !defined(__CROSSWORKS_ARM) && defined(__GNUC__)
  setvbuf(stdout, NULL, _IONBF, 0);   /*Set unbuffered mode for stdout (newlib)*/
  setvbuf(stdin, NULL, _IONBF, 0);   /*Set unbuffered mode for stdin (newlib)*/
#endif
}

/***************************************************************************//**
 * Example ticking function.
 ******************************************************************************/
void app_iostream_usart_process_action(void)
{
  int8_t c = 0;
  static uint8_t index = 0;

  /* Retrieve characters */
  c = getchar();
  if (c > 0) {
    if (c == '\r' || c == '\n') { // reset buffer
      buffer[index] = '\0';
      index = 0;
    } else {
      if (index < BUFSIZE - 1) {
        buffer[index] = c;
        index++;
      }
    }
  }

  // check if "connect" command was received
  if (c == 't') {
    buffer[index] = '\0';
    index = 0;
    if ((strcmp("connect", buffer) == 0) || (strcmp("cnnect", buffer) == 0)) {
      //initialize IMU and start measurement
      app_sensor_imu_init();

      app_sensor_imu_enable(true);

      sl_sleeptimer_stop_timer(&send_config_timer);

      config_received = true;

      // Turn off LED0 (red) to indicate open connection; data transfer
      app_config_led_control(OFF);
      // reset sequence number for this connection, for default channel
      ssi_seqnum_reset(SSI_CHANNEL_DEFAULT);
    }
    else if ((strcmp("disconnect", buffer) == 0)) {
      //initialize IMU and start measurement
      app_sensor_imu_enable(false);

      config_received = false;
      app_config_imu();
      // Turn on LED0 (red) to indicate disconnect
      app_config_led_control(ON);
    }
  }
}
