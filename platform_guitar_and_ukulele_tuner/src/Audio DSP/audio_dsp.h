/***************************************************************************//**
* @file  audio_dsp.h
* @brief DSP functions for the PDM microphone audio analysis (Header)
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

#ifndef AUDIO_DSP_H_
#define AUDIO_DSP_H_

/********************************//**
 * DSP define macros
 ********************************/
// Size of the FFT
// Valid entries for FFT_SIZE are 32, 64, 128, 256, 512, 1024, 2048, 4096 and should be enough to cover the buffer size
#define FFT_SIZE    512

//Number of frequency bins to ignore when getting the highest magnitude bin
#define BIN_OFFSET  2


/********************************//**
 * Function prototypes
 ********************************/
void DSP_InitFFT_fast(void);
float DSP_AnalyzeData(float *dataBuffer, float *binMagBuffer, float fft_res);

// Utility functions
void Init_hanning_coef(void);

#endif /* AUDIO_DSP_H_ */
