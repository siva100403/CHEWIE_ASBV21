/*
 * hmiUartDriver.c
 *
 *  Created on: 14-Aug-2025
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


/*******************************************************************************
 * Variables
 ******************************************************************************/
extern dgRs232TrBuf_t HMItrBuf;


/*******************************************************************************
 * Code
 ******************************************************************************/

void disHMITxInt()
{
	DisableIRQ(HMI_LPUART_IRQn);
	NVIC_SetPriority(HMI_LPUART_IRQn, 4);
	LPUART_DisableInterrupts(HMI_LPUART, LPUART_TX_INT_MASK);
	EnableIRQ(HMI_LPUART_IRQn);
}


void EnaHMITxInt()
{
	DisableIRQ(HMI_LPUART_IRQn);
	NVIC_SetPriority(HMI_LPUART_IRQn, 4);
	LPUART_EnableInterrupts(HMI_LPUART, LPUART_TX_INT_MASK);
	EnableIRQ(HMI_LPUART_IRQn);
}

void EnaHMIRxInt()
{
	LPUART_EnableRx(HMI_LPUART, true);
	DisableIRQ(HMI_LPUART_IRQn);
	NVIC_SetPriority(HMI_LPUART_IRQn, 4);
	LPUART_EnableInterrupts(HMI_LPUART, LPUART_RX_INT_MASK);
	EnableIRQ(HMI_LPUART_IRQn);
}

void DisHMIRxInt()
{
	LPUART_EnableRx(HMI_LPUART, false);
	DisableIRQ(HMI_LPUART_IRQn);
	LPUART_DisableInterrupts(HMI_LPUART, LPUART_RX_INT_MASK);
	EnableIRQ(HMI_LPUART_IRQn);
}




void HMI_LPUART_IRQHandler(void)
{
	uint32_t RS232PortFlags;
    uint8_t data;

    //RED_LED_ON();
    RS232PortFlags = LPUART_GetStatusFlags(HMI_LPUART);

    //Handle receive interrupts
    	if ((kLPUART_RxDataRegFullFlag)& RS232PortFlags)
    	{
    		switch(HMItrBuf.rxState)
    		{
    		case RS232_RCV_IDLE:
    			//Read the received char and look for STX
    			if(LPUART_ReadByte(HMI_LPUART) == START_CHR)
    			{
    				//received STX
        			HMItrBuf.rxState = RS232_RECEIVING_CMD;
    			}
    			break;
    		case RS232_RECEIVING_CMD:
    			HMItrBuf.rxBufPtr[HMItrBuf.rxCount] = LPUART_ReadByte(HMI_LPUART);
    			if(HMItrBuf.rxBufPtr[HMItrBuf.rxCount] != END_CHR )
    			{
        			HMItrBuf.rxCount++;
        			if(HMItrBuf.rxCount > MAX_TX_BUFFER_SIZE)
        			{
        				//Packet cannot be this long. Something is wrong.
        				//Discard the buffer and start again
        				HMItrBuf.rxCount = 0;
        				HMItrBuf.rxState = RS232_RCV_IDLE;
        			}
        			break;
    			}
    			else
    			{
    				if((HMItrBuf.rxBufPtr[(HMItrBuf.rxCount)-1]!=ESCAPE_CHR) ||((HMItrBuf.rxBufPtr[(HMItrBuf.rxCount)-2]==ESCAPE_CHR)&&(HMItrBuf.rxBufPtr[(HMItrBuf.rxCount)-1]==ESCAPE_CHR)))
    				{
    					//Reached end of packet
        				if(HMItrBuf.rxCount< 5)
        				{
        					//Too small a packet. Ignore and start looking for STX again
            				HMItrBuf.rxCount = 0;
            				HMItrBuf.rxState = RS232_RCV_IDLE;
            				break;
        				}
        				else
        				{
            				//Received the command in full. Send the command to HMI task
            				sendRcvBuffer(HMItrBuf.rxBufPtr, HMItrBuf.rxCount);
            				HMItrBuf.rxState = RS232_PROCESSING_CMD;
            				//HMItrBuf.rxCount=0;
            				break;
        				}
    				}
    				else
    				{
    					//Not reached end of packet
            			HMItrBuf.rxCount++;
    				}
    			}
    			break;

    		case RS232_PROCESSING_CMD:
    			//Don't receive further commands until the response has been sent
    			//Read the bytes and ignore
    			LPUART_ReadByte(HMI_LPUART);
    			break;
    		case RS232_SENDING_RESP:
    			//Don't receive further commands until the response has been sent
    			//Read the bytes and ignore
    			LPUART_ReadByte(HMI_LPUART);
    			break;
    		}
    	}
    	else if((kLPUART_IdleLineFlag)& RS232PortFlags)
    	{
    		//Receive is over, but CR-LF is not received.
    		//It is an incomplete command. Discard and start all over again
    		HMItrBuf.rxCount = 0;
    		HMItrBuf.rxState = RS232_RCV_IDLE;

    	}

    	//Handle Tx interrupts
        if (kLPUART_TxDataRegEmptyFlag & RS232PortFlags)
        {
    		switch(HMItrBuf.txState)
    		{
    		case RS232_TX_IDLE:
    			//No need to do anything. Code should not reach here
    			break;

    		case RS232_TX_INPROGRESS:
        		//Send the next char
        		if(HMItrBuf.txCount < HMItrBuf.txBufSize)
        		{
        			data = HMItrBuf.txBufPtr[HMItrBuf.txCount];
        			LPUART_WriteByte(HMI_LPUART,data);
        			HMItrBuf.txCount++;
        		}
        		else
        		{
        			//Buffer is over. Wait for all the bits  to get shifted out
        			HMItrBuf.txState = RS232_TX_END_WAIT;
            		sendTxCompleteEvent();
        		}
    			break;
    		case RS232_TX_COMPLETED:
    			//Code should not reach here. Ignore
    			disHMITxInt();
    			break;
    		case RS232_TX_END_WAIT:
    			//Code should not reach here. Ignore
    			disHMITxInt();
    			break;

    		default:
    			break;
    		}
        }
        else if(kLPUART_TransmissionCompleteFlag & RS232PortFlags)
        {
        	if(HMItrBuf.txState == RS232_TX_END_WAIT)
        	{
        		HMItrBuf.txState = RS232_TX_COMPLETED;
            	//Transmission of the buffer is over. Now send message to HMI Proc
        		//sendTxCompleteEvent();
//            	sendRespCompleteEvent(LPUART_ISR);
        		disHMITxInt();

        	}

        }

        //Handle errors
        if(RS232PortFlags & LPUART_STAT_OR(0b1))
        {
        	//Over run error
        	LPUART_ClearStatusFlags(HMI_LPUART, LPUART_STAT_OR(0b1));
        }
        else if(RS232PortFlags & LPUART_STAT_NF(0b1))
        {
        	//Noise Flag. Clear
        	LPUART_ClearStatusFlags(HMI_LPUART, LPUART_STAT_NF(0b1));
        }
    //RED_LED_OFF();
    SDK_ISR_EXIT_BARRIER;
}



