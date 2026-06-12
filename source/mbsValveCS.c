/*
 * mbsValveCS.c
 *
 *  Created on: 12-Jun-2026
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
#include "fsl_debug_console.h"
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
#include "mbsValveCS.h"

/*******************************************************************************
 * MACRO Definitions
 ******************************************************************************/

#define MBSV_OFF_DURATION_DEFAULT  100

/******************************* Auger states *********************************/
#define MBSV_STATE_IDLE			0     	//Default state after creating the task
#define MBSV_STATE_CYCLE1		1		//MBS Valve going through cycle1
#define MBSV_STATE_CYCLE2		2		//MBS Valve going through cycle1
#define MBSV_STATE_CLOSE		3		//MBS Valve Closing

#define MBSV_EXE_START	0
#define MBSV_EXE_TIMER	1

/*******************************************************************************
 * Global Variables
 ******************************************************************************/
extern mbsVCycleParams_t mbsVCSCfgP1[];
extern bool stopFlapFlag;
/*******************************************************************************
 * Implementation
 ******************************************************************************/

static void exeMbsVCycle(uint8_t exeState, mbsVCycleParams_t *cfg)
{
//MBS Valve Cycle states
#define CYCLE_VALVE_MOVE	0
#define CYCLE_VALVE_REST	1

	static uint8_t cycleCount;
	static uint8_t cycleState;


	switch(exeState)
	{
	case MBSV_EXE_START:
		cycleCount = 0;
		cycleState = CYCLE_VALVE_MOVE;
		switch(cfg[cycleCount].direction)
		{
		case MBSV_CWR:
			flapMotorCWR();
			dgtimerStart(MBSV_MOD, (cfg[cycleCount].moveDur)/portTICK_PERIOD_MS);
			cycleState = CYCLE_VALVE_MOVE;
			break;
		case MBSV_CCWR:
			flapMotorCCWR();
			dgtimerStart(MBSV_MOD, (cfg[cycleCount].moveDur)/portTICK_PERIOD_MS);
			cycleState = CYCLE_VALVE_MOVE;
			break;
		case MBSV_NOP:
			//Nothing to be done
			break;
		}
		break;
	case MBSV_EXE_TIMER:
		if (cycleState == CYCLE_VALVE_MOVE)
		{
			flapMotorStop();
			dgtimerStart(MBSV_MOD, (cfg[cycleCount].restDur)/portTICK_PERIOD_MS);
			cycleState = CYCLE_VALVE_REST;
		}
		else
		{
			cycleCount++;
			if(cycleCount>=MAX_MBSV_CYCLES)
			{
				//It should not reach here
				//Ignore
				printf("mbsValveCS.c:exeMbsVCycle():Cycle count reach 16\r\n");
				break;
			}
			switch(cfg[cycleCount].direction)
			{
			case MBSV_CWR:
				flapMotorCWR();
				dgtimerStart(MBSV_MOD, (cfg[cycleCount].moveDur)/portTICK_PERIOD_MS);
				cycleState = CYCLE_VALVE_MOVE;
				break;
			case MBSV_CCWR:
				flapMotorCCWR();
				dgtimerStart(MBSV_MOD, (cfg[cycleCount].moveDur)/portTICK_PERIOD_MS);
				cycleState = CYCLE_VALVE_MOVE;
				break;
			case MBSV_NOP:
				//Nothing to be done
				break;
			}

		}

		break;
	}
}


static void mbsV_task(void *pvParameters)
{
	QueueHandle_t mbsVQHandle;
	dgMsg_t rcvMsg;				//Holds the currently received message
	static int mbsState;			//


	//Get the Qhandle for this task and store locally
	mbsVQHandle = getQHandle(MBSV_MOD);

	//Initialize the state of MBSV state to IDLE
	mbsState = MBSV_STATE_IDLE;


	while(1)
	{
		//Receive event from Queue. Block until event is available
		if(xQueueReceive(mbsVQHandle, &rcvMsg, portMAX_DELAY ) != pdPASS )
		{
			// Queue did not return an event. Hence go back
			continue;
		}
		printf("mbsValveCS.c:mbsV_task():cmd rcvd %d in state %d\r\n",rcvMsg.command,mbsState);
		switch(mbsState)
		{
		case MBSV_STATE_IDLE:
			switch(rcvMsg.command)
			{
			case MBSV_CYCLE1:
				mbsState = MBSV_STATE_CYCLE1;
				exeMbsVCycle(MBSV_EXE_START,mbsVCSCfgP1);
				break;
			case MBSV_CYCLE2:
				exeMbsVCycle(MBSV_EXE_START,mbsVCSCfgP1);
				mbsState = MBSV_STATE_CYCLE2;
				break;
			case MBSV_CLOSE:
				dgtimerStop(MBSV_MOD);
				mbsState = MBSV_STATE_IDLE;
				break;
			case DG_TIMER_EXPIRY:
				//Ignore
				break;
			}
			break;
		case MBSV_STATE_CYCLE1:
			switch(rcvMsg.command)
			{
			case MBSV_CYCLE1:
				//Ignore. Already in CYCLE1
				break;
			case MBSV_CYCLE2:
				//Ignore. Not allowed
				break;
			case MBSV_CLOSE:
				dgtimerStop(MBSV_MOD);
				mbsState = MBSV_STATE_IDLE;
				break;
			case DG_TIMER_EXPIRY:
				exeMbsVCycle(MBSV_EXE_TIMER,mbsVCSCfgP1);
				break;
			}
			break;
		case MBSV_STATE_CYCLE2:
			switch(rcvMsg.command)
			{
			case MBSV_CYCLE1:
				//Ignore. Already in CYCLE1
				break;
			case MBSV_CYCLE2:
				//Ignore. Not allowed
				break;
			case MBSV_CLOSE:
				dgtimerStop(MBSV_MOD);
				mbsState = MBSV_STATE_IDLE;
				break;
			case DG_TIMER_EXPIRY:
				exeMbsVCycle(MBSV_EXE_TIMER,mbsVCSCfgP1);
				break;
			}
			break;
		case MBSV_STATE_CLOSE:
			switch(rcvMsg.command)
			{
			case MBSV_CYCLE1:
				//Ignore. Already in CYCLE1
				break;
			case MBSV_CYCLE2:
				//Ignore. Not allowed
				break;
			case MBSV_CLOSE:
				dgtimerStop(MBSV_MOD);
				mbsState = MBSV_STATE_CLOSE;
				break;
			case DG_TIMER_EXPIRY:
				exeMbsVCycle(MBSV_EXE_TIMER,mbsVCSCfgP1);
				break;
			}

			break;
		default:
			printf("mbsValveCS.c:mbsV_task():Invalid state\r\n");
			mbsState = MBSV_STATE_IDLE;
			break;

		}

	}

}

