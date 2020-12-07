/***************************************************************************//**
 * @cli.c
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

#include "cli.h"
#include "ioexp_drv.h"

#include "bsp.h"
#include "command_interpreter.h"
#include "rail_types.h"
#include "rail.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

/***************************************************************************//**
 * Global constants
 ******************************************************************************/
#define ONE_MILLISECOND 1000

const char buildDateTime[] = __DATE__ " " __TIME__;

/***************************************************************************//**
 * Function prototypes
 ******************************************************************************/
static void cli_cmd_loop_callback(int argc, char **argv);
static void cli_cmd_get_set_addr_callback(int argc, char **argv);
static void cli_cmd_read_reg_callback(int argc, char **argv);
static void cli_cmd_write_reg_callback(int argc, char **argv);
static void cli_cmd_mod_reg_callback(int argc, char **argv);
static void cli_cmd_sleep_callback(int argc, char **argv);
static void cli_cmd_get_current_time_callback(int argc, char **argv);
static void cli_cmd_get_version_callback(int argc, char **argv);

/***************************************************************************//**
 * Global variables
 ******************************************************************************/
static CommandEntry_t commands[] = {
  COMMAND_ENTRY("loop", "b*", cli_cmd_loop_callback, "Execute commands repeatedly."),
  COMMAND_ENTRY("addr", "w*", cli_cmd_get_set_addr_callback, "Get/Set I2C slave address."),
  COMMAND_ENTRY("read", "w*", cli_cmd_read_reg_callback, "Read register from IOEXP."),
  COMMAND_ENTRY("write", "w*", cli_cmd_write_reg_callback, "Write register of IOEXP."),
  COMMAND_ENTRY("mod", "b*", cli_cmd_mod_reg_callback, "Read Modify Write a register of IOEXP."),
  COMMAND_ENTRY("sleep", "w*", cli_cmd_sleep_callback, "Set a waiting time period (msec)."),
  COMMAND_ENTRY("time", "", cli_cmd_get_current_time_callback, "Get current system time."),
  COMMAND_ENTRY("ver", "", cli_cmd_get_version_callback, "Get build information."),
  COMMAND_ENTRY(NULL, NULL, NULL, NULL)
};
static CommandState_t ciState;
#define APP_COMMAND_INTERFACE_BUFFER_SIZE 256
static char ciBuffer[APP_COMMAND_INTERFACE_BUFFER_SIZE];

#define CLI_CB_CALL_AND_SETUP(cb) {\
    if (NULL != cli_callback) cli_callback(i - arg_0, &(argv[arg_0])); \
    arg_0 = i; \
    cli_callback = (cb);}


/***************************************************************************//**
 * @brief
 *    Initializes CLI.
 ******************************************************************************/
void cli_process_init(void)
{
  printf("> ");
  ciInitState(&ciState, ciBuffer, sizeof(ciBuffer), commands);
}

/***************************************************************************//**
 * @brief
 *    Checks input and starts recognizing and executing commands.
 *
 * @return
 *    Returns -1 if CTRL-C pressed (exit condition for endless loop purpose) or
 *    zero on all other cases
 ******************************************************************************/
int cli_process_input_char_task(void)
{
  char input = getchar();

  if (input == CTRL_C) {
    return -1;
  }
  if (input != '\0' && input != 0xFF) {
    if (input != '\n') {
      printf("%c", input);
      if (input == '\r') { // retargetserial no longer does CR => CRLF
	  printf("\n");
      }
    }
    if (ciProcessInput(&ciState, &input, 1) > 0) {
      printf("> ");
    }
  }
  return 0;
}

/******************************************************************************
 * Command Interpreter Callback Functions
 *****************************************************************************/

/***************************************************************************//**
 * @brief
 *    Executes commands repeatedly (gets commands as parameters)
 *
 * @param[in] argc
 *    The number of arguments passed to argument vector
 *
 * @param[in] argv
 *    The argument vector
 *
 ******************************************************************************/
