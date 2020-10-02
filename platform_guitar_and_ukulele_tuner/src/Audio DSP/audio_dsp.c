/***************************************************************************//**
* @file  audio_dsp.c
* @brief DSP functions for the PDM microphone audio analysis (Source)
*******************************************************************************
* # License
* <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
*******************************************************************************
*
* SPDX-License-Identifier: Zlib
*
* The licensor of this software is Silicon Laboratories Inc.
*
* This software is provided \'as-is\', without any express or implied
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

#include "arm_math.h"
#include "em_core.h"
#include "audio_dsp.h"
#include <math.h>

/********************************//**
 * Global variables
 ********************************/
// Instance structure for float32_t RFFT fast
arm_rfft_fast_instance_f32 rfft_fast_instance;

// Error status structure
arm_status status;

// Temporary buffer to store complex frequency data
static float tempBuffer[FFT_SIZE];

// Hanning window coefficients buffer
static float Hann_coeff[FFT_SIZE];

/********************************//**
 * Static function prototypes
 ********************************/
static uint32_t Check_2nd_harmonic(float *binBuffer, uint32_t bin_index, float maxval);

/**************************************************************************//**
 * @name: DSP_InitFFT_fast
 *
 * @brief Initialize CMSIS ARM FFT instance
 *
 * @param[in]: none
 *
 * @return: none
 *****************************************************************************/
void DSP_InitFFT_fast(void)
{
  status = arm_rfft_fast_init_f32(&rfft_fast_instance, FFT_SIZE);

  while (status != ARM_MATH_SUCCESS) {
  };
}

/**************************************************************************//**
 * @name: DSP_AnalyzeData
 *
 * @brief Perform FFT and extract signal frequency content
 *
 * @param[in]
 * 		databuffer: Float pointer to time domain data
 * 		freqBuffer: Float buffer to store frequency bin magnitudes
 *
 * @return
 * 		bin_index: Index of the maximum magnitud bin
 *****************************************************************************/
float DSP_AnalyzeData(float *dataBuffer, float *binMagBuffer, float fft_res)
{
  float    maxval;
  uint16_t counter;
  uint32_t bin_index;
  float    dataBuffer_cpy[FFT_SIZE];

  // Generate a copy of the original data
  memcpy(dataBuffer_cpy, dataBuffer, sizeof(float)*FFT_SIZE);

  // Apply Hanning window to received data
  for (counter = 0; counter < FFT_SIZE; counter++) {
    dataBuffer_cpy[counter] *= Hann_coeff[counter];
  }

  // Perform FFT and get the frequency bins magnitude
  // Note: First 2 elements of FFT are real components
  //       out[0] = DC offset
  //       out[1] = Real component of N/2
  arm_rfft_fast_f32(&rfft_fast_instance, dataBuffer_cpy, tempBuffer, 0);

  // Calculate the magnitude of the frequency bins
  arm_cmplx_mag_f32(tempBuffer+2, binMagBuffer+1, FFT_SIZE/2-1);

  // Handle special cases: out[0] and out[1]
  binMagBuffer[0] = fabsf(tempBuffer[0]);
  binMagBuffer[(FFT_SIZE/2)-1] = fabsf(tempBuffer[1]);

  // Get the highest magnitude bin
  //   Bin offset is used to ignore the initial BIN_OFFSET bins as high frequency values may be observed
  arm_max_f32(&binMagBuffer[BIN_OFFSET], (FFT_SIZE/2)-BIN_OFFSET, &maxval, &bin_index);
  bin_index += BIN_OFFSET;

  // Check if a second harmonic is detected
  //   Verify only for frequencies below below ~200 Hz (Harmonic behaviour was predominantly observed here)
  if (bin_index < (float)200/fft_res) {
    bin_index = Check_2nd_harmonic(binMagBuffer, bin_index, maxval);
  }

  return ((float)bin_index * fft_res);
}

/**************************************************************************//**
 * @name: Init_hanning_coef
 *
 * @brief Generate a series of Hanning coefficient used to apply a smoothing window
 *        on the data before the FFT calculation. This reduces issues when the input signal
 *        doesn't represent a perfect integer number of periods
 *        The formula for the coefficients is as follows:
 *
 *        w(i) = 0.5 * (1 - cos(2pi*(n/(L-1)))), where n is the index number and L is the number of samples
 *
 * @param[in]
 * 		fft_Size: Number of samples to be processed for the FFT calculation
 *****************************************************************************/
void Init_hanning_coef()
{
  uint16_t count = 0;

  for (count = 0; count < FFT_SIZE; count++) {
    Hann_coeff[count] = 0.5 - (0.5*arm_cos_f32(2*PI*((float)count/((float)FFT_SIZE-1))));
  }
}

/**************************************************************************//**
 * @name: Check_2nd_harmonic
 *
 * @brief Compare the magnitude of the bin actual signal bin in that of half
 *        the frequency. If the half-frequency bin magnitude is at least 5% of the
 *        original input signal magnitude then the original signal is considered
 *        to be a harmonic and the function returns the new bin index.
 *
 * @param[in]
 * 		binBuffer: Pointer to the buffer holding the bin magnitudes
 * 		original_bin_index: The index of the bin with the highest magnitude in the binBuffer
 * 		maxval: The magnitude of the bin_index
 *
 * @return
 * 		bin_index: Index of the maximum magnitude bin
 *****************************************************************************/
static uint32_t Check_2nd_harmonic(float *binBuffer, uint32_t original_bin_index, float maxval)
{
  uint32_t half_frequency_index = original_bin_index >> 1; // Equivalent to dividing the number by 2
  float half_frequency_maxval;

  // Get the half frequency magnitude
  half_frequency_maxval = binBuffer[half_frequency_index];

  // Return the appropriate bin index
  return ((half_frequency_maxval*100)/maxval > 10
		  ? half_frequency_index
	      : original_bin_index);
}
