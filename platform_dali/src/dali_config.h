/***************************************************************************//**
 * @file dali_config.h
 * @brief Header file for DALI configuration.
 * @version 0.01
 *******************************************************************************
 * # License
 * <b>Copyright 2019 Silicon Laboratories Inc. www.silabs.com</b>
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

#ifndef DALI_CONFIG_H
#define DALI_CONFIG_H

#include "em_device.h"

// If 0 - 01 is logic 0 & 10 is logic 1, if 1 - 01 is logic 1 & 10 is logic 0
#define IDLE_LEVEL      1

#if defined(EFR32MG21A010F1024IM32)
// USARTn and TIMERn number (n = 0, 1,...)
#define DALI_TIMER_NUM  0
#define TO_TIMER_NUM    1
#define SPI_USART_NUM   2

// PRS channels
#define PIN_PRS_CH      0
#define TX_PRS_CH       1
#define TIMER_PRS_CH    2

#if !defined(DALI_USE_DMADRV)
// DMA channels
#define DMA_CH_SPI_TX   0
#define DMA_CH_RX_PIN   1
#define DMA_CH_RX_TMR   2
#endif

// TX pin
#define SPI_MOSI_PIN    0
#define SPI_MOSI_PORT   gpioPortC

// RX pin
#define DALI_RX_PIN     1
#define DALI_RX_PORT    gpioPortC

// PRS DMA request number (0 or 1)
#define DMAREQ_NUM      0

// RX TIMER constant, HFXO = 38.4 MHz, TIMER prescaling factor 1
#define DALI_HALF_T     7999

// RX timeout TIMER constants, HFXO = 38.4 MHz, TIMER prescaling factor 16
#define RX_EDGE_TO      5000
#if !defined(DALI_SECONDARY)
#define RX_BWARD_TO     22000
#else
#define TX_BWARD_WAIT   7000
#endif

#elif defined(EFR32MG12P432F1024GL125)
// USARTn and TIMERn number (n = 0, 1,...)
#define DALI_TIMER_NUM  0
#define TO_TIMER_NUM    1
#define SPI_USART_NUM   3

// PRS channels
#define PIN_PRS_CH      0
#define TX_PRS_CH       1
#define TIMER_PRS_CH    3

#if !defined(DALI_USE_DMADRV)
// DMA channels
#define DMA_CH_SPI_TX   0
#define DMA_CH_RX_PIN   1
#define DMA_CH_RX_TMR   2
#endif

// TX pin
#define SPI_MOSI_PIN    11
#define SPI_MOSI_PORT   gpioPortD
#define SPI_TX_LOC      USART_ROUTELOC0_TXLOC_LOC3

// RX pin
#define DALI_RX_PIN     12
#define DALI_RX_PORT    gpioPortD

// PRS DMA request number (0 or 1)
#define DMAREQ_NUM      0

// RX TIMER constant, HFXO = 38.4 MHz, TIMER prescaling factor 1
#define DALI_HALF_T     7999

// RX timeout TIMER constants, HFXO = 38.4 MHz, TIMER prescaling factor 16
#define RX_EDGE_TO      5000
#if !defined(DALI_SECONDARY)
#define RX_BWARD_TO     22000
#else
#define TX_BWARD_WAIT   7000
#endif

#elif defined(EFR32MG24B210F1536IM48)

// TX pin
#define DALI_TX_PIN     1
#define DALI_TX_PORT    gpioPortC

// RX pin
#define DALI_RX_PIN     2
#define DALI_RX_PORT    gpioPortC

#else
#error "dali_config.h: PART NUMBER undefined"
#endif
#endif // DALI_CONFIG_H
