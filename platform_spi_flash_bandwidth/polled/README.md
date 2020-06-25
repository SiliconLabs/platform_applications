# xG21 Polled SPI Throughput Tester #

## Summary ##

This project calculates the bandwidth achievable when data is read from a SPI flash memory (effectively, any device
compatible with the now industry-standard command set first implemented on the Numonyx M25P40) using polled transfers
with the USART operating in synchronous mode.

Because this code benchmarks read performance, there is no need to connect an actual SPI flash device to the EFR32xG21.
The timing of read operations is gated by the timing achievable with the USART and the GPIO pins that would otherwise
interface to such a device. These pins are driven as they would be if connected to an actual IC and can be observed on
an oscilloscope.

Modules used: CMU, EMU, GPIO, SYSTICK, USART0 (for VCOM), and USART2 (SPI flash).

## Gecko SDK version ##

v2.7.x

## Hardware Required ##

* Wireless Starter Kit (WSTK) Mainboard (SLWMB4001A, formerly BRD4001A)
* EFR32xG21 2.4 GHz 20 dBm Radio Board (SLWRB4180A)

## Setup ##

Clone the repository with this project from GitHub onto your local machine.

From within the Simplicity Studio IDE, select Import -> MCU Project... from the Project menu. Click the Browse button
and navigate to the local repository folder, then to the SimplicityStudio folder, select the .slsproj file for the
board, click the Next button twice, and then click Finish.

Note that building the project will generate a warning about the 'data' variable being unused.  This can be safely
ignored but is necessary so that the value return by the call to USART_SpiTransfer() is written to memory.

## How the Project Works ##

The code offers a fair degree of configurability, so it is a simple matter to change the amount of data read or,
in particular, the frequency of the USART module clock. The DPLL is used to generate the SYSCLK, which is the top-level
clock from which the HCLK (bus clock) and PCLK (synchronous peripheral clock) are derived, and initialization
structures are present in the code (all but one of which is commented out) to set the DPLL output to 40, 50, and
80 MHz, which results in PCLK frequencies of 40, 50, and 40 MHz, respectively.

After sending the SPI flash read command and the 24-bit address, 1 Mbyte of dummy data is clocked out the TX pin
(which the SPI flash would ignore) while 1 Mbyte of data is clocked in on the RX pin in blocks of 1 Kbyte at a time.
The SYSTICK timer is started and stopped immediately before and after the read sequence, and the difference between
the start and end times is used to calculate bandwidth.

The code flow is as follows:

1.  Initialize the CMU.

This is where the DPLL is configured for the desired SYSCLK frequency. The HFXO is started first, as it is used as the
reference clock for the DPLL.

2.  The SYSTICK timer is configured for 1 ms ticks.

This code includes a set of SYSTICK utility functions in the Drivers/systick.* files that can be useful for other purposes,

3.  stdio functions are retargeted to use USART0 for VCOM.

4.  USART2 is initialized for operation in synchronous mode (initSPI2).

The divider is set for the maximum possible clock frequency (PCLK / 2), and the USART is set to transfer and receive data
MSB first as this is the standard for SPI devices and what any M25P40-compatible flash expects. The clock phase (CLKPHA)
and clock polarity (CLKPOL) are both set to 0, which is often called SPI mode 0, which means that (a) the clock is inactive
low and the master (b) drives data on the leading (rising) edge of the clock and (c) samples data on the falling edge of
the clock.  

Because the delays through the EFR32 GPIO multiplexing logic are relatively long, it is necessary to enable synchronous
master sample delay (USART_CTRL_SMSDELAY), which results in input data being sampled on the subsequent clock edge. In the
case of SPI mode 0 operation, as used here, input data is sampled not on the falling edge of the clock but on the next
rising edge of the clock.  This is perfectly allowable and expected because any modern SPI flash device is going to support
clock rates well in excess (100 MHz is not unusual) of the maximum 50 MHz read frequency supported by the original M25P40.
The slave device will not change the data it drives on its transmit output until this edge is received, at which point the
master will have already latched it.

The GPIO pins used by USART2 are configured, and the fastest possible slew rate is set so that the clock is a well-defined
square. Note that the USART RX, TX, and CLK pins all use the same GPIO port for this reason, although, technically the slew
rate setting applies only to CLK and TX, as these are outputs. Because the chip select (CS) is a manually controlled GPIO
and is not timing-critical, any GPIO pin can be used.

5. The SYSTICK interrupt is enable and the start time is synchronized (saved  as soon as the SYSTICK counter is incremented).

6. The read sequence is started by asserting the SPI flash chip select (driving a GPIO low), sending the READ command, and
sending the 24-bit address.

7. A for-loop transmits a dummy byte the specified number of times and reads the data that would've been clocked in from the
receive data register. This code could be made interrupt-driven, although only minimal additional bandwidth would be
available (receiving a byte takes 320 ns at 40 MHz and, assuming the Cortex-M33 interrupt latency is the same 16 clocks as
the Cortex-M4, simply entering the USART transmit complete interrupt at 80 MHz would require 200 ns).

8. The SYSTICK count is captured and its interrupt is disabled.

9. The data transfer rate is calculated and displayed.

## Porting to Another EFR32 Series 2 Device ##

Apart from any issues of pin availability on a given radio board, this code should run as-is on any radio board for EFR32xG21
or any module that is based on EFR32xG21, such as BGM210L, BGM210P, MGM210L, and MGM210P.

The code can also run on EFR32xG22, although this has not been tested. However, all module clocks — GPIO, LDMA, and USART2
(retargetio.enables the clock for whatever USART it is configured to use, which is USART0 in this case) — because xG22
does not have the on-demand module clock enable functionality that is present on xG21. 

To change the target board, navigate to Project -> Properties -> C/C++ Build -> Board/Part/SDK. Start typing in the Boards
search box and locate the desired radio board, then click Apply to change the project settings, and go from there.