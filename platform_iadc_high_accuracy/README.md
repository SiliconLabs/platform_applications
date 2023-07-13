# IADC 20-bit High Accuracy: Dedicated analog inputs with LDMA ping-pong #

## Summary ##

This project configures the IADC peripheral to run continously after software trigger performing single conversions of a differential input. The IADC peripheral is configured to utilize dedicated analog inputs, AIN0 and AIN1, as a single differential input, AIN0 as positive input and AIN1 as negative. IADC reference is configured to use external analog reference provided by ADR1581 circuit on BRD2506A.

The LDMA peripheral is configured to ping-pong data transfers of 1024 conversions between two buffers, allowing statistical processing of one buffer without interrupting data conversion and storage in the other. Mean and variance calculations are made with the data buffer contents using a simplified [Welford's algorithm](https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance) and results are stored in local variables. Mean voltage is also computed and stored in a global variable.

IADC is configured to use FSRCO for clock source with prescalers set to optimize conversion speed to the maximum 5 Mhz allowed for high accuracy mode. The IADC operates in high accuracy mode with oversampling rate set to 256 to achieve a 20-bit conversion (16-bit ENOB). With high accuracy mode and clock settings, firmware is able to achieve 3.8 kSps max sampling rate as specified in the device datasheet.

Peripherals used: IADC, LDMA, GPIO, PRS

## Gecko SDK version ##

v4.3.0

## Hardware Required ##

* Board:  Silicon Labs EFM32PG28 Pro Kit Board (BRD2506A)
  * Device: EFM32PG28B310F1024IM68

## Connections Required ##

Connect the board via a micro-USB cable to your PC to flash the example.
Dedicated analog input via SMA connector and expansion header pin 3 are utilized for IADC conversion. Example is configured for differential measurement; jumper exapnsion header pin 3 to GND (expansion header pin 1) for single-ended analog measurement via SMA connector.

## Setup ##

Import the included .sls file to Studio then build and flash the project to the BRD2506A Pro Kit.
In Simplicity Studio select "File->Import" and navigate to the directory with the .sls project file.
The project is built with relative paths to the STUDIO_SDK_LOC variable.

## How It Works ##

The IADC is configured to sample differential voltage across dedicated analog inputs AIN0/AIN1 at ~3.8 kSps. LED0 is configured via PRS to pulse with the conpletion of each single conversion. LED1 will toggle with every LDMA transfer completion of 1024 samples into one of the ping-pong buffers. Setting a breakpoint on line 339 in the iadc_single_process_action(), the mean, meanV and variance can be observed by adding these variable names to the expressions window in the Debug perspective.

## .sls Projects Used ##

platform_iadc_high_accuracy_pg28.sls

## How to Port to Another Part ##

Open the "Project Properties" and navigate to the "C/C++ Build -> Board/Part/SDK" item.  Select the new board or part to target and "Apply" the changes.  Note: This example uses the IADC peripheral, which is only available on our Series 2 MCU and Wireless MCU devices. There may also be dependencies that need to be resolved when changing the target architecture.
