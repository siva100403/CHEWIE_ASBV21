/*
 * ASB_HMI_common.c
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

/* Chewie Includes */
#include "dgCommon.h"
#include "version.h"
#include "modulecom.h"
#include "dgtimer.h"
#include "GPIOSignals.h"
#include "dgI2cDriver.h"
#include "rtc.h"
#include "ASB_HMI_common.h"
#include "sysStart.h"





/***********************Sequence Number Managing methods***************/
/*
void initSeqNumber()
{
	asbComSeqNumber = 0;
}

uint8_t getSeqNumber()
{
	uint8_t temp;
	temp = asbComSeqNumber;
	asbComSeqNumber++;
	asbComSeqNumber = asbComSeqNumber & 0xFF;
	return temp;
}
*/

/***********************Cheksum related methods**************************/

//Computes checksum for the buffer passed based on the size. It does not include the
// first character 'STX' for the checksum calculation
//It expects the size to be even number, otherwise it returns parameter error
//Retrun values:
//   DG_SUCCESS - Checksum Computation success
//   DG_INVALID_PARAM
int computeCheksum(uint8_t *packetBuff, uint16_t size, uint16_t *checksum)
{
	uint16_t check;
	//Parameter Validation
	if((packetBuff == NULL) || (checksum== NULL) || (size%2 != 0))
	{
		return DG_INVALID_PARAM;
	}
	check = 0;

	for(uint16_t count = 0; count < size; count++)
	{
		check+=packetBuff[count];
	}
	*checksum = check;
	return DG_SUCCESS;
}

//Verifies the checksum

int verifyCheksum(uint8_t *packetBuff, uint8_t packetSize)
{
	uint16_t calChecksum, orgChecksum;
	//Parameter Validation
	if((packetBuff==NULL)||(packetSize == 0))
	{
		return DG_INVALID_PARAM;
	}
	calChecksum=0;
	for(uint16_t index=0; index <packetSize-2; index++)
	{
		calChecksum+=packetBuff[index];
	}

	orgChecksum = packetBuff[packetSize - 2] + (packetBuff[packetSize-1]<<8);
	if(orgChecksum == calChecksum )
	{
		return DG_SUCCESS;
	}
	else
	{
		return DG_FAIL;
	}
}
/***********************Packet Manipulation methods*********************/

// This method prepares the packet to be sent to ASB including checksum
// STX, ETX and ESC seq has to be added after this

int preparePacket(uint8_t flags, uint8_t* rcvPacket, uint8_t *payload, uint8_t payloadSize, uint8_t *packetBuff, uint8_t *packetSize)
{
	//uint8_t curChar;
	uint16_t packetBuffIndex=0;
	uint16_t checksum;

	//Input parameter validation
	if ((payloadSize > MAX_PAYLOAD_SIZE) || (packetBuff == NULL))
	{
		return DG_INVALID_PARAM;
	}

	packetBuff[FLAG_BYTE]	= flags;
	packetBuff[COMMAND_BYTE]	= rcvPacket[COMMAND_BYTE];
	packetBuff[SEQUENCE_BYTE]	= rcvPacket[SEQUENCE_BYTE];
	packetBuff[LENGTH_BYTE]	= payloadSize;

	packetBuffIndex = PAYLOAD_START;   //STX and Packet Header has been already inserted
	if(payloadSize != 0)
	{
		//Copy payload to packet
		memcpy(&packetBuff[packetBuffIndex], payload, payloadSize);
		packetBuffIndex+=payloadSize;
		//Pad the buffer with zero if payload size is odd (excluding STX)
		if( packetBuffIndex%2 != 0)
			packetBuff[packetBuffIndex++] = 0;
	}
	//Compute Checksum and add to the packet
	if(computeCheksum(packetBuff, packetBuffIndex, &checksum) == DG_SUCCESS)
	{
		packetBuff[packetBuffIndex++] = checksum & 0x00FF;
		packetBuff[packetBuffIndex++] = (checksum>>8) & 0x00FF;
	}
	else
	{
		return DG_FAIL;
	}

	*packetSize = packetBuffIndex;

	return DG_SUCCESS;
}

//Adds STX, ETX and ESC sequences to the prepared packet. txBuff memory allocation should
//consider additional storage for ESC seq.
int insertEsc(uint8_t *packetBuff, uint8_t packetSize, uint8_t *txBuff, uint16_t *txSize)
{
	uint8_t curChar;
	uint16_t txBuffIndex;

	//Input parameter validation
	if ((txSize == NULL)|| (packetBuff == NULL) || (txBuff == NULL) || (txSize == NULL))
	{
		return DG_INVALID_PARAM;
	}
	txBuffIndex = 0;
	txBuff[txBuffIndex++] = START_CHR;
	for(int count = 0; count <packetSize; count++)
	{
		curChar = packetBuff[count];
		if((curChar==START_CHR) || (curChar==END_CHR) || (curChar==ESCAPE_CHR))
		{
			txBuff[txBuffIndex++] = ESCAPE_CHR;
			txBuff[txBuffIndex++]  = curChar;
		}
		else
		{
			txBuff[txBuffIndex++] = curChar;
		}
	}
	txBuff[txBuffIndex++] = END_CHR;
	*txSize = txBuffIndex;
	return DG_SUCCESS;
}

//This method strips STX, ETX and ESC seq from the rxbuffer and returns in packetBuff
int stripEsc(uint8_t *rxBuff, uint8_t rxSize, uint8_t *packetBuff, uint8_t *packetSize)
{
	uint8_t curChar;
	uint16_t rxBuffIndex, packetBuffIndex;

	//Input parameter validation
	if ((rxSize == 0)|| (rxBuff == NULL) || (packetBuff == NULL) || (packetSize == NULL))
	{
		return DG_INVALID_PARAM;
	}
	packetBuffIndex = 0;

/*	for(rxBuffIndex = 0; rxBuffIndex <rxSize; rxBuffIndex++)
	{
		curChar = rxBuff[rxBuffIndex];
		if(curChar==ESCAPE_CHR)
		{
			//Ignore escape and store the next byte
			//packetBuff[packetBuffIndex++] = rxBuff[rxBuffIndex++];
		}
		else
		{
			packetBuff[packetBuffIndex++] = curChar;
		}
	}*/

	for(rxBuffIndex = 0; rxBuffIndex <rxSize;)
	{
		curChar = rxBuff[rxBuffIndex++];
		if(curChar==ESCAPE_CHR)
		{
			//Ignore escape and store the next byte
			packetBuff[packetBuffIndex++] = rxBuff[rxBuffIndex++];
		}
		else
		{
			packetBuff[packetBuffIndex++] = curChar;
		}
	}
	*packetSize = packetBuffIndex;
	return DG_SUCCESS;
}
