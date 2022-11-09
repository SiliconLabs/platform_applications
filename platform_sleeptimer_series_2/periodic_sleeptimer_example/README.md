# Simple Periodic Sleeptimer Example
![Type badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/periodic_sleeptimer_example_common.json&label=Type&query=type&color=green)
![Technology badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/periodic_sleeptimer_example_common.json&label=Technology&query=technology&color=green)
![License badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/periodic_sleeptimer_example_common.json&label=License&query=license&color=green)
![SDK badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/periodic_sleeptimer_example_common.json&label=SDK&query=sdk&color=green)
![Build badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/periodic_sleeptimer_example_build_status.json)
![Flash badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/periodic_sleeptimer_example_common.json&label=Flash&query=flash&color=blue)
![RAM badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/periodic_sleeptimer_example_common.json&label=RAM&query=ram&color=blue)

## Summary
This project shows how to use the Sleeptimer in EM1 to periodically execute
a callback function. This project shows the program execution by toggling LED0.
EM1 is used for Series 2 examples because PB0 is connected to Port D on xG21. 
Port D is not available for use in EM2, so EM1 must be used. If EM2 is used on 
xG22, an "escape hatch" code is needed, because the device goes into EM2 too 
quickly and will brick the board. 

## Gecko SDK Version
v2.7

## Hardware Required
Listed below are the port and pin mappings for working with this example.

* Board:  Silicon Labs EFR32xG21 Radio Board (BRD4181A) + 
        Wireless Starter Kit Mainboard
	* Device: EFR32MG21A010F1024IM32
		* PB0 - LED0

* Board:  Silicon Labs EFR32xG22 Radio Board (BRD4182A) + 
        Wireless Starter Kit Mainboard
	* Device: EFR32MG22C224F512IM40
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
2. The device will then enter EM1 and wait for the sleeptimer to timeout.
3. After the timeout value of 1 second is reached, the callback fuction will be 
   executed. This will toggle the state of the LED0 pin and exit the callback 
   function, which will put the device back into EM1.
4. After 1 second has passed, the sleeptimer will expire again, and the process
   described in Step 3 will repeat.
   
## .sls Projects Used
* xg21_mcu_sleeptimer_periodic.sls
* xg22_mcu_sleeptimer_periodic.sls

## main.c Used
* xg21_main.c
* xg22_main.c

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

* The default clock used for Series 1 and 2 devices for sleeptimer is the
RTCC. This can be changed by editing the local config file and selecting a 
different clock. Be sure to select the correct low frequency clock branch when 
sourcing sleeptimer from a different clock.


