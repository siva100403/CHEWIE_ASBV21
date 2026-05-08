/*
 * cliProc.c
 *
 *  Created on: 23-Nov-2024
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
#include "motorControl.h"
#include "seqControlCommon.h"
#include "dgI2cDriver.h"
#include "sht40Driver.h"
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
#include "motorControl.h"
#include "sensorMod.h"
#include "sensorModAPI.h"
#include "augerAPI.h"
#include "shredder.h"
#include "shredderAPI.h"
#include "adcs.h"
#include "hatcsMod.h"
#include "limitSwitchMod.h"
#include "transferCS.h"
#include "transferCSAPI.h"
#include "lidModule.h"
#include "lidModuleAPI.h"
#include "HMICmdProc.h"
#include "HMICmdProcAPI.h"
#include "csmMod.h"
#include "csmModAPI.h"
#include "measure.h"
#include "hatcsMod.h"


/*******************************************************************************
 * Global Variables
 ******************************************************************************/

//Buffer used to communicate between CLI processing module and the RS232 ISR

dgRs232TrBuf_t trBuf;


/*******************************************************************************
 * Implementation
 ******************************************************************************/

/************************Function Header: getNextToken ****************************
 * This functions extracts tokens delimited by space or , in the command
 * Input Parameters:
 *   cmd_buffer - is the char array pointer pointing to the beginning of the command buffer
 *   token - is the pointer to the char array where the extracted token will be placed.
 *           This will be a null terminated string
 *   bufPtr - is an integer that points to the current char being processed. This will be
 *           updated by the function at the end. The same value should be passed for next call
 * Return value:
 *    DG_SUCCESS - if the function could identify a token
 *    DG_FAIL - if could not identify a token or reached end of command buffer
 *
 *********************************************************************************************/
int getNextToken(char *cmd_buf, char *token, int *bufptr)
{
	int i=0;

//	Remove leading white spaces
	while ( (cmd_buf[*bufptr]=='\t')||(cmd_buf[*bufptr]==' ') )
	{
		(*bufptr)++;
	}
// Extract token i.e till encounter a white space or \r\n
	for (i=0; i<CMD_MAX_SIZE; i++)
	{
		if((cmd_buf[i+(*bufptr)]=='\r') || (cmd_buf[i+(*bufptr)]=='\t')||(cmd_buf[i+(*bufptr)]==' ') )
		{
			break;
		}
		token[i]=cmd_buf[i+(*bufptr)];
	}
	if (i==0)
	{
		return (DG_FAIL);    // No characters found other than whitespace
	}
	else
	{
		token[i]= 0x00;  //add NULL at the end for strcmp to work
		*bufptr += i;
		return (DG_SUCCESS);
	}
}

/************************Function Header: getCmdCode ***************************************
 * This functions convert command string to command code so that it can be used in switch
 * statement.
 * Input Parameters:
 *   token - char array that contains only the command without parameter
 * Return value:
 *    cmd code - if it can identify a command
 *   -1 - if it is not a valid command
 *
 *********************************************************************************************/

int getCmdCode(char* token)
{
	int cmdCode;
	if (strcmp(&token[0], "CSMSTART\0")==0)
	{
		cmdCode = CSMSTART;
	}
	else if (strcmp(&token[0], "CSMSTOP\0")==0)
	{
		cmdCode = CSMSTOP;
	}
	else if (strcmp(&token[0], "GETCSMSTATUS_CHTL\0")==0)
	{
		cmdCode = GETCSMSTATUS_CHTL;
	}
	else if (strcmp(&token[0], "GETSENSORSTATUS_CHTL\0")==0)
	{
		cmdCode = GETSENSORSTATUS_CHTL;
	}
	else if (strcmp(&token[0], "GETSYSCFGVER_C\0")==0)
	{
		cmdCode = GETSYSCFGVER_C;
	}
	else if (strcmp(&token[0], "GETAUGERCFG_C\0")==0)
	{
		cmdCode = GETAUGERCFG_C;
	}
	else if (strcmp(&token[0], "SETAUGERCFG_C\0")==0)
	{
		cmdCode = SETAUGERCFG_C;
	}
	else if (strcmp(&token[0], "GETHATCSCFG_C\0")==0)
	{
		cmdCode = GETHATCSCFG_C;
	}
	else if (strcmp(&token[0], "SETHATCSCFG_C\0")==0)
	{
		cmdCode = SETHATCSCFG_C;
	}
	else if (strcmp(&token[0], "GETCSMCFG_C\0")==0)
	{
		cmdCode = GETCSMCFG_C;
	}
	else if (strcmp(&token[0], "SETCSMCFG_C\0")==0)
	{
		cmdCode = SETCSMCFG_C;
	}
	else if (strcmp(&token[0], "GETTRFRCFG_C\0")==0)
	{
		cmdCode = GETTRFRCFG_C;
	}
	else if (strcmp(&token[0], "SETTRFRCFG_C\0")==0)
	{
		cmdCode = SETTRFRCFG_C;
	}
	else if (strcmp(&token[0], "SETSHDSEQ_C\0")==0)
	{
		cmdCode = SETSHDSEQ_C;
	}
	else if (strcmp(&token[0], "GETSHDSEQ_C\0")==0)
	{
		cmdCode = GETSHDSEQ_C;
	}
	else if (strcmp(&token[0], "SETSHDTVAR_C\0")==0)
	{
		cmdCode = SETSHDTVAR_C;
	}
	else if (strcmp(&token[0], "GETSHDTVAR_C\0")==0)
	{
		cmdCode = GETSHDTVAR_C;
	}
	else if (strcmp(&token[0], "SAVECONFIGEE_C\0")==0)
	{
		cmdCode = SAVECONFIGEE_C;
	}
	else if (strcmp(&token[0], "GETADCSCFG_C\0")==0)
	{
		cmdCode = GETADCSCFG_C;
	}
	else if (strcmp(&token[0], "SETADCSCFG_C\0")==0)
	{
		cmdCode = SETADCSCFG_C;
	}
	else if (strcmp(&token[0], "FD_CONFIG\0")==0)
	{
		cmdCode = FD_CONFIG;
	}
	else if (strcmp(&token[0], "HEATER\0")==0)
	{
		cmdCode = HEATER;
	}
	else if (strcmp(&token[0], "ADCS\0")==0)
	{
		cmdCode = ADCS;
	}
	else if (strcmp(&token[0], "SHDCS\0")==0)
	{
		cmdCode = SHDCS;
	}
	else if (strcmp(&token[0], "DCSPRAYER\0")==0)
	{
		cmdCode = DC_SPRAYER;
	}
	else if (strcmp(&token[0], "FLUSHSPRAYER\0")==0)
	{
		cmdCode = FLUSH_SPRAYER;
	}
	else if (strcmp(&token[0], "ADDITIVE\0")==0)
	{
		cmdCode = ADDITIVE_DISPR;
	}
	else if (strcmp(&token[0], "FLAPMOTOR\0")==0)
	{
		cmdCode = FLAP_MOTOR;
	}
	else if (strcmp(&token[0], "STMOTOR\0")==0)
	{
		cmdCode = ST_MOTOR;
	}
	else if (strcmp(&token[0], "AIRVALVE1\0")==0)
	{
		cmdCode = AIR_VALVE1;
	}
	else if (strcmp(&token[0], "AIRVALVE2\0")==0)
	{
		cmdCode = AIR_VALVE2;
	}
	else if (strcmp(&token[0], "AIRVALVE3\0")==0)
	{
		cmdCode = AIR_VALVE3;
	}
	else if (strcmp(&token[0], "MOTOR1\0")==0)
	{
		cmdCode = MOTOR1;
	}
	else if (strcmp(&token[0], "MOTOR2\0")==0)
	{
		cmdCode = MOTOR2;
	}
	else if (strcmp(&token[0], "GETTEMP\0")==0)
	{
		cmdCode = GETTEMP;
	}
	else if (strcmp(&token[0], "FAN\0")==0)
	{
		cmdCode = FAN_MOTOR;
	}
	else if (strcmp(&token[0], "AUGER\0")==0)
	{
		cmdCode = AUGER_MOTOR;
	}
	else if (strcmp(&token[0], "GETTIME\0")==0)
	{
		cmdCode = GETTIME;
	}
	else if (strcmp(&token[0], "SETTIME\0")==0)
	{
		cmdCode = SETTIME;
	}
	else if (strcmp(&token[0], "SHD\0")==0)
	{
		cmdCode = SHD_MOTOR;
	}
	else if (strcmp(&token[0], "GETCSMSTATUS\0")==0)
	{
		cmdCode = GETCSMSTATUS;
	}
	else if (strcmp(&token[0], "ADD_WASTE_START\0")==0)
	{
		cmdCode = ADD_WASTE_START;
	}
	else if (strcmp(&token[0], "ADD_WASTE_END\0")==0)
	{
		cmdCode = ADD_WASTE_END;
	}
	else if(strcmp(&token[0], "GETSENSORSTATUS\0")==0)
	{
		cmdCode = GETSENSORSTATUS;
	}
	else if(strcmp(&token[0], "CTCS\0")==0)
	{
		cmdCode = CTCS;
	}
	else if(strcmp(&token[0], "LID\0")==0)
	{
		cmdCode = LID;
	}
	else if(strcmp(&token[0], "TCS\0")==0)
	{
		cmdCode = TCS;
	}
	else if(strcmp(&token[0], "SHDAUG_MOTOR\0")==0)
	{
		cmdCode = SHDAUG_MOTOR;
	}
	else if(strcmp(&token[0], "SHDAUG_MOTORSPEED\0")==0)
	{
		cmdCode = SHDAUG_MOTOR_SPEED;
	}
	else if(strcmp(&token[0], "SPARE2_RELAY\0")==0)
	{
		cmdCode = SPARE2_RELAY;
	}
	else if(strcmp(&token[0], "GETLS_STATUS\0")==0)
	{
		cmdCode = GETLS_STATUS;
	}
	else if(strcmp(&token[0], "MKVALVE\0")==0)
	{
		cmdCode = MKVALVE;
	}
	else
	{
		cmdCode = -1;
	}
	return (cmdCode);
}


