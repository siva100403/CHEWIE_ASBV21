/*
 * compostingModule.c
 *
 *  Created on: 15-Nov-2024
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
#include "motorControl.h"
#include "seqControlCommon.h"
#include "dgI2cDriver.h"
#include "eeConfig.h"
#include "sht40Driver.h"
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
#include "HMICmdProc.h"
#include "HMICmdProcAPI.h"
#include "csmMod.h"
#include "csmModAPI.h"
#include "measure.h"
#include "alert.h"


/*******************************************************************************
 * Definitions
 ******************************************************************************/


uint8_t chooseWasteCat(uint8_t currentWasteCat, uint8_t newWasteCat)
{
	//This method chooses between the current and new waste category based on some criteria
	//The criteria has not been decided and hence chooses the newWasteCat.
	if((currentWasteCat>= WASTE_CAT_MAX)||(newWasteCat >=WASTE_CAT_MAX ))
	{
		return CSM_DEFAULT_WASTE_CAT;
	}
	return newWasteCat;
}



void csm_Task(void* arg)
{

	QueueHandle_t csmQHandle;
	dgMsg_t rcvMsg;				// Holds the currently received message
	dgCtProcessParam_t currentProcessParam;
	dgCtStateVar_t ctVar;
	uint16_t timeout;
	dgCsmParam_t *configparams;


	//Get the Qhandle for this task and store locally
	csmQHandle = getQHandle(CSM_MOD);
	ctVar.state = CSM_STATE_IDLE;


	while (1)
	{
		//Receive event from Queue. Block until event is available
		if(xQueueReceive(csmQHandle, &rcvMsg, portMAX_DELAY ) != pdPASS )
		{
			// Queue did not return an event. Hence go back
			continue;
		}
		printf("csm_Task():Received command:%d in state %d \r\n",rcvMsg.command,ctVar.state );
		switch(rcvMsg.command)
		{
		case CSM_GET_STATE:
			configparams = (dgCsmParam_t*)rcvMsg.cmdParam;
			//Write current state
			configparams->state = ctVar.state;
			configparams->remDur = ctVar.remDur;
			configparams->phase = ctVar.curPhase;
			configparams->wasteCat = ctVar.curWasteCat;

			*rcvMsg.result = DG_SUCCESS;
			if(rcvMsg.taskHandleSM != NULL)
			{
				xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
			}
			break;

		case CSM_START:
			if(ctVar.state == CSM_STATE_IDLE )
			{
				//Only in IDLE state phase can be modified

				configparams = (dgCsmParam_t*)rcvMsg.cmdParam;
				//Update the composting phase after validation
				if ((configparams->phase <PHASE_COUNT_MAX)&&(configparams->wasteCat <WASTE_CAT_MAX))
				{
					ctVar.curPhase = configparams->phase;
					ctVar.curWasteCat = configparams->wasteCat;
					//Update the compost process parameters based on the updated waste category
					getCsmPhaseParam(ctVar.curWasteCat, ctVar.curPhase, &currentProcessParam);
					//loadProcessParam(&currentProcessParam, ctVar.curWasteCat, ctVar.curPhase);
					ctVar.schDur = currentProcessParam.phaseDur;
					//Update the duration with remaining duration
					ctVar.remDur = (configparams->remDur < ctVar.schDur)? configparams->remDur:ctVar.schDur;
					//Update the csmState variable
					ctVar.state = configparams->state;

					//Update RTC RAM
					updateChewieStateStore(&ctVar);
					//Start 1 minute timer
					dgtimerStart(CSM_MOD, CONV_SEC_TO_TICKS(ONE_MIN_TIMEOUT));
					//Set the temperature/humidity threshold to sensor Module and start
					if(sensorSetParam(CSM_MOD, currentProcessParam.temperature, currentProcessParam.humidity,TEMP_CORRECTION_ERROR, HUMIDITY_CORRECTION_ERROR)!= DG_SUCCESS)
					{
						printf("compostingModule.c:csmTask(): sensorSetParam API failure\r\n");
					}
					sensorStart(CSM_MOD);
					//Start the hatcs module which control the actuators to maintain the temp/humidity/aeration
					hatcsStart(CSM_MOD);
					*rcvMsg.result = DG_SUCCESS;
				}
				else if(configparams->phase == TRANSFER_PHASE)
				{
					//Before switching to transfer phase, turn off HATCS and SENSORMOD
					if(checkTransferFeasibility() == DG_SUCCESS)
					{
						ctVar.state = CSM_STATE_TRANSFER;
						ctVar.curPhase = TRANSFER_PHASE;
						ctVar.prevPhase = PATHOGEN_ELM_PHASE;
						//Add timeout to check the transfer time
						ctVar.remDur = TRANFER_TIMEOUT;
						//No need to load any process parameters. Transfer state does not require

						//Store the current state to RTC RAM
						updateChewieStateStore(&ctVar);
						//inform Humidity-Aeration-Temperature CS to go IDLE
						sensorStop(CSM_MOD);
						hatcsStop(CSM_MOD);
						//Initiate transfer
						transferStart(CSM_MOD, TRANSFER_DUR_DEFAULT);
						//Does CT has to be turned ON to easy transfer?
						*rcvMsg.result = DG_SUCCESS;
					}
					else
					{
						generateAlert(ALERT_TRANSFER_PENDING);
					}
				}
				else
				{
					printf("compostingModule.c:csmTask():csmStart command-Invalid Param error\r\n");
					*rcvMsg.result = DG_INVALID_PARAM;
				}
			}
			else
			{
				*rcvMsg.result = DG_FAIL	;
			}
			if(rcvMsg.taskHandleSM != NULL)
			{
				xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
			}
			break;

		case CSM_STOP:
			ctVar.state = CSM_STATE_IDLE;
			//Set the actuators and sensors to safe mode
			sensorStop(CSM_MOD);
			hatcsStop(CSM_MOD);
			transferAbort(CSM_MOD);


			//stop 1 min timer
			dgtimerStop(CSM_MOD);

			//Save the current status of Composting state machine
			updateChewieStateStore(&ctVar);

			*rcvMsg.result = DG_SUCCESS;

			if(rcvMsg.taskHandleSM != NULL)
			{
				xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
			}
			break;

		case CSM_NOTIFY_TRANSFER_END:
			//First send notification to the caller
			*rcvMsg.result = DG_SUCCESS;

			if(rcvMsg.taskHandleSM != NULL)
			{
				xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
			}

			switch(ctVar.state)
			{
			case CSM_STATE_IDLE:
				//Should get this event in this state. Ignore
				break;
			case CSM_STATE_MPHASE:
				//Should get this event in this state. Ignore
				break;
			case CSM_STATE_TPHASE:
				//Should get this event in this state. Ignore
				break;
			case CSM_STATE_PPHASE:
				//Should get this event in this state. Ignore
				break;
			case CSM_STATE_TRANSFER:
				//Transfer is over.
				//Go to IDLE state if there is no waste waiting in the shredder
				ctVar.state = CSM_STATE_IDLE;
				//Stop 1 min timer
				//stop 1 min timer
				dgtimerStop(CSM_MOD);
				ctVar.curPhase = PHASE_IDLE;
				break;
			case CSM_STATE_ADDWASTE_I:
				//Should get this event in this state. Ignore
				break;
			case CSM_STATE_ADDWASTE_M:
				//Should get this event in this state. Ignore
				break;
			case CSM_STATE_ADDWASTE_T:
				//Should get this event in this state. Ignore
				break;
			case CSM_STATE_ERROR:
				//Should get this event in this state. Ignore
				break;
			}
			break;

		case CSM_WASTE_ADD_START:
			//First send notification to the caller
			*rcvMsg.result = DG_SUCCESS;

			if(rcvMsg.taskHandleSM != NULL)
			{
				xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
			}
			switch(ctVar.state)
			{
			case CSM_STATE_IDLE:
				//Move to CSM_STATE_ADDWASTE_I state.
				//Do we need to turn the compost in the chamber for the new waste to come in??
				/*TODO*/
				ctVar.state = CSM_STATE_ADDWASTE_I;
				//Chewie cannot be in this state for long. Hence add a timer
				timeout = ADDWASTE_DURATION_TIMEOUT;
				updateChewieStateStore(&ctVar);
				dgtimerStart(CSM_MOD, CONV_SEC_TO_TICKS(ONE_MIN_TIMEOUT));
				break;
			case CSM_STATE_MPHASE:
				//Change state to CSM_STATE_ADDWASTE_M
				ctVar.state = CSM_STATE_ADDWASTE_M;
				//Chewie cannot be in this state for long. Hence add a timer
				timeout = ADDWASTE_DURATION_TIMEOUT;
				//Do we need to temporarily halt ct operation during waste addition?
				/*TODO*/
				updateChewieStateStore(&ctVar);
				break;
			case CSM_STATE_TPHASE:
				//Change state to CSM_STATE_ADDWASTE_T
				ctVar.state = CSM_STATE_ADDWASTE_T;
				//Chewie cannot be in this state for long. Hence add a timer
				timeout = ADDWASTE_DURATION_TIMEOUT;
				//Do we need to temporarily halt ct operation during waste addition?
				/*TODO*/
				updateChewieStateStore(&ctVar);
				break;
			case CSM_STATE_PPHASE:
				//Ignore the event
				//Shredding operation can wait till this phase and Transfer phase is over
				//This should be taken care by shredding module
				/*TODO*/
				break;
			case CSM_STATE_TRANSFER:
				//Ignore the event
				//Shredding operation should not start when Chewie is transferring compost to storage
				//This should be taken care by shredding module
				/*TODO*/
				break;
			case CSM_STATE_ADDWASTE_I:
				//Ignore the event.
				break;
			case CSM_STATE_ADDWASTE_M:
				//Ignore the event.
				break;
			case CSM_STATE_ADDWASTE_T:
				//Ignore the event.
				break;
			case CSM_STATE_ERROR:
				//Ignore the event.
				break;
			}
			break;

		case CSM_WASTE_ADD_END:
			//First send notification to the caller
			*rcvMsg.result = DG_SUCCESS;

			if(rcvMsg.taskHandleSM != NULL)
			{
				xTaskNotify(rcvMsg.taskHandleSM, 0, eNoAction);
			}
			//get the parameters paased
			configparams = (dgCsmParam_t*)rcvMsg.cmdParam;
			switch(ctVar.state)
			{
			case CSM_STATE_IDLE:
				//Incorrect state for the event
				//Log event
				break;
			case CSM_STATE_MPHASE:
				//Incorrect state for the event
				//Log event
				break;
			case CSM_STATE_TPHASE:
				//Incorrect state for the event
				//Log event
				break;
			case CSM_STATE_PPHASE:
				//Incorrect state for the event
				//Log event
				break;

			case CSM_STATE_TRANSFER:
				//Incorrect state for the event
				//Log event
				break;
			case CSM_STATE_ADDWASTE_I:
				//Get waste category
				ctVar.curWasteCat = chooseWasteCat(ctVar.curWasteCat, configparams->wasteCat);
				//Waste adding ended. Move to MPHASE
				ctVar.state = CSM_STATE_MPHASE;
				ctVar.curPhase = MESOPHILIC_PHASE;
				ctVar.prevPhase = PHASE_IDLE;

				//Load process parameters corresponding to the Phase and waste category
				getCsmPhaseParam(ctVar.curWasteCat, ctVar.curPhase, &currentProcessParam);
				//loadProcessParam(&currentProcessParam, ctVar.curWasteCat, ctVar.curPhase);
				ctVar.schDur = currentProcessParam.phaseDur;
				ctVar.remDur = currentProcessParam.phaseDur;
				ctVar.expDur = 0;

				//Store the current state to RTC RAM
				updateChewieStateStore(&ctVar);
//				dgtimerStart(CSM_MOD, CONV_SEC_TO_TICKS(ONE_MIN_TIMEOUT));
				//Inform the Temperature/Humidity/Aeration module about state change and update parameters
				sensorStop(CSM_MOD);
				if(sensorSetParam(CSM_MOD, currentProcessParam.temperature, currentProcessParam.humidity,TEMP_CORRECTION_ERROR, HUMIDITY_CORRECTION_ERROR)!= DG_SUCCESS)
				{
					printf("compostingModule.c:csmTask(): sensorSetParam API failure\r\n");
				}
				sensorStart(CSM_MOD);
				break;
			case CSM_STATE_ADDWASTE_M:
				//Get waste category
				ctVar.curWasteCat = chooseWasteCat(ctVar.curWasteCat, configparams->wasteCat);

				//Waste adding ended. Move back to MPHASE.
				ctVar.state = CSM_STATE_MPHASE;
				ctVar.curPhase = MESOPHILIC_PHASE;
				//Update process parameter based on new waste cat
				getCsmPhaseParam(ctVar.curWasteCat, ctVar.curPhase, &currentProcessParam);
				//loadProcessParam(&currentProcessParam, ctVar.curWasteCat, ctVar.curPhase);
				//Reset the remaining time
				ctVar.schDur = currentProcessParam.phaseDur;
				ctVar.remDur = ctVar.schDur;

				//Inform the Temperature/Humidity/Aeration module about state change and update parameters
				sensorStop(CSM_MOD);
				if(sensorSetParam(CSM_MOD, currentProcessParam.temperature, currentProcessParam.humidity,TEMP_CORRECTION_ERROR, HUMIDITY_CORRECTION_ERROR)!= DG_SUCCESS)
				{
					printf("compostingModule.c:csmTask(): sensorSetParam API failure\r\n");
				}
				sensorStart(CSM_MOD);
				//Store the current state to RTC RAM
				updateChewieStateStore(&ctVar);
				break;
			case CSM_STATE_ADDWASTE_T:
				//Get waste category
				ctVar.curWasteCat = chooseWasteCat(ctVar.curWasteCat, configparams->wasteCat);
				//Waste adding ended. Move back to TPHASE.
				ctVar.state = CSM_STATE_TPHASE;
				ctVar.curPhase = THERMOPHILIC_PHASE;
				//Update process parameter based on new waste cat
				getCsmPhaseParam(ctVar.curWasteCat, ctVar.curPhase, &currentProcessParam);
				//loadProcessParam(&currentProcessParam, ctVar.curWasteCat, ctVar.curPhase);
				//Reset the remaining time
				ctVar.schDur = currentProcessParam.phaseDur;
				ctVar.remDur = ctVar.schDur;
				//Inform the Temperature/Humidity/Aeration module about state change and update parameters
				sensorStop(CSM_MOD);
				if(sensorSetParam(CSM_MOD, currentProcessParam.temperature, currentProcessParam.humidity,TEMP_CORRECTION_ERROR, HUMIDITY_CORRECTION_ERROR)!= DG_SUCCESS)
				{
					printf("compostingModule.c:csmTask(): sensorSetParam API failure\r\n");
				}
				sensorStart(CSM_MOD);
				break;
			case CSM_STATE_ERROR:
				//Ignore event
				break;

			}
			break;

		case DG_TIMER_EXPIRY:    //1 min event
			switch(ctVar.state)
			{
			case CSM_STATE_IDLE:
				//Nothing to be done
				//Timer event is by mistake. stop 1 min timer
				dgtimerStop(CSM_MOD);
				break;

			case CSM_STATE_MPHASE:
				ctVar.remDur--;
				ctVar.expDur++;
				if(ctVar.remDur == 0)
				{
					//duration is over.Move to TPHASE
					ctVar.state = CSM_STATE_TPHASE;
					ctVar.curPhase = THERMOPHILIC_PHASE;
					ctVar.prevPhase = MESOPHILIC_PHASE;

					//Load process parameters corresponding to the Phase and waste category
					getCsmPhaseParam(ctVar.curWasteCat, ctVar.curPhase, &currentProcessParam);
					//loadProcessParam(&currentProcessParam, ctVar.curWasteCat, ctVar.curPhase);
					ctVar.schDur = currentProcessParam.phaseDur;
					ctVar.remDur = currentProcessParam.phaseDur;
					ctVar.expDur = 0;

					//Store the current state to RTC RAM
					updateChewieStateStore(&ctVar);
					//Inform the Temperature/Humidity/Aeration module about state change and update parameters
					sensorStop(CSM_MOD);
					if(sensorSetParam(CSM_MOD, currentProcessParam.temperature, currentProcessParam.humidity,TEMP_CORRECTION_ERROR, HUMIDITY_CORRECTION_ERROR)!= DG_SUCCESS)
					{
						printf("compostingModule.c:csmTask(): sensorSetParam API failure\r\n");
					}
					sensorStart(CSM_MOD);
				}
				else
				{
					//Phase is not over
					updateChewieStateStoreExpDur(ctVar.expDur, ctVar.remDur);
				}
				break;
			case CSM_STATE_TPHASE:
				ctVar.remDur--;
				ctVar.expDur++;
				if(ctVar.remDur == 0)
				{
					//duration is over.Move to PPHASE
					ctVar.state = CSM_STATE_PPHASE;
					ctVar.curPhase = PATHOGEN_ELM_PHASE;
					ctVar.prevPhase = THERMOPHILIC_PHASE;

					//Load process parameters corresponding to the Phase and waste category
					getCsmPhaseParam(ctVar.curWasteCat, ctVar.curPhase, &currentProcessParam);
					//loadProcessParam(&currentProcessParam, ctVar.curWasteCat, ctVar.curPhase);
					ctVar.schDur = currentProcessParam.phaseDur;
					ctVar.remDur = currentProcessParam.phaseDur;
					ctVar.expDur = 0;

					//Store the current state to RTC RAM
					updateChewieStateStore(&ctVar);
					//Inform the Temperature/Humidity/Aeration module about state change and update parameters
					sensorStop(CSM_MOD);
					if(sensorSetParam(CSM_MOD, currentProcessParam.temperature, currentProcessParam.humidity,TEMP_CORRECTION_ERROR, HUMIDITY_CORRECTION_ERROR)!= DG_SUCCESS)
					{
						printf("compostingModule.c:csmTask(): sensorSetParam API failure\r\n");
					}
					sensorStart(CSM_MOD);
				}
				else
				{
					//Phase is not over
					updateChewieStateStoreExpDur(ctVar.expDur, ctVar.remDur);
				}
				break;
			case CSM_STATE_PPHASE:
				ctVar.remDur--;
				ctVar.expDur++;
				if(ctVar.remDur == 0)
				{
					//duration is over. Time to transfer the content to storage chamber
					//Check transfer feasibility i.e stroage tray is closed and not full
					if(checkTransferFeasibility() == DG_SUCCESS)
					{
						ctVar.state = CSM_STATE_TRANSFER;
						ctVar.curPhase = TRANSFER_PHASE;
						ctVar.prevPhase = PATHOGEN_ELM_PHASE;
						//Add timeout to check the transfer time
						ctVar.remDur = TRANFER_TIMEOUT;
						//No need to load any process parameters. Transfer state does not require

						//Store the current state to RTC RAM
						updateChewieStateStore(&ctVar);
						//inform Humidity-Aeration-Temperature CS to go IDLE
						sensorStop(CSM_MOD);
						hatcsStop(CSM_MOD);
						//Initiate transfer
						transferStart(CSM_MOD, 20);
						//Does CT has to be turned ON to easy transfer?

					}
					else
					{
						generateAlert(ALERT_TRANSFER_PENDING);
					}
				}
				else
				{
					//Phase is not over
					updateChewieStateStoreExpDur(ctVar.expDur, ctVar.remDur);
				}
				break;

			case CSM_STATE_TRANSFER:
				//Reduce timeout count
				ctVar.remDur--;
				if(ctVar.remDur == 0)
				{
					//transfer timeout is over. Transfer complete event not received.
					//Generate Alert
					generateAlert(ALERT_TRANSFER_TIMEOUT);

					//Go back to IDLE state
					ctVar.state = CSM_STATE_IDLE;
					dgtimerStop(CSM_MOD);
					ctVar.prevPhase = ctVar.curPhase;
					ctVar.curPhase = PHASE_IDLE;
					updateChewieStateStore(&ctVar);
					//Stop HATCS
					//inform Humidity-Aeration-Temperature CS to go IDLE
					sensorStop(CSM_MOD);
					hatcsStop(CSM_MOD);
				}
				break;
			case CSM_STATE_ADDWASTE_I:
				//Decrement
				timeout--;
				if(timeout == 0)
				{
					//Chewie remains in Waste adding state and hence generate alert to user
					generateAlert(ALERT_WASTE_ADDITION_TOO_LONG);
				}
				break;
			case CSM_STATE_ADDWASTE_M:
				//Decrement
				timeout--;
				if(timeout == 0)
				{
					//Chewie remains in Waste adding state and hence generate alert to user
					generateAlert(ALERT_WASTE_ADDITION_TOO_LONG);
				}
				break;
			case CSM_STATE_ADDWASTE_T:
				//Decrement
				timeout--;
				if(timeout == 0)
				{
					//Chewie remains in Waste adding state and hence generate alert to user
					generateAlert(ALERT_WASTE_ADDITION_TOO_LONG);
				}
				break;
				break;
			case CSM_STATE_ERROR:
				//Timer expiry event should not occur in this state
				//ignore
				break;
			}
			break;
		default:
			break;
		}

	}

}


