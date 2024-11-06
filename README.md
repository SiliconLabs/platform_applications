<table border="0">
  <tr>
    <td align="left" valign="middle">
    <h1>EFM32 and EFR32 Platform Application Examples</h1>
  </td>
  <td align="left" valign="middle">
    <a href="https://www.silabs.com/mcu/32-bit">
      <img src="http://pages.silabs.com/rs/634-SLU-379/images/WGX-transparent.png"  title="Silicon Labs Gecko and Wireless Gecko MCUs" alt="EFM32 32-bit Microcontrollers" width="250"/>
    </a>
  </td>
  </tr>
</table>

# Silicon Labs Platform Applications #

[![Version Badge](https://img.shields.io/badge/-v2.3.0-green)](https://github.com/SiliconLabs/bluetooth_applications/releases)
[![GSDK Badge](https://img.shields.io/badge/GSDK-v4.4.4-green)](https://github.com/SiliconLabs/gecko_sdk/releases)
![License badge](https://img.shields.io/badge/License-Zlib-green)

This repo contains example projects that demonstrate various applicaitons using the peripherals of Silicon Labs EFM32 and EFR32 parts.

This repository provides both SLCP projects (as External Repositories) and SLS projects as standalone projects, which are configured for development boards.

## Examples ##

| No | Example name | Link to example |
|:--:|:-------------|:---------------:|
| 1 | Platform - Executing Code from RAM | [Click Here](./platform_pg23_code_execution_ram) |
| 2 | Platform - True Random Number Generator (TRNG) | [Click Here](./platform_trng) |
| 3 | Platform - Peripheral LESENSE ADC | [Click Here](./platform_peripheral_lesense_adc) |
| 4 | Platform - SegmentLCD Low Power | [Click Here](./platform_segmentLCD_lowpower) |
| 5 | Platform - Segment LCD and Temperature Sensor | [Click Here](./platform_segmentLCD_tempsensor) |
| 6 | Platform - IADC High Accuracy Mode - PG23 (BRD2504A) | [Click Here](./platform_iadc_high_accuracy_PG23) |
| 7 | Platform - Lean Watchdog | [Click Here](./platform_lean_watchdog) |
| 8 | Platform - Edge Counting Using the EFM32/EFR32 Series 1 Pulse Counter (PCNT) | [Click Here](./platform_pcnt_edge_counter_series1) |
| 9 | Platform - Si7021 RHT Sensor Bare-metal | [Click Here](./platform_rht_baremetal) |
| 10 | Platform - Using Autonomous Peripherals in Low Power EM2 Mode | [Click Here](./platform_peripheral_low_energy) |
| 11 | Platform - DALI Communication using EUSART (Main device) | [Click Here](./platform_dali) |
| 12 | Platform - DALI Communication using EUSART (Secondary device) | [Click Here](./platform_dali) |
| 13 | Platform - DALI Communication using bitbang SPI (Main device) | [Click Here](./platform_dali) |
| 14 | Platform - DALI Communication using bitbang SPI (Secondary device) | [Click Here](./platform_dali) |
| 15 | Platform - DALI Communication using bitbang SPI with DMADRV (Main device) | [Click Here](./platform_dali) |
| 16 | Platform - DALI Communication using bitbang SPI with DMADRV (Secondary device) | [Click Here](./platform_dali) |
| 17 | Platform - Segment LCD with LDMA | [Click Here](./platform_segmentLCD_ldma) |
| 18 | Platform - IADC High-speed Mode | [Click Here](./platform_iadc_high_speed_PG23) |
| 19 | Platform - Segment LCD with Timer | [Click Here](./platform_segmentLCD_timer) |
| 20 | Platform - IADC High-speed Mode | [Click Here](./platform_iadc_high_speed_PG28) |
| 21 | Platform - IADC High Accuracy Mode - PG28 (BRD2506A) | [Click Here](./platform_iadc_high_accuracy) |
| 22 | Platform - Segment LCD with LC Sensor | [Click Here](./platform_segmentLCD_lcsense) |
| 23 | Platform - IADC with LDMA Ping-Pong | [Click Here](./platform_iadc_ldma_ping_pong) |
| 24 | Platform - UART Circular Buffer with LDMA | [Click Here](./platform_uart_ldma_circular_buffer) |
| 25 | Platform - Sisnake | [Click Here](./platform_sisnake) |
| 26 | Platform - PG28 LCD Animation Blink | [Click Here](./platform_lcd_animation_blink_pg28) |
| 27 | Platform - PG23 LCD Animation Blink | [Click Here](./platform_lcd_animation_blink_pg23) |
| 28 | Platform - FG23 LCD Animation Blink | [Click Here](./platform_lcd_animation_blink_fg23) |
| 29 | Platform - Asynchronous EM01GRPA Clock Output | [Click Here](./platform_async_em01grpaclk_out_series2) |
| 30 | Platform - Flash Read-While-Write | [Click Here](./platform_flash_read_while_write) |
| 31 | Platform - I2C Read to NVM3 Test | [Click Here](./platform_i2c_to_nvm3_test) |
| 32 | Platform - I2C Test for EFM8 IOExpander (IOEXP) | [Click Here](./platform_i2c_test_for_efm8_ioexpander) |
| 33 | Platform - RTCC and Watchdog Bare-metal | [Click Here](./platform_rtcc_lcd_wdog) |
| 34 | Platform - I2C Slave Bootloader | [Click Here](./platform_i2cslave_bootloader) |
| 35 | Platform - WS2812 RGB LED Driver | [Click Here](./platform_rgb_led) |
| 36 | Platform - SensiML IMU Data Capture | [Click Here](./platform_SensiML/platform_SensiML_DataCaptureLab/SensiML_IMU) |
| 37 | Platform - SensiML IMU Data Capture with BLE | [Click Here](./platform_SensiML/platform_SensiML_DataCaptureLab/SensiML_IMU_BLE) |
| 38 | Platform - SensiML Microphone Data Capture | [Click Here](./platform_SensiML/platform_SensiML_DataCaptureLab/SensiML_Microphone) |
| 39 | Platform - SensiML IMU Recognition | [Click Here](./platform_SensiML/platform_SensiML_Recognition/SensiML_IMU) |
| 40 | Platform - SensiML IMU Recognition with BLE | [Click Here](./platform_SensiML/platform_SensiML_Recognition/SensiML_IMU_BLE) |
| 41 | Platform - SensiML Microphone Recogniton | [Click Here](./platform_SensiML/platform_SensiML_Recognition/SensiML_Microphone) |
| 42 | Platform - Squash FreeRTOS Glib | [Click Here](./platform_squash_freertos_glib) |
| 43 | Platform - NVM3 Integrity Test | [Click Here](./platform_nvm3_integrity_test) |
| 44 | Platform - Micrium Multiple ADC LMDA Task | [Click Here](./platform_micrium_multiple_adc_ldma_task) |
| 45 | Platform - Linked DMA (LDMA) Arbitration | [Click Here](./platform_peripheral_ldma_arbitration) |
| 46 | Platform - Parse GBL Metadata in Single Shot | [Click Here](./platform_bootloader_interface/parse_gbl_metadata_singleshot) |
| 47 | Platform - Parse GBL Metadata in BufferSize Steps | [Click Here](./platform_bootloader_interface/parse_gbl_metadata_buffersize_steps) |
| 48 | Platform - QI PRx communication protocol | [Click Here](./platform_qi_rx_base) |
| 49 | Platform - QI PRx communication protocol with fast response | [Click Here](./platform_qi_rx_base) |
| 50 | Platform - EFR32xG21 LDMA SPI Throughput | [Click Here](./platform_spi_flash_bandwidth/ldma) |
| 51 | Platform - EFR32xG21 Polled SPI Throughput | [Click Here](./platform_spi_flash_bandwidth/polled) |
| 52 | Platform - Flash blank checking using the GPCRC | [Click Here](./platform_gpcrc_blank_check) |
| 53 | Platform - Timer cascading |[Click Here](./platform_timer_cascade) |
| 54 | Platform - Sine null points detector |[Click Here](./platform_sine_null_points_detector) |
| 55 | Platform - IADC Tailgating |[Click Here](./platform_iadc_tailgating) |
| 56 | Platform - IADC scan multiple external inputs |[Click Here](./platform_iadc_scan_multiple_external_input) |

## Requirements ##

1. Silicon Labs EFR32 Development Kit
2. Simplicity Studio 5
3. Compatible GSDK version that specified in each project's readme file. You can install it via Simplicity Studio or download it from our GitHub [gecko_sdk](https://github.com/SiliconLabs/gecko_sdk)

## Working with Projects ##

1. To add an external repository, perform the following steps.

    - From Simpilicity Studio 5, go to **Preferences > Simplicity Studio > External Repos**. Here you can add the repo `https://github.com/SiliconLabs/platform_applications.git`. 

    - Cloning and then selecting the branch, tag, or commit to add. The default branch is Master. This repo cloned to `<path_to_the_SimplicityStudio_v5>\developer\repos\`

2. From Launcher, select your device from the "Debug Adapters" on the left before creating a project. Then click the **EXAMPLE PROJECTS & DEMOS** tab -> check **platform_applications** under **Provider** to show a list of Bluetooth example projects compatible with the selected device. Click CREATE on a project to generate a new application from the selected template.

## Legacy Projects - Importing *.sls projects ###

1. Place the *.sls file(s) to be imported in a folder.

2. From Simpilicity Studio 5, select **File > Import**, select the folder containing *.sls file(s). Select a project from the detected projects list and click on Next. Name the project and click Finish.

See [Import and Export](https://docs.silabs.com/simplicity-studio-5-users-guide/5.6.0/ss-5-users-guide-about-the-simplicity-ide/import-and-export) for more information.

## Porting to Another Board ##

To change the target board, navigate to Project -> Properties -> C/C++ Build -> Board/Part/SDK. Start typing in the Boards search box and locate the desired development board, then click Apply to change the project settings. Ensure that the board specifics include paths, found in Project -> Properties -> C/C++ General -> Paths and Symbols, correctly match the target board.

## Documentation ##

Official documentation can be found at our [Developer Documentation](https://docs.silabs.com/#section-mcu-wireless) page.

## Reporting Bugs/Issues and Posting Questions and Comments ##

To report bugs in the Application Examples projects, please create a new "Issue" in the "Issues" section of this repo. Please reference the board, project, and source files associated with the bug, and reference line numbers. If you are proposing a fix, also include information on the proposed fix. Since these examples are provided as-is, there is no guarantee that these examples will be updated to fix these issues.

Questions and comments related to these examples should be made by creating a new "Issue" in the "Issues" section of this repo.

## Disclaimer ##

The Gecko SDK suite supports development with Silicon Labs IoT SoC and module devices. Unless otherwise specified in the specific directory, all examples are considered to be EXPERIMENTAL QUALITY which implies that the code provided in the repos has not been formally tested and is provided as-is.  It is not suitable for production environments.  In addition, this code will not be maintained and there may be no bug maintenance planned for these resources. Silicon Labs may update projects from time to time.
