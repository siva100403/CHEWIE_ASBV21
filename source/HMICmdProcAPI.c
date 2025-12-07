/*
 * HMICmdProcAPI.c
 *
 *  Created on: 05-May-2025
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
#include "HMICmdProc.h"
#include "HMICmdProcAPI.h"


int sendRcvBuffer(uint8_t *rcvdBuffer, uint16_t rcvdBufferSize)
{
	dgMsg_t sendMsgBuf;
	uint8_t result;
	dgAsbComParam_t  commBuffers;

	//Validate parameters
	if((rcvdBuffer == NULL)||(rcvdBufferSize>MAX_TX_BUFFER_SIZE))
	{
		return DG_INVALID_PARAM;
	}
	//Populate commBuffer
	commBuffers.rxBufferSize = rcvdBufferSize;
	commBuffers.rxBuffer = rcvdBuffer;
	commBuffers.txBufferSize = 0;
	commBuffers.txBuffer = NULL;

	result = DG_FAIL;

	//Populate the message to send to the modbus task
//	sendMsgBuf.src_module = srcModule;
	sendMsgBuf.command = HMICMD_BUFFER_RECEIVED;

	sendMsgBuf.cmdParam = (void*)&commBuffers;
	sendMsgBuf.dest_module = HMICMDPROC_MOD;
	sendMsgBuf.result = &result;
	sendMsgBuf.taskHandleSM = NULL;  //No reponse expected


	if(sendMsgFromISR(&sendMsgBuf) != DG_SUCCESS)
	{
		PRINTF("HMICmdProcAPI.c:sendRcvBuffer()::Message send failed \r\n" );
		return DG_FAIL;
	}

	//Message send success. Now we will wait for response
	//xTaskNotifyWait(0,0,NULL, portMAX_DELAY);

	return DG_SUCCESS;

}

int sendTxCompleteEvent()
{
	dgMsg_t sendMsgBuf;
	uint8_t result;

	result = DG_FAIL;

	//Populate the message

	sendMsgBuf.command = HMICMD_RESPONSE_SENT;

	sendMsgBuf.cmdParam = NULL;
	sendMsgBuf.dest_module = HMICMDPROC_MOD;
	sendMsgBuf.result = &result;
	sendMsgBuf.taskHandleSM = NULL;  //No reponse expected


	if(sendMsgFromISR(&sendMsgBuf) != DG_SUCCESS)
	{
		PRINTF("HMICmdProcAPI.c:sendTxCompleteEvent()::Message send failed \r\n" );
		return DG_FAIL;
	}

	//Message send success. Now we will wait for response
	//xTaskNotifyWait(0,0,NULL, portMAX_DELAY);

	return DG_SUCCESS;

}

