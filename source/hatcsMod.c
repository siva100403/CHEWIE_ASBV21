/*
 * hatcsMod.c
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


/********************************************************************
 * Variables and constants											*
 ********************************************************************/

//Defualt values for process variables
#define HATCS_DEFAULT_TEMPTARGET			40.0
#define HATCS_DEFAULT_TEMPRANGE				2.0
#define HATCS_DEFAULT_HUMIDITYTARGET		80.0
#define HATCS_DEFAULT_HUMIDITYRANGE			5.0


TimerHandle_t sprayerTimerHandle;


#define DEFAULT_SPRAYER_DURATION 	1   //in seconds

/********************************************************************
 * Implementation													*
 ********************************************************************/


int sprayerTimerStart(TickType_t timeoutValue)
{

	if(xTimerChangePeriod(sprayerTimerHandle,timeoutValue, DGTIMER_BLOCKTIME )== pdFALSE)
	{
		PRINTF("hatcsMod.c:sprayerTimerStart():Change period failed\r\n");
		return DG_FAIL;
	}
	if(xTimerStart(sprayerTimerHandle,DGTIMER_BLOCKTIME) == pdFALSE)
	{
		PRINTF("hacsMod.c:sprayerTimerStart():Start failed\r\n");
		return DG_FAIL;
	}
	return DG_SUCCESS;
}

void sprayerTimerCallback( TimerHandle_t xTimer)
{
	//Turn of sprayer relay
	DCsprayerOff();
}

int sprayerTimerInit()
{
	//Sprayer relay requires a timer to stop the relay
	sprayerTimerHandle = xTimerCreate("sprayerTimer",100, pdFALSE, NULL, sprayerTimerCallback);
	if(sprayerTimerHandle == NULL)
	{
		printf("Sprayer Timer creation failed!.\r\n");
		return DG_FAIL;
	}
	return DG_SUCCESS;
}

int sprayOnceDC(uint8_t duration)
{
	//Turn ON sprayer for Digestion Chamber
	DCsprayerOn();
	//Set timer for turn-off
	sprayerTimerStart(CONV_SEC_TO_TICKS(duration));
	return DG_SUCCESS;
}

