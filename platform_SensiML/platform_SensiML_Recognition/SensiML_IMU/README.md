# SensiML IMU Recognition Example #

## Summary ##

This project uses the Thunderboard Sense 2 (EFR32MG12) and the onboard IMU sensor to take accelerometer and gyroscope data and classifies them into Horizontal, Stationary and Vertical movements. This example project uses the Knowledge Pack created by SensiML along with the IMU component drivers running in a bare-metal configuration. The sensor data output is passed to the Machine Learning model created using SensiML's analytics studio, which is downloaded as a Knowledge Pack and incorporated into a Simplicity Studio V5 project to run inferences on the Thunderboard Sense 2.

Software Components used: IMU - Inertial Measurement Unit, Simple LED, IO Stream: USART, Sleeptimer

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

After flashing the device with the firmware, open a serial terminal program (such as TeraTerm), select the device COM port and observe the classification output. The settings for the Serial Terminal are 115200 bps baud rate, 8N1 and no handshake. 

## How the Project Works ##

The application uses the process-action bare-metal project configuration model. Running a Machine Learning model on an embedded device such as the Thunderboard Sense 2 can be very broadly classified into three steps. 
Step 1: Data collection and labelling which is covered in the IMU Data Capture project. 
Step 2: This labelled data is then passed on to SensiML's Analytics Studio to design a machine learning model based on the end-goal (i.e., classify movements). For inference to run on an embedded device, a Machine Learning model should be created and converted to an embedded device friendly version and flashed to the device. The Machine Learning model is created, trained and tested in SensiML's Analytics Studio. The model that gets generated for the Thunderboard Sense 2 device is called a Knowledge Pack. Going into the details of this process is beyond the scope of this readme, but for more information, refer to SensiML's Analytics Studio Documentation - https://sensiml.com/documentation/guides/analytics-studio/index.html. 
Step 3:  The Knowledge Pack can be downloaded as a library and incorporated into an embedded firmware application. The application can then be flashed onto the device. The model will run on the Thunderboard Sense 2 and can classify incoming IMU data based on the labels created in Steps 1 and 2. This project showcases step 3. 

This project detects and classifies three types of motion - *Horizontal (Classification: 1)*, *Stationary (Classification: 2)*, *Vertical (Classification: 3)*.  

The data obtained from the IMU sensor is passed onto SensiML's Knowledge Pack that then classifies the data. The classification output can be viewed on a serial terminal as shown in the image below. 

![Teraterm output](doc/Classification_output.PNG)

>Note: The Thunderboard Sense 2 must be placed on a stable surface during initialization as the IMU undergoes a calibration routine at this time.

## .sls Projects Used ##

SensiML_IMU_recognition.sls

## How to Port to Another Part ##

Open the "Project Properties" and navigate to the "C/C++ Build -> Board/Part/SDK" item.  Select the new board or part to target and "Apply" the changes.  Note: there may be dependencies that need to be resolved when changing the target architecture.
