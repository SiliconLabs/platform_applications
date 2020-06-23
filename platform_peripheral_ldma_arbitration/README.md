# Linked DMA (LDMA) Arbitration #

## Summary ##

This project demonstrates various arbitration configurations of multiple transfers, each consisting of 256 words transferred from WTIMER0→CNT to memory buffers. Transfers are configured to arbitrate after 32 words. Arbitration changes order in which data is copied. After transfer, the order is reconstructed and shown on Virtual COM.

Peripherals used: LDMA, TIMER, WTIMER

## Gecko SDK version ##

v2.7

## Hardware Required ##

- One SLSTK3701A Giant Gecko GG11 Starter Kit
<https://www.silabs.com/products/development-tools/mcu/32-bit/efm32-giant-gecko-gg11-starter-kit>

## Setup ##

Import the included .sls file to Studio then build and flash the project to the SLSTK3701A STK.
In Simplicity Studio select "File->Import" and navigate to the directory with the .sls project file.
The project is built with relative paths to the STUDIO_SDK_LOC variable which was defined as

C:\SiliconLabs\SimplicityStudio\v4\developer\sdks\gecko_sdk_suite\v2.7

## How the Project Works ##

WTIMER0 is setup as a sequential number source.

TIMER0 is used to trigger all pending DMA transfers

All DMA transfers are setup to transfer 32 words per arbitration
All transfers copy the word WTIMER->CNT to a memory location.
By following sequence of copied numbers, the order in which
they were copied can be traced.

Different behaviors can be observed by varying:

- the number of fixed channels
- the number of arbitration slots assigned to a channel

Virtual COM is used to display results:

![VCOM output when running the example](./doc/mcu_peripheral_ldma_vcom_output.png)

The number in the row indicates the channel that is active, each row represents a transfer of 32 words.  Foe example, first three rows indicates 32 words were transferred on ch2, then 32  words on ch3, then 32 words on ch2.

## .sls Projects Used ##

mcu_peripheral_ldma_p2m_arbitration_gg11.sls

## How to Port to Another Part ##

Open the "Project Properties" and navigate to the "C/C++ Build -> Board/Part/SDK" item.  Select the new board or part to target and "Apply" the changes.  Note: there may be dependencies that need to be resolved when changing the target architecture.
