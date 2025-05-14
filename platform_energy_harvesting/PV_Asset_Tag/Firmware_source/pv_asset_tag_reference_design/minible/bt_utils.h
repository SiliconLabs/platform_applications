/***************************************************************************//**
 * @file bt_utils.h
 * @brief macros for minible.
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
#ifndef COMMON_H
#define COMMON_H

#ifdef __GNUC__
// GCC
#ifdef _WIN32
#define PACKSTRUCT(struct) struct __attribute__((__packed__, gcc_struct))
#else
#define PACKSTRUCT(struct) struct __attribute__((__packed__))
#endif
#define WEAK __attribute__((__weak__))
#define OPTIMIZE_SPEED
#define OPTIMIZE_SIZE
#ifndef static_assert
#define static_assert _Static_assert
#endif
#define FORMAT_PRINTF(format_index, param_index) __attribute__((format(printf, format_index, param_index)));
#else
// IAR
#define PACKSTRUCT(struct) __packed struct
#define WEAK __weak
#define OPTIMIZE_SPEED
#define OPTIMIZE_SIZE
#define FORMAT_PRINTF(format_index, param_index)
#endif

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

#ifndef UTEST
#include <em_core.h>
#define CRITICAL_SECTION(code) CORE_CRITICAL_SECTION(code)
#else
#define CRITICAL_SECTION(code) { code }
#endif

void utils_init();

#endif
