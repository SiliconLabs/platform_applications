#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

// $[CMU]
// [CMU]$

// $[LFXO]
// LFXO LFXTAL_I on PD01
#ifndef LFXO_LFXTAL_I_PORT                      
#define LFXO_LFXTAL_I_PORT                       gpioPortD
#endif
#ifndef LFXO_LFXTAL_I_PIN                       
#define LFXO_LFXTAL_I_PIN                        1
#endif

// LFXO LFXTAL_O on PD00
#ifndef LFXO_LFXTAL_O_PORT                      
#define LFXO_LFXTAL_O_PORT                       gpioPortD
#endif
#ifndef LFXO_LFXTAL_O_PIN                       
#define LFXO_LFXTAL_O_PIN                        0
#endif

// [LFXO]$

// $[PRS.ASYNCH0]
// [PRS.ASYNCH0]$

// $[PRS.ASYNCH1]
// [PRS.ASYNCH1]$

// $[PRS.ASYNCH2]
// [PRS.ASYNCH2]$

// $[PRS.ASYNCH3]
// [PRS.ASYNCH3]$

// $[PRS.ASYNCH4]
// [PRS.ASYNCH4]$

// $[PRS.ASYNCH5]
// [PRS.ASYNCH5]$

// $[PRS.ASYNCH6]
// [PRS.ASYNCH6]$

// $[PRS.ASYNCH7]
// [PRS.ASYNCH7]$

// $[PRS.ASYNCH8]
// [PRS.ASYNCH8]$

// $[PRS.ASYNCH9]
// [PRS.ASYNCH9]$

// $[PRS.ASYNCH10]
// [PRS.ASYNCH10]$

// $[PRS.ASYNCH11]
// [PRS.ASYNCH11]$

// $[PRS.SYNCH0]
// [PRS.SYNCH0]$

// $[PRS.SYNCH1]
// [PRS.SYNCH1]$

// $[PRS.SYNCH2]
// [PRS.SYNCH2]$

// $[PRS.SYNCH3]
// [PRS.SYNCH3]$

// $[GPIO]
// GPIO EM4WU3 on PB01
#ifndef GPIO_EM4WU3_PORT                        
#define GPIO_EM4WU3_PORT                         gpioPortB
#endif
#ifndef GPIO_EM4WU3_PIN                         
#define GPIO_EM4WU3_PIN                          1
#endif

// GPIO EM4WU4 on PB03
#ifndef GPIO_EM4WU4_PORT                        
#define GPIO_EM4WU4_PORT                         gpioPortB
#endif
#ifndef GPIO_EM4WU4_PIN                         
#define GPIO_EM4WU4_PIN                          3
#endif

// GPIO EM4WU6 on PC00
#ifndef GPIO_EM4WU6_PORT                        
#define GPIO_EM4WU6_PORT                         gpioPortC
#endif
#ifndef GPIO_EM4WU6_PIN                         
#define GPIO_EM4WU6_PIN                          0
#endif

// GPIO EM4WU8 on PC07
#ifndef GPIO_EM4WU8_PORT                        
#define GPIO_EM4WU8_PORT                         gpioPortC
#endif
#ifndef GPIO_EM4WU8_PIN                         
#define GPIO_EM4WU8_PIN                          7
#endif

// GPIO EM4WU9 on PD02
#ifndef GPIO_EM4WU9_PORT                        
#define GPIO_EM4WU9_PORT                         gpioPortD
#endif
#ifndef GPIO_EM4WU9_PIN                         
#define GPIO_EM4WU9_PIN                          2
#endif

// GPIO SWCLK on PA01
#ifndef GPIO_SWCLK_PORT                         
#define GPIO_SWCLK_PORT                          gpioPortA
#endif
#ifndef GPIO_SWCLK_PIN                          
#define GPIO_SWCLK_PIN                           1
#endif

// GPIO SWDIO on PA02
#ifndef GPIO_SWDIO_PORT                         
#define GPIO_SWDIO_PORT                          gpioPortA
#endif
#ifndef GPIO_SWDIO_PIN                          
#define GPIO_SWDIO_PIN                           2
#endif

// GPIO SWV on PA03
#ifndef GPIO_SWV_PORT                           
#define GPIO_SWV_PORT                            gpioPortA
#endif
#ifndef GPIO_SWV_PIN                            
#define GPIO_SWV_PIN                             3
#endif

// [GPIO]$

// $[TIMER0]
// [TIMER0]$

// $[TIMER1]
// [TIMER1]$

// $[TIMER2]
// [TIMER2]$

// $[TIMER3]
// [TIMER3]$

// $[TIMER4]
// [TIMER4]$

// $[USART0]
// USART0 CTS on PA08
#ifndef USART0_CTS_PORT                         
#define USART0_CTS_PORT                          gpioPortA
#endif
#ifndef USART0_CTS_PIN                          
#define USART0_CTS_PIN                           8
#endif

// USART0 RTS on PA07
#ifndef USART0_RTS_PORT                         
#define USART0_RTS_PORT                          gpioPortA
#endif
#ifndef USART0_RTS_PIN                          
#define USART0_RTS_PIN                           7
#endif

// USART0 RX on PA06
#ifndef USART0_RX_PORT                          
#define USART0_RX_PORT                           gpioPortA
#endif
#ifndef USART0_RX_PIN                           
#define USART0_RX_PIN                            6
#endif

// USART0 TX on PA05
#ifndef USART0_TX_PORT                          
#define USART0_TX_PORT                           gpioPortA
#endif
#ifndef USART0_TX_PIN                           
#define USART0_TX_PIN                            5
#endif

// [USART0]$

// $[USART1]
// [USART1]$

// $[I2C1]
// [I2C1]$

