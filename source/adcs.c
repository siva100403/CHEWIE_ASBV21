/*
 * ADCS.c
 *
 *  Created on: 06-Sep-2025
 *      Author: Jawahar Arumugam
 */



/************Module Description: Additive Delivery CS Module************************
 *
 * This module is responsible for delivering additives to the Digestion chamber
 * The logic used for additive delivery is as follows and this logic is temporary
 * till AI module is ready. When AI module is ready, the intelligence from AI module
 * will be used in the logic
 *  - Fixed amount of additive will be added when wet waste is dropped into Chewie
 *  - Additional constraint is additive will not be added more than once per hour
 *  - Shredder module sends an event to indicate the waste addition to this module
 ************************************************************************************/

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

/* Chewie Includes */
#include "dgCommon.h"
#include "version.h"
#include "modulecom.h"
#include "dgtimer.h"
#include "GPIOSignals.h"
#include "seqControlCommon.h"
#include "dgI2cDriver.h"
#include "rtc.h"
#include "ASB_HMI_common.h"
#include "sysStart.h"
#include "dgUartDriverCommon.h"
#include "CliUartDriver.h"
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
#include "csmMod.h"
#include "measure.h"


/*******************************************************************************
 * ADCS configuration parameters
 ******************************************************************************/

#define QUIET_PERIOD_BETWEEN_ADDITIVE_DELIVERY  30 			// in minutes
#define AMOUNT_OF_ADDITIVE_PER_DELIVERY			6			// in grams
#define AMOUNT_OF_ADDITIVE_DELIVERED_PER_MIN	1			// in grams/min - property of additive delivery part
#define ADDITIVE_DELIVERY_MIN_DUR				1			// in seconds
#define ADDITIVE_DELIVERY_MAX_DUR				15			// in seconds

#define ADDITIVE_MOTOR_ON_DUR					2			// in minutes

/*******************************************************************************
 * Global Variables
 ******************************************************************************/

/*******************************************************************************
 * Implementation
 ******************************************************************************/


static void adcs_task(void *pvParameters)
{
	QueueHandle_t adcsQHandle;
	dgMsg_t rcvMsg;				//Holds the currently received message
	static uint8_t adcsState;		//
	static uint8_t additiveMotorOnDur;  //in seconds
	dgAdcsConfigParams_t adcsConfig;

	//Compute Additive motor ON duration based on the amount of additives to be delivered
	//additiveMotorOnDur = AMOUNT_OF_ADDITIVE_PER_DELIVERY/AMOUNT_OF_ADDITIVE_DELIVERED_PER_MIN;
	//Validate the computed value and cap if beyond limit
	//additiveMotorOnDur = (additiveMotorOnDur < ADDITIVE_DELIVERY_MIN_DUR) ? ADDITIVE_DELIVERY_MIN_DUR:additiveMotorOnDur;
	//additiveMotorOnDur = (additiveMotorOnDur > ADDITIVE_DELIVERY_MAX_DUR) ? ADDITIVE_DELIVERY_MAX_DUR:additiveMotorOnDur;

	//additiveMotorOnDur = ADDITIVE_MOTOR_ON_DUR;

	//Wait till module registration is complete
	adcsQHandle = NULL;
	while(adcsQHandle== NULL)
	{
		vTaskDelay(100 / portTICK_PERIOD_MS);
		adcsQHandle = getQHandle(ADCS_MOD);
	}

	getAdcsParams(&adcsConfig);
	printf("adcs.c:adcs_task():additive config: Quite period=%d, Additive delivery in gm= %d, additive per min=%d\r\n", adcsConfig.adcsQuietPeriod, adcsConfig.additivePerDelivery, adcsConfig.additivePerMinute);
	additiveMotorOnDur = (adcsConfig.additivePerDelivery * 60)/adcsConfig.additivePerMinute;  //in seconds
	additiveMotorOnDur = (additiveMotorOnDur < ADDITIVE_DELIVERY_MIN_DUR) ? ADDITIVE_DELIVERY_MIN_DUR:additiveMotorOnDur;
	additiveMotorOnDur = (additiveMotorOnDur > ADDITIVE_DELIVERY_MAX_DUR) ? ADDITIVE_DELIVERY_MAX_DUR:additiveMotorOnDur;


	printf("adcs.c:adcs_task():additive motor on dur in seconds: %d\r\n", additiveMotorOnDur);

	getAdcsState(&adcsState);

	if(adcsState == ADCS_STATE_QUIET_PERIOD)
	{
		dgtimerStart(ADCS_MOD, CONV_SEC_TO_TICKS((adcsConfig.adcsQuietPeriod)*60));
	}
	else
	{
		//DELIVERING state is considered as IDLE
		adcsState = ADCS_STATE_IDLE;
	}



	while(1)
	{
		//Receive event from Queue. Block until event is available
		if(xQueueReceive(adcsQHandle, &rcvMsg, portMAX_DELAY ) != pdPASS )
		{
			// Queue did not return an event. Hence go back
			continue;
		}
		//printf("ADCS.c:adcs_task():cmd rcvd %d in state %d\r\n",rcvMsg.command,adcsState);
		switch(rcvMsg.command)
		{
			case ADCS_START:
				printf("ADCS.c:adcs_task(): Start cmd received in state %d\r\n",adcsState );
				switch(adcsState)
				{
				case ADCS_STATE_IDLE:
					//start Additive delivery motor
					additiveDispenseOn();
					//change state to delivering
					adcsState = ADCS_STATE_DELIVERING;
					//Update RTC RAM
					updateAdcsState(adcsState);
					//start a timer
					dgtimerStart(ADCS_MOD, CONV_SEC_TO_TICKS(additiveMotorOnDur));
					break;
				case ADCS_STATE_DELIVERING:
					//Ignore any waste addition event
					break;
				case ADCS_STATE_QUIET_PERIOD:
					//Ignore any waste addition event during quite period
					break;
				default:
					printf("ADCS.c:adcs_task(): Invalid state:%d\r\n",adcsState);
					break;
				}
				break;

				case ADCS_ABORT:
					printf("ADCS.c:adcs_task(): Abort cmd received in state %d\r\n",adcsState );
					switch(adcsState)
					{
					case ADCS_STATE_IDLE:
						//Ignore
						break;
					case ADCS_STATE_DELIVERING:
						//Stope delivering additive
						additiveDispenseOff();
						//change state to Idle
						adcsState = ADCS_STATE_IDLE;
						updateAdcsState(adcsState);
						//stop timer
						dgtimerStop(ADCS_MOD);
						break;
					case ADCS_STATE_QUIET_PERIOD:
						//change state to Idle
						adcsState = ADCS_STATE_IDLE;
						updateAdcsState(adcsState);
						//stop timer
						dgtimerStop(ADCS_MOD);
						break;
					default:
						printf("ADCS.c:adcs_task(): Invalid state:%d\r\n",adcsState);
						break;
					}
					break;

			case DG_TIMER_EXPIRY:
				switch(adcsState)
				{
				case ADCS_STATE_IDLE:
					//Timer event not expected in this state. Ignore
					//Stop timer
					dgtimerStop(ADCS_MOD);
					break;

				case ADCS_STATE_DELIVERING:
					//Delivery duration is over. Stop the delivery motor
					additiveDispenseOff();
					//change state to quite period
					adcsState = ADCS_STATE_QUIET_PERIOD;
					updateAdcsState(adcsState);
					//start quite period timer
					dgtimerStart(ADCS_MOD, CONV_SEC_TO_TICKS(QUIET_PERIOD_BETWEEN_ADDITIVE_DELIVERY*60));
					break;
				case ADCS_STATE_QUIET_PERIOD:
					//Quite period is over. Change state to IDLE
					adcsState = ADCS_STATE_IDLE;
					updateAdcsState(adcsState);
					break;
				default:
					printf("ADCS.c:adcs_task(): Invalid state:%d\r\n",adcsState);
					break;
				}
				break;

			default:
				//Invalid command or event ignore
				printf("ADCS.c:adcs_task():Invalid command %d received)\r\n",rcvMsg.command );
				break;
		}
	}

}

