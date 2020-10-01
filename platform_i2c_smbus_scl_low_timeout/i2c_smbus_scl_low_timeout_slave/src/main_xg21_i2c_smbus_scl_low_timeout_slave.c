/**************************************************************************//**
 * @file main_xg21_i2c_smbus_scl_low_timeout_slave.c
 * @brief This project demonstrates the slave configuration of the
 * EFx32xG21 I2C peripheral with the addition of an implementation of the SCL
 * low timeout required by the SMBus standard.
 *
 * Two EFx32xG21 modules are connected together, one
 * running the master project, the other running the slave project. The master
 * reads the slave's current buffer values, increments each value, and writes
 * the new values back to the slave device. The master then reads back the slave
 * values again and verifies the new values match what was previously written.
 * This program runs in a continuous loop, entering and exiting EM2 to handle
 * I2C transmissions. Slave toggles LED0 on during I2C transaction and off when
 * complete. Slave will set LED1 if an I2C transmission error is encountered.
 * @version 1.0.0
 ******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/

/***************************************************************************//**
 * @section Includes
 ******************************************************************************/
#include <stdio.h>
#include "em_device.h"
#include "em_chip.h"
#include "em_i2c.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_rtcc.h"
#include "em_timer.h"
#include "bsp.h"

/***************************************************************************//**
 * @section Defines
 ******************************************************************************/
#define I2C_ADDRESS                     0xE2
#define I2C_BUFFER_SIZE                 10

#define SCL_PORT	gpioPortA	// PA06 = EXP pin 14
#define SCL_PIN		6
#define SDA_PORT	gpioPortA	// PA05 = EXP pin 12
#define SDA_PIN		5

#define SCL_TIMEOUT_MS	25

/***************************************************************************//**
 * @section Global Variables
 ******************************************************************************/

// Buffers
uint8_t i2c_Buffer[I2C_BUFFER_SIZE];
uint8_t i2c_BufferIndex;

// Transmission flags
volatile bool i2c_gotTargetAddress;
volatile bool i2c_rxInProgress;

volatile uint32_t msTicks; /* counts 1ms timeTicks */
bool resetI2C;

/***************************************************************************//**
 * @section Functions/Subroutines
 ******************************************************************************/
/***************************************************************************//**
 * @brief SysTick_Handler
 * Interrupt Service Routine for system tick counter
 ******************************************************************************/
void SysTick_Handler(void)
{
  msTicks++;       /* increment counter necessary in Delay()*/
}

/***************************************************************************//**
 * @brief Delays number of msTick Systicks (typically 1 ms)
 * @param dlyTicks Number of ticks to delay
 ******************************************************************************/
void Delay(uint32_t dlyTicks)
{
  uint32_t curTicks;

  curTicks = msTicks;
  while ((msTicks - curTicks) < dlyTicks) ;
}

/**************************************************************************//**
 * @brief GPIO initialization
 *****************************************************************************/
