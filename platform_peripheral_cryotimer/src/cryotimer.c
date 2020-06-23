/*
 * cryotimer.c
 *
 * Setup cryotimer to generate 1 Hz interrupt
 * IRQ calls callback function for flexibility
 *
 */

#include "em_cmu.h"
#include "em_cryotimer.h"

static void (*cryo_irq_callback)(void);

void CRYOTIMER_IRQHandler(void) {
  uint32_t flags = CRYOTIMER_IntGet();
  CRYOTIMER_IntClear(flags);
  cryo_irq_callback();
}

int setup_cryotimer(void (*callback)(void)) {
	cryo_irq_callback = callback;
	CMU_ClockEnable(cmuClock_CRYOTIMER, true);
	CRYOTIMER_Init_TypeDef init = CRYOTIMER_INIT_DEFAULT;
	init.enable = 1;
	init.debugRun = 0;
	init.em4Wakeup = 0;
	init.osc = cryotimerOscULFRCO;   /* Use the 1 kHz ULFRCO */
	init.presc = cryotimerPresc_1; /* No prescaling, it is slow enough */
	init.period = 10; /* 1 kHz/1024 = 977 mHz*/
	CRYOTIMER_Init(&init);

	  // Enable cryotimer interrupts
	  CRYOTIMER_IntEnable(CRYOTIMER_IF_PERIOD);
	  NVIC_EnableIRQ(CRYOTIMER_IRQn);
	 return 0;
}
