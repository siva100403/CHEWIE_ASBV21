/*
 * transferCS.c
 *
 *  Created on: 31-Dec-2024
 *      Author: Jawahar Arumugam
 */



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
#include "fsl_debug_console.h"
#include "fsl_spc.h"
#include "fsl_lpi2c.h"
#include "fsl_lpuart.h"
#include "fsl_device_registers.h"
#include "fsl_lpspi.h"
#include "fsl_utick.h"

/* Chewie Includes */
#include "dgCommon.h"
#include "version.h"
#include "modulecom.h"
#include "dgtimer.h"
#include "GPIOSignals.h"
#include "seqControlCommon.h"
#include "dgI2cDriver.h"
#include "eeConfig.h"
#include "rtc.h"
#include "ASB_HMI_common.h"
#include "sysStart.h"
#include "dgUartDriverCommon.h"
#include "CliUartDriver.h"
#include "hmiUartDriver.h"
#include "cliProc.h"
#include "sysConfig.h"
#include "mclsSPIDriver.h"
#include "drv89xxDriver.h"
#include "drv89xxRegisters.h"
#include "mclsSPIDriver.h"
#include "drv89xxDriver.h"
#include "drv89xxRegisters.h"
#include "actuatorCtrl.h"
#include "sensorMod.h"
#include "augerAPI.h"
#include "shredder.h"
#include "shredderAPI.h"
#include "adcs.h"
#include "limitSwitchMod.h"
#include "transferCS.h"


/***************************Regen Soil Transfer Logic***************************
 * -----------------------------------------------------------------------------
 * | Transfer State | CT Motor | ST Motor | Duration | Rpt Cycles |
 * |----------------|----------|----------|----------|------------|
 * | IDLE           |  OFF     | OFF      | NA       | NA         |
 * |----------------|----------|----------|----------|------------|
 * | TRANSFERRING	| CWR      | ON       |  2 min   |     3      |
 * |                | OFF      | OFF      |  1 min   |            |
 * |----------------|----------|----------|----------|------------|
 */

/********************** Resources Used ****************************************/

//AUGER Motor
//ST Motor:


/***********************Configuration Parameters******************************/

const dgTransferCtrlSeq_t trfCtrlSeq[] = { {2, SEQ_CTRL_CTMOTOR_CCWR, SEQ_CTRL_STMOTOR_ON, SEQ_CTRL_START },
		                                   {1, SEQ_CTRL_CTMOTOR_OFF,  SEQ_CTRL_STMOTOR_OFF,SEQ_CTRL_MID },
										   {2, SEQ_CTRL_CTMOTOR_CCWR, SEQ_CTRL_STMOTOR_ON, SEQ_CTRL_MID },
										   {1, SEQ_CTRL_CTMOTOR_OFF,  SEQ_CTRL_STMOTOR_OFF,SEQ_CTRL_MID },
										   {2, SEQ_CTRL_CTMOTOR_CCWR, SEQ_CTRL_STMOTOR_ON, SEQ_CTRL_MID },
										   {1, SEQ_CTRL_CTMOTOR_OFF,  SEQ_CTRL_STMOTOR_OFF,SEQ_CTRL_END },
};

/*******************************************************************************
 * Definitions
 ******************************************************************************/



int executeTRFRSeqControl(uint8_t condition)
{
	static uint8_t controlSeqIndex=0;

	if(condition == SEQ_ENGINE_START)
	{
		controlSeqIndex=0;
	}

	if(trfCtrlSeq[controlSeqIndex].ctCtrl == SEQ_CTRL_CTMOTOR_CCWR)
	{
		augerMotorCWR();
	}
	else if(trfCtrlSeq[controlSeqIndex].ctCtrl == SEQ_CTRL_CTMOTOR_OFF)
	{
		augerMotorStop();
	}
	else
	{
		//Unexpected control
		augerMotorStop();
	}

	if(trfCtrlSeq[controlSeqIndex].stCtrl == SEQ_CTRL_STMOTOR_ON)
	{
		printf("transferCS.c:tcs_Task():ST Motor ON \r\n");
		stMotorOn();
	}
	else if(trfCtrlSeq[controlSeqIndex].ctCtrl == SEQ_CTRL_STMOTOR_OFF)
	{
		printf("transferCS.c:tcs_Task():ST Motor OFF \r\n");
		stMotorOff();
	}
	else
	{
		stMotorOff();
	}

	//Set timer
	dgtimerStart(TCS_MOD, CONV_SEC_TO_TICKS(trfCtrlSeq[controlSeqIndex].duration*60));

	//Check if this is the last control in the sequence
	if(trfCtrlSeq[controlSeqIndex].ctrlSeqRecType ==SEQ_CTRL_END)
	{
		controlSeqIndex = 0;   //Reset the Index to start of control sequence
		return DG_ACTION_COMPLETE;
	}
	else
	{
		controlSeqIndex++;
	}
	return DG_INPROGRESS;
}


void executeTRFRSafeState()
{
	stMotorOff();
	augerMotorStop();
}


