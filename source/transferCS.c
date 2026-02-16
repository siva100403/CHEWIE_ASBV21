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
#include "motorControl.h"
#include "sensorMod.h"
#include "augerAPI.h"
#include "shredder.h"
#include "shredderAPI.h"
#include "adcs.h"
#include "limitSwitchMod.h"
#include "transferCS.h"


/***************************Regen Soil Transfer Logic***************************
 * Whenever transfer command is received (with the duration as parameter) following steps
 * will be executed. If abort is called, it will stop augur motor immediatly and close
 * ST valve. It is expected that CSM is in transfer state or in IDLE state.
 * Step1: Open the ST valve (CCWR rotation for STV_CLOSE_DURATION)
 * Step2: Rotate Auger, CCWR DIR, for specified duration
 * Step3: Stop Auger motor and Close ST Valve (CWR rotation for STV_CLOSE_DURATION
 *        or Limit switch lever pressed
 */

/********************** Resources Used ****************************************/

//AUGER Motor
//ST Motor:


/*************************** Global variables *******************/

extern dgConfigMem_t allConfig;

/***********************Configuration Parameters******************************/

/*

const dgTransferCtrlSeq_t trfCtrlSeq[] = { {1200, SEQ_CTRL_CTMOTOR_CCWR, SEQ_CTRL_STMOTOR_CWR, SEQ_CTRL_START },
		                                   {600, SEQ_CTRL_CTMOTOR_OFF,  SEQ_CTRL_STMOTOR_OFF,SEQ_CTRL_MID },
										   {1200, SEQ_CTRL_CTMOTOR_CCWR, SEQ_CTRL_STMOTOR_CWR, SEQ_CTRL_MID },
										   {600, SEQ_CTRL_CTMOTOR_OFF,  SEQ_CTRL_STMOTOR_OFF,SEQ_CTRL_MID },
										   {1200, SEQ_CTRL_CTMOTOR_CCWR, SEQ_CTRL_STMOTOR_CWR, SEQ_CTRL_MID },
										   {600, SEQ_CTRL_CTMOTOR_OFF,  SEQ_CTRL_STMOTOR_OFF,SEQ_CTRL_END },
};
*/

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*


