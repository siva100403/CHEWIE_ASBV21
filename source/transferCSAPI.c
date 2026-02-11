/*
 * transferCSAPI.c
 *
 *  Created on: 31-Dec-2024
 *      Author: Jawahar Arumugam
 */



#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

/* Freescale includes. */
#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_clock.h"

/* Other Includes */
#include <dgCommon.h>
#include <modulecom.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "transferCS.h"




/******************************transferStart()***********************************
 * This API initiates transfer of content from compost chamber to Storage 		*
 * Chamber. Completion of transfer will be indicated by calling csmAPI			*                													*
 *																				*
 * Input Parameters:															*
 * 		srcModule - Module id of the calling module
 * Return Values:
 * 		DG_SUCCESS - For success
 * 		DG_FAIL    - Failed operation
 ********************************************************************/


int transferStart(uint8_t srcModule, uint16_t trfrDuration)
{
	dgMsg_t sendMsgBuf;
	uint8_t result;
	uint16_t duration;

	//Validate input param
	if((trfrDuration==0) || (trfrDuration > TRANSFER_DUR_MAX))
		return DG_INVALID_PARAM;

	result = DG_FAIL;
	duration = trfrDuration;

	//Populate the message to send to the modbus task
	sendMsgBuf.src_module = srcModule;
	sendMsgBuf.command = TCS_START;
	sendMsgBuf.cmdParam = (void*)&duration;
	sendMsgBuf.dest_module = TCS_MOD;
	sendMsgBuf.result = &result;
	sendMsgBuf.taskHandleSM = xTaskGetCurrentTaskHandle();
	if(sendMsgBuf.taskHandleSM == NULL)
	{
		PRINTF("transferCSAPI.c:transferStart():Task handle is null for module with id: %d \r\n", srcModule);
		return DG_FAIL;
	}

	if(sendMsg(&sendMsgBuf) != DG_SUCCESS)
	{
		PRINTF("transferCSAPI.c:transferStart():Message send failed\r\n" );
		return DG_FAIL;
	}

	//Message send success. Now we will wait for response

	xTaskNotifyWait(0,0,NULL, portMAX_DELAY);

	//Response received. Check the results
	if(result == DG_SUCCESS)
	{
		return DG_SUCCESS;
	}
	return DG_FAIL;
}


/**************************transferAbort() API***************************
 * This method aborts the ongoing transfer operation.					*
 * Ignored if there is no transfer operation in progress				*
 *
 *  * Input Parameters:
 * 		srcModule - Module id of the calling module
 * Return Values:
 * 		DG_SUCCESS - For success
 * 		DG_FAIL    - Failed operation
 ********************************************************************/


int transferAbort(uint8_t srcModule)
{
	dgMsg_t sendMsgBuf;
	uint8_t result;

	//Validate parameters

	result = DG_FAIL;

	//Populate the message to send to the modbus task
	sendMsgBuf.src_module = srcModule;
	sendMsgBuf.command = TCS_ABORT;
	sendMsgBuf.cmdParam = NULL;
	sendMsgBuf.dest_module = TCS_MOD;
	sendMsgBuf.result = &result;
	sendMsgBuf.taskHandleSM = xTaskGetCurrentTaskHandle();
	if(sendMsgBuf.taskHandleSM == NULL)
	{
		PRINTF("transferCSAPI.c:transferAbort():Task handle is null for module with id: %d \r\n", srcModule);
		return DG_FAIL;
	}

	if(sendMsg(&sendMsgBuf) != DG_SUCCESS)
	{
		PRINTF("transferCSAPI.c:transferAbort():Message send failed\r\n" );
		return DG_FAIL;
	}

	//Message send success. Now we will wait for response

	xTaskNotifyWait(0,0,NULL, portMAX_DELAY);

	//Response received. Check the results
	if(result == DG_SUCCESS)
	{
		return DG_SUCCESS;
	}
	return DG_FAIL;
}



int event_ls_stvalveClose(uint8_t srcModule)
{
	dgMsg_t sendMsgBuf;
	uint8_t result;

	//Validate parameters

	result = DG_FAIL;

	//Populate the message to send to the modbus task
	sendMsgBuf.src_module = srcModule;
	sendMsgBuf.command = DG_LS_STVALVECLOSE;
	sendMsgBuf.cmdParam = NULL;
	sendMsgBuf.dest_module = TCS_MOD;
	sendMsgBuf.result = &result;
	sendMsgBuf.taskHandleSM = xTaskGetCurrentTaskHandle();
	if(sendMsgBuf.taskHandleSM == NULL)
	{
		PRINTF("transferCSAPI.c:event_ls_stvalveClose():Task handle is null for module with id: %d \r\n", srcModule);
		return DG_FAIL;
	}

	if(sendMsg(&sendMsgBuf) != DG_SUCCESS)
	{
		PRINTF("transferCSAPI.c:event_ls_stvalveClose():Message send failed\r\n" );
		return DG_FAIL;
	}

	//Message send success. Now we will wait for response

	//xTaskNotifyWait(0,0,NULL, portMAX_DELAY);

	return DG_SUCCESS;

}


int tcsSTValveSync(uint8_t srcModule)
{
	dgMsg_t sendMsgBuf;
	uint8_t result;

	result = DG_FAIL;

	//Populate the message to send to the modbus task
	sendMsgBuf.src_module = srcModule;
	sendMsgBuf.command = TCS_STV_SYNC;
	sendMsgBuf.cmdParam = NULL;
	sendMsgBuf.dest_module = TCS_MOD;
	sendMsgBuf.result = &result;
	sendMsgBuf.taskHandleSM = xTaskGetCurrentTaskHandle();
	if(sendMsgBuf.taskHandleSM == NULL)
	{
		PRINTF("transferCSAPI.c:tcsSTValveSync():Task handle is null for module with id: %d \r\n", srcModule);
		return DG_FAIL;
	}

	if(sendMsg(&sendMsgBuf) != DG_SUCCESS)
	{
		PRINTF("transferCSAPI.c:tcsSTValveSync():Message send failed\r\n" );
		return DG_FAIL;
	}

	//Message send success. Now we will wait for response

	xTaskNotifyWait(0,0,NULL, portMAX_DELAY);

	//Response received. Check the results
	if(result == DG_SUCCESS)
	{
		return DG_SUCCESS;
	}
	return DG_FAIL;
}


int checkTransferFeasibility(void)
{

	//This has to check whether the storage tray is open or full, then return appropriate value
	/*TODO*/
	return DG_SUCCESS;
}
