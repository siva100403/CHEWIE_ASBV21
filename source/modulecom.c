

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

/* Other Includes */
#include <dgCommon.h>
#include <modulecom.h>


/***************Definitions******************************/

#define MSGQUEUE_MAX_ITEMS		8
#define MSGQUEUE_ITEM_SIZE		sizeof(dgMsg_t)

/***************Variable declarations********************/

dgModule_t moduleStore[LAST_MODULE];


/*****************Implementation*************************/

/**************initModuleStore()*************************/
// This module initializes the structure used to store module related information
//This has to be called before calling, registering module and methods related communication

void initModuleStore(void)
{
	int i;
	for(i=0; i<LAST_MODULE; i++)
	{
		moduleStore[i].modQHandle = NULL;
		moduleStore[i].modTaskhandle = NULL;
		moduleStore[i].modTimerHandle = NULL;
	}
}

/**********************registerModule()*****************
 * This method has to be called to register a module after the task for the
 * module has been created. This method creates a message queue for the communication
 * and store in a structure. It also stores the handle of the task for notification.
 * Input Parameters:
 * 		ModuleId - ID defined in the modulecom.h file
 * 		modTaskHandle - Handle of the task corresponding to the module
 * Return Values:
 * 		DG_SUCCESS -
 * 		DG_INVALID_PARAM
 */

int	registerModule(uint8_t moduleId, TaskHandle_t modTaskHandle, TimerHandle_t timerHandle )
{
	//Parameter validations
	if(moduleId >= LAST_MODULE)
	{
		return DG_INVALID_PARAM;
	}
	if(modTaskHandle == NULL)
	{
		return DG_INVALID_PARAM;
	}

	//Check whether if the module is already registered
	if((moduleStore[moduleId].modQHandle != NULL) || (moduleStore[moduleId].modTaskhandle != NULL))
	{
		return DG_INVALID_PARAM;
	}
	
	//Create a queue for the module
	moduleStore[moduleId].modQHandle = xQueueCreate(MSGQUEUE_MAX_ITEMS,MSGQUEUE_ITEM_SIZE);
	if(moduleStore[moduleId].modQHandle == NULL)
	{
		//Message Q creation failed
		return DG_FAIL;
	}
	//Store Task handle
	moduleStore[moduleId].modTaskhandle = modTaskHandle;
	
	//Store timer handle if it is non-null. Timer handle is optional for a module
	moduleStore[moduleId].modTimerHandle = timerHandle;

	return DG_SUCCESS;
}

int sendMsg(dgMsg_t *message)
{

	QueueHandle_t destQHandle;
	uint8_t desModuleId;

	if(message == NULL)
	{
		return DG_INVALID_PARAM;
	}

	desModuleId = message->dest_module;

	if((desModuleId == 0)|| (desModuleId >= LAST_MODULE))
	{
		return DG_INVALID_PARAM;
	}

	destQHandle = moduleStore[desModuleId].modQHandle;
	if(destQHandle == NULL)
	{
		return DG_FAIL;
	}
	//Get the current tasks handle for task notify
//	message->taskHandleSM = moduleStore[desModuleId].modTaskhandle;

	
	//Write the msg into Queue
	if (xQueueSend( destQHandle, message, 0 ) == pdTRUE)
	{
		return DG_SUCCESS;
	}
	return DG_FAIL;
}

int sendMsgFromISR(dgMsg_t *message)
{
    BaseType_t xHigherPriorityTaskWoken;

	QueueHandle_t destQHandle;
	uint8_t desModuleId;

	if(message == NULL)
	{
		return DG_INVALID_PARAM;
	}

	desModuleId = message->dest_module;

	if((desModuleId == 0)|| (desModuleId >= LAST_MODULE))
	{
		return DG_INVALID_PARAM;
	}

	destQHandle = moduleStore[desModuleId].modQHandle;
	if(destQHandle == NULL)
	{
		return DG_FAIL;
	}
	//Get the current tasks handle for task notify
//	message->taskHandleSM = moduleStore[desModuleId].modTaskhandle;



	//Write the msg into Queue
	if (xQueueSendFromISR( destQHandle, (void*)message, &xHigherPriorityTaskWoken ) == pdTRUE)
	{
	    if( xHigherPriorityTaskWoken )
	    {
	        /* Actual macro used here is port specific. */
	        //taskYIELD_FROM_ISR ();
	    }
		return DG_SUCCESS;
	}
	return DG_FAIL;
}

QueueHandle_t getQHandle(uint8_t moduleid)
{
	if((moduleid == 0)||(moduleid >= LAST_MODULE))
	{
		return NULL;
	}
	return moduleStore[moduleid].modQHandle;
}

TaskHandle_t getTaskHandle(uint8_t moduleid)
{
	if((moduleid == 0)||(moduleid >= LAST_MODULE))
	{
		return NULL;
	}
	return moduleStore[moduleid].modTaskhandle;
}

TimerHandle_t getTimerHandle(uint8_t moduleid)
{
	if((moduleid == 0)||(moduleid >= LAST_MODULE))
	{
		return NULL;
	}
	return moduleStore[moduleid].modTimerHandle;
}


int moduleStart(uint8_t moduleId)
{
	dgMsg_t sendMsgBuf;
	uint8_t result;

	result = DG_FAIL;

	//Populate the message to send to the modbus task
	sendMsgBuf.src_module = UNKNOWN;
	sendMsgBuf.command = DG_MODULE_START;
	sendMsgBuf.cmdParam = NULL;
	sendMsgBuf.dest_module = moduleId;
	sendMsgBuf.result = &result;
	sendMsgBuf.cmdParam = NULL;
	sendMsgBuf.taskHandleSM = xTaskGetCurrentTaskHandle();
	if(sendMsgBuf.taskHandleSM == NULL)
	{
		printf("modulecom.c:moduleStart():Task handle is null\r\n");
		return DG_FAIL;
	}

	if(sendMsg(&sendMsgBuf) != DG_SUCCESS)
	{
		printf("modulecom.c:moduleStart():Message send failed \r\n" );
		return DG_FAIL;
	}
	//No need to wait for response and hence return

	return DG_SUCCESS;

}

int moduleStop(uint8_t moduleId)
{
	dgMsg_t sendMsgBuf;
	uint8_t result;

	result = DG_FAIL;

	//Populate the message to send to the modbus task
	sendMsgBuf.src_module = UNKNOWN;
	sendMsgBuf.command = DG_MODULE_STOP;
	sendMsgBuf.cmdParam = NULL;
	sendMsgBuf.dest_module = moduleId;
	sendMsgBuf.result = &result;
	sendMsgBuf.cmdParam = NULL;
	sendMsgBuf.taskHandleSM = xTaskGetCurrentTaskHandle();
	if(sendMsgBuf.taskHandleSM == NULL)
	{
		printf("modulecom.c:moduleStop():Task handle is null\r\n");
		return DG_FAIL;
	}

	if(sendMsg(&sendMsgBuf) != DG_SUCCESS)
	{
		printf("modulecom.c:moduleStop():Message send failed \r\n" );
		return DG_FAIL;
	}
	//No need to wait for response and hence return

	return DG_SUCCESS;

}
