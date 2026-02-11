 /*
 * lidModule.c
 *
 *  Created on: 15-May-2025
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

/*

static void lidModule_task(void *pvParameters)
{
	QueueHandle_t lidModuleQHandle;
	dgMsg_t rcvMsg;					//Holds the currently received message
	static int lidModuleState;		// This stores the state of Lid module
	static uint8_t capProximityState;  //Current state of capacitive proximity state


	//Wait till module registration is complete
	lidModuleQHandle = NULL;
	while(lidModuleQHandle== NULL)
	{
		vTaskDelay(100 / portTICK_PERIOD_MS);
		lidModuleQHandle = getQHandle(LID_MOD);
	}

	//Initialize the state of CT to IDLE
	lidModuleState = LIDMOD_STATE_IDLE;
	capProximityState = CAP_PROXIMITY_EVENT_REMOVED;

	printf("lidModule.c:lidModule_task():started\r\n");

	while(1)
	{
		//Receive event from Queue. Block until event is available
		if(xQueueReceive(lidModuleQHandle, &rcvMsg, portMAX_DELAY ) != pdPASS )
		{
			// Queue did not return an event. Hence go back
			continue;
		}
		printf("lidModule.c:lidModule_task():received event %d in state %d\r\n", rcvMsg.command,lidModuleState );
		switch(lidModuleState)
		{
		case LIDMOD_STATE_IDLE:
			switch(rcvMsg.command)
			{
			case DG_MODULE_START:
				//Read Limit switch
				uint8_t status= getLidSwicthStatus();
				if(status == LID_STATUS_CLOSED)
				{
					lidModuleState = LIDMOD_STATE_CLOSE;
					printf("lidModule.c:lidModule_task():LID is in closed condition\r\n");
				}
				else if(status == LID_STATUS_OPEN)
				{
					lidModuleState = LIDMOD_STATE_OPEN;
					printf("lidModule.c:lidModule_task():LID is in open condition\r\n");
					dgtimerStart(LID_MOD, CONV_SEC_TO_TICKS(NONPROXIMITY_LIDCLOSE_TIMEOUT));
				}
				else if(status == LID_STATUS_INBETWEEN)
				{
					lidModuleState = LIDMOD_STATE_OPEN;
					dgtimerStart(LID_MOD, CONV_SEC_TO_TICKS(NONPROXIMITY_LIDCLOSE_TIMEOUT));
					printf("lidModule.c:lidModule_task():LID is in in between condition\r\n");
				}
				else if(status == LID_STATUS_ERROR)
				{
					//TODO: Handle
					printf("lidModule.c:lidModule_task():LID is in error condition\r\n");
				}
				else
				{
					printf("lidModule.c:lidModule_task():LID is unknown condition\r\n");
				}
				break;
			case DG_MODULE_STOP:
				//Ignore in IDLE state
				break;
			case PROXIMITY_EVENT:
				//Ignore in IDLE state. But we need to send a response back to calling module
				//Get the event details from received message
				if(rcvMsg.taskHandleSM != NULL)
				{
					*(rcvMsg.result) = DG_SUCCESS;
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				break;

			case LIDSWITCH_EVENT:
				//Ignore in IDLE state
				break;
			case DG_TIMER_EXPIRY:
				//Timer event should not occur in IDLE state. Stop timer
				dgtimerStop(LID_MOD);
				//lidMotorStop();
				break;
			default:
				printf("lidModule.c:lidModule_task():Invalid Event:%d in state LIDMOD_STATE_IDLE - Ignored)\r\n",rcvMsg.command);
				break;
			}
			break;

		case LIDMOD_STATE_CLOSE:
			printf("lidModule.c:lidModule_task():Rcvd Event:%d in state LIDMOD_STATE_CLOSE)\r\n",rcvMsg.command);
			switch(rcvMsg.command)
			{
			case DG_MODULE_START:
				//Ignore. Already active
				break;
			case DG_MODULE_STOP:
				dgtimerStop(LID_MOD);
				lidModuleState = LIDMOD_STATE_IDLE;
				break;
			case PROXIMITY_EVENT:
				//Get the event details from received message
				dgProximityEvents_t *proximityEvent;
				uint8_t event;
				proximityEvent = (dgProximityEvents_t*)rcvMsg.cmdParam;

				//Currently we handle only Cap proximity sensor events
				event = ((proximityEvent->proximity) & CAP_PROXIMITY_EVENT_MASK);
				if(event == CAP_PROXIMITY_EVENT_DETECTED)
				{
					printf("lidModule.c:lidModule_task():Proximity Detected in CLOSE state\r\n");
					//Proximity event detected. Give command to OPEN the lid
					LID_MOTOR_DIR_OPEN();
					LID_POWER_ON();

					capProximityState = CAP_PROXIMITY_EVENT_DETECTED;
					lidModuleState = LIDMOD_STATE_OPENING;
					//Start timer to close the Lid if Proximity event is not removed
					dgtimerStart(LID_MOD, LIDOPENING_DURATION/portTICK_PERIOD_MS);
				}
				else if (event == CAP_PROXIMITY_EVENT_REMOVED)
				{
					//Proximity event removed when LID is in closed state. Ignore
					capProximityState = CAP_PROXIMITY_EVENT_REMOVED;
					printf("lidModule.c:lidModule_task():Proximity removed in CLOSE state\r\n");
				}
				else
				{
					printf("lidModule.c:lidModule_task():No Event in CLOSE state\r\n");
					//No event reported. Ignore
				}
				if(rcvMsg.taskHandleSM != NULL)
				{
					*(rcvMsg.result) = DG_SUCCESS;
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				break;
			case LIDSWITCH_EVENT:
				//Not implemented. Ignore
				TODO
				break;
			case DG_TIMER_EXPIRY:
				break;
			default:
				printf("lidModule.c:lidModule_task():Invalid Event:%d in state LIDMOD_STATE_CLOSE - Ignored)\r\n",rcvMsg.command);
				break;
			}
			break;

		case LIDMOD_STATE_OPENING:
			printf("lidModule.c:lidModule_task():Rcvd Event:%d in state LIDMOD_STATE_OPENING)\r\n",rcvMsg.command);
			switch(rcvMsg.command)
			{
			case DG_MODULE_START:
				//Ignore. Already active
				break;
			case DG_MODULE_STOP:     TODO //This is way of handling will cause Lid to stop midway. Need to be modified
				dgtimerStop(LID_MOD);
				//lidMotorStop();
				lidModuleState = LIDMOD_STATE_IDLE;
				break;
			case PROXIMITY_EVENT:
				//Get the event details from received message
				dgProximityEvents_t *proximityEvent;
				uint8_t event;
				proximityEvent = (dgProximityEvents_t*)rcvMsg.cmdParam;

				//Currently we handle only Cap proximity sensor events
				event = ((proximityEvent->proximity) & CAP_PROXIMITY_EVENT_MASK);
				if(event == CAP_PROXIMITY_EVENT_DETECTED)
				{
					//Proximity event detected. Lid is opening. Ignore event
					capProximityState = CAP_PROXIMITY_EVENT_DETECTED;
				}
				else if (event == CAP_PROXIMITY_EVENT_REMOVED)
				{
					//Proximity event removed. Ignore event.
					capProximityState = CAP_PROXIMITY_EVENT_REMOVED;
				}
				else
				{
					//No event reported. Ignore
				}
				if(rcvMsg.taskHandleSM != NULL)
				{
					*(rcvMsg.result) = DG_SUCCESS;
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				break;
			case LIDSWITCH_EVENT:
				//Ignore. we are not generating this event now
				//lidMotorStop(); //In case motor direction is reversed for OPEN and CLOSE
				TODO
				break;
			case DG_TIMER_EXPIRY:
				//Lid open duration reached. Stop lid motor
				LID_POWER_OFF();
				lidModuleState = LIDMOD_STATE_OPEN;
				event_lid_open(LIMITSWITCH_MOD);
				dgtimerStart(LID_MOD, CONV_SEC_TO_TICKS(PROXIMITY_LIDCLOSE_TIMEOUT));
				break;
			default:
				printf("lidModule.c:lidModule_task():Invalid Event:%d in state LIDMOD_STATE_OPENING - Ignored)\r\n",rcvMsg.command);
				break;
			}
			break;

		case LIDMOD_STATE_OPEN:
			printf("lidModule.c:lidModule_task():Rcvd Event:%d in state LIDMOD_STATE_OPEN)\r\n",rcvMsg.command);
			switch(rcvMsg.command)
			{
			case DG_MODULE_START:
				//Ignore. Already active
				break;
			case DG_MODULE_STOP:  TODO // Lid has to be closed before going to IDLE
				dgtimerStop(LID_MOD);
				lidModuleState = LIDMOD_STATE_IDLE;
				break;
			case PROXIMITY_EVENT:
				//Get the event details from received message
				dgProximityEvents_t *proximityEvent;
				uint8_t event;
				proximityEvent = (dgProximityEvents_t*)rcvMsg.cmdParam;

				//Currently we handle only Cap proximity sensor events
				event = ((proximityEvent->proximity) & CAP_PROXIMITY_EVENT_MASK);
				if(event == CAP_PROXIMITY_EVENT_DETECTED)
				{
					//Proximity event detected. Lid is already open. Extend the time
					dgtimerStart(LID_MOD, CONV_SEC_TO_TICKS(PROXIMITY_LIDCLOSE_TIMEOUT));
				}
				else if (event == CAP_PROXIMITY_EVENT_REMOVED)
				{
					//Proximity event removed. After a timeout, we can close the lid.
					dgtimerStop(LID_MOD);
					dgtimerStart(LID_MOD, CONV_SEC_TO_TICKS(NONPROXIMITY_LIDCLOSE_TIMEOUT));
					capProximityState = CAP_PROXIMITY_EVENT_REMOVED;
				}
				else
				{
					capProximityState = CAP_PROXIMITY_EVENT_DETECTED;
					//No event reported. Ignore
				}
				if(rcvMsg.taskHandleSM != NULL)
				{
					*(rcvMsg.result) = DG_SUCCESS;
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				break;
			case LIDSWITCH_EVENT:
				//Ignore. we are not generating this event now
				TODO
				break;
			case DG_TIMER_EXPIRY:
				//LID Open for longer than 15 sec. Close the Lid
				LID_MOTOR_DIR_CLOSE();
				LID_POWER_ON();
				lidModuleState = LIDMOD_STATE_CLOSING;
				dgtimerStart(LID_MOD, LIDCLOSING_DURATION/portTICK_PERIOD_MS);
				break;
			default:
				printf("lidModule.c:lidModule_task():Invalid Event:%d in state LIDMOD_STATE_OPEN - Ignored)\r\n",rcvMsg.command);
				break;
			}
			break;


		case LIDMOD_STATE_CLOSING:
			printf("lidModule.c:lidModule_task():Rcvd Event:%d in state LIDMOD_STATE_CLOSING)\r\n",rcvMsg.command);
			switch(rcvMsg.command)
			{
			case DG_MODULE_START:
				//Ignore. Already active
				break;
			case DG_MODULE_STOP:      TODO //Modify to stop the motor
				dgtimerStop(LID_MOD);
				lidModuleState = LIDMOD_STATE_IDLE;
				break;
			case PROXIMITY_EVENT:
				//Get the event details from received message
				dgProximityEvents_t *proximityEvent;
				uint8_t event;
				proximityEvent = (dgProximityEvents_t*)rcvMsg.cmdParam;

				//Currently we handle only Cap proximity sensor events
				event = ((proximityEvent->proximity) & CAP_PROXIMITY_EVENT_MASK);
				if(event == CAP_PROXIMITY_EVENT_DETECTED)
				{
					//Proximity event detected. Lid is already open. Extend the time
					capProximityState = CAP_PROXIMITY_EVENT_DETECTED;
				}
				else if (event == CAP_PROXIMITY_EVENT_REMOVED)
				{
					//Ignore. This event should not occur.
					capProximityState = CAP_PROXIMITY_EVENT_REMOVED;
				}
				else
				{
					//No event reported. Ignore
				}
				if(rcvMsg.taskHandleSM != NULL)
				{
					*(rcvMsg.result) = DG_SUCCESS;
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				break;
			case LIDSWITCH_EVENT:
				//Stop the motor. Lid closed
				lidMotorStop();
				dgtimerStop(LID_MOD);
				lidModuleState = LIDMOD_STATE_CLOSE_WAIT;
				dgtimerStart(LID_MOD, LID_CLOSE_WAIT/portTICK_PERIOD_MS);
				break;
			case DG_TIMER_EXPIRY:
				//Timer expired. Now we can stop the motor
				LID_POWER_OFF();
				event_lid_close(LIMITSWITCH_MOD);
				lidModuleState = LIDMOD_STATE_CLOSE;
				//dgtimerStart(LID_MOD, LID_CLOSE_WAIT/portTICK_PERIOD_MS);
				break;
			default:
				printf("lidModule.c:lidModule_task():Invalid Event:%d in state LIDMOD_STATE_CLOSETIMEOUT - Ignored)\r\n",rcvMsg.command);
				break;
			}
			break;
		default:
			//Invalid state
			printf("lidModule.c:lidModule_task(): Unkown state:%d. Ignored)\r\n",lidModuleState);
			break;
		}

	}
}

*/



