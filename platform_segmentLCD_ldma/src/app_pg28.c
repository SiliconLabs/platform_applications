/***************************************************************************//**
 * @file
 * @brief Top level application functions
 *******************************************************************************
 * # License
 * <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
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

#include "sl_segmentlcd.h"
#include "em_cmu.h"
#include "em_ldma.h"

#define LDMA_CHANNEL      0
#define LDMA_CH_MASK      (1 << LDMA_CHANNEL)

#define NUM_DIGIT         4
#define NUM_NUMBER        10
#define NUM_SEG           8
#define NUM_STATE         10

uint32_t segments[NUM_DIGIT][NUM_NUMBER][NUM_SEG];
uint32_t display[NUM_STATE][NUM_SEG];
LDMA_Descriptor_t descriptors[3];
extern sl_segment_lcd_mcu_display_t efm_display;
extern uint16_t segment_numbers[];

/***************************************************************************//**
 * Intialize buffer.
 ******************************************************************************/
void sl_segment_lcd_ldma_init(void)
{
  uint16_t bitpattern;
  int dig, num, seg, i, bit;

  for (dig = 0; dig < NUM_DIGIT; dig++) {
    for (num = 0; num < NUM_NUMBER; num++) {
      for (seg = 0; seg < NUM_DIGIT; seg++) {
        segments[dig][num][seg] = 0;
      }
    }
  }

  for (dig = 0; dig < NUM_DIGIT; dig++) {
    for (num = 0; num < NUM_NUMBER; num++) {
      bitpattern = segment_numbers[num];
      for (i = 0; i < 7; i++) {
        bit = efm_display.number[dig].bit[i];
        seg = efm_display.number[dig].com[i];
        if (bitpattern & (1 << i)) {
          segments[dig][num][seg] |= (1 << bit);
        }
      }
    }
  }
}

/***************************************************************************//**
 * LDMA IRQ handler.
 ******************************************************************************/
void LDMA_IRQHandler(void)
{
  uint32_t pending;

  // Read interrupt source
  pending = LDMA_IntGet();

  // Clear interrupts
  LDMA_IntClear(pending);

  // Check for LDMA error
  if (pending & LDMA_IF_ERROR) {
    // Loop here to enable the debugger to see what has happened
    while (1) {}
  }
}

/***************************************************************************//**
* Disable Unused LCD Segments
*******************************************************************************/
void disableUnusedLCDSeg(void)
{
/***************************************************************************//**
* The LCD driver enables all segments, even those that are not mapped to
* segments on the pro kit board. These are disabled below in order to
* minimize current consumption.
*******************************************************************************/
  LCD_SegmentEnable(SL_SEGMENT_LCD_SEG_S00, false);
  LCD_SegmentEnable(SL_SEGMENT_LCD_SEG_S01, false);
  LCD_SegmentEnable(SL_SEGMENT_LCD_SEG_S02, false);
  LCD_SegmentEnable(SL_SEGMENT_LCD_SEG_S03, false);
  LCD_SegmentEnable(SL_SEGMENT_LCD_SEG_S04, false);
  LCD_SegmentEnable(SL_SEGMENT_LCD_SEG_S05, false);
  LCD_SegmentEnable(SL_SEGMENT_LCD_SEG_S06, false);
  LCD_SegmentEnable(SL_SEGMENT_LCD_SEG_S07, false);
  LCD_SegmentEnable(SL_SEGMENT_LCD_SEG_S08, false);
  LCD_SegmentEnable(SL_SEGMENT_LCD_SEG_S09, false);
  LCD_SegmentEnable(SL_SEGMENT_LCD_SEG_S10, false);
  LCD_SegmentEnable(SL_SEGMENT_LCD_SEG_S11, false);
  LCD_SegmentEnable(SL_SEGMENT_LCD_SEG_S12, false);
  LCD_SegmentEnable(SL_SEGMENT_LCD_SEG_S13, false);
  LCD_SegmentEnable(SL_SEGMENT_LCD_SEG_S14, false);
}

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  int num, seg, dig;

  CMU_ClockEnable(cmuClock_GPIO, true);

  // Initialize the LCD
  sl_segment_lcd_init(false);

  // Example only used upper numeric segments; disable unused segments
  disableUnusedLCDSeg();

  // Send a DMA request on each Frame Counter event
  LCD->BIASCTRL_SET = LCD_BIASCTRL_DMAMODE_DMAFC;

  // Frame Counter event occurs every 1 second
  LCD_FrameCountInit_TypeDef fc_init = {
    .enable = true,
    .top = 32,
    .prescale = lcdFCPrescDiv1
  };

  LCD_FrameCountInit(&fc_init);

  // Sync the SEGn registers automatically when SEG7 is written to
  LCD_SyncStart(true, lcdLoadAddrSegd7);

  // Fill the segments[][][] buffer
  sl_segment_lcd_ldma_init();

  // Fill the display[][] buffer to output:
  // 00000 -> 11111 -> 22222 -> ... -> 99999
  for (num = 0; num < 10; num++) {
    for (seg = 0; seg < NUM_SEG; seg++) {
      display[num][seg] = 0;
      for (dig = 0; dig < NUM_DIGIT; dig++) {
        display[num][seg] |= segments[dig][num][seg];
      }
    }
  }

  // Initialize the LDMA
  LDMA_Init_t init = LDMA_INIT_DEFAULT;
  LDMA_Init(&init);

  // Configure the LDMA to trigger on an LCD DMA request
  LDMA_TransferCfg_t transferCfg = LDMA_TRANSFER_CFG_PERIPHERAL_LOOP(
    ldmaPeripheralSignal_LCD,
    NUM_STATE - 1);

  // 1st descriptor sets the base SRC address of the LDMA channel
  descriptors[0] = (LDMA_Descriptor_t)LDMA_DESCRIPTOR_LINKREL_WRITE(
    (uint32_t)&(display[0][0]),
    &(LDMA->CH[LDMA_CHANNEL].SRC),
    1);

  // 2nd descriptor writes values from display[][] buffer to the LCD_SEGn
  // registers
  descriptors[1] = (LDMA_Descriptor_t)LDMA_DESCRIPTOR_LINKREL_M2M_WORD(
    0,
    &(LCD->SEGD0),
    8,
    0);
  descriptors[1].xfer.srcAddrMode = ldmaCtrlSrcAddrModeRel;
  descriptors[1].xfer.srcInc = ldmaCtrlSrcIncOne;
  descriptors[1].xfer.dstInc = ldmaCtrlDstIncTwo;
  descriptors[1].xfer.structReq = false;
  descriptors[1].xfer.decLoopCnt = 1;

  // 3rd descriptor resets the LOOP counter
  descriptors[2] = (LDMA_Descriptor_t)LDMA_DESCRIPTOR_LINKREL_WRITE(
    NUM_STATE - 1,
    &(LDMA->CH[LDMA_CHANNEL].LOOP),
    -2);

  // Start LDMA transfers
  LDMA_StartTransfer(0, (void *)&transferCfg, (void *)&descriptors[0]);
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
}
