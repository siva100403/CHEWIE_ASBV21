/*
 * compostingModAPI.c
 *
 *  Created on: 24-Dec-2024
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
#include "fsl_flexspi.h"

/* Chewie Includes */
#include "dgCommon.h"
#include "version.h"
#include "modulecom.h"
#include "psramDriver.h"
#include "dgtimer.h"
#include "GPIOSignals.h"
#include "motorControl.h"
#include "seqControlCommon.h"
#include "dgI2cDriver.h"
#include "eeConfig.h"
#include "sht40Driver.h"
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
#include "sensorModAPI.h"
#include "sht40Driver.h"
#include "augerAPI.h"
#include "shredder.h"
#include "shredderAPI.h"
#include "adcs.h"
#include "limitSwitchMod.h"
#include "transferCS.h"
#include "transferCSAPI.h"
#include "lidModule.h"
#include "lidModuleAPI.h"
#include "hatcsMod.h"
#include "hatcsModAPI.h"
#include "HMICmdProc.h"
#include "HMICmdProcAPI.h"
#include "csmMod.h"
#include "csmModAPI.h"



/**************************csmStart API**********************************
 * This method starts the compost State Machine module. This is usually	*
 * done after power failure recovery. Power fail recovery module will	*
 * identify the composting phase Chewie was in before power failure and	*
 * set that phase, remaining duration and wasteCat 						*									*
 * 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	  	 	*
 *  * Input Parameters:
 * 		srcModule - Module id of the calling module
 *		compostingPhase - before power failure
 *		WasteCat		- before power failure
 *		remainingDur	- before failure + power failure duration
 * Return Values:
 * 		DG_SUCCESS - For success
 * 		DG_FAIL    - Failed operation
 ********************************************************************/

