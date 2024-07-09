/***************************************************************************//**
 * @file cli_cmd_function.c
 * @brief cli command functions
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

#include "cli_cmd_function.h"
#include "ioexp_drv.h"
#include "sl_cli_handles.h"
#include "sl_cli_input.h"
#include "printf.h"
#include "rail.h"
#include "sl_rail_util_init.h"
#include "sli_cli_io.h"

#define ONE_MILLISECOND 1000

const char buildDateTime[] = __DATE__ " " __TIME__;

#define CLI_CB_CALL_AND_SETUP(cb) {      \
    if (NULL != sl_cli_command_func_t) { \
      sl_cli_command_func_t(arguments);  \
    }                                    \
    arguments->arg_ofs = i + 1;          \
    sl_cli_command_func_t = (cb); }

/***************************************************************************//**
 * @brief
 *    Gets/Sets I2C slave address
 *
 * @param[in] argc
 *    The number of arguments passed to argument vector
 *
 * @param[in] argv
 *    The argument vector
 *
 ******************************************************************************/
void cli_cmd_get_set_addr_callback(sl_cli_command_arg_t *arguments)
{
  uint16_t addr;
  if (arguments->argc > 1) {
    addr = atoi(sl_cli_get_argument_string(arguments, 0));
    ioexp_set_i2c_addr(addr);
  }
  printf("address: %d (%02X)\n", ioexp_get_i2c_addr(), ioexp_get_i2c_addr());
}

/***************************************************************************//**
 * @brief
 *    Reads one or more registers from IOEXP
 *
 * @param[in] argc
 *    The number of arguments passed to argument vector
 *
 * @param[in] argv
 *    The argument vector
 *
 ******************************************************************************/
void cli_cmd_read_reg_callback(sl_cli_command_arg_t *arguments)
{
  uint8_t reg = strtol(sl_cli_get_argument_string(arguments, 0),
                       NULL,
                       16);
  uint8_t len = 1;
  uint8_t data[32];
  uint8_t i;

  if (arguments->argc > 2) {
    len = atoi(sl_cli_get_argument_string(arguments, 1));
  } else {
    printf("Not enough parameter!\n");
    return;
  }
  if (IOEXP_STATUS_OK == ioexp_read_regs(reg, len, data)) {
    printf("Read register [0x%02X]:", reg);
    for (i = 0; i < len; i++) {
      printf(" 0x%02X ('%c'),", *(data + i), *(data + i));
    }
    printf("\n");
  } else {
    printf("Read register [0x%02X]: FAILED\n", reg);
  }
}

/***************************************************************************//**
 * @brief
 *    Writes one or more registers of IOEXP
 *
 * @param[in] argc
 *    The number of arguments passed to argument vector
 *
 * @param[in] argv
 *    The argument vector
 *
 ******************************************************************************/
void cli_cmd_write_reg_callback(sl_cli_command_arg_t *arguments)
{
  uint8_t reg = strtol(sl_cli_get_argument_string(arguments, 0),
                       NULL,
                       16);
  uint8_t len = arguments->argc - arguments->arg_ofs - 1;
  uint8_t data[4];
  uint8_t i;

  if (arguments->argc < 2) {
    printf("Not enough parameter!\n");
    return;
  }
  for (i = 0; i < len; i++) {
    *(data + i) = (uint8_t)strtol(sl_cli_get_argument_string(arguments, i + 1),
                                  NULL,
                                  16);
  }

  if (IOEXP_STATUS_OK == ioexp_write_regs(reg, len, data)) {
    printf("Write register [0x%02X]: SUCCEDED\n", reg);
  } else {
    printf("Write register [0x%02X]: FAILED\n", reg);
  }
}

/***************************************************************************//**
 * @brief
 *    Reads modifies writes a register of IOEXP
 *
 * @param[in] argc
 *    The number of arguments passed to argument vector
 *
 * @param[in] argv
 *    The argument vector
 *
 ******************************************************************************/