int mbsVCycle1()
{
	dgMsg_t sendMsgBuf;
	uint8_t result;

	result = DG_FAIL;

	//Populate the message to send to the modbus task
	sendMsgBuf.src_module = UNKNOWN;
	sendMsgBuf.command = MBSV_CYCLE1;
	sendMsgBuf.cmdParam = NULL;
	sendMsgBuf.dest_module = MBSV_MOD;
	sendMsgBuf.result = &result;
	sendMsgBuf.taskHandleSM = xTaskGetCurrentTaskHandle();
	if(sendMsgBuf.taskHandleSM == NULL)
	{
		PRINTF("mbsValveCS.c:mbsVCycle1():Task handle is null\r\n");
		return DG_FAIL;
	}

	if(sendMsg(&sendMsgBuf) != DG_SUCCESS)
	{
		PRINTF("mbsValveCS.c:mbsVCycle1():Message send failed \r\n" );
		return DG_FAIL;
	}

	return DG_SUCCESS;

}


int mbsVCycle2()
{
	dgMsg_t sendMsgBuf;
	uint8_t result;

	result = DG_FAIL;

	//Populate the message to send to the modbus task
	sendMsgBuf.src_module = UNKNOWN;
	sendMsgBuf.command = MBSV_CYCLE2;
	sendMsgBuf.cmdParam = NULL;
	sendMsgBuf.dest_module = MBSV_MOD;
	sendMsgBuf.result = &result;
	sendMsgBuf.taskHandleSM = xTaskGetCurrentTaskHandle();
	if(sendMsgBuf.taskHandleSM == NULL)
	{
		PRINTF("mbsValveCS.c:mbsVCycle2():Task handle is null \r\n");
		return DG_FAIL;
	}

	if(sendMsg(&sendMsgBuf) != DG_SUCCESS)
	{
		PRINTF("mbsValveCS.c:mbsVCycle2():Message send failed \r\n" );
		return DG_FAIL;
	}

	return DG_SUCCESS;

}

int mbsVClose()
{
	dgMsg_t sendMsgBuf;
	uint8_t result;

	result = DG_FAIL;

	//Populate the message to send to the modbus task
	sendMsgBuf.src_module = UNKNOWN;
	sendMsgBuf.command = MBSV_CLOSE;
	sendMsgBuf.cmdParam = NULL;
	sendMsgBuf.dest_module = MBSV_MOD;
	sendMsgBuf.result = &result;
	sendMsgBuf.taskHandleSM = xTaskGetCurrentTaskHandle();
	if(sendMsgBuf.taskHandleSM == NULL)
	{
		PRINTF("mbsValveCS.c:mbsvClose():Task handle is null\r\n");
		return DG_FAIL;
	}

	if(sendMsg(&sendMsgBuf) != DG_SUCCESS)
	{
		PRINTF("mbsValveCS.c:mbsVClose():Message send failed \r\n" );
		return DG_FAIL;
	}

	return DG_SUCCESS;

}

int initMbsValveCS(void)
{
	//Create mbs valve cs task
	TaskHandle_t mbsVTaskHandle;
	TimerHandle_t mbsVTimerHandle;
	BaseType_t result;

	result = xTaskCreate(mbsV_task, "mbsV_task", configMINIMAL_STACK_SIZE + 100, NULL, task_PRIORITY, &mbsVTaskHandle);
    if ( result !=    pdPASS)
    {
        printf("mbsValveCS.c:initMbsValveCS():mbsV Task creation failed!.\r\n");
        return DG_FAIL;
    }
    //MBSV requires timer and hence create FreeRTOS SW timer
    mbsVTimerHandle = xTimerCreate("mbsVTimer",MBSV_OFF_DURATION_DEFAULT, pdFALSE, (void*)MBSV_MOD, dgTimerCallback);
    if(mbsVTimerHandle == NULL)
    {
        printf("mbsValveCS.c:initMbsValveCS():Timer creation failed!.\r\n");
    	vTaskDelete(mbsVTaskHandle);
        return DG_FAIL;
    }
    if(registerModule(MBSV_MOD, mbsVTaskHandle, mbsVTimerHandle)!= DG_SUCCESS)
    {
    	//Registering the module failed. Hence kill the task and return error
        printf("mbsValveCS.c:initMbsValveCS():mbsV Task registration failed!.\r\n");
        xTimerDelete(mbsVTimerHandle,100 / portTICK_PERIOD_MS);
    	vTaskDelete(mbsVTaskHandle);

    	return DG_FAIL;
    }
    return DG_SUCCESS;
}







