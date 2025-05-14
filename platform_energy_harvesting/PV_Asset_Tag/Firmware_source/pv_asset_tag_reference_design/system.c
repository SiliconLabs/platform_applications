/***************************************************************************//**
 * @file system.c
 * @brief System init and control
 *******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
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

#include "system.h"

#include <em_cmu.h>
#include <em_emu.h>
#include <em_prs.h>

#include "sl_component_catalog.h"

// for GSDK 4.4.3 and below, if the ram_interrupt_vector_init component presents
// (unfortunately the component is not registered in the catalog)
// #define RAM_INTERRUPT_VECTOR_INIT

#define TUNE_ICACHE
// #define ENABLE_PTI
// #define ENABLE_MODEM_SIGS

#define PTI_DOUT_PRS                            10
#define PTI_DFRAME_PRS                          11

#if defined(SL_CATALOG_DEVICE_INIT_NVIC_PRESENT)
#include "sl_device_init_nvic.h"
#endif

#if defined(RAM_INTERRUPT_VECTOR_INIT)
#include "sl_ram_interrupt_vector_init.h"
#endif

#if defined(SL_CATALOG_INTERRUPT_MANAGER_PRESENT)
#include "sl_interrupt_manager.h"
#endif

#if defined(SL_CATALOG_DEVICE_INIT_HFRCO_PRESENT)
#include "sl_device_init_hfrco.h"
#endif

#if defined(SL_CATALOG_DEVICE_INIT_HFXO_PRESENT)
#include "sl_device_init_hfxo.h"
#endif

#if defined(SL_CATALOG_DEVICE_INIT_DCDC_PRESENT)
#include "sl_device_init_dcdc.h"
#endif

#if defined(SL_CATALOG_DEVICE_INIT_DPLL_PRESENT)
#include "sl_device_init_dpll.h"
#endif

#if defined(SL_CATALOG_DEVICE_INIT_LFRCO_PRESENT)
#include "sl_device_init_lfrco.h"
#endif

#if defined(SL_CATALOG_DEVICE_INIT_EMU_PRESENT)
#include "sl_device_init_emu.h"
#endif

#include "pin_config.h"

#if defined(_SILICON_LABS_32B_SERIES_1)
enum
{
  prsTypeDefault = prsTypeSync,
};
#define PRS_Setup(channel, sync, sourcesignal, port, pin, loc)                  \
  do {                                                                          \
    unsigned int __channel = (channel);                                         \
    unsigned int __sync = (sync);                                               \
    unsigned int __sourcesignal = (sourcesignal);                               \
    unsigned int __port = (port);                                               \
    unsigned int __pin = (pin);                                                 \
    unsigned int __loc = (loc);                                                 \
    BUS_RegBitWrite(&PRS->ROUTEPEN, _PRS_ROUTEPEN_CH0PEN_SHIFT + __channel, 0); \
    GPIO_PinModeSet(__port, __pin, gpioModePushPull, 0);                        \
    PRS_ConnectSignal(__channel, __sync, __sourcesignal);                       \
    PRS_GpioOutputLocation(__channel, __loc);                                   \
  } while (0)
#elif defined(_SILICON_LABS_32B_SERIES_2)
enum
{
  prsTypeDefault = prsTypeAsync,
};
#define PRS_Setup(channel, sync, sourcesignal, port, pin, loc)                  \
  do { \
    unsigned int __channel = (channel);                                         \
    unsigned int __sync = (sync);                                               \
    unsigned int __sourcesignal = (sourcesignal);                               \
    unsigned int __port = (port);                                               \
    unsigned int __pin = (pin);                                                 \
    BUS_RegBitWrite(&GPIO->PRSROUTE[0].ROUTEEN, __channel +                     \
                    ((__sync == prsTypeAsync) ?                                 \
                        _GPIO_PRS_ROUTEEN_ASYNCH0PEN_SHIFT :                    \
                        _GPIO_PRS_ROUTEEN_SYNCH0PEN_SHIFT), 0);                 \
    GPIO_PinModeSet(__port, __pin, gpioModePushPull, 0);                        \
    PRS_ConnectSignal(__channel, __sync, __sourcesignal);                       \
    PRS_PinOutput(__channel, __sync, __port, __pin);                            \
  } while (0)
#else
#error "Please add an implementation for PRS_Setup()"
#endif

// Provide backwards compatible names for cleaner code
#if !defined(PRS_MODEM_FRAMEDET) && defined(PRS_MODEML_FRAMEDET)
#define PRS_MODEM_FRAMEDET                      PRS_MODEML_FRAMEDET
#endif

#if !defined(PRS_MODEM_TIMDET) && defined(PRS_MODEMH_TIMDET)
#define PRS_MODEM_TIMDET                        PRS_MODEMH_TIMDET
#endif

#if !defined(PRS_MODEM_SYNCSENT) && defined(PRS_MODEMH_SYNCSENT)
#define PRS_MODEM_SYNCSENT                      PRS_MODEMH_SYNCSENT
#endif

#if !defined(PRS_MODEM_PRESENT) && defined(PRS_MODEMH_PRESENT)
#define PRS_MODEM_PRESENT                       PRS_MODEMH_PRESENT
#endif

#if !defined(PRS_MODEM_DOUT)
#if defined(PRS_MODEML_DOUT)
#define PRS_MODEM_DOUT                          PRS_MODEML_DOUT
#else
// https://community.silabs.com/s/article/rail-tutorial-debugging
// https://community.silabs.com/s/question/0D51M00007xeOXBSA2/efr32-rail-getting-out-the-tx-packet-frame-to-a-gpio-pin
#define PRS_MODEM_DOUT                          0x5607
#endif
#endif

#if !defined(PRS_MODEM_DCLK)
#if defined(PRS_MODEML_DCLK)
#define PRS_MODEM_DCLK                          PRS_MODEML_DCLK
#else
#define PRS_MODEM_DCLK                          0x5606
#endif
#endif

#if !defined(PRS_RAC_TX) && defined(PRS_RACL_TX)
#define PRS_RAC_TX                              PRS_RACL_TX
#endif

#if !defined(PRS_RAC_RX) && defined(PRS_RACL_RX)
#define PRS_RAC_RX                              PRS_RACL_RX
#endif

#if !defined(PRS_RAC_ACTIVE) && defined(PRS_RACL_ACTIVE)
#define PRS_RAC_ACTIVE                          PRS_RACL_ACTIVE
#endif

void system_init(void)
{
#if defined(SL_CATALOG_DEVICE_INIT_NVIC_PRESENT)
  sl_device_init_nvic();
#endif

#if defined(RAM_INTERRUPT_VECTOR_INIT)
  sl_ram_interrupt_vector_init();
#endif

#if defined(SL_CATALOG_INTERRUPT_MANAGER_PRESENT)
  sl_interrupt_manager_init();
#endif

#if defined(SL_CATALOG_DEVICE_INIT_DCDC_PRESENT)
  sl_device_init_dcdc();
#endif

  NVIC_DisableIRQ(DCDC_IRQn);

  DCDC->IF_CLR = _DCDC_IF_MASK;
  EMU->IF_CLR = _EMU_IF_MASK;
  NVIC_ClearPendingIRQ(DCDC_IRQn);

#if defined(SL_CATALOG_DEVICE_INIT_HFRCO_PRESENT)
  sl_device_init_hfrco();
#endif

#if defined(SL_CATALOG_DEVICE_INIT_DPLL_PRESENT)
  sl_device_init_dpll();
#endif

#if defined(SL_CATALOG_DEVICE_INIT_LFRCO_PRESENT)
  sl_device_init_lfrco();
#endif

  CMU_ClockSelectSet(cmuClock_SYSCLK, cmuSelect_HFRCODPLL);

#if defined(_CMU_EM01GRPACLKCTRL_MASK)
  CMU_ClockSelectSet(cmuClock_EM01GRPACLK, cmuSelect_HFRCODPLL);
#endif

#if defined(_CMU_EM01GRPBCLKCTRL_MASK)
  CMU_ClockSelectSet(cmuClock_EM01GRPBCLK, cmuSelect_HFRCODPLL);
#endif

#if defined(_CMU_EM01GRPCCLKCTRL_MASK)
  CMU_ClockSelectSet(cmuClock_EM01GRPCCLK, cmuSelect_HFRCODPLL);
#endif

#if defined(ULFRCO_FOR_EM2)
  CMU_ClockSelectSet(cmuClock_EM23GRPACLK, cmuSelect_ULFRCO);
#else
  CMU_ClockSelectSet(cmuClock_EM23GRPACLK, cmuSelect_LFRCO);
#endif

  SCB->SCR |= SCB_SCR_SEVONPEND_Msk;

#if defined(SL_CATALOG_DEVICE_INIT_EMU_PRESENT)
  sl_device_init_emu();
#endif

  EMU_EM01Init_TypeDef em01Init = EMU_EM01INIT_DEFAULT;
  em01Init.vScaleEM01LowPowerVoltageEnable = true;
  EMU_EM01Init(&em01Init);

  EMU_EM23Init_TypeDef em23Init = EMU_EM23INIT_DEFAULT;
  em23Init.vScaleEM23Voltage = emuVScaleEM23_LowPower;
  EMU_EM23Init(&em23Init);

  CMU_ClockDivSet(cmuClock_HCLK, 1);

#if defined(DISABLE_ICACHE)
  // power down the ICACHE
  CMU_ClockEnable(cmuClock_ICACHE, true);
  BUS_RegBitWrite(&ICACHE0->CTRL, _ICACHE_CTRL_CACHEDIS_SHIFT, 1);
  BUS_RegMaskedWrite(&SYSCFG->ICACHERAMRETNCTRL, _SYSCFG_ICACHERAMRETNCTRL_RAMRETNCTRL_MASK,
                   SYSCFG_ICACHERAMRETNCTRL_RAMRETNCTRL_ALLOFF);
  CMU_ClockEnable(cmuClock_ICACHE, false);
#elif defined(TUNE_ICACHE)
  CMU_ClockEnable(cmuClock_ICACHE, true);
  BUS_RegBitWrite(&ICACHE0->CTRL, _ICACHE_CTRL_CACHEDIS_SHIFT, 0);
  BUS_RegMaskedWrite(&ICACHE0->LPMODE, _ICACHE_LPMODE_LPLEVEL_MASK, ICACHE_LPMODE_LPLEVEL_ADVANCED);
  CMU_ClockEnable(cmuClock_ICACHE, false);
#endif

#if defined(TUNE_WS0)
  MSC_ExecConfig_TypeDef execConfig = MSC_EXECCONFIG_DEFAULT;
  // disable the data out pipeline (the power-on default state is enabled)
  // it's somewhat superfluous if the flash runs at WS0
  // however wastes power if the fetched data does get dropped
  execConfig.doutBufEn = false;
  MSC_Init();
  MSC_ExecConfigSet(&execConfig);
  MSC_Deinit();
  // disable folding of IT blocks
  // it's somewhat superfluous if the flash runs at WS0
  // however wastes power if the fetched data does get dropped
  SCnSCB->ACTLR |= SCnSCB_ACTLR_DISFOLD_Msk;
#endif
}

void setup_modem_signals(void)
{
#if defined(ENABLE_PTI)
  RAIL_PtiConfig_t ptiConfig = {
    .mode = RAIL_PTI_MODE_UART,
    .baud = 1600000,
    .doutPort = PTI_DOUT_PORT,
    .doutPin = PTI_DOUT_PIN,
    .dframePort = PTI_DFRAME_PORT,
    .dframePin = PTI_DFRAME_PIN,
  };

  RAIL_ConfigPti(RAIL_EFR32_HANDLE, &ptiConfig);
  RAIL_EnablePti(RAIL_EFR32_HANDLE, true);
  RAIL_SetPtiProtocol(RAIL_EFR32_HANDLE, RAIL_PTI_PROTOCOL_BLE);
#elif defined(ENABLE_MODEM_SIGS)
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(cmuClock_PRS, true);

  PRS_Setup(PTI_DOUT_PRS, prsTypeDefault, PRS_MODEM_DOUT, PTI_DOUT_PORT,
            PTI_DOUT_PIN, PTI_DOUT_LOC);
  PRS_Setup(PTI_DFRAME_PRS, prsTypeDefault, PRS_MODEM_DCLK, PTI_DFRAME_PORT,
            PTI_DFRAME_PIN, PTI_DFRAME_LOC);
#endif
}

void prepare_hfxo(void)
{
#if defined(SL_CATALOG_DEVICE_INIT_HFXO_PRESENT)
  sl_device_init_hfxo();
#endif

  // enable now to hide at least a part of the startup time
  HFXO0->CTRL_SET = HFXO_CTRL_FORCEEN;
}

void select_hfxo(void)
{
  CMU_ClockSelectSet(cmuClock_SYSCLK, cmuSelect_HFXO);
  HFXO0->CTRL_CLR = HFXO_CTRL_FORCEEN;
}