void cli_cmd_mod_reg_callback(sl_cli_command_arg_t *arguments)
{
  uint8_t reg, read_reg;
  uint8_t len = 1;
  uint8_t data, data0;

  if (arguments->argc < 4) {
    printf("Not enough parameter!\n");
    return;
  }

  reg = strtol(sl_cli_get_argument_string(arguments, 0), NULL, 16);

  /* There are difference between reading back a port as input or output
   * While reading a port as input or writing it as output, a polarity mask is
   * involved to the calculation of its final value, until reading a port as
   * output gives back the raw port value without polarity mask. Therefore the
   * read-modify-write process will be correct only, if both reading and
   * writing use polarity! */
  read_reg = reg & ~(CLI_IOEXP_OUTPUT_CMD_SEL);

  if (IOEXP_STATUS_OK == ioexp_read_regs(read_reg, len, &data)) {
    data0 = data; // keep original value for displaying
    if (0 == strncmp(arguments->argv[2], "+", 1)) {
      data += strtol(sl_cli_get_argument_string(arguments, 2), NULL, 16);
    } else if (0 == strncmp(arguments->argv[2], "-", 1)) {
      data -= strtol(sl_cli_get_argument_string(arguments, 2), NULL, 16);
    } else if (0 == strncmp(arguments->argv[2], "*", 1)) {
      data *= strtol(sl_cli_get_argument_string(arguments, 2), NULL, 16);
    } else if (0 == strncmp(arguments->argv[2], "/", 1)) {
      data /= strtol(sl_cli_get_argument_string(arguments, 2), NULL, 16);
    } else if (0 == strncmp(arguments->argv[2], "%", 1)) {
      data %= strtol(sl_cli_get_argument_string(arguments, 2), NULL, 16);
    } else if (0 == strncmp(arguments->argv[2], "&", 1)) {
      data &= strtol(sl_cli_get_argument_string(arguments, 2), NULL, 16);
    } else if (0 == strncmp(arguments->argv[2], "|", 1)) {
      data |= strtol(sl_cli_get_argument_string(arguments, 2), NULL, 16);
    } else if (0 == strncmp(arguments->argv[2], "^", 1)) {
      data ^= strtol(sl_cli_get_argument_string(arguments, 2), NULL, 16);
    } else if (0 == strncmp(arguments->argv[2], "<<", 2)) {
      data <<= strtol(sl_cli_get_argument_string(arguments, 2), NULL, 16);
    } else if (0 == strncmp(arguments->argv[2], ">>", 2)) {
      data >>= strtol(sl_cli_get_argument_string(arguments, 2), NULL, 16);
    } else {
      printf(
        "Unknown operation!\nOnly the following arithmetic or bitwise operators are available:\n\t +,-,*,/,%%,&,|,^,<<,>> \n");
      return;
    }
    if (IOEXP_STATUS_OK == ioexp_write_regs(reg, len, &data)) {
      printf(
        "Modification of register [0x%02X]: 0x%02X %s= 0x%02X -> 0x%02X SUCCEDED\n",
        reg,
        data0,
        (char *)arguments->argv[2],
        (uint8_t)strtol(sl_cli_get_argument_string(arguments, 2), NULL, 16),
        data);
    } else {
      printf("Write register [0x%02X]: FAILED\n", reg);
    }
  } else {
    printf("Read register [0x%02X]: FAILED\n", reg);
  }
}

/***************************************************************************//**
 * @brief
 *    Sets a waiting time period (msec)
 *
 * @param[in] argc
 *    The number of arguments passed to argument vector
 *
 * @param[in] argv
 *    The argument vector
 *
 ******************************************************************************/
void cli_cmd_sleep_callback(sl_cli_command_arg_t *arguments)
{
  uint32_t msec;
  msec = atoi(sl_cli_get_argument_string(arguments, 0));
  RAIL_Time_t endTime;
  endTime = RAIL_GetTime();
  endTime += msec * ONE_MILLISECOND;

  printf("Sleep for %lums...", msec);
  while (endTime > RAIL_GetTime()) {}
  printf(" => Done\n");
}

/***************************************************************************//**
 * @brief
 *    Gets current system time
 *
 * @param[in] argc
 *    The number of arguments passed to argument vector
 *
 * @param[in] argv
 *    The argument vector
 *
 ******************************************************************************/
void cli_cmd_get_current_time_callback(sl_cli_command_arg_t *arguments)
{
  (void)arguments;
  RAIL_Time_t sysTime;
  sysTime = RAIL_GetTime();
  printf("System time: %lu\n", (uint32_t)sysTime);
}

/***************************************************************************//**
 * @brief
 *    Gets build information
 *
 * @param[in] argc
 *    The number of arguments passed to argument vector
 *
 * @param[in] argv
 *    The argument vector
 *
 ******************************************************************************/
void cli_cmd_get_version_callback(sl_cli_command_arg_t *arguments)
{
  (void)arguments;
  printf("Build: %s\n", buildDateTime);
}

int cli_process_input_char_task(void)
{
  char input = getchar();

  if (input == CTRL_C) {
    return -1;
  }
  if ((input != '\0') && (input != 0xFF)) {
    if (sl_cli_input_char(sl_cli_inst_handle, input)) {
      printf("> ");
    }
  }

  return 0;
}

void cli_cmd_loop_callback(sl_cli_command_arg_t *arguments)
{
  int i, l, loop_max;
  void (*sl_cli_command_func_t)(sl_cli_command_arg_t *) = NULL;

  if (arguments->argc > 2) {
    l = loop_max = atoi(sl_cli_get_argument_string(arguments, 0));
    while (!cli_process_input_char_task() && (l || (!loop_max && !l)))
    {
      arguments->arg_ofs = 0; sl_cli_command_func_t = NULL;
      for ( i = 2; i < arguments->argc; i++)
      {
        if (0 == strncmp(arguments->argv[i], "addr", 1)) {
          CLI_CB_CALL_AND_SETUP(cli_cmd_get_set_addr_callback)
        } else if (0 == strncmp(arguments->argv[i], "read", 1)) {
          CLI_CB_CALL_AND_SETUP(cli_cmd_read_reg_callback)
        } else if (0 == strncmp(arguments->argv[i], "write", 1)) {
          CLI_CB_CALL_AND_SETUP(cli_cmd_write_reg_callback)
        } else if (0 == strncmp(arguments->argv[i], "mod", 1)) {
          CLI_CB_CALL_AND_SETUP(cli_cmd_mod_reg_callback)
        } else if (0 == strncmp(arguments->argv[i], "sleep", 1)) {
          CLI_CB_CALL_AND_SETUP(cli_cmd_sleep_callback)
        } else {
        }
      }
      if (NULL != sl_cli_command_func_t) {
        sl_cli_command_func_t(arguments);
      }
      if (l) {
        l--;
      }
    }
  }
}
