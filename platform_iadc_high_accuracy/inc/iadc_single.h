/***************************************************************************//**
 * @file
 * @brief IADC example functions
 *******************************************************************************
 * # License
 * <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
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

#ifndef IADC_SINGLE_H
#define IADC_SINGLE_H

/***************************************************************************//**
 * Initialize IADC for single 20-bit high accuracy conversion.
 ******************************************************************************/
void iadc_single_init(void);

/***************************************************************************//**
 * IADC conversion complete process action.
 ******************************************************************************/
void iadc_single_process_action(void);

#endif // IADC_SINGLE_H
