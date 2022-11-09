# Flash Blank Check Using the GPCRC #
![Type badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_gpcrc_blank_check_common.json&label=Type&query=type&color=green)
![Technology badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_gpcrc_blank_check_common.json&label=Technology&query=technology&color=green)
![License badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_gpcrc_blank_check_common.json&label=License&query=license&color=green)
![SDK badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_gpcrc_blank_check_common.json&label=SDK&query=sdk&color=green)
![Build badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_gpcrc_blank_check_build_status.json)
![Flash badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_gpcrc_blank_check_common.json&label=Flash&query=flash&color=blue)
![RAM badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_gpcrc_blank_check_common.json&label=RAM&query=ram&color=blue)

## Summary ##

This project benchmarks the performance of blank checking a page of flash using the GPCRC fed by the LDMA vs. brute forcing blank checking with the CPU.

Modules used: CMU, EMU, GPCRC, GPIO, LDMA, TIMER, USART (for VCOM).

## Gecko SDK version ##

v2.7.x or v3.x

## Hardware Supported ##

* For Series 1 EFM32 devices:
  * EFM32PG1 Starter Kit (SLSTK3401A)
  * EFM32PG12 Starter Kit (SLSTK3402A)
  * EFM32GG11 Starter Kit (SLSTK3701A)
  * EFM32TG11 Starter Kit (SLSTK3301A)
* For Series 1 EFR32 devices, the Wireless Starter Kit (WSTK) Mainboard (SLWMB4001A, formerly BRD4001A) is required along with one of these supported radio boards:
  * BGM111 Bluetooth Module Radio Board (SLWRB4300A, formerly BRD4300A)
  * BGM121 Bluetooth SiP Module Radio Board (SLWRB4302A, formerly BRD4302A)
  * BGM13P32 Bluetooth SiP Module Radio Board (SLWRB4306B, formerly BRD4306B)
  * EFR32BG1 2.4 GHz 10.5 dBm Radio Board (SLWRB4100A, formerly BRD4100A)
  * EFR32BG12 2.4 GHz 10 dBm Radio Board (SLWRB4103A, formerly BRD4103A)
  * EFR32BG13 2.4 GHz 10 dBm Radio Board (SLWRB4104A, formerly BRD4104A)
  * EFR32FG1 2.4 GHz / 915 MHz Dual Band 19.5 dBm Radio Board (SLWRB4250A, formerly BRD4250A)
  * EFR32FG12 2.4 GHz / 915 MHz Dual Band 19 dBm Radio Board (SLWRB4253A, formerly BRD4253A)
  * EFR32FG13 2.4 GHz / 915 MHz Dual Band 19 dBm Radio Board (SLWRB4255A, formerly BRD4255A)
  * EFR32FG13 2.4 GHz / 868 MHz Dual Band 10 dBm Radio Board (SLWRB4256A, formerly BRD4256A)
  * EFR32FG14 2.4 GHz / 915 MHz Dual Band 19 dBm Radio Board (SLWRB4257A, formerly BRD4257A)
  * EFR32MG1 2.4 GHz 19.5 dBm Radio Board (SLWRB4151A, formerly BRD4151A)
  * EFR32MG12 2.4 GHz 10 dBm Radio Board (SLWRB4162A, formerly BRD4162A)
  * EFR32MG12 2.4 GHz/915 MHz Dual Band 19 dBm Radio Board (SLWRB4164A, formerly BRD4164A)
  * EFR32MG13 2.4 GHz 10 dBm Radio Board (SLWRB4159A, formerly BRD4159A)
  * EFR32MG13 2.4 GHz/915 MHz Dual Band 19 dBm Radio Board (SLWRB4158A, formerly BRD4158A)
  * EFR32MG14 2.4 GHz/915 MHz Dual Band 19 dBm Radio Board (SLWRB4169B, formerly BRD4169B)
* For Series 2 EFR32 devices, the Wireless Starter Kit (WSTK) Mainboard (SLWMB4001A, formerly BRD4001A) is required along with one of these supported radio boards:
  * EFR32xG21 2.4 GHz 20 dBm Radio Board (SLWRB4180A, formerly BRD4180A) Radio Board
  * EFR32xG21 2.4 GHz 10 dBm Radio Board (SLWRB4181A, formerly BRD4181A) Radio Board

## Setup ##

1. Clone the repository with this project from GitHub onto your local machine.

2. From within the Simplicity Studio IDE, select Import -> MCU Project... from the Project menu.

3. Click the Browse button and navigate to the local repository folder, then to the SimplicityStudio folder.

   a. In Simplicity Studio ***v4***, select the ***.slsproj*** file for one of the supported board.

   b. In Simplicity Studio ***v5***, select the ***.sls*** file for one of the supported board.

4. click the Next button twice and then click Finish.

Note that, regardless of the version of Studio and SDK used, the source file for all Series 1 devices is main_s1.c and the source file for all Series 2 devices (currently only EFR32xG21) is main_xg21.c.

## How the Project Works ##

Because the CPU, GPCRC, and LDMA are clocked at the same frequency, execution times are calculated by starting TIMER0 at 0, stopping it when the blank check algorithm is complete, and multiplying the resulting count by the appropriate clock cycle time.

