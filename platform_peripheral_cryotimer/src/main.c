#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_emu.h"

/*
 * Toggles PA11 (LCD EXTCOMIN) at approximately 1s interval.
 * Drops to EM3 after setup.  Consumes 10 uA, with 100 uA pulses
 */

void setup_cryotimer(void (*callback)(void));

void cryo_callback(void) {
  GPIO->P[0].DOUTTGL = 1 << 11; /* toggle PA11 */
}

int main(void)
{
  /* Chip errata */
  CHIP_Init();
  CMU_ClockEnable(cmuClock_GPIO,1);
  GPIO_PinModeSet(gpioPortA,11,gpioModePushPull,0);
  setup_cryotimer(cryo_callback);
  /* Infinite loop */
  while (1) {
	  EMU_EnterEM3(0);
  }
}
