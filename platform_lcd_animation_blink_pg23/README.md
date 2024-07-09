# Platform - PG23 LCD Animation Blink #

![Type badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_lcd_animation_blink_pg23_common.json&label=Type&query=type&color=green)
![Technology badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_lcd_animation_blink_pg23_common.json&label=Technology&query=technology&color=green)
![License badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_lcd_animation_blink_pg23_common.json&label=License&query=license&color=green)
![SDK badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_lcd_animation_blink_pg23_common.json&label=SDK&query=sdk&color=green)
![Build badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_lcd_animation_blink_pg23_build_status.json)
![Flash badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_lcd_animation_blink_pg23_common.json&label=Flash&query=flash&color=blue)
![RAM badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_lcd_animation_blink_pg23_common.json&label=RAM&query=ram&color=blue)
## Overview ##

This project shows how to use the PG23 LCD peripheral on the BRD2504A board to perform auto animation and blink without CPU intervention.

The EFM32PG23 Pro Kit supports the segment LCD peripheral. This project displays specialized patterns on the segment LCD while the CPU is in EM2 sleep mode for energy saving and does not involve the LCD data registers.

More information on [How it works](#how-it-works) below.

## Gecko SDK version ##

GSDK v4.4.3

## Hardware Required ##

* Board:  Silicon Labs EFM32PG23 Pro Kit (BRD2504A)
  * Device: EFM32PG23B310F512IM48

## Connections Required ##

Connect the board via a micro-USB cable to your PC to flash the example.

## Setup ##

To test this application, you can either create a project based on an example project or start with an empty example project.

### Create a project based on an example project ###

1. Make sure that this repository is added to [Preferences > Simplicity Studio > External Repos](https://docs.silabs.com/simplicity-studio-5-users-guide/latest/ss-5-users-guide-about-the-launcher/welcome-and-device-tabs).

2. From the Launcher Home, add your board to My Products, click on it, and click on the **EXAMPLE PROJECTS & DEMOS** tab. Find the example project filtering by **'lcd animation'**.

3. Click the **Create** button on the **Platform - PG23 LCD Animation Blink** example. Example project creation dialog pops up -> click **Finish** and the project should be generated.

    ![Create_example](image/create_example.png)

4. Build and flash this example to the board.

### Start with an empty example project ###

1. Create an **Empty C Project** project for your hardware using Simplicity Studio 5.

2. Replace the `app.c` file in the project root folder with the `app.c` file in the src folder.

3. Open the .slcp file. Select the SOFTWARE COMPONENTS tab and install the software components:

    - [Platform] → [Peripheral] → [LCD]
    - [Services] → [Power Manager] → [Power Manager]

4. Build and flash the project to your device.

## How It Works ##

The LCD peripheral can execute various animations without involving the LCD data registers as well as the CPU. It allows specialized patterns to run on the LCD panel while the CPU remains in Low Energy Mode.

A maximum of 8 segments can be used for the animation feature. They can either be segments 0-7 controlled by COM0 or segments 8-15 controlled by COM0. The animation is implemented as two programmable 8-bit registers that are shifted either left or right for every other animation state for a total of 16 states. The LCD_AREGA register is shifted in every odd state. The LCD_AREGB register is shifted in every even state. The two registers can either be OR'ed or AND'ed to achieve the desired animation pattern. The animation state machine is described in section 27.3.13.3 of the reference manual.

This example uses segments 0-7 that are controlled by COM0 to demonstrate the animation feature.

The LCD peripheral can also blink at a frequency given by CLKevent every 2Hz. The segments will be alternating between on and off when the LCD is blinking. Refer to section 27.3.13.1 of the reference manual for more information regarding the blinking feature.

For this example, the LCD segments controlled by the animation feature are 1D, 1C, 2D, P2, 3D, P3, 4D, 4C. Refer to the BRD2504A schematic for additional information.

![pg23_lcd_mapping](image/pg23_lcd_segment_mapping.png)

## Testing ##

1. Build and flash the hex image onto the board. Reset the board and observe the segment LCD displaying animation at a 2Hz rate.
2. Change BLINK_ENABLE define on line 30 of the `app.c` file to 1.
3. Rebuild and flash the hex image onto the board. Reset the board and observe the segment LCD displaying animation and blinking at a 2Hz rate. This example runs as it is and requires no user intervention.

![pg23 lcd animation demo](image/pg23_lcd_animation_demo.gif)

**Note**:

  The blink feature is not enabled in the video.