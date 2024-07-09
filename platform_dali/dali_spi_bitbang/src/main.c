/***************************************************************************//**
 * @file main.c
 * @brief DALI main and secondary example.
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

#include <stdio.h>
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "dali_config.h"
#include "dali_define.h"
#include "dali_macro.h"
#include "retargetserial.h"

/***************************************************************************//**
 * @brief
 *   Main function
 ******************************************************************************/
int main(void)
{
#if (IDLE_LEVEL == 0)           // Get idle level
  uint8_t idleLevel = 0;
#else
  uint8_t idleLevel = 1;
#endif
  int8_t c;
  uint8_t fwdAddr = 0;          // Forward frame address
  uint8_t fwdData = 0;          // Forward frame data
  uint8_t bwdData = 0;          // Backward frame data
  DaliStatus_t state;           // DALI status
  CMU_HFXOInit_TypeDef hfxoInit = CMU_HFXOINIT_DEFAULT;

  // Chip errata
  CHIP_Init();

#if defined(_EMU_DCDCCTRL_MASK)
  // Initialize DCDC regulator with kit specific parameters
  EMU_DCDCInit_TypeDef dcdcInit = EMU_DCDCINIT_DEFAULT;
  EMU_DCDCInit(&dcdcInit);
#endif

  // Initialize HFXO with kit specific parameters
  CMU_HFXOInit(&hfxoInit);

  // Switch core clock to HFXO
#if defined(_SILICON_LABS_32B_SERIES_2)
  CMU_ClockSelectSet(cmuClock_SYSCLK, cmuSelect_HFXO);
#else
  CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
  CMU_OscillatorEnable(cmuOsc_HFRCO, false, false);
#endif

  // Initialize LEUART/USART and map LF to CRLF
  RETARGET_SerialInit();
  RETARGET_SerialCrLf(1);

  // Initialize DALI interface
  initDali();

  // Press 1 to start
#if !defined(DALI_SECONDARY)
  fwdAddr = 0xff;       // Broadcast address
  fwdData = 0x90;       // Query status command
  printf("\nPress 1: DALI Main (Idle Level %d) - Forward Frame TX\n",
         idleLevel);
#else
  bwdData = 0x64;       // Return status
  printf("\nPress 1: DALI Secondary (Idle Level %d) - Forward Frame RX\n",
         idleLevel);
#endif

  // While loop for DALI main or secondary
  while (1) {
    EMU_EnterEM1();
    state = getDaliStatus();

#if !defined(DALI_SECONDARY)
    // Handle different status of DALI main
    if (state == DALI_IDLE) {
      c = getchar();
      if (c == '1') {
        c = 0;
        printf("Sending DALI Main Forward Frame\n");
        startDaliTxDma(fwdAddr, fwdData);
      }
    } else {
      switch (state) {
        case DALI_DATA_RX_TIMEOUT:
          printf("Data RX Timeout\n");
          setDaliStatus(DALI_IDLE);
          printf("\nPress 1: DALI Main (Idle Level %d) - "
                 "Forward Frame TX\n", idleLevel);
          break;

        case DALI_BACKWARD_RX_TIMEOUT:
          printf("Backward RX Timeout\n");
          setDaliStatus(DALI_IDLE);
          printf("\nPress 1: DALI Main (Idle Level %d) - "
                 "Forward Frame TX\n", idleLevel);
          break;

        case DALI_BACKWARD_RX:
          bwdData = 0;
          if (decodeDaliRx(0, &bwdData)) {
            printf("FWD TX - Address: %3d Data: %3d\n", fwdAddr, fwdData);
            printf("BWD RX - Data: %3d\n", bwdData);
            printf("\nPress 1: DALI Main (Idle Level %d) - "
                   "Forward Frame TX\n", idleLevel);
          } else {
            printf("Backward RX Framing Error\n");
            printf("\nPress 1: DALI Main (Idle Level %d) - "
                   "Forward Frame TX\n", idleLevel);
          }
          break;

        default:
          break;
      }
    }
#else
    // Handle different status of DALI secondary
    if (state == DALI_IDLE) {
      c = getchar();
      if (c == '1') {
        c = 0;
        printf("Waiting DALI Main Forward Frame\n");
        startDaliRxDma();
      }
    } else {
      switch (state) {
        case DALI_DATA_RX_TIMEOUT:
          printf("Data RX Timeout\n");
          setDaliStatus(DALI_IDLE);
          printf(
            "\nPress 1: DALI Secondary (Idle Level %d) - Forward Frame RX\n",
            idleLevel);
          break;

        case DALI_FORWARD_RX:
          fwdAddr = 0;
          fwdData = 0;
          if (!decodeDaliRx(&fwdAddr, &fwdData)) {
            printf("Forward RX Framing Error\n");
            printf("\nPress 1: DALI Secondary (Idle Level %d) - "
                   "Forward Frame RX\n", idleLevel);
          }
          break;

        case DALI_BACKWARD_TX_READY:
          startDaliTxDma(0, bwdData);
          break;

        case DALI_BACKWARD_TX_DONE:
          setDaliStatus(DALI_IDLE);
          printf("FWD RX - Address: %3d Data: %3d\n", fwdAddr, fwdData);
          printf("BWD TX - Data: %3d\n", bwdData);
          printf(
            "\nPress 1: DALI Secondary (Idle Level %d) - Forward Frame RX\n",
            idleLevel);
          break;

        default:
          break;
      }
    }
#endif
  }
}
