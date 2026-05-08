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
#include "actuatorStatusTracker.h"


/********************************************************************
 * Variables and constants											*
 ********************************************************************/

//Defualt values for process variables
#define HATCS_DEFAULT_TEMPTARGET			40.0
#define HATCS_DEFAULT_TEMPRANGE				2.0
#define HATCS_DEFAULT_HUMIDITYTARGET		80.0
#define HATCS_DEFAULT_HUMIDITYRANGE			5.0


TimerHandle_t sprayerTimerHandle;
bool mkValveMovingFlag;




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

int sprayOnceDCmSec(uint16_t duration)
{
	//Turn ON sprayer for Digestion Chamber
	DCsprayerOn();
	//Set timer for turn-off
	sprayerTimerStart(CONV_MSEC_TO_TICKS(duration));
	return DG_SUCCESS;
}


/*---------------------- mKValve Operation -------------------------*/
/* MK Valve assumed to be at AIR_RECIRC position when Chewie is started.
 * Currently there is no limit switch to sense the position.
 * MK Valve motor is 2 RPM.
 * AIR_RECIRC position and AIR_OUT position are 90 deg away
 * MK Valve motor will be rotated 45 deg to move between AIR_CIRC position
 *  and AIR_OUT position.
 * To move from AIR_RECIRC position to AIR_OUT position motor has to be rotated
 *  in CCWR
 */


#define AIR_RECIRC		0
#define AIR_OUT			1

#define MKVALVE_CWR_DURATION		4700 	//Specified in mSec
#define MKVALVE_CCWR_DURATION		4700  	//Specified in mSec

uint8_t mkValveStatus;
TimerHandle_t mkValveTimerHandle;


int mkValveTimerStart(TickType_t timeoutValue)
{

	if(xTimerChangePeriod(mkValveTimerHandle,timeoutValue, DGTIMER_BLOCKTIME )== pdFALSE)
	{
		printf("hatcsMod.c:mkValveTimerStart():Change period failed\r\n");
		return DG_FAIL;
	}
	if(xTimerStart(mkValveTimerHandle,DGTIMER_BLOCKTIME) == pdFALSE)
	{
		printf("hacsMod.c:mkValveTimerStart():Start failed\r\n");
		return DG_FAIL;
	}
	return DG_SUCCESS;
}

void mkValveTimerCallback( TimerHandle_t xTimer)
{
	//Turn of mkValve
	mkValveStop();
	mkValveMovingFlag = false;
}

int mkValveTimerInit()
{
	//mkValve requires a timer to stop
	mkValveTimerHandle = xTimerCreate("makValveTimer",100, pdFALSE, NULL, mkValveTimerCallback);
	if(mkValveTimerHandle == NULL)
	{
		printf("mkValve Timer creation failed!.\r\n");
		return DG_FAIL;
	}
	return DG_SUCCESS;
}

int setMkValveStatus(uint8_t status)
{
	uint8_t waitCount;
	//Wait for any mkValve operation to complete
	waitCount = 0;
	while(mkValveMovingFlag == true)
	{
		vTaskDelay(200);
		waitCount++;
		if(waitCount > 5)
		{
			printf("hatcsMod.c:setMKValveStatus(): Valve movingflag error\r\n");
			return DG_FAIL;
		}
	}
	switch(status)
	{
	case AIR_RECIRC:
		if(mkValveStatus != AIR_RECIRC){
			//start mkValve motor in CWR
			mkValveCCWR();
			mkValveMovingFlag = true;
			//start mkValve timer to stop
			mkValveTimerStart(CONV_MSEC_TO_TICKS(MKVALVE_CWR_DURATION));
			mkValveStatus = AIR_RECIRC;
		}
		break;
	case AIR_OUT:
		if(mkValveStatus != AIR_OUT){
			//start mkValve motor in CCWR
			mkValveCWR();
			mkValveMovingFlag = true;
			//start mkValve timer to stop
			mkValveTimerStart(CONV_MSEC_TO_TICKS(MKVALVE_CCWR_DURATION));
			mkValveStatus = AIR_OUT;
		}
		break;
	default:
		printf("Incorrect mkValve status\r\n");
		return DG_FAIL;
		break;
	}
	return DG_SUCCESS;
}



