# EM3 Sleep with Micrium Using the Cryotimer #

## Summary ##

This project allows an MCU running Micrium to enter EM3 sleep using the cryotimer.

## Gecko SDK Version ##

v2.5

## Hardware Required ##

- SLSTK3701A EFM32 Giant Gecko GG11

## Setup ##

Import the included .sls file to Studio then build and flash the project to the SLSTK3701A. The LED on the device should being blinking. Use the Energy Profiler to measure the power consumption when the device has entered EM3 sleep. Note that the bsp_tick_rtcc.c file has been modified to use the cyrotimer instead. This file was not renamed inorder to not break the project's include paths.

## How it Works ##

This project works by replacing the standard bsp_tick_rtcc.c with a custom one that uses the cryotimer and ULFRCO as the RTOS's system timer. The device will enter EM3 sleep from the idle task hook. You will see the least current consumption if you sleep for time periods which are powers of two. You can see the effect of this by changing the value of the dly parameter in the call to OSTimeDly() on line 237 of ex_main.c

## .sls Projects Used ##

rtos_micrium_cryotimer_em3_sleep_gg11.sls

## How to Port to Another Part ##

1. Create a new example based on the micrium blink example
2. Replace the contentes of the generated ex_main.c with the one from the repo.
3. You will need to make a copy os_cfg.h from the SDK before modifying it.
   - It can be found by opening os.h in "kernel/include".
   - On line 97 you will see os_cfg.h referenced.
   - Right click on os_cfg.h and click "Open Declaration".
   - Try making a change to the file and Simplicty will ask if you'd like to make a copy of the file. Allow it to make the copy, then you will find the file in "external_copied_files".
4. Replace the contentes of the copied os_cfg.h with the one from the repo.
5. Add bsp_tick_rtcc.c from this repo to "BSP/siliconlabs/generic/source" folder.
6. Add em_cryotmer.c to the "emlib" folder.
