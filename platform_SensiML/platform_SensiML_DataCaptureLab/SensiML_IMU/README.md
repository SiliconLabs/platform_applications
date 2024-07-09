# Platform - SensiML IMU Data Capture #

![Type badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/SensiML_IMU_common.json&label=Type&query=type&color=green)
![Technology badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/SensiML_IMU_common.json&label=Technology&query=technology&color=green)
![License badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/SensiML_IMU_common.json&label=License&query=license&color=green)
![SDK badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/SensiML_IMU_common.json&label=SDK&query=sdk&color=green)
![Build badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/SensiML_IMU_build_status.json)
![Flash badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/SensiML_IMU_common.json&label=Flash&query=flash&color=blue)
![RAM badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/SensiML_IMU_common.json&label=RAM&query=ram&color=blue)

## Overview ##

This project uses the Thunderboard Sense 2 (EFR32MG12) and onboard IMU to measure accelerometer and gyroscope data. After that, the measurement data are transmitted via serial UART for consumption by the SensiML Data Studio application. The example project uses the I/O Stream service along with IMU component drivers running in a bare-metal configuration. Sensor data from the ICM-20648 IMU is transferred over a virtual COM port (VCOM) at a 921600 baud rate. The sensor data output data rate is configured at 102.3Hz.

## Gecko SDK version ##

- v4.4.3

## Software Required ##

- [SensiML Data Studio](https://sensiml.com/download/)

## Hardware Required ##

- Thunderboard Sense 2 Development Kit - SLTB004A

## Connections Required ##

- Connect the Kit to the PC through a micro USB cable.

## Setup ##

To test this application, you can either create a project based on an example project or start with an "Empty C Project" project based on your hardware.

### Create a project based on an example project ###

1. Make sure that this repository is added to [Preferences > Simplicity Studio > External Repos](https://docs.silabs.com/simplicity-studio-5-users-guide/latest/ss-5-users-guide-about-the-launcher/welcome-and-device-tabs).

2. From the Launcher Home, add your device to My Products, click on it, and click on the **EXAMPLE PROJECTS & DEMOS** tab. Find the example project filtering by **sensiml**.

3. Click the **Create** button on the **Platform - SensiML IMU Data Capture** example. Example project creation dialog pops up -> click **Finish** and Project should be generated.

    ![Create_example](image/create_project.png)

4. Build and flash this example to the board.

### Start with an "Empty C Project" project ###

1. Create an **Empty C Project** project for your hardware using Simplicity Studio 5.

2. Copy all attached files in *inc* and *src* folders into the project root folder (overwriting existing).

3. Open the .slcp file. Select the SOFTWARE COMPONENTS tab and install the software components:

   - [Platform] → [Driver] → [LED] → [Simple LED] → default instance name: **led0** and **led1**.
   - [Platform] → [Board Drivers] → [IMU - Inertial Measurement Unit]
   - [Platform] → [Board] → [Board Control] → Active "Enable Inertial Measurement Unit"
   - [Services] → [IO Stream] → [IO Stream: USART] → default instance name: **vcom** → Set Baud rate to 921600
   - [Services] → [IO Stream] → [IO Stream: Retarget STDIO]
   - [Application] → [Utility] → [Assert]
   - [Third Party] → [Tiny printf]

4. Build and flash the project to your device.

## How It Works ##

The application uses the process-action bare-metal project configuration model.

- First, the IO Stream/USART is configured for 8-bit, no parity, 1 stop bit, and 921600 baud rate. A periodic sleep timer is also configured with a 1-second interrupt.
- Within the application's process actions, the serial input is monitored for a connection command expected from SensiML Data Studio. A JSON configuration packet is sent via USART/VCOM to the PC once per second (via sleep timer) until the connection command is received, at which point the application halts monitoring the serial input and sending configuration information and the IMU is initialized and motion data sent to PC at the specified data rate (default setting is 102.3 Hz).
- The onboard LEDs are also used to indicate that the application is running (blinking green LED at 2 Hz) and when the application is waiting for the connection command (solid red LED). Once connected, the red LED will turn off. Upon disconnecting from SensiML Data Studio, the red LED will be turned on again and the device will resume sending configuration packets. The device can be reconnected without a reset.

The output data rate is hard-coded in the application in "app_sensor_imu.h" defined as ACCEL_GYRO_DEFAULT_ODR (line 60) using the enumeration "accel_gyro_odr_t" starting at line 44. A subset of IMU data sampling rates is made available through this enumeration. For more information, see the ICM-20648 datasheet available here - <https://invensense.tdk.com/download-pdf/icm-20648-datasheet/>

![Application flowchart - high-level system overview](image/app_flowchart.png)

The sensor sampling rate can be modified by changing the #define ACCEL_GYRO_DEFAULT_ODR in the source. When doing so, it is important for the user to also configure/re-configure the sensor settings within SensiML Data Studio to reflect these changes. The configurations must match for the data acquisition to operate correctly.

**Note:** The Thunderboard Sense 2 must be placed on a stable surface during initialization as the IMU undergoes a calibration routine at this time.  

## Testing ##

- Flash the project to your device. Make sure that the Thunderboard Sense 2 board is configured to change VCOM baud rate to 921600. For more detail, see [initial setup for flashing](https://sensiml.com/documentation/firmware/silicon-labs-thunderboard-sense-2/silicon-labs-thunderboard-sense-2.html#initial-setup-for-flashing).

- On your PC, run the SensiML Data Studio application. Connect to the Thunderboard Sense 2 board. You should expect a similar output to the one below.

![Application Data Capture](image/sensiml_data_capture_logs.png)
