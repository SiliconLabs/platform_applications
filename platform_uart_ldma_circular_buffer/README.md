# UART Circular Buffer with LDMA Example #
![Type badge](https://img.shields.io/badge/Type-Virtual%20application-green)
![Technology badge](https://img.shields.io/badge/Technology-Platform-green)
![License badge](https://img.shields.io/badge/License-Zlib-green)
![SDK badge](https://img.shields.io/badge/SDK-v3.0.0-green)
![GCC badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_uart_ldma_circular_buffer_gcc.json)

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

Connect the GG11 STK's PC1 pin (RX signal) to CP2102N-EK's TXD pin.

Open up a serial terminal device such as Tera Term, and open the port connected to the CP2102N device. 

Import the included .sls file to Studio then build and flash the project to the SLSTK3701A STK.
In Simplicity Studio select "File->Import" and navigate to the directory with the .sls project file.
The project is built with relative paths to the STUDIO_SDK_LOC variable which was defined as

C:\SiliconLabs\SimplicityStudio\v4\developer\sdks\gecko_sdk_suite\v3.0

## How the Project Works ##

The application sits in EM1 until an interrupt occurs. After typing into Tera Term, the serial data from the USB is converted by the CP2102N into a UART signal that is stored into the GG11's USART RX FIFO. The LDMA is configured to start a transfer as soon as it sees data available in the FIFO. AFter the LDMA transfers BUFFER_SIZE/2 bytes of data to the circular buffer, the LDMA triggers an interrupt. In the interrupt handler, the application updates the stop index of the circular buffer to either 0 or BUFFER_SIZE/2.

In the case where less than BUFFER_SIZE/2 data bytes are received, the LDMA interrupt will not trigger. This presents an issue to the user: the circular buffer contains updated data, but the stop index is not updated. To fix this, the RX Timeout feature triggers an interrupt after 256 baud times if a new UART frame isn't received after a end of frame event. The interrupt handler updates the stop index according to the number of LDMA transfers that occured.

## .sls Projects Used ##

gg11_uart_ldma_circular_buffer.sls

## How to Port to Another Part ##

Open the "Project Properties" and navigate to the "C/C++ Build -> Board/Part/SDK" item.  Select the new board or part to target and "Apply" the changes.  Note: there may be dependencies that need to be resolved when changing the target architecture.
