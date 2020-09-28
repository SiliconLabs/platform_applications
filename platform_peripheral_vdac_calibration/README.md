# platform_peripheral_vdac_calibration

## Summary
This project demonstrates how to use the internal ADC to calibrate the DAC.
The project is written for and tested on an EFM32TG11 starter kit. 
Both VDAC channels are enabled and channel 0's main output and alternate 
output 1 is enabled. Note channel 0 main output has no breakout pin available
for EFM32TG11 starter kit. Therefore, testing measurement for channel 0 is 
only taken on channel 0 alternate output 1.

## Gecko SDK version
v2.7

## Hardware Required
SLSTK3301A EFM32TG11 Starter Kit

## Setup
Import the included .sls file then build and flash the project to the SLSTK3301A STK.

In Simplicity Studio select "File->Import" and navigate to the directory with the .sls project file. Import the project sls file. The project is built with relative paths to the STUDIO_SDK_LOC variable which was defined as

C:\SiliconLabs\SimplicityStudio\v4\developer\sdks\gecko_sdk_suite\v2.7

## How It Works
The DAC calibration routine for this project is as follows:

1. Initialize VDAC with the desired configuration
2. Initialize ADC with high accuracy setting
3. Output 80 percent of VFS (0xCCC) on both VDAC channel 0 and channel 1
4. Calibrate GAINERRTRIM - set GAINERRTRIM to max value 0x3F.
5. Use ADC internal DACOUT input to read DAC channel 0 output value.
6. Calculate the error. error = abs(Vout / (Vref \* 0.8)-1) or 
   abs((Vout / Vref \* 4096) / (Vref \* 0.8 / Vref \* 4096) - 1)
7. Decrease GAINERRORTRIM by 1, recalculate the error. Continue to do so
   until the smallest error is found.
8. Repeat the same process for GAINERRORTRIMCH1.
9. Update the VDAC0 calibration register.
10. Change VDAC output to the desired output value.

## .sls Projects Used
platform_peripheral_vdac_calibration.sls

## How to Port to Another Part
Open the "Project Properties" and navigate to the "C/C++ Build -> Board/Part/SDK" item.  Select the new board or part to target and "Apply" the changes.  
This example can only ported to series 1 devices.
Note: there may be dependencies that need to be resolved when changing the target architecture.

## Special Notes
This project uses the internal ADC to measure the VDAC output and calibrate
accordingly. This method will introduce measurement error caused by the ADC
so it is important to verify whether the result will meet the requirement needed.
