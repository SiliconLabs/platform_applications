#include "systick.h"

void SysTick_Start(void)
{
  //
  // Enable SysTick.
  //
  SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
}

void SysTick_Stop(void)
{
  //
  // Disable SysTick.
  //
  SysTick->CTRL &= ~(SysTick_CTRL_ENABLE_Msk);
}

void SysTick_IntEnable(void)
{
  //
  // Enable the SysTick interrupt.
  //
  SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
}

void SysTick_IntDisable(void)
{
  //
  // Disable the SysTick interrupt.
  //
  SysTick->CTRL &= ~(SysTick_CTRL_TICKINT_Msk);
}

void SysTick_SetPeriod(uint32_t period)
{
  //
  // Check the arguments.
  //
  EFM_ASSERT((period > 0) && (period <= 16777216));

  //
  // Set the period of the SysTick counter.
  //
  SysTick->LOAD = period - 1;
}

uint32_t SysTick_GetPeriod(void)
{
  //
  // Return the period of the SysTick counter.
  //
  return (SysTick->LOAD + 1);
}

uint32_t SysTick_GetValue(void)
{
  //
  // Return the current value of the SysTick counter.
  //
  return (SysTick->VAL);
}
