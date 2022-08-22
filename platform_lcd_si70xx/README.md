# Segment LCD and Temperature Sensor Example
![Type badge](https://img.shields.io/badge/Type-Virtual%20application-green)
![Technology badge](https://img.shields.io/badge/Technology-Platform-green)
![License badge](https://img.shields.io/badge/License-Zlib-green)
![SDK badge](https://img.shields.io/badge/SDK-v4.0.0-green)
![GCC badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_lcd_si70xx_gcc.json)

## Summary
This project demonstrates how to use the Si70xx temperature sensor to measure
and record temperature values and how to use the Segment LCD to display those
values. 

## Gecko SDK Version
v3.2

## Hardware Required
Listed below are the port and pin mappings for working with this example.

* Board:  Silicon Labs EFR32FG23 Dev Kit Board (BRD2600A) 
	* Device: EFR32FG23B010F512IM48
* Board:  Silicon Labs EFM32PG23 Pro Kit Board (BRD2504A) 
	* Device: EFM32PG23B310F512IM48

## Setup
1. Clone this repository from GitHub onto your local machine.
2. Open Simplicity Studio IDE and navigate to Project > Import > MCU project.
3. Click the browse button and navigate to the local repository folder.
4. Select the .sls file for the board, click Next twice, and then click Finish.
5. Build the project and download it to the Board.
6. Reset the board by pressing the Reset button on the board.
7. Observe the LCD displaying 00000 after the board is reset.
8. After 5 seconds, the LCD will update with the current temperature reading 
from the Si70xx RHT sensor. Every 5 seconds the LCD will update with the new
temperature reading.

## How the Project Works
The project uses a periodic sleeptimer that executes a callback function every
5,000ms. This callback function calls APIs from the Si70xx driver library that
measure and read the temperature and relative humidity values from the sensor. 
This value is then displayed on the segment LCD using APIs from the Segment LCD
driver library. 

## .sls Projects Used
* platform_lcd_si70xx_fg23.sls
* platform_lcd_si70xx_pg23.sls

## How to Port to Another Part
Right click on the project and select "Properties" and navigate to "C/C++ 
Build" then "Board/Part/SDK". Select the new board or part to target and apply 
the changes. There may be some dependencies that need to be resolved when 
changing the target architecture. 

## Note
Although measures have been taken to thermally isolate the sensor from the 
board, temperature readings will be influenced when power is dissipated on the
board. More accurate temperature measurements are achieved when powering the 
board with a battery or through the Mini Simplicity connector as self-heating 
from the on-board LDO is eliminated and the on-board debugger is put in a low
power state.