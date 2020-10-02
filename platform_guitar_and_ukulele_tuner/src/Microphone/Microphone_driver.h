/***************************************************************************//**
* @file  microphone_driver.h
* @brief Driver for the SPK0838HT4H-B MEMS microphone (Header file)
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

#ifndef MICROPHONE_DRIVER_H_
#define MICROPHONE_DRIVER_H_

#include "Microphone_config.h"

/********************************//**
 * Function prototypes
 ********************************/
void Init_MIC(void);
void InitLDMA_MIC(uint32_t *bufferA, uint32_t *bufferB, uint32_t buffer_size);

void MIC_disable(void);

#endif /* MICROPHONE_DRIVER_H_ */
