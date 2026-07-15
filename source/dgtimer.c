/*
 * dgtimer.c
 *
 *  Created on: 30-May-2024
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
//#include "fsl_debug_console.h"
#include "fsl_spc.h"
#include "fsl_lpi2c.h"
#include "fsl_lpuart.h"
#include "fsl_device_registers.h"


/* Chewie Includes */
#include "dgCommon.h"
#include "version.h"
#include "modulecom.h"
#include "dgtimer.h"



int dgtimerStart(uint8_t moduleid, TickType_t timeoutValue)
{
	TimerHandle_t timerHandle;
	//Validate module id
	if((moduleid == 0)||(moduleid >= LAST_MODULE))
	{
		return DG_INVALID_PARAM;
	}
	//Get the timer handle for the module id and validate
	timerHandle = getTimerHandle(moduleid);
	if(timerHandle == NULL)
	{
		printf("dgtimerStart():Timer Handle is NULL for the module %d \r\n",moduleid );
		return DG_FAIL;
	}
	if(xTimerChangePeriod( timerHandle,	timeoutValue, DGTIMER_BLOCKTIME )== pdFALSE)
	{
		printf("dgtimerStart():Timer Period set failed for the module %d \r\n",moduleid );
		return DG_FAIL;
	}
	if(xTimerStart(timerHandle,DGTIMER_BLOCKTIME) == pdFALSE)
	{
		printf("dgtimerStart():Timer start failed for the module %d \r\n",moduleid );
		return DG_FAIL;
	}
	return DG_SUCCESS;
}

int dgtimerStop(uint8_t moduleid)
{
	TimerHandle_t timerHandle;
	//Validate module id
	if((moduleid == 0)||(moduleid >= LAST_MODULE))
	{
		return DG_INVALID_PARAM;
	}
	//Get the timer handle for the module id and validate
	timerHandle = getTimerHandle(moduleid);
	if(timerHandle == NULL)
	{
		printf("dgtimerStop():Timer Handle is NULL for the module %d \r\n",moduleid );
		return DG_FAIL;
	}
	if(xTimerStop(timerHandle,DGTIMER_BLOCKTIME)==pdTRUE)
	{
		return DG_SUCCESS;
	}
	else
	{
		printf("dgtimerStop():Failed for the module %d \r\n",moduleid );
		return DG_FAIL;
	}
}

void dgTimerCallback( TimerHandle_t xTimer)
{
	int moduleidInt;
	uint8_t moduleid;
	dgMsg_t sendMsgBuf;
	uint8_t result;

	//Get the moduleid corresponding to the timer
	moduleidInt = ( int ) pvTimerGetTimerID( xTimer );
	moduleid = (uint8_t)moduleidInt;

	//validate moduleid
	if((moduleid == 0)||(moduleid >= LAST_MODULE))
	{
		return;
	}
	//Create Timer Expired message

	sendMsgBuf.src_module = TIMER_MOD;
	sendMsgBuf.command = DG_TIMER_EXPIRY;
	sendMsgBuf.cmdParam = NULL;
	sendMsgBuf.dest_module = moduleid;
	sendMsgBuf.result = &result;
	sendMsgBuf.taskHandleSM = NULL;


	if(sendMsg(&sendMsgBuf) != DG_SUCCESS)
	{
		printf("dgTimerCallback():Message send failed\r\n" );
	}
}
