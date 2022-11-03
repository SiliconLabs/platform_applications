# Low Power Segment LCD example

## Summary
This project demonstrates the FG23 LCD peripheral on the BRD2600A and is configured for
low power consumption with all segments enabled.


## Gecko SDK version
v4.1.1

## Hardware Required

* Board:  Silicon Labs EFM32FG23 Dev Kit Board (BRD2600A)
	* Device: EFR32FG23B010F512IM48

## Connections Required
Connect the board via a micro-USB cable to your PC to flash the example.
To take current measurements, disconnect USB and connect power supply or current sensing supply to VMCU and GND

## Setup
1. Clone the repository with this project from GitHub onto your local machine.

2. From within the Simplicity Studio IDE, select Import -> MCU Project... from the Project menu.

3. Click the Browse button and navigate to the local repository folder, then to the SimplicityStudio folder.

4. Click the Next button twice and then click Finish.

## How It Works
This example enables all LCD segments with no animation or interrupts.  The LCD settings have been selected
for low power consumption.  The contrast and frame rate is readable indoors or outdoors and the display does
not flicker.  Typical current consumption for the board in this configuration is 3.8 uA.


To test:
1. Build and flash the hex image onto the board. 
2. Disconnect USB and connect current sensing power supply to VMCU and GND.
3. (optional) set ALL_SEGMENTS to 0 to see power consumption with only one segment enabled 



## .sls Projects Used
* platform_lowpower_segmentLCD.sls

## How to Port to Another Part
Right click on the project and select "Properties" and navigate to "C/C++ 
Build" then "Board/Part/SDK". Select the new board or part to target and apply 
the changes. There may be some dependencies that need to be resolved when 
changing the target architecture. This example can only run out of the box on
BRD2600A device.
