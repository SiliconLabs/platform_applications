# Segment LCD with Animation and Blink Functionality

## Summary

This project shows how to use the FG23 LCD peripheral on the BRD2600A board to perform
auto animation and blink without CPU intervention, and displaying a pattern
on the segment LCD (more information on "How it works" below).

## Gecko SDK version

v4.3.0

## Hardware Required

* Board:  Silicon Labs EFR32FG23 Dev Kit (BRD2600A)
  * Device: EFR32FG23B010F512GM48

## Connections Required

Connect the board via a micro-USB cable to your PC to flash the example.

## Setup

1. Clone the repository with this project from GitHub onto your local machine.

2. From within the Simplicity Studio IDE, select Import -> MCU Project... from the Project menu.

3. Click the Browse button and navigate to the local repository folder, then to the SimplicityStudio folder.

4. Click the Next button twice and then click Finish.

## How It Works

The LCD peripheral has the capability to do animation without involving the LCD data
registers as well as the CPU. It allows specialized patterns running on the LCD panel while the 
microcontoller remains in low energy mode. A max of 8 segments can be used for animation feature
and they can either be segments 0-7 controlled by COM0 or segments 8-15 controlled by COM0. The animation
is implemented as two programmable 8 bit registers that are shifted either left or right every other
animation state for a total of 16 states. The LCD_AREGA register is shifted every odd state and the
LCD_AREGB register is shifted every even state. The two registers can either be OR'ed or AND'ed to achieve
the desired animation pattern. The animation state machine is described in section 27.3.13.3 of the reference manual.

The BRD2600A board only maps segment 0-1 and segment 4-7 to the segment LCD panel, therefore the animation
feature can only control 6 physical LCD segments on this board.

The LCD peripheral also has the capability to blink at a frequency given by CLKevent every 2Hz. The segments will be
alternating between on and off when the LCD is blinking. Refer to section 27.3.13.1 of the reference manual
for more information regarding the blinking feature.

For this example, the LCD segments controlled by the animation feature are: 1D, 1C, 2D, P2, 3D, P3. 
Refer to the BRD2600A schematic for additional information.

![fg23_lcd_segment_mapping](https://user-images.githubusercontent.com/66031031/193937564-7755558f-d8a2-4e97-b436-760965fd6afa.png)

To test:

1. Build and flash the hex image onto the board. Reset board and observe the segment LCD displaying animation at a 2Hz rate
2. Change BLINK_ENABLE define on line 30 of app.c file to 1
3. Rebuild and flash the hex image onto the board. Reset board and observe the segment LCD displaying animation and blinking at a 2Hz rate. This example runs as it is and requires no user intervention.

![fg23_animation_demo](https://user-images.githubusercontent.com/66031031/193944633-d7a458a9-49b9-4eaa-afec-87ecd30036e5.mov)

**Note**:
As seen in the video above, the LCD will be blank for 2 seconds since segments 2 and 3 are not
mapped to the onboard LCD. Blink feature is not enabled in the video.

## .sls Projects Used

* platform_lcd_animation_blink_fg23.sls

## How to Port to Another Part

Right click on the project and select "Properties" and navigate to "C/C++ 
Build" then "Board/Part/SDK". Select the new board or part to target and apply 
the changes. There may be some dependencies that need to be resolved when 
changing the target architecture. This example can only run out of the box on
BRD2600A device.
