/*
 * sensorMod.c
 *
 *  Created on: 02-Jan-2025
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
#define TH_AVERAGE_SIZE		4					//Should be power of 2
#define TH_AVERAGE_MODULO	TH_AVERAGE_SIZE-1
#define WITHIN_RANGE		0  //Value within range
#define ABOVE_RANGE			1	//Value Above Range
#define BELOW_RANGE			2 	//Value Below Range



float curTemp, curHumidity, avgTemp, avgHumidity;


//This method read the Temperature and Humidity values from sensor
//Compares them with respective thresholds and decides the next THCS state

int computeSensorStatus(dgSensorThreshold_t *threshold, uint8_t *sensorStatus)
{


//	float curTemp, curHumidity, avgTemp, avgHumidity;
	static int sensorReadFailCount=0;
	static float tempArray[TH_AVERAGE_SIZE];
	static float humidityArray[TH_AVERAGE_SIZE];
	static uint8_t writePtr=0;
	static uint8_t prevTempState=WITHIN_RANGE, prevHumidityState=WITHIN_RANGE;
	static uint8_t sensorArrayFillCount=0;
	uint8_t count;
	uint8_t curTempState, curHumidityState;



	//Read new values from temperature and humidity sensors
	if(readShtTempHumidityHighPrecision(&curTemp, &curHumidity) != DG_SUCCESS)
	{
		//Sensor read failure and hence cannot modify the sensor status
		sensorReadFailCount++;
		//Sensor readFailCount is more than certain value declare sensor is issue
		//           *TODO*
		return DG_FAIL;
	}
	else
	{
		//Sensor read passed
		sensorReadFailCount--;
		if(sensorReadFailCount <0) {sensorReadFailCount =0;}
		//Validate sensor value by checking subsequent readings are within, say 10%
		//  *TODO*

		//Store sensor values in the array
		tempArray[writePtr] = curTemp;
		humidityArray[writePtr] = curHumidity;
		writePtr++;
		writePtr &= TH_AVERAGE_MODULO;  //To make the pointer wraparound

		if(sensorArrayFillCount < 3)  //Wait for the sensor Array to fill
		{
			sensorArrayFillCount++;
			return DG_INPROGRESS;
		}


		//Calculate the average value of temp and Humidity
		avgTemp = 0; avgHumidity=0;
		for(count = 0; count<TH_AVERAGE_SIZE; count++)
		{
			avgTemp += tempArray[count];
			avgHumidity += humidityArray[count];
		}
		avgTemp = avgTemp/4.0;
		avgHumidity = avgHumidity/4.0;
		//printf("sensorMod.c:computeSensorStatus():temp=%f, hum=%f\r\n",avgTemp, avgHumidity);
		//Compute the temp range
		switch(prevTempState)
		{
		case WITHIN_RANGE:
			if(avgTemp > threshold->targetTempHigh)
			{
				curTempState = ABOVE_RANGE;
			}
			else if(avgTemp < threshold->targetTempLow)
			{
				curTempState = BELOW_RANGE;
			}
			else
			{
				curTempState = WITHIN_RANGE;
			}
			break;

		case ABOVE_RANGE:
			if(avgTemp < threshold->targetTempLow)
			{
				curTempState = BELOW_RANGE;
			}
			else if(avgTemp < threshold->targetTemp)
			{
				curTempState = WITHIN_RANGE;
			}
			else
			{
				curTempState = ABOVE_RANGE;
			}
			break;

		case BELOW_RANGE:
			if(avgTemp > threshold->targetTempHigh)
			{
				curTempState = ABOVE_RANGE;
			}
			else if(avgTemp > threshold->targetTemp)
			{
				curTempState = WITHIN_RANGE;
			}
			else
			{
				curTempState = BELOW_RANGE;
			}
			break;
		default:
			//Code should not reach here. Some error. Set default values
			printf("THCs.c:computeSensorStatus():Invalid value for prevTempState\r\n");
			prevTempState = WITHIN_RANGE;
			curTempState = WITHIN_RANGE;
			break;
		}

		//Compute the humidity range
		switch(prevHumidityState)
		{
		case WITHIN_RANGE:
			if(avgHumidity > threshold->targetHumidityHigh)
			{
				curHumidityState = ABOVE_RANGE;
			}
			else if(avgHumidity < threshold->targetHumidityLow)
			{
				curHumidityState = BELOW_RANGE;
			}
			else
			{
				curHumidityState = WITHIN_RANGE;
			}
			break;

		case ABOVE_RANGE:
			if(avgHumidity < threshold->targetHumidityLow)
			{
				curHumidityState = BELOW_RANGE;
			}
			else if(avgHumidity < threshold->targetHumidity)
			{
				curHumidityState = WITHIN_RANGE;
			}
			else
			{
				curHumidityState = ABOVE_RANGE;
			}
			break;

		case BELOW_RANGE:
			if(avgHumidity > threshold->targetHumidityHigh)
			{
				curHumidityState = ABOVE_RANGE;
			}
			else if(avgHumidity > threshold->targetHumidity)
			{
				curHumidityState = WITHIN_RANGE;
			}
			else
			{
				curHumidityState = BELOW_RANGE;
			}
			break;
		default:
			//Code should not reach here. Some error. Set default values
			printf("THCs.c:computeSensorStatus():Invalid value for prevTempState\r\n");
			prevHumidityState = WITHIN_RANGE;
			curHumidityState = WITHIN_RANGE;
			break;
		}
	}
	//THCS state should not change too frequently. Add check here
	//   *TODO*

	//Convert Individual states to combined state which is same format as THCS_STATE
	*sensorStatus = curTempState*3 + curHumidityState;
	prevTempState = curTempState;
	prevHumidityState = curHumidityState;
	return DG_SUCCESS;
}


void sensor_Task(void* arg)
{

	QueueHandle_t sensorQHandle;
	dgMsg_t rcvMsg;						// Holds the currently received message
	uint8_t sensorModState;
	//Get the Qhandle for this task and store locally
	sensorQHandle = getQHandle(SENSOR_MOD);
	sensorModState = SENSOR_STATE_IDLE;
	dgSensorThreshold_t threshold;    	//To store sensor threshold values
	uint8_t curSensorStatus;


	curSensorStatus = HAT_SENSOR_TEMP_WR_HUM_WR;

	while (1)
	{
		//Receive event from Queue. Block until event is available
		if(xQueueReceive(sensorQHandle, &rcvMsg, portMAX_DELAY ) != pdPASS )
		{
			// Queue did not return an event. Hence go back
			continue;
		}
		//printf("sensorMod.c:sensor_Task():rcvd command %d in state %d\r\n",rcvMsg.command, sensorModState);
		switch(rcvMsg.command)
		{
		case SENSOR_SET_PARAM:
			dgSensorParam_t *configparams;

			printf("sensorMod.c:sensor_Task():rcvd SET_PARAM in state %d\r\n", sensorModState);
			//Check the current state of Modbus before accepting the command
			if(sensorModState == SENSOR_STATE_ACTIVE)
			{
				//printf("THCs.c:thcs_task():THCS:Busy)\r\n");
				//Some command is already active. Return the command with busy
				*rcvMsg.result = DG_BUSY;
			}
			else
			{
				//Parameter validation to be done    *TODO*
				configparams = (dgSensorParam_t*)rcvMsg.cmdParam;
				threshold.targetTemp = configparams->tempSetValue;
				threshold.targetHumidity = configparams->humiditySetValue;

				threshold.targetTempLow = threshold.targetTemp - configparams->tempRange;
				threshold.targetTempHigh = threshold.targetTemp + configparams->tempRange;
				threshold.targetHumidityLow = threshold.targetHumidity - configparams->humidityRange;
				threshold.targetHumidityHigh = threshold.targetHumidity + configparams->humidityRange;
				sensorModState = SENSOR_STATE_READY;
				*rcvMsg.result = DG_SUCCESS;
			}

			if(rcvMsg.taskHandleSM != NULL)
			{
				xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
			}
			break;

		case GET_SENSOR_STATUS:
			dgSensorModStatus_t *statusStruct;

			printf("sensorMod.c:sensor_Task():rcvd GET_SENSOR_STATUS in state %d\r\n", sensorModState);
			statusStruct = (dgSensorModStatus_t*)rcvMsg.cmdParam;

			statusStruct->sensorModStatus = sensorModState;
			statusStruct->sensorStatus = curSensorStatus;
			statusStruct->avgHumidity = avgHumidity;
			statusStruct->avgTemp = avgTemp;
			statusStruct->curHumidity = curHumidity;
			statusStruct->curTemp = curTemp;
			statusStruct->setTemp = threshold.targetTemp;
			statusStruct->setHumidity = threshold.targetHumidity;
			*rcvMsg.result = DG_SUCCESS;


			if(rcvMsg.taskHandleSM != NULL)
			{
				xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
			}
			break;

		case SENSOR_START:
			printf("sensorMod.c:sensor_Task():rcvd SENSOR_START in state %d\r\n", sensorModState);
			switch(sensorModState)
			{
			case SENSOR_STATE_IDLE:
				//IDLE state indicates the parameters have not been initialized
				//Cannot accept start at this state. Return error
				*rcvMsg.result = DG_INVALID_STATE;
				if(rcvMsg.taskHandleSM != NULL)
				{
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				break;
			case SENSOR_STATE_READY:
				//Valid state for start command. Change to ACTIVE state
				sensorModState = SENSOR_STATE_ACTIVE;
				//Start timer
				dgtimerStart(SENSOR_MOD, CONV_SEC_TO_TICKS(SENSOR_SAMPLING_TIME));
				*rcvMsg.result = DG_SUCCESS;
				if(rcvMsg.taskHandleSM != NULL)
				{
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				break;
			case SENSOR_STATE_ACTIVE:
				//Already sensor is active. Return success
				*rcvMsg.result = DG_SUCCESS;
				if(rcvMsg.taskHandleSM != NULL)
				{
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				break;
			default:
				break;
			}
			break;

		case SENSOR_STOP:
			printf("sensorMod.c:sensor_Task():rcvd SENSOR_STOP in state %d\r\n", sensorModState);
			switch(sensorModState)
			{
			case SENSOR_STATE_IDLE:
				//Already in IDLE. Return INVALID State
				*rcvMsg.result = DG_INVALID_STATE;
				if(rcvMsg.taskHandleSM != NULL)
				{
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				break;
			case SENSOR_STATE_READY:
				//Already in Ready state. return success
				*rcvMsg.result = DG_SUCCESS;
				if(rcvMsg.taskHandleSM != NULL)
				{
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				break;
			case SENSOR_STATE_ACTIVE:
				//Stop timer and change to READY state
				dgtimerStop(SENSOR_MOD);
				sensorModState = SENSOR_STATE_READY;
				*rcvMsg.result = DG_SUCCESS;
				if(rcvMsg.taskHandleSM != NULL)
				{
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				break;
			default:
				break;
			}
			break;

		case DG_TIMER_EXPIRY:
			uint8_t status;
			switch(sensorModState)
			{
			case SENSOR_STATE_IDLE:
				//ignore. We don't expect timer event idle
				break;

			case SENSOR_STATE_ACTIVE:
				//Take sensor sample and process
				//printf("sensorMod.c:sensor_Task():received timer event\r\n");
				if(computeSensorStatus(&threshold, &status) != DG_SUCCESS)
				{
					printf("sensorMod.c:sensor_Task():ComputeSensorStatus error\r\n");
					break;
				}
				//Check whether there is a change in status
				if(curSensorStatus != status)
				{
					//change in sensor state. Notify HATCS module
					hatcsNotifySensorState(SENSOR_MOD, status);
					printf("sensorMod.c:sensor_Task():Sensor State Changed to %d\r\n", status);
					//Set new state to current state
					curSensorStatus = status;
				}
				break;

			case SENSOR_STATE_READY:
				//In error state. we don't expect timer event. Ignore
				break;

			default:
				break;
			}
			break;

		default:
			break;
		}
	}
}




/******************************* initSensorMod() ************************************************
*Description: This function initializes sensor module. Create queue, task and  					*
* initialized the data structures. This should be called at the time of power up initialization.	*				*
* It does the following																			*
* 																								*
*	- Creates a Queue handle for receiving event messages from other modules					*
* Command Parameters: None																		*
* Return Values:																				*
*	- DG_SUCCESS: When initialization is successful												*
*	- SF_FAIL: When Queue allocation failed														*
*************************************************************************************************/

