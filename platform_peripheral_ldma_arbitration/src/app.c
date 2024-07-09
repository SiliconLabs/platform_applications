/***************************************************************************//**
 * @file
 * @brief Top level application functions
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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
#include "em_cmu.h"
#include "em_timer.h"
#include "dmadrv.h"

#define FIXEDCHANNELS                     5
#define CHANNELS                          4
#define N                                 4 /* "LOOPS" */
#define M                                 64 /* "XFERCOUNT" */

#define LOOP_EXEC(loop_num, func) \
  for (uint32_t i = 0; i < loop_num; i++) func;

static unsigned int DMA_CH2;
static unsigned int DMA_CH3;
static unsigned int DMA_CH4;
static unsigned int DMA_CH5;

/* descriptors must be global, not on stack, because begin_transfer may exit
 *   before last LDMA access */
LDMA_Descriptor_t descriptors[5];
volatile unsigned int irq_counter = 0;
volatile uint32_t data[CHANNELS][N * M + 1];

void setup_timer0(void)
{
  CMU_ClockEnable(cmuClock_TIMER0, 1);
  TIMER_Init_TypeDef init;
  init.enable = 1;
  init.debugRun = 0;
  init.prescale = 0;
  init.clkSel = 0;
  init.count2x = 0;
  init.fallAction = 0;
  init.riseAction = 0;
  init.dmaClrAct = 0;
  init.oneShot = 0;
  init.sync = 0;
  init.mode = 0; /* count up */
  TIMER0->TOP = 0xffff;

  /* Setup PD1 to toggle on overflow (3.45 ms) */
  TIMER0->CC[0].CTRL = timerCCModeCompare | TIMER_CC_CTRL_COFOA_TOGGLE;
  TIMER0->ROUTELOC0 = TIMER_ROUTELOC0_CC0LOC_LOC2;
  TIMER0->ROUTEPEN = TIMER_ROUTEPEN_CC0PEN;
  GPIO_PinModeSet(gpioPortD, 1, gpioModePushPull, 0);
  TIMER_Init(TIMER0, &init);
}

void setup_wtimer0(void)
{
  CMU_ClockEnable(cmuClock_WTIMER0, 1);
  CMU->HFPERPRESC = 0;
  TIMER_Init_TypeDef init;
  init.enable = 1;
  init.debugRun = 0;
  init.prescale = 0;
  init.clkSel = 0;
  init.count2x = 0;
  init.fallAction = 0;
  init.riseAction = 0;
  init.dmaClrAct = 0;
  init.oneShot = 0;
  init.sync = 0;
  init.mode = 0; /* count up */
  WTIMER0->TOP = 0xffffffff; /* period 3m 46s */
  /* Setup PC15 to toggle on overflow */
  WTIMER0->CC[0].CTRL = timerCCModeCompare | TIMER_CC_CTRL_COFOA_TOGGLE;
  WTIMER0->ROUTELOC0 = TIMER_ROUTELOC0_CC0LOC_LOC4;
  WTIMER0->ROUTEPEN = TIMER_ROUTEPEN_CC0PEN;
  GPIO_PinModeSet(gpioPortC, 15, gpioModePushPull, 0);
  TIMER_Init(WTIMER0, &init);
}

Ecode_t setup_ldma()
{
  Ecode_t err;

  err = DMADRV_Init();
  if ((err != ECODE_OK)
      && (err != ECODE_EMDRV_DMADRV_ALREADY_INITIALIZED)) {
    return err;
  }

  err = DMADRV_AllocateChannel(&DMA_CH2, NULL);
  if (err != ECODE_EMDRV_DMADRV_OK) {
    return err;
  }
  err = DMADRV_AllocateChannel(&DMA_CH3, NULL);
  if (err != ECODE_EMDRV_DMADRV_OK) {
    return err;
  }

  err = DMADRV_AllocateChannel(&DMA_CH4, NULL);
  if (err != ECODE_EMDRV_DMADRV_OK) {
    return err;
  }

  err = DMADRV_AllocateChannel(&DMA_CH5, NULL);
  if (err != ECODE_EMDRV_DMADRV_OK) {
    return err;
  }
  return ECODE_OK;
}

bool dma_callback(unsigned int channel,
                  unsigned int sequenceNo,
                  void *userParam)
{
  (void)channel;
  (void)sequenceNo;
  (void)userParam;

  irq_counter++;

  return false;
}

