 /*
 * HMICmdProc.c
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
#include "sensorModAPI.h"
#include "augerAPI.h"
#include "shredder.h"
#include "shredderAPI.h"
#include "adcs.h"
#include "limitSwitchMod.h"
#include "hatcsMod.h"
#include "hatcsModAPI.h"
#include "lidModule.h"
#include "lidModuleAPI.h"
#include "HMICmdProc.h"
#include "HMICmdProcAPI.h"
#include "csmMod.h"
#include "csmModAPI.h"
#include "actuatorStatusTracker.h"



//Buffer used to communicate between CLI processing module and the RS232 ISR
dgRs232TrBuf_t HMItrBuf;

//txBuffer is used to pass the transmit data to ISR using sendHMIResponse()
uint8_t txBuffer[MAX_TX_BUFFER_SIZE];

void setRxStatusHmiCmdProc(uint8_t status)
{
	DisableIRQ(HMI_LPUART_IRQn);
	LPUART_EnableRx(HMI_LPUART, false);
	LPUART_EnableTx(HMI_LPUART, false);
	HMItrBuf.rxState = status;
	HMItrBuf.rxCount = 0;
	//Clear OR and
	LPUART_ClearStatusFlags(HMI_LPUART, LPUART_STAT_OR(0b1)|LPUART_STAT_NF(0b1));
	LPUART_EnableRx(HMI_LPUART, true);
	LPUART_EnableTx(HMI_LPUART, true);
	NVIC_SetPriority(HMI_LPUART_IRQn, 4);
	EnableIRQ(HMI_LPUART_IRQn);
}

/*This function transmits the response to HMI over UART*/

void sendHMIResponse(char* response, uint16_t size)
{

	if ((response == NULL)|| (size == 0))
	{
		//Input parameter validation
		return;
	}
	//Disable CLI LPUART interrupt - we are going to access the common data structure
	DisableIRQ(HMI_LPUART_IRQn);

	HMItrBuf.txBufPtr = response;
	HMItrBuf.txState = RS232_TX_INPROGRESS;
	HMItrBuf.txBufSize =size;


	//send the first byte
	LPUART_WriteByte(HMI_LPUART,HMItrBuf.txBufPtr[0]);
	HMItrBuf.txCount = 1;

	//enable interrupt
    LPUART_EnableInterrupts(HMI_LPUART, LPUART_TX_INT_MASK);
	NVIC_SetPriority(HMI_LPUART_IRQn, 4);
	EnableIRQ(HMI_LPUART_IRQn);

}


int createAndSendResponse2HMI(uint8_t executionFlag, uint8_t *rcvdPacketBuffer, uint8_t *payload, uint8_t payloadSize)
{
	uint8_t flags;
	uint8_t txPacketBuff[MAX_PACKET_SIZE];
	uint8_t txPacketSize;
	//uint8_t txBuffer[MAX_TX_BUFFER_SIZE];
	uint16_t txBufferSize;


	flags = ASB_TO_HMI | (executionFlag & 0x0E);
	//Prepare packet to be sent to HMI'
	if(preparePacket(flags,rcvdPacketBuffer, payload, payloadSize, txPacketBuff, &txPacketSize) != DG_SUCCESS)
	{
		//Prepare Packet error. Report and return error
		printf("HMICmdProc.c:sendResponse2HMI(): PreparePacket returned error\r\n");
		return DG_FAIL;
	}
	//Insert esc seq and form the Tx buffer
	if(insertEsc(txPacketBuff, txPacketSize, txBuffer, &txBufferSize) != DG_SUCCESS)
	{
		//Insert esc seq error. Report and return error
		printf("HMICmdProc.c:sendResponse2HMI(): insertESC returned error\r\n");
		return DG_FAIL;
	}

	//Transmit buffer is ready. Now send it to HMI board over UART
	sendHMIResponse((char*)txBuffer, txBufferSize);
	return DG_SUCCESS;
}