// $[PDM]
// [PDM]$

// $[LETIMER0]
// [LETIMER0]$

// $[IADC0]
// IADC0 SCAN0POS on PB04
#ifndef IADC0_SCAN0POS_PORT                     
#define IADC0_SCAN0POS_PORT                      gpioPortB
#endif
#ifndef IADC0_SCAN0POS_PIN                      
#define IADC0_SCAN0POS_PIN                       4
#endif

// IADC0 SCAN1POS on PB02
#ifndef IADC0_SCAN1POS_PORT                     
#define IADC0_SCAN1POS_PORT                      gpioPortB
#endif
#ifndef IADC0_SCAN1POS_PIN                      
#define IADC0_SCAN1POS_PIN                       2
#endif

// [IADC0]$

// $[I2C0]
// [I2C0]$

// $[EUART0]
// [EUART0]$

// $[PTI]
// PTI DFRAME on PC05
#ifndef PTI_DFRAME_PORT                         
#define PTI_DFRAME_PORT                          gpioPortC
#endif
#ifndef PTI_DFRAME_PIN                          
#define PTI_DFRAME_PIN                           5
#endif

// PTI DOUT on PC04
#ifndef PTI_DOUT_PORT                           
#define PTI_DOUT_PORT                            gpioPortC
#endif
#ifndef PTI_DOUT_PIN                            
#define PTI_DOUT_PIN                             4
#endif

// [PTI]$

// $[MODEM]
// [MODEM]$

// $[CUSTOM_PIN_NAME]
#ifndef CALIB_PORT                              
#define CALIB_PORT                               gpioPortA
#endif
#ifndef CALIB_PIN                               
#define CALIB_PIN                                0
#endif

#ifndef _PORT                                   
#define _PORT                                    gpioPortA
#endif
#ifndef _PIN                                    
#define _PIN                                     1
#endif

#ifndef LED0_PORT                               
#define LED0_PORT                                gpioPortA
#endif
#ifndef LED0_PIN                                
#define LED0_PIN                                 4
#endif

#ifndef VCOM_TX_PORT                            
#define VCOM_TX_PORT                             gpioPortA
#endif
#ifndef VCOM_TX_PIN                             
#define VCOM_TX_PIN                              5
#endif

#ifndef VCOM_RX_PORT                            
#define VCOM_RX_PORT                             gpioPortA
#endif
#ifndef VCOM_RX_PIN                             
#define VCOM_RX_PIN                              6
#endif

#ifndef VCOM_RTS_PORT                           
#define VCOM_RTS_PORT                            gpioPortA
#endif
#ifndef VCOM_RTS_PIN                            
#define VCOM_RTS_PIN                             7
#endif

#ifndef VCOM_CTS_PORT                           
#define VCOM_CTS_PORT                            gpioPortA
#endif
#ifndef VCOM_CTS_PIN                            
#define VCOM_CTS_PIN                             8
#endif

#ifndef MODE_SELECT_PORT                        
#define MODE_SELECT_PORT                         gpioPortB
#endif
#ifndef MODE_SELECT_PIN                         
#define MODE_SELECT_PIN                          0
#endif

#ifndef ST_STO_RDY_PORT                         
#define ST_STO_RDY_PORT                          gpioPortB
#endif
#ifndef ST_STO_RDY_PIN                          
#define ST_STO_RDY_PIN                           1
#endif

#ifndef VADC_VSTO_PORT                          
#define VADC_VSTO_PORT                           gpioPortB
#endif
#ifndef VADC_VSTO_PIN                           
#define VADC_VSTO_PIN                            2
#endif

#ifndef ST_LOAD_PORT                            
#define ST_LOAD_PORT                             gpioPortB
#endif
#ifndef ST_LOAD_PIN                             
#define ST_LOAD_PIN                              3
#endif

#ifndef VADC_VOPV_PORT                          
#define VADC_VOPV_PORT                           gpioPortB
#endif
#ifndef VADC_VOPV_PIN                           
#define VADC_VOPV_PIN                            4
#endif

#ifndef WAKE_UP_PORT                            
#define WAKE_UP_PORT                             gpioPortC
#endif
#ifndef WAKE_UP_PIN                             
#define WAKE_UP_PIN                              0
#endif

#ifndef MODE_SWITCH0_PORT                       
#define MODE_SWITCH0_PORT                        gpioPortC
#endif
#ifndef MODE_SWITCH0_PIN                        
#define MODE_SWITCH0_PIN                         1
#endif

#ifndef MODE_SWITCH1_PORT                       
#define MODE_SWITCH1_PORT                        gpioPortC
#endif
#ifndef MODE_SWITCH1_PIN                        
#define MODE_SWITCH1_PIN                         2
#endif

#ifndef ENABLE_VSTO_PORT                        
#define ENABLE_VSTO_PORT                         gpioPortC
#endif
#ifndef ENABLE_VSTO_PIN                         
#define ENABLE_VSTO_PIN                          3
#endif

#ifndef ENABLE_VOPV_PORT                        
#define ENABLE_VOPV_PORT                         gpioPortC
#endif
#ifndef ENABLE_VOPV_PIN                         
#define ENABLE_VOPV_PIN                          6
#endif

#ifndef BUTTON0_PORT                            
#define BUTTON0_PORT                             gpioPortC
#endif
#ifndef BUTTON0_PIN                             
#define BUTTON0_PIN                              7
#endif

#ifndef ST_STO_OVDIS_PORT                       
#define ST_STO_OVDIS_PORT                        gpioPortD
#endif
#ifndef ST_STO_OVDIS_PIN                        
#define ST_STO_OVDIS_PIN                         2
#endif

// [CUSTOM_PIN_NAME]$

#endif // PIN_CONFIG_H

