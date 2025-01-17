# AN0816: Brushless DC Motor Control #

![Type badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_bldc_motor_controller_common.json&label=Type&query=type&color=green)
![Technology badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_bldc_motor_controller_common.json&label=Technology&query=technology&color=green)
![License badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_bldc_motor_controller_common.json&label=License&query=license&color=green)
![SDK badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_bldc_motor_controller_common.json&label=SDK&query=sdk&color=green)
![Build badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_bldc_motor_controller_build_status.json)
![Flash badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_bldc_motor_controller_common.json&label=Flash&query=flash&color=blue)
![RAM badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_bldc_motor_controller_common.json&label=RAM&query=ram&color=blue)

## Summary ##

The project is part of AN0816. Brushless DC (BLDC) motors can be driven by applying PWM signals to each terminal in a sequential order. To know when to drive the each terminal, the position of rotor must be known. The rotor position can be determined by either using hall effect sensors or by measuring the back-EMF of the motor terminals (sensorless). The speed of the BLDC motor is proportional to the voltage applied at the terminals. PWM waveform is often used to modulate the perceived voltage on the pins, thus controlling the speed. This project demonstrate how to configure Silicon Labs MCUs to start, stop, increase speed, decrease speed, and change direction of a BLDC motor in sensorless mode.

Peripherals used: TIMER, ACMP, IADC, USART

## SDK version ##

SiSDK v2024.6.2

## Hardware Required ##

* C8051F850 Motor Control Reference Design Board (C8051F850-BLDC-RD)
* CP2102N USBXpress Bridge Development Kit (CP2102N-EK)
* Board: Silicon Labs EFR32xG24 Radio Board (BRD4186C) + Wireless Starter Kit Mainboard
  * Device: EFR32MG24B210F1536IM48

## Connections Required ##

Connect an MCU board via a micro-USB cable to your PC to flash the example.

The table below defines the GPIO pins from the MCU board that connects to the reference design board

| Reference Board | EFM32MG24 Radio Board (BRD4186C) |
| --------------- | -------------------------------- |
| IM_2N           |                                  |
| IM_0N           | PD5 (WSTK Pin 24)                |
| IM_1N           |                                  |
| IM_2P           |                                  |
| VMC             | PC7 (EXP 16)                     |
| IM_0P           | PD4 (WSTK Pin 25)                |
| IM_1P           |                                  |
| VMDC            |                                  |
| VMB             | PC5 (EXP 15)                     |
| VMY             | PC0 (EXP 10)                     |
| ID_SDA          |                                  |
| VMA             | PB5 (EXP 3)                      |
| ID_SCL          |                                  |
| GD2_EN          | GND                              |
| GD1_EN          | GND                              |
| GD0_EN          | GND                              |
| PWM1A           | PA5 (EXP 7)                      |
| PWM2A           | PA6 (EXP 11)                     |
| PWM0A           | PA0 (EXP 5)                      |
| PWM1B           | PA9 (EXP 12)                     |
| PWM2B           | PA9 (EXP 14)                     |
| PWM0B           | PA7 (EXP 13)                     |

The table below defines the GPIO pins from the MCU board that connects to the UART-to-USB bridge

| CP2102N         | EFM32MG24 Radio Board (BRD4186C) |
| --------------- | -------------------------------- |
| UART RX         | UART TX PC1 (EXP 4)              |
| UART TX         | UART RX PC2 (EXP 6)              |
| GND             | GND                              |

## Setup ##

### Create a project based on an example project

1. From the Launcher Home, add your hardware to My Products, click on it, and click on the **EXAMPLE PROJECTS & DEMOS** tab. Find the example project with the filter "bldc".
2. Choose corresponding project for device -> click Create and Finish and Project should be generated.
3. Build and flash this example to the board.
4. Connect the radio board, motor board, and the CP2102N board as described in the Connections Required section 

## How It Works ##

The motor is controlled with the buttons Pushbutton 0 (PB0) and Pushbutton 1 (PB1):
* PB0 – Stop the motor or change direction:
* If the motor is stopped, this button changes direction.
* If the motor is running, this button stops the motor.
* PB1 – Start the motor or adjust speed:
* If the motor is stopped, this button starts the motor.
* If the motor is running, a long press of this button will decrease the speed of the motor, and a short press of this button will increase the speed of the motor.

The motor starts in 2 stages:
1. Commutations are triggered manually to start the motor. The motor is started slowly and the speed of the motor increases linearly until the desired motor speed is reached. 
2. Once the desired motor speed is reached, the ACMP measures the back-EMF to trigger commutations.

The IADC is used to measure the motor current. The motor will be stopped if overcurrent is detected by the IADC.

The USART is configured as a UART to connect with the PC tool. Data is sent to the PC tool for logging, and commands can be sent from the PC tool to configure and control the motor.

For more detailed information, read AN0816 EMF32 Brushless DC Motor Control.

## Testing ##

1. Connect the MCU board to the motor board. Refer [Connections Required](#connections-required) section to know how to connect these boards.
2. Power the motor board with the external jack.
3. Press PB1 on the MCU board to start the motor.
4. Short press PB1 to make the motor spin faster.
5. Long press PB1 to make the motor spin slower.
6. Press PB0 to stop the motor.
7. Press PB0 while the motor is stopped to change directions.