int csmStart(uint8_t srcModule, uint8_t state, uint8_t phase, uint16_t remainingDuration, uint8_t wasteCat)
{
	dgMsg_t sendMsgBuf;
	dgCsmParam_t param;
	uint8_t result;

	//Validate parameters

	//Populate the parameter structure
	param.phase = phase;
	param.wasteCat = wasteCat;
	param.remDur =remainingDuration;
	param.state = state;

	result = DG_FAIL;

	//Populate the message to send to the modbus task
	sendMsgBuf.src_module = srcModule;
	sendMsgBuf.command = CSM_START;
	sendMsgBuf.cmdParam = (void*)&param;
	sendMsgBuf.dest_module = CSM_MOD;
	sendMsgBuf.result = &result;
	sendMsgBuf.taskHandleSM = xTaskGetCurrentTaskHandle();
	if(sendMsgBuf.taskHandleSM == NULL)
	{
		PRINTF("csmAPI.c:csmStart():Task handle is null for module with id: %d \r\n", srcModule);
		return DG_FAIL;
	}

	if(sendMsg(&sendMsgBuf) != DG_SUCCESS)
	{
		PRINTF("csmAPI.c:csmStart():Message send failed\r\n" );
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

/**************************csmStop API**********************************
 * This method stops the compost State Machine module. This is usually	*
 * done before switching to other operating modes.						*
 * It save the current status of composting, bring the actuators and	*
 * sensors to a safe state and then move to IDLE state.					*
 * 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	  	 	*
 *  * Input Parameters:
 * 		srcModule - Module id of the calling module
 * Return Values:
 * 		DG_SUCCESS - For success
 * 		DG_FAIL    - Failed operation
 ********************************************************************/


int csmStop(uint8_t srcModule)
{
	dgMsg_t sendMsgBuf;
	uint8_t result;

	//Validate parameters

	result = DG_FAIL;

	//Populate the message to send to the modbus task
	sendMsgBuf.src_module = srcModule;
	sendMsgBuf.command = CSM_STOP;
	sendMsgBuf.cmdParam = NULL;
	sendMsgBuf.dest_module = CSM_MOD;
	sendMsgBuf.result = &result;
	sendMsgBuf.taskHandleSM = xTaskGetCurrentTaskHandle();
	if(sendMsgBuf.taskHandleSM == NULL)
	{
		PRINTF("csmAPI.c:csmStop():Task handle is null for module with id: %d \r\n", srcModule);
		return DG_FAIL;
	}

	if(sendMsg(&sendMsgBuf) != DG_SUCCESS)
	{
		PRINTF("csmAPI.c:csmStop():Message send failed\r\n" );
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

/**************************set composting Phase API******************************
 * This method sets the Composting phase. This is usually done after power
 * failure recovery. Power fail recovery module will identify the composting
 * phase Chewie was in before power failure and set that phase. 				*
 * 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 *
 *  * Input Parameters:
 * 		srcModule - Module id of the calling module
 *		compostingPhase
 * Return Values:
 * 		DG_SUCCESS - For success
 * 		DG_FAIL    - Failed operation
 ********************************************************************/

/*
int csmSetCompostingPhase(uint8_t srcModule, uint8_t phase, uint16_t remainingDuration)
{
	dgMsg_t sendMsgBuf;
	dgCsmParam_t param;
	uint8_t result;

	//Validate parameters

	//Populate the parameter structure
	param.phase = phase;
	param.remainingDuration =remainingDuration;

	result = DG_FAIL;

	//Populate the message to send to the modbus task
	sendMsgBuf.src_module = srcModule;
	sendMsgBuf.command = CSM_SET_PHASE;
	sendMsgBuf.cmdParam = (void*)&param;
	sendMsgBuf.dest_module = CSM_MOD;
	sendMsgBuf.result = &result;
	sendMsgBuf.taskHandleSM = getTaskHandle(srcModule);
	if(sendMsgBuf.taskHandleSM == NULL)
	{
		PRINTF("csmAPI.c:csmSetCompostingPhase():Task handle is null for module with id: %d \r\n", srcModule);
		return DG_FAIL;
	}

	if(sendMsg(&sendMsgBuf) != DG_SUCCESS)
	{
		PRINTF("csmAPI.c:csmSetCompostingPhase():Message send failed\r\n" );
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
}*/

/******************************csmGetState()API**********************************
 * This method get the current state of Composting state machine. 				*
 * 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	*
 *  * Input Parameters:
 * 		srcModule - Module id of the calling module
 *		uint8_t *state - pointer to return the state
 * Return Values:
 * 		DG_SUCCESS - For success
 * 		DG_FAIL    - Failed operation
 ********************************************************************/

int csmGetState(uint8_t srcModule, dgCsmParam_t *state)
{
	dgMsg_t sendMsgBuf;
//	dgCsmParam_t param;
	uint8_t result;

	//Validate parameters
	if(state == NULL) return DG_INVALID_PARAM;

	result = DG_FAIL;

	//Populate the message to send to the modbus task
	sendMsgBuf.src_module = srcModule;
	sendMsgBuf.command = CSM_GET_STATE;
	sendMsgBuf.cmdParam = (void*)state;
	sendMsgBuf.dest_module = CSM_MOD;
	sendMsgBuf.result = &result;
	sendMsgBuf.taskHandleSM = xTaskGetCurrentTaskHandle();
	if(sendMsgBuf.taskHandleSM == NULL)
	{
		PRINTF("csmAPI.c:csmGetState():Task handle is null for module with id: %d \r\n", srcModule);
		return DG_FAIL;
	}

	if(sendMsg(&sendMsgBuf) != DG_SUCCESS)
	{
		PRINTF("csmAPI.c:csmGetState():Message send failed\r\n" );
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
int csmNotifyWasteAddStart(uint8_t srcModule)
{
	dgMsg_t sendMsgBuf;
	uint8_t result;

	result = DG_FAIL;

	//Populate the message to send to the modbus task
	sendMsgBuf.src_module = srcModule;
	sendMsgBuf.command = CSM_WASTE_ADD_START;
	sendMsgBuf.cmdParam = NULL;
	sendMsgBuf.dest_module = CSM_MOD;
	sendMsgBuf.result = &result;
	sendMsgBuf.taskHandleSM = xTaskGetCurrentTaskHandle();
	if(sendMsgBuf.taskHandleSM == NULL)
	{
		PRINTF("csmAPI.c:csmNotifyWasteAddStart():Task handle is null for module with id: %d \r\n", srcModule);
		return DG_FAIL;
	}

	if(sendMsg(&sendMsgBuf) != DG_SUCCESS)
	{
		PRINTF("csmAPI.c:csmNotifyWasteAddStart():Message send failed\r\n" );
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
int csmNotifyWasteAddEnd(uint8_t srcModule, uint8_t wasteCat)
{
	dgMsg_t sendMsgBuf;
	dgCsmParam_t param;
	uint8_t result;

	//Validate parameters
	if(wasteCat >=WASTE_CAT_MAX) return DG_INVALID_PARAM;

	param.wasteCat = wasteCat;
	result = DG_FAIL;

	//Populate the message to send to the modbus task
	sendMsgBuf.src_module = srcModule;
	sendMsgBuf.command = CSM_WASTE_ADD_END;
	sendMsgBuf.cmdParam = (void*)&param;
	sendMsgBuf.dest_module = CSM_MOD;
	sendMsgBuf.result = &result;
	sendMsgBuf.taskHandleSM = xTaskGetCurrentTaskHandle();
	if(sendMsgBuf.taskHandleSM == NULL)
	{
		PRINTF("csmAPI.c:csmNotifyWasteAddEnd():Task handle is null for module with id: %d \r\n", srcModule);
		return DG_FAIL;
	}

	if(sendMsg(&sendMsgBuf) != DG_SUCCESS)
	{
		PRINTF("csmAPI.c:csmNotifyWasteAddEnd():Message send failed\r\n" );
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

int csmNotifyTransferComplete(uint8_t srcModule)
{
	dgMsg_t sendMsgBuf;
	uint8_t result;

	result = DG_FAIL;

	//Populate the message to send to the modbus task
	sendMsgBuf.src_module = srcModule;
	sendMsgBuf.command = CSM_NOTIFY_TRANSFER_END;
	sendMsgBuf.cmdParam = NULL;
	sendMsgBuf.dest_module = CSM_MOD;
	sendMsgBuf.result = &result;
	sendMsgBuf.taskHandleSM = xTaskGetCurrentTaskHandle();
	if(sendMsgBuf.taskHandleSM == NULL)
	{
		PRINTF("csmAPI.c:csmNotifyTransferComplete():Task handle is null for module with id: %d \r\n", srcModule);
		return DG_FAIL;
	}

	if(sendMsg(&sendMsgBuf) != DG_SUCCESS)
	{
		PRINTF("csmAPI.c:csmNotifyTransferComplete():Message send failed\r\n" );
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
