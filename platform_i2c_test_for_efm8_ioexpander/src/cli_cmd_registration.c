/***************************************************************************//**
 * @file cli_cmd_registration.c
 * @brief CLI command registration
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
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

#include "sl_cli.h"
#include "sl_cli_handles.h"
#include "cli_cmd_function.h"
#include "cli_cmd_registration.h"

static const sl_cli_command_info_t cmd_loop = \
  SL_CLI_COMMAND(cli_cmd_loop_callback,
                 "b* Excute commands repeatedly",
                 "" SL_CLI_UNIT_SEPARATOR,
                 { SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd_addr = \
  SL_CLI_COMMAND(cli_cmd_get_set_addr_callback,
                 "w* Get/Set I2C slave address",
                 "",
                 { SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd_read = \
  SL_CLI_COMMAND(cli_cmd_read_reg_callback,
                 "w* Read register from IOEXP.",
                 "",
                 { SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END });

static const sl_cli_command_info_t cmd_write = \
  SL_CLI_COMMAND(cli_cmd_write_reg_callback,
                 "w* Write register from IOEXP",
                 "",
                 { SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END });

static const sl_cli_command_info_t cmd_mod = \
  SL_CLI_COMMAND(cli_cmd_mod_reg_callback,
                 "b* Read Modify Write a register of IOEXP.",
                 "",
                 { SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END });

static const sl_cli_command_info_t cmd_sleep = \
  SL_CLI_COMMAND(cli_cmd_sleep_callback,
                 "w* Set a waiting time period (msec).",
                 "",
                 { SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END });

static const sl_cli_command_info_t cmd_time = \
  SL_CLI_COMMAND(cli_cmd_get_current_time_callback,
                 "Get current system time",
                 "",
                 { SL_CLI_ARG_END });

static const sl_cli_command_info_t cmd_ver = \
  SL_CLI_COMMAND(cli_cmd_get_version_callback,
                 "Get build information.",
                 "",
                 { SL_CLI_ARG_END });

static sl_cli_command_entry_t commands[] = {
  { "loop", &cmd_loop, false },
  { "addr", &cmd_addr, false },
  { "read", &cmd_read, false },
  { "write", &cmd_write, false },
  { "mod", &cmd_mod, false },
  { "sleep", &cmd_sleep, false },
  { "time", &cmd_time, false },
  { "ver", &cmd_ver, false },
  { NULL, NULL, false },
};

static sl_cli_command_group_t cmd_group = {
  { NULL },
  false,
  commands
};

void cli_register_commands(void)
{
  sl_cli_command_add_command_group(sl_cli_inst_handle, &cmd_group);
}
