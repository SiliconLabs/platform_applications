# Segment LCD and Temperature Sensor Example
![Type badge](https://img.shields.io/badge/Type-Virtual%20application-green)
![Technology badge](https://img.shields.io/badge/Technology-Platform-green)
![License badge](https://img.shields.io/badge/License-Zlib-green)
![SDK badge](https://img.shields.io/badge/SDK-v4.1.1-green)
![GCC badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_lcd_si70xx_gcc.json)

## Summary
This project demonstrates how to use the Si70xx temperature sensor to measure
and record temperature values and how to use the Segment LCD to display those
values. 

## Gecko SDK Version
v4.1.1

## Hardware Required
* Board:  Silicon Labs EFR32FG23 Dev Kit Board (BRD2600A) 
	* Device: EFR32FG23B010F512IM48
* Board:  Silicon Labs EFM32PG23 Pro Kit Board (BRD2504A) 
	* Device: EFM32PG23B310F512IM48

## Connections Required 
Connect the board via a micro-USB or USB-C cable to your PC to flash the example. 

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

## How It Works
The project uses a periodic sleeptimer that executes a callback function every
5 seconds. This callback function calls APIs from the Si70xx driver library that
measure and read the temperature and relative humidity values from the sensor. 
This value is then displayed on the segment LCD using APIs from the Segment LCD
driver library. 

## .sls Projects Used
* platform_tempsensor_segmentlcd_fg23.sls
* platform_tempsensor_segmentlcd_pg23.sls

## Note
Although measures have been taken to thermally isolate the sensor from the 
board, temperature readings will be influenced when power is dissipated on the
board. More accurate temperature measurements are achieved when powering the 
board with a battery or through the Mini Simplicity connector as self-heating 
from the on-board LDO is eliminated and the on-board debugger is put in a low
power state.