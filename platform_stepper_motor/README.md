# Stepper Motor Example #
![Type badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_stepper_motor_common.json&label=Type&query=type&color=green)
![Technology badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_stepper_motor_common.json&label=Technology&query=technology&color=green)
![License badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_stepper_motor_common.json&label=License&query=license&color=green)
![SDK badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_stepper_motor_common.json&label=SDK&query=sdk&color=green)
![Build badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_stepper_motor_build_status.json)
![Flash badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_stepper_motor_common.json&label=Flash&query=flash&color=blue)
![RAM badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_stepper_motor_common.json&label=RAM&query=ram&color=blue)

## Summary ##

This project uses an EFM32GG11 to drive a unipolar 4-phase stepper motor. A TIMER is used to set the angular speed of the motor, and the GPIO is used to drive the individual motor coils. The application goes into EM1 after setup. When the left button is pressed (BTN1), the motor rotates counter-clockwise by 90 degrees. When the left button is pressed (BTN0), the motor rotates clockwise by 90 degrees.

Peripherals used: TIMER, GPIO, EMU

## Gecko SDK version ##

v3.0

## Hardware Required ##

- One SLSTK3701A Giant Gecko GG11 Starter Kit
<https://www.silabs.com/products/development-tools/mcu/32-bit/efm32-giant-gecko-gg11-starter-kit>
- One Stepper Motor. This example was tested using the 28BYJ-48 motor, but any unipolar 4-phase stepper motor should work, as long as the speed and full rotation step count is updated according to the specific motor.
<https://components101.com/motors/28byj-48-stepper-motor>
- One ULN2003 Driver Board. The motor requires much more current than the GG11 STK's GPIOs can provide, so the driver board is required drive the motor coils with enough current.

## Setup ##

Connect the GG11 STK's pins with the ULN2003 driver board, and connect the 28BYJ-48 stepper motor to the ULN2003 driver board via the connector.

The final connections should looks like so:

| GG11 | ULN2003 Input | ULN2003 Output | 28BYJ-48        |
|------|---------------|----------------|-----------------|
| PC5  | IN1           | OUT1           | Coil 1 (Orange) |
| PC4  | IN2           | OUT2           | Coil 3 (Yellow) |
| PA13 | IN3           | OUT3           | Coil 2 (Pink)   |
| PA12 | IN4           | OUT4           | Coil 4 (Blue)   |
| 5V   | VCC/VM        | VM             | 5V (Red)        |
| GND  | GND           |                |                 |


Import the included .sls file to Studio then build and flash the project to the SLSTK3701A STK.
In Simplicity Studio select "File->Import" and navigate to the directory with the .sls project file.
The project is built with relative paths to the STUDIO_SDK_LOC variable which was defined as

C:\SiliconLabs\SimplicityStudio\v4\developer\sdks\gecko_sdk_suite\v3.0

## How the Project Works ##

The application sits in EM1 until an interrupt occurs. The push buttons on the GG11 Starter Kit is used to start the TIMER and choose a rotation direction. TIMER1 is set to overflow at a frequency of 200 Hz and set to interrupt in an overflow event. In the TIMER1 interrupt handler, the software sets the coils to the next state in order to step the motor. In order to rotate the motor counter-clockwise, the motor coils need to be driven in the following order: Coil 1 -> Coil 3 -> Coil 2 -> Coil 4. In order to rotate the motor clockwise, the motor coils need to be driven in the following order: Coil 4 -> Coil 2 -> Coil 3 -> Coil 1. The calculateSteps() function determines the number of full steps required to rotate by a specified angle. The desired delta angle can be set using the ANGLE_PER_TRIGGER macro. TIMER1 will continue to interrupt until the motor shaft rotates by the desired angle. Once the desired angle is reached, TIMER1 stops and the application waits for the next pushbutton press.

## .sls Projects Used ##

gg11_stepper_motor.sls

## How to Port to Another Part ##

Open the "Project Properties" and navigate to the "C/C++ Build -> Board/Part/SDK" item.  Select the new board or part to target and "Apply" the changes.  Note: there may be dependencies that need to be resolved when changing the target architecture.
