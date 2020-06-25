# Simple Periodic Sleeptimer Example

## Summary
This project shows how to use the Sleeptimer in EM2 to periodically execute
a callback function. This project shows the program execution by toggling LED0.

## Gecko SDK Version
v2.7

## Hardware Required
Listed below are the port and pin mappings for working with this example.

* Board:  Silicon Labs EFM32GG Starter Kit (STK3700)
	* Device: EFM32GG990F1024
		* PE2 - LED0
		* PB9 - Push Button PB0

* Board:  Silicon Labs EFM32HG Starter Kit (SLSTK3400A)
	* Device: EFM32HG322F64
		* PF4 - LED0
		* PC9 - Push Button PB0

* Board:  Silicon Labs EFM32LG Starter Kit (STK3600)
	* Device: EFM32LG990F256
		* PE2 - LED0
		* PB9 - Push Button PB0
		
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
* mcu_sleeptimer_periodic_gg.sls
* mcu_sleeptimer_periodic_hg.sls
* mcu_sleeptimer_periodic_lg.sls

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

* The default timer peripheral used for Series 0 devices for sleeptimer is 
the RTC. This can be changed by editing the local config file and selecting a 
different clock. Be sure to select the correct low frequency clock branch when 
sourcing sleeptimer from a different clock.
