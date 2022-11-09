# Edge Counting Using the EFM32/EFR32 Series 1 Pulse Counter (PCNT) #
![Type badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_pcnt_edge_counter_series1_common.json&label=Type&query=type&color=green)
![Technology badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_pcnt_edge_counter_series1_common.json&label=Technology&query=technology&color=green)
![License badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_pcnt_edge_counter_series1_common.json&label=License&query=license&color=green)
![SDK badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_pcnt_edge_counter_series1_common.json&label=SDK&query=sdk&color=green)
![Build badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_pcnt_edge_counter_series1_build_status.json)
![Flash badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_pcnt_edge_counter_series1_common.json&label=Flash&query=flash&color=blue)
![RAM badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_pcnt_edge_counter_series1_common.json&label=RAM&query=ram&color=blue)

## Summary ##

This project demonstrates a way to count edges using the Pulse Counter (PCNT) on Series 1 EFM32 and EFR32 devices.

Although it might seem that a hardware module intended to count pulses ought to also be able to count edges, this is not the case with the PCNT. Specifically, a pulse is counted when either a rising or falling edge is detected on the S0IN pin depending on the value of the PCNT_CTRL_EDGE bit.

However, by using the PRS GPIO producer and the GPIO edge detection logic, it is possible to turn edges into countable pulses.

Modules used: CMU, EMU, GPIO, PCNT, and PRS.

## Gecko SDK version ##

v2.7.x (should work as-is under Gecko SDK 3.0)

## Hardware Required ##

* Wireless Starter Kit (WSTK) Mainboard (SLWMB4001A, formerly BRD4001A) and one of the following:

  * EFR32BG1 2.4 GHz 10.5 dBm Radio Board (SLWRB4100A)
  * EFR32BG12 2.4 GHz 10 dBm Radio Board (SLWRB4103A)
  * EFR32BG13 2.4 GHz 10 dBm Radio Board (SLWRB4104A)
  * EFR32FG1 2.4 GHz/915 MHz 19.5 dBm Radio Board (SLWRB4250A)
  * EFR32FG12 2.4 GHz/915 MHz 19 dBm Radio Board (SLWRB4253A)
  * EFR32FG13 2.4 GHz/915 MHz 19 dBm Radio Board (SLWRB4255A)
  * EFR32FG14 2.4 GHz/915 MHz 19 dBm Radio Board (SLWRB4257A)
  * EFR32MG1 2.4 GHz MHz 19.5 dBm Radio Board (SLWRB4151A)
  * EFR32MG12 2.4 GHz MHz 10 dBm Radio Board (SLWRB4162A)
  * EFR32MG13 2.4 GHz/915 MHz 19 dBm Radio Board (SLWRB4158A)

* Or one of the following:

  * EFM32GG11 Starter Kit (SLSTK3701A)
  * EFM32PG1 Starter Kit (SLSTK3401A)
  * EFM32PG12 Starter Kit (SLSTK3402A)
  * EFM32TG11 Starter Kit (SLSTK3301A)

## Setup ##

Clone the repository with this project from GitHub onto your local machine.

From within the Simplicity Studio IDE, select Import -> MCU Project... from the Project menu. Click the Browse button
and navigate to the local repository folder, then to the SimplicityStudio folder, select the .slsproj file for the
board, click the Next button twice, and then click Finish.

## How the Project Works ##

As noted above, the PCNT recognizes either a rising or falling edge as a pulse. To count all edges requires some logic outside of the PCNT to perform edge-to-pulse conversion.

Fortunately, between the GPIO and PRS modules and the ability of the PCNT to accept input from the PRS, EFM32 and EFR32 devices have this capability.

The first part of this equation, the edge-to-pulse conversion, is handled by the GPIO and PRS. Typically, the GPIO edge detection logic is used to configure a pin for interrupt capability by enabling detection of the desired edge(s).  For example...

    GPIO_ExtIntConfig(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, BSP_GPIO_PB0_PIN, true, false, true);

...requests interrupts upon detection of rising edges on the GPIO pin connected to STK/WSTK push button 0. By contrast...

    GPIO_ExtIntConfig(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, BSP_GPIO_PB0_PIN, true, true, false);

