# Platform - PG28 LCD Animation Blink #

![Type badge](https://img.shields.io/badge/Type-Virtual%20Application-green)
![Technology badge](https://img.shields.io/badge/Technology-Platform-green)
![License badge](https://img.shields.io/badge/License-Zlib-green)
![SDK badge](https://img.shields.io/badge/SDK-v2024.12.2-green)
![Build badge](https://img.shields.io/badge/Build-passing-green)
![Flash badge](https://img.shields.io/badge/Flash-18.46%20KB-blue)
![RAM badge](https://img.shields.io/badge/RAM-4.58%20KB-blue)
## Overview ##

This project shows how to use the PG26 LCD peripheral on the BRD2505A board to perform auto animation and blink without CPU intervention.

This project displays a pattern on the segment LCD. The LCD peripheral can execute various animations without involving the LCD data registers as well as the CPU. It allows specialized patterns to run on the LCD panel while the microcontroller remains in Low Energy Mode.

More information on [How it works](#how-it-works) below.

## SDK version ##

SiSDK v2024.12.2

## Hardware Required ##

* Board:  Silicon Labs EFM32PG26 Pro Kit Board (BRD2505A)
  * Device: EFM32PG26B500F3200IL136

## Connections Required ##

Connect the board via a USB-C cable to your PC to flash the example.

## Setup ##

To test this application, you can either create a project based on an example project or start with an empty example project.

### Create a project based on an example project ###

1. Make sure that this repository is added to [Preferences > Simplicity Studio > External Repos](https://docs.silabs.com/simplicity-studio-5-users-guide/latest/ss-5-users-guide-about-the-launcher/welcome-and-device-tabs).

2. From the Launcher Home, add your board to My Products, click on it, and click on the **EXAMPLE PROJECTS & DEMOS** tab. Find the example project filtering by **'lcd animation'**.

3. Click the **Create** button on the **Platform - PG26 LCD Animation Blink** example. Example project creation dialog pops up -> click **Finish** and Project should be generated.

    ![Create_example](image/create_example.png)

4. Build and flash this example to the board.

### Start with an empty example project ###

1. Create an **Empty C Project** project for your hardware using Simplicity Studio 5.

2. Replace the `app.c` file in the project root folder with the `app.c` file in the src folder.

3. Open the .slcp file. Select the SOFTWARE COMPONENTS tab and install the software components:

    - [Platform] → [Peripheral] → [LCD]
    - [Services] → [Power Manager] → [Power Manger]
	- [Platform] → [Utilities] → [Microsecond Delay]

4. Build and flash the project to your device.

## How It Works ##

The EFR32PG26 Pro Kit supports the segment LCD segment peripheral.

A maximum of 8 segments can be used for the animation feature. They can either be segments 0-7 controlled by COM0 or segments 8-15 controlled by COM0. The animation is implemented as two programmable 8-bit registers that are shifted either left or right in every other animation state for a total of 16 states. The LCD_AREGA register is shifted in every odd state. The LCD_AREGB register is shifted in every even state. The two registers can either be OR'ed or AND'ed to achieve the desired animation pattern. The animation state machine is described in section 31.3.13.3 of the reference manual.

This example uses segments 0-7 that are controlled by COM0 to demonstrate the animation feature.

The LCD peripheral can also blink at a frequency given by CLKevent every 2Hz. The segments will be alternating between on and off when the LCD is blinking. Refer to section 31.3.13.1 of the reference manual for more information regarding the blinking feature.

For this example, the LCD segments controlled by the animation feature are DP2, 1E, 1D, 2E, 2D, 3E, 3D, and 4E. Refer to the BRD2505A schematic for additional information.

![pg26_lcd_mapping](image/pg26_lcd_segment_mapping.png)

## Testing ##

1. Build and flash the hex image onto the board. Reset the board and observe the segment LCD displaying animation at a 2Hz rate.
2. Change BLINK_ENABLE define on line 33 of `app.c` file to 1.
3. Rebuild and flash the hex image onto the board. Reset the board and observe the segment LCD displaying animation and blinking at a 2Hz rate. This example runs as it is and requires no user intervention.
