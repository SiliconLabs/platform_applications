# bgm iadc Example #

## Summary ##

This project uses bgm board to evaluate the EFR32BG22 16 bit ADC performance (14.3 ENOB with 32x OVS). 
a. EMU temperature
b. led
c. button
d. adc calibration

Peripherals used: ADC, GPIO, I2C, USART, EMU, CMU

## Gecko SDK version ##

v3.2.1

## Hardware Required ##

- One WSTK main board
<https://www.silabs.com/products/development-tools/mcu/32-bit/efm32-giant-gecko-gg11-starter-kit>
- One bgm board
<https://components101.com/motors/28byj-48-stepper-motor>
- Which include 
  -- DAC70501 https://www.ti.com/lit/ds/symlink/dac70501.pdf
  -- ADC1220 https://www.ti.com/lit/ds/symlink/ads1220.pdf
  -- REF3312 https://www.ti.com/product/REF3312
- Schematic for the bgm board
<https://components101.com/motors/28byj-48-stepper-motor>
## Setup ##

Connect bgm board with WSTK main board via Simplicity 10 pins adater, and connect WSTK main board to PC via mini USB.
a. Set the debug mode as 'OUT'.
b. set target in Simplicity launcher as EFR32BG22C112F352GM32.
c. read back the secure FW version.
d. flash the bootloader first via commander or flash programmer.
C:\SiliconLabs\SimplicityStudio\v5\developer\sdks\gecko_sdk_suite\v3.2\platform\bootloader\sample-apps\bootloader-storage-internal-single-512k\efr32mg22c224f512im40-brd4182a


The final connections should looks like so:



| EFR32BG22 | REF3312 Input | ULN2003 Output | 28BYJ-48        |
|------|---------------|----------------|-----------------|
| PA0  | IN1           | OUT1           | ADC reference   |
| GND  | GND           |                |                 |


| EFR32BG22 | ADC Input | ULN2003 Output | 28BYJ-48        |
|------|---------------|----------------|-----------------|
| PD0  | IN1           | OUT1           | ADC INPUT   |
| PD1  | IN2           |                | ADC INPUT   |
|------|---------------|----------------|-----------------|
| PC0  | IN1           | OUT1           | ADC INPUT |
| PC1  | IN2           | OUT2           | ADC INPUT |


| EFR32BG22 | ADC1220 Input | ULN2003 Output | 28BYJ-48        |
|------|---------------|----------------|-----------------|
| PA3  | IN1           | OUT1           | SPI MISO |
| PA4  | IN2           | OUT2           | SPI MOSI |
| PC4  | IN3           | OUT3           | SPI CLK  |
| PC2  | IN4           | OUT4           | SPI CS   |
| PB0  | IN4           | OUT4           | SPI INT  |


VCOM:
| PA5  | IN1           | OUT1           | USART0 TX |
| PA6  | IN1           | OUT1           | USART0 RX |

LED:
| NO  | IN4           | OUT4           | SPI INT  |
Button:
| PC5  | IN4           | OUT4           | SPI INT  |
CLK OUT:
| PC3  | IN4           | OUT4           | SPI INT  |


Import the included .sls file to Simplicity Studio then build and flash the project to the bgm board.
In Simplicity Studio select "File->Import" and navigate to the directory with the .sls project file.
The project is built with relative paths to the STUDIO_SDK_LOC variable which was defined as

C:\SiliconLabs\SimplicityStudio\v4\developer\sdks\gecko_sdk_suite\v3.2

## How the Project Works ##

The application sits in EM1 until an interrupt occurs. The push buttons on the GG11 Starter Kit is used to start the TIMER and choose a rotation direction. TIMER1 is set to overflow at a frequency of 200 Hz and set to interrupt in an overflow event. In the TIMER1 interrupt handler, the software sets the coils to the next state in order to step the motor. In order to rotate the motor counter-clockwise, the motor coils need to be driven in the following order: Coil 1 -> Coil 3 -> Coil 2 -> Coil 4. In order to rotate the motor clockwise, the motor coils need to be driven in the following order: Coil 4 -> Coil 2 -> Coil 3 -> Coil 1. The calculateSteps() function determines the number of full steps required to rotate by a specified angle. The desired delta angle can be set using the ANGLE_PER_TRIGGER macro. TIMER1 will continue to interrupt until the motor shaft rotates by the desired angle. Once the desired angle is reached, TIMER1 stops and the application waits for the next pushbutton press.

## How to test ##
a. run the code
b. dump the adc data via vcom
c. import the data into excel
d. calcuate the ENOB

## .sls Projects Used ##

gg11_stepper_motor.sls

## Steps to create the project ##
a. add EFR32BG22C224F512IM32 in my products and select it.
a. For EFR32BG22C224F512IM32 (EFR32BG22C112F352GM32), start with "Platform - Empty C Example" project.
b. Add software component Services->IO Stream->IO Stream: USART. also configure it.
c. Add Add platform->peripheral->iadc and letimer
d. Add platform->peripheral->i2c and usart
e. add folder inc and drv.
f. drag the files into the folder.
g. add the inc path.
h. replace the app.c
e. ignore PTI warning in pintool.

## How to Port to Another Part ##

Open the "Project Properties" and navigate to the "C/C++ Build -> Board/Part/SDK" item. Select the new board or part to target and "Apply" the changes.  Note: there may be dependencies that need to be resolved when changing the target architecture.
note: only EFR32/EFM32 S2 support this.

## reference ##
DAC70501 datasheet, https://www.ti.com/lit/ds/symlink/dac70501.pdf
ADC1220 datasheet, https://www.ti.com/lit/ds/symlink/ads1220.pdf
REF3312 datasheet, https://www.ti.com/product/REF3312