# Simple One-shot Sleeptimer Example 

## Summary
This project shows how to use the Sleeptimer as a one shot timer in EM1 to 
execute a callback function. The user presses PB0 to generate an interrupt that
will turn on LED0 and start a one shot sleeptimer that will turn off LED0 after
the timer has expired. EM1 is used for Series 2 examples because PB0 is 
connected to Port D on xG21. Port D is not available for use in EM2, so EM1
must be used. If EM2 is used on xG22, an "escape hatch" code is needed, 
because the device goes into EM2 too quickly and will brick the board. 

## Gecko SDK Version
v2.7

## Hardware Required
Listed below are the port and pin mappings for working with this example.

* Board:  Silicon Labs EFR32xG21 Radio Board (BRD4181A) + 
        Wireless Starter Kit Mainboard
	* Device: EFR32MG21A010F1024IM32
		* PB0 - LED0
		* PD2 - Push Button PB0

* Board:  Silicon Labs EFR32xG22 Radio Board (BRD4182A) + 
        Wireless Starter Kit Mainboard
	* Device: EFR32MG22C224F512IM40
		* PD2 - LED0
		* PB0 - Push Button PB0

## Setup
0. Build the project and download it to the Starter Kit
1. Both LEDs will be off
2. Press PB0 to turn on LED0.
3. After 1 second, LED0 will turn off.
4. Steps 2 and 3 can be repeated.

## How the Project Works
The following is the program flow corresponding to the LEDs:
1. Upon reset, both LEDs will be off. Since the board was reset through 
   powering on the board (POR), hitting the reset button (external reset pin), 
   or requesting a system reset (through the debugger), LED0 will be turned off.
2. The board will then enter EM1 and wait for an interrupt triggered by the user
   pressing PB0. 
3. Once PB0 is pressed, LED0 will turn on. A one shot sleeptimer will start that
   will return to the LED0 callback function once the timer expires. The device 
   returns to EM1 until sleeptimer expires.
4. After half a second, the LED0 callback function will be executed, which will
   turn off LED0. The device will re-enter EM1. 
5. Steps 2-4 can be repeated.

## .sls Projects Used
* mcu_sleeptimer_oneshot_gxg21.sls
* mcu_sleeptimer_oneshot_xg22.sls

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

* To allow for compatibility between boards, logic for both even and odd
GPIO interrupt request handlers are included. Only one will execute once the 
GPIO_setup function executes, which will determine whether PB0 is on an even
or odd pin depending on the device used.