static void lidModule_task(void *pvParameters)
{
	QueueHandle_t lidModuleQHandle;
	dgMsg_t rcvMsg;					//Holds the currently received message
	static int lidModuleState;		// This stores the state of Lid module
	static uint8_t capProximityState;  //Current state of capacitive proximity state
	uint8_t *lidEvent;


	//Wait till module registration is complete
	lidModuleQHandle = NULL;
	while(lidModuleQHandle== NULL)
	{
		vTaskDelay(100 / portTICK_PERIOD_MS);
		lidModuleQHandle = getQHandle(LID_MOD);
	}

	//Initialize the state of CT to IDLE
	lidModuleState = LIDMOD_STATE_IDLE;
	capProximityState = CAP_PROXIMITY_EVENT_REMOVED;

	printf("lidModule.c:lidModule_task():started\r\n");

	while(1)
	{
		//Receive event from Queue. Block until event is available
		if(xQueueReceive(lidModuleQHandle, &rcvMsg, portMAX_DELAY ) != pdPASS )
		{
			// Queue did not return an event. Hence go back
			continue;
		}
		printf("lidModule.c:lidModule_task():received event %d in state %d\r\n", rcvMsg.command,lidModuleState );
		switch(lidModuleState)
		{
		case LIDMOD_STATE_IDLE:
			switch(rcvMsg.command)
			{
			case DG_MODULE_START:
				//Set LID Motor direction to CLOSE and Turn On supply to LID motor
				LID_MOTOR_DIR_CLOSE();
				LID_POWER_ON();

				//enable lidstatus sensing by limitswitch sensing module
				enableLidStatusSensing();
				dgtimerStart(LID_MOD, CONV_SEC_TO_TICKS(LIDCLOSING_DURATION));

				//Go to READY state and wait for limit switch event
				lidModuleState = LIDMOD_STATE_READY;
				break;
			case DG_MODULE_STOP:
				//Ignore in IDLE state
				break;
			case PROXIMITY_EVENT:
				//Ignore in IDLE state. But we need to send a response back to calling module
				//Get the event details from received message
				if(rcvMsg.taskHandleSM != NULL)
				{
					*(rcvMsg.result) = DG_SUCCESS;
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				break;

			case LIDSWITCH_EVENT:
				//Ignore in IDLE state
				break;
			case DG_TIMER_EXPIRY:
				//Timer event should not occur in IDLE state. Stop timer
				dgtimerStop(LID_MOD);
				//lidMotorStop();
				break;
			default:
				printf("lidModule.c:lidModule_task():Invalid Event:%d in state LIDMOD_STATE_IDLE - Ignored)\r\n",rcvMsg.command);
				break;
			}
			break;

		case LIDMOD_STATE_READY:
			switch(rcvMsg.command)
			{
			case DG_MODULE_START:
				//Already active. No need to do anything
				break;
			case LIDSWITCH_EVENT:

				//Get the event parameter
				lidEvent = (uint8_t*)rcvMsg.cmdParam;
				if(*lidEvent == LID_STATUS_CLOSED)
				{
					dgtimerStop(LID_MOD);
					lidModuleState = LIDMOD_STATE_CLOSE;
					printf("lidModule.c:lidModule_task():LID is in closed condition\r\n");
				}
				else if(*lidEvent  == LID_STATUS_OPEN)
				{
					lidModuleState = LIDMOD_STATE_CLOSING;
					printf("lidModule.c:lidModule_task():LID is in open condition\r\n");
					dgtimerStart(LID_MOD, CONV_SEC_TO_TICKS(LIDCLOSING_DURATION));
				}
				else if(*lidEvent  == LID_STATUS_INBETWEEN)
				{
					lidModuleState = LIDMOD_STATE_CLOSING;
					dgtimerStart(LID_MOD, CONV_SEC_TO_TICKS(LIDCLOSING_DURATION));
					printf("lidModule.c:lidModule_task():LID is in-between\r\n");
				}
				else if(*lidEvent == LID_STATUS_ERROR)
				{
					//TODO: Handle
					printf("lidModule.c:lidModule_task():LID is in error condition\r\n");
				}
				else
				{
					printf("lidModule.c:lidModule_task():LID is unknown condition\r\n");
				}
				break;
			case DG_MODULE_STOP:
				//Disable lid status sensing by limit switch sensing module
				disableLidStatusSensing();
				//Disable power to Lid motor
				LID_MOTOR_DIR_CLOSE();
				LID_POWER_OFF();
				break;

			case PROXIMITY_EVENT:
				//Ignore in READY state. But we need to send a response back to calling module
				//Get the event details from received message
				if(rcvMsg.taskHandleSM != NULL)
				{
					*(rcvMsg.result) = DG_SUCCESS;
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				break;

			case DG_TIMER_EXPIRY:
				//Timer event should not occur in IDLE state. Stop timer
				LID_MOTOR_DIR_CLOSE();
				LID_POWER_OFF();
				dgtimerStop(LID_MOD);
				lidModuleState = LIDMOD_STATE_ERROR;
				break;
			default:
				printf("lidModule.c:lidModule_task():Invalid Event:%d in state LIDMOD_STATE_READY - Ignored)\r\n",rcvMsg.command);
				break;
			}
			break;

		case LIDMOD_STATE_CLOSE:
			printf("lidModule.c:lidModule_task():Rcvd Event:%d in state LIDMOD_STATE_CLOSE)\r\n",rcvMsg.command);
			switch(rcvMsg.command)
			{
			case DG_MODULE_START:
				//Ignore. Already active
				break;
			case DG_MODULE_STOP:
				//Disable lid status sensing by limit switch sensing module
				disableLidStatusSensing();
				//Disable power to Lid motor
				LID_MOTOR_DIR_CLOSE();
				LID_POWER_OFF();
				dgtimerStop(LID_MOD);
				lidModuleState = LIDMOD_STATE_IDLE;
				break;
			case PROXIMITY_EVENT:
				//Get the event details from received message
				dgProximityEvents_t *proximityEvent;
				uint8_t event;
				proximityEvent = (dgProximityEvents_t*)rcvMsg.cmdParam;

				//Currently we handle only Cap proximity sensor events
				event = ((proximityEvent->proximity) & CAP_PROXIMITY_EVENT_MASK);
				if(event == CAP_PROXIMITY_EVENT_DETECTED)
				{
					printf("lidModule.c:lidModule_task():Proximity Detected in CLOSE state\r\n");
					//Proximity event detected. Give command to OPEN the lid
					LID_MOTOR_DIR_OPEN();
					LID_POWER_ON();

					capProximityState = CAP_PROXIMITY_EVENT_DETECTED;
					lidModuleState = LIDMOD_STATE_OPENING;
					//Start timer to close the Lid if Proximity event is not removed
					dgtimerStart(LID_MOD, LIDOPENING_DURATION/portTICK_PERIOD_MS);
				}
				else if (event == CAP_PROXIMITY_EVENT_REMOVED)
				{
					//Proximity event removed when LID is in closed state. Ignore
					capProximityState = CAP_PROXIMITY_EVENT_REMOVED;
					printf("lidModule.c:lidModule_task():Proximity removed in CLOSE state\r\n");
				}
				else
				{
					printf("lidModule.c:lidModule_task():No Event in CLOSE state\r\n");
					//No event reported. Ignore
				}
				if(rcvMsg.taskHandleSM != NULL)
				{
					*(rcvMsg.result) = DG_SUCCESS;
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				break;
			case LIDSWITCH_EVENT:
				//LIMITSWITCH event not expected at this state
				printf("lidModule.c:lidModule_task():LS event 0x%X received in state LIDMOD_STATE_CLOSE \r\n", *(uint8_t*)rcvMsg.cmdParam);
				break;
			case DG_TIMER_EXPIRY:
				break;
			default:
				printf("lidModule.c:lidModule_task():Invalid Event:%d in state LIDMOD_STATE_CLOSE - Ignored)\r\n",rcvMsg.command);
				break;
			}
			break;

		case LIDMOD_STATE_OPENING:
			printf("lidModule.c:lidModule_task():Rcvd Event:%d in state LIDMOD_STATE_OPENING)\r\n",rcvMsg.command);
			switch(rcvMsg.command)
			{
			case DG_MODULE_START:
				//Ignore. Already active
				break;
			case DG_MODULE_STOP:     /*TODO*/ //This is way of handling will cause Lid to stop midway. Need to be modified
				dgtimerStop(LID_MOD);
				lidModuleState = LIDMOD_STATE_IDLE;
				break;
			case PROXIMITY_EVENT:
				//Get the event details from received message
				dgProximityEvents_t *proximityEvent;
				uint8_t event;
				proximityEvent = (dgProximityEvents_t*)rcvMsg.cmdParam;

				//Currently we handle only Cap proximity sensor events
				event = ((proximityEvent->proximity) & CAP_PROXIMITY_EVENT_MASK);
				if(event == CAP_PROXIMITY_EVENT_DETECTED)
				{
					//Proximity event detected. Lid is opening. Ignore event
					capProximityState = CAP_PROXIMITY_EVENT_DETECTED;
				}
				else if (event == CAP_PROXIMITY_EVENT_REMOVED)
				{
					//Proximity event removed. Ignore event.
					capProximityState = CAP_PROXIMITY_EVENT_REMOVED;
				}
				else
				{
					//No event reported. Ignore
				}
				if(rcvMsg.taskHandleSM != NULL)
				{
					*(rcvMsg.result) = DG_SUCCESS;
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				break;
			case LIDSWITCH_EVENT:

				//Get the event parameter
				lidEvent = (uint8_t*)rcvMsg.cmdParam;
				if(*lidEvent == LID_STATUS_OPEN)
				{
					//Stop timer
					dgtimerStop(LID_MOD);
					lidModuleState = LIDMOD_STATE_OPEN;
					dgtimerStart(LID_MOD, CONV_SEC_TO_TICKS(PROXIMITY_LIDCLOSE_TIMEOUT));
				}
				else
				{
					//We don't expect any other LS event. Ignore
				}

				break;
			case DG_TIMER_EXPIRY:
				//Lid open timer expired. Limit switch has not detected Open. It is an error condition
				LID_POWER_OFF();
				lidModuleState = LIDMOD_STATE_ERROR;
				printf("lidModule.c:lidModule_task():Timer Expiry in state LIDMOD_STATE_OPENING - Error condition\r\n");
				break;
			default:
				printf("lidModule.c:lidModule_task():Invalid Event:%d in state LIDMOD_STATE_OPENING - Ignored)\r\n",rcvMsg.command);
				break;
			}
			break;

		case  LIDMOD_STATE_ERROR:
			printf("lidModule.c:lidModule_task():LIDMOD_STATE_ERROR\r\n");
			break;

		case LIDMOD_STATE_OPEN:
			printf("lidModule.c:lidModule_task():Rcvd Event:%d in state LIDMOD_STATE_OPEN)\r\n",rcvMsg.command);
			switch(rcvMsg.command)
			{
			case DG_MODULE_START:
				//Ignore. Already active
				break;
			case DG_MODULE_STOP:  /*TODO*/ // Lid has to be closed before going to IDLE
				dgtimerStop(LID_MOD);
				lidModuleState = LIDMOD_STATE_IDLE;
				break;
			case PROXIMITY_EVENT:
				//Get the event details from received message
				dgProximityEvents_t *proximityEvent;
				uint8_t event;
				proximityEvent = (dgProximityEvents_t*)rcvMsg.cmdParam;

				//Currently we handle only Cap proximity sensor events
				event = ((proximityEvent->proximity) & CAP_PROXIMITY_EVENT_MASK);
				if(event == CAP_PROXIMITY_EVENT_DETECTED)
				{
					dgtimerStop(LID_MOD);
					//Proximity event detected. Lid is already open. Extend the time
					dgtimerStart(LID_MOD, CONV_SEC_TO_TICKS(PROXIMITY_LIDCLOSE_TIMEOUT));
				}
				else if (event == CAP_PROXIMITY_EVENT_REMOVED)
				{
					//Proximity event removed. After a timeout, we can close the lid.
					dgtimerStop(LID_MOD);
					dgtimerStart(LID_MOD, CONV_SEC_TO_TICKS(NONPROXIMITY_LIDCLOSE_TIMEOUT));
					capProximityState = CAP_PROXIMITY_EVENT_REMOVED;
				}
				else
				{
					capProximityState = CAP_PROXIMITY_EVENT_DETECTED;
					//No event reported. Ignore
				}
				if(rcvMsg.taskHandleSM != NULL)
				{
					*(rcvMsg.result) = DG_SUCCESS;
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				break;
			case LIDSWITCH_EVENT:
				//We don't expect any Limit switch event in LID OPEN state
				printf("lidModule.c:lidModule_task():Rcvd LS Event 0x%X in LID OPEN state\r\n",rcvMsg.command);

				break;
			case DG_TIMER_EXPIRY:
				//LID Open for longer than 15 sec. Close the Lid
				LID_MOTOR_DIR_CLOSE();
				LID_POWER_ON();
				lidModuleState = LIDMOD_STATE_CLOSING;
				dgtimerStart(LID_MOD, LIDCLOSING_DURATION/portTICK_PERIOD_MS);
				break;
			default:
				printf("lidModule.c:lidModule_task():Invalid Event:%d in state LIDMOD_STATE_OPEN - Ignored)\r\n",rcvMsg.command);
				break;
			}
			break;


		case LIDMOD_STATE_CLOSING:
			printf("lidModule.c:lidModule_task():Rcvd Event:%d in state LIDMOD_STATE_CLOSING)\r\n",rcvMsg.command);
			switch(rcvMsg.command)
			{
			case DG_MODULE_START:
				//Ignore. Already active
				break;
			case DG_MODULE_STOP:      /*TODO*/ //Modify to stop the motor
				dgtimerStop(LID_MOD);
				lidModuleState = LIDMOD_STATE_IDLE;
				break;
			case PROXIMITY_EVENT:
				//Get the event details from received message
				dgProximityEvents_t *proximityEvent;
				uint8_t event;
				proximityEvent = (dgProximityEvents_t*)rcvMsg.cmdParam;

				//Currently we handle only Cap proximity sensor events
				event = ((proximityEvent->proximity) & CAP_PROXIMITY_EVENT_MASK);
				if(event == CAP_PROXIMITY_EVENT_DETECTED)
				{
					//Proximity event detected. Lid is already open. Extend the time
					capProximityState = CAP_PROXIMITY_EVENT_DETECTED;
				}
				else if (event == CAP_PROXIMITY_EVENT_REMOVED)
				{
					//Ignore. This event should not occur.
					capProximityState = CAP_PROXIMITY_EVENT_REMOVED;
				}
				else
				{
					//No event reported. Ignore
				}
				if(rcvMsg.taskHandleSM != NULL)
				{
					*(rcvMsg.result) = DG_SUCCESS;
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				break;
			case LIDSWITCH_EVENT:
				uint8_t *lidevent;
				//Get the event parameter
				lidevent = (uint8_t*)rcvMsg.cmdParam;
				if(*lidevent == LID_STATUS_CLOSED)
				{
					//Stop timer
					dgtimerStop(LID_MOD);
					lidModuleState = LIDMOD_STATE_CLOSE;
				}
				else
				{
					//We don't expect any other LS event. Ignore
				}
				break;
			case DG_TIMER_EXPIRY:
				//Timer expired. Now we can stop the motor
				LID_POWER_OFF();
				lidModuleState = LIDMOD_STATE_ERROR;
				break;
			default:
				printf("lidModule.c:lidModule_task():Invalid Event:%d in state LIDMOD_STATE_CLOSETIMEOUT - Ignored)\r\n",rcvMsg.command);
				break;
			}
			break;
		default:
			//Invalid state
			printf("lidModule.c:lidModule_task(): Unkown state:%d. Ignored)\r\n",lidModuleState);
			break;
		}

	}
}



int initLidModule(void)
{

	TaskHandle_t lidModuleTaskHandle;
	TimerHandle_t lidModuleTimerHandle;
	BaseType_t result;

	//Create lidModule task
	result = xTaskCreate(lidModule_task, "lidModule_task", configMINIMAL_STACK_SIZE + 500, NULL, task_PRIORITY, &lidModuleTaskHandle);
    if ( result !=    pdPASS)
    {
        PRINTF("lidModule_task creation failed!.\r\n");
        return DG_FAIL;
    }
    //HMI CMD Proc requires single shot timer and hence create FreeRTOS SW timer
    lidModuleTimerHandle = xTimerCreate("lidModuleTimer",100, pdFALSE, (void*)LID_MOD, dgTimerCallback);
    if(lidModuleTimerHandle == NULL)
    {
        PRINTF("Timer creation failed!.\r\n");
    	vTaskDelete(lidModuleTaskHandle);
        return DG_FAIL;
    }
    if(registerModule(LID_MOD, lidModuleTaskHandle, lidModuleTimerHandle)!= DG_SUCCESS)
    {
    	//Registering the module failed. Hence kill the task and return error
        PRINTF("lidModule_task registration failed!.\r\n");
        xTimerDelete(lidModuleTimerHandle,100 / portTICK_PERIOD_MS);
    	vTaskDelete(lidModuleTaskHandle);

    	return DG_FAIL;
    }
    return DG_SUCCESS;
}

