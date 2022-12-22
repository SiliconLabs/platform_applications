# Trigger multiple ADC to sample data using PRS and GPIO which is processed using multiple tasks in Micrium OS  #

## Summary ##

This project configures ADCs to be triggered by GPIO via PRS. The sampled data is then transferred to memory via LDMA. The sampled data is then converted to voltages and printed on the serial terminal using multiple tasks in Micirum OS.  

## Gecko SDK version ##

v3.2.3

## Hardware Required ##

- SLSTK3701A EFM32 Giant Gecko GG11

## Setup ##

Import the included .sls file to Studio then build and flash the project to the SLSTK3701A. 

To manually build the project,
1. Create the Platform - I/O Stream USART on Micrium OS kernel project in Simplicity Studio
2. Copy the files in src and inc folders to the top level of the project. 
3. In app.c, modify the app_init() function to call `app_iostream_usart_init()` and `task_init()`. 
4. Replace the `app_iostream_usart_init()` function with the following code snippet. 

```
void app_iostream_usart_init(void)
{

  /* Prevent buffering of output/input.*/
#if !defined(__CROSSWORKS_ARM) && defined(__GNUC__)
  setvbuf(stdout, NULL, _IONBF, 0);   /*Set unbuffered mode for stdout (newlib)*/
  setvbuf(stdin, NULL, _IONBF, 0);   /*Set unbuffered mode for stdin (newlib)*/
#endif

}
```

5. Add the following components in the .slcp file
 - ADC
 - LDMA
 - PRS

## Connections Required ##

Connect pins PA13 (EXP Header Pin 5) and PE11 (EXP Header Pin 6) to the signals to be sampled. 

## How it Works ##

The ADCs are configured to be triggered by BTN0. This signal is sent via one of the PRS channels. The ADCs sample the analog signals in single conversion mode and store the data in the internal FIFO. Once the conversion is completed, the sampled data is copied from the FIFO to a buffer in memory. Each ADC has its own buffer in memory. Once the LDMA transfer is done, the data is converted into voltages in one task and printed on the serial terminal in the other task. Access to data and synchronization between tasks is implemented using sempahores, mutex and event flags.

For the SLSTK3701A, ADC0 and ADC1 are used. 

ADC0 - channel 13
ADC1 - channel 11 

ADC0 - Pin PA13 (EXP Header Pin 5)
ADC1 - Pin PE11 (EXP Header Pin 6)
