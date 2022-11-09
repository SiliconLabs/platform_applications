# QI PRx communication protocol #
![Type badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_qi_rx_base_common.json&label=Type&query=type&color=green)
![Technology badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_qi_rx_base_common.json&label=Technology&query=technology&color=green)
![License badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_qi_rx_base_common.json&label=License&query=license&color=green)
![SDK badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_qi_rx_base_common.json&label=SDK&query=sdk&color=green)
![Build badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_qi_rx_base_build_status.json)
![Flash badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_qi_rx_base_common.json&label=Flash&query=flash&color=blue)
![RAM badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_qi_rx_base_common.json&label=RAM&query=ram&color=blue)

## Summary ##

This project implements the QI Power Receiver protocol (PRx) as defined by the [**Wireless Power Consortium**](https://www.wirelesspowerconsortium.com/)
It allows a device based on any SiLabs EFx32 to implement the PRx protocol for both Base and Extended Power Profile Power Receivers (through Load Modulation) and enables building a device that can be wirelessly charged without the need of a separate QI charge controller.

The project complies with v1.3 of the specification which can be downloaded from [**here**](https://www.wirelesspowerconsortium.com/data/downloadables/3/3/2/3/qi-v13-public.zip)

A special second project is available for implementation in e.g. a bootloader, when a very fast reaction time (e.g. after full power loss) is required, to guarantee meeting the timing requirements for maintaining the Power transmit charging power after the first appliance. (The Power transmitter needs feedback from Power Receiver within 65ms after applying power, otherwise, it has to shut down power transmission again, see par Qi Specification Communication Protocol, par 4.2.3, ping timeout)

*Please note this project does not implement FSK decoding to receive PTx messages*

## Gecko SDK version ##

v3.2.3

## Hardware Required ##

To run the example without modification one needs a [Wireless Starter Kit Mainboard](https://www.silabs.com/development-tools/wireless/wireless-starter-kit-mainboard) with a [SLWRB4181B](https://www.silabs.com/development-tools/wireless/slwrb4181b-efr32xg21-wireless-gecko-radio-board) radio board. 
E.g. the wireless starter kit [SLWSTK6023A](https://www.silabs.com/development-tools/wireless/efr32xg21-bluetooth-starter-kit) has everything you need

For full testing, one needs also a QI compliant Power Receiver hardware for which the QI specification zip file contains a pdf with Power Receiver Design Examples (for link see Summary). The output GPIO from this project needs to control the load modulation switch in these examples

## Connections Required ##

GPIO controlling the Load Modulation in the Power Receiver circuitry: in the example the SPI_MOSI output of USART2 connected to PC00, on pin 4 of the EXP header of the SLWSTK4001 board. (See [UG429](https://www.silabs.com/documents/public/user-guides/ug429-brd4181b-user-guide.pdf) for details)

## Setup ##

### Test 1 ###

Import the included [platform-qi_rx_base.sls](SimplicityStudio/platform-qi_rx_base.sls) file to Studio then build and flash the project to the SLWRB4181B plugged into a SLWMB4001A.
In Simplicity Studio v5 select "File->Import" and navigate to the directory with the .sls project file.
The project is built with relative paths to the STUDIO_SDK_LOC variable which was defined as
*C:\SiliconLabs\SimplicityStudio\v5\developer\sdks\gecko_sdk_suite\v3.2*

### Test 2 ###

If you need a very fast response (e.g. after POR), like in a bootloader, please import the included [BRD4181B_Qi_rx_base_POR_GPIO](SimplicityStudio/BRD4181B_Qi_rx_base_POR_GPIO.sls). This project uses **fixed** PRx Qi messages in flash.
The source for these fixed messages can be generated from the main (test 1) project. The const structures needed are generated and output to UART and need to be copy-pasted inside the main_qi_base_class0_after_por_gpio.c source file.

## How It Works ##

For a QI compliant Power Receiver (PRx) to communicate to a QI compliant Power transmitter it needs to be able to send data through "Load Modulation" over the established Power Transfer coupling. The protocol scheme is defined in **Qi-v1.3-comms-physical.pdf**, part of the zip file referenced in the Summary. 
The bit coding is differential bi-phasic (2kHz). To be able to generate this signal a 4kHz bitstream is generated which can be shifted out by SPI peripheral using LDMA to generate the bit coding per specification. The SPI peripheral is misused here and will only require its Tx signal (MOSI) and not the other SPI signals like Rx (MISO), clock, or chip select.

![bit timing](doc/images/qi_msg_timing.jpg)

The byte encoding is 11-bit asynchronous serial: Start bit, Data bits LSB first, parity and stop bit. The Data packet structure is also defined here consisting of a Preamble, Header, Message, and Checksum.

The actual timing and message content a PRx needs to send is defined in **Qi-v1.3-comms-protocol.pdf**, also part of the Zip package referenced in the Summary.
This example implements the messages needed for a BPP-PRx device (Base protocol class 0).

In this example, the SPI peripheral is used to shift out corresponding levels to create the required coding scheme. With SPI USART running from HFRCO, the required accuracy is accomplished by setting the SPI frequency to 4.000 kHz.

Here is the first message (PRx_SIG):

![qi sig message](doc/images/qi_sig_msg.png)

### Physical load modulation encoding ###

The source [qi.c](src/qi.c) contains all function related to bit (qi_add_to_spi_stream()), byte encoding (qi_code_byte()) es well as data packet (qi_create_packet()) encoding.
Multiple messages can be concatenated through repeat calls to qi_create_spi_stream_buffer() function, including an option for pre-delay, to create a required pause between messages by just shifting out constant level to the load modulation GPIO.

### PRx communication protocol ###

The source [app_qidrv_prx.c](test/test_1_dynamic_at_runtime/app_qidrv_prx.c) defines the qi messages needed to comply with BPP-PRx (Base protocol class 0), which require a qi signal strength message, an identification message, a configuration message, followed by control error messages.
This example implements static default messages for this, to get the charging initialized and going. After this, the application can take over the charge control by modifying the control error messages (and potentially sending the PRx_EPT message).

### Sending the data ###

The source [app_qidrv_prx.c](test/test_1_dynamic_at_runtime/app_qidrv_prx.c) takes the generated SPI stream and sets up an LDMA transfer through the SPI/USART peripheral so the complete slow communication will run in hardware, without any further software intervention through a call to SPIDRV_MTransmit. Interrupts are informing the software when the transfer is finished so it can update/change messages if needed. In the meanwhile, the firmware can perform any other (application) function.

Here is the complete sequence after POR:

![qi_class0_prx_init](doc/images/qi_class0_prx_init.jpg)

showing SIG ---- followed by IDENT ---- followed by CONFIG packet according to the specification.

### Performance ###

This example (sources in the [first test project](test/test_1_dynamic_at_runtime/)) will start the first data packet after approx 47ms on referenced board, which is well within the maximum allowed (70ms). Critical here is to NOT initialize the LFXO as this takes too long.

![QI Base timing After POR](doc/images/qi_base_por_time.png)

In case this application takes too long to start, e.g caused by adding security features and/or additional bootloader operations, a solution is provided through the second project.

### Sub-project for a very fast reaction after POR (or on digital input)

The second test project is optimized for very fast sending of fixed qi messages after POR (or after triggering from a digital signal, GPIO).

With this, we achieve the start of the first data packet after 30ms:

![QI Base timing After POR optimized](doc/images/qi_base_por_time_optimized.png)

The source [main.c](test/test_2_fixed_fast_after_por_gpio/main.c) uses the same message from the main project, but the messages are fixed and already encoded as SPI streams and declared "const" so they are located in flash. These arrays can be generated from the main project by setting the 

    #define QI_GENERATE_CODE 1
    
in qidrv_prx.c. The C code will be output through the UART:

    /******************************************************
    * generated buffer for QI transmit over SPI, rev 0.1
    ******************************************************/
    const uint8_t QI_init[] = {
    0xAA, 0xAA, 0xAB, 0x4C, 0xCC, 0xD2, 0xAA, 0xAA,
    0xB2, 0xAA, 0xAD, 0xFF, 0xFF, 0xFF, 0xFA, 0xAA,
    0xAA, 0xB4, 0xCA, 0xB5, 0x2C, 0xD3, 0x2B, 0x55,
    0x55, 0x52, 0xAA, 0xAA, 0xB5, 0x52, 0xAD, 0x2A,
    0xAA, 0xAB, 0x55, 0x55, 0x52, 0xAA, 0xAA, 0xB3,
    0x35, 0x4D, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xAA, 0xAA, 0xAB, 0x4C, 0xB4, 0xD3, 0x4B,
    0x32, 0xB3, 0x33, 0x35, 0x33, 0x33, 0x2B, 0x2C,
    0xCB, 0x53, 0x33, 0x32, 0xB4, 0xD4, 0xCD
    };

    /******************************************************
    * generated buffer for QI transmit over SPI, rev 0.1
    ******************************************************/
    const uint8_t QI_control_error_PREDELAY[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xAA, 0xAA, 0xAB,
    0x53, 0x33, 0x53, 0x33, 0x32, 0xB5, 0x33, 0x35
    };

Back to main_qi_base_class0_after_por_gpio.c:
Now the only thing required is to initialize the GPIO for triggering (InitPRSGpio()), initialize the USART2 for SPI Master operation (InitiUsart2()), and enable LDMA (InitLDMA()).
After the first base transfer (for POR), it is shown how to "arm" the same by GPIO.
The intend is to react fast enough, after Power is applied (device put on qi charger), with QI load modulated messages to the charger to keep the Power Transfer (charging) going. This is done by connecting a signal from the power receive circuitry, indicating charging has started, to this GPIO. 
The QI messages triggered by this GPIO run long enough until the application can take over and modulate/end power transfer when needed.
See this timing capture where button0 is used as GPIO trigger:

![QI Base timing After GPIO](doc/images/qi_base_gpio.png)


## .sls Projects Used ##

+ [platform-qi_rx_base.sls](SimplicityStudio/platform-qi_rx_base.sls)
+ [BRD4181B_Qi_rx_base_POR_GPIO](SimplicityStudio/BRD4181B_Qi_rx_base_POR_GPIO.sls)

## How to Port to Another Part ##

As only specific peripherals are used, the project can be easily ported to any EFx32 device with the required peripherals on board: USART in SPI (Master) mode, PRS, LDMA.

Open the "Project Properties" and navigate to the "C/C++ Build -> Board/Part/SDK" item.  Select the new board or part to target and "Apply" the changes.  Note: there may be dependencies that need to be resolved when changing the target architecture.

## Special Notes ##

The example does not implement all QI exceptions, like a restart of the Power transmitter requiring a restart of PRx sequence. However, this is an easy addition based on a digital input (GPIO) indicating Power present or not and reacting on this signal by an interrupt.
