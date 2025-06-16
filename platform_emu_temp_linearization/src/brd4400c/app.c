/***************************************************************************//**
 * @file
 * @brief EMU Temperature Sensor Linearization
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
#include <stdio.h>
#include <string.h>

#include "em_device.h"
#include "em_chip.h"
#include "em_emu.h"

#include "app.h"

#include "sl_iostream.h"
#include "sl_iostream_init_instances.h"
#include "sl_iostream_handles.h"

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/

static volatile uint32_t measure_EMU_temp = false;

static float x3_term_3rd_order = -1.613e-6;
static float x2_term_3rd_order = 2.001e-5;
static float x1_term_3rd_order = 1.012;
static float x0_term_3rd_order = -2.894;

static float x2_term_2nd_order = -2.037e-4;
static float x1_term_2nd_order = 1.014;
static float x0_term_2nd_order = -2.683;

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * Initialize example.
 ******************************************************************************/
void app_init(void)
{
  // Prevent buffering of output/input.
#if !defined(__CROSSWORKS_ARM) && defined(__GNUC__)
  setvbuf(stdout, NULL, _IONBF, 0);   /*Set unbuffered mode for stdout
                                       *   (newlib)*/
  setvbuf(stdin, NULL, _IONBF, 0);   /*Set unbuffered mode for stdin (newlib)*/
#endif

  // Setting default stream
  sl_iostream_set_default(sl_iostream_vcom_handle);

  // Using printf to print to UART VCOM instance
  printf("\nEMU_Temp linearization example \n\n");

  // Initialize the EMU temperature sensor. Enable interrupts when temperature
  // measurement completes (every 250ms according to RM)

  EMU_IntClear(_EMU_IF_MASK);
  EMU_IntDisable(_EMU_IF_MASK);
  NVIC_ClearPendingIRQ(EMU_IRQn);
  NVIC_EnableIRQ(EMU_IRQn);
  EMU_IntEnable(EMU_IEN_TEMP);
}

/***************************************************************************//**
 * Example ticking function.
 ******************************************************************************/
void app_process_action(void)
{
  float EMU_temp_raw = 0.0;
  float EMU_temp_2nd_order_poly = 0.0;
  float EMU_temp_3rd_order_poly = 0.0;

  if (measure_EMU_temp == true) {
    // Get EMU temperature in degrees C
    EMU_temp_raw = EMU_TemperatureGet();

    // Polynomial Corrections. Get the coefficients from the RM
    EMU_temp_2nd_order_poly =
      ((x2_term_2nd_order * EMU_temp_raw) + x1_term_2nd_order)
      * EMU_temp_raw + x0_term_2nd_order;

    EMU_temp_3rd_order_poly =
      (((x3_term_3rd_order * EMU_temp_raw) + x2_term_3rd_order)
       * EMU_temp_raw + x1_term_3rd_order)
      * EMU_temp_raw + x0_term_3rd_order;

    // Output via serial UART
    printf(
      "EMU_TempRaw:\t\t\t%3.2f\t  2nd Poly Correction:\t%3.2f\t  3rd Poly Correction:\t%3.2f\n",
      EMU_temp_raw,
      EMU_temp_2nd_order_poly,
      EMU_temp_3rd_order_poly);

    // Clear flag
    measure_EMU_temp = false;
  }
}

/***************************************************************************//**
 * @brief EMU Interrupt Handler
 ******************************************************************************/
void EMU_IRQHandler(void)
{
  EMU_IntClear(EMU_IEN_TEMP);

  // Flag main to get EMU temperature and send value over UART
  measure_EMU_temp = true;
}
