/***************************************************************************//**
 * @file main.c
 * @brief GG11 driving a unipolar 4-phase stepper motor (28BYJ-48) with
 * linear velocity and full stepping mode
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
 * Includes
 ******************************************************************************/
#include <stdint.h>
#include "em_chip.h"
#include "em_cmu.h"
#include "em_device.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_timer.h"

/***************************************************************************//**
 * Button Settings
 ******************************************************************************/
#define BTN0_PORT gpioPortC
#define BTN0_PIN  8
#define BTN1_PORT gpioPortC
#define BTN1_PIN  9

/***************************************************************************//**
 * Motor Settings
 * ===================================================
 * GG11 -> ULN2003 input -> ULN2003 output -> 28BYJ-48
 * ===================================================
 * PC5  -> IN1 -> O1 -> Coil 1 (Orange)
 * PC4  -> IN2 -> O2 -> Coil 3 (Yellow)
 * PA13 -> IN3 -> O3 -> Coil 2 (Pink)
 * PA12 -> IN4 -> O4 -> Coil 4 (Blue)
 ******************************************************************************/
// The number of full steps required to rotate the motor by 360 degrees
#define FULL_ROTATION_STEPS 2048

// The motor frequency in Hz
#define MOTOR_FREQ 200

// The number of coils/phases
#define NUM_COILS 4

// Motor port connections
GPIO_Port_TypeDef coilPorts[NUM_COILS] = {
    gpioPortC,
    gpioPortC,
    gpioPortA,
    gpioPortA
};

// Motor pin connections
uint8_t coilPins[NUM_COILS] = {
    5,
    4,
    13,
    12
};

// One button press will rotate the motor by ANGLE_PER_TRIGGER degrees
#define ANGLE_PER_TRIGGER 90

#define CCW 0
#define CW 1

int direction;
int num_steps;
int current_step;

/***************************************************************************//**
 * @brief Returns the number of steps required to rotate a specified angle
 ******************************************************************************/
int calculateSteps(int angle)
{
  return (angle * FULL_ROTATION_STEPS) / 360;
}

/***************************************************************************//**
 * @brief Magnetize the coil
 ******************************************************************************/
void coilOn(GPIO_Port_TypeDef gpioPort, int pin)
{
  GPIO_PinOutSet(gpioPort, pin);
}

/***************************************************************************//**
 * @brief Demagnetize the coil
 ******************************************************************************/
void coilOff(GPIO_Port_TypeDef gpioPort, int pin)
{
  GPIO_PinOutClear(gpioPort, pin);
}

/***************************************************************************//**
 * @brief Turns on the specified coil, and turns off the remaining coils
 ******************************************************************************/
void coilOutput(int coil)
{
  int i;

  for(i=0; i<coil; i++) {
     coilOff(coilPorts[i], coilPins[i]);
  }

  coilOn(coilPorts[coil], coilPins[coil]);

  for(i=coil+1; i<NUM_COILS; i++) {
      coilOff(coilPorts[i], coilPins[i]);
  }
}

/***************************************************************************//**
 * @brief GPIO Even IRQ for pushbuttons on odd-numbered pins
 ******************************************************************************/
void GPIO_EVEN_IRQHandler(void)
{
  // Clear all even pin interrupt flags
  GPIO_IntClear(0x5555);

  // Set direction and begin TIMER1
  direction = CW;
  TIMER_Enable(TIMER1, true);
}

/***************************************************************************//**
 * @brief GPIO Odd IRQ for pushbuttons on odd-numbered pins
 ******************************************************************************/
void GPIO_ODD_IRQHandler(void)
{
  // Clear all odd pin interrupt flags
  GPIO_IntClear(0xAAAA);

  // Set direction and begin TIMER1
  direction = CCW;
  TIMER_Enable(TIMER1, true);
}

/***************************************************************************//**
 * @brief Init GPIO
 ******************************************************************************/
void initGpio(void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Pushbuttons
  GPIO_PinModeSet(BTN0_PORT, BTN0_PIN, gpioModeInputPullFilter, 1);
  GPIO_PinModeSet(BTN1_PORT, BTN1_PIN, gpioModeInputPullFilter, 1);

  // Motor Pins
  GPIO_PinModeSet(coilPorts[0], coilPins[0], gpioModePushPull, 0);
  GPIO_PinModeSet(coilPorts[1], coilPins[1], gpioModePushPull, 0);
  GPIO_PinModeSet(coilPorts[2], coilPins[2], gpioModePushPull, 0);
  GPIO_PinModeSet(coilPorts[3], coilPins[3], gpioModePushPull, 0);

  // Enable IRQ for even and odd numbered GPIO pins
  NVIC_EnableIRQ(GPIO_EVEN_IRQn);
  NVIC_EnableIRQ(GPIO_ODD_IRQn);

  // Enable falling-edge interrupts for pushbutton pins
  GPIO_ExtIntConfig(BTN0_PORT, BTN0_PIN,BTN0_PIN, 0, 1, true);
  GPIO_ExtIntConfig(BTN1_PORT, BTN1_PIN, BTN1_PIN, 0, 1, true);
}

/***************************************************************************//**
 * @brief Handle Timer overflow event and increment step
 ******************************************************************************/
void TIMER1_IRQHandler(void)
{
  // Acknowledge the interrupt
  uint32_t flags = TIMER_IntGet(TIMER1);
  TIMER_IntClear(TIMER1, flags);

  // Rotate the motor by one full step
  current_step++;

  if(direction == CW) {
    coilOutput(current_step % NUM_COILS);
  }
  else {
      coilOutput((num_steps - current_step) % NUM_COILS);
  }

  // Stop rotating the motor if the desired angle is reached
  if(current_step == num_steps) {
    TIMER_Enable(TIMER1, false);
    current_step = 0;
  }
}

/***************************************************************************//**
 * @brief Init Timer
 ******************************************************************************/
void initTimer(void)
{
  // Enable clock for TIMER1 module
  CMU_ClockEnable(cmuClock_TIMER1, true);

  // Configure TIMER1 Compare/Capture for output compare
  TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;
  timerCCInit.mode = timerCCModeCompare;
  TIMER_InitCC(TIMER1, 0, &timerCCInit);

  // Initialize TIMER1 with the highest prescaler
  TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
  timerInit.enable = false;
  timerInit.prescale = timerPrescale1024;
  TIMER_Init(TIMER1, &timerInit);

  // Set the TIMER1 overflow at MOTOR_FREQ Hz
  uint32_t topValue = CMU_ClockFreqGet(cmuClock_HFPER) /
                      (2*MOTOR_FREQ * (1 << timerPrescale1024))-1;
  TIMER_TopSet(TIMER1, topValue);

  // Enable TIMER1 interrupts
  TIMER_IntEnable(TIMER1, TIMER_IEN_OF);
  NVIC_EnableIRQ(TIMER1_IRQn);
}

/***************************************************************************//**
 * @brief Main function
 ******************************************************************************/
int main(void)
{
  CHIP_Init();

  num_steps = calculateSteps(ANGLE_PER_TRIGGER);
  current_step = 0;

  initTimer();
  initGpio();

  while(1) {
    EMU_EnterEM1();
  }
}
