# RTCC and Watchdog baremetal example #

## Summary ##

This example's purpose is to use the RTCC to display time and date on the LCD. A watchdog timer is configured to keep track of the system and reset it if the LCD isn't updated with time regularly.

## Gecko SDK version ##

 v4.0.2 

## Hardware Required ##

- EFM32GG11 SLSTK3701A

## Setup ##

If using this project with the SLSTK3701A, it is enough to import the project through the provided .sls file into Simplicity Studio v5 (for details, see the [following site](https://docs.silabs.com/simplicity-studio-5-users-guide/latest/ss-5-users-guide-about-the-simplicity-ide/import-and-export)), build the project, and flash it to your board.

To manually build the project,
1. Create an empty C project
2. Add all the files in src and inc folders to the top level of the project directory. 
3. Add the following components to your project in the .slcp file of your project
	- Color Sharp Meory LCD
	- Memory LCD with usart SPI driver
	- EMU
	- GPIO
	- RTC
	- RTCC
	- WDOG
	- Power Manager 
	- Sleep Timer (Go to the configuration for sleep timer and change the timer used by sleep timer to RTC)
	- GLIB Graphics Library
	- GLIB driver for SHARP Memory LCD
4. Build the project

## How It Works ##

The RTCC is configured to use the 32768Hz LFXO Clock and generate an interrupt every second. The LCD is updated every second as a result of this interrupt. The watchdog timer is configured to overflow in 8s. If the LCD is updated atleast 7 times in the 8s period, the watchdog timer will be cleared. Otherwise, the system will be reset. Additionally, a warning is also displayed on the LCD when the watchdog timer has reached 75% of its total count.

To test the project, simply run the project and observe the output on the LCD. The date and time should be displayed according to the configuration. If BTN0 is pressed, then the WDOG timer should overflow and the system should reset.  

To disable system reset when the WDOG timer overflows, set the `resetDisable` variable in ` WDOG_Init_TypeDef` struct to true. This will trigger the TOUT interrupt on WDOG overflow and print "System Reset" on the LCD.

