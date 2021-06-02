/***************************************************************************//**
 * @file
 * @brief Bootloader I2C communication tester.
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
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

#define _BSD_SOURCE 1 /* for glibc <= 2.19 */
#define _DEFAULT_SOURCE 1 /* for glibc >= 2.19 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include "btl_i2c_communication.h"
#include "i2c-protocol-prototypes.h"
#include "version.h"
#include <time.h>

///all command function using the standard argc/argv format
typedef void (*command_function)(int argc, char *argv[]);

///forwarding structure def
typedef struct COMMAND_TABLE_ITEM command_table_item_t;

///command might be a function or (sub-)table
typedef union {
  command_function fx;
  command_table_item_t *table;
} command_p;

///command type
typedef enum {
  function,
  table,
  /// End Of Table
  EOT
} command_type;

///an item of a command table
typedef struct COMMAND_TABLE_ITEM {
  /// type
  command_type type;
  /// command id in the command line, for example 'B'
  char command_id;
  /// command value
  command_p command;
} command_table_item_t;

///special end of table descriptor
#define END_OF_TABLE {.type = EOT, .command_id = 0, .command.fx = NULL}

/// command prototypes
void cmd_print_boot_version(int argc, char *argv[]);
void cmd_download_file(int argc, char *argv[]);
void cmd_boot_application(int argc, char *argv[]);
void cmd_verify_application(int argc, char *argv[]);
void cmd_print_help(int argc, char *argv[]);

/// main command table
static command_table_item_t main_command_table[] = {
   {.type = function, .command_id = 'G', .command.fx = cmd_download_file},
   {.type = function, .command_id = 'B', .command.fx = cmd_boot_application},
   {.type = function, .command_id = 'V', .command.fx = cmd_verify_application},
   {.type = function, .command_id = 'I', .command.fx = cmd_print_boot_version},
   {.type = function, .command_id = 'H', .command.fx = cmd_print_help},
   {.type = function, .command_id = 'h', .command.fx = cmd_print_help},
   END_OF_TABLE
};

/// this holds the I2C handle which is opened
static int i2c_handle = 0;
/// I2C bus device id from command line
static unsigned long bus;
/// I2C slave address from command line
static int address;

/***************************************************************************//**
* @brief prints the usage
*******************************************************************************/
void print_usage()
{
  fprintf(stderr, "i2c-tester %s\n"
          "Usage:\n"
          "i2c-tester BUS I2CADDR CMD [command opts]\n\n"
          " BUS: the bus number\n"
          " I2CADDR: must between the 7-bit address space (0x00...0x7F)\n"
          " CMD:\n"
          "    G file   - download file\n"
          "    I        - show boot version\n"
          "    V        - verify application\n"
          "    B        - Boot application\n"
          "    (H or h) - this help\n", VERSION_STR
          );
}

/***************************************************************************//**
* @brief prints the version info from the slave
* @param address slave address
* @return 0 on success
*******************************************************************************/
int print_bootloader_version_info(int address)
{
  btl_version_info_t version_info;
  int status;
  status = get_version_info(i2c_handle, address, &version_info);
  if (status) {
    fprintf(stderr, "Unable to get bootloader version!\n");
    return status;
  }
  fprintf(stderr, "Bootloader version : %d.%d.%d\n",
          version_info.boot_version.major,
          version_info.boot_version.minor,
          version_info.boot_version.patch);
  return 0;
}

/***************************************************************************//**
* @brief lookup and call the command based on command line. Exits when error
*        detected.
*******************************************************************************/
void call_command(int argc, char *argv[])
{
  command_table_item_t *table_item = main_command_table;
  while (1) {
    if (argv[0][1] != '\0' ||
        table_item->type == EOT) {
        print_usage(),
        exit(1);
    }
    if (table_item->command_id == argv[0][0]) {
      switch (table_item->type )  {
        case function :
          table_item->command.fx(argc, argv);
          break;
        case table :
          table_item = table_item->command.table;
          if (argc == 0) {
            print_usage();
            exit(1);
          }
          argc--; argv++;
          break;
        default:
          break;
      }
    } else {
      table_item++;
    }
  }
  print_usage();
  exit(1);
}

