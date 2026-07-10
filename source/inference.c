/*
 * inference.c
 *
 *  Created on: 10-Jul-2026
 *      Author: Jawahar Arumugam
 */

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"

/* Freescale includes. */
#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_clock.h"
#include "fsl_lpuart.h"
#include "fsl_lpspi.h"
#include "fsl_lpi2c.h"

/* Other Includes */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
#include "hatcsMod.h"
#include "hatcsModAPI.h"
#include "HMICmdProc.h"
#include "HMICmdProcAPI.h"
#include "actuatorStatusTracker.h"
#include "AlarmManagerAPI.h"
#include "AlarmManager.h"
#include "inference.h"




int doInference()
{
	dgMsg_t sendMsgBuf;
	uint8_t result;

	result = DG_FAIL;

	//Populate the message to send to the modbus task
	sendMsgBuf.src_module = UNKNOWN;
	sendMsgBuf.command = INF_CMD_START;
	sendMsgBuf.cmdParam = NULL;
	sendMsgBuf.dest_module = INF_MOD;
	sendMsgBuf.result = &result;
	sendMsgBuf.cmdParam = NULL;
	sendMsgBuf.taskHandleSM = xTaskGetCurrentTaskHandle();
	if(sendMsgBuf.taskHandleSM == NULL)
	{
		printf("inference.c:doInference():Task handle is null\r\n");
		return DG_FAIL;
	}

	if(sendMsg(&sendMsgBuf) != DG_SUCCESS)
	{
		printf("inference.c:doInference()::Message send failed \r\n" );
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


static void inferenceModule_task(void *pvParameters)
{
	QueueHandle_t inferenceModuleQHandle;
	dgMsg_t rcvMsg;					//Holds the currently received message
	static int infModuleState;		// This stores the state of Lid module


	//Wait till module registration is complete
	inferenceModuleQHandle = NULL;
	while(inferenceModuleQHandle== NULL)
	{
		vTaskDelay(100 / portTICK_PERIOD_MS);
		inferenceModuleQHandle = getQHandle(INF_MOD);
	}

	//Initialize the state of CT to IDLE
	infModuleState = INFMOD_STATE_IDLE;


	printf("infModule.c:infModule_task():started\r\n");

	while(1)
	{
		//Receive event from Queue. Block until event is available
		if(xQueueReceive(inferenceModuleQHandle, &rcvMsg, portMAX_DELAY ) != pdPASS )
		{
			// Queue did not return an event. Hence go back
			continue;
		}
		printf("infModule.c:infModule_task():received event %d in state %d\r\n", rcvMsg.command,infModuleState );
		switch(infModuleState)
		{
		case INFMOD_STATE_IDLE:
			switch(rcvMsg.command)
			{
			case DG_MODULE_START:
				//Create interpreter and load model.
				  //Success go to ready state else error state
					/*TODO*/
				//Go to READY state and wait for commands
				infModuleState = INFMOD_STATE_READY;
				break;
			case DG_MODULE_STOP:
				//Ignore in IDLE state
				break;
			case INF_CMD_START:
				//Ignore in IDLE state. But we need to send a response back to calling module
				//Get the event details from received message
				if(rcvMsg.taskHandleSM != NULL)
				{
					*(rcvMsg.result) = DG_FAIL;
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				break;

			case DG_TIMER_EXPIRY:
				//Timer event should not occur in IDLE state. Stop timer
				dgtimerStop(INF_MOD);
				break;

			default:
				printf("infModule.c:infModule_task():Invalid Event:%d in state INFMOD_STATE_IDLE - Ignored)\r\n",rcvMsg.command);
				break;
			}
			break;

		case INFMOD_STATE_READY:
			switch(rcvMsg.command)
			{
			case DG_MODULE_START:
				//Already active. No need to do anything
				break;

			case DG_MODULE_STOP:
				//Free the interpreter
				/*TODO*/

				infModuleState = INFMOD_STATE_IDLE;
				break;
			case INF_CMD_START:

				//Image pre-processing
				/*TODO*/
					//Crop 320X480 to 288X288 -consider the center
					//Subsample  the image to 96X96 (pick one in 3
					//Convert RGB565 to RGB888 for the model
				//Run Inference
				/*TODO*/
					//Run inference with the 96X96image

				//Post processing
				/*TODO*/
					//Process the output to get EMPTY/NON-EMPTY
					//Send event to shredder Module
				//Change state to READY
				infModuleState = INFMOD_STATE_READY;
				break;

			case DG_TIMER_EXPIRY:
				//Timer event should not occur in IDLE state. Stop timer
				break;

			default:
				printf("infModule.c:infModule_task():Invalid Event:%d in state INFMOD_STATE_READY - Ignored)\r\n",rcvMsg.command);
				break;
			}
			break;

		default:
			//Invalid state
			printf("infModule.c:infModule_task(): Unkown state:%d. Ignored)\r\n",infModuleState);
			break;
		}
	}
}



int initInferenceModule(void)
{

	TaskHandle_t inferenceModuleTaskHandle;
	TimerHandle_t inferenceModuleTimerHandle;
	BaseType_t result;

	//Create Inference Module task
	result = xTaskCreate(inferenceModule_task, "inferenceModule_task", configMINIMAL_STACK_SIZE + 500, NULL, task_PRIORITY, &inferenceModuleTaskHandle);
    if ( result !=    pdPASS)
    {
        PRINTF("inferenceModule_task creation failed!.\r\n");
        return DG_FAIL;
    }
    //inferenceModule_task requires single shot timer and hence create FreeRTOS SW timer
    inferenceModuleTimerHandle = xTimerCreate("inferenceModuleTimer",100, pdFALSE, (void*)LID_MOD, dgTimerCallback);
    if(inferenceModuleTimerHandle == NULL)
    {
        PRINTF("Timer creation failed!.\r\n");
    	vTaskDelete(inferenceModuleTaskHandle);
        return DG_FAIL;
    }
    if(registerModule(INF_MOD, inferenceModuleTaskHandle, inferenceModuleTimerHandle)!= DG_SUCCESS)
    {
    	//Registering the module failed. Hence kill the task and return error
        PRINTF("inferenceModule_task registration failed!.\r\n");
        xTimerDelete(inferenceModuleTimerHandle,100 / portTICK_PERIOD_MS);
    	vTaskDelete(inferenceModuleTaskHandle);

    	return DG_FAIL;
    }
    return DG_SUCCESS;
}
