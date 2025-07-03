/***************************************************************************//**
 * @file app.c
 * @brief TRNG baremetal example
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

#include "em_cmu.h"
#include "sl_udelay.h"
#include "stdint.h"
#include "stdbool.h"

#include "printf.h"

// Defining test data from gg11-rm section 32.3.4.1

/*
 * TRNG_TEST_KEY     0x2B7E151628AED2A6ABF7158809CF4F3C
 */
uint32_t trng_test_key[] = {
  0x2B7E1516,
  0x28AED2A6,
  0xABF71588,
  0x09CF4F3C
};

/*
 * TRNG_TEST_INPUT
 *  TRNG_TEST_IN0     0x6BC0BCE12A459991E134741A7F9E1925
 *  TRNG_TEST_IN1     0xAE2D8A571E03AC9C9EB76FAC45AF8E51
 *  TRNG_TEST_IN2     0x30C81C46A35CE411E5FBC1191A0A52EF
 *  TRNG_TEST_IN3     0xF69F2445DF4F9B17AD2B417BE66C3710
 */
uint32_t trng_test_data[] = {
  0x6BC0BCE1,
  0x2A459991,
  0xE134741A,
  0x7F9E1925,
  0xAE2D8A57,
  0x1E03AC9C,
  0x9EB76FAC,
  0x45AF8E51,
  0x30C81C46,
  0xA35CE411,
  0xE5FBC119,
  0x1A0A52EF,
  0xF69F2445,
  0xDF4F9B17,
  0xAD2B417B,
  0xE66C3710
};

/*
 *  TRNG_TEST_OUT     0x3FF1CAA1681FAC09120ECA307586E1A7
 */
uint32_t trng_test_out[] = {
  0x3FF1CAA1,
  0x681FAC09,
  0x120ECA30,
  0x7586E1A7
};

// Function prototypes for Conditioning and Entropy Check
bool trng_check_conditioning(void);
bool trng_check_entropy(void);
void trng_int_enable(void);
void trng_reset(void);

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init()
{
  printf("bare-metal TRNG example using ADC as entropy source\r\n\r\n");

  // enable TRNG clock
  CMU_ClockEnable(cmuClock_TRNG0, true);

  bool trng_status;

  // TRNG Check Conditioning
  trng_status = trng_check_conditioning();
  printf("TRNG Condition Function Check -> %s\r\n\r\n",
         trng_status ? "PASSED" : "FAILED");

  // TRNG Check Entropy
  trng_status = trng_check_entropy();
  printf("TRNG Entropy Source Check -> %s\r\n\r\n",
         trng_status ? "PASSED" : "FAILED");

  // software reset
  trng_reset();

  // Enable Interrupts
  trng_int_enable();

  // Enable trng
  TRNG0->CONTROL |= TRNG_CONTROL_ENABLE;
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
  // get data from trng fifo
  if (TRNG0->FIFOLEVEL > 0) {
    printf("0x%08lX\r\n", __REV(TRNG0->FIFO));
  }

  // delay 500 ms
  sl_udelay_wait(500000);
}

/***************************************************************************//**
 * TRNG0 Interrupt Handler
 *
 * Resets trng on test failure or noise alarm
 ******************************************************************************/
void TRNG0_IRQHandler(void)
{
  uint32_t temp = 0;

  // get which interrupt flags are set
  uint32_t flags = TRNG0->STATUS & 0x270;

  // disable trng
  TRNG0->CONTROL &= ~TRNG_CONTROL_ENABLE;

  // empty fifo
  while (TRNG0->FIFOLEVEL != 0) {
    // dummy read to clear fifo before TRNG reset
    temp |= TRNG0->FIFO;
  }

  // state which flag set
  printf("\r\n====================================================\r\n");
  printf("TRNG noise/failure flag set\r\n");
  if (flags & _TRNG_STATUS_ALMIF_MASK >> _TRNG_STATUS_ALMIF_SHIFT) {
    printf("\t- AIS32 Noise alarm detected\r\n");
  }
  if (flags & _TRNG_STATUS_PREIF_MASK >> _TRNG_STATUS_PREIF_SHIFT) {
    printf("\t- AIS32 Preliminary Noise alarm detected\r\n");
  }
  if (flags & _TRNG_STATUS_APT4096IF_MASK >> _TRNG_STATUS_APT4096IF_SHIFT) {
    printf(
      "\t- Adaptive Proportion test (4096-sample window) failure detected\r\n");
  }
  if (flags & _TRNG_STATUS_APT64IF_MASK >> _TRNG_STATUS_APT64IF_SHIFT) {
    printf(
      "\t- Adaptive Proportion test (64-sample window) failure detected\r\n");
  }
  if (flags & _TRNG_STATUS_REPCOUNTIF_MASK >> _TRNG_STATUS_REPCOUNTIF_SHIFT) {
    printf("\t- Repetition count test failure detected\r\n");
  }
  printf("Reseting TRNG\r\n");
  printf("\r\n====================================================\r\n");

  // software reset
  trng_reset();

  // enable trng
  TRNG0->CONTROL |= TRNG_CONTROL_ENABLE;
}

