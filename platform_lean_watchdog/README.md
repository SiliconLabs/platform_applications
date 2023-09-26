# AN1426: Lean Watchdog #

## Summary ##

This project is part of AN1426. A watchdog provides a way to reset the device in case of a system failure. Series 2 EFM32/EFR32 devices have the WDOG peripheral which functions as the dedicated watchdog peripheral. Although the WDOG is capable of running in EM2/3 and consumes very little power, systems with very strict power requirements may benefit from using the "lean watchdog" alternative.

In EM2/3, peripherals are powered by power domains, and these power domains are automatically turned off when not in use. Each power domain is used to power multiple peripherals. If a single peripheral in a power domain is enabled, that entire power domain turns on. If the WDOG is the only peripheral enabled in its power domain, it may be more power efficient to use another peripheral that provides watchdog functionality. The BURTC is a peripheral that resides in the PDHV power domain, which is a power domain that is always on in EM2/3. Enabling the BURTC in EM2/3 adds an insignificant amount of current consumption compared to enabling the WDOG's power domain, especially at higher temperatures. The BURTC can be used as an alternative to the WDOG. Additionally, the LFXO's failure detection (FAILDET) feature can be used to detect a faulty LFXO. This feature is especially useful if the WDOG or BURTC uses the LFXO as the clock source.

Peripherals used: WDOG0, BURTC, LFXO, GPIO, SYSRTC

## Gecko SDK version ##

v4.3.1

## Hardware Required ##

* Board: Silicon Labs EFR32xG23 Radio Board (BRD4204D) + Wireless Starter Kit Mainboard
  * Device: EFR32ZG23B010F512IM48

## Connections Required ##

Connect the radio board to the WSTK, and connect the WSTK via a micro-USB cable to your PC to flash the example.

## Setup ##

Import the included .sls file to Simplicity Studio then build and flash the project to the BRD4204D Radio Board attached to the WSTK.

## How It Works ##

Pressing PB0 toggles the energy mode between EM1 and EM2. When in EM1, the WDOG0 is configured as the system's watchdog. WDOG0 selects the LFXO as its clock source and is configured to reset the system if the WDOG0 is not fed within 2 seconds. When in EM2, the BURTC is configured as the system's watchdog. The BURTC selects the LFXO as its clock source and is configued to trigger an interrupt if the BURTC is not fed within 2 seconds. The BURTC's interrupt handler is written to reset the system on a compare match event.

The sleeptimer service uses the SYSRTC to feed the appropriate watchdog every second. Pressing PB1 while the sleeptimer attempts to feed the watchdog will prevent the watchdog from being fed.

Pressing PB1 for at most 2 seconds prevents the watchdog from being fed and will trigger a watchdog reset. Out of reset, the software checks the reset cause. If the reset cause was due to the WDOG0, LED0 will turn on and the software will stall in an infinite while loop. If the reset cause was due to a system reset request, LED1 will turn on and the software will stall in an infinite while loop.

The LFXO's FAILDET feature provides extra system failure detection. By enabling the FAILDET feature, the LFXO interrupt is triggered if fewer than 3 LFXO clock positive edges occur during 1 ms. The LFXO interrupt handler is written to reset the system if an LFXO failure is detected.

After flashing the code to the radio board, the user may see that LED1 turns on and the software stalls although PB1 was not pressed. This is because the flash program sent a system reset request to reset the device after downloading the code. Pressing the RESET button on the WSTK will trigger a pin reset and the software should function as expected.

This example does not function properly with debuggers in the Simplicty Studio IDE because the debugger sends a system reset request upon connection, and the software will stall in the infinite while loop out of reset.

## .sls Projects Used ##

platform_lean_watchdog.sls

## How to Port to Another Part ##

Open the "Project Properties" and navigate to the "C/C++ Build -> Board/Part/SDK" item. Select the new board or part to target and "Apply" the changes.

The GPIO ports and pins configured as LED0, LED1, PB0, and PB1 vary between radio boards. The user must modify these macros defined in the app.c file to use the correct GPIO ports and pins.
