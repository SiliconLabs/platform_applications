
![Type badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/parse_gbl_metadata_buffersize_steps_common.json&label=Type&query=type&color=green)
![Technology badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/parse_gbl_metadata_buffersize_steps_common.json&label=Technology&query=technology&color=green)
![License badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/parse_gbl_metadata_buffersize_steps_common.json&label=License&query=license&color=green)
![SDK badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/parse_gbl_metadata_buffersize_steps_common.json&label=SDK&query=sdk&color=green)
![Build badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/parse_gbl_metadata_buffersize_steps_build_status.json)

# Parse GBL Metadata in BufferSize Steps #

## Summary ##

This project demonstrates how to use Application Parser Interface described in UG266. 
After pressing Push Button 1, the GBL image in the storage slot is first verified and the metadata in the stored image is parsed in BufferSize steps using image parser function of the gecko bootloader interface API. Once the metadata is parsed, the raw metadata is transmitted over VCOM(USART0).

Modules used:
HFRCO  - 19 MHz
USART0 - Asynchronous Mode, 1152008N1

## Gecko SDK version ##

v3.0

## Hardware Required ##

* Board:  Silicon Labs EFR32MG13 2.4 GHz 10 dBm Board (BRD4159A) + 
        Wireless Starter Kit Mainboard
	* Device: EFR32MG13P632F512GM48
		* PF5 - LED1
		* PF7 - Push Button 1

## Setup ##

Clone the repository with this project from GitHub onto your local machine.

Place parse_gbl_metadata_buffersize_steps folder in the following location: 
C:\SiliconLabs\SimplicityStudio\v5\developer\sdks\gecko_sdk_suite\v#.#\platform\bootloader\sample-apps.

From within the Simplicity Studio IDE, select Import -> MCU Project... from the Project menu. Click the Browse button and navigate to the parse_gbl_metadata_buffersize_steps folder in the SDK, then to the SimplicityStudio folder, select the .slsproj file for the board, click the Next button twice, and then click Finish.

## How the Project Works ##

When Push Button 1 is pressed, LED1 is toggled, GBL Image in slot0 is verified, decrypted/parsed in BufferSize steps where BufferSize is configurable in project and then raw metadata is transmitted over USART0(VCOM). In this example, the encrypted/signed GBL image that is stored in slot0 is created from a metadata binary image.

**Below are the steps to perform the test:**

 1. Perform erase on the device before starting the test. 
   Below is the Simplicity Commander CLI command:
    >     Run: commander device pageerase --region @mainflash --region @userdata <--region @bootloader> <--region @lockbits>

    Note: Series 2 devices don't have dedicated bootloader and lockbits area. Also EFx32xG1 devices don't have dedicated bootloader area.

**Bootloader Project:**

 2. Import bootloader-storage-internal-single-512k project in the SDK and enable secturity features i.e., signing, encryption and secureboot. 
 
 3. Build the bootloader-storage-internal-single-512k project and flash the bootloader project at the start address of bootloader area.
    >     Run: commander flash <bootloader_project>.s37 --address <start_address>
    
    * For Series 1 devices without dedicated bootloader area(e.g. EFR32xG1) : Flash bootloader-storage-internal-single-512k-combined.s37 (i.e., First Stage Bootloader + Second Stage Bootloader at 0x0000) 
    * For Series 1 devices with dedicated bootloader area(e.g. EFR32xG13): Flash bootloader-storage-internal-single-512k-combined.s37 (i.e., First Stage Bootloader + Second Stage Bootloader at 0xFE10000) 
    * For Series 2 devices(e.g. EFR32xG21):  Flash bootloader-storage-internal-single-512k.s37 (i.e., Second Stage Bootloader at 0x0000) 
 
**Security Keys:**

 4. Generate signing keys. Execute the following command to generate a key-pair for signing.
    >     Run: commander gbl keygen --type ecc-p256 --outfile signing-key

 5. Generate encryption key. Execute the following command to generate an encryption key.
    >     Run: commander gbl keygen --type aes-ccm --outfile encryption-key

 6. Write the encryption key and public key to the EFR32.
    >     Run: commander flash --tokengroup znet --tokenfile encryption-key --tokenfile signing-key-tokens.txt.

**Application Project:**
 
 7. Import and build BRD4159A_EFR32MG13_parse_gbl_metadata_buffersize_steps project to generate .s37/.hex files.
 
 8. Sign the application image to enable secure boot of the application image.
    >     Run: commander convert <application_project>.s37 --secureboot --keyfile signing-key --outfile <application_project_signed>.s37

 9. Flash signed application image at the start address of application area.
    >     Run: commander flash <application_project_signed>.s37 --address <start_address>
    - For Series 1 devices without dedicated bootloader area(e.g. EFR32xG1) : start address of application area varies from device to device (see linker script of bootloader)
    - For Series 1 devices with dedicated bootloader area(e.g. EFR32xG13): Default start of application area - 0x0000
    - For Series 2 devices:  
        * For EFR32xG21: Default start of application area -  0x40000
        * For EFR32xG22: Default start of application area -  0x60000
       
**OTA/GBL image:**

 10. Create a signed and encrypted GBL file using metadata binary file. (waveform_test1 & waveform_test2 files in metadata_test_files folder can be used for testing)
     >     Run: commander gbl create <waveform_metadata>.gbl -- metadata <waveform_test1>.bin --sign signing-key --encrypt encryption-key

 11. Flash *waveform_metadata*.gbl at the start address of Slot0.
     >     Run: commander flash <waveform_metadata>.gbl.bin --address <slot0_start_address>
     * For all Series 1 / Series 2 (xG21,xG22) devices: Start address of slot0 for default bootloader-storage-internal-single-512k project - 0x44000
     
     Note: To flash a GBL image add .bin extension to the GBL file Ii.e., *OTA_image*.gbl.bin. To perform OTA/OTW upgrade .bin extension is not needed.

**Test:**

 12. Connect the radio board to a terminal program (1152008N1) and PressButton1 for GBL image verification and parsing. Status of application can be seen in the terminal application.
 
**Resources:**

 * For more details about gecko bootloader refer UG266.
 * For more details about Simplicity Commander CLI refer UG162.
    
## Porting to Another EFx32 Series 1 or Series 2 Device ##

Apart from any issues of pin availability on a given radio board, this code should run as-is on any Series 1 or Series 2 radio board having gecko bootloader support.

To change the target board, navigate to Project -> Properties -> C/C++ Build -> Board/Part/SDK. Start typing in the Boards
search box and locate the desired radio board, then click Apply to change the project settings, and go from there.