int executeTRFRSeqControl(uint8_t condition)
{
	static uint8_t controlSeqIndex=0;

	if(condition == SEQ_ENGINE_START)
	{
		controlSeqIndex=0;
	}

	if(trfCtrlSeq[controlSeqIndex].ctCtrl == SEQ_CTRL_CTMOTOR_CCWR)
	{
		augerMotorCCWR();
	}
	if(trfCtrlSeq[controlSeqIndex].ctCtrl == SEQ_CTRL_CTMOTOR_CWR)
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

	if(trfCtrlSeq[controlSeqIndex].stCtrl == SEQ_CTRL_STMOTOR_CWR)
	{
		printf("transferCS.c:tcs_Task():ST Motor CWR \r\n");
		stMotorCWR();
	}
	if(trfCtrlSeq[controlSeqIndex].stCtrl == SEQ_CTRL_STMOTOR_CCWR)
	{
		printf("transferCS.c:tcs_Task():ST Motor CCWR \r\n");
		stMotorCCWR();
	}
	else if(trfCtrlSeq[controlSeqIndex].ctCtrl == SEQ_CTRL_STMOTOR_OFF)
	{
		printf("transferCS.c:tcs_Task():ST Motor OFF \r\n");
		stMotorStop();;
	}
	else
	{
		stMotorStop();;
	}

	//Set timer
	dgtimerStart(TCS_MOD, (trfCtrlSeq[controlSeqIndex].duration*100)/portTICK_PERIOD_MS);

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

*/

/*

int flapTimerStart(TickType_t timeoutValue)
{

	if(xTimerChangePeriod(flapTimerHandle,timeoutValue, DGTIMER_BLOCKTIME )== pdFALSE)
	{
		PRINTF("shredder.c:flapTimerStart():Change period failed\r\n");
		return DG_FAIL;
	}
	if(xTimerStart(flapTimerHandle,DGTIMER_BLOCKTIME) == pdFALSE)
	{
		PRINTF("shredder.c:flapTimerStart():Start failed\r\n");
		return DG_FAIL;
	}
	return DG_SUCCESS;
}


void flapTimerCallback( TimerHandle_t xTimer)
{
	//Turn of flap motor
	flapMotorStop();
}
*/


void executeTRFRSafeState()
{
	stMotorStop();
	augerMotorStop();
}



void tcs_Task(void* arg)
{

	QueueHandle_t tcsQHandle;
	dgMsg_t rcvMsg;				// Holds the currently received message
	uint8_t tcsState;
	uint16_t transferDuration;  	// in Secs
	uint16_t stvOpenDuration, stvCloseDuration;   //in mSec


	//Init variables
	tcsState = TCS_STATE_IDLE;
	transferDuration = TRANSFER_DUR_DEFAULT;

	//Wait till module registration is complete
	tcsQHandle = NULL;
	while(tcsQHandle== NULL)
	{
		vTaskDelay(100 / portTICK_PERIOD_MS);
		tcsQHandle = getQHandle(TCS_MOD);
	}

	//Initialize Transfer Control system configuration parameters
	//Initialize Timeout variables
	stvOpenDuration = allConfig.transferConfig.transferParams.stvOpenDur;
	stvCloseDuration = allConfig.transferConfig.transferParams.stvCloseDur;
	transferDuration = allConfig.transferConfig.transferParams.transferDur;

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
				//Get the transfer duration from rcvMsg
				//transferDuration = *(uint16_t*)rcvMsg.cmdParam;
				printf("transferCS.c:tcs_Task():transfer duration:%dr\n",transferDuration);

				//change state to transfer
				tcsState = TCS_STATE_OPENING;

				*rcvMsg.result = DG_SUCCESS;
				if(rcvMsg.taskHandleSM != NULL)
				{
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}

				//Open the ST Valve
				dgtimerStart(TCS_MOD, (stvOpenDuration)/portTICK_PERIOD_MS);
				stMotorCCWR();
				break;
			case TCS_STATE_OPENING:
			case TCS_STATE_CLOSING:
			case TCS_STATE_TRANSFERRING:
				//Already in transfer state. return success
				*rcvMsg.result = DG_SUCCESS;
				if(rcvMsg.taskHandleSM != NULL)
				{
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				break;
			case TCS_STATE_ERROR:
				//In error state. Cannot accept transfer command. return failure
				*rcvMsg.result = DG_FAIL;
				if(rcvMsg.taskHandleSM != NULL)
				{
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				break;
			default:
				//Unknown stated. Cannot accept transfer command. return failure
				*rcvMsg.result = DG_FAIL;
				if(rcvMsg.taskHandleSM != NULL)
				{
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
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
			case TCS_STATE_OPENING:
			case TCS_STATE_CLOSING:
			case TCS_STATE_ERROR:
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

		case TCS_STV_SYNC:
			switch(tcsState)
			{
			case TCS_STATE_IDLE:
				//Check - ST Valve in closed condition
				*rcvMsg.result = DG_SUCCESS;
				if(rcvMsg.taskHandleSM != NULL)
				{
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				if(getStorageTraySwicthStatus() == STTV_POSITION_CLOSED)
				{
					//STTV is in closed condition. No need to do anything
				}
				else
				{
					printf("transferCS.c:tcsTask():STTV is not in closed condition. Closing");
					//Change state to SHD_STATE_FLAPSYNC
					tcsState = TCS_STV_SYNC;

					//Initiate closing stflap
					stMotorCWR();
					dgtimerStart(TCS_MOD, (stvOpenDuration+2)/portTICK_PERIOD_MS);
				}
				break;
			case TCS_STATE_TRANSFERRING:
			case TCS_STATE_OPENING:
			case TCS_STATE_CLOSING:
			case TCS_STATE_ERROR:
				//Sync is allowed only in IDLE state.Return failure
				*rcvMsg.result = DG_FAIL;
				if(rcvMsg.taskHandleSM != NULL)
				{
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				break;
			default:
				//Sync is allowed only in IDLE state.Return failure
				*rcvMsg.result = DG_FAIL;
				if(rcvMsg.taskHandleSM != NULL)
				{
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				break;
			}
			break;


		case DG_TIMER_EXPIRY:
			switch(tcsState)
			{
			case TCS_STATE_IDLE:
				//ignore. We don't expect timer event idle
				break;
			case TCS_STATE_OPENING:
				//Open duration is over. Stop motor and timer
				stMotorStop();
				dgtimerStop(TCS_MOD);
				//Change state to transfer and start Augur motor
				tcsState = TCS_STATE_TRANSFERRING;
				dgtimerStart(TCS_MOD, (transferDuration*1000)/portTICK_PERIOD_MS);
				augerMotorCCWR();
				break;

			case TCS_STATE_TRANSFERRING:
				//Transfer duration is over. Stop augur motor
				augerMotorStop();
				//Change to CLOSING state
				tcsState = TCS_STATE_CLOSING;
				dgtimerStart(TCS_MOD, (stvCloseDuration)/portTICK_PERIOD_MS);
				stMotorCWR();
				break;

			case TCS_STATE_CLOSING:
				//Close duration is over. Stop motor and timer
				stMotorStop();
				dgtimerStop(TCS_MOD);
				//Change state to transfer and start Augur motor
				tcsState = TCS_STATE_IDLE;
				break;

			case TCS_STV_SYNC:
				stMotorStop();
				tcsState = TCS_STATE_IDLE;
				break;

			case TCS_STATE_ERROR:
				//In error state. we don't expect timer event. Ignore
				break;

			default:
				break;
			}
			break;
		case DG_LS_STVALVECLOSE:
			stMotorStop();
			break;

		default:
			break;
		}

	}

}


/******************************* initTCS() ******************************************************
*Description: This function initialises transfer control system module. Create queue, task and  *
* initialized the data structures. This should be called at the time of power up initialization.	*				*
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
	TimerHandle_t stflapTimerHandle;

    if (xTaskCreate(tcs_Task, "tcs_Task", configMINIMAL_STACK_SIZE + 100, NULL, task_PRIORITY, &tcsTaskHandle) !=
        pdPASS)
    {
        PRINTF("TCS Task creation failed!.\r\n");
        return DG_FAIL;
    }
    //TCS requires timer and hence create FreeRTOS SW timer
    tcsTimerHandle = xTimerCreate("tcsTimer",TRANSFER_DUR_DEFAULT, pdFALSE, (void*)TCS_MOD, dgTimerCallback);
    if(tcsTimerHandle == NULL)
    {
        PRINTF("Timer creation failed!.\r\n");
    	vTaskDelete(tcsTaskHandle);
        return DG_FAIL;
    }
/*	stflapTimerHandle = xTimerCreate("stflapTimer",100, pdFALSE, NULL, stflapTimerCallback);
	if(stflapTimerHandle == NULL)
	{
		printf("stFlap Timer creation failed!.\r\n");
	}*/
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

