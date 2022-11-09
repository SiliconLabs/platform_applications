# xG21 Low-Power IADC Use with Timestamping #
![Type badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_timestamped_adc_em2_series2_common.json&label=Type&query=type&color=green)
![Technology badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_timestamped_adc_em2_series2_common.json&label=Technology&query=technology&color=green)
![License badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_timestamped_adc_em2_series2_common.json&label=License&query=license&color=green)
![SDK badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_timestamped_adc_em2_series2_common.json&label=SDK&query=sdk&color=green)
![Build badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_timestamped_adc_em2_series2_build_status.json)
![Flash badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_timestamped_adc_em2_series2_common.json&label=Flash&query=flash&color=blue)
![RAM badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_timestamped_adc_em2_series2_common.json&label=RAM&query=ram&color=blue)

## Summary ##

This project collects a user-specified number of IADC conversion results and saves a timestamp for each one all
while operating in EM2. Conversions are performed in response to an external trigger that is provided via GPIO
pin. Because the PRS routes the conversion start pulse from a GPIO pin to the IADC, it would be a simple matter
to use another triggering event that might be available in EM2, such as a RTCC compare match event.

As with any use of the IADC in EM2, energy use is directly proportional to the IADC sampling rate, e.g. not the
IADC conversion clock frequency but the number of samples collected over a given period of time. The IADC clock
is set to 1 MHz in this example and is provided by the HFRCOEM23. Current draw when sampling at 100 Hz is as
follows on the different boards:

| Radio Board |            Target            | Current (uA) | AVDD Supply (V) |
|-------------|------------------------------|--------------|-----------------|
|  BRD4179B   | EFR32MG21A010F1024 + EFP0104 |     28.3     |       1.8       |
|  BRD4180A   | EFR32MG21A020F1024 (20 dBm)  |     49.0     |       3.3       |
|  BRD4181A   | EFR32MG21A020F1024 (10 dBm)  |     48.5     |       3.3       |

Modules used: CMU, EMU, GPIO, IADC, LDMA, PRS, RTCC.

## Gecko SDK version ##

v2.7.x

## Hardware Required ##

* Wireless Starter Kit (WSTK) Mainboard (SLWMB4001A, formerly BRD4001A)
* EFR32xG21 + EFP 2.4 GHz 20 dBm Radio Board (BRD4179B)

## Setup ##

Clone the repository with this project from GitHub onto your local machine.

From within the Simplicity Studio IDE, select Import -> MCU Project... from the Project menu. Click the Browse button
and navigate to the local repository folder, then to the SimplicityStudio folder, select the .sls file for the board,
click the Next button twice, and then click Finish.

## How the Project Works ##

Configurable parts of the example, such as the pins used, HFRCOEM23 frequency band, and IADC clocks are #defines
located along with extensive comments among the first 150 lines at the beginning of the source file. The IADC
conversions are triggered by pulses on the selected input pin, and must be provided by an external device. The
pg12_iadc_stimulus example project can be used for this purpose and is readily ported to other devices. It also
outputs a voltage using the VDAC that can be connected to the IADC input on the EFR32xG21 to demonstrate that the
analog-to-digital conversions are occurring. Of course, any other suitable voltage input can be used so long as
it is between 0 and the VDDX reference supply (the 1.8V AVDD supplied by the EFP0104 on BRD4179B and the 3.3V
VMCU supply on BRD4180A and BRD4181A).

Existing peripheral examples show how to perform conversions using the IADC in EM1P (the system has entered EM2,
but the IADC has started the HFRCOEM23 in order to perform a conversion and have the LDMA save the result). The
complicating factor demonstrated in this project is the saving of a timestamp. On first blush, it might seem that
the way to do this is to configure the same pin to be both the IADC conversion trigger and an input captured by the
RTCC. LDMA descriptors for both the IADC and the RTCC would transfer the conversion result and the timestamp.

Simplicity ought to rule the day in this example save for the fact that only the IADC can wake the system by
starting the HFRCOEM23, which clocks not only the IADC but also the internal peripheral bus interface, the LDMA,
and the RAM. The same pulse that starts the IADC would be captured by the RTCC before the analog conversion is
complete. Because the IADC only wakes the LDMA to transfer the result AFTER the conversion is complete, the captured
timestamp would never be transferred from the RTCC_CCn_ICVALUE register to RAM. This also happens to ignore the even
simpler fact that the RTCC is not able to request LDMA service (see Table 25.3. LDMA Source Selection Details in
the EFR32xG21 Reference Manual). 

Fortunately, how the LDMA arbitrates among multiple channels requesting service is governed by the fact that, by
default, all channels have fixed priority. While this can be changed by the LDMA_CTRL_NUMFIXED bit field such that
all channels above and including the specified channel are permitted to use round robin prioritzation, all
lower-numbered channels retain fixed priority, where channel N has higher priority than channel N + 1. This fixed
prioritization of channels is what this example takes advantage of to make timestamping work.

