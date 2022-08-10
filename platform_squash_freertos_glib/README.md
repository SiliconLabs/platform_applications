# Squash demo using FreeRTOS v1.0 #
![Type badge](https://img.shields.io/badge/Type-Virtual%20application-green)
![Technology badge](https://img.shields.io/badge/Technology-Platform-green)
![License badge](https://img.shields.io/badge/License-Zlib-green)
![SDK badge](https://img.shields.io/badge/SDK-v3.2.2-green)
![GCC badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/platform_applications/platform_squash_freertos_glib_gcc.json)

## Summary ##

This demo application demonstrates FreeRTOS tasks using interrupts.
The demo uses the integrated LCD, buttons and implements a simple squash game.
Buttons are used to move the racket up and down. 
Button0 moves down while button1 moves up the racket.

## Gecko SDK version ##

SDK 3.2.2

## Hardware required ##

Wireless Starter Kit Mainboard
Mighty Gecko Radio board  EFR32MG12P332F1024GL125 (BRD4162A Rev03)

## Setup ##

> **NOTE: All software components are added from**
>
> **SOFTWARE COMPONENTS configuration menu**

1. Create an "Empty C Project" project.
2. Add freeRTOS kernel and Heap4.
3. Disable configSUPPORT_STATIC_ALLOCATION in Simplicity Studio [Project_Name]\config\FeeRTOSConfig.h
4. Add Glib Graphics Library (Platform/Driver/GLIB Driver for SHARP Memory and GLIB Graphics Library) and add middleware/glib and middleware/glib/glib to include path if missing!
5. Add simple button driver (we don't use it we only need the interrupt handlers)
6. Optionally add LED component and enable LED_DEMO in squash_config.h. If you want to add LED module and its specific components are missing from file sl_event_handler.c add #include "sl_simple_led_instances.h" and sl_simple_led_init_instances(); call to sl_driver_init() function!
7. Remove sl_simple_button_init_instances() from sl_driver_init() function in file: [Project_Name]\autogen\sl_event_handler.c
8. Add squash.c, squash.h, squash_config.h and squash_api.h to the Project root.
9. Include squash_api.h in the project main.c
10. Invoke sl_squash_init() after system init but before starting scheduler (sl_system_kernel_start).

### Preparation steps after installation and before using the demo. ###

1. LCD module needs to be modified to enable LCD display if it is not implemented.
   Add the following line to LCD config file, if missing: [Project_Name]\config\sl_memlcd_config.h ( that may differ in newer SDK e.g. sl_memlcd_usart_config.h )  
   Display enable PIN

   ```C
   #define SL_MEMLCD_SPI_EN_PORT   gpioPortD  
   #define SL_MEMLCD_SPI_EN_PIN    15
   ```  

   Add the following lines after line "*GPIO_PinModeSet(SL_MEMLCD_SPI_CS_PORT, SL_MEMLCD_SPI_CS_PIN,gpioModePushPull, 0);*" in function sl_memlcd_configure(), in file: [SDK]\hardware\driver\memlcd\src\sl_memlcd.c

   ```C
   GPIO_PinModeSet(SL_MEMLCD_SPI_EN_PORT, SL_MEMLCD_SPI_EN_PIN, gpioModePushPull, 0);
   GPIO_PinOutSet(SL_MEMLCD_SPI_EN_PORT, SL_MEMLCD_SPI_EN_PIN);
   ```

2. Include the following in [SDK]\platform\emdrv\gpiointerrupt\src\gpiointerrupt.c

   ```C
   #include "FreeRTOS.h"  
   #include "semphr.h"
   
   extern xSemaphoreHandle xBSemaphore;  
   extern xSemaphoreHandle xBSemaphoreUp;
   ```

   Add the following lines to GPIO_EVEN_IRQHandler and GPIO_ODD_IRQHandler in file gpiointerrupt.c  

   ```C
   portBASE_TYPE xHigherPTWoken = 0;     
   xSemaphoreGiveFromISR( xBSemaphore, &xHigherPTWoken );  
   portYIELD_FROM_ISR( xHigherPTWoken );
   ```

   The should look like as follows:

   ```C
   void GPIO_EVEN_IRQHandler(void)
   {
     uint32_t iflags;
     portBASE_TYPE xHigherPTWoken = 0;

     /* Get all even interrupts. */
     iflags = GPIO_IntGetEnabled() & 0x00005555;

     /* Clean only even interrupts. */
     GPIO_IntClear(iflags);

     xSemaphoreGiveFromISR( xBSemaphore, &xHigherPTWoken );

     GPIOINT_IRQDispatcher(iflags);

     portYIELD_FROM_ISR( xHigherPTWoken );
   }

   void GPIO_ODD_IRQHandler(void)
   {
     uint32_t iflags;
     portBASE_TYPE xHigherPTWoken = 0;

     /* Get all even interrupts. */
     iflags = GPIO_IntGetEnabled() & 0x0000AAAA;

     /* Clean only even interrupts. */
     GPIO_IntClear(iflags);

     xSemaphoreGiveFromISR( xBSemaphore, &xHigherPTWoken );

     GPIOINT_IRQDispatcher(iflags);

     portYIELD_FROM_ISR( xHigherPTWoken );
   }
   ```

## How the Project Works ##
  
Three tasks control the game and an additional fourth task can be configured to blink the LED0.

Each press on any of the buttons triggers an interrupt that sends a semaphore to the associated task.
Tasks are blocked on the semaphores, IRQ handlers send the semaphore that unblocks the tasks ( vRacket_down, vRacket_up ) that perform moving operation of the racket.
The vBall function moves the ball.

## Porting to Another EFR32 Series Device ##

In other devices the buttons' PINs may differ, this can be set in [Project_Name]\squash_config.h header file under `/* GPIO Porting */` section and `/*  PINS here buttons are allocated   */`