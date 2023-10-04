# WS2812 RGB LED Driver #

## Summary ##

This driver controls WS2812 RGB LEDs.  These LED's use a fast asynchronous single-wire serial protocol and are daisy-chainable.  The protocol runs around 800kHz which is too fast to efficiently bitbang without a lot of optimzation and overhead.  Generally drivers for this device are written in assembly.  This driver instead uses the USART peripheral in an unconventional way to accomplish this with minimal overhead.  This also allows the peripheral to transmit using the LDMA instead of a blocking mode like other drivers.  This driver can support any number of LEDs, however if powering the LED string using the DevKit, the number of LEDs will be limited by the current requirements of the LED string. 

## Gecko SDK version ##

v4.0.2

## Hardware Required ##

- SLSTK3701A GG11 DevKit
- String of WS2812 LEDs.  These come in many forms but Adafruit's Neopixel line is most common.   

## Connections Required ##

- The LED string needs to be powered.  This can be done using an external 3.3v power source or using the VMCU and GND pins on the EXP Header of the SLSTK3701A board.  If using an external power source, the grounds should be tied together.
- The default serial output of this driver is PE10 which is located on the EXP Header.

## Setup ##

- ws2812.c should be added to the src directory and ws2812.h should be added to the inc directory.  
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> b12beca (fixed emlib linking and added function briefs)
- The user will need to define NUMBER_OF_LEDS and set it to the number of LEDs in the LED string they'd like to control.  if no user definition exists, NUMBER_OF_LEDS defaults to 0.  ws2812_rgb_led.h has a _#include "app.h"_ so the NUMBER_OF_LEDS definiton can be done in app.h for convenience.  
- By default the LDMA uses Channel 1.  By default the USART uses USART0 and has PE10 (LOC0) as the serial output.  Both the LDMA and UART can be redefined by the user in a similar fashion to NUMBER_OF_LEDS. 
- The application should call init_ws2812_driver() which will intiialize the USART as well as the LDMA.
- To change the lights the user should call set_color_buffer(); This function takes a uint8_t array as it's parameter.  The array should be sequences of 8 bit HEX color values corresponding to the desired RGB colors.  The bytes should be arranged in sequences of Green, Red then Blue.  Therefore 3 bytes total for every RGB LED.  For example: if there are 10 LEDs in the string, the array should be 30 bytes long (10 sequences of 3 HEX values per LED).  The LEDs are addressed sequencially starting with the first LED in the daisy-chain.
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD

=======
=======
- The user will need to define NUMBER_OF_LEDS and set it to the number of LEDs in the LED string they'd like to control.  if no user definition exists, NUMBER_OF_LEDS defaults to 0.  ws2812.h has a _#include "app.h"_ so the NUMBER_OF_LEDS definition can be done in app.h for convenience.  
- By default, the LDMA uses Channel 1.  By default, the USART uses USART0 and has PE10 (LOC0) as the serial output.  Both the LDMA and UART can be redefined by the user in a similar fashion to NUMBER_OF_LEDS. 
- The application should call init_ws2812_driver() which will initialize the USART as well as the LDMA.
- To change the lights the user should call set_color_buffer(); This function takes a uint8_t array as its parameter.  The array should be sequences of 8-bit HEX color values corresponding to the desired RGB colors.  The bytes should be arranged in sequences of Green, Red then Blue.  Therefore 3 bytes total for every RGB LED.  For example: if there are 10 LEDs in the string, the array should be 30 bytes long (10 sequences of 3 HEX values per LED).  The LEDs are addressed sequentially starting with the first LED in the daisy chain.
>>>>>>> cf50de1 (Update README.md)
=======
>>>>>>> b12beca (fixed emlib linking and added function briefs)
- 
>>>>>>> c063752 (added rgb_led)
=======

>>>>>>> 6cbcffd (removed extra bullet point in README)
## How It Works ##

The datasheet can be found here: https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf
According to the WS2812 datasheet the protocol runs at 800kHz.  A '1' bit is created by a pulse which starts high, ends low and has a duty cycle of 64% (.8uS HIGH and 0.4uS LOW).  A '0' bit is created by a pulse which starts high, ends low and has a duty cycle of 32% (.4uS HIGH and 0.85uS LOW).  The tolerance on every edge is 150nS.  By using a frequency of 3 x 800 kHz = 2.4MHz, 3 bits at 2.4MHz represent each bit at 800KHz.  Each series of 3 bit begins with a '1' and ends with a '0'.  The middle bit is modified to represent the color bit.  If the middle bit is set to '1' the duty cycle will be 66% (0.833uS HIGH and 0.416uS LOW).  If the middle bit is set to '0' the duty cycle will be 33% (0.416uS HIGH and 0.833uS LOW).  These HIGH and LOW times are within 40nS of the specifications from the datasheet and are well within the 150nS allowed tolerance for the protocol.  This driver uses the USART peripheral with the CLK, CS and RX disabled as only the TX pin is needed.

## .sls Projects Used ##

There is a ws2812_rgb_led_demo.sls file.  This demo project is setup to control a string of 24 RGB LEDs.  There is an LETIMER which is setup to update the RGB LEDs every 0.5 seconds.  The colors are randomly chosen.  To improve the usability and add abstraction there is a 'colors' library which includes a struct the hold the 8-bit RGB values for a single LED as well as a method for proportionally reducing the brightness intensity of an LED.  When the device is waiting for the next update, it goes into EM2 sleep.

## How to Port to Another Part ##

This driver should be easily compatible with other Silabs parts which include the USART and LDMA peripheral.  The only thing to modify should be the serial TX output pin.  

