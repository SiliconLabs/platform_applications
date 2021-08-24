/***************************************************************************//**
 * @file
 * @brief Helper functions for capacitive touch using CSEN
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *******************************************************************************
 *
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 *
 ******************************************************************************/

#include <stdbool.h>
#include "em_cmu.h"
#include "em_rtcc.h"
#include "em_csen.h"

#include "app_csen.h"

#include "types.h"

#define RIGHT_BUTTON_LEFT_LIMIT 100
#define RIGHT_BUTTON_RIGHT_LIMIT 190

extern enum event_t irq_type;

static uint32_t pad_capacitance[4] = { 0, 0, 0, 0 };
static uint32_t pad_mins[4] = { 60000, 60000, 60000, 60000 };
static uint32_t pad_level[6] = PAD_LEVEL_THRS;
static uint8_t current_channel = 0;
static int8_t max_pad = -1;
static uint32_t maxval = PAD_THRS;

static CSEN_Event_t current_event = CSEN_EVENT_DEFAULT;

static const uint8_t pad_channels[4] = { 3, 2, 0, 1 };
static const CSEN_SingleSel_TypeDef slider_input_channels[] = {
                                                csenSingleSelAPORT1XCH22,
                                                csenSingleSelAPORT1YCH31,
                                                csenSingleSelAPORT1XCH20,
                                                csenSingleSelAPORT1YCH7 };

/*******************************************************************************
 **************************   LOCAL FUNCTIONS   ********************************
 ******************************************************************************/

/**************************************************************************//**
 * Return the current system run time in milliseconds.
 *****************************************************************************/
static uint32_t get_system_runtime_ms(void)
{
  return (RTCC_CounterGet() * 1000) / 1024;
}

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/**************************************************************************//**
 * Setup CSEN, do initial calibration and start continuous scan
 *****************************************************************************/
void setup_CSEN(void)
{
  CSEN_Init_TypeDef csen_init = CSEN_INIT_DEFAULT;
  CSEN_InitMode_TypeDef csen_measure_mode_init = CSEN_INITMODE_DEFAULT;
  uint8_t i;

  /* Use LFXO as LF clock source since we are already using it
     for the RTCC */
  CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_LFXO);

  CMU_ClockEnable(cmuClock_CSEN_HF, true);
  CMU_ClockEnable(cmuClock_CSEN_LF, true);

  /* Setup timer to do 16 CSEN scans per second
     The input to the CSEN_LF clock is by default LFB/16 */
  csen_init.pcPrescale = csenPCPrescaleDiv16;
  csen_init.pcReload = 7;

  CSEN_Init(CSEN, &csen_init);

  NVIC_ClearPendingIRQ(CSEN_IRQn);
  NVIC_EnableIRQ(CSEN_IRQn);

  // Setup measurement mode to single conversion for calibration
  csen_measure_mode_init.sampleMode = csenSampleModeSingle;
  csen_measure_mode_init.trigSel = csenTrigSelStart;
  csen_measure_mode_init.sarRes = csenSARRes16;
  /* Normal reference current when measuring single pads */
  csen_measure_mode_init.gainSel = csenGainSel8X;

  // Do an initial reading of each pad to establish the zero-level
  for (i = 0; i <= 3; i++) {
    csen_measure_mode_init.singleSel = slider_input_channels[i];
    CSEN_InitMode(CSEN, &csen_measure_mode_init);
    CSEN_Enable(CSEN);
    CSEN_Start(CSEN);
    while (CSEN_IsBusy(CSEN) || !(CSEN_IntGet(CSEN) && CSEN_IF_CONV)) ;
    CSEN_IntClear(CSEN, CSEN_IF_CONV);
    // Subtract a margin from the reading to account for read noise
    pad_mins[i] = CSEN_DataGet(CSEN) - APP_CSEN_NOISE_MARGIN;
  }

  // Setup measurement mode to timer-triggered scan
  csen_measure_mode_init.inputMask0 = (1U << 31) |
                                   (1U << 22) |
                                   (1U << 20) |
                                   (1U << 7);
  csen_measure_mode_init.sampleMode = csenSampleModeScan;
  csen_measure_mode_init.trigSel = csenTrigSelTimer;

  CSEN_InitMode(CSEN, &csen_measure_mode_init);
  CSEN_Enable(CSEN);

  CSEN_IntEnable(CSEN, CSEN_IEN_CONV);

  // Start continuous scan cycle
  current_channel = 0;
  CSEN_Start(CSEN);
}

