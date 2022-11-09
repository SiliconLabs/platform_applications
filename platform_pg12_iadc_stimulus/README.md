# xG21 Timestamped IADC Stimulus Generator #
![Type badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_pg12_iadc_stimulus_common.json&label=Type&query=type&color=green)
![Technology badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_pg12_iadc_stimulus_common.json&label=Technology&query=technology&color=green)
![License badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_pg12_iadc_stimulus_common.json&label=License&query=license&color=green)
![SDK badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_pg12_iadc_stimulus_common.json&label=SDK&query=sdk&color=green)
![Build badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_pg12_iadc_stimulus_build_status.json)
![Flash badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_pg12_iadc_stimulus_common.json&label=Flash&query=flash&color=blue)
![RAM badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_pg12_iadc_stimulus_common.json&label=RAM&query=ram&color=blue)

## Summary ##

The sole purpose of this project is to generate stimulus for the
EFR32xG21 Timestamped IADC in EM2 example, which expects conversion
start pulses on the selected digital input pin and some analog voltage
on the selected analog input pin.

All this program does is, in response to PB0 being pressed, generate a
set number of 1 microsecond pulses spaced at 10 microseconds (100 Hz)
while a fixed analog voltage is output by the VDAC. After the specified
number of pulses are output, the program again waits for PB0 to be
pressed and repeats the pulse output sequence.

While not very useful by itself, the project could serve as the basis
for a testbed that generates other kinds of synchronized analog or
digital stimulus for another device, such as another EFM32 or  EFR32
family member that needs to collect some kind of data while, ideally,
remaining in EM2 to minimize energy use.
  
Modules used: CMU, EMU, GPIO, SYSTICK, and VDAC0.

## Gecko SDK version ##

v2.7.x

## Hardware Required ##

* EFM32PG12 Starter Kit (SLSTK3402A)
* Jumper wires to connect the outputs to another (W)STK

## Setup ##

Clone the repository with this project from GitHub onto your local machine.

From within the Simplicity Studio IDE, select Import -> MCU Project...
from the Project menu. Click the Browse button and navigate to the
local repository folder, then to the SimplicityStudio folder, select
the .slsproj file for the board, click the Next button twice, and then
click Finish.

Connect expansion header pins 3 and 16 to the digital and analog inputs
specified in the README.md file for the EFR32xG21 Timestamped IADC
project.

## How the Project Works ##

1. Initialize the DCDC.

2. Setup VDAC0 and output a voltage that is 50% of full scale
   (0.625V @ VREF = 1.25V) on the selected analog output pin (PC10,
   which is pin 16 on the EFM32PG12 STK expansion header).

3. Configure SYSTICK for 1 ms ticks.

This code includes a set of SYSTICK utility functions in the
Drivers/systick.* files that can be useful for other purposes.

4. Setup the two GPIOs used (PC9 for the pulse output and the
   BSP-specified pin connected to push button 0).

5. Wait in EM1 for PB0 to be pressed.

6. Disable the PB0 interrupt.

7. Output the set number of pulses at 100 Hz on PC9 by driving the pin
   high for 1 microsecond and then low for 9 microseconds.

8. Re-enable the PB1 interrupt and go back to step 5.

## Porting to Another EFR32 Series 1 Device ##

Apart from any issues of pin availability on the expansion header for a
given board, this code should run as-is on any device with a VDAC. This
would preclude devices that only have the IDAC (e.g. EFM32JG1, EFM32PG1,
and EFR32xG1), although it would be a simple matter to use the IDAC for
the same purpose by connecting an appropriately sized resistor to the
output pin.

To change the target board, navigate to Project -> Properties -> C/C++ Build -> Board/Part/SDK.
Start typing in the Boards search box and locate the desired radio board,
then click Apply to change the project settings, and go from there.