int initSensor(void)
{
	//Create Sensor Module task
	TaskHandle_t sensorTaskHandle;
	TimerHandle_t sensorTimerHandle;


    if (xTaskCreate(sensor_Task, "sensor_Task", configMINIMAL_STACK_SIZE + 100, NULL, task_PRIORITY, &sensorTaskHandle) !=
        pdPASS)
    {
        printf("Sensor Task creation failed!.\r\n");
        return DG_FAIL;
    }
    //Sensor module requires timer and hence create FreeRTOS SW timer
    sensorTimerHandle = xTimerCreate("sensorTimer", CONV_SEC_TO_TICKS(SENSOR_SAMPLING_TIME), pdTRUE, (void*)SENSOR_MOD, dgTimerCallback);
    if(sensorTimerHandle == NULL)
    {
        printf("Timer creation failed!.\r\n");
    	vTaskDelete(sensorTaskHandle);
        return DG_FAIL;
    }
    if(registerModule(SENSOR_MOD, sensorTaskHandle, sensorTimerHandle)!= DG_SUCCESS)
    {
    	//Registering the module failed. Hence kill the task and return error
        printf("SENSOR Task registration failed!.\r\n");
        xTimerDelete(sensorTimerHandle,100 / portTICK_PERIOD_MS);
    	vTaskDelete(sensorTaskHandle);

    	return DG_FAIL;
    }
    return DG_SUCCESS;
}