int begin_transfer(unsigned int ch,
                   uint32_t peripheral,
                   uint32_t mem,
                   unsigned int count,
                   unsigned int blocklen,
                   uint8_t slots)
{
  EFM_ASSERT(count < 0x100);
  EFM_ASSERT(slots < 4);
  LDMA_TransferCfg_t transfer;
  transfer.ldmaCfgArbSlots = slots;
  transfer.ldmaCfgDstIncSign = 0;
  transfer.ldmaCfgSrcIncSign = 0;
  transfer.ldmaCtrlSyncPrsClrOff = 0;
  transfer.ldmaCtrlSyncPrsClrOn = 0;
  transfer.ldmaCtrlSyncPrsSetOff = 0;
  transfer.ldmaCtrlSyncPrsSetOn = 0;
  transfer.ldmaDbgHalt = 1;
  transfer.ldmaLoopCnt = count - 2; /* loop should repeat count-1 times */
  transfer.ldmaReqDis = 0;
  // transfer.ldmaReqSel = 0; /* None, this is a software initiated transfer */
  transfer.ldmaReqSel = ((0x19) << 16); /* Request on TIMER0 Overflow/Underflow
                                         */

  /********************************** Initialize common values */
  for (unsigned int i = 0; i < 2; i++) {
    descriptors[i].xfer.structType = 0; /* transfer */
    descriptors[i].xfer.structReq = 0; /* don not start on load */
    descriptors[i].xfer.xferCnt = blocklen - 1; /* blocklen words */
    descriptors[i].xfer.byteSwap = 0;
    descriptors[i].xfer.blockSize = ldmaCtrlBlockSizeUnit32;
    descriptors[i].xfer.doneIfs = 0; /* no interrupt on completion */
    descriptors[i].xfer.reqMode = 0;
    descriptors[i].xfer.size = 2; /* word */
    descriptors[i].xfer.srcInc = 3; /* always read same location */
    descriptors[i].xfer.srcAddr = (unsigned int) peripheral;
    descriptors[i].xfer.srcAddrMode = 0; /* always absolute */
    descriptors[i].xfer.dstInc = 0; /* no skipping */
  }

  /************************************ First descriptor transfers a single
   *   value */
  descriptors[0].xfer.dstAddrMode = 0; /* absolute first time */
  descriptors[0].xfer.dstAddr = (unsigned int)mem;
  descriptors[0].xfer.link = 1; /* load next descriptor */
  descriptors[0].xfer.linkMode = 0; /* absolute */
  descriptors[0].xfer.linkAddr = (unsigned int)&descriptors[1] >> 2;
  descriptors[0].xfer.decLoopCnt = 0; /* no loop */

  /************************************ Second descriptor performs looping */
  descriptors[1].xfer.dstAddrMode = 1; /* relative */
  descriptors[1].xfer.dstAddr = 0;
  descriptors[1].xfer.link = 0;
  descriptors[1].xfer.linkMode = 1; /* relative */
  descriptors[1].xfer.linkAddr = 0;
  descriptors[1].xfer.decLoopCnt = 1; /* loop */
//  LDMA_StartTransfer(ch,&transfer,&descriptors[0]);
  DMADRV_LdmaStartTransfer(ch,
                           &transfer,
                           &descriptors[0],
                           dma_callback,
                           NULL);
  return 0;
}

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  for (int i = 0; i < (N * M); i++) {
    for (uint32_t j = 0; j < CHANNELS; j++) {
      data[j][i] = -1;
    }
  }
  setup_ldma(); /* parameter is number of fixed channels */
  CMU_ClockEnable(cmuClock_GPIO, 1);
  GPIO_PinModeSet(gpioPortI, 0, gpioModePushPull, 0);
  setup_timer0();
  setup_wtimer0();

  uint32_t peripheral = (uint32_t) &WTIMER0->CNT;

  /* Last parameter of begin_transfer is the encoded arbitration slots */
  begin_transfer(DMA_CH2, peripheral, (uint32_t)&data[0][0], N, M, 0);
  begin_transfer(DMA_CH3, peripheral, (uint32_t)&data[1][0], N, M, 1);
  begin_transfer(DMA_CH4, peripheral, (uint32_t)&data[2][0], N, M, 2);
  begin_transfer(DMA_CH5, peripheral, (uint32_t)&data[3][0], N, M, 3);

  printf(
    "Start --- Each row represents 32 words transfered; channels below %d are fixed\n",
    FIXEDCHANNELS);
  uint32_t indexes[CHANNELS] = { 0, 0, 0 };
  uint32_t min = 0;

  /* Next lines sets min to MAX!!! so that we observe transition at start */
  for (uint32_t i = 0; i < CHANNELS; i++) {
    if (data[i][indexes[i]] > data[min][indexes[min]]) {
      min = i;
    }
  }
  while (1) {
    uint32_t counter;
    uint32_t oldmin = min;
    for (uint32_t i = 0; i < CHANNELS; i++) {
      if (data[i][indexes[i]] < data[min][indexes[min]]) {
        min = i;
      }
    }
    if (min != oldmin) {
      counter = 0;
    } else {
      indexes[min]++;
      counter++;
      if (32 == counter) {
        LOOP_EXEC(4 * min, printf(" "));
        LOOP_EXEC(4, printf("%ld", min + 2));
        printf("\n");
        counter = 0;
      }
    }
    if (indexes[min] == (N * M)) {
      data[min][indexes[min]] = -1;
      bool ok = 0; /* if any channel has not completed, continue */
      LOOP_EXEC(CHANNELS, ok |= (indexes[i] != (N * M)));
      if (!ok) {
        break;
      }
    }
  }

  if (irq_counter) {
    printf("Interrupt handler was called %u times\n", irq_counter);
  }
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
}
