# Simple One-shot Sleeptimer Example 

## Summary
This project shows how to use the Sleeptimer as a one shot timer in EM2 to 
execute a callback function. The user presses PB0 to generate an interrupt that
will turn on LED0 and start a one shot sleeptimer that will turn off LED0 after
the timer has expired.

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
2. After PB0 is pressed, LED0 will turn on.
3. After half a second, LED0 will turn off.
4. Repeat steps 2-3.

## How the Project Works
The following is the program flow corresponding to the LEDs:
1. Upon reset, both LEDs will be off. Since the board was reset through 
   powering on the board (POR), hitting the reset button (external reset pin), 
   or requesting a system reset (through the debugger), LED0 will be turned off.
2. The board will then enter EM2 and wait for an interrupt triggered by the user
   pressing PB0. 
3. Once PB0 is pressed, LED0 will turn on. A one shot sleeptimer will start that
   will return to the LED0 callback function once the timer expires. The device 
   returns to EM2 until sleeptimer expires.
4. After half a second, the LED0 callback function will be executed, which will
   turn off LED0. The device will re-enter EM2. 
5. Steps 2-4 can be repeated.

## .sls Projects Used
* gg_mcu_sleeptimer_oneshot.sls
* hg_mcu_sleeptimer_oneshot.sls
* lg_mcu_sleeptimer_oneshot.sls

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

* To allow for compatibility between boards, logic for both even and odd
GPIO interrupt request handlers are included. Only one will execute once the 
GPIO_setup function executes, which will determine whether PB0 is on an even
or odd pin depending on the device used.
