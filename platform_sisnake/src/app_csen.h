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

#ifndef APP_CSEN_H
#define APP_CSEN_H

#include <stdbool.h>

#define PAD_THRS 1500
#define PAD_LEVEL_0 { 0, 0, 0, 0, 0, 0 }
#define PAD_LEVEL_THRS { PAD_THRS, PAD_THRS, PAD_THRS, PAD_THRS, PAD_THRS, PAD_THRS }
#define APP_CSEN_NOISE_MARGIN 500

typedef struct {
  int32_t sliderPos;
  int32_t sliderPrevPos;
  int32_t sliderStartPos;
  int32_t sliderTravel;
  uint32_t eventStart;
  uint32_t eventDuration;
  uint32_t touchForce;
  bool eventActive;
} CSEN_Event_t;

#define CSEN_EVENT_DEFAULT \
  {                        \
    -1,                    \
    -1,                    \
    -1,                    \
    0,                     \
    0,                     \
    0,                     \
    0,                     \
    false,                 \
  }

#ifdef __cplusplus
extern "C" {
#endif

// Function prototypes
void setup_CSEN(void);

int32_t csen_calculate_slider_position(void);

void csen_check_scanned_data(void);

CSEN_Event_t csen_get_event(void);

bool is_CSEN_irq_happened(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_CSEN_H */
