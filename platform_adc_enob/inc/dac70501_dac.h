/***************************************************************************//**
 * @file dac70501_dac.h
 * @brief dac70501 i2c driver header file.
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
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
 * # Evaluation Quality
 * This code has been minimally tested to ensure that it builds and is suitable
 * as a demonstration for evaluation purposes only. This code will be maintained
 * at the sole discretion of Silicon Labs.
 ******************************************************************************/
#ifndef DAC70501_DAC_H
#define DAC70501_DAC_H

#ifdef __cplusplus
extern "C" {
#endif
uint16_t dac70501_init(void);                    // dac70501 initialization
float dac70501_read_ref(void);                   // dac70501 voltage read
uint16_t dac70501_set_ref(uint8_t dacValueHigh,  // dac70501 register set
                          uint8_t dacValueLow);
uint16_t dac70501_set_volt(float voltValue);     // dac70501 voltage set
uint16_t dac070501_power_down(uint8_t dac_pwdwn, // dac70501 power down
                              uint8_t ref_pwdwn);
uint16_t dac70501_restart(void);                 // dac70501 powerup(restart)

#ifdef __cplusplus
}
#endif

#endif /* DAC70501_DAC_H */
