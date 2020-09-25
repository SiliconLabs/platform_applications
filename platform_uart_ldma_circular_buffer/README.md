# UART Circular Buffer with LDMA Example #

## Summary ##

This project uses an EFM32GG11 to receive UART frames into a circular buffer using LDMA.

Peripherals used: USART, LDMA, EMU

## Gecko SDK version ##

v3.0

## Hardware Required ##

- One SLSTK3701A Giant Gecko GG11 Starter Kit
<https://www.silabs.com/products/development-tools/mcu/32-bit/efm32-giant-gecko-gg11-starter-kit>
- One CP2102N-EK USBXpress Bridge Development Kit
<https://www.silabs.com/development-tools/interface/cp2102n-development-kit>

## Setup ##

Connect the GG11 STK's pins with the CP2102N-EK.

The final connections should looks like so:

| GG11     | CP2102N |
|----------|---------|
| PC1 (RX) | TXD     |
| PC0 (TX) | RXD     |

Open up a serial terminal device such as Tera Term, and open the port connected to the CP2102N device. 

Import the included .sls file to Studio then build and flash the project to the SLSTK3701A STK.
In Simplicity Studio select "File->Import" and navigate to the directory with the .sls project file.
The project is built with relative paths to the STUDIO_SDK_LOC variable which was defined as

C:\SiliconLabs\SimplicityStudio\v4\developer\sdks\gecko_sdk_suite\v3.0

## How the Project Works ##

The application sits in EM1 until an interrupt occurs. After typing into Tera Term and pressing enter, the serial signal from the USB is converted by the CP2102N into a UART signal that is readable by the GG11. The application then receives a data byte from the UART and transfers it to the UART's FIFO. The LDMA is configured to start a transfer as soon as it sees data available in the FIFO. AFter the LDMA transfers BUFFER_SIZE/2 bytes of data to the RX buffer, the LDMA interrupts. In the interrupt handler, the application updates the stop index of the circular buffer.

## .sls Projects Used ##

gg11_stepper_motor.sls

## How to Port to Another Part ##

Open the "Project Properties" and navigate to the "C/C++ Build -> Board/Part/SDK" item.  Select the new board or part to target and "Apply" the changes.  Note: there may be dependencies that need to be resolved when changing the target architecture.
