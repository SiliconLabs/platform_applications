#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_vdac.h"

#include "systick.h"

#include "bsp.h"

// EXP header pin 3 (PC9)
#define PULSE_OUT_PORT  gpioPortC
#define PULSE_OUT_PIN   9
#define PULSE_CNT       256

// EXP header pin 16 (PC10)
#define VDAC_APORT_OUT  VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH10

// Counts 1 ms SysTicks
static volatile uint32_t msTicks = 0;

void SysTick_Handler(void)
{
  msTicks++;
}

static void delay(uint32_t dlyTicks)
{
  uint32_t curTicks = msTicks;

  SysTick_SetValue(0);

  SysTick_IntEnable();

  while ((msTicks - curTicks) < dlyTicks);

  SysTick_IntDisable();
}

void vdacInit(void)
{
  CMU_ClockEnable(cmuClock_VDAC0, true);

  // Modify default initialization
  VDAC_Init_TypeDef init = VDAC_INIT_DEFAULT;

  // Alternate output calibration because APORT to be used
  init.mainCalibration = false;

  // Use the 12 MHz captive VDAC oscillator for the converter clock
  init.asyncClockMode = true;

  // Determine the prescaler needed for the VDAC clock to be 1 MHz
  init.prescaler = VDAC_PrescaleCalc(1000000, false, 0);

  VDAC_Init(VDAC0, &init);

  // Disable unused main VDAC output
  VDAC0->OPA[0].OUT &= ~(VDAC_OPA_OUT_MAINOUTEN);

  // Use APORT2Y output channel 10 (PC10)
  VDAC0->OPA[0].OUT |= VDAC_APORT_OUT;

  // Zero settle time for maximum update rate;
  VDAC0->OPA[0].TIMER &= ~(_VDAC_OPA_TIMER_SETTLETIME_MASK);

  // Default channel initialization
  VDAC_InitChannel_TypeDef initChannel = VDAC_INITCHANNEL_DEFAULT;

  VDAC_InitChannel(VDAC0, &initChannel, 0);

  // Enable APORT output
  VDAC0->OPA[0].OUT |= VDAC_OPA_OUT_APORTOUTEN;

  // Enable the VDAC
  VDAC_Enable(VDAC0, 0, true);
}

void gpioInit(void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Use button 0 as input; configure for rising edge interrupt
  GPIO_PinModeSet(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, gpioModeInputPull, 1);

  GPIO_ExtIntConfig(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, BSP_GPIO_PB0_PIN, true, false, true);
  GPIO_IntClear(1 << BSP_GPIO_PB0_PIN);

  // Enable appropriate odd/even interrupt source depending on the pin number
#if (BSP_GPIO_PB0_PIN & 1)
  NVIC_EnableIRQ(GPIO_ODD_IRQn);
#else
  NVIC_EnableIRQ(GPIO_EVEN_IRQn);
#endif  // (BSP_GPIO_PB0_PIN & 1)

  // Use PC9 (EXP pin 3) as level output for target pulse
  GPIO_PinModeSet(PULSE_OUT_PORT, PULSE_OUT_PIN, gpioModePushPull, 0);
}

int main(void)
{
  uint32_t pcount;

  CHIP_Init();

  // Init DCDC regulator with kit specific parameters
  EMU_DCDCInit_TypeDef dcdcInit = EMU_DCDCINIT_DEFAULT;
  EMU_DCDCInit(&dcdcInit);

  vdacInit();

  // Set VDAC output to 50% (0.625V with VREF = 1.25V)
  VDAC_ChannelOutputSet(VDAC0, 0, 0x800);

  // Setup SysTick for 1 msec interval
  SysTick_Start();
  SysTick_SetPeriod(CMU_ClockFreqGet(cmuClock_CORE) / 1000);

  gpioInit();

  while (1)
  {
    EMU_EnterEM1();

    // Disable button 0 interrupts
    GPIO_IntDisable(1 << BSP_GPIO_PB0_PIN);

    for (pcount = 0; pcount < PULSE_CNT; pcount++)
    {
      // Pulse output for 1 ms
      GPIO_PinOutSet(PULSE_OUT_PORT, PULSE_OUT_PIN);
      delay(1);
      GPIO_PinOutClear(PULSE_OUT_PORT, PULSE_OUT_PIN);
      delay(9);
    }

    // Re-enable button 0 interrupts
    GPIO_IntEnable(1 << BSP_GPIO_PB0_PIN);
  }

  // Should never get here
  __BKPT(0);
}

#if (BSP_GPIO_PB0_PIN & 1)
void GPIO_ODD_IRQHandler(void)
#else
void GPIO_EVEN_IRQHandler(void)
#endif  // (BSP_GPIO_PB0_PIN & 1)
{
  // Clear interrupt flag
  GPIO_IntClear(1 << BSP_GPIO_PB0_PIN);
}
