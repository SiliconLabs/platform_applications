/***************************************************************************//**
 * @file
 * @brief Main app logic.
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

#ifndef SISNAKE_H_
#define SISNAKE_H_

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * Initialize Sisnake.
 ******************************************************************************/
void sisnake_init(void);

/***************************************************************************//**
 * Sisnake main mechanics state machine.
 ******************************************************************************/
void sisnake_process_action(void);

#ifdef __cplusplus
}
#endif

#endif /* SISNAKE_H_ */
