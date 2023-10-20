# IADC High Speed Mode functionality example

## Summary
This project shows how to use the IADC in high speed mode on EFM32PG23-PK2504A
(BRD2504A). 

## Gecko SDK Version
v4.3.1

## Hardware Required

* Board:  Silicon Labs EFM32PG23 Pro Kit (BRD2504A)
	* Device: EFM32PG23B310F512IM48

## Setup
Clone the repository with this project from GitHub onto your local machine.

From within the Simplicity Studio IDE, select Import -> MCU Project... from the 
Project menu. Click the Browse button and navigate to the local repository 
folder, then to the SimplicityStudio folder, select the .sls file for the 
board, click the Next button twice, and then click Finish.

How To Test:
1. Update the kit's firmware from the Simplicity Launcher (if necessary).
2. Build the project and download to the Pro Kit.
3. Open the Simplicity Debugger and add "singleBuffer" to the 
   Expressions Window.
4. Apply a voltage to the IADC input pin (SMA connector).
5. Observe the singleBufffer array as it will display the ADC results
   12-bit result x 1024 samples - "singleBuffer" is VDD referenced and scaled
   0-4095
6. Observe output pin (PA05) pulsing:
   One edge (rising or falling) every 1024 samples, about every 520 uS
7. Suspend the debugger, observe the measured voltage change in the Expressions
   Window and how it responds to different voltage values on the corresponding
   pins (see below).


## How the Project Works
This project demonstrates using the IADC peripheral's oversampling and high speed
features to acquire 12-bit resolution conversion results. The
firmware utilizes emlib IADC structures and functions to properly configure the
peripheral and employ the appropriate offset corrections.  Once conversions are
started, the IADC converts continuously and the LDMA transfers the results into
an array.  The IADC is clocked from the 39 MHz HFXO, and the ADC prescaler divides
this by 2.  The sampling rate is 1.95 Msps


## .sls Projects Used
* platform_iadc_high_speed_PG23.sls

## How to Port to Another Part
Right click on the project and select "Properties" and navigate to "C/C++ 
Build" then "Board/Part/SDK". Select the new board or part to target and apply 
the changes. There may be some dependencies that need to be resolved when 
changing the target architecture. This project can only be run on other 
Series 2 devices. 
