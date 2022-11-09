# SensiML IMU Data Capture IMU with BLE Example #
![Type badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/SensiML_IMU_BLE_common.json&label=Type&query=type&color=green)
![Technology badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/SensiML_IMU_BLE_common.json&label=Technology&query=technology&color=green)
![License badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/SensiML_IMU_BLE_common.json&label=License&query=license&color=green)
![SDK badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/SensiML_IMU_BLE_common.json&label=SDK&query=sdk&color=green)
![Build badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/SensiML_IMU_BLE_build_status.json)
![Flash badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/SensiML_IMU_BLE_common.json&label=Flash&query=flash&color=blue)
![RAM badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/SensiML_IMU_BLE_common.json&label=RAM&query=ram&color=blue)

## Summary ##

This project uses the Thunderboard Sense 2 (EFR32MG12) and onboard IMU to measure accelerometer and gyroscope data and transmit the measurement data via Bluetooth Low Energy (BLE) for consumption by [SensiML's open-gateway](https://github.com/sensiml/open-gateway/). The example project use Bluetooth services along with IMU component drivers running in a bare-metal configuration. The sensor data output data rate is configured at 102 Hz.

## Gecko SDK version ##

v3.1

## Hardware Required ##

- One SLTB004A Thunderboard Sense 2 Development Kit
<https://www.silabs.com/development-tools/thunderboard/thunderboard-sense-two-kit>
- One micro USB cable

## Setup ##

Import the included .sls file to Studio then build and flash the project to the SLTB004A development kit.
In Simplicity Studio select "File->Import" and navigate to the directory with the .sls project file.
The project is built with relative paths to the STUDIO_SDK_LOC variable which was defined as

C:\SiliconLabs\SimplicityStudio\v5\developer\sdks\gecko_sdk_suite\v3.1

In Simplicity Studio, under the Debug Adapters window, right-click on the Thunderboard Sense 2 device and select "Launch console..." from the drop-down menu. In the Adapter Console window, select the "Admin" tab and type "serial vcom config speed 921600" into the terminal input. This will modify the VCOM baudrate to match the application settings. If making any changes to the USART baudrate, the baudrate change must also be modified in the VCOM debug adapter settings.

## How the Project Works ##

The application uses the process-action bare-metal project configuration model. The application advertises over BLE every 100 ms. The app sets the config GATT table entry statically in the GATT configuration file ({"sample_rate":102,"samples_per_packet":1,"column_location":{"AccelerometerX":0,"AccelerometerY":1,"AccelerometerZ":2,"GyroscopeX":3,"GyroscopeY":4,"GyroscopeZ":5}}). After connecting it the app can recieve notifications to transmit "data", which in this case is a single sample of acceleration and orientation data in a 6 dimensional vector.


 

## .sls Projects Used ##

SensiML_IMU_data_capture_ble.sls

## How to Port to Another Part ##

Open the "Project Properties" and navigate to the "C/C++ Build -> Board/Part/SDK" item.  Select the new board or part to target and "Apply" the changes.  Note: there may be dependencies that need to be resolved when changing the target architecture.