int executeActuatorControl(uint8_t hatSensorState, uint8_t stateChange)
{
	static dgActuatorCtrlCode_t  controlCode[MAX_CTRL_CODE_PER_SEQ];

	static uint8_t curHatSensorState;
	static uint8_t controlSeqIndex=0;

	printf("hatcsMod.c:executeActuatorControl():sensor state %d\r\n",hatSensorState);

	if(stateChange == DG_BOOL_TRUE)
	{
		//State change occurred. Load the control sequence corresponding to the new state
		//Validate parameter
		if(hatSensorState > HAT_SENSOR_TEMP_BR_HUM_BR)
		{
			printf("hatcsMod.c:executeActuatorControl():Invalid sensorstate param\r\n");
			return DG_INVALID_PARAM;
		}
		curHatSensorState = hatSensorState;

		//Load control sequence from EEPROM
		if(loadActuatorSeq(&controlCode[0], curHatSensorState) != DG_SUCCESS)
		{
			printf("hatcsMod.c:executeActuatorControl():Config read from EEPROM failed\r\n");
			return DG_FAIL;
		}
		printf("hatcsMod.c:executeActuatorControl():Sensor status changed to %d\r\n",curHatSensorState);
		controlSeqIndex = 0;
	}
	//Execute control sequences
	//validate index
	if(controlSeqIndex >= MAX_CTRL_CODE_PER_SEQ)
	{
		controlSeqIndex=0;  //In order to avoid memory boundary issue
	}
	//Control CT
	if(controlCode[controlSeqIndex].ctCtrl == SEQ_CTRL_CTCS_ON)
	{
		printf("hatcsMod.c:executeActuatorControl():CTCS_START\r\n");
		ctStart(HATCS_MOD);
	}
	else
	{
		printf("hatcsMod.c:executeActuatorControl():CTCS_STOP\r\n");
		ctStop(HATCS_MOD);
	}
	//Control Heater
	if(controlCode[controlSeqIndex].heaterCtrl == SEQ_CTRL_HTR_ON)
	{
		printf("hatcsMod.c:executeActuatorControl():HEATER_ON\r\n");
		HEATER_ON();
	}
	else
	{
		printf("hatcsMod.c:executeActuatorControl():HEATER_OFF\r\n");
		HEATER_OFF();
	}

	//Fan Control
	switch(controlCode[controlSeqIndex].fanCtrl)
	{
		case SEQ_CTRL_FAN_OFF:
			fanMotorStop();
			printf("hatcsMod.c:executeActuatorControl():FAN_OFF\r\n");
			break;
		case SEQ_CTRL_FAN_CWR:
			fanMotorCWR();
			printf("hatcsMod.c:executeActuatorControl():FAN_CWR\r\n");
			break;
		case SEQ_CTRL_FAN_CCWR:
			fanMotorCCWR();
			printf("hatcsMod.c:executeActuatorControl():FAN_CCWR\r\n");
			break;
		default:
			break;
	}
	//Air Circulation Control
	//Solenoid mapping
	//AIR_VALVE_1  -> S1
	//AIR_VALVE_2  -> S2
	//AIR_VALVE_3  -> S3
	switch(controlCode[controlSeqIndex].airCircCtrl)
	{
		case SEQ_CTRL_AIR_OFF:
			airValve1Off();
			airValve3Off();
			printf("hatcsMod.c:executeActuatorControl():Air Control OFF\r\n");
			break;
		case SEQ_CTRL_AIR_IN:
			airValve1On();
			airValve3Off();
			printf("hatcsMod.c:executeActuatorControl():AIR_IN\r\n");
			break;
		case SEQ_CTRL_AIR_OUT:
			airValve1On();
			airValve3On();
			printf("hatcsMod.c:executeActuatorControl():AIR_OUT\r\n");
			break;
		case SEQ_CTRL_AIR_RECIRC:
			airValve1Off();
			airValve3On();
			printf("hatcsMod.c:executeActuatorControl():AIR_RECIRC\r\n");
			break;
		default:
			break;
	}

	//Control Sprayer
	if(controlCode[controlSeqIndex].sprayerCtrl == SEQ_CTRL_SPRAYER_ONCE)
	{
		printf("hatcsMod.c:executeActuatorControl():SPRAY_ONCE\r\n");
		sprayOnceDC(DEFAULT_SPRAYER_DURATION);
	}
	//Set timer
	dgtimerStart(HATCS_MOD, CONV_SEC_TO_TICKS(controlCode[controlSeqIndex].duration*60));
	printf("hatcsMod.c:executeActuatorControl():Duration %d\r\n",controlCode[controlSeqIndex].duration);
	//Check if this is the last control in the sequence
	if(controlCode[controlSeqIndex].ctrlSeqEnd == SEQ_CTRL_END)
	{
		controlSeqIndex = 0;   //Reset the Index to start of control sequence
	}
	else
	{
		controlSeqIndex++;
	}
	return DG_SUCCESS;
}

int executeActuatorSafeState(void)
{
	//Stop CT
	ctStop(HATCS_MOD);
	//Stop Heater
	HEATER_OFF();
	//Stop Fan
	fanMotorStop();
	DCsprayerOff();

	return DG_SUCCESS;
}


