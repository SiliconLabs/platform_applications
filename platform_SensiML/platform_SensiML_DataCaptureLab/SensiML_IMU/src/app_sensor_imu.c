/***************************************************************************//**
 * @file
 * @brief Inertial Measurement Unit sensor
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

#include <stdio.h>
#include "sl_board_control.h"
#include "sl_iostream.h"
#include "sl_imu.h"
#include "sl_app_assert.h"
#include "app_sensor_imu.h"
#include "sl_sleeptimer.h"
#include "ssi_comms.h"

/** Time (in ms) between periodic JSON template messages. */
#define JSON_TEMPLATE_INTERVAL_MS      1000

/** Samples per packet to send out
 * NOTE: a "Sample" when using ACC and Gyro is 12 bytes, or all 6 axes
 * */
#define APP_IMU_SAMPLES_PER_PACKET     10
#define APP_IMU_NUMBER_SENSORS         2
#define APP_IMU_AXES_PER_SENSOR        3
#define APP_IMU_BYTES_TO_WRITE (APP_IMU_SAMPLES_PER_PACKET * APP_IMU_AXES_PER_SENSOR * APP_IMU_NUMBER_SENSORS * sizeof(int16_t) )

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/
static volatile bool send_config_flag = true;
sl_sleeptimer_timer_handle_t send_config_timer;

/*******************************************************************************
 *********************   LOCAL FUNCTION PROTOTYPES   ***************************
 ******************************************************************************/

static float get_acc_gyro_odr(void);

static void send_config_callback(sl_sleeptimer_timer_handle_t *handle, void *data);

static void send_json_config(void);

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * Setup periodic timer for sending configuration messages.
 ******************************************************************************/
void app_config_imu(void)
{
  /* Set up periodic JSON configuration timer. */
  sl_sleeptimer_start_periodic_timer_ms(&send_config_timer, JSON_TEMPLATE_INTERVAL_MS, send_config_callback, NULL, 0, 0);

  // Send initial JSON config message
  send_json_config();
}

/***************************************************************************//**
 * JSON configuration message ticking function.
 ******************************************************************************/
void app_config_process_action(void)
{
  if (send_config_flag == true) {
    send_json_config();
    send_config_flag = false;
  }
}

/***************************************************************************//**
 * IMU ticking function.
 ******************************************************************************/
typedef struct
{
  int16_t acc_vector[APP_IMU_AXES_PER_SENSOR];
  int16_t orientation[APP_IMU_AXES_PER_SENSOR];
} imu_6_dof_data_t;

static imu_6_dof_data_t imu_data[APP_IMU_SAMPLES_PER_PACKET];
static uint8_t samples_collected = 0;

void app_sensor_imu_process_action(void)
{
  sl_status_t sc;

  unsigned char *p;

  sc = app_sensor_imu_get(imu_data[samples_collected].orientation, imu_data[samples_collected].acc_vector);

  if (sc == SL_STATUS_OK) {
      samples_collected++;
      if(samples_collected == APP_IMU_SAMPLES_PER_PACKET){
          samples_collected = 0;
          p = (unsigned char*) imu_data;
          // send data using SSI v2 on default Channel
          ssiv2_publish_sensor_data(SSI_CHANNEL_DEFAULT, p, APP_IMU_BYTES_TO_WRITE);
      }
  }
}

/***************************************************************************//**
 * Board control initialization of IMU.
 ******************************************************************************/
void app_sensor_imu_init(void)
{
  (void)sl_board_enable_sensor(SL_BOARD_SENSOR_IMU);
}

/***************************************************************************//**
 * Board control de-initialization of IMU.
 ******************************************************************************/
void app_sensor_imu_deinit(void)
{
  (void)sl_board_disable_sensor(SL_BOARD_SENSOR_IMU);
}

/***************************************************************************//**
 * IMU driver configuration and initialization.
 ******************************************************************************/
void app_sensor_imu_enable(bool enable)
{
  sl_status_t sc;
  uint8_t state = sl_imu_get_state();
  if (enable && (IMU_STATE_DISABLED == state)) {
    sc = sl_imu_init();
    sl_app_assert(sc == SL_STATUS_OK,
                  "[E: 0x%04x] IMU init failed\n",
                  (int)sc);
    sl_imu_configure(get_acc_gyro_odr());
  } else if (!enable && (IMU_STATE_READY == state)) {
    sl_imu_deinit();
  }
}

/***************************************************************************//**
 * IMU get latest accelerometer/gyroscope data.
 ******************************************************************************/
sl_status_t app_sensor_imu_get(int16_t ovec[3], int16_t avec[3])
{
  sl_status_t sc = SL_STATUS_NOT_READY;
  if (sl_imu_is_data_ready()) {
    sl_imu_update();
    sl_imu_get_orientation(ovec);
    sl_imu_get_acceleration(avec);
    sc = SL_STATUS_OK;
  }
  return sc;
}

/***************************************************************************//**
 * Get accelerometer/gyroscope output data rate - defined in app_sensor_imu.h.
 ******************************************************************************/
static float get_acc_gyro_odr(void)
{
    switch (ACCEL_GYRO_DEFAULT_ODR)
    {
        case ACCEL_GYRO_ODR_4p4HZ:
            return 4.4;
        case ACCEL_GYRO_ODR_17p6HZ:
            return 17.6;
        case ACCEL_GYRO_ODR_35p2HZ:
            return 35.2;
        case ACCEL_GYRO_ODR_48p9HZ:
            return 48.9;
        case ACCEL_GYRO_ODR_70p3HZ:
            return 70.3;
        case ACCEL_GYRO_ODR_102p3HZ:
            return 102.3;
        case ACCEL_GYRO_ODR_140p6HZ:
            return 140.6;
        case ACCEL_GYRO_ODR_187p5HZ:
            return 187.5;
        case ACCEL_GYRO_ODR_281p3HZ:
            return 281.3;
        case ACCEL_GYRO_ODR_562p5HZ:
            return 562.5;
        default:
            return 102.3;
    }
}

/***************************************************************************//**
 * JSON send configuration timeout callback.
 ******************************************************************************/
static void send_config_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void)handle;
  (void)data;
  send_config_flag = true;
}

/***************************************************************************//**
 * Sends JSON configuration over iostream.
 ******************************************************************************/
static void send_json_config()
{
#if (SSI_JSON_CONFIG_VERSION == 1)
  printf("{\"sample_rate\":%3.0f,"
    "\"samples_per_packet\":%d,"
    "\"column_location\":{"
    "\"AccelerometerX\":0,"
    "\"AccelerometerY\":1,"
    "\"AccelerometerZ\":2,"
    "\"GyroscopeX\":3,"
    "\"GyroscopeY\":4,"
    "\"GyroscopeZ\":5}}\n", get_acc_gyro_odr(), APP_IMU_SAMPLES_PER_PACKET);
#elif (SSI_JSON_CONFIG_VERSION == 2)
  printf("{\"version\":%d, \"sample_rate\":%3.0f,"
    "\"samples_per_packet\":%d,"
    "\"column_location\":{"
    "\"AccelerometerX\":0,"
    "\"AccelerometerY\":1,"
    "\"AccelerometerZ\":2,"
    "\"GyroscopeX\":3,"
    "\"GyroscopeY\":4,"
    "\"GyroscopeZ\":5}}\n", SSI_JSON_CONFIG_VERSION, get_acc_gyro_odr(), APP_IMU_SAMPLES_PER_PACKET);
#else
#error "Unknown SSI_JSON_CONFIG_VERSION"
#endif
}