...detects both rising and falling edges on this pin but with no subsequent interrupt requests. This becomes useful when the PRS GPIO high (pins [15:8]) or low (pins [7:0]) producer is used to generate pulses in response to edges. In this case, the following code...

    // Select the PRS source/signal depending on the button 0 port/pin
    if (BSP_GPIO_PB0_PIN >= 8)
    {
      source = PRS_CH_CTRL_SOURCESEL_GPIOH;
      signal = (uint32_t)(BSP_GPIO_PB0_PIN - 8);
    }
    else
    {
      source = PRS_CH_CTRL_SOURCESEL_GPIOL;
      signal = BSP_GPIO_PB0_PIN;
    }
    
    // Select GPIO as PRS source and signal for both falling edges
    PRS_SourceSignalSet(PCNT_PRS_CH, source, signal, prsEdgeBoth);

...generates pulses on the specified PRS channel in response to both rising and falling edges from the appropriate GPIO producer (depending on which half of the GPIO port's pins BSP_GPIO_PB0_PIN resides).

With the edge-to-pulse conversion taken care of in hardware, it's a simple matter to configure the PCNT to accept input on the designated PRS channel instead of the S0IN pin:

    PCNT_Init_TypeDef pcntInit = PCNT_INIT_DEFAULT;
    
    pcntInit.mode     = pcntModeExtSingle;
    pcntInit.top      = PCNT_EDGE_COUNT;
    pcntInit.s1CntDir = false;
    pcntInit.s0PRS    = PCNT_PRS_CH;
    pcntInit.filter   = false;
    
    PCNT_Init(PCNT0, &pcntInit);
    
    PCNT_PRSInputEnable(PCNT0, pcntPRSInputS0, true);

This method has some limitations:

1. It can only run in EM0 and EM1 because the PRS pulses are synchronous. Technically, GPIO edge detection is not available in EM2 because it's only possible to detect a change away from the level present on a pin prior to entering EM2.

2. When the PCNT is running in externally clocked mode, the first three edges detected appear to not be counted. This is because they are needed to actually clock the PCNT registers and synchronize them with the HFCLKLE domain.

That said, it's possible to get around #2 by faking the first three pulses:

    for (i = 0; i < 3; i++)
      PRS_PulseTrigger(1 << PCNT_PRS_CH);

After this, each edge is counted as it happens.

The code flow is as follows:

1. Initialize the CMU (HFXO, HFCLKE, and LFACLK).

2. Initialize the GPIO.

  In addition to configuring the GPIO pin connected to push button 0, which is used to generated the edges, the GPIO pin connected to LED 0 is also configured, so that it can be toggled in response to pulse count overflows.

3. Initialize the PRS.

4. Initialize the PCNT.

  In addition to the configuration shown above, the PCNT top value (the value at which the counter rolls over to 0) is set, in this example, to 5. This means that 3 button pushes (each of which causes 2 edges) results in a rollover and PCNT overflow interrupt request.

5. Three dummy pulses are triggered under software control on the specified PRS channel in order to prime the PCNT logic so that the edges associated with subsequent button presses are immediately counted.

6. The device enters EM1 and waits until PCNT_EDGE_COUNT + 1 edges (5 + 1) are detected, at which point the PCNT overflow interrupt is requested, and the associated handler clears the resulting interrupt flag and toggles LED 0 on the STK/WSTK.
  
  **NOTE:** On the EFM32GG11 Starter Kit, LED 0 is initially red because of how the RGB LEDs are wired. Once the first PCNT overflow occurs, the LED is toggled off. On all other STKs and WSTKs, LED 0 is initially off and turns on after the first 3 button presses (5 + 1 edges).


## Porting to Another EFM32/EFR32 Device ##

Apart from any issues of pin availability on a given radio board (e.g. Blue Gecko Module radio boards often use the same GPIO pins for both the push buttons and LEDs), this code should run as-is on any radio board for Series 1 EFR32 devices. For that matter, it should also run as-is on any given Series 1 device on a custom board, again, subject to GPIO pin and PRS channel availability.

Likewise, this code should also run, with minimal changes, on Series 0 EFM32 and EZR32 devices, except for the original EFM32 Gecko, which does not have PCNT PRS input capability. Note that Series 0 devices do not support the stretching of PRS pulses when crossing clock domains, which would likely limit the maximum achievable edge detection frequency.
