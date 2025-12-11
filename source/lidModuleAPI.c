/*
 * lidModuleAPI.c
 *
 *  Created on: 16-May-2025
 *      Author: Jawahar Arumugam
 */

/* FreeRTOS kernel includes. */

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
#include "augerAPI.h"
#include "shredder.h"
#include "shredderAPI.h"
#include "adcs.h"
#include "limitSwitchMod.h"
#include "transferCS.h"
#include "transferCSAPI.h"
#include "lidModule.h"
#include "lidModuleAPI.h"





int sendProximityEvent(dgProximityEvents_t *eventPtr)
{
	dgMsg_t sendMsgBuf;
	uint8_t result;

	result = DG_FAIL;

	//Populate the message to send to the modbus task
	sendMsgBuf.src_module = UNKNOWN;
	sendMsgBuf.command = PROXIMITY_EVENT;
	sendMsgBuf.cmdParam = NULL;
	sendMsgBuf.dest_module = LID_MOD;
	sendMsgBuf.result = &result;
	sendMsgBuf.cmdParam = (void*)eventPtr;
	sendMsgBuf.taskHandleSM = xTaskGetCurrentTaskHandle();
	if(sendMsgBuf.taskHandleSM == NULL)
	{
		printf("lidModuleAPI.c:sendProximityEvnt():Task handle is null\r\n");
		return DG_FAIL;
	}

	if(sendMsg(&sendMsgBuf) != DG_SUCCESS)
	{
		printf("lidModuleAPI.c:sendProximityEvnt():Message send failed \r\n" );
		return DG_FAIL;
	}
	//Message sent. Wait for response
	//Message send success. Now we will wait for response

	xTaskNotifyWait(0,0,NULL, portMAX_DELAY);

	//Response received. Check the results
	if(result == DG_SUCCESS)
	{
		return DG_SUCCESS;
	}
	return DG_FAIL;

}

int sendLidSwitchEvent(uint8_t switchEvent)
{
	dgMsg_t sendMsgBuf;
	uint8_t result;

	result = DG_FAIL;

	//Populate the message to send to the modbus task
	sendMsgBuf.src_module = UNKNOWN;
	sendMsgBuf.command = LIDSWITCH_EVENT;
	sendMsgBuf.cmdParam = NULL;
	sendMsgBuf.dest_module = LID_MOD;
	sendMsgBuf.result = &result;
	sendMsgBuf.cmdParam = (void*)&switchEvent;
	sendMsgBuf.taskHandleSM = xTaskGetCurrentTaskHandle();
	if(sendMsgBuf.taskHandleSM == NULL)
	{
		PRINTF("lidModuleAPI.c:sendLidSwitchEvnt():Task handle is null\r\n");
		return DG_FAIL;
	}

	if(sendMsg(&sendMsgBuf) != DG_SUCCESS)
	{
		PRINTF("lidModuleAPI.c:sendLidSwitchEvnt():Message send failed \r\n" );
		return DG_FAIL;
	}
	//No need to wait for response and hence return

	return DG_SUCCESS;

}
