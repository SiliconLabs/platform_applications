# IADC High Accuracy Mode functionality example

## Summary
This project shows how to use the IADC in high accuracy mode on EFM32PG23-PK2504A
(BRD2504A). 

## Gecko SDK Version
v4.3.1

## Hardware Required

* Board:  Silicon Labs EFM32PG23 Pro Kit (BRD2504A)
	* Device: EFM32PG23B310F512IM48

## Setup
Clone the repository with this project from GitHub onto your local machine.

From within the Simplicity Studio IDE, select Import -> MCU Project... from the 
Project menu. Click the Browse button and navigate to the local repository 
folder, then to the SimplicityStudio folder, select the .sls file for the 
board, click the Next button twice, and then click Finish.

How To Test:
1. Update the kit's firmware from the Simplicity Launcher (if necessary).
2. Build the project and download to the Pro Kit.
3. Open the Simplicity Debugger and add "sample" and "singleResult" to the 
   Expressions Window.
4. Apply a voltage to the IADC input pin (PA05).
5. Observe the sample field as it will display as a:
   20-bit result - "singleResult" is obtained by the formula: sample*VREF/(2^20).
6. Observe output pin (PB04) pulsing:
   20-bit resolution - conversion time formula: ((5*OSR) + 7)/fCLK_ADC
     with high accuracy OSR set to 256 and 1 MHz fCLK_ADC, this corresponds to single
     conversion time of ~1.3ms. With digital averaging set to 2X, final conversion time
     is ~2.6ms, or a sampling frequency of roughly 388 Hz
7. Suspend the debugger, observe the measured voltage change in the Expressions
   Window and how it responds to different voltage values on the corresponding
   pins (see below).


## How the Project Works
This project demonstrates using the IADC peripheral's oversampling and high accuracy
features to acquire 20-bit resolution conversion results while operating in EM2. The
firmware utilizes emlib IADC structures and functions to properly configure the
peripheral and employ the appropriate offset corrections. IADC interrupts on conversion
completion wake the MCU into EM0, where the IADC interrupt handler converts the result 
to a voltage before returning to EM2 (through software component "Power Manager"). Using
the HFRCOEM23 clock source configured for 1 MHz, the IADC sampling rate is 388 Sps with 
an oversampling rate of 256(OSRHA) * 2(DIGAVG) = 512, and the IADC reads GPIO pin PA05 
as input. The PRS peripheral will output a pulse on PB04 whenever the IADC finishes one 
single conversion. Additionally, LED0 blinks at a 1 Hz frequency (500 ms toggle rate) to
indicate firmware is running.

## .sls Projects Used
* platform_iadc_high_accuracy_PG23.sls

## How to Port to Another Part
Right click on the project and select "Properties" and navigate to "C/C++ 
Build" then "Board/Part/SDK". Select the new board or part to target and apply 
the changes. There may be some dependencies that need to be resolved when 
changing the target architecture. This project can only be run on other 
Series 2 devices. 