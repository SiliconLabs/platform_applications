# Using Autonomous Peripherals in Low Power EM2 Mode #

## Summary ##

This project configures multiple peripherals to work together in low power EM2 mode. The application is capable of triggering the ADC Conversion from a GPIO button press or an LETimer timeout through PRS while in EM2. Once the ADC conversion is completed, the system transitions to EM1 and the LDMA copies the data into a buffer. Once the buffer is full, an LDMA interrupt is triggered to get the device out of EM2 and update the display. The display plots the recorded points and goes back to EM2 once complete.

Peripherals used: ADC, LETimer, LDMA, GPIO, PRS, EMU, SPI

## SDK version ##

v2.7

## Hardware Required ##

- One SLSTK3701A Giant Gecko GG11 Starter Kit
<https://www.silabs.com/products/development-tools/mcu/32-bit/efm32-giant-gecko-gg11-starter-kit>

## Setup ##

Import the included .sls file to Studio then build and flash the project to the SLSTK3701A STK.
In Simplicity Studio select "File->Import" and navigate to the directory with the .sls project file.
The project is built with relative paths to the STUDIO_SDK_LOC variable which was defined as

C:\SiliconLabs\SimplicityStudio\v4\developer\sdks\gecko_sdk_suite\v2.7

## .sls Projects Used ##

mcu_peripheral_low_energy_gg11.sls

## How to Port to Another Part ##

Open the "Project Properties" and navigate to the "C/C++ Build -> Board/Part/SDK" item.  Select the new board or part to target and "Apply" the changes.  Note: there may be dependencies that need to be resolved when changing the target architecture.
