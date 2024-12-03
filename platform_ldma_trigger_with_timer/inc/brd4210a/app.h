/***************************************************************************//**
 * @file
 * @brief LDMA examples function
 *******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
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

#ifndef APP_H
#define APP_H

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

#define PRS_MODE                   1

#define TIMER_DEBUG                1

// Set the VDAC to max frequency of 1 MHz
#define CLK_VDAC_FREQ              1000000

// Note: change this to determine the frequency of the sine wave
#define WAVEFORM_FREQ              10000

// Size of the number of points in the generated sine wave
#define SINE_TABLE_SIZE            32

// The timer needs to run at SINE_TABLE_SIZE times faster than the desired
// waveform frequency because it needs to output SINE_TABLE_SIZE points in that
// same amount of time
#define TIMER0_FREQ                (WAVEFORM_FREQ * SINE_TABLE_SIZE)

// Note: change this to change which channel the VDAC outputs to. This value can
// be either a zero or one
#define CHANNEL_NUM                0

/*
 * The port and pin for the VDAC output is set in VDAC_OUTCTRL register. The
 * ABUSPORTSELCHx fields in this register are defined as follows:
 * 0  No GPIO selected for CHx ABUS output
 * 1  Port A selected
 * 2  Port B selected
 * 3  Port C selected
 * 4  Port D selected
 *
 * The VDAC port pin settings do not need to be set when the main output is
 * used. Refer to the device Reference Manual and Datasheet for more details.
 * For this example, the CH0 main output is selected.
 */

#if (PRS_MODE)
#define TIMER0_PRS_CHANNEL         0
#endif

/***************************************************************************//**
 * Initialize application
 ******************************************************************************/
void app_init(void);

/***************************************************************************//**
 * App ticking function
 ******************************************************************************/
void app_process_action(void);

#endif // APP_H