/*This function informs CliDriver to send the response over CLI
*/

void sendCliResponse(char* response, uint8_t size)
{

	if ((response == NULL)|| (size == 0))
	{
		//Input parameter validation
		return;
	}
	//Disable CLI LPUART interrupt - we are going to access the common data structure
	DisableIRQ(CLI_LPUART_IRQn);

	trBuf.txBufPtr = response;
	trBuf.txState = RS232_TX_INPROGRESS;
	trBuf.txBufSize =size;


	//send the first byte
	LPUART_WriteByte(CLI_LPUART,trBuf.txBufPtr[0]);
	trBuf.txCount = 1;

	//enable interrupt
    LPUART_EnableInterrupts(CLI_LPUART, LPUART_TX_INT_MASK);
	NVIC_SetPriority(CLI_LPUART_IRQn, 4);
	EnableIRQ(CLI_LPUART_IRQn);
}

void setRxStatus(uint8_t status)
{
	DisableIRQ(CLI_LPUART_IRQn);
	trBuf.rxState = status;
	NVIC_SetPriority(CLI_LPUART_IRQn, 4);
	EnableIRQ(CLI_LPUART_IRQn);
}

void cli_Task(void* arg)
{

	QueueHandle_t cliQHandle;
	dgMsg_t rcvMsg;				// Holds the currently received message
//	static int cliState;		// This stores the status of CLI Processing

	char cmdString[CMD_MAX_SIZE];
	char response[CLI_RESP_MAX_SIZE];
	char token[20];
	int bufptr, cmdCode;
	char version[12];


	//Initialize CLI Buffer


	trBuf.txBufPtr = NULL;
	trBuf.rxBufPtr = cmdString;
	trBuf.txState = RS232_TX_IDLE;
	trBuf.rxState = RS232_RCV_IDLE;
	trBuf.txBufSize =0;
	trBuf.rxBufSize =0;
	trBuf.txCount = 0;
	trBuf.rxCount = 0;

	//Get the Qhandle for this task and store locally
	cliQHandle = getQHandle(CLI_MOD);

	//Print version number and board details
	getFwVersion(&version[0]);

	strcpy(response, "\r\n\r\n****************  ASB V2.2.1 CLI  ****************\r\n\r\n");
	sendCliResponse(response, strlen(response));
	vTaskDelay(100);
	strcpy(response, "************      FW version:");
	strcat(response, version);
	strcat(response, "     ************\r\n>");
	sendCliResponse(response, strlen(response));
	vTaskDelay(100);

	while (1)
	{
		//Receive event from Queue. Block until event is available
		if(xQueueReceive(cliQHandle, &rcvMsg, portMAX_DELAY ) != pdPASS )
		{
			// Queue did not return an event. Hence go back
			continue;
		}

		switch(rcvMsg.command)
		{
		case CLI_CMD:

			bufptr = 0;  //For every new command received this should be made 0

			if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Get command word
			{

				//Send error response
				strcpy(response, "CERROR:Command not supported\r\n>");
				sendCliResponse(response, strlen(response));
				setRxStatus(RS232_RCV_IDLE);
				continue;
			}

			// Convert command string to command code
			cmdCode = getCmdCode(token);
			if (cmdCode == -1)
			{
				//Send error response
				strcpy(response, "CERROR:Command not supported\r\n>");
				sendCliResponse(response, strlen(response));
				setRxStatus(RS232_RCV_IDLE);
				continue;
			}

			switch (cmdCode)
			{
				case HEATER:
				{
					//HEATER ON/OFF

					// Extract from command string
					if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Extract Heater number
					{
						strcpy(response, "CERROR:Less Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					if (strcmp(&token[0], "ON\0")==0)
					{
						HEATER_ON();
					}
					else if (strcmp(&token[0], "OFF\0")==0)
					{
						HEATER_OFF();
					}
					else
					{
						strcpy(response, "CERROR:Invalid Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}

					strcpy(response, "CC:\r\n>");
					sendCliResponse(response, strlen(response));
					setRxStatus(RS232_RCV_IDLE);
					break;
				}

				case DC_SPRAYER:
				{
					//DCSPRAYER ON/OFF/ONCE

					// Extract from command string
					if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Extract Heater number
					{
						strcpy(response, "CERROR:Less Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					if (strcmp(&token[0], "ON\0")==0)
					{
						DCsprayerOn();
					}
					else if (strcmp(&token[0], "OFF\0")==0)
					{
						DCsprayerOff();
					}
					else if (strcmp(&token[0], "ONCE\0")==0)
					{
						uint16_t duration;
						// Extract duration
						if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Extract Heater number
						{
							strcpy(response, "CERROR:Less Parameters\r\n>");
							sendCliResponse(response, strlen(response));
							setRxStatus(RS232_RCV_IDLE);
							break;
						}
						duration = atoi(token);
						if(duration == 0)
						{
							strcpy(response, "CERROR:Invalid Parameter\r\n>");
							sendCliResponse(response, strlen(response));
							setRxStatus(RS232_RCV_IDLE);
							break;
						}
						 (duration);
					}
					else
					{
						strcpy(response, "CERROR:Invalid Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					strcpy(response, "CC:\r\n>");
					sendCliResponse(response, strlen(response));
					setRxStatus(RS232_RCV_IDLE);
					break;

				}
				case FLUSH_SPRAYER:
				{
					//FLUSHSPRAYER ON/OFF

					// Extract from command string
					if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Extract Heater number
					{
						strcpy(response, "CERROR:Less Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					if (strcmp(&token[0], "ON\0")==0)
					{
						flushSprayerOn();
					}
					else if (strcmp(&token[0], "OFF\0")==0)
					{
						flushSprayerOff();
					}
					else
					{
						strcpy(response, "CERROR:Invalid Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					strcpy(response, "CC:\r\n>");
					sendCliResponse(response, strlen(response));
					setRxStatus(RS232_RCV_IDLE);
					break;

				}
				case ST_MOTOR:
				{
					//STMOTOR RR/RL/OFF

					// Extract from command string
					if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Extract Heater number
					{
						strcpy(response, "CERROR:Less Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					if (strcmp(&token[0], "RR\0")==0)
					{
						stMotorCWR();
					}
					if (strcmp(&token[0], "RL\0")==0)
					{
						stMotorCCWR();
					}
					else if (strcmp(&token[0], "OFF\0")==0)
					{
						stMotorStop();
					}
					else
					{
						strcpy(response, "CERROR:Invalid Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					strcpy(response, "CC:\r\n>");
					sendCliResponse(response, strlen(response));
					setRxStatus(RS232_RCV_IDLE);
					break;

				}
				case ADDITIVE_DISPR:
				{
					//ADDITIVE START/STOP

					// Extract from command string
					if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Extract Heater number
					{
						strcpy(response, "CERROR:Less Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					if (strcmp(&token[0], "OPEN\0")==0)
					{
						additiveDispenseOn();
					}
					else if (strcmp(&token[0], "CLOSE\0")==0)
					{
						additiveDispenseOff();
					}
					else
					{
						strcpy(response, "CERROR:Invalid Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}

					strcpy(response, "CC:\r\n>");
					sendCliResponse(response, strlen(response));
					setRxStatus(RS232_RCV_IDLE);
					break;
				}

				case CTCS:
				{
					//CTCS START/STOP

					// Extract from command string
					if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Extract Heater number
					{
						strcpy(response, "CERROR:Less Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					if (strcmp(&token[0], "START\0")==0)
					{
						ctStart(CLI_MOD);
					}
					else if (strcmp(&token[0], "STOP\0")==0)
					{
						ctStop(CLI_MOD);
					}
					else
					{
						strcpy(response, "CERROR:Invalid Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}

					strcpy(response, "CC:\r\n>");
					sendCliResponse(response, strlen(response));
					setRxStatus(RS232_RCV_IDLE);
					break;
				}

				case FLAP_MOTOR:
				{
					//FLAP OPEN/CLOSE/OFF

					// Extract from command string
					if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Extract Heater number
					{
						strcpy(response, "CERROR:Less Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					if (strcmp(&token[0], "OPEN\0")==0)
					{
						flapMotorCWR();
					}
					else if (strcmp(&token[0], "CLOSE\0")==0)
					{
						flapMotorCCWR();
					}
					else if (strcmp(&token[0], "OFF\0")==0)
					{
						flapMotorStop();
					}
					else
					{
						strcpy(response, "CERROR:Invalid Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}

					strcpy(response, "CC:\r\n>");
					sendCliResponse(response, strlen(response));
					setRxStatus(RS232_RCV_IDLE);
					break;
				}
				case AIR_VALVE1:
				{
					//AIR_VALVE1 OPEN/CLOSE

					// Extract from command string
					if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Extract Heater number
					{
						strcpy(response, "CERROR:Less Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					if (strcmp(&token[0], "OPEN\0")==0)
					{
						airValve1On();
					}
					else if (strcmp(&token[0], "CLOSE\0")==0)
					{
						airValve1Off();
					}
					else
					{
						strcpy(response, "CERROR:Invalid Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}

					strcpy(response, "CC:\r\n>");
					sendCliResponse(response, strlen(response));
					setRxStatus(RS232_RCV_IDLE);
					break;
				}
				case AIR_VALVE2:
				{
					//AIR_VALVE2 OPEN/CLOSE

					// Extract from command string
					if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Extract Heater number
					{
						strcpy(response, "CERROR:Less Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					if (strcmp(&token[0], "OPEN\0")==0)
					{
						airValve2On();
					}
					else if (strcmp(&token[0], "CLOSE\0")==0)
					{
						airValve2Off();
					}
					else
					{
						strcpy(response, "CERROR:Invalid Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}

					strcpy(response, "CC:\r\n>");
					sendCliResponse(response, strlen(response));
					setRxStatus(RS232_RCV_IDLE);
					break;
				}
				case AIR_VALVE3:
				{
					//AIR_VALVE3 OPEN/CLOSE

					// Extract from command string
					if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Extract Heater number
					{
						strcpy(response, "CERROR:Less Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					if (strcmp(&token[0], "OPEN\0")==0)
					{
						airValve3On();
					}
					else if (strcmp(&token[0], "CLOSE\0")==0)
					{
						airValve3Off();
					}
					else
					{
						strcpy(response, "CERROR:Invalid Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}

					strcpy(response, "CC:\r\n>");
					sendCliResponse(response, strlen(response));
					setRxStatus(RS232_RCV_IDLE);
					break;
				}


				case SPARE2_RELAY:
				{
					//SPARE2_RELAY ON/OFF

					// Extract from command string
					if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Extract Heater number
					{
						strcpy(response, "CERROR:Less Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					if (strcmp(&token[0], "ON\0")==0)
					{
						SPARE2_RELAY_ON();
					}
					else if (strcmp(&token[0], "OFF\0")==0)
					{
						SPARE2_RELAY_OFF();
					}
					else
					{
						strcpy(response, "CERROR:Invalid Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}

					strcpy(response, "CC:\r\n>");
					sendCliResponse(response, strlen(response));
					setRxStatus(RS232_RCV_IDLE);
					break;
				}

				case LID:
				{
					//Usage: LID OPEN/CLOSE

					dgProximityEvents_t event;
					// Extract from command string
					if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Extract Heater number
					{
						strcpy(response, "CERROR:Less Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					if (strcmp(&token[0], "OPEN\0")==0)
					{
						event.proximity = CAP_PROXIMITY_EVENT_DETECTED;

					}
					else if (strcmp(&token[0], "CLOSE\0")==0)
					{
						event.proximity = CAP_PROXIMITY_EVENT_REMOVED;
					}
					else
					{
						strcpy(response, "CERROR:Invalid Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					sendProximityEvent(&event);
					strcpy(response, "CC:\r\n>");
					sendCliResponse(response, strlen(response));
					setRxStatus(RS232_RCV_IDLE);
					break;
				}

				case TCS:
				{
					//Usage: TCS START/STOP

					uint16_t duration;

					dgProximityEvents_t event;
					// Extract from command string
					if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Extract command
					{
						strcpy(response, "CERROR:Less Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					if (strcmp(&token[0], "START\0")==0)
					{
						if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Extract transfer duration
						{
							strcpy(response, "CERROR:Less Parameters\r\n>");
							sendCliResponse(response, strlen(response));
							setRxStatus(RS232_RCV_IDLE);
							break;
						}
						duration = atoi(token);
						if(duration == 0)
						{
							strcpy(response, "CERROR:Invalid Parameter\r\n>");
							sendCliResponse(response, strlen(response));
							setRxStatus(RS232_RCV_IDLE);
							break;
						}
						transferStart(CLI_MOD, duration);

					}
					else if (strcmp(&token[0], "STOP\0")==0)
					{
						transferAbort(CLI_MOD);
					}
					else
					{
						strcpy(response, "CERROR:Invalid Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					//sendProximityEvent(&event);
					strcpy(response, "CC:\r\n>");
					sendCliResponse(response, strlen(response));
					setRxStatus(RS232_RCV_IDLE);
					break;
				}




				case FAN_MOTOR:
				{
					//FAN RR/RL/OFF

					// Extract from command string
					if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Extract Heater number
					{
						strcpy(response, "CERROR:Less Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					if (strcmp(&token[0], "RL\0")==0)
					{
						fanMotorCWR();
					}
					else if (strcmp(&token[0], "RR\0")==0)
					{
						fanMotorCCWR();
					}
					else if (strcmp(&token[0], "OFF\0")==0)
					{
						fanMotorStop();
					}
					else
					{
						strcpy(response, "CERROR:Invalid Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}

					strcpy(response, "CC:\r\n>");
					sendCliResponse(response, strlen(response));
					setRxStatus(RS232_RCV_IDLE);
					break;
				}

				case GETTEMP:
				{
					//This command reads temperature and humidity and display the value
					//usage: GETTEMP
					float temp, humidity;
					if(readShtTempHumidityHighPrecision(&temp, &humidity) == DG_SUCCESS)
					{
						sprintf(response, "CC:Temp=%f, Humidity=%f\r\n>", temp, humidity);
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					else
					{
						strcpy(response, "CERROR:Error in execution\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
				}

				case GETSENSORSTATUS:
				{
					//This command gets the status of sensor module
					//usage: GETSENSORSTATUS
					float temp, humidity, sT, sH;
					uint8_t modulestatus, sensorstatus;
					if(getSensorStatus(CLI_MOD, &temp, &humidity,&sT, &sH, &sensorstatus, &modulestatus) == DG_SUCCESS)
					{
						sprintf(response, "CC:Module State=%d, Sensor State=%d, Temp=%f, Humidity=%f, sT=%f, sH=%f\r\n>", modulestatus, sensorstatus,temp, humidity, sT, sH);
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					else
					{
						strcpy(response, "CERROR:Error in execution\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
				}
				case GETSENSORSTATUS_CHTL:
				{
					//This command gets the status of sensor module
					//usage: GETSENSORSTATUS_CHTL
					float temp, humidity, sT, sH;
					uint8_t modulestatus, sensorstatus;
					if(getSensorStatus(CLI_MOD, &temp, &humidity,&sT, &sH, &sensorstatus, &modulestatus) == DG_SUCCESS)
					{
						sprintf(response, "CC:%d,%f,%f,%f,%f\r\n>", sensorstatus,temp, humidity, sT, sH);
						printf("cliProc.c:GETSENSORSTATUS_CHTL, %s\r\n", response);
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						printf("cliProc.c:():GETSENSORSTATUS_CHTL cmd success\r\n");
						break;
					}
					else
					{
						strcpy(response, "CE:\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						printf("cliProc.c:():GETSENSORSTATUS_CHTL cmd fail\r\n");
						break;
					}
				}
				case GETTIME:
				{
					//This command reads the current time from RTC and display
					//usage: GETTIME
					char time[32];
					if(getRTCtime(time) == DG_SUCCESS)
					{
						sprintf(response, "CC:Current Time=%s\r\n>", time);
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					else
					{
						strcpy(response, "CERROR:Error in execution\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
				}

				case SETTIME:
				{
					//This command reads the current time from RTC and display
					//usage: SETTIME YY MM DD HH MM SS

					if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Extract year
					{
						strcpy(response, "CERROR:Less Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					uint8_t year=atoi(&token[0]);
					if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Extract month
					{
						strcpy(response, "CERROR:Less Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					uint8_t month=atoi(&token[0]);
					if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Extract date
					{
						strcpy(response, "CERROR:Less Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					uint8_t date=atoi(&token[0]);
					if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Extract hour
					{
						strcpy(response, "CERROR:Less Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					uint8_t hour=atoi(&token[0]);
					if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Extract minutes
					{
						strcpy(response, "CERROR:Less Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					uint8_t min=atoi(&token[0]);
					if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Extract seconds
					{
						strcpy(response, "CERROR:Less Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					uint8_t sec=atoi(&token[0]);
					if(setrtctime(year,month,date,hour,min,sec)==DG_SUCCESS)
					{
						sprintf(response, "CC:r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
					}
					else
					{
						strcpy(response, "CERROR:Error in execution\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
					}
					break;
				}
				case GETLS_STATUS:
				{
					//This command reads the all the limit switch status and displays
					//usage: GETLS_STATUS
					uint8_t s1, s2, s3;
					s1 = getLidSwicthStatus();
					s2 = getStorageTraySwicthStatus();
					s3 = getFlapStatus();
					sprintf(response, "CC:lid=%d, st=%d, flap=%d\r\n>", s1, s2,s3);
					sendCliResponse(response, strlen(response));
					setRxStatus(RS232_RCV_IDLE);
					break;
				}

				case CSMSTART:
				{
					uint8_t phase, wasteCat, csmState;
					uint16_t remainingDuration;

					//This command starts CSM state machine
					//usage: CSMSTART phase remainingDuration wasteCat
					if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Get CSM Phase
					{
						strcpy(response, "CERROR:Less Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					if (strcmp(&token[0], "MPHASE\0")==0)
					{
						phase = MESOPHILIC_PHASE;
						csmState = CSM_STATE_MPHASE;
					}
					else if (strcmp(&token[0], "TPHASE\0")==0)
					{
						phase = THERMOPHILIC_PHASE;
						csmState = CSM_STATE_TPHASE;
					}
					else if (strcmp(&token[0], "PPHASE\0")==0)
					{
						phase = PATHOGEN_ELM_PHASE;
						csmState = CSM_STATE_PPHASE;
					}
					else if (strcmp(&token[0], "DPHASE\0")==0)
					{
						phase = DRYING_PHASE;
						csmState = CSM_STATE_I_DRY;
					}
					else
					{
						strcpy(response, "CERROR:Invalid Phase\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}

					if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Get remaining Duration
					{
						strcpy(response, "CERROR:Less Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					remainingDuration=atoi(&token[0]);

					if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Get wasteCat
					{
						strcpy(response, "CERROR:Less Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					wasteCat=atoi(&token[0]);
					if(wasteCat>=WASTE_CAT_MAX)
					{
						strcpy(response, "CERROR:Invalid Waste Category\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					if(csmStart(CLI_MOD, csmState, phase, remainingDuration, wasteCat) == DG_SUCCESS)
					{
						sprintf(response, "CC:\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
					}
					else
					{
						strcpy(response, "CERROR:Error in execution\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
					}
					break;
				}
				case CSMSTOP:
				{
					if(csmStop(CLI_MOD) == DG_SUCCESS)
					{
						sprintf(response, "CC:\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
					}
					else
					{
						strcpy(response, "CERROR:Error in execution\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
					}
					break;
				}

				case MKVALVE:
				{
					//AUGER RL/RR/OFF/RECIRC/AIROUT

					// Extract from command string
					if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Extract Heater number
					{
						strcpy(response, "CERROR:Less Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					if (strcmp(&token[0], "RL\0")==0)
					{
						mkValveCCWR();
					}
					else if (strcmp(&token[0], "RR\0")==0)
					{
						mkValveCWR();
					}
					else if (strcmp(&token[0], "OFF\0")==0)
					{
						mkValveStop();
					}
					else if (strcmp(&token[0], "AIROUT\0")==0)
					{
						setMkValveStatus(AIR_OUT);
					}
					else if (strcmp(&token[0], "RECIRC\0")==0)
					{
						setMkValveStatus(AIR_RECIRC);
					}
					else
					{
						strcpy(response, "CERROR:Invalid Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					strcpy(response, "CC:\r\n>");
					sendCliResponse(response, strlen(response));
					setRxStatus(RS232_RCV_IDLE);
					break;

				}
				case AUGER_MOTOR:
				{
					//AUGER RL/RR/OFF

					// Extract from command string
					if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Extract Heater number
					{
						strcpy(response, "CERROR:Less Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					if (strcmp(&token[0], "RL\0")==0)
					{
						augerMotorCCWR();
					}
					else if (strcmp(&token[0], "RR\0")==0)
					{
						augerMotorCWR();
					}
					else if (strcmp(&token[0], "OFF\0")==0)
					{
						augerMotorStop();
					}
					else
					{
						strcpy(response, "CERROR:Invalid Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					strcpy(response, "CC:\r\n>");
					sendCliResponse(response, strlen(response));
					setRxStatus(RS232_RCV_IDLE);
					break;

				}
				case MOTOR1:
				{
					//MOTRO1 RL/RR/OFF DUTY
					uint8_t duty;
					// Extract from command string
					if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Extract Heater number
					{
						strcpy(response, "CERROR:Less Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					if (strcmp(&token[0], "RL\0")==0)
					{
						if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Get wasteCat
						{
							strcpy(response, "CERROR:Less Parameters\r\n>");
							sendCliResponse(response, strlen(response));
							setRxStatus(RS232_RCV_IDLE);
							break;
						}
						duty=atoi(&token[0]);
						if (duty > 100) {duty = 100;}
						MOTOR1_REVERSE();
						//MOTOR1_START();
						pwmStart(duty);
					}
					else if (strcmp(&token[0], "RR\0")==0)
					{
						if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Get wasteCat
						{
							strcpy(response, "CERROR:Less Parameters\r\n>");
							sendCliResponse(response, strlen(response));
							setRxStatus(RS232_RCV_IDLE);
							break;
						}
						duty=atoi(&token[0]);
						if (duty > 100) {duty = 100;}
						MOTOR1_FORWARD();
						//MOTOR1_START();
						pwmStart(duty);
					}
					else if (strcmp(&token[0], "OFF\0")==0)
					{
						//MOTOR1_STOP();
						pwmStart(0);
						//pwmStop();
					}
					else
					{
						strcpy(response, "CERROR:Invalid Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					strcpy(response, "CC:\r\n>");
					sendCliResponse(response, strlen(response));
					setRxStatus(RS232_RCV_IDLE);
					break;
				}
				case MOTOR2:
				{
					//MOTRO2 RL/RR/OFF

					// Extract from command string
					if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Extract Heater number
					{
						strcpy(response, "CERROR:Less Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					if (strcmp(&token[0], "RL\0")==0)
					{
						MOTOR2_REVERSE();
						MOTOR2_START();
					}
					else if (strcmp(&token[0], "RR\0")==0)
					{
						MOTOR2_FORWARD();
						MOTOR2_START();
					}
					else if (strcmp(&token[0], "OFF\0")==0)
					{
						MOTOR2_STOP();
					}
					else
					{
						strcpy(response, "CERROR:Invalid Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					strcpy(response, "CC:\r\n>");
					sendCliResponse(response, strlen(response));
					setRxStatus(RS232_RCV_IDLE);
					break;
				}

				case GETCSMSTATUS:  //Gets the current status of CSM
				{
					//GETCSMSTATUS
					 dgCsmParam_t status;
					if(csmGetState(CLI_MOD, &status) == DG_SUCCESS)
					{
						//Send response
						sprintf(response, "CC:CSM state=%d, phase=%d, cat=%d, remDur=%d\r\n>",status.state, status.phase, status.wasteCat, status.remDur );
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
					}
					else
					{
						strcpy(response, "CERROR:in execution\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					break;
				}
				case GETCSMSTATUS_CHTL:  //Gets the current status of CSM
				{
					//GETCSMSTATUS_CHTL
					dgCsmParam_t status;
					if(csmGetState(CLI_MOD, &status) == DG_SUCCESS)
					{
						//Send response
						sprintf(response, "CC:%d,%d,%d,%d\r\n>",status.state, status.phase, status.wasteCat, status.remDur );
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
					}
					else
					{
						strcpy(response, "CE:\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					break;
				}
				case GETSYSCFGVER_C:
				    // Usage: GETSYSCFGVER_C
				    // Response Example:CC:V1.1\r\n
					char version[10];
					if(getSysconfigVersion(version)==DG_SUCCESS)
					{
				        sprintf(response, "CC:%s\r\n", version);
					}
					else
					{
				        // Error reading version
				        strcpy(response, "CE:\r\n");
					}
				    sendCliResponse(response, strlen(response));
				    setRxStatus(RS232_RCV_IDLE);
					break;

				case GETAUGERCFG_C:
				{
				    dgCtConfigParam_t agrCfg;

				    if (getAugerConfig(&agrCfg) == DG_SUCCESS)
				    {
				        // CC:cwrDur,ccwrDur,pauseDur,rptCount,offDur
				        // Fields are uint8_t, but they get promoted to int when passed to sprintf.
				        // Use %u and cast to unsigned to be explicit.
				        sprintf(response, "CC:%u,%u,%u,%u,%u\r\n",
				                (unsigned)agrCfg.cwrDuration,
				                (unsigned)agrCfg.ccwrDuration,
				                (unsigned)agrCfg.pauseDuration,
				                (unsigned)agrCfg.rptCount,
				                (unsigned)agrCfg.offDuration);
				    }
				    else
				    {
				        // Error reading auger config
				        strcpy(response, "CE:\r\n");
				    }

				    sendCliResponse(response, strlen(response));
				    setRxStatus(RS232_RCV_IDLE);
				    break;
				}

				case SETAUGERCFG_C:
				{
				    dgCtConfigParam_t agrCfg;
				    unsigned int cwrTmp, ccwrTmp, pauseTmp, rptTmp, offTmp;  // wider for sscanf
				    int parsed;

				    // Usage: SETAUGERCFG_C cwrDur,ccwrDur,pauseDur,rptCount,offDur
				    // Example: SETAUGERCFG_C 10,5,2,3,15
				    parsed = sscanf(cmdString,
				                    "SETAUGERCFG_C %u,%u,%u,%u,%u",
				                    &cwrTmp,
				                    &ccwrTmp,
				                    &pauseTmp,
				                    &rptTmp,
				                    &offTmp);

				    if (parsed == 5)
				    {
				        // Validate that all values fit into uint8_t (0..255)
				        if (cwrTmp   <= 0xFF &&
				            ccwrTmp  <= 0xFF &&
				            pauseTmp <= 0xFF &&
				            rptTmp   <= 0xFF &&
				            offTmp   <= 0xFF)
				        {
				            agrCfg.cwrDuration   = (uint8_t)cwrTmp;
				            agrCfg.ccwrDuration  = (uint8_t)ccwrTmp;
				            agrCfg.pauseDuration = (uint8_t)pauseTmp;
				            agrCfg.rptCount      = (uint8_t)rptTmp;
				            agrCfg.offDuration   = (uint8_t)offTmp;

				            if (setAugerConfig(&agrCfg) == DG_SUCCESS)
				            {
				                // Success acknowledgement
				                strcpy(response, "CC:\r\n");
				            }
				            else
				            {
				                // Validation / range error inside setAugerConfig
				                strcpy(response, "CE:\r\n");
				            }
				        }
				        else
				        {
				            // One or more parameters out of uint8_t range
				            strcpy(response, "CE:\r\n");
				        }
				    }
				    else
				    {
				        // Parsing error
				        strcpy(response, "CE:\r\n");
				    }

				    sendCliResponse(response, strlen(response));
				    setRxStatus(RS232_RCV_IDLE);
				    break;
				}


				case GETHATCSCFG_C:
				{
				    // Usage: GETHATCSCFG_C sensorState,index
				    // Example: GETHATCSCFG_C 0,3

				    dgActuatorCtrlCode_t hatcsCfg;

				    uint8_t      sStatus = 0, index = 0;
				    unsigned int sStatusTmp = 0, indexTmp = 0;  // wider for sscanf
				    int          parsed = 0;

				    printf("cliProc.c:cmd received: %s\r\n", cmdString);

				    // Parse "GETHATCSCFG_C sStatus,index"
				    parsed = sscanf(cmdString,
				                    "GETHATCSCFG_C %u,%u",
				                    &sStatusTmp,
				                    &indexTmp);

				    if (parsed == 2)
				    {
				        // Validate ranges before casting
				        // There are 10 HAT sensor states (0..9),
				        // and index is 0..MAX_CTRL_CODE_PER_SEQ-1
				        if (sStatusTmp <= AIR_IN_CTRL_SEQ &&    // 0..9
				            indexTmp   <  MAX_CTRL_CODE_PER_SEQ)
				        {
				            sStatus = (uint8_t)sStatusTmp;
				            index   = (uint8_t)indexTmp;

				            if (getHatcsActuatorCtrlSeq(sStatus, index, &hatcsCfg) == DG_SUCCESS)
				            {
				                // CC:duration,ctCtrl,heaterCtrl,sprayerCtrl,fanCtrl,ctrlSeqEnd,airCircCtrl
				                sprintf(response, "CC:%u,%u,%u,%u,%u,%u,%u\r\n",
				                        (unsigned)hatcsCfg.duration,
				                        (unsigned)hatcsCfg.ctCtrl,
				                        (unsigned)hatcsCfg.heaterCtrl,
				                        (unsigned)hatcsCfg.sprayerCtrl,
				                        (unsigned)hatcsCfg.fanCtrl,
				                        (unsigned)hatcsCfg.ctrlSeqEnd,
				                        (unsigned)hatcsCfg.airCircCtrl);

				                printf("cliProc.c:response: %s\r\n", response);
				            }
				            else
				            {
				                // Invalid state/index or internal error
				                strcpy(response, "CE:\r\n");
				            }
				        }
				        else
				        {
				            // Out-of-range sensorState or index
				            strcpy(response, "CE:\r\n");
				        }
				    }
				    else
				    {
				        // Parsing error
				        strcpy(response, "CE:\r\n");
				    }

				    sendCliResponse(response, strlen(response));
				    setRxStatus(RS232_RCV_IDLE);
				    break;
				}

				case SETHATCSCFG_C:
				{
				    // Usage:
				    // SETHATCSCFG_C sStatus,index,duration,ctCtrl,heaterCtrl,
				    //               sprayerCtrl,fanCtrl,ctrlSeqEnd,airCircCtrl
				    //
				    // Example:
				    // SETHATCSCFG_C 0,0,10,1,1,1,1,2,3

				    dgActuatorCtrlCode_t hatcsCfg;

				    uint8_t sStatus = 0, index = 0;

				    // Temporaries for sscanf (wider than uint8_t)
				    unsigned int sStatusTmp, indexTmp;
				    unsigned int durTmp, ctTmp, heaterTmp, sprayTmp, fanTmp, endTmp, airTmp;

				    int parsed;

				    printf("cliProc.c:cmd received: %s\r\n", cmdString);

				    parsed = sscanf(cmdString,
				                    "SETHATCSCFG_C %u,%u,%u,%u,%u,%u,%u,%u,%u",
				                    &sStatusTmp,
				                    &indexTmp,
				                    &durTmp,
				                    &ctTmp,
				                    &heaterTmp,
				                    &sprayTmp,
				                    &fanTmp,
				                    &endTmp,
				                    &airTmp);

				    if (parsed == 9)
				    {
				        // Validate ranges for all uint8_t fields
				        if (sStatusTmp <= AIR_IN_CTRL_SEQ &&
				            indexTmp   <  MAX_CTRL_CODE_PER_SEQ  &&
				            durTmp     <= 0xFF &&
				            ctTmp      <= 0xFF &&
				            heaterTmp  <= 0xFF &&
				            sprayTmp   <= 0xFF &&
				            fanTmp     <= 0xFF &&
				            endTmp     <= 0xFF &&
				            airTmp     <= 0xFF)
				        {
				            sStatus             = (uint8_t)sStatusTmp;
				            index               = (uint8_t)indexTmp;
				            hatcsCfg.duration   = (uint8_t)durTmp;
				            hatcsCfg.ctCtrl     = (uint8_t)ctTmp;
				            hatcsCfg.heaterCtrl = (uint8_t)heaterTmp;
				            hatcsCfg.sprayerCtrl= (uint8_t)sprayTmp;
				            hatcsCfg.fanCtrl    = (uint8_t)fanTmp;
				            hatcsCfg.ctrlSeqEnd = (uint8_t)endTmp;
				            hatcsCfg.airCircCtrl= (uint8_t)airTmp;

				            if (setHatcsActuatorCtrlSeq(sStatus, index, &hatcsCfg) == DG_SUCCESS)
				            {
				                strcpy(response, "CC:\r\n");  // Success acknowledgment
				            }
				            else
				            {
				                // Invalid index or parameter at deeper level
				                strcpy(response, "CE:\r\n");
				            }
				        }
				        else
				        {
				            // Out-of-range values
				            strcpy(response, "CE:\r\n");
				        }
				    }
				    else
				    {
				        // Parsing error
				        strcpy(response, "CE:\r\n");
				    }

				    sendCliResponse(response, strlen(response));
				    setRxStatus(RS232_RCV_IDLE);
				    break;
				}

				case GETCSMCFG_C:
				{
				    // Usage: GETCSMCFG_C wasteCat,phase
				    // Example: GETCSMCFG_C 0,1

				    dgCtProcessParam_t phaseParam;

				    uint8_t  wasteCat = 0, phase = 0;
				    unsigned int wasteCatTmp = 0, phaseTmp = 0;   // wider types for sscanf
				    int parsed = 0;

				    printf("cliProc.c: cmd received: %s\r\n", cmdString);

				    // Parse CLI buffer
				    //   wasteCat: 0..7  -> %u  into unsigned int, then cast/validate
				    //   phase:    0..2  -> %u  into unsigned int, then cast/validate
				    //
				    // If your syntax is "GETCSMCFG_C 0,1" (comma-separated), use:
				    parsed = sscanf(cmdString,
				                    "GETCSMCFG_C %u,%u",
				                    &wasteCatTmp,
				                    &phaseTmp);

				    // If instead your syntax is "GETCSMCFG_C 0 1" (space-separated),
				    // comment the above and use this line instead:
				    // parsed = sscanf(cmdString, "GETCSMCFG_C %u %u", &wasteCatTmp, &phaseTmp);

				    if (parsed == 2)
				    {
				        // Validate ranges before casting
				        if (wasteCatTmp < WASTE_CAT_MAX && phaseTmp < PHASE_COUNT_MAX)
				        {
				            wasteCat = (uint8_t)wasteCatTmp;
				            phase    = (uint8_t)phaseTmp;

				            if (getCsmPhaseParam(wasteCat, phase, &phaseParam) == DG_SUCCESS)
				            {
				                // CC:temperature,humidity,phaseDur,aeration
				                sprintf(response, "CC:%.1f,%.1f,%d,%u,%u\r\n",
				                        phaseParam.temperature,
				                        phaseParam.humidity,
				                        phaseParam.phaseDur,
				                        phaseParam.aerationFreq,
										phaseParam.aerationDur);

				                sendCliResponse(response, strlen(response));
				                printf("cliProc.c: response: %s\r\n", response);
				            }
				            else
				            {
				                // Invalid index or parameter
				                strcpy(response, "CE:\r\n");
				                sendCliResponse(response, strlen(response));
				            }
				        }
				        else
				        {
				            // Out-of-range wasteCat or phase
				            strcpy(response, "CE:\r\n");
				            sendCliResponse(response, strlen(response));
				        }
				    }
				    else
				    {
				        // Parsing error
				        strcpy(response, "CE:\r\n");
				        sendCliResponse(response, strlen(response));
				    }

				    setRxStatus(RS232_RCV_IDLE);
				    break;
				}

				case SETCSMCFG_C:
				{
				    // Usage: SETCSMCFG_C wasteCat,phase,temperature,humidity,phaseDur,aerationFreq
				    dgCtProcessParam_t phaseParam;

				    uint8_t  wasteCat, phase;
				    unsigned int wasteCatTmp, phaseTmp;  // wider types for sscanf
				    int parsed;

				    printf("cliProc.c: cmd received: %s\r\n", cmdString);

				    // temperature, humidity: float  -> %f
				    // phaseDur, aeration:   uint16_t -> %hu (this 'h' is supported on your compiler)
				    parsed = sscanf(cmdString,
				                    "SETCSMCFG_C %u,%u,%f,%f,%hu,%u,%u",
				                    &wasteCatTmp,
				                    &phaseTmp,
				                    &phaseParam.temperature,
				                    &phaseParam.humidity,
				                    &phaseParam.phaseDur,
									&phaseParam.aerationFreq,
				                    &phaseParam.aerationDur);

				    if (parsed == 7)
				    {
				        // Validate ranges before casting to uint8_t
				        if (wasteCatTmp < WASTE_CAT_MAX && phaseTmp < PHASE_COUNT_MAX)
				        {
				            wasteCat = (uint8_t)wasteCatTmp;
				            phase    = (uint8_t)phaseTmp;

				            if (setCsmPhaseParam(wasteCat, phase, &phaseParam) == DG_SUCCESS)
				            {
				                sprintf(response, "CC:\r\n");
				                sendCliResponse(response, strlen(response));
				            }
				            else
				            {
				                strcpy(response, "CE:\r\n");  // Invalid index or parameter
				                sendCliResponse(response, strlen(response));
				            }
				        }
				        else
				        {
				            // Out-of-range category / phase
				            strcpy(response, "CE:\r\n");
				            sendCliResponse(response, strlen(response));
				        }
				    }
				    else
				    {
				        // Parsing error
				        strcpy(response, "CE:\r\n");
				        sendCliResponse(response, strlen(response));
				    }

				    setRxStatus(RS232_RCV_IDLE);
				    break;
				}

				case GETTRFRCFG_C:
				{
					dgTcsConfigParam_t trCfg;

				    if (getTransferCtrlParams(&trCfg) == DG_SUCCESS)
				    {
				        sprintf(response, "CC:%d,%d,%d\r\n",
				                trCfg.stvOpenDur,
				                trCfg.stvCloseDur,
				                trCfg.transferDur);
				        sendCliResponse(response, strlen(response));
				        printf("cliProc.c:response %s\r\n",response);
				    }
				    else
				    {
				        strcpy(response, "CE:\r\n");
				        sendCliResponse(response, strlen(response));
				    }

				    setRxStatus(RS232_RCV_IDLE);
				    break;
				}
				case SETTRFRCFG_C:
				{
					dgTcsConfigParam_t trCfg;

				    // Temporaries for sscanf (wider than uint8_t)
				    unsigned int stvOpenDur, stvCloseDur, transferDur;
				    int parsed;

				    printf("cliProc.c: cmd received: %s\r\n", cmdString);

				    // Usage:
				    // SETTRFRCFG_C stvOpenDur,stvCloseDur,transferDur
				    // Example:
				    // SETTRFRCFG_C 600,600,40  -mSec, mSec, Sec
				    parsed = sscanf(cmdString,
				                    "SETTRFRCFG_C %u,%u,%u",
				                    &stvOpenDur,
				                    &stvCloseDur,
				                    &transferDur);

				    if (parsed == 3)
				    {
				        // Validate as uint16_t
				        if (stvOpenDur <= 0xFFFF &&
				        	stvCloseDur  <= 0xFFFF &&
							transferDur <= 0xFFFF )

				        {
				        	trCfg.stvOpenDur 	= (uint16_t)stvOpenDur;
				        	trCfg.stvCloseDur   = (uint16_t)stvCloseDur;
				        	trCfg.transferDur   = (uint16_t)transferDur;


				            if (setTransferCtrlParams(&trCfg) == DG_SUCCESS)
				            {
				                strcpy(response, "CC:\r\n");  // Success acknowledgment
				            }
				            else
				            {
				                strcpy(response, "CE:\r\n");  // Validation / internal error
				            }
				        }
				        else
				        {
				            // Out-of-range values
				            strcpy(response, "CE:\r\n");
				        }
				    }
				    else
				    {
				        // Parsing error
				        strcpy(response, "CE:\r\n");
				    }

				    sendCliResponse(response, strlen(response));
				    setRxStatus(RS232_RCV_IDLE);
				    break;
				}

				case GETADCSCFG_C:
				{
					//Usage:
					//Command: GETADCSCFG_C
					//Example positive response: "CC:50,6,1\r\n"
					dgAdcsConfigParams_t adcsCfg;

				    if (getAdcsParams(&adcsCfg) == DG_SUCCESS)
				    {
				        sprintf(response, "CC:%u,%u,%u\r\n",
				        		adcsCfg.adcsQuietPeriod,
								adcsCfg.additivePerDelivery,
								adcsCfg.additivePerMinute);
				        sendCliResponse(response, strlen(response));
				        printf("cliProc.c:response %s\r\n",response);
				    }
				    else
				    {
				        strcpy(response, "CE:\r\n");
				        sendCliResponse(response, strlen(response));
				    }

				    setRxStatus(RS232_RCV_IDLE);
				    break;
				}
				case SETADCSCFG_C:
				{
					dgAdcsConfigParams_t adcsCfg;

				    // Temporaries for sscanf (wider than uint8_t)
				    unsigned int adcsQuietPeriod, additivePerDelivery, additivePerMinute;
				    int parsed;

				    printf("cliProc.c: cmd received: %s\r\n", cmdString);

				    // Usage:
				    // dgAdcsConfigParams_t adcsCfg; adcsQuietPeriod,additivePerDelivery,additivePerMinute
				    // Example:
				    // SETTRFRCFG_C 50,10,1  - min, grams, grams/min
				    parsed = sscanf(cmdString,
				                    "SETADCSCFG_C %u,%u,%u",
				                    &adcsQuietPeriod,
				                    &additivePerDelivery,
				                    &additivePerMinute);

				    if (parsed == 3)
				    {
				        // Validate as uint8_t
				        if (adcsQuietPeriod <= 0xFF &&
				        		additivePerDelivery  <= 0xFF &&
							additivePerMinute <= 0xFF )

				        {
				        	adcsCfg.adcsQuietPeriod 	= (uint8_t)adcsQuietPeriod;
				        	adcsCfg.additivePerDelivery   = (uint8_t)additivePerDelivery;
				        	adcsCfg.additivePerMinute   = (uint8_t)additivePerMinute;


				            if (setAdcsParams(&adcsCfg) == DG_SUCCESS)
				            {
				                strcpy(response, "CC:\r\n");  // Success acknowledgment
				            }
				            else
				            {
				                strcpy(response, "CE:\r\n");  // Validation / internal error
				            }
				        }
				        else
				        {
				            // Out-of-range values
				            strcpy(response, "CE:\r\n");
				        }
				    }
				    else
				    {
				        // Parsing error
				        strcpy(response, "CE:\r\n");
				    }

				    sendCliResponse(response, strlen(response));
				    setRxStatus(RS232_RCV_IDLE);
				    break;
				}


				case GETSHDSEQ_C:
				{
				    // Usage: GETSHDSEQ_C index
				    // Example: GETSHDSEQ_C 0

				    dgShredderCtrlSeq_t shdSeq;

				    uint8_t      index = 0;
				    unsigned int indexTmp = 0;   // wider for sscanf
				    int          parsed  = 0;

				    printf("cliProc.c: cmd received: %s\r\n", cmdString);

				    parsed = sscanf(cmdString, "GETSHDSEQ_C %u", &indexTmp);

				    if (parsed == 1)
				    {
				        if (indexTmp < MAX_SHD_SEQ)
				        {
				            index = (uint8_t)indexTmp;

				            if (getShredderCtrlSeq(index, &shdSeq) == DG_SUCCESS)
				            {
				                // CC:durationSec,shdMotor,shdFlapMotor,flushSprayer,ctrlSeqRecType
				                sprintf(response, "CC:%u,%u,%u,%u,%u,%u\r\n",
				                        (unsigned)shdSeq.durationSec,
				                        (unsigned)shdSeq.shdMotor,
				                        (unsigned)shdSeq.shdFlapMotor,
				                        (unsigned)shdSeq.flushSprayer,
				                        (unsigned)shdSeq.shdAugMotor,
				                        (unsigned)shdSeq.ctrlSeqRecType);
				            }
				            else
				            {
				                // Invalid index or internal error
				                strcpy(response, "CE:\r\n");
				            }
				        }
				        else
				        {
				            // Out-of-range index
				            strcpy(response, "CE:\r\n");
				        }
				    }
				    else
				    {
				        // Parsing error
				        strcpy(response, "CE:\r\n");
				    }

				    sendCliResponse(response, strlen(response));
				    setRxStatus(RS232_RCV_IDLE);
				    break;
				}

				case SETSHDSEQ_C:
				{
				    // Usage:
				    // SETSHDSEQ_C index,durationSec,shdMotor,shdFlapMotor,flushSprayer,shdaugMotor,ctrlSeqRecType
				    //
				    // Example:
				    // SETSHDSEQ_C 0,10,1,1,0,2

				    dgShredderCtrlSeq_t shdSeq;

				    uint8_t index = 0;

				    // Temporaries for sscanf (wider than uint8_t)
				    unsigned int indexTmp;
				    unsigned int durTmp, motorTmp, flapTmp, flushTmp, shdaugTmp, ctrlTmp;

				    int parsed;

				    printf("cliProc.c: cmd received: %s\r\n", cmdString);

				    parsed = sscanf(cmdString,
				                    "SETSHDSEQ_C %u,%u,%u,%u,%u,%u,%u",
				                    &indexTmp,
				                    &durTmp,
				                    &motorTmp,
				                    &flapTmp,
				                    &flushTmp,
									&shdaugTmp,
				                    &ctrlTmp);

				    if (parsed == 7)
				    {
				        // Validate ranges (all uint8_t fields)
				        if (indexTmp < MAX_SHD_SEQ &&
				            durTmp   <= 0xFFFF &&
				            motorTmp <= 0xFF &&
				            flapTmp  <= 0xFF &&
				            flushTmp <= 0xFF &&
							shdaugTmp <= 0xFF &&
				            ctrlTmp  <= 0xFF)
				        {
				            index                 = (uint8_t)indexTmp;
				            shdSeq.durationSec    = (uint16_t)durTmp;
				            shdSeq.shdMotor       = (uint8_t)motorTmp;
				            shdSeq.shdFlapMotor   = (uint8_t)flapTmp;
				            shdSeq.flushSprayer   = (uint8_t)flushTmp;
				            shdSeq.shdAugMotor   = (uint8_t)shdaugTmp;
				            shdSeq.ctrlSeqRecType = (uint8_t)ctrlTmp;

				            if (setShredderCtrlSeq(index, &shdSeq) == DG_SUCCESS)
				            {
				                strcpy(response, "CC:\r\n");   // success
				            }
				            else
				            {
				                strcpy(response, "CE:\r\n");   // validation / storage error
				            }
				        }
				        else
				        {
				            // Out-of-range values
				            strcpy(response, "CE:\r\n");
				        }
				    }
				    else
				    {
				        // Parsing error
				        strcpy(response, "CE:\r\n");
				    }

				    sendCliResponse(response, strlen(response));
				    setRxStatus(RS232_RCV_IDLE);
				    break;
				}

				case GETSHDTVAR_C:
				{
				    dgShredderTimingVar_t shdTimingParam;

				    printf("cliProc.c: cmd received: %s\r\n", cmdString);

				    if (getShredderTimingParam(&shdTimingParam) == DG_SUCCESS)
				    {
				        // CC:startDelay,flapOpenDur,flapCloseDur,numOfCtrlSeq
				        sprintf(response, "CC:%u,%u,%u,%u\r\n",
				                (unsigned)shdTimingParam.shdStartDlyFmLidclose,
				                (unsigned)shdTimingParam.flapOpenDur,
				                (unsigned)shdTimingParam.flapCloseDur,
				                (unsigned)shdTimingParam.numOfCtrlSeq);
				    }
				    else
				    {
				        strcpy(response, "CE:\r\n");
				    }

				    sendCliResponse(response, strlen(response));
				    setRxStatus(RS232_RCV_IDLE);
				    break;
				}

				case SETSHDTVAR_C:
				{
				    dgShredderTimingVar_t shdTimingParam;

				    // Temporaries for sscanf (wider than uint8_t)
				    unsigned int startTmp, openTmp, closeTmp, numTmp;
				    int parsed;

				    printf("cliProc.c: cmd received: %s\r\n", cmdString);

				    // Usage:
				    // SETSHDTVAR_C shdStartDlyFmLidclose,flapOpenDur,flapCloseDur,numOfCtrlSeq
				    // Example:
				    // SETSHDTVAR_C 5,2,2,8
				    parsed = sscanf(cmdString,
				                    "SETSHDTVAR_C %u,%u,%u,%u",
				                    &startTmp,
				                    &openTmp,
				                    &closeTmp,
				                    &numTmp);

				    if (parsed == 4)
				    {
				        // Validate as uint8_t and numOfCtrlSeq within MAX_SHD_SEQ
				        if (startTmp <= 0xFF &&
				            openTmp  <= 0xFF &&
				            closeTmp <= 0xFF &&
				            numTmp   <= 0xFF &&
				            numTmp   <= MAX_SHD_SEQ)
				        {
				            shdTimingParam.shdStartDlyFmLidclose = (uint8_t)startTmp;
				            shdTimingParam.flapOpenDur           = (uint8_t)openTmp;
				            shdTimingParam.flapCloseDur          = (uint8_t)closeTmp;
				            shdTimingParam.numOfCtrlSeq          = (uint8_t)numTmp;

				            if (setShredderTimingParam(&shdTimingParam) == DG_SUCCESS)
				            {
				                strcpy(response, "CC:\r\n");  // Success acknowledgment
				            }
				            else
				            {
				                strcpy(response, "CE:\r\n");  // Validation / internal error
				            }
				        }
				        else
				        {
				            // Out-of-range values
				            strcpy(response, "CE:\r\n");
				        }
				    }
				    else
				    {
				        // Parsing error
				        strcpy(response, "CE:\r\n");
				    }

				    sendCliResponse(response, strlen(response));
				    setRxStatus(RS232_RCV_IDLE);
				    break;
				}

				case SAVECONFIGEE_C:
				{
				    if (storeAllconfigEEPROM() == DG_SUCCESS) {
				        strcpy(response, "CC:\r\n");
				    } else {
				        strcpy(response, "CE:\r\n");
				    }
				    sendCliResponse(response, strlen(response));
				    setRxStatus(RS232_RCV_IDLE);
				    break;
				}

/*				case ADD_WASTE_START:
				{
					if(csmNotifyWasteAddStart(CLI_MOD) == DG_SUCCESS)
					{
						//Send response
						sprintf(response, "CC:\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
					}
					else
					{
						strcpy(response, "CERROR:in execution\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}

					break;
				}*/
/*				case ADD_WASTE_END:
				{
					//ADD_WASTE_END wastecateory (0-4)

					uint8_t wasteCat;

					if (getNextToken(cmdString,&token[0], &bufptr)==-1)  //Extract Heater number
					{
						strcpy(response, "CERROR:Less Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					wasteCat = atoi(&token[0]);
					if(wasteCat >= WASTE_CAT_MAX)  wasteCat = 0;

					if(csmNotifyWasteAddEnd(CLI_MOD, wasteCat) == DG_SUCCESS)
					{
						//Send response
						sprintf(response, "CC:\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
					}
					else
					{
						strcpy(response, "CERROR:in execution\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					break;
				}*/
				case ADCS:
				{
					//ADCS START/STOP

					// Extract from command string
					if (getNextToken(cmdString,&token[0], &bufptr)==-1)
					{
						strcpy(response, "CERROR:Less Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					if (strcmp(&token[0], "START\0")==0)
					{
						adcsStart(CLI_MOD);
					}
					else if (strcmp(&token[0], "STOP\0")==0)
					{
						adcsAbort(CLI_MOD);
					}
					else
					{
						strcpy(response, "CERROR:Invalid Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					strcpy(response, "CC:\r\n>");
					sendCliResponse(response, strlen(response));
					setRxStatus(RS232_RCV_IDLE);
					break;

				}
				case FD_CONFIG:
				{
					//FD_CONFIG

					if(loadDefaultConfig()== DG_SUCCESS)
					{
						strcpy(response, "CC:\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
					}
					else
					{
						strcpy(response, "CERROR:Execution Failed\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					break;

				}

				case SHDCS:
				{
					//SHDCS START/STOP

					// Extract from command string
					if (getNextToken(cmdString,&token[0], &bufptr)==-1)
					{
						strcpy(response, "CERROR:Less Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					if (strcmp(&token[0], "START\0")==0)
					{
						//Send lid close event
						event_lid_close(CLI_MOD);
					}
					else if (strcmp(&token[0], "STOP\0")==0)
					{
						event_lid_open(CLI_MOD);
					}
					else
					{
						strcpy(response, "CERROR:Invalid Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					strcpy(response, "CC:\r\n>");
					sendCliResponse(response, strlen(response));
					setRxStatus(RS232_RCV_IDLE);
					break;

				}
				case SHD_MOTOR:
				{
					//SHD RL/RR/OFF

					// Extract from command string
					if (getNextToken(cmdString,&token[0], &bufptr)==-1)
					{
						strcpy(response, "CERROR:Less Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					if (strcmp(&token[0], "RL\0")==0)
					{
						//shredderReverse();
						//SHD_STOP();
						SHD_DIR_CW();
						SHD_START();
					}
					else if (strcmp(&token[0], "RR\0")==0)
					{
						//shredderForward();
						//SHD_STOP();
						SHD_DIR_CCW();
						SHD_START();
					}
					else if (strcmp(&token[0], "OFF\0")==0)
					{
						//shredderStop();
						SHD_STOP();
					}
					else
					{
						strcpy(response, "CERROR:Invalid Parameters\r\n>");
						sendCliResponse(response, strlen(response));
						setRxStatus(RS232_RCV_IDLE);
						break;
					}
					strcpy(response, "CC:\r\n>");
					sendCliResponse(response, strlen(response));
					setRxStatus(RS232_RCV_IDLE);
					break;

				}

				default:
					strcpy(response, "CERROR:Command not supported\r\n>");
					sendCliResponse(response, strlen(response));
					setRxStatus(RS232_RCV_IDLE);
					break;
			}
			break;

		default:
			break;
		}

	}
}

/******************************* initCmdProc() **************************************************
*Description: This function initialises data strcutures, create queue handlers and starts 		*
* the cli task. This should be called at the time of power up initialization.					*
* It does the following																			*
* 																								*
*	- Creates a Queue handle for receiving event messages from other modules					*
* Command Parameters: None																		*
* Return Values:																				*
*	- DG_SUCCESS: When initialization is successful												*
*	- SF_FAIL: When Queue allocation failed														*
*************************************************************************************************/

int initCli(void)
{
	//Create cli task task
	TaskHandle_t cliTaskHandle;
	TimerHandle_t cliTimerHandle;

	cliTimerHandle = NULL;

    if (xTaskCreate(cli_Task, "cli_Task", configMINIMAL_STACK_SIZE + 1000, NULL, task_PRIORITY, &cliTaskHandle) !=
        pdPASS)
    {
        PRINTF("CLI Task creation failed!.\r\n");
        return DG_FAIL;
    }
    //CLI requires timer and hence create FreeRTOS SW timer
/*    cliTimerHandle = xTimerCreate("cliTimer",CLI_TIMER_DEFAULT, pdFALSE, (void*)CLI_MOD, dgTimerCallback);
    if(cliTimerHandle == NULL)
    {
        PRINTF("Timer creation failed!.\r\n");
    	vTaskDelete(cliTaskHandle);
        return DG_FAIL;
    }*/
    if(registerModule(CLI_MOD, cliTaskHandle, cliTimerHandle)!= DG_SUCCESS)
    {
    	//Registering the module failed. Hence kill the task and return error
        PRINTF("CLI Task registration failed!.\r\n");
//        xTimerDelete(cliTimerHandle,100 / portTICK_PERIOD_MS);
    	vTaskDelete(cliTaskHandle);

    	return DG_FAIL;
    }
    setRxStatus(RS232_RCV_IDLE);
    return DG_SUCCESS;
}

//This method sends a CLI command string to CLI Task.
//This method just post the message to the destination module queue and does not wait for the response.
//This method is designed to be called from an IRQ. This should not be called from a task.

int sendCliCmd(uint8_t srcModule, char* cmdstring)
{
	dgMsg_t sendMsgBuf;
	uint8_t result;

	result = DG_FAIL;

	//Populate the message to send to the modbus task
	sendMsgBuf.src_module = srcModule;
	sendMsgBuf.command = CLI_CMD;
	sendMsgBuf.cmdParam = (void*)cmdstring;
	sendMsgBuf.dest_module = CLI_MOD;
	sendMsgBuf.result = &result;
	sendMsgBuf.taskHandleSM = NULL;

	if(sendMsgFromISR(&sendMsgBuf) != DG_SUCCESS)
	{
		PRINTF("rs232CliDriver.c:sendCliCmd()::Message send failed \r\n" );
		return DG_FAIL;
	}
	return DG_SUCCESS;

}

//This method sends an event indicating CLI response sending has been completed
//This method just post the message to the destination module queue and does not wait for the response.
//This method is designed to be called from an IRQ. This should not be called from a task.
/*
int sendRespCompleteEvent(uint8_t srcModule)
{
	dgMsg_t sendMsgBuf;
	uint8_t result;

	result = DG_FAIL;

	//Populate the message to send to the modbus task
	sendMsgBuf.src_module = srcModule;
//	sendMsgBuf.command = CLI_RESP_COMPLETE_EVENT;
	sendMsgBuf.cmdParam = NULL;
	sendMsgBuf.dest_module = CLI_MOD;
	sendMsgBuf.result = &result;
	sendMsgBuf.taskHandleSM = NULL;


	if(sendMsgfromISR(&sendMsgBuf) != DG_SUCCESS)
	{
//		PRINTF("heaterAPI.c:heaterStop()::Message send failed \r\n" );
		return DG_FAIL;
	}
	return DG_SUCCESS;

}
*/