int commandProcessor(uint8_t *packetBuffer, uint8_t packetSize, uint8_t *payload, uint8_t *payloadSize)
{
	uint8_t command;


	command = packetBuffer[COMMAND_BYTE];

	switch(command)
	{
	case ACMD_KEEP_ALIVE:
		//There is nothing to be done except clearing KEEP ALIVE TIMER
		//Payload Size is zero
		*payloadSize = 0;
		return DG_SUCCESS;
		break;

	case GET_SENSOR_VALUES:
		dgSensorPayload_t *sensorValues;
		sensorValues = (dgSensorPayload_t*)payload;
		sensorValues->temp = 23.6;
		sensorValues->humidity = 65.7;
		sensorValues->storageLevel = 4.37;
		sensorValues->enzymeLevel = 8.00;
		sensorValues->digChamberLevel = 7.00;
		*payloadSize = sizeof(dgSensorPayload_t);
		return DG_SUCCESS;
		break;
	case PROXIMITY_EVENTS:
		dgProximityEvents_t *proximityEevents;
		proximityEevents = (dgProximityEvents_t *) &packetBuffer[PAYLOAD_START];
		printf("HMICmdProc.c:commandProcessor():Sending proximity event to LidModule\r\n");
		*payloadSize = 0;
		if(sendProximityEvent(proximityEevents) == DG_SUCCESS)
		{
			printf("HMICmdProc.c:commandProcessor():Received success res from Lidmodule \r\n");
			return DG_SUCCESS;
		}
		else
		{
			printf("HMICmdProc.c:commandProcessor():Received fail res from Lidmodule \r\n");
			return DG_FAIL;
		}

		break;
	case ACTUATOR_CTRL_REQ:
		dgactuatorCtrlCmd_t *actuatorCtrlCmd;

		*payloadSize = 0;
		actuatorCtrlCmd = (dgactuatorCtrlCmd_t*)&packetBuffer[PAYLOAD_START];
		switch(actuatorCtrlCmd->deviceID)
		{
		case HEATER_DEV:
			if(actuatorCtrlCmd->ctrlParam1 == DEVICE_ON)
			{
				HEATER_ON()
			}
			else if(actuatorCtrlCmd->ctrlParam1 == DEVICE_OFF)
			{
				HEATER_OFF()
			}
			return DG_SUCCESS;
			break;

/*		case SHD_MOTOR_DEV:
			if(actuatorCtrlCmd->ctrlParam1 == DEVICE_ROTATE_CW)
			{
//				SHREDDER_FORWARD();
//				SHREDDER_START();
			}
			else if(actuatorCtrlCmd->ctrlParam1 == DEVICE_ROTATE_CCW)
			{
//				SHREDDER_REVERSE();
//				SHREDDER_START();
			}
			else if(actuatorCtrlCmd->ctrlParam1 == DEVICE_OFF)
			{
				SHREDDER_STOP();
			}
			else
			{
				return DG_FAIL;
			}
			return DG_SUCCESS;
			break;*/
/*		case AUGER_MOTOR_DEV:
			if(actuatorCtrlCmd->ctrlParam1 == DEVICE_ROTATE_CW)
			{
				//Start motor in CWR
				ctMotorCwr();
			}
			else if(actuatorCtrlCmd->ctrlParam1 == DEVICE_ROTATE_CCW)
			{
				//Start motor in CWR
				ctMotorCcwr();
			}
			else if(actuatorCtrlCmd->ctrlParam1 == DEVICE_OFF)
			{
				MC1_CT_STOP();
			}
			else
			{
				return DG_FAIL;
			}
			return DG_SUCCESS;
			break;*/
/*		case FAN_MOTOR_DEV:
			if(actuatorCtrlCmd->ctrlParam1 == DEVICE_ROTATE_CW)
			{
				//Start motor in CWR
				MC1_CD_FORWARD();
				MC1_CD_START();
			}
			else if(actuatorCtrlCmd->ctrlParam1 == DEVICE_ROTATE_CCW)
			{
				//Start motor in CWR
				MC1_CD_REVERSE();
				MC1_CD_START();
			}
			else if(actuatorCtrlCmd->ctrlParam1 == DEVICE_OFF)
			{
				MC1_CD_STOP();
			}
			else
			{
				return DG_FAIL;
			}
			return DG_SUCCESS;
			break;*/
/*		case LID_ACTUATOR_DEV:
			if(actuatorCtrlCmd->ctrlParam1 == DEVICE_OPEN)
			{
				//Start motor in CWR
				lidOpen();
			}
			else if(actuatorCtrlCmd->ctrlParam1 == DEVICE_CLOSE)
			{
				//Start motor in CWR
				lidClose();
			}
			else if(actuatorCtrlCmd->ctrlParam1 == DEVICE_OFF)
			{
				lidStop();
			}
			else
			{
				return DG_FAIL;
			}
			return DG_SUCCESS;
			break;*/
/*		case CCV_MOTOR_DEV:
			if(actuatorCtrlCmd->ctrlParam1 == DEVICE_ROTATE_CW)
			{
				//Start motor in CWR
				MC2_CCV_FORWARD();
				MC2_CCV_START();
			}
			else if(actuatorCtrlCmd->ctrlParam1 == DEVICE_ROTATE_CCW)
			{
				//Start motor in CWR
				MC2_CCV_REVERSE();
				MC2_CCV_START();
			}
			else if(actuatorCtrlCmd->ctrlParam1 == DEVICE_OFF)
			{
				MC2_CCV_STOP();
			}
			else
			{
				return DG_FAIL;
			}
			return DG_SUCCESS;
			break;*/
/*		case AIR_RECIRC_VALVE:   //ST_MOTOR Control
			if(actuatorCtrlCmd->ctrlParam1 == DEVICE_ON)
			{
				AIR_RECIRC_VALVE_OPEN();
			}
			else if(actuatorCtrlCmd->ctrlParam1 == DEVICE_OFF)
			{
				AIR_RECIRC_VALVE_CLOSE();
			}
			else
			{
				return DG_FAIL;
			}
			return DG_SUCCESS;
			break;*/
/*		case SPRAYER_DEV:
			if(actuatorCtrlCmd->ctrlParam1 == DEVICE_ON)
			{
				//Param2 is the duration
				if(sprayOnce(HMICMDPROC_MOD, actuatorCtrlCmd->ctrlParam2) == DG_SUCCESS)
				{
					return DG_SUCCESS;
				}
				else
				{
					return DG_FAIL;
				}
			}
			else if(actuatorCtrlCmd->ctrlParam1 == DEVICE_OFF)
			{
				return DG_SUCCESS;
			}
			else
			{
				return DG_FAIL;
			}
			break;*/
/*		case ENZYME_VALVE:
			if(actuatorCtrlCmd->ctrlParam1 == DEVICE_ON)
			{
				ENZYME_VALVE_OPEN();
			}
			else if(actuatorCtrlCmd->ctrlParam1 == DEVICE_OFF)
			{
				ENZYME_VALVE_CLOSE();
			}
			else
			{
				return DG_FAIL;
			}
			return DG_SUCCESS;
			break;*/
/*		case SHREDDER_FLAP_VALVE:
			if(actuatorCtrlCmd->ctrlParam1 == DEVICE_OPEN)
			{
				flapOpen();
			}
			else if(actuatorCtrlCmd->ctrlParam1 == DEVICE_CLOSE)
			{
				flapClose();
			}
			else if(actuatorCtrlCmd->ctrlParam1 == DEVICE_OFF)
			{
				flapMotorStop();
			}
			else
			{
				return DG_FAIL;
			}
			return DG_SUCCESS;
			break;*/
		default:
			return DG_FAIL;
			break;
		}
		break;
	case ALARM_STATUS_REQ:
		break;
	case ALARM_CLEAR_REQ:
		break;
	case GET_ASB_VERSION:
		dgAsbVersion_t *absVersion;
		absVersion = (dgAsbVersion_t*)payload;
		*payloadSize = sizeof(dgAsbVersion_t);
		getFwVersion(absVersion->fwVersion);
		strcpy(absVersion->hwVersion, "HW V1.0.0");
		return DG_SUCCESS;
		break;
	case GET_ASB_DATETIME:
		dgDateTime_t *dateTime;
		dateTime = (dgDateTime_t*)payload;
		*payloadSize = sizeof(dgDateTime_t);
		getRTCtimeMMDDHHMM(dateTime);
		printf("HMICmdProc.c:CmdProc():GET_ASB_DATETIME:hour=%d, min=%d", dateTime->hour, dateTime->minute);
		return DG_SUCCESS;
		break;
	case GET_INT_REASON:
		intReason_t *reasonPayload;
		reasonPayload = (intReason_t*)payload;
		*payloadSize = sizeof(intReason_t);
		printf("HMICmdProc.c():GET_INT_REASON:CMD RECEIVED ");
		if(getIntReason() == REASON_AI_INF)
		{
			reasonPayload->reasonCode = REASON_AI_INF;
			reasonPayload->param = 0;
		}
		else
		{
			reasonPayload->reasonCode = REASON_ALARM_NOTIFICATION;
			reasonPayload->param = 0;
		}
		return DG_SUCCESS;
		break;
	case EXTAI_INF_OUTCOME:
	{
	    infOutcome_t *infPayload;

	    *payloadSize = 0;

	    infPayload = (infOutcome_t *)&packetBuffer[PAYLOAD_START];

	    printf("HMICmdProc.c:CmdProc():EXTAI_INF_OUTCOME:modelType: %d\r\n",
	           infPayload->modelType);

	    printf("HMICmdProc.c:CmdProc():EXTAI_INF_OUTCOME:inferenceResult: %d\r\n",
	           infPayload->inferenceResult);

	    printf("HMICmdProc.c:CmdProc():EXTAI_INF_OUTCOME:infOutcome: %d\r\n",
	           infPayload->infOutcome);

	    if ((infPayload->inferenceResult == INF_RESULT_SUCCESS) &&
	        (infPayload->infOutcome == CLASS_EMPTY))
	    {
	        event_lid_aiinferencecomplete(infPayload->infOutcome);
	    }

	    return DG_SUCCESS;
	}
	break;
	    break;
	case GET_RTSS_DATA:
		dgRtssPayload_t *rtssData;
		uint8_t sensorStatus, sensorModulestatus;
		dgCsmParam_t csmstate;
		rtssData = (dgRtssPayload_t*)payload;
		*payloadSize = sizeof(dgRtssPayload_t);
		//Update temperature and humidity
		if(getSensorStatus(HMICMDPROC_MOD, &(rtssData->curTemp), &(rtssData->curHumidity), &(rtssData->setTemp), &(rtssData->setHumidity), &sensorStatus, &sensorModulestatus) != DG_SUCCESS)
		{
			return DG_FAIL;
		}
		//Update actuator status
		rtssData->actuatorStatus = getActuatorStatus();

		//Update CSM status
		if(csmGetState(HMICMDPROC_MOD, &csmstate) == DG_SUCCESS)
		{
			rtssData->wasteCat = csmstate.wasteCat;
			rtssData->curPhase = csmstate.phase;
			rtssData->remainingDur = csmstate.remDur;
		}
		else
		{
			return DG_FAIL;
		}
		//Get current time
		getRTCtimeMMDDHHMM(&(rtssData->timeStamp));
		return DG_SUCCESS;
		break;
	case GET_SYSTEM_STATUS:
/*		dgSysStatusPayload_t *systemStatus;
		uint8_t sensorStatus, sensorModulestatus;
		dgCsmParam_t csmstate;
		systemStatus = (dgSysStatusPayload_t*)payload;
		*payloadSize = sizeof(dgSysStatusPayload_t);
		//Update temperature and humidity
		if(getSensorStatus(HMICMDPROC_MOD, &(systemStatus->curTemp), &(systemStatus->curHumidity), &(systemStatus->setTemp), &(systemStatus->setHumidity), &sensorStatus, &sensorModulestatus) != DG_SUCCESS)
		{
			return DG_FAIL;
		}
		//Update composting params

		//Update CSM status
		if(csmGetState(HMICMDPROC_MOD, &csmstate) == DG_SUCCESS)
		{
			dgCtProcessParam_t phaseParam;

			systemStatus->wasteCat = csmstate.wasteCat;
			systemStatus->curPhase = csmstate.phase;
			getCsmPhaseParam(csmstate.wasteCat, MESOPHILIC_PHASE, &phaseParam);
			//loadProcessParam(&phaseParam, csmstate.wasteCat, MESOPHILIC_PHASE);
			systemStatus->mPhaseDur = phaseParam.phaseDur;
			getCsmPhaseParam(csmstate.wasteCat, THERMOPHILIC_PHASE, &phaseParam);
			//loadProcessParam(&phaseParam, csmstate.wasteCat, THERMOPHILIC_PHASE);
			systemStatus->tPhaseDur = phaseParam.phaseDur;
			getCsmPhaseParam(csmstate.wasteCat, PATHOGEN_ELM_PHASE, &phaseParam);
			//loadProcessParam(&phaseParam, csmstate.wasteCat, PATHOGEN_ELM_PHASE);
			systemStatus->pPhaseDur = phaseParam.phaseDur;
			if(csmstate.phase == MESOPHILIC_PHASE)
			{
				systemStatus->mPhaseElapsedDur = csmstate.remDur ;
				systemStatus->tPhaseElapsedDur = 0;
				systemStatus->pPhaseElapsedDur = 0;
			}
			else if (csmstate.phase == THERMOPHILIC_PHASE)
			{
				systemStatus->mPhaseElapsedDur = systemStatus->mPhaseDur;
				systemStatus->tPhaseElapsedDur = csmstate.remDur;
				systemStatus->pPhaseElapsedDur = 0;
			}
			else
			{
				systemStatus->mPhaseElapsedDur = systemStatus->mPhaseDur;
				systemStatus->tPhaseElapsedDur = systemStatus->tPhaseDur;
				systemStatus->pPhaseElapsedDur = csmstate.remDur;
			}
		}*/
		return DG_SUCCESS;
		break;
	default:
		//Invalid command code
		*payloadSize = 0;
		return DG_INVALID_CMD;
		break;

	}
	return DG_FAIL;
}

