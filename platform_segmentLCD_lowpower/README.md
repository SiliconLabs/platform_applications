# Low Power Segment LCD example

## Summary

This project demonstrates the FG23 LCD peripheral on BRD2600A, PG28 on BRD2506A, and is configured for
low power consumption with all segments enabled.


## Gecko SDK version

v4.3.0

## Hardware Required

* Board:  Silicon Labs EFM32FG23 Dev Kit Board (BRD2600A)
  * Device: EFR32FG23B010F512IM48
* Board:  Silicon Labs EFM32PG28 Pro Kit Board (BRD2506A)
  * Device: EFM32PG28B310F1024IM68

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
not flicker.  Typical current consumption for the FG23 BRD2600A board in this configuration is 3.8 uA. Typical
current consumption for the PG28 BRD2506A in this configuration is 6.5 uA.

To test:

1. Build and flash the hex image onto the board. 
2. Disconnect USB and connect current sensing power supply to VMCU and GND.
3. (optional) set ALL_SEGMENTS to 0 to see power consumption with only one segment enabled 

## .sls Projects Used

* platform_lowpower_segmentLCD_fg23.sls
* platform_lowpower_segmentLCD_pg28.sls

## How to Port to Another Part

Right click on the project and select "Properties" and navigate to "C/C++ 
Build" then "Board/Part/SDK". Select the new board or part to target and apply 
the changes. There may be some dependencies that need to be resolved when 
changing the target architecture. This project can only work out of the box
on the boards listed in the Hardware Required section of this readme.