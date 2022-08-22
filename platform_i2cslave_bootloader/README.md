# I2C slave bootloader example for EFR32xG1x and EFR32xG2x devices #
![Type badge](https://img.shields.io/badge/Type-Virtual%20application-green)
![Technology badge](https://img.shields.io/badge/Technology-Platform-green)
![License badge](https://img.shields.io/badge/License-Zlib-green)
![SDK badge](https://img.shields.io/badge/SDK-v3.1.0-green)
![GCC badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_i2cslave_bootloader_gcc.json)

## Summary ##

This is an example bootloader which can accept data as I2C slave. Also contains an example master on RPi3 for testing purposes.

## Gecko SDK version ##

v3.1

## Hardware Required ##

EFR32xGxx device + a master (for example, a Raspberry Pi 3+)

## Connections Required ##

I2C wiring between the master and the slave.

## Setup ##

For further info for master and the bootloader setup, see : \
[Raspberry 3+ example master documentation](i2ctester_Rpi3/doc/index.md)\
[Bootloader documentation](doc/index.md).

## How It Works ##

It uses proprietary communication, see [Bootloader documentation](doc/index.md#figure-1).

## .sls Projects Used ##

platform_i2cslave_bootloader.sls

## How to Port to Another Part ##

See [bootloader documentation](doc/index.md#Configuration).

## Special Notes ##

