/*
 * dgUartDriverCommon.h
 *
 *  Created on: 14-Aug-2025
 *      Author: Jawahar Arumugam
 */

#ifndef DGUARTDRIVERCOMMON_H_
#define DGUARTDRIVERCOMMON_H_


/*************Common definitions between HMI and CLI UART**************/

//Masks used for interrupt enable/disable

#define LPUART_INT_MASK		kLPUART_IdleLineInterruptEnable|\
							kLPUART_RxDataRegFullInterruptEnable|\
							kLPUART_TxDataRegEmptyInterruptEnable|\
							kLPUART_TransmissionCompleteInterruptEnable
#define LPUART_TX_INT_MASK	kLPUART_TxDataRegEmptyInterruptEnable|\
							kLPUART_TransmissionCompleteInterruptEnable

#define LPUART_RX_INT_MASK	kLPUART_RxDataRegFullInterruptEnable
//Clear status flag mask
#define LPUART_CSF_MASK		0xC00EC000



#define MAX_CMD_LEN		64

//RS232 receiver states
#define RS232_RCV_IDLE				0
#define RS232_RECEIVING_CMD			1
#define RS232_PROCESSING_CMD		2
#define RS232_SENDING_RESP			3

//RS232 transmitter states

#define RS232_TX_IDLE				10
#define RS232_TX_INPROGRESS			11
#define RS232_TX_COMPLETED			12
#define RS232_TX_END_WAIT			13

/************* Common Data structure Definitions****************/

typedef struct rs232TrBuf
{
	char *txBufPtr;
	char *rxBufPtr;
	uint8_t txState;
	uint8_t rxState;
	uint8_t txBufSize;
	uint8_t rxBufSize;
	uint8_t	txCount;
	uint8_t	rxCount;
}dgRs232TrBuf_t;



#endif /* DGUARTDRIVERCOMMON_H_ */
