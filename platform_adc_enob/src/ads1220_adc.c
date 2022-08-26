/***************************************************************************//**
 * @file ads1220_adc.c
 * @brief ads1220 spi driver.
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 *******************************************************************************
 * # Evaluation Quality
 * This code has been minimally tested to ensure that it builds and is suitable
 * as a demonstration for evaluation purposes only. This code will be maintained
 * at the sole discretion of Silicon Labs.
 ******************************************************************************/

#include "em_cmu.h"
#include "em_gpio.h"
#include "em_usart.h"
#include "ads1220_adc.h"
#include "dac70501_dac.h"
#include "efr32bg22_adc.h"

// spi register read protocol description:
//  |MSB  ...  LSB|  |MSB  ...  LSB|  |MSB ... LSB|......  |MSB ... LSB|
//  RREG|rr|nn           reg[r]         reg[r+1]         reg[r+n]

// spi register write protocol description:
//  |MSB  ...  LSB|  |MSB  ...  LSB|  |MSB ... LSB|......  |MSB ... LSB|
//  WREG|rr|nn           reg[r]        reg[r+1]         reg[r+n]

// spi ADC data read protocol description:
//    |MSB  ...  LSB|  |MSB  ...  LSB|  |MSB ... LSB|  |MSB ... LSB|
//    START command      DAC[24:16]       DAC[15:8]       DAC[7:0]

// usage description:
// reference:
//    pos, pin7, REFP0
//    neg, pin6, REFN0
// diff input:
//    pos, pin5, AIN2
//    neg, pin4, AIN3/REFN1
// backup input:
//    pos, pin5, AIN0/REFP1
//    neg, pin4, AIN1
// spi:
//    manual operate CS, don't use auto mode
//    ADS1220 ready polling (not interrupt)

// Size of the data buffers
#define ADC_BUFLEN  10
// Outgoing data
uint8_t adc_out_buf[ADC_BUFLEN];
// Incoming data
uint8_t adc_in_buf[ADC_BUFLEN];

// Ports and pins for SPI/USART0 interface
#define US0_MISO_PORT                 gpioPortA
#define US0_MISO_PIN                  3
#define US0_MOSI_PORT                 gpioPortA
#define US0_MOSI_PIN                  4
#define US0_CLK_PORT                  gpioPortC
#define US0_CLK_PIN                   4
#define US0_CS_PORT                   gpioPortC
#define US0_CS_PIN                    2
// Ready pin, not used
#define ADC_READY_PORT                gpioPortB
#define ADC_READY_PIN                 0

// configuration register define (WREG/RREG)
#define ADS1220_REG_CONFIGURATION0    0x00
#define ADS1220_REG_CONFIGURATION1    0x01
#define ADS1220_REG_CONFIGURATION2    0x02
#define ADS1220_REG_CONFIGURATION3    0x03

// COMMAND registers define
#define ADS1220_REG_COMMAND_RESET     0x06      // 0000 011x
#define ADS1220_REG_COMMAND_START     0x08      // 0000 100x
#define ADS1220_REG_COMMAND_POWERDOWN 0x02      // 0000 001x
#define ADS1220_REG_COMMAND_RDATA     0x10      // 0001 xxxx
#define ADS1220_REG_COMMAND_RREG      0x20      // 0010 rrnn
#define ADS1220_REG_COMMAND_WREG      0x40      // 0100 rrnn

// number of registers to write and read
uint8_t read_num_byte, write_num_byte;
uint8_t status = 0;

/**************************************************************************//**
 * @brief
 *    USART0 initialization
 * @param[in]
 *    none
 * @return
 *    none
 *****************************************************************************/
