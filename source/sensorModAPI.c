/*
 * sensorModAPI.c
 *
 *  Created on: 06-Jan-2025
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



/*******************************************************************************
 * Definitions
 ******************************************************************************/








/***********************************sensorStart()*********************************
 * This API starts the temperature and humidity sensing operations. Once started it will
 * sample the temperature and humidity once in 4 secs, compare with threshold values and
 * inform the sensorStatus.
 *
 *****************************************************************************/


int sensorStart(uint8_t srcModule)
{
	dgMsg_t sendMsgBuf;
	uint8_t result;

	result = DG_FAIL;

	//Populate the message to send to the modbus task
	sendMsgBuf.src_module = srcModule;
	sendMsgBuf.command = SENSOR_START;
	sendMsgBuf.cmdParam = NULL;
	sendMsgBuf.dest_module = SENSOR_MOD;
	sendMsgBuf.result = &result;
	sendMsgBuf.taskHandleSM = xTaskGetCurrentTaskHandle();
	if(sendMsgBuf.taskHandleSM == NULL)
	{
		PRINTF("sensorModAPI.c:sensorStart():Task handle is null for module with id: %d \r\n", srcModule);
		return DG_FAIL;
	}

	if(sendMsg(&sendMsgBuf) != DG_SUCCESS)
	{
		PRINTF("sensorModAPI.c:sensorStart()::Message send failed \r\n" );
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

/***************************sensorStop()******************************
 * This API stops the temperature and humidity sensing operation.
 *
 **********************************************************************/


int sensorStop(uint8_t srcModule)
{
	dgMsg_t sendMsgBuf;
	uint8_t result;

	result = DG_FAIL;

	//Populate the message to send to the modbus task
	sendMsgBuf.src_module = srcModule;
	sendMsgBuf.command = SENSOR_STOP;
	sendMsgBuf.cmdParam = NULL;
	sendMsgBuf.dest_module = SENSOR_MOD;
	sendMsgBuf.result = &result;
	sendMsgBuf.taskHandleSM = xTaskGetCurrentTaskHandle();
	if(sendMsgBuf.taskHandleSM == NULL)
	{
		PRINTF("sensorModAPI.c:sensorStop():Task handle is null for module with id: %d \r\n", srcModule);
		return DG_FAIL;
	}

	if(sendMsg(&sendMsgBuf) != DG_SUCCESS)
	{
		PRINTF("sensorModAPI.c:sensorStop():Message send failed \r\n" );
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



/*********************sensorSetParam()*************************
 * This method can be used to configure the temperature/humidity threshold
 *  parameters
 *
 * Input Parameters:
 * 		srcModule - Module id of the calling module
 * 		config  - structure ptr having  parameters
 *
 * Return Values:
 * 		DG_SUCCESS - For success
 * 		DG_FAIL    - Failed operation
 * */


int sensorSetParam(uint8_t srcModule, float setvalueTemp, float setvalueHumidity, float tempRange, float humidityRange )
{
	dgMsg_t sendMsgBuf;
	dgSensorParam_t config;
	uint8_t result;

	config.humidityRange = humidityRange;
	config.tempRange = tempRange;
	config.humiditySetValue = setvalueHumidity;
	config.tempSetValue = setvalueTemp;

	result = DG_FAIL;

	//Populate the message to send to the modbus task
	sendMsgBuf.src_module = srcModule;
	sendMsgBuf.command = SENSOR_SET_PARAM;
	sendMsgBuf.cmdParam = (void*)&config;
	sendMsgBuf.dest_module = SENSOR_MOD;
	sendMsgBuf.result = &result;
	sendMsgBuf.taskHandleSM = xTaskGetCurrentTaskHandle();
	if(sendMsgBuf.taskHandleSM == NULL)
	{
		PRINTF("sensorModAPI.c:sensorSetParam():Task handle is null for module with id: %d \r\n", srcModule);
		return DG_FAIL;
	}

	if(sendMsg(&sendMsgBuf) != DG_SUCCESS)
	{
		PRINTF("sensorModAPI.c:sensorSetParam()::Message send failed for cmd CT_CONFIGURE \r\n" );
		return DG_FAIL;
	}

	//Message send success. Now we will wait for response
	PRINTF("CTAPI.c:ctConfigure():Message sent. Waiting for response\r\n" );
	xTaskNotifyWait(0,0,NULL, portMAX_DELAY);

	//Response received. Check the results
	if(result == DG_SUCCESS)
	{
		return DG_SUCCESS;
	}
	return DG_FAIL;

}

int getSensorStatus(uint8_t srcModule, float *curTemp, float *curHumidity, float *sT, float *sH, uint8_t *sensorStatus, uint8_t *moduleStatus)
{
	dgMsg_t sendMsgBuf;
	dgSensorModStatus_t sensorModStatus;
	uint8_t result;

	result = DG_FAIL;

	//Populate the message to send to the modbus task
	sendMsgBuf.src_module = srcModule;
	sendMsgBuf.command = GET_SENSOR_STATUS;
	sendMsgBuf.cmdParam = (void*)&sensorModStatus;
	sendMsgBuf.dest_module = SENSOR_MOD;
	sendMsgBuf.result = &result;
	sendMsgBuf.taskHandleSM = xTaskGetCurrentTaskHandle();
	if(sendMsgBuf.taskHandleSM == NULL)
	{
		PRINTF("sensorModAPI.c:getSensorStatus():Task handle is null for module with id: %d \r\n", srcModule);
		return DG_FAIL;
	}

	if(sendMsg(&sendMsgBuf) != DG_SUCCESS)
	{
		PRINTF("sensorModAPI.c:getSensorStatus()::Message send failed for cmd CT_CONFIGURE \r\n" );
		return DG_FAIL;
	}

	//Message send success. Now we will wait for response
	xTaskNotifyWait(0,0,NULL, portMAX_DELAY);

	//Response received. Check the results
	if(result == DG_SUCCESS)
	{
		*curTemp = sensorModStatus.curTemp;
		*curHumidity = sensorModStatus.curHumidity;
		*sensorStatus = sensorModStatus.sensorStatus;
		*moduleStatus = sensorModStatus.sensorModStatus;
		*sT = sensorModStatus.setTemp;
		*sH = sensorModStatus.setHumidity;

		return DG_SUCCESS;
	}
	return DG_FAIL;
}

