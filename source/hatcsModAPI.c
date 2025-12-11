/*
 * hatcsModAPI.c
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
#include "fsl_flexspi.h"

/* Chewie Includes */
#include "dgCommon.h"
#include "version.h"
#include "modulecom.h"
#include "psramDriver.h"
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


/***********************************hatcsStart()*********************************
 * This API starts controlling the actuators necessary to maintain the temperature,
 * humidity and aeration within the range. This relies on SensorMod for sensing
 * temperature and humidity.
 *
 *****************************************************************************/


int hatcsStart(uint8_t srcModule)
{
	dgMsg_t sendMsgBuf;
	uint8_t result;

	result = DG_FAIL;

	//Populate the message to send to the modbus task
	sendMsgBuf.src_module = srcModule;
	sendMsgBuf.command = HATCS_START;
	sendMsgBuf.cmdParam = NULL;
	sendMsgBuf.dest_module = HATCS_MOD;
	sendMsgBuf.result = &result;
	sendMsgBuf.taskHandleSM = xTaskGetCurrentTaskHandle();
	if(sendMsgBuf.taskHandleSM == NULL)
	{
		PRINTF("hatcsModAPI.c:hatcsStart():Task handle is null for module with id: %d \r\n", srcModule);
		return DG_FAIL;
	}

	if(sendMsg(&sendMsgBuf) != DG_SUCCESS)
	{
		PRINTF("hatcsModAPI.c:hatcsStart()::Message send failed \r\n" );
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

/***************************hatcsStop()******************************
 * This API stops the temperature and humidity control.
 *
 **********************************************************************/


int hatcsStop(uint8_t srcModule)
{
	dgMsg_t sendMsgBuf;
	uint8_t result;

	result = DG_FAIL;

	//Populate the message to send to the modbus task
	sendMsgBuf.src_module = srcModule;
	sendMsgBuf.command = HATCS_STOP;
	sendMsgBuf.cmdParam = NULL;
	sendMsgBuf.dest_module = HATCS_MOD;
	sendMsgBuf.result = &result;
	sendMsgBuf.taskHandleSM = xTaskGetCurrentTaskHandle();
	if(sendMsgBuf.taskHandleSM == NULL)
	{
		PRINTF("hatcsModAPI.c:hatcsStop():Task handle is null for module with id: %d \r\n", srcModule);
		return DG_FAIL;
	}

	if(sendMsg(&sendMsgBuf) != DG_SUCCESS)
	{
		PRINTF("hatcsModAPI.c:hatcsStop():Message send failed \r\n" );
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


/***************************hatcsSetParam()******************************
 * This API sets the paramter for the Humidity-Aeration-Temperature control system
 *
 **********************************************************************/


/*int hatcsSetParam(uint8_t srcModule, dgHatcsParam_t *param)
{
	dgMsg_t sendMsgBuf;
	uint8_t result;

	result = DG_FAIL;

	//Populate the message to send to the modbus task
	sendMsgBuf.src_module = srcModule;
	sendMsgBuf.command = HATCS_SET_PARAM;
	sendMsgBuf.cmdParam = (void*)param;
	sendMsgBuf.dest_module = HATCS_MOD;
	sendMsgBuf.result = &result;
	sendMsgBuf.taskHandleSM = xTaskGetCurrentTaskHandle();
	if(sendMsgBuf.taskHandleSM == NULL)
	{
		PRINTF("hatcsModAPI.c:hatcsSetParam():Task handle is null for module with id: %d \r\n", srcModule);
		return DG_FAIL;
	}

	if(sendMsg(&sendMsgBuf) != DG_SUCCESS)
	{
		PRINTF("hatcsModAPI.c:hatcsSetParam():Message send failed \r\n" );
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

/********************hatcsNotifySensorState()***************************
 * This API is used by Sensor module to notify the change in sensor state
 *
 **********************************************************************/


int hatcsNotifySensorState(uint8_t srcModule, uint8_t sensorState)
{
	dgMsg_t sendMsgBuf;
	uint8_t result;

	result = DG_FAIL;

	//Populate the message to send to the modbus task
	sendMsgBuf.src_module = srcModule;
	sendMsgBuf.command = HATCS_NOTIFY_SENSOR_STATE_CHANGE;
	sendMsgBuf.cmdParam = (void*)&sensorState;
	sendMsgBuf.dest_module = HATCS_MOD;
	sendMsgBuf.result = &result;
	sendMsgBuf.taskHandleSM = xTaskGetCurrentTaskHandle();
	if(sendMsgBuf.taskHandleSM == NULL)
	{
		PRINTF("hatcsModAPI.c:hatcsNotifySensorState():Task handle is null for module with id: %d \r\n", srcModule);
		return DG_FAIL;
	}

	if(sendMsg(&sendMsgBuf) != DG_SUCCESS)
	{
		PRINTF("hatcsModAPI.c:hatcsNotifySensorState():Message send failed \r\n" );
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