void ads1220_init_usart0(void)
{
  // Default asynchronous initializer (master mode, 1000k bps, 8-bit data)
  USART_InitSync_TypeDef init = USART_INITSYNC_DEFAULT;

  // Enable gpio clock
  CMU_ClockEnable(cmuClock_GPIO, true);
  // Enable usart0 clock
  CMU_ClockEnable(cmuClock_USART0, true);

  // MSB first transmission for SPI compatibility
  init.msbf = true;
  // ADS1220 only support mode 1
  init.clockMode = usartClockMode1;
  init.baudrate = 1000000;

  // Configure RX pin as an input, filter off
  GPIO_PinModeSet(US0_MISO_PORT, US0_MISO_PIN, gpioModeInput, 0);
  // Configure TX pin as an output
  GPIO_PinModeSet(US0_MOSI_PORT, US0_MOSI_PIN, gpioModePushPull, 0);
  // Configure CLK pin as an output low (CPOL = 0)
  GPIO_PinModeSet(US0_CLK_PORT, US0_CLK_PIN, gpioModePushPull, 0);
  // Configure CS pin as an output and drive inactive high
  GPIO_PinModeSet(US0_CS_PORT, US0_CS_PIN, gpioModePushPull, 1);
  // Configure READY pin as an input, filter off
  GPIO_PinModeSet(ADC_READY_PORT, ADC_READY_PIN, gpioModeInput, 0);

  // Route USART0 RX, TX, and CLK to the specified pins. Note that CS is
  // not controlled by USART0 so there is no write to the corresponding
  // USARTROUTE register to do this.

  GPIO->USARTROUTE[0].RXROUTE =                       \
    (US0_MISO_PORT << _GPIO_USART_RXROUTE_PORT_SHIFT) \
    | (US0_MISO_PIN << _GPIO_USART_RXROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[0].TXROUTE =                       \
    (US0_MOSI_PORT << _GPIO_USART_TXROUTE_PORT_SHIFT) \
    | (US0_MOSI_PIN << _GPIO_USART_TXROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[0].CLKROUTE =                      \
    (US0_CLK_PORT << _GPIO_USART_CLKROUTE_PORT_SHIFT) \
    | (US0_CLK_PIN << _GPIO_USART_CLKROUTE_PIN_SHIFT);
  // Enable USART interface pins
  GPIO->USARTROUTE[0].ROUTEEN = GPIO_USART_ROUTEEN_RXPEN        // MISO
                                | GPIO_USART_ROUTEEN_TXPEN      // MOSI
                                | GPIO_USART_ROUTEEN_CLKPEN;    // CLK

  // Configure and enable USART0
  USART_InitSync(USART0, &init);
}

/**************************************************************************//**
 * @brief
 *    test adc uart functionality
 * @param[in]
 *    readnumBytes:
 *    writenumBytes:
 *    readnumBytes >= writenumBytes
 * @return
 *    none
 *****************************************************************************/
void ads1220_uart0_test(uint8_t writenumBytes, uint8_t readnumBytes)
{
  uint8_t i;
  uint8_t adc_inbuf_save[4];

  // read and save for restore later
  GPIO_PinOutClear(US0_CS_PORT, US0_CS_PIN);
  letimer_delay(1);
  // start from reg0
  status = USART_SpiTransfer(USART0, ADS1220_REG_COMMAND_RREG
                             | ((readnumBytes - 1) & 0x3));
  for (i = 0; i < readnumBytes; i++) {
    adc_inbuf_save[i] = USART_SpiTransfer(USART0, 0xff);
  }
  // De-assert chip select upon transfer completion
  letimer_delay(1);
  GPIO_PinOutSet(US0_CS_PORT, US0_CS_PIN);

  // write registers temporary value 0xaa
  GPIO_PinOutClear(US0_CS_PORT, US0_CS_PIN);
  letimer_delay(1);
  // start from reg0
  status = USART_SpiTransfer(USART0, ADS1220_REG_COMMAND_WREG
                             | ((writenumBytes - 1) & 0x3));
  for (i = 0; i < writenumBytes; i++) {
    // dummy read, written 0xaa
    adc_in_buf[i] = USART_SpiTransfer(USART0, 0xaa);
  }
  letimer_delay(1);
  GPIO_PinOutSet(US0_CS_PORT, US0_CS_PIN);

  // read back temporary register value
  GPIO_PinOutClear(US0_CS_PORT, US0_CS_PIN);
  letimer_delay(1);
  // start from reg0
  status = USART_SpiTransfer(USART0, ADS1220_REG_COMMAND_RREG
                             | ((readnumBytes - 1) & 0x3));
  for (i = 0; i < readnumBytes; i++) {
    // expect 0xaa, written value previously
    adc_in_buf[i] = USART_SpiTransfer(USART0, 0xff);
  }
  letimer_delay(1);
  GPIO_PinOutSet(US0_CS_PORT, US0_CS_PIN);

  // check write and read back
  for (i = 0; i < writenumBytes; i++) {
    if (adc_in_buf[i] != 0xaa) {
      while (1) {}
    }
  }

  // restore the register value
  GPIO_PinOutClear(US0_CS_PORT, US0_CS_PIN);
  letimer_delay(1);
  // start from reg0
  status = USART_SpiTransfer(USART0, ADS1220_REG_COMMAND_WREG
                             | ((writenumBytes - 1) & 0x3));
  for (i = 0; i < writenumBytes; i++) {
    // dummy read, write back
    adc_in_buf[i] = USART_SpiTransfer(USART0, adc_inbuf_save[i]);
  }
  letimer_delay(1);
  GPIO_PinOutSet(US0_CS_PORT, US0_CS_PIN);
}

/**************************************************************************//**
 * @brief
 *    adc reset
 * @param[in]
 *    none
 * @return
 *    none
 *****************************************************************************/
void ads1220_reset(void)
{
  // reset ads1220
  GPIO_PinOutClear(US0_CS_PORT, US0_CS_PIN);
  letimer_delay(1);
  // RESET command
  USART_SpiTransfer(USART0, ADS1220_REG_COMMAND_RESET);
  letimer_delay(1);
  GPIO_PinOutSet(US0_CS_PORT, US0_CS_PIN);
  letimer_delay(5);
}

/**************************************************************************//**
 * @brief
 *    start or restart conversions
 * @param[in]
 *    none
 * @return
 *    none
 *****************************************************************************/
void ads1220_sync(void)
{
  // single-shot -             start single
  // continuous conversion -   start sequence

  GPIO_PinOutClear(US0_CS_PORT, US0_CS_PIN);
  letimer_delay(1);
  // START command
  USART_SpiTransfer(USART0, ADS1220_REG_COMMAND_START);
  letimer_delay(1);
  GPIO_PinOutSet(US0_CS_PORT, US0_CS_PIN);
}

/**************************************************************************//**
 * @brief
 *    adc enter power down
 * @param[in]
 *    none
 * @return
 *    none
 *****************************************************************************/
void ads1220_power_down(void)
{
  // a. power down internal analog components
  // b. open low-side switch
  // c. turn of both IDAC
  // d. start will resume previous

  GPIO_PinOutClear(US0_CS_PORT, US0_CS_PIN);
  // reg1
  status = USART_SpiTransfer(USART0, ADS1220_REG_COMMAND_WREG
                             | ((ADS1220_REG_CONFIGURATION1 << 2) & 0x6) // reg1
                             | (0 & 0x3));                               // 1

  // ads1220 operation mode
  adc_in_buf[0] = USART_SpiTransfer(USART0, 0x0);
  GPIO_PinOutSet(US0_CS_PORT, US0_CS_PIN);

  GPIO_PinOutClear(US0_CS_PORT, US0_CS_PIN);
  letimer_delay(1);
  // POWERDOWN command
  USART_SpiTransfer(USART0, ADS1220_REG_COMMAND_POWERDOWN);
  letimer_delay(1);
  GPIO_PinOutSet(US0_CS_PORT, US0_CS_PIN);
}

/**************************************************************************//**
 * @brief
 *    set convert mode
 * @param[in]
 *    modeConv, valid value is 0/1
 * @return
 *    none
 * @comment
 *    single-shot mode default
 *****************************************************************************/
void ads1220_conver_mode_metting(uint8_t modeConv)
{
  uint8_t adc_conv_mode = modeConv << 2;

  GPIO_PinOutClear(US0_CS_PORT, US0_CS_PIN);
  // reg1
  status = USART_SpiTransfer(USART0, ADS1220_REG_COMMAND_WREG
                             | ((ADS1220_REG_CONFIGURATION1 << 2) & 0x6) // reg1
                             | (0 & 0x3));                               // 1
  // ads1220 convert mode
  adc_in_buf[0] = USART_SpiTransfer(USART0, adc_conv_mode);
  GPIO_PinOutSet(US0_CS_PORT, US0_CS_PIN);

  // followed by start
  ads1220_sync();
}

/**************************************************************************//**
 * @brief
 *    set operation mode
 * @param[in]
 *    modeOpe, normal/duty-cycle/turbo
 * @return
 *    none
 * @comment
 *    normal mode default
 *****************************************************************************/
void ads1220_oper_mode_setting(uint8_t modeOpe)
{
  uint8_t adc_ope_mode = modeOpe << 4;

  if (modeOpe > 2) {
    return;
  }

  GPIO_PinOutClear(US0_CS_PORT, US0_CS_PIN);
  // reg1
  status = USART_SpiTransfer(USART0, ADS1220_REG_COMMAND_WREG
                             | ((ADS1220_REG_CONFIGURATION1 << 2) & 0x6) // reg1
                             | (0 & 0x3));                               // 1

  // ads1220 operation mode
  adc_in_buf[0] = USART_SpiTransfer(USART0, adc_ope_mode);
  GPIO_PinOutSet(US0_CS_PORT, US0_CS_PIN);

  // use default data rate
}

/**************************************************************************//**
 * @brief
 *    ads1220 configuration
 * @param[in]
 *    none
 * @return
 *    none
 * @Comment
 *    input: dac70501
 *    ref:   ref3312
******************************************************************************/
void ads1220_reg_config(void)
{
  uint8_t reg0, reg1, reg2, reg3;
  // ADS1220_REG_CONFIGURATION0
  // 7-4          3:1         0
  // mux          gain        pga_bypass
  // b0101        000         0 (enabled pga)

  // default input channel is REF3312 (0x6)
  // AIN0, AIN1, dac70501

  reg0 = 0x50;  // can use pga power down

  // ADS1220_REG_CONFIGURATION1
  // 7-5         4:3          2              1          0
  // DR          MODE         CM             TS         BCS
  // 000         0 (256k)     0 (single      0 (temp    burn-out
  // 20 SPS                      shot       disable     off

  // use default
  reg1 = 0xd0;  // can use duty-cycle mode

  // ADS1220_REG_CONFIGURATION2
  // 7-6           5:4         3               2:0
  // VREF          FIR         PSW             IDAC
  // 010           NO reject   OPEN            0 (OFF)
  // ext

  // external REFP0 and REFN0
  reg2 = 0x40;

  // ADS1220_REG_CONFIGURATION3
  // 7-5          4:2            1              0
  // I1MUX        I2MUX         DRDYM          res
  // 000          000            0               x
  // disable      disable    dedicated only

  // use default
  reg3 = 0x00;

  // write registers values
  GPIO_PinOutClear(US0_CS_PORT, US0_CS_PIN);
  letimer_delay(1);
  // register write, write 3 registers start from reg0
  // command register WREG

  USART_SpiTransfer(USART0, ADS1220_REG_COMMAND_WREG
                    // rr=0        nn=3
                    | (0x0 << 2) | ((3 - 1) & 0x3));
  adc_in_buf[0] = USART_SpiTransfer(USART0, reg0);  // reg0, dac70501 input
  adc_in_buf[1] = USART_SpiTransfer(USART0, reg1);  // reg1
  adc_in_buf[2] = USART_SpiTransfer(USART0, reg2);  // reg2, ref3312 reference
  adc_in_buf[3] = USART_SpiTransfer(USART0, reg3);  // reg2, ref3312 reference
  letimer_delay(1);
  GPIO_PinOutSet(US0_CS_PORT, US0_CS_PIN);
}

/**************************************************************************//**
 * @brief
 *    read ADC raw data
 * @param[in]
 *    none
 * @return
 *    ADC raw value
 *****************************************************************************/
uint32_t ads1220_get_adc_data_raw(void)
{
  uint32_t adc_result = 0;

  // start command
  // delay
  ads1220_sync();
  letimer_delay(100);

  // read data
  GPIO_PinOutClear(US0_CS_PORT, US0_CS_PIN);
  letimer_delay(1);
  USART_SpiTransfer(USART0, ADS1220_REG_COMMAND_RDATA);
  adc_in_buf[0] = USART_SpiTransfer(USART0, 0xff);
  adc_in_buf[1] = USART_SpiTransfer(USART0, 0xff);
  adc_in_buf[2] = USART_SpiTransfer(USART0, 0xff);
  letimer_delay(1);
  GPIO_PinOutSet(US0_CS_PORT, US0_CS_PIN);

  // convert to final data
  adc_result = adc_in_buf[0] << 16 | adc_in_buf[1] << 8 | adc_in_buf[2];

  return adc_result;
}

/**************************************************************************//**
 * @brief
 *    read ADC voltage
 * @param[in]
 *    none
 * @return
 *    ADC voltage value in mV unit
 *****************************************************************************/
double ads1220_get_adc_data_volt(void)
{
  uint32_t adc_result = 0;
  double adc_volt = 0.0;

  adc_result = ads1220_get_adc_data_raw();

  if (adc_result > 0x7fffff) {
    adc_result = -(0xffffff - adc_result + 1);
  }

  adc_volt = adc_result * 1.25 * 2 / 0xFFFFFF;
  return adc_volt;
}

/**************************************************************************//**
 * @brief
 *    read die temperature
 * @param[in]
 *    none
 * @return
 *    temperature value in Celcius unit
 *****************************************************************************/
double ads1220_get_adc_temp(void)
{
  uint32_t adc_volt = 0;
  double adc_temperature;
  char temp_mode = 0x02;
  // set temperature mode, enable temperature measurement,
  // this will override register 0 setting
  // this channel use internal reference 2.048v
  // result need shift right by 10 bit

  GPIO_PinOutClear(US0_CS_PORT, US0_CS_PIN);
  letimer_delay(1);
  // ADS1220_REG_CONFIGURATION1
  // 7-5          4:3         2              1          0
  // DR           MODE        CM             TS         BCS
  // 000          0 (256k)    0 (single      0 (temp    burn-out
  // 20 SPS                      shot        disable    off
  // reg1 = 0x02

  USART_SpiTransfer(USART0, ADS1220_REG_COMMAND_WREG
                    // rr=1                                       nn=1
                    | ((ADS1220_REG_CONFIGURATION1 << 2) & 0x6) | (0 & 0x3));

  // write the value containing the new value to the ADS register
  USART_SpiTransfer(USART0, temp_mode);
  letimer_delay(1);
  GPIO_PinOutSet(US0_CS_PORT, US0_CS_PIN);
  letimer_delay(10);
  adc_volt = ads1220_get_adc_data_raw();
  // 2048 / 2^16 = 0.03125 degC/lsb
  adc_temperature = 0.03125 * (adc_volt >> 10);

  // write back the mode (non temperature)
  GPIO_PinOutClear(US0_CS_PORT, US0_CS_PIN);
  letimer_delay(1);
  USART_SpiTransfer(USART0, ADS1220_REG_COMMAND_WREG
                    //  rr=1          nn=1
                    | ((0x1 << 2) & 0x6) | (0 & 0x3));

  // write the register back to the ADS
  USART_SpiTransfer(USART0, 0x00);
  letimer_delay(1);
  GPIO_PinOutSet(US0_CS_PORT, US0_CS_PIN);

  return adc_temperature;
}

/**************************************************************************//**
 * @brief
 *    calibrate adc offset
 *    also calculate gain
 * @param[in]
 *    none
 * @return
 *    none
 *****************************************************************************/
void ads1220_calibrate(void)
{
  uint32_t i, raw_Value;
  double average = 0.0;
  double correction_gain, correction_offset;
  float dac_Volt;

  // read back dac voltage set
  dac_Volt = dac70501_read_ref();
  // calibrate offset only since the gain resolution is coarse

  dac70501_set_volt(1.25f);
  // adc read operation
  for (i = 0; i < 10; i++) {
    average += ads1220_get_adc_data_raw();
  }
  // get average
  average /= 10;

  // need software calibration
  correction_gain = average / (double)8388607.0;

  // set dac7051 zero output
  dac70501_set_volt(0.0f);
  average = 0.0;
  // adc read operation
  for (i = 0; i < 10; i++) {
    raw_Value = ads1220_get_adc_data_raw();
    if (raw_Value <= 0x7fffff) {
      average += raw_Value;
    } else {
      average += -(0xffffff - raw_Value + 1);
    }
  }
  correction_offset = average / 10;

  average = 0.0;
  for (i = 0; i < 10; i++) {
    average += (ads1220_get_adc_data_raw() - correction_offset)
               / correction_gain;
  }
  // get average
  average /= 10;

  // restore dac voltage
  dac70501_set_volt(dac_Volt);
}

/**************************************************************************//**
 * @brief
 *    ads1220 initialization
 * @param[in]
 *    voltage read
 * @return
 *    none
 *    Vout = data/16384 * vref/div * gain
******************************************************************************/
uint32_t ads1220_init(void)
{
  // delay 1mS to allow power supply to settle and power up reset to complete
  // 60uS minimum

  letimer_delay(1);

  // configure SPI interface
  ads1220_init_usart0();

  // reset ads1220
  ads1220_reset();
  letimer_delay(10);

  // write first then read test
  read_num_byte = 3;
  write_num_byte = 2;
  ads1220_uart0_test(write_num_byte, read_num_byte);

  // config ads1220
  ads1220_reg_config();
  ads1220_sync();
  letimer_delay(100);

  // ret
  return 1;
}
