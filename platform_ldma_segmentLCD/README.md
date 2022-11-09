# Segment LCD with LC Sensor Functionality
![Type badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_ldma_segmentLCD_common.json&label=Type&query=type&color=green)
![Technology badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_ldma_segmentLCD_common.json&label=Technology&query=technology&color=green)
![License badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_ldma_segmentLCD_common.json&label=License&query=license&color=green)
![SDK badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_ldma_segmentLCD_common.json&label=SDK&query=sdk&color=green)
![Build badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_ldma_segmentLCD_build_status.json)
![Flash badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_ldma_segmentLCD_common.json&label=Flash&query=flash&color=blue)
![RAM badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_ldma_segmentLCD_common.json&label=RAM&query=ram&color=blue)

## Summary
This project shows how to use the FG23 LCD peripheral with the LDMA on the
BRD2600A board to update the segment LCD in EM2 using a custom segment LCD
driver.

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
segment LCD displaying 00000 -> 11111 -> 22222 -> ... -> 99999 -> 00000 -> ...,
updating every second.

## How It Works
The LCD peripheral is configured to send a DMA request on an LCD Frame Counter
event, which occurs every 1 second. The LDMA uses looping linked-list
descriptors to update the LCD_SEGn registers. A display buffer contains the
desired values to write to the LCD_SEGn registers.

The LCD_SEGn registers are clocked by a different clock domain and must be
synchronized in order for the register writes to take effect. The LCD
peripheral is configured so the LCD_SEGn registers are automatically synced
once the LCD_SEG3 register is written to.

The 1st descriptor sets the starting address of the display buffer as the base
source address of the LDMA channel. The 2nd desciptor is loaded and waits for a
DMA request.

The 2nd descriptor transfers the contents in the display buffer to the LCD_SEGn
registers. This descriptor loops 9 times, and then the 3rd descriptor is
loaded.

The 3rd descriptor updates the loop counter back to 9, and then loads the 1st
descriptor.

This example does not require processor intervention once entering EM2.

## .sls Projects Used
* platform_ldma_segmentLCD.sls

## How to Port to Another Part
Right click on the project and select "Properties" and navigate to "C/C++ 
Build" then "Board/Part/SDK". Select the new board or part to target and apply 
the changes. There may be some dependencies that need to be resolved when 
changing the target architecture. This example can only run out of the box on
BRD2600A device.
