/*
 * shredder.c
 *
 *  Created on: 28-Apr-2025
 *      Author: Anusha
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


#define INCLUDE_FLAP_CONTROL
#define INCLUDE_FLUSH_CONTROL



/*************************** Global variables *******************/

TimerHandle_t flapTimerHandle;
extern dgConfigMem_t allConfig;
dgShredderCtrlSeq_t *shdCtrlSeq;
dgShredderTimingVar_t *shdTiming;

/***********************Macro definitions************************/


//Valid values for event input parameter
#define SHREDMOTOR_START_EVENT		0
#define SHREDMOTOR_STOP_EVENT		1
#define SHREDMOTOR_TIMER_EVENT		2





int flapTimerStart(TickType_t timeoutValue)
{

	if(xTimerChangePeriod(flapTimerHandle,timeoutValue, DGTIMER_BLOCKTIME )== pdFALSE)
	{
		PRINTF("shredder.c:flapTimerStart():Change period failed\r\n");
		return DG_FAIL;
	}
	if(xTimerStart(flapTimerHandle,DGTIMER_BLOCKTIME) == pdFALSE)
	{
		PRINTF("shredder.c:flapTimerStart():Start failed\r\n");
		return DG_FAIL;
	}
	return DG_SUCCESS;
}


void flapTimerCallback( TimerHandle_t xTimer)
{
	//Turn of flap motor
	flapMotorStop();
}


int executeSHDSeqControl(uint8_t seqEngineControl)
{
	static uint8_t controlSeqIndex=0;

	printf("shredder.c:sexecuteSHDSeqControl():ControlSeqIndex= %d, seqEngineControl= %d\r\n",controlSeqIndex,seqEngineControl);
	switch(seqEngineControl)
	{
	case SEQ_ENGINE_START:
		controlSeqIndex=0;
		break;
	case SEQ_ENGINE_STOP:
		//Stop timer
		dgtimerStop(SHREDDER_MOD);
		//Take all the actuator to safe state
		SHD_STOP();
#ifdef INCLUDE_FLAP_CONTROL
		flapMotorStop();
#endif /*INCLUDE_FLAP_CONTROL */

#ifdef INCLUDE_FLUSH_CONTROL
		//Stop spraying water
#endif /*INCLUDE_FLUSH_CONTROL */
		return DG_ACTION_COMPLETE;
		break;
	case SEQ_ENGINE_CONTINUE:
		break;
	}

	if(controlSeqIndex == 0xFF)
	{
		//Start condition is expected
		return DG_FAIL;
	}

	//Set timer
	dgtimerStart(SHREDDER_MOD, CONV_SEC_TO_TICKS(shdCtrlSeq[controlSeqIndex].durationSec));

	//Shredder Motor Control
	if(shdCtrlSeq[controlSeqIndex].shdMotor == SEQ_CTRL_SHD_MOTOR_CCWR)
	{
		SHD_DIR_CW();
		SHD_START();
	}
	else if(shdCtrlSeq[controlSeqIndex].shdMotor == SEQ_CTRL_SHD_MOTOR_CWR)
	{
		SHD_DIR_CCW();
		SHD_START();
	}
	else
	{
		//SEQ_CTRL_SHD_MOTOR_OFF or any invalid value -> safe state
		SHD_STOP();
	}

	//Shredder Flap Control
#ifdef INCLUDE_FLAP_CONTROL
	if(shdCtrlSeq[controlSeqIndex].shdFlapMotor == SEQ_CTRL_SHD_FLAP_OFF)
	{
		flapMotorStop();
	}
	else if(shdCtrlSeq[controlSeqIndex].shdFlapMotor == SEQ_CTRL_SHD_FLAP_OPEN)
	{
		flapMotorCCWR();
		flapTimerStart(((shdTiming->flapOpenDur)*1000)/ portTICK_PERIOD_MS);
	}
	else if(shdCtrlSeq[controlSeqIndex].shdFlapMotor == SEQ_CTRL_SHD_FLAP_CLOSE)
	{
		flapMotorCWR();
		flapTimerStart(((shdTiming->flapCloseDur)*1000)/ portTICK_PERIOD_MS);
	}
	else
	{
		flapMotorStop();
	}
#endif /*INCLUDE_FLAP_CONTROL */

	//Flush Control
#ifdef INCLUDE_FLUSH_CONTROL
	if(shdCtrlSeq[controlSeqIndex].flushSprayer == SEQ_CTRL_FLUSH_ON)
	{
		flushSprayerOn();  // Controls the flush valcve
	}
	else if(shdCtrlSeq[controlSeqIndex].flushSprayer == SEQ_CTRL_FLUSH_OFF)
	{
		flushSprayerOff();  // Controls the flush valcve
	}
	else
	{

	}
#endif /*INCLUDE_FLUSH_CONTROL*/


	//Shredder Auger Motor Control
	if(shdCtrlSeq[controlSeqIndex].shdAugMotor == SEQ_CTRL_SHDAUG_MOTOR_CCWR)
	{
		shdAugMotorCCWR();
	}
	else if(shdCtrlSeq[controlSeqIndex].shdAugMotor == SEQ_CTRL_SHDAUG_MOTOR_CWR)
	{
		shdAugMotorCWR();
	}
	else
	{
		//SEQ_CTRL_SHDAUG_MOTOR_OFF or any invalid value -> safe state
		shdAugMotorStop();
	}

	//Check if this is the last control in the sequence
	if(shdCtrlSeq[controlSeqIndex].ctrlSeqRecType ==SEQ_CTRL_END)
	{
		controlSeqIndex = 0xFF;   //Reset the Index to start of control sequence
		return DG_ACTION_COMPLETE;
	}
	else
	{
		controlSeqIndex++;
	}
	return DG_INPROGRESS;
}