Consider, then, the following: If LDMA channel N is assigned to transfer the timestamp value from the RTCC (the CNT
register is used in this example, but it would be a simple matter to configure the RTCC's input capture capability
and transfer the timestamp from the RTCC_CCn_ICVALUE register) and channel N + 1 is assigned to transfer the
conversion result, both channels will be requesting service simultaneously once the IADC conversion is complete.
The fixed prioritization of channels means that the LDMA has to transfer the timestamp first, thus preventing
the IADC from placing the system back into EM2 until after the channel that transfers the conversion result has
been serviced. 

Q.E.D.

As long as two adjacent LDMA channels can be allocated to retain fixed prioritzation such that the lower-numbered
channel is assigned to the timestamp transfer and the higher-numbered adjacent channel is assigned to transfer the
conversion result, this technique can always be used and also overcomes the fact that the RTCC has no means of
requesting LDMA service. It does require use of a second PRS channel to request the timestamp transfers (think of
this as an old-fashioned hardware DMA request input as might be found on classic M68000 family devices), but no
additional pin is required beyond that already configured as the IADC trigger input because there is nothing
preventing two PRS channels from being assigned to the same GPIO input pin.   

The code flow is as follows:

1. Initialize the EFP (when run on BRD4179B).

2. Configure operation in EM2/3 and set the HFRCOEM23 to the selected tuning band. Check for button 0
to prevent going into EM2 and losing debug access.

3. Setup the RTCC for timestamping (default parameters save for the prescaler which is set to 1 so that each
counter tick is 1/32768 seconds).

4. Setup the IADC. Options specific to this example include configuring the clock for 1 MHz operation such that
conversions take 10 us (10 clocks) and enabling wake-up from EM2 when the FIFO has a single entry.

5. Initialize two LDMA channels with separate descriptors: one is triggered by the LDMAXBAR_DMA_PRSREQ0 requester
(the hardware input pin for the timestamp transfers) and the other is triggered by IADC0_DMA_IADC_SINGLE (the single
conversion complete requester).

6. Initialize the GPIO used to trigger conversions, including setting up the PRS producer (GPIO pin) and consumer
links between the hardware LDMA PRS requester (CONSUMER_LDMAXBAR_DMAREQ0) and the IADC single conversion trigger
(CONSUMER_IADC0_SINGLETRIGGER).

7. Wait in EM2 until NUM_SAMPLES conversions and timestamps are processed, at which point the CPU will take a
software breakpoint so that the results can be inspected in Simplicity Studio's Expressions viewer.

As noted above, the stimulus this example code expects are a sequences of pulses on the trigger pin (expansion
header pin 12) and an analog voltage between 0 and VDDX on the specified analog input pin (expansion header pin 11).
The easiest way to do this is to install the pg12_iadc_stimulus example project on an EFM32PG12 Starter Kit
(SLSTK3402A) and connect its outputs to the WSTK mainboard via jumper wires. The stimulus project is readily ported
to other Series 1 EFM32/EFR32 devices that have a VDAC. Minimal changes related to GPIO configuration would be
needed to run the code on a Series 2 device (e.g. another EFR32xG21 board), and the analog voltage would have to be
generated through alternative means as neither the EFR32xG21 nor the EFR32xG22 has a VDAC. 

| Radio Board | Trigger Input | Analog Input | VDDX Reference (V) |
|-------------|---------------|--------------|--------------------|
|  BRD4179B   |  PA5 / EXP12  |  PD2 / EXP11 |         1.8        |
|  BRD4180A   |  PA5 / EXP12  |  PB0 / EXP11 |         3.3        |
|  BRD4181A   |  PA5 / EXP12  |  PB0 / EXP11 |         3.3        |

To view the results after NUM_SAMPLES are collected, select the Expressions panel in the Simplicity Studio debugger
and add the 'valbuffer' and 'timebuffer' global arrays.

## Porting to Another EFR32 Series 2 Device ##

Apart from any issues of pin availability on a given radio board, this code should run as-is on any radio board for
EFR32xG21 or any module that is based on EFR32xG21, such as BGM210L, BGM210P, MGM210L, and MGM210P.

The code can also run on EFR32xG22, although this has not been tested. However, all module clocks --- GPIO, IADC, LDMA,
PRS, and RTCC --- would need to be specifically enabled because xG22 does not have the on-demand module clock enable
functionality that is present on xG21.

To change the target board, navigate to Project -> Properties -> C/C++ Build -> Board/Part/SDK. Start typing in the Boards
search box and locate the desired radio board, then click Apply to change the project settings, and go from there.