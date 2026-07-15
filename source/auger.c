/*
 * auger.c
 *
 *  Created on: 29-Aug-2024
 *      Author: Jawahar Arumugam
 */

/*****************************Module Description: CT Module**************************
 *
 * This module when enabled controls Auger rotation in the following sequence
 * 		- Rotates Auger in CW direction for 	CT_CWR_DURATION seconds
 * 		- Rotates Auger in CCW direction for 	CT_CCWR_DURATION seconds
 * 		- Stops Auger rotation for  			CT_OFF_DURATION seconds
 *
 * This module, on power up, starts with IDLE state in which it keeps the Auger OFF.
 * This module supports following three commands
 * 		CT_START - start the auger rotation in the sequence specified above
 * 		CT_STOP  - Stops auger control. ie IDLE state
 * 		CT_CONFIGURE - Change the default duration values for auger control
 ************************************************************************************/

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"

/* Standard C includes */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* NXP includes */
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
//#include "fsl_debug_console.h"
#include "fsl_spc.h"
#include "fsl_lpi2c.h"
#include "fsl_lpuart.h"
#include "fsl_device_registers.h"
#include "fsl_lpspi.h"

/* Chewie Includes */
#include "dgCommon.h"
#include "version.h"
#include "modulecom.h"
#include "dgtimer.h"
#include "GPIOSignals.h"
#include "dgI2cDriver.h"
#include "rtc.h"
#include "ASB_HMI_common.h"
#include "sysStart.h"
#include "dgUartDriverCommon.h"
#include "CliUartDriver.h"
#include "cliProc.h"
#include "sysConfig.h"
#include "mclsSPIDriver.h"
#include "drv89xxDriver.h"
#include "drv89xxRegisters.h"
#include "mclsSPIDriver.h"
#include "drv89xxDriver.h"
#include "drv89xxRegisters.h"
#include "actuatorCtrl.h"
#include "motorControl.h"
#include "augerAPI.h"

/*******************************************************************************
 * MACRO Definitions
 ******************************************************************************/

#define CT_OFF_DURATION_DEFAULT  100

/******************************* Auger states *********************************/
#define CT_STATE_IDLE		0     	//Default state after powerup
#define CT_STATE_CWR		1		//Currently rotating auger in clockwise direction
#define CT_STATE_CWRPAUSE	2		//Pause after Clockwise rotation
#define CT_STATE_CCWR		3		//Currently rotating auger in counter clockwise direction
#define CT_STATE_CCWRPAUSE	4		//Pause after Counter Clockwise rotation
#define CT_STATE_OFF		5		//Currently in OFF period - auger not rotating


/*******************************************************************************
 * Global Variables
 ******************************************************************************/

/*******************************************************************************
 * Implementation
 ******************************************************************************/