void tcs_Task(void* arg)
{

	QueueHandle_t tcsQHandle;
	dgMsg_t rcvMsg;				// Holds the currently received message
	uint8_t tcsState;


	//Get the Qhandle for this task and store locally
	tcsQHandle = getQHandle(TCS_MOD);
	tcsState = TCS_STATE_IDLE;


	while (1)
	{
		//Receive event from Queue. Block until event is available
		if(xQueueReceive(tcsQHandle, &rcvMsg, portMAX_DELAY ) != pdPASS )
		{
			// Queue did not return an event. Hence go back
			continue;
		}
		printf("transferCS.c:tcs_Task():Received command:%d in state %d \r\n",rcvMsg.command,tcsState);
		switch(rcvMsg.command)
		{
		case TCS_START:
			switch(tcsState)
			{
			case TCS_STATE_IDLE:
				//change state to transfer
				tcsState = TCS_STATE_TRANSFERRING;

				*rcvMsg.result = DG_SUCCESS;
				if(rcvMsg.taskHandleSM != NULL)
				{
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}

				//Start the control seq execution with start
				executeTRFRSeqControl(SEQ_ENGINE_START);
				break;
			case TCS_STATE_TRANSFERRING:
				//Already in transfer state. return success
				*rcvMsg.result = DG_SUCCESS;
				if(rcvMsg.taskHandleSM != NULL)
				{
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				break;
			case TCS_ERROR:
				//In error state. Cannot accept transfer command. return failure
				*rcvMsg.result = DG_FAIL;
				if(rcvMsg.taskHandleSM != NULL)
				{
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				break;
			default:
				break;
			}
			break;

		case TCS_ABORT:
			switch(tcsState)
			{
			case TCS_STATE_IDLE:
				//Nothing to be done. Already in IDLE
				*rcvMsg.result = DG_SUCCESS;
				if(rcvMsg.taskHandleSM != NULL)
				{
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				break;
			case TCS_STATE_TRANSFERRING:
				//Stop Timer so
				dgtimerStop(TCS_MOD);
				//Bring relevant actuators to safe condition
				executeTRFRSafeState();
				//Change state to IDLE
				tcsState = TCS_STATE_IDLE;

				*rcvMsg.result = DG_SUCCESS;
				if(rcvMsg.taskHandleSM != NULL)
				{
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				break;
			case TCS_ERROR:
				//In error state. Cannot accept transfer command. return failure
				*rcvMsg.result = DG_FAIL;
				if(rcvMsg.taskHandleSM != NULL)
				{
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				break;
			default:
				break;
			}
			break;

		case DG_TIMER_EXPIRY:
			switch(tcsState)
			{
			case TCS_STATE_IDLE:
				//ignore. We don't expect timer event idle
				break;

			case TCS_STATE_TRANSFERRING:
				//one control seq execution is over. Continue
				if(executeTRFRSeqControl(SEQ_ENGINE_CONTINUE) == DG_ACTION_COMPLETE)
				{
					//Control sequence execution is complete.
					//Inform CSM and terminate
					//csmNotifyTransferComplete(TCS_MOD);
					tcsState = TCS_STATE_IDLE;
					dgtimerStop(TCS_MOD);
				}
				//Continue
				break;

			case TCS_ERROR:
				//In error state. we don't expect timer event. Ignore
				break;

			default:
				break;
			}
			break;

		default:
			break;
		}

	}

}


/******************************* initTCS() ******************************************************
*Description: This function initialises transfer control system moduke. Create queue, task and  *
* initialized the datastructures. This should be called at the time of power up initialization.	*				*
* It does the following																			*
* 																								*
*	- Creates a Queue handle for receiving event messages from other modules					*
* Command Parameters: None																		*
* Return Values:																				*
*	- DG_SUCCESS: When initialization is successful												*
*	- SF_FAIL: When Queue allocation failed														*
*************************************************************************************************/

int initTCS(void)
{
	//Create csm task task
	TaskHandle_t tcsTaskHandle;
	TimerHandle_t tcsTimerHandle;


    if (xTaskCreate(tcs_Task, "tcs_Task", configMINIMAL_STACK_SIZE + 100, NULL, task_PRIORITY, &tcsTaskHandle) !=
        pdPASS)
    {
        PRINTF("TCS Task creation failed!.\r\n");
        return DG_FAIL;
    }
    //TCS requires timer and hence create FreeRTOS SW timer
    tcsTimerHandle = xTimerCreate("tcsTimer",TCS_TIMER_DEFAULT, pdFALSE, (void*)TCS_MOD, dgTimerCallback);
    if(tcsTimerHandle == NULL)
    {
        PRINTF("Timer creation failed!.\r\n");
    	vTaskDelete(tcsTaskHandle);
        return DG_FAIL;
    }
    if(registerModule(TCS_MOD, tcsTaskHandle, tcsTimerHandle)!= DG_SUCCESS)
    {
    	//Registering the module failed. Hence kill the task and return error
        PRINTF("TCS Task registration failed!.\r\n");
        xTimerDelete(tcsTimerHandle,100 / portTICK_PERIOD_MS);
    	vTaskDelete(tcsTaskHandle);

    	return DG_FAIL;
    }
    return DG_SUCCESS;
}