static void shredder_task(void *pvParameters)
{
	QueueHandle_t shdQHandle;
	dgMsg_t rcvMsg;				    //Holds the currently received message
	static int shredderState;		// This stores the state of modbus transaction



	//Initialize the state of shredder to IDLE
	shredderState = SHD_STATE_IDLE;

	//Initialize shredder Control system configuration parameters
	//Initialize Timeout variables
	shdTiming = &allConfig.shredderConfig.shdParams.shdTimingVars;
	shdCtrlSeq = &allConfig.shredderConfig.shdParams.shdActCtrlSeq[0];

	//Wait till module registration is complete
	shdQHandle = NULL;
	while(shdQHandle== NULL)
	{
		vTaskDelay(100 / portTICK_PERIOD_MS);
		shdQHandle = getQHandle(SHREDDER_MOD);
	}


	while(1)
	{
		//Receive event from Queue. Block until event is available
		if(xQueueReceive(shdQHandle, &rcvMsg, portMAX_DELAY ) != pdPASS )
		{
			// Queue did not return an event. Hence go back
			continue;
		}
		printf("shredder.c:shredder_Task():Command %d rcvd in state %d\r\n",rcvMsg.command,shredderState);
		switch(shredderState)
		{
		case SHD_STATE_IDLE:
			switch(rcvMsg.command)
			{
			case DG_LID_OPEN:
				//Now lid is open. Wait for close to start shredder
				shredderState = SHD_STATE_WAITFORCLOSE;
				break;
			case DG_LID_CLOSE:
				//Ignore
				break;
			case DG_TIMER_EXPIRY:
				//Ignored. Not expected in IDLE state
				break;
			default:
				//Ignored. Unexpected event
				break;
			}
			break;
		case SHD_STATE_WAITFORCLOSE:
			switch(rcvMsg.command)
			{
			case DG_LID_OPEN:
				//No need to do anything. Stay in the same state
				break;
			case DG_LID_CLOSE:
				//Shredding operation will start after a delay. Hence start a timer
				shredderState = SHD_STATE_STARTDELAY;
				dgtimerStart(SHREDDER_MOD, CONV_SEC_TO_TICKS(shdTiming->shdStartDlyFmLidclose));
				break;
			case DG_TIMER_EXPIRY:
				//Ignored. Not expected in IDLE state
				break;
			default:
				//Ignored. Unexpected event
				break;
			}
			break;
		case SHD_STATE_STARTDELAY:
			switch(rcvMsg.command)
			{
			case DG_LID_OPEN:
				//Go back to WAIT FOR CLOSE
				shredderState = SHD_STATE_WAITFORCLOSE;
				dgtimerStop(SHREDDER_MOD);
				break;
			case DG_LID_CLOSE:
				//Ignore. This event cannot occur in this state
				break;
			case DG_TIMER_EXPIRY:
				//Shredder start delay is over. Start shredding
				shredderState = SHD_STATE_ACTIVE;
				executeSHDSeqControl(SEQ_ENGINE_START);
				break;
			default:
				break;
			}
			break;
		case SHD_STATE_ACTIVE:
			switch(rcvMsg.command)
			{
			case DG_LID_OPEN:
				// Need to stop shredding operation
				executeSHDSeqControl(SEQ_ENGINE_STOP);
				shredderState = SHD_STATE_WAITFORCLOSE;
				break;
			case DG_LID_CLOSE:
				//Ignore. Not expected in this state
				break;
			case DG_TIMER_EXPIRY:
				if(executeSHDSeqControl(SEQ_ENGINE_CONTINUE) == DG_ACTION_COMPLETE)
				{
					shredderState = SHD_STATE_IDLE;
				}
				break;
			default:
				//Ignore the event
				break;
			}
			break;

		default:
			break;
		}
	}

}





int initShd(void)
{
	//Create shredder task task
	TaskHandle_t shdTaskHandle;
	TimerHandle_t shdTimerHandle;
	BaseType_t result;

	result = xTaskCreate(shredder_task, "shredder_task", configMINIMAL_STACK_SIZE + 100, NULL, task_PRIORITY, &shdTaskHandle);
	if ( result !=    pdPASS)
	{
		printf("shredder Task creation failed!.\r\n");
		return DG_FAIL;
	}
	//Shredder requires timer and hence create FreeRTOS SW timer
	shdTimerHandle = xTimerCreate("shdTimer",100, pdFALSE, (void*)SHREDDER_MOD, dgTimerCallback);
	if(shdTimerHandle == NULL)
	{
		printf("Timer creation failed!.\r\n");
		vTaskDelete(shdTaskHandle);
		return DG_FAIL;
	}
	//Flap motor requires a timer to stop the motor
	flapTimerHandle = xTimerCreate("flapTimer",100, pdFALSE, NULL, flapTimerCallback);
	if(flapTimerHandle == NULL)
	{
		printf("Flap Timer creation failed!.\r\n");
	}
	if(registerModule(SHREDDER_MOD, shdTaskHandle, shdTimerHandle)!= DG_SUCCESS)
	{
		//Registering the module failed. Hence kill the task and return error
		printf("Shredder Task registration failed!.\r\n");
		xTimerDelete(shdTimerHandle,100 / portTICK_PERIOD_MS);
		vTaskDelete(shdTaskHandle);

		return DG_FAIL;
	}
	return DG_SUCCESS;
}