static void cli_cmd_loop_callback(int argc, char **argv)
{
  int i, l, loop_max;
  int arg_0;
  void (*cli_callback)(int, char **) = NULL;

  if (argc > 2)
  {
    l = loop_max = ciGetUnsigned(argv[1]);
    while (!cli_process_input_char_task() && (l || (!loop_max && !l)))
    {
      arg_0 = 0; cli_callback = NULL;
      for ( i = 2; i < argc; i++)
      {
        if (0 == strncmp(argv[i], "addr", 1)) CLI_CB_CALL_AND_SETUP(cli_cmd_get_set_addr_callback)
        else if (0 == strncmp(argv[i], "read", 1)) CLI_CB_CALL_AND_SETUP(cli_cmd_read_reg_callback)
        else if (0 == strncmp(argv[i], "write", 1)) CLI_CB_CALL_AND_SETUP(cli_cmd_write_reg_callback)
        else if (0 == strncmp(argv[i], "mod", 1)) CLI_CB_CALL_AND_SETUP(cli_cmd_mod_reg_callback)
        else if (0 == strncmp(argv[i], "sleep", 1)) CLI_CB_CALL_AND_SETUP(cli_cmd_sleep_callback)
        else {}
      }
      if (NULL != cli_callback) cli_callback(i - arg_0, &(argv[arg_0]));
      if (l) l--;
    }
  }
}

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
static void cli_cmd_get_set_addr_callback(int argc, char **argv)
{
  uint16_t addr;

  if (argc > 1) {
      addr = ciGetUnsigned(argv[1]);
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
static void cli_cmd_read_reg_callback(int argc, char **argv)
{
  uint8_t reg = ciGetUnsigned(argv[1]);
  uint8_t len = 1;
  uint8_t data[32];
  uint8_t i;

  if (argc > 2) {
      len = ciGetUnsigned(argv[2]);
  }

  if (IOEXP_STATUS_OK == ioexp_read_regs(reg, len, data)) {
    printf("Read register [0x%02X]:", reg);
    for (i = 0; i < len; i++) {
	printf(" 0x%02X ('%c'),", *(data+i), *(data+i));
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
static void cli_cmd_write_reg_callback(int argc, char **argv)
{
  uint8_t reg = ciGetUnsigned(argv[1]);
  uint8_t len = argc - 2;
  uint8_t data[4];
  uint8_t i;

  for (i = 0; i < len; i++) {
      *(data+i) = ciGetUnsigned(argv[i + 2]);
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
static void cli_cmd_mod_reg_callback(int argc, char **argv)
{
  uint8_t reg, read_reg;
  uint8_t len = 1;
  uint8_t data, data0;

  if (argc < 4) {
      printf("Not enough parameter!\n");
      return;
  }

  reg = ciGetUnsigned(argv[1]);
  /* There are difference between reading back a port as input or output
   * While reading a port as input or writing it as output, a polarity mask is
   * involved to the calculation of its final value, until reading a port as
   * output gives back the raw port value without polarity mask. Therefore the
   * read-modify-write process will be correct only, if both reading and
   * writing use polarity! */
  read_reg = reg & ~(CLI_IOEXP_OUTPUT_CMD_SEL);

  if (IOEXP_STATUS_OK == ioexp_read_regs(read_reg, len, &data))
  {
    data0 = data; // keep original value for displaying
    if (0 == strncmp(argv[2], "+", 1)) data += ciGetUnsigned(argv[3]);
    else if (0 == strncmp(argv[2], "-", 1)) data -= ciGetUnsigned(argv[3]);
    else if (0 == strncmp(argv[2], "*", 1)) data *= ciGetUnsigned(argv[3]);
    else if (0 == strncmp(argv[2], "/", 1)) data /= ciGetUnsigned(argv[3]);
    else if (0 == strncmp(argv[2], "%", 1)) data %= ciGetUnsigned(argv[3]);
    else if (0 == strncmp(argv[2], "&", 1)) data &= ciGetUnsigned(argv[3]);
    else if (0 == strncmp(argv[2], "|", 1)) data |= ciGetUnsigned(argv[3]);
    else if (0 == strncmp(argv[2], "^", 1)) data ^= ciGetUnsigned(argv[3]);
    else if (0 == strncmp(argv[2], "<<", 2)) data <<= ciGetUnsigned(argv[3]);
    else if (0 == strncmp(argv[2], ">>", 2)) data >>= ciGetUnsigned(argv[3]);
    else {printf("Unknown operation!\nOnly the following arithmetic or bitwise operators are available:\n\t +,-,*,/,%%,&,|,^,<<,>> \n"); return;}
    if (IOEXP_STATUS_OK == ioexp_write_regs(reg, len, &data)) {
      printf("Modification of register [0x%02X]: 0x%02X %s= 0x%02X -> 0x%02X SUCCEDED\n", reg, data0, argv[2], (uint8_t)ciGetUnsigned(argv[3]), data);
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
static void cli_cmd_sleep_callback(int argc, char **argv)
{
  uint32_t msec = ciGetUnsigned(argv[1]);
  RAIL_Time_t endTime;
  endTime = RAIL_GetTime();

  endTime += msec * ONE_MILLISECOND;

  printf("Sleep for %lums...", msec);
  while ( endTime > RAIL_GetTime());
  printf("Done\n");
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
static void cli_cmd_get_current_time_callback(int argc, char **argv)
{
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
static void cli_cmd_get_version_callback(int argc, char **argv)
{
  printf("Build:%s\n", buildDateTime);
}