int executeActuatorControl(dgActuatorCtrlCode_t  *controlCode, uint8_t index)
{



	//Execute control sequences
	//validate index
	if(index >= MAX_CTRL_CODE_PER_SEQ)
	{
		index=0;  //In order to avoid memory boundary issue
	}
	//Control CT
	if(controlCode[index].ctCtrl == SEQ_CTRL_CTCS_ON)
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
	if(controlCode[index].heaterCtrl == SEQ_CTRL_HTR_ON)
	{
		printf("hatcsMod.c:executeActuatorControl():HEATER_ON\r\n");
		HEATER_ON();
		updateHeaterStatus(ON);
	}
	else
	{
		printf("hatcsMod.c:executeActuatorControl():HEATER_OFF\r\n");
		HEATER_OFF();
		updateHeaterStatus(OFF);
	}

	//Fan Control
	switch(controlCode[index].fanCtrl)
	{
		case SEQ_CTRL_FAN_OFF:
			fanMotorStop();
			hFanStop();
			printf("hatcsMod.c:executeActuatorControl():FAN_OFF\r\n");
			break;
		case SEQ_CTRL_FAN_CWR:
			fanMotorStop();
			if(controlCode[index].airCircCtrl == SEQ_CTRL_AIR_OUT)
			{
				hFanCWR(100);
			}
			else
			{
				hFanCWR(70);
			}

			//fanMotorCWR();
			printf("hatcsMod.c:executeActuatorControl():FAN_CWR\r\n");
			break;
		case SEQ_CTRL_FAN_CCWR:
			fanMotorCCWR();
			hFanStop();
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
	switch(controlCode[index].airCircCtrl)
	{
		case SEQ_CTRL_AIR_OFF:
			airValve1Off();
			airValve3Off();
			fanMotorStop();
			printf("hatcsMod.c:executeActuatorControl():Air Control OFF\r\n");
			break;
		case SEQ_CTRL_AIR_IN:
			airValve1On();
			airValve3Off();
			fanMotorStop();
			setMkValveStatus(AIR_OUT);
			printf("hatcsMod.c:executeActuatorControl():AIR_OUT\r\n");
			break;
		case SEQ_CTRL_AIR_OUT:
			airValve1On();
			airValve3On();
			fanMotorCCWR();
			setMkValveStatus(AIR_OUT);
			printf("hatcsMod.c:executeActuatorControl():AIR_OUT\r\n");
			break;
		case SEQ_CTRL_AIR_RECIRC:
			airValve1Off();
			airValve3On();
			fanMotorStop();
			setMkValveStatus(AIR_RECIRC);
			printf("hatcsMod.c:executeActuatorControl():AIR_RECIRC\r\n");
			break;
		default:
			break;
	}

	//Control Sprayer
	if(controlCode[index].sprayerCtrl == SEQ_CTRL_SPRAYER_ONCE)
	{
		printf("hatcsMod.c:executeActuatorControl():SPRAY_ONCE\r\n");
		sprayOnceDC(DEFAULT_SPRAYER_DURATION);
	}
	return DG_SUCCESS;
}

/*int executeActuatorControl(uint8_t hatSensorState, uint8_t stateChange)
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
		updateHeaterStatus(ON);
	}
	else
	{
		printf("hatcsMod.c:executeActuatorControl():HEATER_OFF\r\n");
		HEATER_OFF();
		updateHeaterStatus(OFF);
	}

	//Fan Control
	switch(controlCode[controlSeqIndex].fanCtrl)
	{
		case SEQ_CTRL_FAN_OFF:
			fanMotorStop();
			hFanStop();
			printf("hatcsMod.c:executeActuatorControl():FAN_OFF\r\n");
			break;
		case SEQ_CTRL_FAN_CWR:
			fanMotorStop();
			if(controlCode[controlSeqIndex].airCircCtrl == SEQ_CTRL_AIR_OUT)
			{
				hFanCWR(100);
			}
			else
			{
				hFanCWR(70);
			}

			//fanMotorCWR();
			printf("hatcsMod.c:executeActuatorControl():FAN_CWR\r\n");
			break;
		case SEQ_CTRL_FAN_CCWR:
			fanMotorCCWR();
			hFanStop();
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
			fanMotorStop();
			printf("hatcsMod.c:executeActuatorControl():Air Control OFF\r\n");
			break;
		case SEQ_CTRL_AIR_IN:
			airValve1On();
			airValve3Off();
			fanMotorStop();
			setMkValveStatus(AIR_RECIRC);
			printf("hatcsMod.c:executeActuatorControl():AIR_IN\r\n");
			break;
		case SEQ_CTRL_AIR_OUT:
			airValve1On();
			airValve3On();
			fanMotorCCWR();
			setMkValveStatus(AIR_OUT);
			printf("hatcsMod.c:executeActuatorControl():AIR_OUT\r\n");
			break;
		case SEQ_CTRL_AIR_RECIRC:
			airValve1Off();
			airValve3On();
			fanMotorStop();
			setMkValveStatus(AIR_RECIRC);
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
}*/

int executeActuatorSafeState(void)
{
	//Stop CT
	ctStop(HATCS_MOD);
	//Stop Heater
	HEATER_OFF();
	updateHeaterStatus(OFF);
	//Stop suction Fan
	fanMotorStop();
	//Stop Heater fan
	hFanStop();
	DCsprayerOff();
	airValve1Off();
	airValve3Off();


	return DG_SUCCESS;
}


static void hatcs_task(void *pvParameters)
{
	QueueHandle_t hatcsQHandle;
	dgMsg_t rcvMsg;							//Holds the currently received message
	static int hatcsState;					// This stores the state of HAT control system
	static int hatSensorState;				//This variable stores the Temp/humidity sensor state
	dgActuatorCtrlCode_t  controlCode[MAX_CTRL_CODE_PER_SEQ];
	uint8_t newSensorState;
	bool	sensorStateChangeFlag;
	bool	aerationFlag;
	uint8_t ctrlSeqIndex;

	printf("hatcs.c:hatcs_task():started\r\n");

	//Get the Qhandle for this task and store locally
	hatcsQHandle = getQHandle(HATCS_MOD);

	//Initialize the state of controller to IDLE
	hatcsState = HATCS_STATE_IDLE;
	hatSensorState = HAT_SENSOR_TEMP_WR_HUM_WR;

	sprayerTimerInit();
	mkValveStatus = AIR_RECIRC;
	mkValveTimerInit();
	mkValveMovingFlag = false;

	aerationFlag = false;
	sensorStateChangeFlag = false;
	ctrlSeqIndex = 0;


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
				//Change in sensor state notification received
				//Get the new sensor status from the command
				newSensorState = *((uint8_t*)rcvMsg.cmdParam);
				*rcvMsg.result = DG_SUCCESS;
				if(newSensorState > HAT_SENSOR_TEMP_BR_HUM_BR)
				{
					newSensorState = HAT_SENSOR_TEMP_WR_HUM_WR;   //Safe condition
				}
				hatSensorState = newSensorState;
/*
				sensorStateChangeFlag = true;

				if(rcvMsg.taskHandleSM != NULL)
				{
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
*/

				//Load control sequence from EEPROM
				if(loadActuatorSeq(&controlCode[0], hatSensorState) != DG_SUCCESS)
				{
					printf("hatcsMod.c:hatcs_task():Config read from EEPROM failed\r\n");
					*rcvMsg.result = DG_FAIL;
				}
				printf("hatcsMod.c:hatcs_task():Sensor status changed to %d\r\n",hatSensorState);
				ctrlSeqIndex = 0;
				if(rcvMsg.taskHandleSM != NULL)
				{
					xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
				}
				executeActuatorControl(&controlCode[0],ctrlSeqIndex );
				break;

			case HATCS_INSTRUCT_AIRINLET:
				aerationFlag = true ;
				*rcvMsg.result = DG_SUCCESS;
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

					//Load control sequence from EEPROM
					if(loadActuatorSeq(&controlCode[0], hatSensorState) != DG_SUCCESS)
					{
						printf("hatcsMod.c:executeActuatorControl():Config read from EEPROM failed\r\n");
						/*TODO*/ /* This error to be handled */
					}
					ctrlSeqIndex = 0;
					//Execute Actuator Control for TWR_HWR state
					executeActuatorControl(&controlCode[0],ctrlSeqIndex );
					dgtimerStart(HATCS_MOD, CONV_SEC_TO_TICKS(controlCode[ctrlSeqIndex].duration*60));
					printf("hatcsMod.c:executeActuatorControl():Duration %d\r\n",controlCode[ctrlSeqIndex].duration);
					ctrlSeqIndex++;
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
					//Check for 2A Motor driver error flag
	                if((READ_TC78H660_ERR_STATUS() & 0x01) == 0)
	                {
	                	HEATER_OFF();
	            		updateHeaterStatus(OFF);
	                    printf("hatcs.c:hatcs_task():TC78H660 Error Flag active\r\n");
						sendCliResponse("2A Motor driver Error Flag Active\r\n", 35);
						//Correct the error
						TC78H660_Stby();
						hFanStop();
						augerMotorStop();
						vTaskDelay(1);   //About 5mSec delay
						TC78H660_Active();
	                }
/*
	                if(sensorStateChangeFlag == true)
	                {
	    				hatSensorState = newSensorState;
	    				//Load control sequence from EEPROM
	    				if(loadActuatorSeq(&controlCode[0], hatSensorState) != DG_SUCCESS)
	    				{
	    					printf("hatcsMod.c:hatcs_task():Config read from EEPROM failed\r\n");
	    				}
	    				printf("hatcsMod.c:hatcs_task():Sensor status changed to %d\r\n",hatSensorState);
	    				ctrlSeqIndex = 0;
	    				sensorStateChangeFlag = false;
	                }
*/
					//Execute Actuator Control
					executeActuatorControl(&controlCode[0],ctrlSeqIndex );
					dgtimerStart(HATCS_MOD, CONV_SEC_TO_TICKS(controlCode[ctrlSeqIndex].duration*60));
					printf("hatcsMod.c:executeActuatorControl():Duration %d, Index: %d, sensorState:%d\r\n",controlCode[ctrlSeqIndex].duration,ctrlSeqIndex, hatSensorState);
	            	if(controlCode[ctrlSeqIndex].ctrlSeqEnd == SEQ_CTRL_END)
	            	{
	            		ctrlSeqIndex = 0;   //Reset the Index to start of control sequence
	            		if(aerationFlag == true)
	            		{
	    					//Load control sequence from EEPROM
	    					if(loadActuatorSeq(&controlCode[0], AIR_IN_CTRL_SEQ) != DG_SUCCESS)
	    					{
	    						printf("hatcsMod.c:executeActuatorControl():Config read from EEPROM failed\r\n");
	    						/*TODO*/ /* This error to be handled */
	    					}
	    					aerationFlag = false;
	            		}
	            		else
	            		{
	    					//Load control sequence from EEPROM
	    					if(loadActuatorSeq(&controlCode[0], hatSensorState) != DG_SUCCESS)
	    					{
	    						printf("hatcsMod.c:executeActuatorControl():Config read from EEPROM failed\r\n");
	    						/*TODO  This error to be handled */
	    					}
	            		}
	            	}
	            	else
	            	{
						ctrlSeqIndex++;
	            	}

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

