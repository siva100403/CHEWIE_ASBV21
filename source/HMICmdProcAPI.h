/*
 * HMICmdProcAPI.h
 *
 *  Created on: 05-May-2025
 *      Author: Jawahar Arumugam
 */

#ifndef HMICMDPROCAPI_H_
#define HMICMDPROCAPI_H_

/************Structure definition for passing between HMICmdProc API and the UART Serial driver*****/

//Following structure is used to share the Tx and Rx buffers between HMICmdProc task API and UART driver

typedef struct asbComParam
{
	uint16_t txBufferSize;		// Packet to be sent to ASB
	uint8_t* txBuffer;			// Size of the packet
	uint16_t rxBufferSize;
	uint8_t* rxBuffer;
	uint8_t  status;			// Status of execution of command or receiving status of response buffer
}dgAsbComParam_t;


int sendTxCompleteEvent();
int sendRcvBuffer(uint8_t *rcvdBuffer, uint16_t rcvdBufferSize);

#endif /* HMICMDPROCAPI_H_ */
