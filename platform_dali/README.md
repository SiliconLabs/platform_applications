# AN1220: DALI Communication #

## Summary ##

These projects are part of AN1220. DALI is an international standard lighting control system, which uses an asynchronous serial protocol with manchester encoding to send data between the main and secondary devices. These projects demonstrate how to configure Silicon Labs MCUs to send DALI frames between main and secondary devices.

Peripherals used: EUSART, USART, PRS, LDMA, GPIO, TIMER, SYSRTC

## Gecko SDK version ##

v4.3.1

## Hardware Required ##

* Board: Silicon Labs EFM32MG12P Radio Board (BRD4161A) + Wireless Starter Kit Mainboard
  * Device: EFR32MG12P432F1024GL125
* Board: Silicon Labs EFR32xG21 Radio Board (BRD4181A) + Wireless Starter Kit Mainboard
  * Device: EFR32MG21A010F1024IM32
* Board: Silicon Labs EFR32xG24 Radio Board (BRD4186C) + Wireless Starter Kit Mainboard
  * Device: EFR32MG24B210F1536IM48

## Connections Required ##

Connect two boards (a main board and a secondary board) via a micro-USB cable to your PC to flash the example.

The table below defines the GPIO pins and expansion headers used for DALI pins for each radio board.

| Pin     | EFR32MG12 WSTK (BRD4161A) | EFR32xG21 WTSK (BRD4181A) | EFR32xG24 WTSK (BRD4186C) |
| ------- | ------------------------- | ------------------------- | ------------------------- |
| DALI TX | EXP09 - PD11              | EXP04 - PC0               | EXP04 - PC1               |
| DALI RX | EXP11 - PD12              | EXP06 - PC1               | EXP06 - PC2               |
| GND     | EXP01                     | EXP01                     | EXP01                     |

## Setup ##

To setup the main device, import one of the .sls files for the main radio board. Check the project's properties to ensure the DALI_SECONDARY symbol isn't defined, then build and flash the project to the radio board.

To setup the secondary device, import one of the .sls files for the secondary radio board. Check the project's properties to ensure the DALI_SECONDARY symbol is defined, then build and flash the project to the radio board.

Connect the main's DALI TX pin to the secondary's DALI RX pin, the main's DALI RX pin to the secondary's DALI TX pin, and the main's GND pin to the secondary's GND pin.

Open a terminal program (like Tera Term) for each radio board and set the baud rate to 115201-8-N-1.

## How It Works ##

The WSTK is connected to the computer via VCOM. Typing '1' into the secondary's terminal prepares the secondary device for reception of the forward frame from the main device. Typing '1' into the main's terminal triggers the main device to transmit the forward frame to the secondary device. The secondary device waits about 4 ms and then transmits a backward frame to the main device. Both the secondary and main devices will display the address and data from the forward and the data from the backward frame onto the terminals.

Devices with EUSART DALI support (like the EFR32xG24) can use the EUSART in asynchronous DALI mode to transmit forward frames and receive backward frames. Dedicated EUSART registers are used to configure the device for DALI. The GPIO and SYSRTC are used to configure the settling time and timeout between frames.

Devices without EUSART DALI support (like the EFR32xG12 and EFR32xG21) bit-bang the DALI frames. These projects use SPI to bit-bang DALI forward frames. The GPIO, TIMER, and PRS are used to receive backward frames.

## .sls Projects Used ##

BRD4161A_EFR32MG12P_dali.sls
BRD4161A_EFR32MG12P_dali_dmadrv.sls
BRD4181A_EFR32xG21_dali.sls
BRD4181A_EFR32xG21_dali_dmadrv.sls
BRD4186C_EFR32xG24_dali_dmadrv.sls

## How to Port to Another Part ##

Open the "Project Properties" and navigate to the "C/C++ Build -> Board/Part/SDK" item. Select the new board or part to target and "Apply" the changes. The dali_config.h file must be modified to add configuration to the new part number.