static void ct_task(void *pvParameters)
{
	QueueHandle_t ctQHandle;
	dgMsg_t rcvMsg;				//Holds the currently received message
	static int ctState;			//
	int curCount;
	dgCtConfigParam_t ctConfig;

	//Get the Qhandle for this task and store locally
	ctQHandle = getQHandle(CT_MOD);

	//Initialize the state of CT to IDLE
	ctState = CT_STATE_IDLE;

	//Read configuration from EEPROM
	if(getAugerConfig(&ctConfig)!= DG_SUCCESS)
	{
        printf("auger.c:ct_task():Read ctconfig Failure \r\n");
	}
	else
	{
        printf("auger.c:ct_task():Read ctconfig Success:ccwr:%d sec, cwr:%d sec, pause:%d sec\r\n",ctConfig.ccwrDuration, ctConfig.cwrDuration,ctConfig.pauseDuration);
	}



	while(1)
	{
		//Receive event from Queue. Block until event is available
		if(xQueueReceive(ctQHandle, &rcvMsg, portMAX_DELAY ) != pdPASS )
		{
			// Queue did not return an event. Hence go back
			continue;
		}
		//printf("CT.c:ct_task():cmd rcvd %d in state %d\r\n",rcvMsg.command,ctState);
		switch(rcvMsg.command)
		{
			case CT_START:
				//printf("ct.c:ct_Task: CT_START command  received)\r\n");
				if(ctState == CT_STATE_IDLE)
				{
					//Change ct state to clockwise rotation
					ctState = CT_STATE_CWR;

					//Start motor in CWR
					augerMotorCWR();

					curCount = ctConfig.rptCount;
					//start timer for CW duration
					dgtimerStart(CT_MOD, CONV_SEC_TO_TICKS(ctConfig.cwrDuration));
					*rcvMsg.result = DG_SUCCESS;
				}
				else
				{
					*rcvMsg.result = DG_FAIL;
					//printf("ct.c:ct_Task: CT_START received in state %d and csEnable=%d ignored\r\n",ctState, csEnable );
				}
				if(rcvMsg.taskHandleSM != NULL)
				{
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				break;

			case CT_STOP:
				//printf("ct.c:ct_Task: CT_STOP command  received)\r\n");
				ctState = CT_STATE_IDLE;
				dgtimerStop(CT_MOD);

				//Turn OFF auger motor
				augerMotorStop();

				*rcvMsg.result = DG_SUCCESS;
				if(rcvMsg.taskHandleSM != NULL)
				{
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				break;


			case DG_TIMER_EXPIRY:
				//printf("ct.c:ct_Task: Timer expiry event  received in %d state, count=%d\r\n",ctState, curCount);
				switch (ctState)
				{
				case CT_STATE_IDLE:
					//printf("ct.c:ct_Task: Timer event in IDLE state. Ignored)\r\n");
					break;

				case CT_STATE_CWR:
					//Turn off motor
					augerMotorStop();
					ctState = CT_STATE_CWRPAUSE;
					//Set timer for ccw duration
					dgtimerStart(CT_MOD, CONV_SEC_TO_TICKS(ctConfig.pauseDuration));

					break;
				case CT_STATE_CWRPAUSE:
					//Change motor direction from CW to CCW
					augerMotorCCWR();
					ctState = CT_STATE_CCWR;
					//Set timer for ccw duration
					dgtimerStart(CT_MOD, CONV_SEC_TO_TICKS(ctConfig.ccwrDuration));
					break;

				case CT_STATE_CCWR:
					//Turn off auger motor
					augerMotorStop();
					ctState = CT_STATE_CCWRPAUSE;
					//Set timer for off duration
					dgtimerStart(CT_MOD, CONV_SEC_TO_TICKS(ctConfig.pauseDuration));
					break;

				case CT_STATE_CCWRPAUSE:
					//One cycle is over. Check repeat count
					curCount--;
					if (curCount <= 0)  //Move to OFF state
					{
						if(ctConfig.offDuration == 0)
						{
							//ONECYCLE
							ctState = CT_STATE_IDLE;
							augerMotorStop();
							dgtimerStop(CT_MOD);
						}
						else
						{
							ctState = CT_STATE_OFF;
							augerMotorStop();
							dgtimerStart(CT_MOD, CONV_SEC_TO_TICKS(ctConfig.offDuration));
						}

					}
					else	//Move to CW Rotation state
					{
						//Change motor direction from CW to CCW
						augerMotorCWR();
						ctState = CT_STATE_CWR;
						//Set timer for ccw duration
						dgtimerStart(CT_MOD, CONV_SEC_TO_TICKS(ctConfig.cwrDuration));
					}

					break;

				case CT_STATE_OFF:
					//Change ct state to clockwise rotation
					ctState = CT_STATE_CWR;
					//set auger motor direction to CW and turn ON motor
					augerMotorCWR();
					curCount = ctConfig.rptCount;
					//start timer for CW duration
					dgtimerStart(CT_MOD, CONV_SEC_TO_TICKS(ctConfig.cwrDuration));
					break;

				default:
					printf("auger.c:ct_Task: Unkown state. Ignored)\r\n");
					break;
				}
				break;

			default:
				//Invalid command or event ids
				printf("auger.c:CTTask:Invalid command %d received)\r\n",rcvMsg.command );
				*rcvMsg.result = DG_FAIL;
				if(rcvMsg.taskHandleSM != NULL)
				{
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				break;
		}
	}

}

int initCt(void)
{
	//Create ct task task
	TaskHandle_t ctTaskHandle;
	TimerHandle_t ctTimerHandle;
	BaseType_t result;

	result = xTaskCreate(ct_task, "ct_task", configMINIMAL_STACK_SIZE + 100, NULL, task_PRIORITY, &ctTaskHandle);
    if ( result !=    pdPASS)
    {
        printf("auger.c:initCt():CT Task creation failed!.\r\n");
        return DG_FAIL;
    }
    //CT requires timer and hence create FreeRTOS SW timer
    ctTimerHandle = xTimerCreate("ctTimer",CT_OFF_DURATION_DEFAULT, pdFALSE, (void*)CT_MOD, dgTimerCallback);
    if(ctTimerHandle == NULL)
    {
        printf("auger.c:initCt():Timer creation failed!.\r\n");
    	vTaskDelete(ctTaskHandle);
        return DG_FAIL;
    }
    if(registerModule(CT_MOD, ctTaskHandle, ctTimerHandle)!= DG_SUCCESS)
    {
    	//Registering the module failed. Hence kill the task and return error
        printf("auger.c:initCt():CT Task registration failed!.\r\n");
        xTimerDelete(ctTimerHandle,100 / portTICK_PERIOD_MS);
    	vTaskDelete(ctTaskHandle);

    	return DG_FAIL;
    }
    return DG_SUCCESS;
}





