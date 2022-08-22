# TensorFlow Micro Speech
![Type badge](https://img.shields.io/badge/Type-Virtual%20application-green)
![Technology badge](https://img.shields.io/badge/Technology-Platform-green)
![License badge](https://img.shields.io/badge/License-Zlib-green)
![SDK badge](https://img.shields.io/badge/SDK-v3.0.0-green)
![GCC badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_tensorflow_micro_speech_gcc.json)

## Summary

This project is a Simplicity Studio project file for the tensorflow micro_speech demo. This demo has been ported to the EFM32GG11 Starter Kit as well as the Thunderboard Sense 2 in [SiliconLabs/tensorflow](https://github.com/SiliconLabs/tensorflow/tree/silabs_stk3701a_port).

The Simplicity Studio project allows for testing and development of the micro_speech demo within the Studio development environment, as well as allowing for use of the Studio debugger, energy profiler, and flash programming utility.

## Gecko SDK version

v3.0 or later

## Hardware Required

[SLSTK3701A EFM32GG11 Starter Kit](https://www.silabs.com/development-tools/mcu/32-bit/efm32gg11-starter-kit)

**OR**

[Thunderboard Sense 2](https://www.silabs.com/development-tools/thunderboard/thunderboard-sense-two-kit)

## Setup

Import the included .sls file into Studio, then build and flash the project to the starter kit.
In Simplicity Studio select "File->Import" and navigate to the directory with the .sls project file.


## How the project works

This project consists of the TensorFlow micro_speech example code and Silicon Labs driver code that interfaces between the microphone, LEDs, and TensorFlow model on supported boards.

The Silicon Labs developed driver code can be viewed in the Drivers/ and src/ directory. The TensorFlow sources are unmodified by this example and can be obtained from [SiliconLabs/tensorflow](https://github.com/SiliconLabs/tensorflow/tree/silabs_stk3701a_port)

The microphone driver in Drivers/microphone uses the USART and the DMA to sample audio data from the onboard i2s microphones. The input from the microphone driver is provided to the TensorFlow model in the audio_provider.cc source file. The tensorflow code then runs preprocessing, inference, and postprocessing on the audio data and outputs the result to the command_responder.cc file, which is configured to control the LEDs on the supported boards.

For more information about TensorFlow and the micro_speech example code, see the following third party resources:
* [TensorFlow Lite Micro](https://www.tensorflow.org/lite/microcontrollers)
* [TensorFlow Micro Speech Readme](https://github.com/SiliconLabs/tensorflow/tree/silabs_stk3701a_port/tensorflow/lite/micro/examples/micro_speech)
* [TinyML Textbook Chapter 7](https://www.oreilly.com/library/view/tinyml/9781492052036/ch07.html)

## How to port to other devices

Support will need to be added to the microphone driver found in the Drivers/ folder for that specific board. Additionally the audio_provider.cc and command_responder.cc files will need modification to support the target board and application.

