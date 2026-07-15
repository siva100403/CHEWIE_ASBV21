/*
 * CliUartDriver.c
 *
 *  Created on: 24-Nov-2024
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
//#include "fsl_debug_console.h"
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
#include <dgUartDriverCommon.h>
#include <CliUartDriver.h>
#include "cliProc.h"
#include "rtc.h"
#include "ASB_HMI_common.h"
#include "sysStart.h"




/*******************************************************************************
 * Variables
 ******************************************************************************/
extern dgRs232TrBuf_t trBuf;



/*******************************************************************************
 * Code
 ******************************************************************************/

void disCliTxInt()
{
	DisableIRQ(CLI_LPUART_IRQn);
	NVIC_SetPriority(CLI_LPUART_IRQn, 4);
	LPUART_DisableInterrupts(CLI_LPUART, LPUART_TX_INT_MASK);
	EnableIRQ(CLI_LPUART_IRQn);
}


void EnaCliTxInt()
{
	DisableIRQ(CLI_LPUART_IRQn);
	NVIC_SetPriority(CLI_LPUART_IRQn, 4);
	LPUART_EnableInterrupts(CLI_LPUART, LPUART_TX_INT_MASK);
	EnableIRQ(CLI_LPUART_IRQn);
}

void EnaCliRxInt()
{
	LPUART_EnableRx(CLI_LPUART, true);
	DisableIRQ(CLI_LPUART_IRQn);
	NVIC_SetPriority(CLI_LPUART_IRQn, 4);
	LPUART_EnableInterrupts(CLI_LPUART, LPUART_RX_INT_MASK);
	EnableIRQ(CLI_LPUART_IRQn);
}

void DisCliRxInt()
{
	LPUART_EnableRx(CLI_LPUART, false);
	DisableIRQ(CLI_LPUART_IRQn);
	LPUART_DisableInterrupts(CLI_LPUART, LPUART_RX_INT_MASK);
	NVIC_SetPriority(CLI_LPUART_IRQn, 4);
	EnableIRQ(CLI_LPUART_IRQn);
}


