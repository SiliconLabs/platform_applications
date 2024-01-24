/***************************************************************************//**
 * @file
 * @brief Top level application functions
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
#include "em_gpio.h"
#include "em_cmu.h"
#include "em_core.h"

#define IRQ_TABLE_SIZE      (EXT_IRQ_COUNT + 16)

#define BSP_GPIO_PB0_PORT   gpioPortA
#define BSP_GPIO_PB0_PIN    5

#define BSP_GPIO_PB1_PORT   gpioPortB
#define BSP_GPIO_PB1_PIN    4

#define BSP_GPIO_LED0_PORT  gpioPortC
#define BSP_GPIO_LED0_PIN   8

#define BSP_GPIO_LED1_PORT  gpioPortC
#define BSP_GPIO_LED1_PIN   9

typedef void (*vectors_irq_func_ptr)(void);

extern vectors_irq_func_ptr gecko_vector_table[IRQ_TABLE_SIZE];

extern void Default_Handler(void);
void RAM_Default_Handler(void);
void ram_interrupt_vector_update(void);
void gpio_setup(void);

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  ram_interrupt_vector_update();
  gpio_setup();
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
}

/**************************************************************************//**
 * @brief
 *   Setup GPIO for pushbuttons and LEDs
 *****************************************************************************/
void gpio_setup(void)
{
  // Configure GPIO Clock.
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Configure Button PB0 and PB1 as input and enable interrupt
  GPIO_PinModeSet(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, gpioModeInputPull, 1);
  GPIO_ExtIntConfig(BSP_GPIO_PB0_PORT,
                    BSP_GPIO_PB0_PIN,
                    BSP_GPIO_PB0_PIN,
                    false,
                    true,
                    true);
  GPIO_PinModeSet(BSP_GPIO_PB1_PORT, BSP_GPIO_PB1_PIN, gpioModeInputPull, 1);
  GPIO_ExtIntConfig(BSP_GPIO_PB1_PORT,
                    BSP_GPIO_PB1_PIN,
                    BSP_GPIO_PB1_PIN,
                    false,
                    true,
                    true);

  // Enable EVEN and ODD interrupt to catch button press that changes slew rate
  NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
  NVIC_EnableIRQ(GPIO_ODD_IRQn);
  NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
  NVIC_EnableIRQ(GPIO_EVEN_IRQn);

  // Configure LED0 and LED1 as a push pull output
  GPIO_PinModeSet(BSP_GPIO_LED0_PORT, BSP_GPIO_LED0_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(BSP_GPIO_LED1_PORT, BSP_GPIO_LED1_PIN, gpioModePushPull, 0);
}

/**************************************************************************//**
 * @brief
 *   GPIO Interrupt handler for odd pins.
 *****************************************************************************/
void GPIO_ODD_IRQHandler(void)
{
  // Get and clear all pending GPIO interrupts
  uint32_t interruptMask = GPIO_IntGet();
  GPIO_IntClear(interruptMask);

  // Check if button 0 was pressed
  if (interruptMask & (1 << BSP_GPIO_PB0_PIN)) {
    GPIO_PinOutToggle(BSP_GPIO_LED0_PORT, BSP_GPIO_LED0_PIN);
  }
}

/***************************************************************************//**
 * @brief
 *   Place Default Handler for Exceptions / Interrupts in RAM
 ******************************************************************************/
void RAM_Default_Handler(void)
{
  GPIO_PinOutSet(BSP_GPIO_LED1_PORT, BSP_GPIO_LED1_PIN);
  while (true) {
  }
}

/***************************************************************************//**
 * @brief
 *   Configures the vector table in RAM to point to the new Default handler
 *   address that is located in RAM.
 *   The new Default handler can be used to catch exceptions
 *   and external interrupts which are not handled by specific ISRs.
 ******************************************************************************/
void ram_interrupt_vector_update(void)
{
  for (uint32_t i = 0; i < IRQ_TABLE_SIZE; i++) {
    // Overwrite target entries.
    if (gecko_vector_table[i] == Default_Handler) {
      gecko_vector_table[i] = RAM_Default_Handler;
    }
  }
}
