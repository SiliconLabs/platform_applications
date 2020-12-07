/***************************************************************************//**
 * @main.c
 * @brief Test application for evaluation of EFM8-IOExpander (see AN1304)
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

#include "cli.h"
#include "ioexp_drv.h"
#include "gpio.h"

#include "hal_common.h"
#include "hal-config.h"
#include "rail.h"
#include "retargetserial.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

/***************************************************************************//**
 * Function prototypes
 ******************************************************************************/
void RAILCb_Generic(RAIL_Handle_t railHandle, RAIL_Events_t events);

void RAILCb_Generic(RAIL_Handle_t railHandle, RAIL_Events_t events)
{
  (void) railHandle;
  (void) events;
}

RAIL_Handle_t railHandle;

static RAIL_Config_t railCfg = {
  .eventsCallback = &RAILCb_Generic,
};

/**************************************************************************//**
 * @brief  Main function
 *****************************************************************************/
int main(void)
{
  /* Initialize HAL. Chip errata included */
  halInit();
  /* RAIL is only for timing purpose */
  railHandle = RAIL_Init(&railCfg, NULL);
  if (railHandle == NULL) {
    while (1) ;
  }
  /* Initialize I2C and check whether IOEXP board hang on it */
  ioexp_init();
  /* Initialize GPIO for sensing IOEXP interrupt signal */
  gpio_init();
  /* Initialize LEUART/USART and map LF to CRLF */
  RETARGET_SerialInit();
  RETARGET_SerialCrLf(1);
  /* Print application information and start command line interface */
  printf("IOExpander test interface\n");
  cli_process_init();

  while (1)
  {
    cli_process_input_char_task();
  }

  return 0;
}