/***********************sendWasteAddEvent To Adcs()******************************
 * This API can be used to send waste added event to ADCS module
 *
 *****************************************************************************/


int adcsStart(uint8_t srcModule)
{
	dgMsg_t sendMsgBuf;
	uint8_t result;

	result = DG_FAIL;

	//Populate the message to send to the modbus task
	sendMsgBuf.src_module = srcModule;
	sendMsgBuf.command = ADCS_START;
	sendMsgBuf.cmdParam = NULL;
	sendMsgBuf.dest_module = ADCS_MOD;
	sendMsgBuf.result = &result;
	sendMsgBuf.taskHandleSM = NULL;


	if(sendMsg(&sendMsgBuf) != DG_SUCCESS)
	{
		PRINTF("ADCS.c:adcsSendWasteAddEvent():Message send failed \r\n" );
		return DG_FAIL;
	}

	return DG_SUCCESS;

}

int adcsAbort(uint8_t srcModule)
{
	dgMsg_t sendMsgBuf;
	uint8_t result;

	result = DG_FAIL;

	//Populate the message to send to the modbus task
	sendMsgBuf.src_module = srcModule;
	sendMsgBuf.command = ADCS_ABORT;
	sendMsgBuf.cmdParam = NULL;
	sendMsgBuf.dest_module = ADCS_MOD;
	sendMsgBuf.result = &result;
	sendMsgBuf.taskHandleSM = NULL;


	if(sendMsg(&sendMsgBuf) != DG_SUCCESS)
	{
		PRINTF("ADCS.c:adcsAbort():Message send failed \r\n" );
		return DG_FAIL;
	}

	return DG_SUCCESS;

}

int initAdcs(void)
{
	//Create ADCS task
	TaskHandle_t adcsTaskHandle;
	TimerHandle_t adcsTimerHandle;
	BaseType_t result;

	result = xTaskCreate(adcs_task, "adcs_task", configMINIMAL_STACK_SIZE + 100, NULL, task_PRIORITY, &adcsTaskHandle);
    if ( result !=    pdPASS)
    {
        printf("ADCS.c:initAdcs(): Task creation failed!.\r\n");
        return DG_FAIL;
    }
    //ADCS requires timer and hence create FreeRTOS single shot SW timer
    adcsTimerHandle = xTimerCreate("adcsTimer",100 /*DEFAULT DUR*/, pdFALSE, (void*)ADCS_MOD, dgTimerCallback);
    if(adcsTimerHandle == NULL)
    {
        printf("ADCS.c:initAdcs():Timer creation failed!.\r\n");
    	vTaskDelete(adcsTaskHandle);
        return DG_FAIL;
    }
    if(registerModule(ADCS_MOD, adcsTaskHandle, adcsTimerHandle)!= DG_SUCCESS)
    {
    	//Registering the module failed. Hence kill the task and return error
        printf("ADCS.c:initAdcs(): Task registration failed!.\r\n");
        xTimerDelete(adcsTimerHandle,100 / portTICK_PERIOD_MS);
    	vTaskDelete(adcsTaskHandle);

    	return DG_FAIL;
    }
    return DG_SUCCESS;
}





