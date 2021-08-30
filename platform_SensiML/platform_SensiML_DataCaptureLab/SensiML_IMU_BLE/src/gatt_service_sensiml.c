/***************************************************************************//**
 * @file
 * @brief Top level application functions
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
#include "em_common.h"
#include "sl_status.h"
#include "gatt_db.h"
#include "app_assert.h"
#include "app_log.h"
#include "gatt_service_sensiml.h"
#include "sl_imu.h"

// -----------------------------------------------------------------------------
// Private macros

// Max sample rate supported by the ICM-20648 IMU is 1125 samples/s
#ifndef IMU_SAMPLE_RATE
#define IMU_SAMPLE_RATE                     102
#endif
#ifndef IMU_SAMPLES_PER_PACKET
#define IMU_SAMPLES_PER_PACKET              2
#endif

#define STR(s) #s
#define XSTR(s) STR(s)
static const char config_str[] = "{"
  "\"sample_rate\":" XSTR(IMU_SAMPLE_RATE) ","
  "\"samples_per_packet\":" XSTR(IMU_SAMPLES_PER_PACKET) ","
  "\"column_location\":{"
    "\"AccelerometerX\":0,"
    "\"AccelerometerY\":1,"
    "\"AccelerometerZ\":2,"
    "\"GyroscopeX\":3,"
    "\"GyroscopeY\":4,"
    "\"GyroscopeZ\":5"
  "}"
"}";

static uint8_t imu_connection = 0;
static bool imu_state = false; /* disabled / enabled */
static volatile bool imu_data_notification = false;
static int16_t data[6*IMU_SAMPLES_PER_PACKET] = {0};
static int current_sample = 0;

// -----------------------------------------------------------------------------
// Private function declarations

static void imu_update_state(void);
static void imu_data_notify(void);
static void imu_connection_closed_cb(sl_bt_evt_connection_closed_t *data);
static void imu_char_config_changed_cb(sl_bt_evt_gatt_server_characteristic_status_t *data);

// -----------------------------------------------------------------------------
// Private function definitions
static void imu_update_state(void)
{
  bool imu_state_old = imu_state;
  imu_state = imu_data_notification;
  if (imu_state_old != imu_state) {
    gatt_service_sensiml_imu_enable(imu_state);
  }
}

static void imu_data_notify(void)
{
  sl_status_t sc;
  sc = sl_bt_gatt_server_send_notification(
    imu_connection,
    gattdb_sensiml_data,
    sizeof(data),
    (uint8_t*)data);
  if (sc != SL_STATUS_OK) {
    app_log_error("[E: 0x%04x] Failed to send characteristic notification\n", (int)sc);
  }
}

static void imu_connection_closed_cb(sl_bt_evt_connection_closed_t *data)
{
  (void)data;
  imu_data_notification = false;
  imu_update_state();
}

static void imu_char_config_changed_cb(sl_bt_evt_gatt_server_characteristic_status_t *data)
{
  bool enable = gatt_disable != data->client_config_flags;
  imu_connection = data->connection;
  // update notification status
  switch (data->characteristic) {
    case gattdb_sensiml_data:
      imu_data_notification = enable;
      break;
    default:
      app_assert(false, "Unexpected characteristic\n");
      break;
  }
  imu_update_state();
}

void gatt_service_sensiml_imu_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  // Handle stack events
  switch (SL_BT_MSG_ID(evt->header)) {
    case sl_bt_evt_system_boot_id:
      // Setup config characteristic
      sc = sl_bt_gatt_server_write_attribute_value(
        gattdb_sensiml_config,
        0,
        sizeof(config_str),
        (const uint8_t *)config_str
      );
      app_assert_status(sc);
      break;
    case sl_bt_evt_connection_closed_id:
      imu_connection_closed_cb(&evt->data.evt_connection_closed);
      break;
    case sl_bt_evt_gatt_server_characteristic_status_id:
      if ((gatt_server_client_config == (gatt_server_characteristic_status_flag_t)evt->data.evt_gatt_server_characteristic_status.status_flags)
          && ((gattdb_sensiml_data == evt->data.evt_gatt_server_user_read_request.characteristic))) {
        // client characteristic configuration changed by remote GATT client
        imu_char_config_changed_cb(&evt->data.evt_gatt_server_characteristic_status);
      }
      break;
  }
}

void gatt_service_sensiml_imu_step(void)
{
  if (imu_state) {
    if (SL_STATUS_OK == gatt_service_sensiml_imu_get(&data[6 * current_sample])) {
      current_sample++;
      if (current_sample == IMU_SAMPLES_PER_PACKET) {
        current_sample = 0;
        if (imu_data_notification) {
          imu_data_notify();
        }
      }
    }
  }
}

sl_status_t gatt_service_sensiml_imu_get(int16_t data[6])
{
  sl_imu_update();
  if (!sl_imu_is_data_ready()) {
    return SL_STATUS_NOT_READY;
  }
  sl_imu_get_acceleration(data);
  sl_imu_get_orientation(&data[3]);

  return SL_STATUS_OK;
}

void gatt_service_sensiml_imu_enable(bool enable)
{
  if (enable) {
    sl_imu_init();
    sl_imu_configure(IMU_SAMPLE_RATE);
  } else {
    sl_imu_reset();
    sl_imu_deinit();
  }
}

bool app_is_ok_to_sleep(void)
{
  return !imu_state;
}
