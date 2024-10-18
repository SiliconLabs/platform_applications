# I2C leader interrupt based C Example

## Summary
This project shows how to configure the I2C periphreal in leader mode,
and use interrupt based routine to perform I2C transfers with a 
secondary device. The I2C state machine is implemented in the I2C interrupt
handler. Both read and write operations are supported and can be configured
using the push buttons.

## Gecko SDK Version
v4.4.3

## Hardware Required

* Board:  2x Silicon Labs EFR32xG24 Starter Kit + WSTK (BRD4186C + BRD4001A)
	* Device: EFR32MG24B210F1536IM48

## Setup
Clone the repository with this project from GitHub onto your local machine.

From within the Simplicity Studio IDE, select Import -> MCU Project... from the 
Project menu. Click the Browse button and navigate to the local repository 
folder, then to the SimplicityStudio folder, select the .slsproj/.sls file for the 
board, click the Next button twice, and then click Finish.

Import the [I2C_Follower](https://github.com/SiliconLabs/peripheral_examples/tree/master/series2/i2c/i2c_follower) example from the peripehral examples repo
	

Connect the GND, I2C_SCL, and I2C_SDA pins between the two boards and use a
logic analyzer to probe the SCL and SDA pin. Or run the project in debug mode
and check the receiveBuffer for read operations.

I2C_SCL -> PC05 EXP 15  
I2C_SDA -> PC07 EXP 16

Press Push Button 0 for read request from leader to follower
Press Push Button 1 for write request from leader to follower

## How the Project Works
The I2C leader state machine is configured in the I2C handler using different
I2C interrupt. The state machine itself is documented in the EFR32xg24 reference
manual's I2C chapter.
The I2C follower example is used to test the leader project, but ideally any I2C
follower device should work as long as the follower's address is written correctly.

## .sls Projects Used
* i2c_leader_interrupt.sls

## How to Port to Another Part
Right click on the project and select "Properties" and navigate to "C/C++ 
Build" then "Board/Part/SDK". Select the new board or part to target and apply 
the changes. There may be some dependencies that need to be resolved when 
changing the target architecture. This project can only be run on other 
Series 1 devices. 