//This method generates an interrupt to HMI board when there is something to be notified to HMI
//On getting the interrupt HMI will request for the information
int setHMI_interrupt()
{
	//Raise HW interrupt signal to HMI board


	//Inform hmiCmdProc that HMI interrupt has been raised and the reason
	//HMIProc will keep a timer for HMI to req for info. Otherwise inform healthcheck module


	return DG_SUCCESS;
}

static void hmiCmdProc_task(void *pvParameters)
{
	QueueHandle_t hmiCmdProcQHandle;
	dgMsg_t rcvMsg;					//Holds the currently received message
	static int hmiCmdProcState;		// This stores the state of HMI command processing
	//dgAsbComParam_t  *commBufferPtr;
	uint8_t packetBuffer[MAX_PACKET_SIZE];
	uint8_t packetSize;
	uint8_t payload[MAX_PAYLOAD_SIZE];
	uint8_t payloadSize;
	uint8_t rcvBuffer[MAX_TX_BUFFER_SIZE];
	int retValue;

	//Initialize UART Transaction Buffer
	HMItrBuf.txBufPtr = NULL;
	HMItrBuf.rxBufPtr = (char*)rcvBuffer;
	HMItrBuf.txState = RS232_TX_IDLE;
	HMItrBuf.rxState = RS232_RCV_IDLE;
	HMItrBuf.txBufSize =0;
	HMItrBuf.rxBufSize =0;
	HMItrBuf.txCount = 0;
	HMItrBuf.rxCount = 0;


	//Get the Qhandle for this task and store locally
	hmiCmdProcQHandle = getQHandle(HMICMDPROC_MOD);

	//Initialize the state of CT to IDLE
	hmiCmdProcState = HMICMDPROC_STATE_WAIT_FOR_CMD;

	uint8_t rcvNumberCur=0,rcvNumberPrev=0;

	while(1)
	{
		//Receive event from Queue. Block until event is available
		if(xQueueReceive(hmiCmdProcQHandle, &rcvMsg, portMAX_DELAY ) != pdPASS )
		{
			// Queue did not return an event. Hence go back
			continue;
		}
		printf("HMICmdProc.c:hmiCmdProc_task():Received cmd:%d, in state:%d\r\n", rcvMsg.command,hmiCmdProcState );
		switch(hmiCmdProcState)
		{
		case HMICMDPROC_STATE_IDLE:
			break;
		case HMICMDPROC_STATE_WAIT_FOR_CMD:
			int i,j;
			uint8_t a;
			char printBuff[64];
			switch(rcvMsg.command)
			{
			case HMICMD_BUFFER_RECEIVED:
				//Get the commBuffer from the rcvMsg
				//commBufferPtr = (dgAsbComParam_t*)rcvMsg.cmdParam;
				j=0;
				for(i=0; i<HMItrBuf.rxCount; i++)
				{
					a = ((rcvBuffer[i]>>4)&0x0F);
					printBuff[j++]= ((a <=9)? (a+0x30): (a-0x0A+0x41));
					a = ((rcvBuffer[i])&0x0F);
					printBuff[j++]= ((a <=9)? (a+0x30): (a-0x0A+0x41));
					printBuff[j++]= ',';
					if(j>=63)
						break;
				}
				printBuff[j]= 0;
				printf("HMICmdProc.c:hmiCmdProc_task():received packet:%s\r\n", printBuff);
				//Remove Esc sequences
//				if(stripEsc(commBufferPtr->rxBuffer, commBufferPtr->rxBufferSize, packetBuffer, &packetSize) != DG_SUCCESS)
				if(stripEsc(rcvBuffer, HMItrBuf.rxCount, packetBuffer, &packetSize) != DG_SUCCESS)
				{
					//To be handled by sending error response to HMI
					//Improper packet
					printf("HMICmdProc.c:hmiCmdProc_task(): Packet rcd with improper format\r\n");
					payloadSize = 0;
					createAndSendResponse2HMI(CMD_RCVD_WITH_ERROR, packetBuffer, payload, payloadSize);
					break;
				}
				//Verify Checksum
				if(verifyCheksum(packetBuffer, packetSize) != DG_SUCCESS)
				{
					//Checksum Error.
					printf("HMICmdProc.c:hmiCmdProc_task():Packet rcd with checksum error;Bytes=0x%X,0x%X,0x%X,0x%X,0x%X,0x%X\r\n",packetBuffer[0],packetBuffer[1],packetBuffer[2],packetBuffer[3],packetBuffer[4],packetBuffer[5] );
					payloadSize = 0;
					createAndSendResponse2HMI(CMD_RCVD_WITH_ERROR, packetBuffer, payload, payloadSize);
					break;
				}
				//Checksum passed
				rcvNumberCur = packetBuffer[SEQUENCE_BYTE];
				retValue = commandProcessor(packetBuffer, packetSize, payload, &payloadSize);
				if(retValue == DG_SUCCESS)
				{
					//Command has been executed successfully
					createAndSendResponse2HMI(CMD_EXECUTION_SUCCESS, packetBuffer, payload, payloadSize);

				}
				else if(retValue == DG_INVALID_CMD)
				{
					createAndSendResponse2HMI(CMD_INVALID, packetBuffer, payload, payloadSize);
				}
				else
				{
					createAndSendResponse2HMI(CMD_EXECUTION_ERROR, packetBuffer, payload, payloadSize);
				}
				hmiCmdProcState = HMICMDPROC_STATE_SENDING_RESPONSE;
				if((rcvNumberCur>0) && (rcvNumberCur == rcvNumberPrev ))
				{
					//duplicate packet
					printf("HMICmdProc.c:hmiCmdProc_task(): Duplicate rcv buffer");
				}
				rcvNumberPrev = rcvNumberCur;
				break;
			case HMICMD_RESPONSE_SENT:
				//Don't expect this event in this state. Ignore
				break;
			case GEN_HMI_INTERRUPT:
				//To  be handled.
				break;
			case DG_TIMER_EXPIRY:
				//Timer expiry means no Keep-alive. Inform System Health Module
				printf("HMICmdProc.c:hmiCmdProc_task(): DG_TIMER_EXPIRY event in HMICMDPROC_STATE_WAIT_FOR_CMD.\r\n");
				break;
			}
			break;
		case HMICMDPROC_STATE_PROCESSING:
			break;
		case HMICMDPROC_STATE_SENDING_RESPONSE:
			switch(rcvMsg.command)
			{
			case HMICMD_BUFFER_RECEIVED:
				//Don't expect this event in this state. Ignore
				break;
			case HMICMD_RESPONSE_SENT:
				// Response to HMI has been sent. We can configure UART to receive command again
				setRxStatusHmiCmdProc(RS232_RCV_IDLE);
				hmiCmdProcState = HMICMDPROC_STATE_WAIT_FOR_CMD;
				break;
			case GEN_HMI_INTERRUPT:
				//To  be handled.
				break;
			case DG_TIMER_EXPIRY:
				//Ignore. Don't expect the event here
				printf("HMICmdProc.c:hmiCmdProc_task(): DG_TIMER_EXPIRY event in HMICMDPROC_STATE_SENDING_RESPONSE.\r\n");
				break;
			}
			break;
		default:
			//Invalid state
			printf("HMICmdProc.c:hmiCmdProc_task(): Unkown state:%d. Ignored)\r\n",hmiCmdProcState);
			break;
		}

	}
}