/***************************************************************************//**
 * Enable Interrupts for trng noise alarms
 *
 * Monitors the following:
 *  - AIS31 noise alarm
 *  - AIS31 preliminary noise alarm
 *  - 4096-sample adaptive proportion test failure
 *  - 64-sample adaptive proportion test failure
 *  - Repetition count test failure
 ******************************************************************************/
void trng_int_enable(void)
{
  // Enable interrupts for all failure sources
  TRNG0->CONTROL |= TRNG_CONTROL_ALMIEN | TRNG_CONTROL_PREIEN
                    | TRNG_CONTROL_APT4096IEN | TRNG_CONTROL_APT64IEN
                    | TRNG_CONTROL_REPCOUNTIEN;

  // Enable NVIC
  NVIC_ClearPendingIRQ(TRNG0_IRQn);
  NVIC_EnableIRQ(TRNG0_IRQn);
}

/***************************************************************************//**
 * Software Reset the trng
 ******************************************************************************/
void trng_reset(void)
{
  TRNG0->CONTROL |= TRNG_CONTROL_SOFTRESET;
  TRNG0->CONTROL &= ~TRNG_CONTROL_SOFTRESET;
}

/***************************************************************************//**
 * TRNG Check Conditioning
 *
 * As defined by 32.3.4.1 'Checking the Conditioning Function'
 *    of GG11-rm
 ******************************************************************************/
bool trng_check_conditioning(void)
{
  // Software Reset
  trng_reset();

  // Bypass AIS31 and NIST startup tests
  TRNG0->CONTROL |= TRNG_CONTROL_BYPAIS31_BYPASS | TRNG_CONTROL_BYPNIST_BYPASS;

  // Set input to condition function to TESTDATA
  TRNG0->CONTROL |= TRNG_CONTROL_TESTEN;
  // Clear Conditioning Bypass
  TRNG0->CONTROL &= ~(TRNG_CONTROL_CONDBYPASS);

  // Enable TRNG Peripheral
  TRNG0->CONTROL |= TRNG_CONTROL_ENABLE;

  // Write key into registers
  TRNG0->KEY0 = __REV(trng_test_key[0]);
  TRNG0->KEY1 = __REV(trng_test_key[1]);
  TRNG0->KEY2 = __REV(trng_test_key[2]);
  TRNG0->KEY3 = __REV(trng_test_key[3]);

  // write 512bits of data to TESTDATA 32bits at a time
  // wait for STATUS_TESTDATA_BUSY = 0 after each write.
  for (uint8_t index = 0; index < 512 / 32; index++) {
    TRNG0->TESTDATA = __REV(trng_test_data[index]);
    while (TRNG0->STATUS & TRNG_STATUS_TESTDATABUSY) {}
  }

  // wait until fifo has data?
  while (TRNG0->FIFOLEVEL < 4) {}

  // describe what is happening
  printf("====================================================\r\n");
  printf("Known-Answer Test for Conditioning Function\r\n");
  printf("====================================================\r\n");
  printf("\r\n%-18s0x%08lX%08lX%08lX%08lX\r\n", "Key", trng_test_key[0],
         trng_test_key[1], trng_test_key[2], trng_test_key[3]);
  printf("\r\n");
  printf("%-18s0x%08lX%08lX%08lX%08lX\r\n", "Input", trng_test_data[0],
         trng_test_data[1], trng_test_data[2], trng_test_data[3]);
  printf("%-18s0x%08lX%08lX%08lX%08lX\r\n", "", trng_test_data[4],
         trng_test_data[5], trng_test_data[6], trng_test_data[7]);
  printf("%-18s0x%08lX%08lX%08lX%08lX\r\n", "", trng_test_data[8],
         trng_test_data[9], trng_test_data[10], trng_test_data[11]);
  printf("%-18s0x%08lX%08lX%08lX%08lX\r\n", "", trng_test_data[12],
         trng_test_data[13], trng_test_data[14], trng_test_data[15]);
  printf("\r\n");
  printf("%-18s0x%08lX%08lX%08lX%08lX\r\n", "Expected Output", trng_test_out[0],
         trng_test_out[1], trng_test_out[2], trng_test_out[3]);
  printf("%-18s0x", "Received Output");

  // read fifo output
  uint32_t trng_fifo_out;
  for (uint8_t i = 0; i < 4; i++) {
    trng_fifo_out = TRNG0->FIFO;
    if (__REV(trng_fifo_out) != trng_test_out[i]) {
      printf("\r\nERROR\r\n\r\nExpected:\t0x%08lX\tReceived:\t0x%08lX\r\n",
             trng_test_out[i], __REV(trng_fifo_out));
      return false;
    } else {
      printf("%08lX", __REV(trng_fifo_out));
    }
  }

  printf("\r\n");
  printf("====================================================\r\n");

  return true;
}

