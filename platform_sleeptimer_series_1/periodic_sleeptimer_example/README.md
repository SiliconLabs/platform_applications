# Simple Periodic Sleeptimer Example
![Type badge](https://img.shields.io/badge/Type-Virtual%20application-green)
![Technology badge](https://img.shields.io/badge/Technology-Platform-green)
![License badge](https://img.shields.io/badge/License-Zlib-green)
![SDK badge](https://img.shields.io/badge/SDK-v2.7.6-green)
![GCC badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/periodic_sleeptimer_example_gcc.json)

## Summary
This project shows how to use the Sleeptimer in EM2 to periodically execute
a callback function. This project shows the program execution by toggling LED0.

## Gecko SDK Version
v2.7

## Hardware Required
Listed below are the port and pin mappings for working with this example.

* Board:  Silicon Labs EFM32PG1 Starter Kit (SLSTK3401A)
	* Device: EFM32PG1B200F256GM48
		* PF4 - LED0

* Board:  Silicon Labs EFM32GG11 Starter Kit (SLSTK3701A)
	* Device: EFM32GG11B820F2048GL192
		* PH10 - LED0 (Red)

* Board:  Silicon Labs EFM32TG11 Starter Kit (SLSTK3301A)
	* Device: EFM32TG11B520F128GM80
		* PD2 - LED0

## Setup
0. Build the project and download it to the Starter Kit
1. Both LEDs will be off
2. After 1 second, LED0 will turn on.
3. After 1 second, LED0 will turn off.
4. Steps 2 and 3 will repeat until program is stopped or power is disconnected.

## How the Project Works
The following is the program flow corresponding to the LEDs:
1. Upon reset, both LEDs will be off. Since the board was reset through 
   powering on the board (POR), hitting the reset button (external reset pin), 
   or requesting a system reset (through the debugger), LED0 will be turned off.
2. The device will then enter EM2 and wait for the sleeptimer to timeout.
3. After the timeout value of 1 second is reached, the callback fuction will be 
   executed. This will toggle the state of the LED0 pin and exit the callback 
   function, which will put the device back into EM2.
4. After 1 second has passed, the sleeptimer will expire again, and the process
   described in Step 3 will repeat.

## .sls Projects Used
* gg11_mcu_sleeptimer_periodic.sls
* pg1_mcu_sleeptimer_periodic.sls
* tg11_mcu_sleeptimer_periodic.sls

## How to Port to Another Part
Right click on the project and select "Properties" and navigate to "C/C++ 
Build" then "Board/Part/SDK". Select the new board or part to target and apply 
the changes. There may be some dependencies that need to be resolved when 
changing the target architecture. 

## Special Notes
* The value of a tick can be changed based on clock source or the frequency 
divider, which can be changed in the local config file. This example uses a 
frequency divider of 1, and timer frequency of 32768 Hz, which is equivalent 
to 1 tick every 30.5 microseconds, which is the highest resolution setting. 
This can be calculated using the equation below.
	TICK_VALUE = 1/(CLOCK_FREQ/FREQ_DIVIDER)

* The default timer peripheral used for Series 1 and 2 devices is the
RTCC. This can be changed by editing the local config file and selecting a 
different clock. Be sure to select the correct low frequency clock branch when 
sourcing sleeptimer from a different clock.