int initHMICmdProc(void)
{
	//Create HMICmdProc task task
	TaskHandle_t hmiCmdProcTaskHandle;
	TimerHandle_t hmiCmdProcTimerHandle;
	BaseType_t result;

	result = xTaskCreate(hmiCmdProc_task, "hmiCmdProc_task", configMINIMAL_STACK_SIZE + 1200, NULL, task_PRIORITY, &hmiCmdProcTaskHandle);
    if ( result !=    pdPASS)
    {
        PRINTF("hmiCmdProc_task creation failed!.\r\n");
        return DG_FAIL;
    }
    //HMI CMD Proc requires single shot timer and hence create FreeRTOS SW timer
    hmiCmdProcTimerHandle = xTimerCreate("HMICmdProcTimer",100, pdFALSE, (void*)HMICMDPROC_MOD, dgTimerCallback);
    if(hmiCmdProcTimerHandle == NULL)
    {
        PRINTF("Timer creation failed!.\r\n");
    	vTaskDelete(hmiCmdProcTaskHandle);
        return DG_FAIL;
    }
    if(registerModule(HMICMDPROC_MOD, hmiCmdProcTaskHandle, hmiCmdProcTimerHandle)!= DG_SUCCESS)
    {
    	//Registering the module failed. Hence kill the task and return error
        PRINTF("hmiCmdProc_task registration failed!.\r\n");
        xTimerDelete(hmiCmdProcTimerHandle,100 / portTICK_PERIOD_MS);
    	vTaskDelete(hmiCmdProcTaskHandle);

    	return DG_FAIL;
    }
    return DG_SUCCESS;
}
