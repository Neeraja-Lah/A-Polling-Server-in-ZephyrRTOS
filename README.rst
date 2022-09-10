
**********************CSE522-Real Time Embedded Systems**********************
*****************************Assignment-4*************************************


Name - Neeraja Lahudva Kuppuswamy
ASU ID - 1224187432

Description : 

In this assignment, we have implemented a polling server on Zephyr as a periodic task (with a priority set to 6) that serves aperodic requests queued in a message queue. Polling server has a budget for every replenishment period. Once the server is invoked, it  serves the aperiodic arrival until the budget expires or the message queue is empty. At this point, the polling server still executes but in the background mode (with a priority of 14). The budget is tracked using tracing hooks and a budget timer to control how it consumes the budget. To replenish the budget, polling server is changed back to its original priority.  

******************************************************************************
*****************Steps to compile and execute the code*****************

1. Patch polling_p4.patch using the following command 'patch -u -b tracing_sysview.h -i ~/zephyrproject/RTES-LahudvaKuppuswamy-N_04/src/polling_p4.patch'

2. Unzip the RTES-LahudvaKuppuswamy-N_04.zip in the zephyrproject directory

3. To build, run west build -b mimxrt1050_evk project_4

4. Run west -v flash
	
5. Open putty and select port /dev/ttyACM0 and enter baud rate 115200





	

 