void CLI_LPUART_IRQHandler(void)
{
	uint32_t RS232PortFlags;
    uint8_t data;

    RS232PortFlags = LPUART_GetStatusFlags(CLI_LPUART);

    //Handle receive interrupts
    	if ((kLPUART_RxDataRegFullFlag)& RS232PortFlags)
    	{
    		switch(trBuf.rxState)
    		{
    		case RS232_RCV_IDLE:
    			trBuf.rxBufPtr[0] = LPUART_ReadByte(CLI_LPUART);
    			trBuf.rxCount = 1;
    			trBuf.rxState = RS232_RECEIVING_CMD;
    			break;
    		case RS232_RECEIVING_CMD:
    			trBuf.rxBufPtr[trBuf.rxCount] = LPUART_ReadByte(CLI_LPUART);

    			//Check whether end of line is received (CR, LF)
    			if ((trBuf.rxBufPtr[trBuf.rxCount-1]==0x0D) && (trBuf.rxBufPtr[trBuf.rxCount]==0x0A))
    			{
    				trBuf.rxCount++;
    				trBuf.rxBufPtr[trBuf.rxCount]=0x00; //ADD Null char at the end to make it a string
    				//Received the command in full. Send the command to CLI task
    				sendCliCmd(LPUART_ISR, trBuf.rxBufPtr);
    				trBuf.rxState = RS232_PROCESSING_CMD;
    				break;
    			}
    			if(trBuf.rxCount > MAX_CMD_LEN-3)  //To account for adding NULL char at the end
    			{
    				//Command cannot be this long. Something is wrong.
    				//Discard the buffer and start again
    				trBuf.rxCount = 0;
    				trBuf.rxState = RS232_RCV_IDLE;
    				break;
    			}
    			trBuf.rxCount++;
    			break;

    		case RS232_PROCESSING_CMD:
    			//Don't receive further commands until the response has been sent
    			//Read the bytes and ignore
    			LPUART_ReadByte(CLI_LPUART);
    			break;
    		case RS232_SENDING_RESP:
    			//Don't receive further commands until the response has been sent
    			//Read the bytes and ignore
    			LPUART_ReadByte(CLI_LPUART);
    			break;
    		}
    	}
    	else if((kLPUART_IdleLineFlag)& RS232PortFlags)
    	{
    		//Receive is over, but CR-LF is not received.
    		//It is an incomplete command. Discard and start all over again
    		trBuf.rxCount = 0;
    		trBuf.rxState = RS232_RCV_IDLE;

    	}

    	//Handle Tx interrupts
        if (kLPUART_TxDataRegEmptyFlag & RS232PortFlags)
        {
    		switch(trBuf.txState)
    		{
    		case RS232_TX_IDLE:
    			//No need to do anything. Code should not reach here
    			break;

    		case RS232_TX_INPROGRESS:
        		//Send the next char
        		if(trBuf.txCount < trBuf.txBufSize)
        		{
        			data = trBuf.txBufPtr[trBuf.txCount];
        			LPUART_WriteByte(CLI_LPUART,data);
        			trBuf.txCount++;
        		}
        		else
        		{
        			//Buffer is over. Wait for all the bits  to get shifted out
        			trBuf.txState = RS232_TX_END_WAIT;

        		}
    			break;
    		case RS232_TX_COMPLETED:
    			//Code should not reach here. Ignore
    			disCliTxInt();
    			break;
    		case RS232_TX_END_WAIT:
    			//Code should not reach here. Ignore
    			disCliTxInt();
    			break;

    		default:
    			break;
    		}
        }
        else if(kLPUART_TransmissionCompleteFlag & RS232PortFlags)
        {
        	if(trBuf.txState == RS232_TX_END_WAIT)
        	{
        		trBuf.txState = RS232_TX_COMPLETED;
            	//Transmission of the buffer is over. Now send message to CLI Proc
//            	sendRespCompleteEvent(LPUART_ISR);
        		disCliTxInt();

        	}

        }

        //Handle errors
        if(RS232PortFlags & LPUART_STAT_OR(0b1))
        {
        	//Over run error
        	LPUART_ClearStatusFlags(CLI_LPUART, LPUART_STAT_OR(0b1));
        }
        else if(RS232PortFlags & LPUART_STAT_NF(0b1))
        {
        	//Noise Flag. Clear
        	LPUART_ClearStatusFlags(CLI_LPUART, LPUART_STAT_NF(0b1));
        }

    SDK_ISR_EXIT_BARRIER;
}




int initCliUart()
{

	trBuf.txState = RS232_TX_IDLE;
	trBuf.rxState = RS232_RCV_IDLE;

    /* attach FRO 12M to FLEXCOMM1 */
    CLOCK_SetClkDiv(kCLOCK_DivFlexcom1Clk, 1u);
    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM1);


	 if(LP_FLEXCOMM_Init(LPFLEXCOMM_INSTANCE1, LP_FLEXCOMM_PERIPH_LPUART) != kStatus_Success)
	    {
		   printf("CliUartDriver.c:initCliUart(): Flexcomm1 init failed. Code=%d\r\n",kStatus_Success);

	    }
	 else
		   printf("CliUartDriver.c:initCliUart(): Flexcomm1 init success\r\n");



    lpuart_config_t config;

/*     config.baudRate_Bps = 115200U;
     config.parityMode = kLPUART_ParityDisabled;
     config.stopBitCount = kLPUART_OneStopBit;
     config.txFifoWatermark = 0;
     config.rxFifoWatermark = 0;
     config.enableTx = false;
     config.enableRx = false;*/

    LPUART_GetDefaultConfig(&config);
    config.baudRate_Bps = CLI_UART_BAUDRATE;
    config.parityMode = kLPUART_ParityDisabled;
    config.stopBitCount = kLPUART_OneStopBit;
    config.enableTx     = true;
    config.enableRx     = true;

   if( LPUART_Init(CLI_LPUART, &config, CLI_LPUART_CLK_FREQ)!= kStatus_Success)
   {
	   printf("CliUartDriver.c:initCliUart(): Failed\r\n");
	   return DG_FAIL;
   }


   NVIC_SetPriority(CLI_LPUART_IRQn, 4);
    /* Enable RX interrupt. */
    LPUART_EnableInterrupts(CLI_LPUART, kLPUART_RxDataRegFullInterruptEnable);
    EnableIRQ(CLI_LPUART_IRQn);


    return(DG_SUCCESS);
}