void initGPIO(void)
{
  // Configure LED0 and LED1 as output
  GPIO_PinModeSet(BSP_GPIO_LED0_PORT, BSP_GPIO_LED0_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(BSP_GPIO_LED1_PORT, BSP_GPIO_LED1_PIN, gpioModePushPull, 0);
}

/**************************************************************************//**
 * @brief
 *    TIMER initialization
 *****************************************************************************/
void initTimer(void)
{
  CMU_ClockEnable(cmuClock_TIMER0, true);
  // Initialize timer
  TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;

  timerInit.fallAction = timerInputActionReloadStart;
  timerInit.riseAction = timerInputActionStop;
  timerInit.oneShot = true;   // on-shot timer for SCL low timeout
  timerInit.enable = false;   // TIMER0 will be enabled (i.e. started) on SCL falling edge

  TIMER_Init(TIMER0, &timerInit);

  // Route Timer0 CC0 to PB0
  GPIO->TIMERROUTE[0].ROUTEEN  = GPIO_TIMER_ROUTEEN_CC0PEN;
  GPIO->TIMERROUTE[0].CC0ROUTE = (SCL_PORT << _GPIO_TIMER_CC0ROUTE_PORT_SHIFT)
                    | (SCL_PIN << _GPIO_TIMER_CC0ROUTE_PIN_SHIFT);

  // set timer TOP value
  TIMER_TopSet(TIMER0, (CMU_ClockFreqGet(cmuClock_TIMER0) / 1000) * SCL_TIMEOUT_MS);

  // Enable TIMER0 interrupts for Capture/Compare on channel 0
  TIMER_IntEnable(TIMER0, TIMER_IF_OF);
  TIMER_IntClear(TIMER0, TIMER_IF_OF);
  NVIC_EnableIRQ(TIMER0_IRQn);
}

/**************************************************************************//**
 * @brief SCL low timeout initialization
 *****************************************************************************/
void initSCLlowTimeout(void)
{
  initTimer();
}

/**************************************************************************//**
 * @brief  Setup I2C
 *****************************************************************************/
void initI2C(void)
{
  // Using default settings
  I2C_Init_TypeDef i2cInit = I2C_INIT_DEFAULT;

  // Configure to be addressable as slave
  i2cInit.master = false;

  // Using PA5 (SDA) and PA6 (SCL)
  GPIO_PinModeSet(gpioPortA, 5, gpioModeWiredAndPullUpFilter, 1);
  GPIO_PinModeSet(gpioPortA, 6, gpioModeWiredAndPullUpFilter, 1);

  // Enable pins at location 15 as specified in datasheet
  GPIO->I2CROUTE[0].SDAROUTE = (GPIO->I2CROUTE[0].SDAROUTE & ~_GPIO_I2C_SDAROUTE_MASK)
                        | (SDA_PORT << _GPIO_I2C_SDAROUTE_PORT_SHIFT
                        | (SDA_PIN << _GPIO_I2C_SDAROUTE_PIN_SHIFT));
  GPIO->I2CROUTE[0].SCLROUTE = (GPIO->I2CROUTE[0].SCLROUTE & ~_GPIO_I2C_SCLROUTE_MASK)
                        | (SCL_PORT << _GPIO_I2C_SCLROUTE_PORT_SHIFT
                        | (SCL_PIN << _GPIO_I2C_SCLROUTE_PIN_SHIFT));
  GPIO->I2CROUTE[0].ROUTEEN = GPIO_I2C_ROUTEEN_SDAPEN | GPIO_I2C_ROUTEEN_SCLPEN;

  // Initializing the I2C
  I2C_Init(I2C0, &i2cInit);

  // Initializing the buffer index
  i2c_BufferIndex = 0;

  // Setting up to enable slave mode
  I2C_SlaveAddressSet(I2C0, I2C_ADDRESS);
  I2C_SlaveAddressMaskSet(I2C0, 0x7F); // must match exact address
  I2C0->CTRL = I2C_CTRL_SLAVE;

  // Configure interrupts
  I2C_IntClear(I2C0, _I2C_IF_MASK);
  I2C_IntEnable(I2C0, I2C_IEN_ADDR | I2C_IEN_RXDATAV | I2C_IEN_ACK | I2C_IEN_SSTOP | I2C_IEN_BUSERR | I2C_IEN_ARBLOST);
  NVIC_EnableIRQ(I2C0_IRQn);
}

/**************************************************************************//**
 * @brief I2C Interrupt Handler.
 *        The interrupt table is in assembly startup file startup_efm32.s
 *****************************************************************************/
void I2C0_IRQHandler(void)
{
  uint32_t pending;
  uint32_t rxData;

  pending = I2C0->IF;

  /* If some sort of fault, abort transfer. */
  if (pending & (I2C_IF_BUSERR | I2C_IF_ARBLOST))
  {
    i2c_rxInProgress = false;
    GPIO_PinOutSet(BSP_GPIO_LED1_PORT, BSP_GPIO_LED1_PIN);
  } else
  {
    if(pending & I2C_IF_ADDR)
    {
      // Address Match
      // Indicating that reception is started
      rxData = I2C0->RXDATA;
      I2C0->CMD = I2C_CMD_ACK;
      i2c_rxInProgress = true;

      if(rxData & 0x1) // read bit set
      {
        if(i2c_BufferIndex < I2C_BUFFER_SIZE) {
          // transfer data
          I2C0->TXDATA     = i2c_Buffer[i2c_BufferIndex++];
        } else
        {
          // invalid buffer index; transfer data as if slave non-responsive
          I2C0->TXDATA     = 0xFF;
        }
      } else
      {
        i2c_gotTargetAddress = false;
      }

      I2C_IntClear(I2C0, I2C_IF_ADDR | I2C_IF_RXDATAV);

      GPIO_PinOutSet(BSP_GPIO_LED0_PORT, BSP_GPIO_LED0_PIN);
    } else if(pending & I2C_IF_RXDATAV)
    {
      rxData = I2C0->RXDATA;

      if(!i2c_gotTargetAddress)
      {
        /******************************************************/
        /* Read target address from master.                   */
        /******************************************************/
        // verify that target address is valid
        if(rxData < I2C_BUFFER_SIZE)
        {
          // store target address
          i2c_BufferIndex = rxData;
          I2C0->CMD = I2C_CMD_ACK;
          i2c_gotTargetAddress = true;
        } else
        {
          I2C0->CMD = I2C_CMD_NACK;
        }
      } else
      {
        /******************************************************/
        /* Read new data and write to target address          */
        /******************************************************/
        // verify that target address is valid
        if(i2c_BufferIndex < I2C_BUFFER_SIZE)
        {
          // write new data to target address; auto increment target address
          i2c_Buffer[i2c_BufferIndex++] = rxData;
          I2C0->CMD = I2C_CMD_ACK;
        } else
        {
          I2C0->CMD = I2C_CMD_NACK;
        }
      }

      I2C_IntClear(I2C0, I2C_IF_RXDATAV);
    }

    if(pending & I2C_IF_ACK)
    {
      /******************************************************/
      /* Master ACK'ed, so requesting more data.            */
      /******************************************************/
      if(i2c_BufferIndex < I2C_BUFFER_SIZE)
      {
        // transfer data
        I2C0->TXDATA     = i2c_Buffer[i2c_BufferIndex++];
      } else
      {
        // invalid buffer index; transfer data as if slave non-responsive
        I2C0->TXDATA     = 0xFF;
      }

      I2C_IntClear(I2C0, I2C_IF_ACK);
    }

    if(pending & I2C_IF_SSTOP)
    {
      // end of transaction
      i2c_rxInProgress = false;

      I2C_IntClear(I2C0, I2C_IF_SSTOP);
    }
  }
}

/**************************************************************************//**
 * @brief
 *    TIMER 0 handler
 *****************************************************************************/
void TIMER0_IRQHandler(void)
{
//  uint32_t i;

  /* Acknowledge the interrupt */
  uint32_t flags = TIMER_IntGet(TIMER0);
  TIMER_IntClear(TIMER0, flags);

  if(flags & TIMER_IF_OF){
	  /* signal reset and re-initialize I2C0 */
	  resetI2C = true;
  }
}

/***************************************************************************//**
 * @brief   Disable high frequency clocks
 ******************************************************************************/
static void disableHFClocks(void)
{
  // Make sure all high frequency peripherals are disabled
  USART0->EN_CLR = 0x1;
  USART1->EN_CLR = 0x1;
  USART2->EN_CLR = 0x1;
  TIMER0->EN_CLR = 0x1;
  TIMER1->EN_CLR = 0x1;
  TIMER2->EN_CLR = 0x1;
  TIMER3->EN_CLR = 0x1;
  ACMP0->EN_CLR = 0x1;
  ACMP1->EN_CLR = 0x1;
  IADC0->EN_CLR = 0x1;
  I2C1->EN_CLR = 0x1;
  GPCRC->EN_CLR = 0x1;

  CMU_ClockSelectSet(cmuClock_SYSCLK, cmuSelect_FSRCO);

  // Check that HFRCODPLL and HFXO are not requested
  while (((HFRCO0->STATUS & _HFRCO_STATUS_ENS_MASK) != 0U)
         || ((HFXO0->STATUS & _HFXO_STATUS_ENS_MASK) != 0U));
}

/***************************************************************************//**
 * @brief   Disable low frequency clocks
 ******************************************************************************/
static void disableLFClocks(void)
{
  // Make sure all low frequency peripherals are disabled
  RTCC->EN_CLR = 0x1;
  WDOG0->EN_CLR = 0x1;
  WDOG1->EN_CLR = 0x1;
  LETIMER0->EN_CLR = 0x1;
  BURTC->EN_CLR = 0x1;

  // Check that all low frequency oscillators are stopped
  while ((LFRCO->STATUS != 0U) && (LFXO->STATUS != 0U));
}

/***************************************************************************//**
 * @brief   Disable all clocks to achieve lowest current consumption numbers.
 ******************************************************************************/
static void disableClocks(void)
{
  // Disable High Frequency Clocks
  disableHFClocks();

  // Disable Low Frequency Clocks
  disableLFClocks();
}

/***************************************************************************//**
 * @brief
 *   Enter EM2 with RTCC running on a low frequency oscillator.
 *
 * @param[in] osc
 *   Oscillator to run RTCC from (LFXO or LFRCO).
 * @param[in] powerdownRam
 *   Power down all RAM except the first 16 kB block or retain full RAM.
 *
 * @details
 *   Parameter:
 *     EM2. Deep Sleep Mode.@n
 *   Condition:
 *     RTCC, 32.768 kHz LFXO or LFRCO.@n
 *
 * @note
 *   To better understand disabling clocks and oscillators for specific modes,
 *   see Reference Manual section EMU-Energy Management Unit and Table 9.2.
 ******************************************************************************/
void em_EM2_RTCC(CMU_Select_TypeDef osc, bool powerdownRam)
{
  // Make sure clocks are disabled.
  disableClocks();

  // Route desired oscillator to RTCC clock tree.
  CMU_ClockSelectSet(cmuClock_RTCCCLK, osc);

  // Setup RTC parameters
  RTCC_Init_TypeDef rtccInit = RTCC_INIT_DEFAULT;
  rtccInit.presc = rtccCntPresc_1;

  // Initialize RTCC
  RTCC_Reset();
  RTCC_Init(&rtccInit);

  // Power down all RAM blocks except block 1
  if (powerdownRam)
  {
    EMU_RamPowerDown(SRAM_BASE, 0);
  }

  // Enter EM2.
  EMU_EnterEM2(false);
}

/**************************************************************************//**
 * @brief  Main function
 *****************************************************************************/
int main(void)
{
  uint32_t i;
  resetI2C = false;

  // Chip errata
  CHIP_Init();

  /* Setup SysTick Timer for 1 msec interrupts  */
  if (SysTick_Config(CMU_ClockFreqGet(cmuClock_CORE) / 1000)) {
    while (1) ;
  }

  // Setting up i2c
  initI2C();

  initGPIO();

  // Set up SCL low timeout
  initSCLlowTimeout();

  while (1)
  {
	// Receiving I2C data; keep in EM1 during transmission
	if(!i2c_rxInProgress)
	{
		GPIO_PinOutClear(BSP_GPIO_LED0_PORT, BSP_GPIO_LED0_PIN);
	}

	EMU_EnterEM1();

	/* Handle reset if I2C/SMBus, if necessary */
	if(resetI2C){
	  /* Reset and re-initialize I2C0.  Note that the I2C/SMBus should be
	   * reset within 10 ms of sCL low timeout in order to comply with SMBus
	   * standards. It is left as a further exercise for the developer to ensure
	   * this deterministic latency. */
	  I2C_Reset(I2C0);
	  initI2C();
	  resetI2C = false;

	  /* Flash LEDs to indicate the SCL low timeout happened and I2C was reset */
	  for(i=0;i<4;i+=1)
	  {
		  GPIO_PinOutToggle(BSP_GPIO_LED0_PORT, BSP_GPIO_LED0_PIN);
		  GPIO_PinOutToggle(BSP_GPIO_LED1_PORT, BSP_GPIO_LED1_PIN);
		  Delay(500);
	  }
	}
	/* End I2C reset procedure */
  }
}
