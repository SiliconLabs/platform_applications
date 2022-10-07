# Segment LCD with Animation and Blink Functionality

## Summary
This project shows how to use the PG23 LCD peripheral on the BRD2504A board to perform
auto animation and blink without CPU intervention, and displaying a pattern
on the segment LCD (more information on "How it works" below).

## Gecko SDK version
v4.1.1

## Hardware Required

* Board:  Silicon Labs EFM32PG23 Pro Kit (BRD2504A)
	* Device: EFM32PG23B310F512IM48

## Connections Required
Connect the board via a micro-USB cable to your PC to flash the example.

## Setup
1. Clone the repository with this project from GitHub onto your local machine.

2. From within the Simplicity Studio IDE, select Import -> MCU Project... from the Project menu.

3. Click the Browse button and navigate to the local repository folder, then to the SimplicityStudio folder.

4. Click the Next button twice and then click Finish.

## How It Works
The LCD peripheral has the capability to do different animation without involving the LCD data
registers as well as CPU. It allows specialized patterns running on the LCD panel while the 
microcontoller remains in Low Energy Mode. A max of 8 segments can be used for animation feature
and they can either be segment 0-7 controlled by COM0 or segment 8-15 controlled by COM0. The animation
is implemented as two programmable 8 bit registers that are shifted either left or right every other
Animation state for a total of 16 states. The LCD_AREGA register is shifted every odd state and the
LCD_AREGB register is shifted every even state. The two registers can either be OR'ed or AND'ed to achieve
the desired animation pattern. The animation state machine is described in section 27.3.13.3 of the reference manual.

This example uses segment 0-7 that are controlled by COM0 to demonstrate the animation feature.

The LCD peripheral also has the capability to blink at a frequency given by CLKevent every 2Hz. The segments will be
alternating between on and off when the LCD is blinking. Refer to section 27.3.13.1 of the reference manual
for more infomration regarding the blinking feature.

For this example, the LCD segments controlled by the animation feature are: 1D, 1C, 2D, P2, 3D, P3,
4D, 4C. Refer to the BRD2504A schematic for additional information.

![pg23_lcd_mapping](https://user-images.githubusercontent.com/66031031/194141386-f2389206-243f-4ad7-82ac-83adf5a2cc7e.png)

To test:
1. Build and flash the hex image onto the board. Reset board and observe the
   segment LCD displaying animation at a 2Hz rate.
2. Change BLINK_ENABLE define on line 30 of app.c file to 1.
3. Rebuild and flash the hex image onto the board. Reset board and observe the
   segment LCD displaying animation and blinking at a 2Hz rate.
   This example runs as it is and requires no user intervention. 


https://user-images.githubusercontent.com/66031031/194141962-336489ec-55b6-4eb8-825a-892cf1ea7abc.mov

**Note**:
Blink feature is not enabled in the video. 

## .sls Projects Used
* platform_lcd_animation_blink_pg23.sls

## How to Port to Another Part
Right click on the project and select "Properties" and navigate to "C/C++ 
Build" then "Board/Part/SDK". Select the new board or part to target and apply 
the changes. There may be some dependencies that need to be resolved when 
changing the target architecture. This example can only run out of the box on
BRD2600A device.
