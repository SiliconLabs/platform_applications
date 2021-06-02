# I2C slave bootloader example for EFR32xG1x and EFR32xG2x devices #

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

