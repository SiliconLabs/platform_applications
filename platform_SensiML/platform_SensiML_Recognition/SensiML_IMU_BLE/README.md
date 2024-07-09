# Platform - SensiML IMU Recognition with BLE #

![Type badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/SensiML_IMU_BLE_common.json&label=Type&query=type&color=green)
![Technology badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/SensiML_IMU_BLE_common.json&label=Technology&query=technology&color=green)
![License badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/SensiML_IMU_BLE_common.json&label=License&query=license&color=green)
![SDK badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/SensiML_IMU_BLE_common.json&label=SDK&query=sdk&color=green)
![Build badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/SensiML_IMU_BLE_build_status.json)
![Flash badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/SensiML_IMU_BLE_common.json&label=Flash&query=flash&color=blue)
![RAM badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/SensiML_IMU_BLE_common.json&label=RAM&query=ram&color=blue)

## Summary ##

This project uses the Thunderboard Sense 2 (EFR32MG12) and the onboard IMU sensor to take accelerometer and gyroscope data and classify them into Horizontal, Stationary and Vertical movements. The output can be viewed over serial VCOM or through the SensiML BLE application. This example project uses the Knowledge Pack created by SensiML along with the IMU component drivers running in a bare-metal configuration. The sensor data output is passed to the Machine Learning model created using SensiML's analytics studio, which is downloaded as a Knowledge Pack and incorporated into a Simplicity Studio V5 project to run inferences on the Thunderboard Sense 2.

For more on Knowledge Packs, see the [Knowledge Pack](https://sensiml.com/documentation/knowledge-packs/index.html) documentation.

## Gecko SDK version ##

- v4.4.3

## Hardware Required ##

- Thunderboard Sense 2 Development Kit - SLTB004A

## Connections Required ##

- Connect the Kit to the PC through a micro USB cable.

## Software Required ##

- [SensiML TestApp](https://sensiml.com/download/)

## Setup ##

To test this application, you can either create a project based on an example project or start with a "Bluetooth - SOC Empty" project based on your hardware.

### Create a project based on an example project ###

1. Make sure that this repository is added to [Preferences > Simplicity Studio > External Repos](https://docs.silabs.com/simplicity-studio-5-users-guide/latest/ss-5-users-guide-about-the-launcher/welcome-and-device-tabs).

2. From the Launcher Home, add the BRD4166A to My Products, click on it, and click on the **EXAMPLE PROJECTS & DEMOS** tab. Find the example project filtering by **sensiml**.

3. Click **Create** button on the **Platform - SensiML IMU Recognition with BLE** example. Example project creation dialog pops up -> click Create and Finish and Project should be generated.
![create_project](image/create_project.png)

4. Build and flash this example to the board.

### Start with a "Bluetooth - SOC Empty" project ###

1. Create a **Bluetooth - SOC Empty** project for your hardware using Simplicity Studio 5.

2. Copy all attached files in *inc* and *src* folders into the project root folder (overwriting existing).

3. Copy the `knowledgepack` (located in the SimplicityStudio folder) into the project root folder. Link the SensiML library to the project. See [Simplicity Studio Setup](https://github.com/SiliconLabs/platform_applications/tree/master/platform_SensiML/platform_SensiML_Recognition#simplicity-studio-setup) for details.

4. Import the GATT configuration:

   - Open the .slcp file in the project.

   - Select the **CONFIGURATION TOOLS** tab and open the **Bluetooth GATT Configurator**.

   - Find the Import button and import the attached **config/btconf/gatt_configuration.btconf** file.

   - Save the GATT configuration (ctrl-s).

5. Open the .slcp file. Select the SOFTWARE COMPONENTS tab and install the software components:

   - [Platform] → [Driver] → [LED] → [Simple LED] → default instance name: **led0** and **led1**.
   - [Application] → [Sensor] → [Inertial Measurement Unit sensor]
   - [Services] → [IO Stream] → [IO Stream: USART] → default instance name: **vcom**
   - [Application] → [Utility] → [Log]

6. Uninstall the software components:

   - [Bluetooth] → [OTA] → [In-Place OTA DFU]
   - [Platform] → [Bootloader] → [Bootloader Application Interface]

7. Build and flash the project to your device.

## How It Works ##

The application uses the process-action bare-metal project configuration model. Running a Machine Learning model on an embedded device such as the Thunderboard Sense 2 can be very broadly classified into three steps.

- Step 1: Data collection and labeling which is covered in the IMU Data Capture project.

- Step 2: This labeled data is then passed on to SensiML's Analytics Studio to design a machine learning model based on the end goal (i.e., classify movements). For inference to run on an embedded device, a Machine Learning model should be created and converted to an embedded device-friendly version and flashed to the device. The Machine Learning model is created, trained and tested in SensiML's Analytics Studio. The model that gets generated for the Thunderboard Sense 2 device is called a Knowledge Pack. Going into the details of this process is beyond the scope of this readme, but for more information, refer to SensiML's Analytics Studio Documentation - <https://sensiml.com/documentation/guides/analytics-studio/index.html>.

- Step 3:  The Knowledge Pack can be downloaded as a library and incorporated into an embedded firmware application. The application can then be flashed onto the device. The model will run on the Thunderboard Sense 2 and can classify IMU data based on the labels created in Steps 1 and 2. This project showcases step 3.

>Note: If you want to use your own knowledge pack with this example, please do not include the "sml_output.c" file that is included within the knowledge pack. This is because the "sml_output_results()" function has been reimplemented by Silicon Labs in "app_output.c" to also feature BLE.

## Testing ##

This project detects and classifies three types of motion:

- *Horizontal (Classification: 1)*

- *Stationary (Classification: 2)*

- *Vertical (Classification: 3)*

The data obtained from the IMU sensor is passed onto SensiML's Knowledge Pack which then classifies the motion. The classification output can be viewed via either a serial or a Bluetooth app.

### Serial output ###

After flashing the device with the firmware, open a serial terminal program (such as TeraTerm), select the device COM port and observe the classification output. The settings for the Serial Terminal are 115200 bps baud rate, 8N1 and no handshake.

![output](image/logs.png)

### Bluetooth output ###

To view classification results over BLE, download the [SensiML TestApp](https://sensiml.com/download/) for Android, and select the board named "SensiML IMU" when it is discovered.

![output](image/sensiml_app.png)

> Note: The Thunderboard Sense 2 must be placed on a stable surface during initialization as the IMU undergoes a calibration routine at this time.
