# NVM3 Integrity Test

## Summary

This project performs frequent NVM3 writes and checks that there has been no corruption of the NVM3 data, or changes to the CRC of the program flash space.

## Gecko SDK Version ##

v2.7 or later with Simplicity Studio v4

## Hardware Required ##

* BGM11S Blue Gecko Module Radio Board (BRD4303A) + Wireless Starter Kit Mainboard

## Setup

Clone the repository with this project from GitHub onto your local machine.

From within the Simplicity Studio IDE, select Import -> MCU Project... from the
Project menu. Click the Browse button and navigate to the local repository
folder, then to the SimplicityStudio folder, select the .sls file for the
board, click the Next button twice, and then click Finish. 

Use the flash programmer tool to erase the board first (to ensure there is not NVM3 data in the flash).  Optionally, load any flash patterns to the device.

Build and run the project on a connected WSTK.

LED1 (PF5) will flash quickly (~10 Hz).  Press PB0 to start.

If there is an error, LED1 will flash a count of pulses indicating which error occurred.  LED0 will flash while NVM3 is being written.

Errors:

1 - NVM3 cannot be opened, or number of objects incorrect

2 - Cannot get object info

3 - NVM3 data does not match what was written

4 - CRC of program space does not match original value

5 - NVM3 repack failed

## How the Project Works

The first time the device runs, the NVM3 is uninitialized.  LED1 (PF5) will flash quickly (~10 Hz).  Press PB0 to start.  The program will initialize NVM3 and write the program space CRC to NVM3.

After initialization, the program will periodically write to NVM3 storage.  It will stop and flash LED1 with an error number (~2 Hz) if the NVM3 keys do not have the expected value, or if the CRC does not match.

The program drives PF4 high when NVM3 operations (writes or repack) are being conducted.  This will allow an outside device to interrupt power specifically during NVM3 operations to test the device's robustness to sudden power loss.

## .sls project used

* platform_nvm3_integrity_test.sls

## How to Port to Another Part

Open the "Project Properties" and navigate to the "C/C++ Build -> Board/Part/SDK" item.  Select the new board or part to target and "Apply" the changes.
Linker mapping for NVM3 space may need to be changed in bgm1.ld (see https://www.silabs.com/community/mcu/32-bit/knowledge-base.entry.html/2018/11/21/nvm3_example-DP9N for more information).
GPIO pins may need to be reassigned if using a different WSTK or custom board.