int initHMIUart()
{
    lpuart_config_t config;

	HMItrBuf.txState = RS232_TX_IDLE;
	HMItrBuf.rxState = RS232_RCV_IDLE;

    /* attach FRO 12M to FLEXCOMM0 */
    CLOCK_SetClkDiv(kCLOCK_DivFlexcom0Clk, 1u);
    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM0);

	 if(LP_FLEXCOMM_Init(LPFLEXCOMM_INSTANCE0, LP_FLEXCOMM_PERIPH_LPUART) != kStatus_Success)
	 {
	    	printf("hmiUartDriver.c:initHMIUart():Flexcomm0 Init Fail code:%d\r\n",kStatus_Success);
	 }
	 else
	    	printf("hmiUartDriver.c:initHMIUart():Flexcomm0 Init Success\r\n");


/*     config.baudRate_Bps = 115200U;
     config.parityMode = kLPUART_ParityDisabled;
     config.stopBitCount = kLPUART_OneStopBit;
     config.enableTx = true;
     config.enableRx = true;*/

    LPUART_GetDefaultConfig(&config);
    config.baudRate_Bps = HMI_UART_BAUDRATE;
    config.parityMode = kLPUART_ParityDisabled;
    config.stopBitCount = kLPUART_OneStopBit;
    config.enableTx     = true;
    config.enableRx     = true;

   if( LPUART_Init(HMI_LPUART, &config, HMI_LPUART_CLK_FREQ)!= kStatus_Success)
   {
	   printf("hmiUartDriver.c:initHMIUart(): fail\r\n");
	   return DG_FAIL;
   }

   NVIC_SetPriority(HMI_LPUART_IRQn, 4);
    /* Enable RX interrupt. */
   LPUART_EnableInterrupts(HMI_LPUART, kLPUART_RxDataRegFullInterruptEnable);
   EnableIRQ(HMI_LPUART_IRQn);

   return(DG_SUCCESS);
}