On all devices, the system clock (HFCLK or SYSCLK) is sourced from what is generally the highest frequency oscillator available (the HFXO on Series1 and the DPLL on Series 2), so the execution times shown are generally the fastest possible. On EFM32GG11 and EFM32TG11, the maximum HFRCO frequency is actually higher than the frequency of the HFXO crystal, so the algorithms in this example can run even faster on these devices.

The code flow is as follows:

1.  Initialize the CMU.

This is where the oscillator for the system clock (HFCLK or SYSCLK) is enabled/configured.

2.  TIMER0 is setup for basic operation but is not started.
3.  Basic LDMA initialization is performed.
4.  The GPCRC is setup to calculate the standard IEEE 802.3 polynomial.
5.  stdio functions are retargeted to use USART0 for VCOM.
6.  The TIMER0 counter is started and the brute force CPU blank check routine is executed on the last page of flash.
7.  TIMER0 is stopped, the CPU clock frequency is displayed along with the algorithm execution time, and the result of the blank check (blank or not blank) is printed.
8.  Steps 6 and 7 are repeated, this time with the LDMA-driven GPCRC blank check.
9.  Steps 6 and 7 are again repeated, this time with a hybrid routine wherein the CPU feeds the flash page contents to the GPCRC. This algorithm is potentially useful because it can be used to accelerate flash programming by performing blank checking of the next page in parallel.

## Theory

The simplest way to blank check a page of flash is to see if each byte/halfword/word is erased.

```
bool cpuBlankCheckFlashPage(uint32_t baseAddr)
{
  bool result = true;
  uint32_t i = 0;

  while (i < FLASH_PAGE_SIZE)
  {
    // Check using 32-bit read
    if ((*(uint32_t *)(baseAddr + i)) == 0xFFFFFFFF)
      i += 4;
    else
    {
      result = false;
      i = FLASH_PAGE_SIZE;
    }
  }
  return result;
}
```

There is nothing particularly special about this algorithm. It can be made faster by performing a 64-bit memory read, at least on ARM Cortex-M4/M33 type devices, because the C compiler (GCC in this case) will output a LDRD (load double) instruction if the `*(uint32_t *)(baseAddr + i)` changed to `*(uint64_t *)(baseAddr + i)`. Early exit is also possible by using `(result && (i < FLASH_PAGE_SIZE))` as the `while` condition.

However, regardless of the technique used, the CPU is going to be tied up, potentially for the maximum time required to check a full page, and this can be appreciable on Series 2 devices where flash pages are 8 Kbytes vs. 2 Kbytes on Series 1 devices.

Using a CRC to blank check a flash page is simple enough. A blank flash page of a given size is always going to have the same CRC. In fact, this project includes binary files with 2048 and 8192 bytes of 0xFF, for which the CRC-32 checksums are 0x3f55d17f and 0xb4293435, respectively. These can be verified on a host machine with a tool like HashCheck (http://code.kliu.org/hashcheck/ for Windows) or rhash (http://rhash.sourceforge.net/ on Linux, macOS, and Windows).

Blank checking a flash page with the GPCRC has two benefits:

1. It does not tie up the CPU. Once the LDMA descriptors are programmed and the selected channel is started, the full page of flash is fed to the GPCRC without any CPU intervention.
2. Execution is deterministic. The LDMA always feeds an entire page into the GPCRC, and this is going to have a set execution time (unless other bandwidth intensive LDMA traffic is present).

What this benchmark shows, however, is that the GPCRC + LDMA combination requires more time to blank check a flash page than the CPU alone. The reasons for this are simple:

1. The LDMA can only move 32 bits at a time, so it requires 512 (a 2 KB page on Series 1) or 2048 (an 8 KB page on Series 2) reads from flash and the same number of writes to the GPCRC to do so.
2. While the Cortex-M4/M33 has only a 32-bit compare instruction, the double register load (LDRD) instruction allows the entire flash page to be checked with 256/1024 (Series 1/Series 2) reads and 512 /2048 register compares (Series 1/Series 2).

Simply put, each loop iteration of the CPU-driven blank check takes fewer clock cycles than the equivalent operation using the LDMA and the GPCRC. Furthermore, the LDMA and GPCRC have some non-neglibile setup overhead that adds to the execution time.

Although the CPU cannot be beat in terms of raw speed, use of the LDMA and GPCRC has the benefit of being effectively non-blocking. CPU contention with the LDMA for access to the flash is minimized courtesy of the MSC cache and is non-existent on dual-bank Series 1 devices (EFM32PG12, EFR32xG12, EFM32GG11, and EFM32GG12) when the CPU and LDMA are accessing alternate flash banks. Furthermore, in EFR32 systems, the deterministic execution time of the LDMA and GPCRC may prove useful if there is a need to interleave flash operations with radio traffic.

One final note is that use of the CPU and GPCRC for blank checking are not mutually exclusive. As mentioned above, it would not be particularly complicated to use a single loop index to program the *nth* word of flash in page *A* immediately after loading the *nth* word of page *A+1* into the GPCRC. When the firmware is finished programming page *A*, whether or not page *A+1* is blank is already known.

Even though the flash page programming time is comparable to the execution time of either blank checking algorithm, the time and energy saved by not having to erase a flash page (10s of ms @ around 2 mA) makes the concurrent program and blank check technique particularly efficient.