/**************************************************************************//**
 * Calculate slider position
 *
 * @returns slider position
 *****************************************************************************/
int32_t csen_calculate_slider_position(void)
{
  int32_t tmp_pos = -1;

  if (max_pad != -1) {
    tmp_pos = (max_pad) << 6;

    // Avoid moving calculated position to the left when we are close to the
    // right edge and the measured capacitance at the rightmost pad is
    // close to the noise floor
    tmp_pos -= (pad_level[max_pad] << 5) / pad_level[max_pad + 1];

    // Avoid moving calculated position to the right when we are close to the
    // left edge and the measured capacitance at the leftmost pad is close
    // to the noise floor
    tmp_pos += (pad_level[max_pad + 2] << 5) / pad_level[max_pad + 1];
  }

  return tmp_pos;
}

/**************************************************************************//**
 * Get touch event data
 *****************************************************************************/
CSEN_Event_t csen_get_event(void)
{
  return current_event;
}

/**************************************************************************//**
 * Check CSEN data after a scan is completed
 *****************************************************************************/
void csen_check_scanned_data(void)
{
  uint8_t i;
  uint32_t tmp_max_val = PAD_THRS;

  CSEN_Event_t tmp_event = current_event;

  max_pad = -1;

  for (i = 0; i < 4; i++) {
    // Order scan results and check if any pad has exceeded the threshold
    // defining a touch event
    pad_level[i + 1] = ((pad_capacitance[i] - pad_mins[i]) << 16) /
                       (65535 - pad_mins[i]);
    if (pad_level[i + 1] > tmp_max_val) {
      tmp_max_val = pad_level[i + 1];
      max_pad = i;
    } else if (pad_level[i + 1] < PAD_THRS) {
      pad_level[i + 1] = PAD_THRS;
    }
  }

  maxval = tmp_max_val;

  tmp_event.sliderPos = csen_calculate_slider_position();

  if (tmp_event.eventActive) {
    tmp_event.eventDuration = get_system_runtime_ms() - tmp_event.eventStart;
  } else if (max_pad != -1) {
    tmp_event.eventActive = true;
    tmp_event.eventStart = get_system_runtime_ms();
    tmp_event.sliderStartPos = tmp_event.sliderPos;
    tmp_event.sliderTravel = 0;
  }

  if (max_pad == -1) {
    tmp_event.eventActive = false;
  }

  if (tmp_event.eventActive) {
    tmp_event.touchForce = maxval;
    tmp_event.sliderPrevPos = tmp_event.sliderPos;
    tmp_event.sliderTravel = tmp_event.sliderPos - tmp_event.sliderStartPos;
  }

  current_event = tmp_event;
}

/***************************************************************************//**
  * Get touch slider state.
 ******************************************************************************/
enum event_t get_touch_slider_state(void)
{
  static bool ongoing_touch_event = false;

  CSEN_Event_t touch_slider_event = csen_get_event();
  if (touch_slider_event.eventActive &&
      touch_slider_event.sliderPos > RIGHT_BUTTON_LEFT_LIMIT) {
    ongoing_touch_event = true;
    return TOUCH_SLIDER_RIGHT_PUSH;
  }
  if (!touch_slider_event.eventActive && ongoing_touch_event == true) {
    ongoing_touch_event = false;
    if (touch_slider_event.sliderPrevPos > RIGHT_BUTTON_LEFT_LIMIT &&
         touch_slider_event.sliderPrevPos < RIGHT_BUTTON_RIGHT_LIMIT)
      return TOUCH_SLIDER_RIGHT_RELEASE;
    else
      return TOUCH_SLIDER_RIGHT_CANCEL;
  }

  return UNDETERMINED;
}

/**************************************************************************//**
 * CSEN Interrupt handler
 *****************************************************************************/
void CSEN_IRQHandler(void)
{
  uint8_t pad_number;

  CSEN->IFC = _CSEN_IFC_MASK;

  pad_number = pad_channels[current_channel];

  pad_capacitance[pad_number] = CSEN->DATA;

  current_channel++;


  /* If a scan is completed, do some more checking of the data since we have
     time to spare before the next scan is scheduled to start */
  if (current_channel > 3) {
    current_channel = 0;
    csen_check_scanned_data();
  }
  // to prevent overriding btn push, or tick
  if (irq_type == UNDETERMINED)
    irq_type = get_touch_slider_state();
}
