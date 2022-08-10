# platform_peripheral_lesense_adc #
![Type badge](https://img.shields.io/badge/Type-Virtual%20application-green)
![Technology badge](https://img.shields.io/badge/Technology-Platform-green)
![License badge](https://img.shields.io/badge/License-Zlib-green)
![SDK badge](https://img.shields.io/badge/SDK-v2.7.6-green)
![GCC badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_peripheral_lesense_adc_gcc.json)

## Summary ##

This project demonstrates using LESENSE to trigger ADC conversions and read the result from the LESENSE interrupt handler. This project
scans over two LESENSE channels and sample two ADC inputs at a rate of 1Hz.

## Gecko SDK version ##

v2.7

## Hardware Required ##

- One SLSTK3701A Giant Gecko GG11 Starter Kit
<https://www.silabs.com/products/development-tools/mcu/32-bit/efm32-giant-gecko-gg11-starter-kit>

## Setup ##

Import the included .sls file then build and flash the project to the SLSTK3701A STK.

In Simplicity Studio select "File->Import" and navigate to the directory with the .sls project file.
Import the project sls file.
The project is built with relative paths to the STUDIO_SDK_LOC variable which was defined as

C:\SiliconLabs\SimplicityStudio\v4\developer\sdks\gecko_sdk_suite\v2.7

## How the Project Works ##

ADC is configured in scan mode with two channels enabled:
1. APORT3XCH8 - PE8 (Expansion Header 12)
2. APORT3YCH9 - PE9 (Expansion Header 14)

The two channels are in input_group 0 with channel number 0 and 1 respectively.

LESENSE is configured in periodic scan mode with channel 0 and 1 enabled. Channel 0 and 1 evaluation mode is set to
ADC output. The BUFDATA_VALID interrupt flag is enabled for LESENSE

LESENSE will run in periodic mode with 1Hz frequency. For each LESENSE scan, two channels will be scanned and
two ADC conversions are triggered. The result is read by the LESENSE interrupt.

To test LED blinking at 1Hz:
1. Import the project
2. Build the project, and flash the hex image onto the device.
3. Observe LED0 toggling at a rate of 1Hz

To test ADC sampling (Note: there will be some synchronization issue between the ADC and LESENSE in debug mode):
1. Import the project
2. Run the project in debug mode by clicking on the debug icon.
3. Connect a known voltage supply that is between 0-2.5V to the ADC input pins PE8/PE9.
4. Add variable "inputs" to the watch expression window.
5. Run the project, then halt the debugger at any point in time.
6. Observe the values change in the "inputs" variable.

Note: the "inputs" variable is updated with the new ADC value sampled every 1Hz.

## .sls Projects Used ##

platform_peripheral_lesense_adc.sls

## How to Port to Another Part ##

Open the "Project Properties" and navigate to the "C/C++ Build -> Board/Part/SDK" item.  Select the new board or part to target and "Apply" the changes. This project can only be ported to series 1 devices.  Note: there may be dependencies that need to be resolved when changing the target architecture.
