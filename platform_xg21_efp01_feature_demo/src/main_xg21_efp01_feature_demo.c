/***************************************************************************//**
 * @file main_xg21_efp01_feature_demo.c
 * @brief EFP01 Basic Feature Demo
 * @version 1.0.0
 *******************************************************************************
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
#include "sl_efp_instance_config_brd4179b.h"
#include "em_device.h"
#include "em_chip.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_rmu.h"
/* Include generated 'sl_efpdrv_calc.h' in project */
#include "sl_efpdrv_calc.h"

/***************************************************************************//**
 * @section Defines
 ******************************************************************************/
//#define USINGBRD4181
#ifdef USINGBRD4181
/* Defines for WSTK/BRD4181A LEDs and Pushbuttons */
#define BUTTON0PORT		gpioPortA  // jumper EXP7 (PD2) to EXP14 (PA6)
#define BUTTON0PIN		6
#define BUTTON1PORT		gpioPortD
#define BUTTON1PIN		3
#define LED0PORT		gpioPortB
#define LED0PIN			0
#define LED1PORT		gpioPortB
#define LED1PIN			1

#else
/* Defines for WSTK/BRD4179B LEDs and Pushbuttons */
#define BUTTON0PORT		gpioPortB
#define BUTTON0PIN		0
#define BUTTON1PORT		gpioPortB
#define BUTTON1PIN		1
#define LED0PORT		gpioPortD
#define LED0PIN			2
#define LED1PORT		gpioPortD
#define LED1PIN			3
#endif

/***************************************************************************//**
 * @section Global Variables
 ******************************************************************************/
/* Data Structure and handle pointer for EFP instance  */
static sl_efp_handle_data_t efp_handle_data;
static sl_efp_handle_t      efp = &efp_handle_data;

volatile uint32_t msTicks; /* counts 1ms timeTicks */

bool enterEM2;   // variable used to toggle between EM0 and EM2
bool voa_sw_on;  // variable used to toggle VOA_SW state

/***************************************************************************//**
 * @section Function Prototypes
 ******************************************************************************/
void sl_efp_decouple_handoff(sl_efp_handle_t handle, uint8_t bk_iri_con, 
							 uint8_t bk_ton_max, uint8_t bk_ipk);
void Delay(uint32_t dlyTicks);
void em_EM2(bool powerdownRam);
static void disableClocks(void);

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
 * @brief GPIO Interrupt handler for even pins.
 *****************************************************************************/
void GPIO_EVEN_IRQHandler(void)
{
  /* Get and clear all pending GPIO interrupts */
  uint32_t interruptMask = GPIO_IntGet();
  GPIO_IntClear(interruptMask);  

  /* Check if button 0 was pressed */
  if (interruptMask & (1 << BUTTON0PIN))
  {
      /* handle energy mode switching */
	  enterEM2 = !enterEM2;
  }
}

/**************************************************************************//**
 * @brief GPIO Interrupt handler for odd pins.
 *****************************************************************************/
void GPIO_ODD_IRQHandler(void)
{
  /* Get and clear all pending GPIO interrupts */
  uint32_t interruptMask = GPIO_IntGet();
  GPIO_IntClear(interruptMask); // & 0xAA ?

  /* Check if button 1 was pressed */
  if (interruptMask & (1 << BUTTON1PIN))
  {
      /* toggle VOA_SW flag to signal VOA_SW state change in main loop*/
	  voa_sw_on = !voa_sw_on;
  }
}

/**************************************************************************//**
 * @brief  Main function
 *****************************************************************************/
/* 	Compile and run code as-is.  EFP01 DCDC A will power
 * 	EFR32xG21 AVDD, DVDD, RFVDD, and PAVDD from VOA at 1.8 V.
 * 	EFP01 DCDC B will power xG21 DECOUPLE from VOB.
 * 	Using Energy Profiler, you can measure EM0 current and
 * 	EM2 current (press PB0 to toggle between EM0 and EM1).
 * 	Note that LED0 is ON in EM0 and OFF in EM2.
 **/
