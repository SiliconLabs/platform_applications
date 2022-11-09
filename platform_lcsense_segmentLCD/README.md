# Segment LCD with LC Sensor Functionality
![Type badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_lcsense_segmentLCD_common.json&label=Type&query=type&color=green)
![Technology badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_lcsense_segmentLCD_common.json&label=Technology&query=technology&color=green)
![License badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_lcsense_segmentLCD_common.json&label=License&query=license&color=green)
![SDK badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_lcsense_segmentLCD_common.json&label=SDK&query=sdk&color=green)
![Build badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_lcsense_segmentLCD_build_status.json)
![Flash badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_lcsense_segmentLCD_common.json&label=Flash&query=flash&color=blue)
![RAM badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_lcsense_segmentLCD_common.json&label=RAM&query=ram&color=blue)

## Summary
This project shows how to use the FG23 LESENSE peripheral on the BRD2600A board to detect
metal near the LCSENSE tank circuit, then display the detection via the segment LCD.

## Gecko SDK version
v4.1.1

## Hardware Required
* Board:  Silicon Labs EFR32FG23 Dev Kit (BRD2600A)
	* Device: EFR32FG23B010F512GM48

## Connections Required
Connect the board via a micro-USB cable to your PC to flash the example.

## Setup
Clone the repository with this project from GitHub onto your local machine.

From within the Simplicity Studio IDE, select Import -> MCU Project... from the 
Project menu. Click the Browse button and navigate to the local repository 
folder, then to the SimplicityStudio folder, select the .sls file for the 
board, click the Next button twice, and then click Finish.

Build and flash the hex image onto the board. Reset board and observe the
segment LCD displaying 00000. Make sure no metal is near the tank circuit yet

To test:
1. Place a metal object near (right above or touching) the LC sensor
2. Observe the segment LCD counter increment by 1
3. Remove the metal object, and then place the metal object near the LC sensor again
4. Observe the segment LCD counter increment by 1
5. Press push button 0, observe the segment LCD display reset to 0
6. Repeat step 3 five times, observe the segment LCD increment by 1

## How It Works
The LESENSE peripheral is configured to scan the sensor periodically. The sensor is routed to
PB3 which in this case is configured as LESENSE channel 0. The pin is also routed to the ACMP, 
and the ACMP is controlled by the LESENSE to compare the dampling waveform with a divided
voltage of about 1.7 V. The VDAC is used to short the two ends of the tank circuit. 
During the excite stage, the measured pin is pulled low to charge the circuit.
The number of ACMP pulses is then compared with a threshold to determine if metal is near.
When metal is near the sensor, the waveform damps faster, which makes the number of ACMP
pulses less than the case where no metal is near. When this change in count occurs, the LESENSE 
decoder will update its state and signal the pulse counter peripheral (PCNT). 
Then the pulse counter will trigger an interrupt when its counter overflows to update the segment LCD.

## .sls Projects Used
* platform_lcsense_segmentLCD.sls