/***************************************************************************//**
 * TRNG Check Entropy Source
 *
 * As defined by 32.3.4.2 'Checking the Entropy Source' of GG11-rm
 ******************************************************************************/
bool trng_check_entropy(void)
{
  // software reset
  trng_reset();

  // Bypass AIS31 and NIST startup tests
  TRNG0->CONTROL |= TRNG_CONTROL_BYPAIS31_BYPASS | TRNG_CONTROL_BYPNIST_BYPASS;

  // Disable test mode
  TRNG0->CONTROL &= ~TRNG_CONTROL_TESTEN;

  // bypass conditioning function
  TRNG0->CONTROL |= TRNG_CONTROL_CONDBYPASS_BYPASS;

  // enable trng
  TRNG0->CONTROL |= TRNG_CONTROL_ENABLE;

  printf("====================================================\r\n");
  printf("Entropy Source Checking using repetition, 64-sample,"
         "\r\n\tand 4096-sample\r\n");
  printf("====================================================\r\n");
  printf("Dumping test data:\r\n");

  // keep reading and discarding until 257 32-bit words have been read
  static uint16_t count = 257;
  while (count > 0) {
    // wait until full
    while (TRNG0->FIFOLEVEL < 64) {}

    // Read from Fifo
    if (TRNG0->FIFOLEVEL > 0) {
      printf(" 0x%08lX ", __REV(TRNG0->FIFO));
      --count;
      if (((257 - count) % 4) == 0) {
        printf("\r\n");
      }
    }
  }

  printf("\r\n\r\n");
  printf("%-40s: 0x%01lX\r\n", "AIS31 Noise Alarm",
         (TRNG0->STATUS & _TRNG_STATUS_ALMIF_MASK) >> _TRNG_STATUS_ALMIF_SHIFT);
  printf("%-40s: 0x%01lX\r\n", "AIS31 Preliminary Noise Alarm",
         (TRNG0->STATUS & _TRNG_STATUS_PREIF_MASK) >> _TRNG_STATUS_PREIF_SHIFT);
  printf("%-40s: 0x%01lX\r\n", "4096-sample Adaptive Proportion Test",
         (TRNG0->STATUS & _TRNG_STATUS_APT4096IF_MASK)
         >> _TRNG_STATUS_APT4096IF_SHIFT);
  printf("%-40s: 0x%01lX\r\n", "64-sample Adaptive Proportion Test",
         (TRNG0->STATUS & _TRNG_STATUS_APT64IF_MASK)
         >> _TRNG_STATUS_APT64IF_SHIFT);
  printf("%-40s: 0x%01lX\r\n", "Repetition Count Test",
         (TRNG0->STATUS & _TRNG_STATUS_REPCOUNTIF_MASK)
         >> _TRNG_STATUS_REPCOUNTIF_SHIFT);

  printf("====================================================\r\n");

  // Check status register for any error flags.
  if (TRNG0->STATUS & ~TRNG_STATUS_FULLIF) {
    printf("0x%08lX\r\n", TRNG0->STATUS);
    return false;
  }

  return true;
}