/******************************* initCSM() **************************************************
*Description: This function initialises Composting state machine. Create queue, task and  		*
* initialized the datastructures. This should be called at the time of power up initialization.					*
* It does the following																			*
* 																								*
*	- Creates a Queue handle for receiving event messages from other modules					*
* Command Parameters: None																		*
* Return Values:																				*
*	- DG_SUCCESS: When initialization is successful												*
*	- SF_FAIL: When Queue allocation failed														*
*************************************************************************************************/

int initCSM(void)
{
	//Create csm task task
	TaskHandle_t csmTaskHandle;
	TimerHandle_t csmTimerHandle;


    if (xTaskCreate(csm_Task, "csm_Task", configMINIMAL_STACK_SIZE + 200, NULL, task_PRIORITY, &csmTaskHandle) !=
        pdPASS)
    {
        printf("CSM Task creation failed!.\r\n");
        return DG_FAIL;
    }
    //CSM requires timer and hence create FreeRTOS SW timer
    csmTimerHandle = xTimerCreate("csmTimer",CSM_TIMER_DEFAULT, pdTRUE, (void*)CSM_MOD, dgTimerCallback);
    if(csmTimerHandle == NULL)
    {
        printf("Timer creation failed!.\r\n");
    	vTaskDelete(csmTaskHandle);
        return DG_FAIL;
    }
    if(registerModule(CSM_MOD, csmTaskHandle, csmTimerHandle)!= DG_SUCCESS)
    {
    	//Registering the module failed. Hence kill the task and return error
        printf("CLI Task registration failed!.\r\n");
        xTimerDelete(csmTimerHandle,100 / portTICK_PERIOD_MS);
    	vTaskDelete(csmTaskHandle);

    	return DG_FAIL;
    }
    printf("initCSM():successful\r\n");
    return DG_SUCCESS;
}
