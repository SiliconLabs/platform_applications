/***************************************************************************//**
 * @cli_cmd_function.h
 * @brief Test application for evaluation of EFM8-IOExpander (see AN1304)
 * @version 0.0.1
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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

#ifndef CLI_CMD_FUNCTION_H_
#define CLI_CMD_FUNCTION_H_

#include "sl_cli.h"
#define CTRL_C                                  0x03
#define CLI_IOEXP_OUTPUT_CMD_SEL                0x04

void cli_cmd_loop_callback(sl_cli_command_arg_t *arguments);
void cli_cmd_get_set_addr_callback(sl_cli_command_arg_t *arguments);
void cli_cmd_read_reg_callback(sl_cli_command_arg_t *arguments);
void cli_cmd_write_reg_callback(sl_cli_command_arg_t *arguments);
void cli_cmd_mod_reg_callback(sl_cli_command_arg_t *arguments);
void cli_cmd_sleep_callback(sl_cli_command_arg_t *arguments);
void cli_cmd_get_current_time_callback(sl_cli_command_arg_t *arguments);
void cli_cmd_get_version_callback(sl_cli_command_arg_t *arguments);
int cli_process_input_char_task(void);

#endif /* CLI_CMD_FUNCTION_H_ */