static void hatcs_task(void *pvParameters)
{
	QueueHandle_t hatcsQHandle;
	dgMsg_t rcvMsg;							//Holds the currently received message
	static int hatcsState;					// This stores the state of HAT control system
	static int hatSensorState;				//This variable stores the Temp/humidity sensor state


	//printf("THCs.c:thcs_task():started\r\n");

	//Get the Qhandle for this task and store locally
	hatcsQHandle = getQHandle(HATCS_MOD);

	//Initialize the state of controller to IDLE
	hatcsState = HATCS_STATE_IDLE;
	hatSensorState = HAT_SENSOR_TEMP_WR_HUM_WR;

	sprayerTimerInit();


	while(1)
	{
		//Receive event from Queue. Block until event is available
		if(xQueueReceive(hatcsQHandle, &rcvMsg, portMAX_DELAY ) != pdPASS )
		{
			// Queue did not return an event. Hence go back
			continue;
		}
		printf("hatcsMod.c:hatcs_task():Rcd cmd=%d, in hatcs state=%d and sensorState %d\r\n", rcvMsg.command,hatcsState, hatSensorState);
		switch(rcvMsg.command)
		{
			case HATCS_NOTIFY_SENSOR_STATE_CHANGE:
				uint8_t newSensorState;

				//Change in sensor state notification received
				//Get the new sensor status from the command
				newSensorState = *((uint8_t*)rcvMsg.cmdParam);
				if(newSensorState <= HAT_SENSOR_TEMP_BR_HUM_BR)
				{
					hatSensorState = newSensorState;
				}

				//Execute Actuator Control for the new sensor state
				executeActuatorControl(hatSensorState, DG_BOOL_TRUE);

				if(rcvMsg.taskHandleSM != NULL)
				{
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}

				break;

			case HATCS_START:
				//printf("THCs.c:thcs_task(): THCS_START event  received\r\n");
				if(hatcsState == HATCS_STATE_IDLE)
				{
					//Change state to
					hatcsState = HATCS_STATE_ACTIVE;
					//hatSensorState = HAT_SENSOR_TEMP_WR_HUM_WR;
					//Execute Actuator Control for TWR_HWR state
					executeActuatorControl(hatSensorState, DG_BOOL_TRUE);
					*rcvMsg.result = DG_SUCCESS;
				}
				else
				{
					//printf("hatcsMod.c:hatcs_task():THCS_START event received in state %d; ignored\r\n", thcsState);
					*rcvMsg.result = DG_FAIL;
				}

				if(rcvMsg.taskHandleSM != NULL)
				{
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				break;

			case HATCS_STOP:
				//printf("THCs.c:thcs_task(): THCS_STOP event  received)\r\n");
				switch(hatcsState)
				{
				case HATCS_STATE_IDLE:
					//No need to do anything. Already in IDLE
					break;
				case HATCS_STATE_ACTIVE:
					hatcsState = HATCS_STATE_IDLE;
					dgtimerStop(HATCS_MOD);
					executeActuatorSafeState();
					break;
				case HATCS_STATE_ERROR:
					hatcsState = HATCS_STATE_IDLE;
					hatSensorState = HAT_SENSOR_TEMP_WR_HUM_WR;
					break;
				default:
					break;
				}
				hatcsState = HATCS_STATE_IDLE;

				dgtimerStop(HATCS_MOD);
				*rcvMsg.result = DG_SUCCESS;
				if(rcvMsg.taskHandleSM != NULL)
				{
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				break;

			case DG_TIMER_EXPIRY:
				switch (hatcsState)
				{
				case HATCS_STATE_IDLE:
					//Do nothing
					break;

				case HATCS_STATE_ACTIVE:
					executeActuatorControl(hatSensorState, DG_BOOL_FALSE);
					break;

				case HATCS_STATE_ERROR:
					break;

				default:
					break;
				}
				break;

			default:
				//Invalid command or event ids
				//printf("tempCtrl.c:tempctrl_task()::Invalid command %d received)\r\n",rcvMsg.command );
				break;
		}
	}

}


int initHatcs(void)
{
	//Create Temperature and humidity  Control task
	TaskHandle_t hatcsTaskHandle;
	TimerHandle_t hatcsTimerHandle;

    if (xTaskCreate(hatcs_task, "hatcs_task", configMINIMAL_STACK_SIZE + 100, NULL, task_PRIORITY, &hatcsTaskHandle) !=
        pdPASS)
    {
        printf("hatcsMod.c:initHatcs():Task creation failed!.\r\n");
        return DG_FAIL;
    }
    //Temperature Control requires timer and hence create FreeRTOS SW timer
    hatcsTimerHandle = xTimerCreate("hatcsTimer", 100, pdFALSE, (void*)HATCS_MOD, dgTimerCallback);
    if(hatcsTimerHandle == NULL)
    {
        printf("hatcs.c:initHatcs():Timer creation failed!.\r\n");
    	vTaskDelete(hatcsTaskHandle);
        return DG_FAIL;
    }
    if(registerModule(HATCS_MOD, hatcsTaskHandle, hatcsTimerHandle)!= DG_SUCCESS)
    {
    	//Registering the module failed. Hence kill the task and return error
        //printf("THCs.c:initThcs(): Task registration failed!.\r\n");
        xTimerDelete(hatcsTimerHandle,100 / portTICK_PERIOD_MS);
    	vTaskDelete(hatcsTaskHandle);

    	return DG_FAIL;
    }
    return DG_SUCCESS;
}