int main(void)
{
  enterEM2 = false;
  voa_sw_on = false;

  /* Chip errata */
  CHIP_Init();

  /* GPIO trap to prevent bricking the part */
  GPIO_PinModeSet(BUTTON1PORT, BUTTON1PIN, gpioModeInputPullFilter, 1);

  if (GPIO_PinInGet(BUTTON1PORT, BUTTON1PIN) == 0)
  {
    GPIO_PinModeSet(LED0PORT, LED0PIN, gpioModePushPull, 1);
    GPIO_PinModeSet(LED1PORT, LED1PIN, gpioModePushPull, 1);
    __BKPT(0);
  }
  GPIO_PinModeSet(BUTTON1PORT, BUTTON1PIN, gpioModeDisabled, 0);
  /* end GPIO trap */

  /* Setup SysTick Timer for 1 msec interrupts  */
  if (SysTick_Config(CMU_ClockFreqGet(cmuClock_CORE) / 1000)) {
    while (1) ;
  }

  /* enable GPIO even interrupt on push button 0 for energy mode switching */
  GPIO_PinModeSet(BUTTON0PORT, BUTTON0PIN, gpioModeInputPullFilter, 1);
  GPIO_ExtIntConfig(BUTTON0PORT, BUTTON0PIN, BUTTON0PIN, false, true, true);
  NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
  NVIC_EnableIRQ(GPIO_EVEN_IRQn);
  GPIO_PinModeSet(LED0PORT, LED0PIN, gpioModePushPull, 0);

  /* enable GPIO odd interrupt on push button 1 for VOA_SW toggling */
  GPIO_PinModeSet(BUTTON1PORT, BUTTON1PIN, gpioModeInputPullFilter, 1);
  GPIO_ExtIntConfig(BUTTON1PORT, BUTTON1PIN, BUTTON1PIN, false, true, true);
  NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
  NVIC_EnableIRQ(GPIO_ODD_IRQn);
  GPIO_PinModeSet(LED1PORT, LED1PIN, gpioModePushPull, 0);

#ifndef USINGBRD4181
  // Initialize EFP.
  sl_efp_init_data_t init = SL_EFP_INSTANCE_INIT_BRD4179B;

  /* Use config data generated in sl_efpdrv_calc.h */
  uint8_t config_data[SL_EFP_GEN_SIZE][2] = SL_EFP_GEN;
  init.config_data = config_data[0];
  init.config_size = (unsigned int) SL_EFP_GEN_SIZE;
  sl_efp_init(efp, &init);

  /* Implement DECOUPLE handoff to EFP01 DCDC B/VOB */
  /* BK_IRI_CON=0, BK_TON_MAX=7, BK_IPK=10 */
  sl_efp_decouple_handoff(efp, 0, 7, 10);  
#endif

  EMU_EM23Init_TypeDef em23init = EMU_EM23INIT_DEFAULT;
  EMU_EM23Init(&em23init);

  /* Infinite loop */
  while (1) {
#ifndef USINGBRD4181
	  if(voa_sw_on){
		  /* turn VOA_SW ON */
		  sl_efp_set_voa_sw_mode(efp, efp_voa_sw_mode_on);
	  }
	  else{
		  /* turn VOA_SW OFF */
		  sl_efp_set_voa_sw_mode(efp, efp_voa_sw_mode_off);
	  }
#endif
	  if(enterEM2){
		  GPIO_PinOutClear(LED0PORT, LED0PIN);
		  em_EM2(true);
	  }
	  else{
		  GPIO_PinOutSet(LED0PORT, LED0PIN);
	  }
  }
}

/***************************************************************************//**
* @brief
*   Perform DECOUPLE LDO->DCDC Handoff sequence
******************************************************************************/
void sl_efp_decouple_handoff(sl_efp_handle_t handle, uint8_t bk_iri_con, 
							 uint8_t bk_ton_max, uint8_t bk_ipk)
{
  sl_status_t status;
  uint8_t tmp;

  /* Set VOB target to higher level to guarantee it will overdrive the EFR32 
     DECOUPLE LDO output */
  sl_efp_set_vob_em01_voltage(handle, 1130);

  // Set peak current to minimum
  sl_efp_write_register_field(handle, EFP01_BK_IPK, 0,
                              _EFP01_BK_IPK_BK_IPK_MASK,
                              _EFP01_BK_IPK_BK_IPK_SHIFT);

  // Set Ton time to minimum
  sl_efp_write_register_field(handle, EFP01_BK_CTRL1,
                              1,
                              _EFP01_BK_CTRL1_BK_TON_MAX_MASK,
                              _EFP01_BK_CTRL1_BK_TON_MAX_SHIFT);

  // Set current limit to maximum
  sl_efp_write_register_field(handle, EFP01_BK_CTRL2,
                              15,
                              _EFP01_BK_CTRL2_BK_IRI_CON_MASK,
                              _EFP01_BK_CTRL2_BK_IRI_CON_SHIFT);

  // Enable VOB DCDC in buck only mode
  sl_efp_set_vob_mode(handle, efp_vob_mode_buck);

  // Make sure VOB output is ready before turning off internal LDO regulator.
  do {
    status = sl_efp_read_register(handle, EFP01_STATUS_LIVE, &tmp);
  } while (((tmp & _EFP01_STATUS_LIVE_VOB_INREG_LIVE_MASK) == 0)
           || (status != SL_STATUS_OK));

  // Set desired peak current
  sl_efp_write_register_field(handle, EFP01_BK_IPK, bk_ipk,
                              _EFP01_BK_IPK_BK_IPK_MASK,
                              _EFP01_BK_IPK_BK_IPK_SHIFT);

  // Set desired TON MAX
  sl_efp_write_register_field(handle, EFP01_BK_CTRL1,
                              bk_ton_max,
                              _EFP01_BK_CTRL1_BK_TON_MAX_MASK,
                              _EFP01_BK_CTRL1_BK_TON_MAX_SHIFT);

  // Set desired current limit
  sl_efp_write_register_field(handle, EFP01_BK_CTRL2,
                              bk_iri_con,
                              _EFP01_BK_CTRL2_BK_IRI_CON_MASK,
                              _EFP01_BK_CTRL2_BK_IRI_CON_SHIFT);

  // Turn off internal EFR32xG21 LDO regulator.
  sl_efp_emu_ldo_enable(handle, false);

  // Set desired VOB voltage
  sl_efp_set_vob_em01_voltage(handle, 1100);
}

/***************************************************************************//**
* @brief
*   Enter EM2 with no RTCC
*   This function saves additional power in EM2 by shutting down clocks and
*   powering down RAM blocks, then entering EM2.
*   EM2 is entered by emlib call to EMU_EnterEM2().
*
* @param[in] powerdownRam
*   Power down all RAM except the first 16 kB block or retain full RAM.
*
* @details
*   Parameter:
*     EM2. Deep Sleep Mode.@n
*
******************************************************************************/
void em_EM2(bool powerdownRam)
{
  // Make sure clocks are disabled.
  disableClocks();

  // Power down all RAM blocks except block 1
  if (powerdownRam) {
    EMU_RamPowerDown(SRAM_BASE, 0);
  }

  // Enter EM2.
  EMU_EnterEM2(true);
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
         || ((HFXO0->STATUS & _HFXO_STATUS_ENS_MASK) != 0U)) {
         }
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
  while ((LFRCO->STATUS != 0U) && (LFXO->STATUS != 0U)) {
  }
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