/***************************************************************************//**
* @brief main function
* @return 0 on success
*******************************************************************************/
int main(int argc, char *argv[])
{
  const char *value;
  char *parse_end;
  srand(time(NULL));
  if (argc <4)
  {
    print_usage();
    exit(1);
  }
  argc--; argv++;
  value = *argv;
  bus = strtoul(value, &parse_end, 0);
  if (*parse_end || !*value) {
    print_usage();
    exit(1);
  }
  argc--; argv++;
  value = *argv;
  address = strtol(value, &parse_end, 0);
  fprintf(stderr, "bus: %lX, address: %X\n", bus, address);
  if (*parse_end || !*value ||
      address < 0 || address > 0x7F) {
    print_usage();
    exit(1);
  }
  argc--; argv++;
  call_command(argc, argv);
}

/***************************************************************************//**
* @brief prints the boot version
*******************************************************************************/
void cmd_print_boot_version(int argc, char *argv[])
{
  int result;
  i2c_handle = open_i2c(bus);
  if (i2c_handle <0 ) {
    fprintf(stderr, "Error: Cannot open i2c bus %lx\n", bus);
    exit(1);
  }
  result = print_bootloader_version_info(address);
  close_i2c(i2c_handle);
  exit(result);
}

/***************************************************************************//**
* @brief download the GBL file to the slave
*******************************************************************************/
void cmd_download_file(int argc, char *argv[])
{
  int result = 0;
  if (argc<2) {
    print_usage();
    exit(1);
  }
  i2c_handle = open_i2c(bus);
  if (i2c_handle <0 ) {
    fprintf(stderr, "Error: Cannot open i2c bus %lx\n", bus);
    exit(1);
  }
  print_bootloader_version_info(address);
  result = download_gbl_file(i2c_handle, address, argv[1]);
  if (result) {
    fprintf(stderr, "\nDownload failed (%d)\n", result);
  } else {
    fprintf(stderr, "\nDownload success\n");
  }
  close_i2c(i2c_handle);
  exit(0);
}

/***************************************************************************//**
* @brief boots the application on slave
*******************************************************************************/
void cmd_boot_application(int argc, char *argv[])
{
  int result =0;
  i2c_handle = open_i2c(bus);
  if (i2c_handle <0 ) {
    fprintf(stderr, "Error: Cannot open i2c bus %lx\n", bus);
    exit(1);
  }
  print_bootloader_version_info(address);
  result = boot_application(i2c_handle, address, -1);
  if (!result) {
   fprintf(stderr, "boot! \n");
  }
  close_i2c(i2c_handle);
  exit(result);
}

/***************************************************************************//**
* @brief verify application on slave
*******************************************************************************/
void cmd_verify_application(int argc, char *argv[])
{
  i2c_handle = open_i2c(bus);
  int result = 0;
  int status;
  uint8_t verifyResult;
  if (i2c_handle <0 ) {
    fprintf(stderr, "Error: Cannot open i2c bus %lx\n", bus);
    exit(1);
  }
  status = verify_app(i2c_handle, address, &verifyResult);
  if (status) {
    result = status;
  } else if (verifyResult == BOOT_REPLY_OK) {
    fprintf(stderr, "Application verify OK!\n");
  } else {
    fprintf(stderr, "Application verify FAIL (%02X)!\n", verifyResult);
    result = 1;
  }
  close_i2c(i2c_handle);
  exit(result);
}

/***************************************************************************//**
* @brief prints the usage
*******************************************************************************/
void cmd_print_help(int argc, char *argv[])
{
  print_usage();
  exit(0);
}

