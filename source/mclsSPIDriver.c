/*
 * mclsSPIDriver.c
 *
 *  Created on: 21-Aug-2025
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
#include "dgUartDriverCommon.h"
#include "CliUartDriver.h"
#include "cliProc.h"
#include "sysConfig.h"
#include "mclsSPIDriver.h"
#include "drv89xxDriver.h"
#include "drv89xxRegisters.h"



/*******************************************************************************
 * Global Variables
 ******************************************************************************/

dgSpiHandle_t spihandle;

/*******************************************************************************
 * Implementation
 ******************************************************************************/


int initSPI_MCLS()  //Motor control and Level sensor SPI init
{
    lpspi_master_config_t masterConfig;
    uint32_t srcClock_Hz;

	//FC2 Used for SPI to interface with DRV8912/DRV8908/TDC1000

    /* attach FRO 12M to FLEXCOMM2 */
    CLOCK_SetClkDiv(kCLOCK_DivFlexcom2Clk, 1u);
    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM2);

	//Initialize LP_FLEXCOMM2 as LPSPI
	 if(LP_FLEXCOMM_Init(LPFLEXCOMM_INSTANCE2, LP_FLEXCOMM_PERIPH_LPSPI) != kStatus_Success)
	 {
	 	printf("mclsSPIDriver.c:initSPI_MCLS():Flexcomm2 init fail, code:%d\r\n",kStatus_Success);
	 }
	 else
	    	printf("mclsSPIDriver.c:initSPI_MCLS():Flexcomm2 as LPSPI init Success\r\n");

	 //Configure LPSPI port
	 /*Master config*/
	 LPSPI_MasterGetDefaultConfig(&masterConfig);

	 masterConfig.baudRate = TRANSFER_BAUDRATE;
	 masterConfig.bitsPerFrame = 16;               //16 bits
	 //masterConfig.cpha = kLPSPI_ClockPhaseSecondEdge;
	 //masterConfig.cpol = kLPSPI_ClockPolarityActiveHigh;
	 masterConfig.pcsToSckDelayInNanoSec        = 1000000000U / (masterConfig.baudRate * 2U);
	 masterConfig.lastSckToPcsDelayInNanoSec    = 1000000000U / (masterConfig.baudRate * 2U);
	 masterConfig.betweenTransferDelayInNanoSec = 1000000000U / (masterConfig.baudRate * 2U);
 	 masterConfig.whichPcs = (kLPSPI_Pcs1);

	 srcClock_Hz = CLOCK_GetLPFlexCommClkFreq(LPFLEXCOMM_INSTANCE2);
	 LPSPI_MasterInit(MCLS_LPSPI_MASTER_BASEADDR, &masterConfig, srcClock_Hz);


	(void)memset(&spihandle, 0, sizeof(dgSpiHandle_t));

	spihandle.mutex = xSemaphoreCreateMutex();
	if (spihandle.mutex == NULL)
	{
		return DG_FAIL;
	}

	spihandle.semaphore = xSemaphoreCreateBinary();
	if (spihandle.semaphore == NULL)
	{
		vSemaphoreDelete(spihandle.mutex);
		return DG_FAIL;
	}

	 return DG_SUCCESS;

}

int spiReadDrv89xx(uint8_t spiDeviceID, uint8_t regAddr, uint16_t *data)
{
	uint32_t writeData, readData;
	uint8_t spiDevicePCS;

	if(data == NULL)
	{
		return DG_INVALID_PARAM;
	}

	//lock mutex
    if(xSemaphoreTake(spihandle.mutex, portMAX_DELAY) != pdTRUE)
    {
        return DG_BUSY;
    }

	switch(spiDeviceID)
	{
	case DRV89XX_1:
		spiDevicePCS = (kLPSPI_Pcs1);
		break;
	case DRV89XX_2:
		spiDevicePCS = (kLPSPI_Pcs2);
		break;
	case TDC1000_1:
		spiDevicePCS = (kLPSPI_Pcs0);
		break;
	case TDC1000_2:
		spiDevicePCS = (kLPSPI_Pcs3);
		break;

	case DRV89XX_3:
	default:
	    /* Unlock resource mutex */
	    xSemaphoreGive(spihandle.mutex);
		return DG_INVALID_PARAM;
		break;
	}


    //Enable LPSPI2
    LPSPI_Enable(MCLS_LPSPI_MASTER_BASEADDR, true);

    /*Flush FIFO , clear status , disable all the inerrupts.*/
    LPSPI_FlushFifo(MCLS_LPSPI_MASTER_BASEADDR, true, true);
    LPSPI_ClearStatusFlags(MCLS_LPSPI_MASTER_BASEADDR, kLPSPI_AllStatusFlag);
    LPSPI_DisableInterrupts(MCLS_LPSPI_MASTER_BASEADDR, kLPSPI_AllInterruptEnable);

    //Create a command word for read with regAddr
    writeData = (((uint32_t)regAddr << 8) & 0x00003F00) | 0x00004000;


	//Write Transmit Control register TCR
    MCLS_LPSPI_MASTER_BASEADDR->TCR = (MCLS_LPSPI_MASTER_BASEADDR->TCR & ~(LPSPI_TCR_CPHA_MASK | LPSPI_TCR_CPOL_MASK | LPSPI_TCR_CONT_MASK | LPSPI_TCR_CONTC_MASK | LPSPI_TCR_RXMSK_MASK |LPSPI_TCR_TXMSK_MASK | LPSPI_TCR_PCS_MASK)) |
    		LPSPI_TCR_CPHA(kLPSPI_ClockPhaseSecondEdge) | LPSPI_TCR_CPOL(kLPSPI_ClockPolarityActiveHigh) | LPSPI_TCR_CONT(0) | LPSPI_TCR_CONTC(0) | LPSPI_TCR_RXMSK(0) | LPSPI_TCR_TXMSK(0) | LPSPI_TCR_PCS(spiDevicePCS);
    //MCLS_LPSPI_MASTER_BASEADDR->TCR = (MCLS_LPSPI_MASTER_BASEADDR->TCR & ~(LPSPI_TCR_CPHA_MASK | LPSPI_TCR_CPOL_MASK | LPSPI_TCR_CONT_MASK | LPSPI_TCR_CONTC_MASK | LPSPI_TCR_RXMSK_MASK |LPSPI_TCR_TXMSK_MASK | LPSPI_TCR_PCS_MASK)) |
       		//LPSPI_TCR_CPHA(kLPSPI_ClockPhaseSecondEdge) | LPSPI_TCR_CPOL(kLPSPI_ClockPolarityActiveLow) | LPSPI_TCR_CONT(0) | LPSPI_TCR_CONTC(0) | LPSPI_TCR_RXMSK(0) | LPSPI_TCR_TXMSK(0) | LPSPI_TCR_PCS(spiDevicePCS);

    //Write the word into transmit register
    LPSPI_WriteData(MCLS_LPSPI_MASTER_BASEADDR, writeData);
    //Enable interrupt
	NVIC_SetPriority(LPSPI_IRQN, 4);
    //LPSPI_EnableInterrupts(MCLS_LPSPI_MASTER_BASEADDR, kLPSPI_RxInterruptEnable|kLPSPI_FrameCompleteInterruptEnable );
    LPSPI_EnableInterrupts(MCLS_LPSPI_MASTER_BASEADDR, kLPSPI_FrameCompleteInterruptEnable );
    EnableIRQ(LPSPI_IRQN);

    /* Wait for transfer to finish */
    /*TODO*/
    //Add a timeout instead of indefinite waite. Change portMAX_DELAY to some value
    (void)xSemaphoreTake(spihandle.semaphore, portMAX_DELAY);

    readData = LPSPI_ReadData(MCLS_LPSPI_MASTER_BASEADDR);
    *data = (uint16_t)readData;
    /* Unlock resource mutex */
    xSemaphoreGive(spihandle.mutex);

	return DG_SUCCESS;
}

int spiWriteDrv89xx(uint8_t spiDeviceID, uint8_t regAddr, uint8_t data)
{
	uint32_t writeData, readData;
	uint8_t spiDevicePCS;

	//lock mutex
    if(xSemaphoreTake(spihandle.mutex, portMAX_DELAY) != pdTRUE)
    {
        return DG_BUSY;
    }

	switch(spiDeviceID)
	{
	case DRV89XX_1:
		spiDevicePCS = (kLPSPI_Pcs1);
		break;
	case DRV89XX_2:
		spiDevicePCS = (kLPSPI_Pcs2);
		break;
	case TDC1000_1:
		spiDevicePCS = (kLPSPI_Pcs0);
		break;
	case TDC1000_2:
		spiDevicePCS = (kLPSPI_Pcs3);
		break;

	case DRV89XX_3:
	default:
	    /* Unlock resource mutex */
	    xSemaphoreGive(spihandle.mutex);
		return DG_INVALID_PARAM;
		break;
	}
    //Enable LPSPI2
    LPSPI_Enable(MCLS_LPSPI_MASTER_BASEADDR, true);

    /*Flush FIFO , clear status , disable all the inerrupts.*/
    LPSPI_FlushFifo(MCLS_LPSPI_MASTER_BASEADDR, true, true);
    LPSPI_ClearStatusFlags(MCLS_LPSPI_MASTER_BASEADDR, kLPSPI_AllStatusFlag);
    LPSPI_DisableInterrupts(MCLS_LPSPI_MASTER_BASEADDR, kLPSPI_AllInterruptEnable);

    //Create a command word to write with regAddr
    writeData = (((uint32_t)regAddr << 8) & 0x00003F00) + (data & 0xFF);


	//Write Transmit Control register TCR
    MCLS_LPSPI_MASTER_BASEADDR->TCR = (MCLS_LPSPI_MASTER_BASEADDR->TCR & ~(LPSPI_TCR_CPHA_MASK | LPSPI_TCR_CPOL_MASK | LPSPI_TCR_CONT_MASK | LPSPI_TCR_CONTC_MASK | LPSPI_TCR_RXMSK_MASK |LPSPI_TCR_TXMSK_MASK | LPSPI_TCR_PCS_MASK)) |
    		LPSPI_TCR_CPHA(kLPSPI_ClockPhaseSecondEdge) | LPSPI_TCR_CPOL(kLPSPI_ClockPolarityActiveHigh) | LPSPI_TCR_CONT(0) | LPSPI_TCR_CONTC(0) | LPSPI_TCR_RXMSK(0) | LPSPI_TCR_TXMSK(0) | LPSPI_TCR_PCS(spiDevicePCS);
    //MCLS_LPSPI_MASTER_BASEADDR->TCR = (MCLS_LPSPI_MASTER_BASEADDR->TCR & ~(LPSPI_TCR_CPHA_MASK | LPSPI_TCR_CPOL_MASK | LPSPI_TCR_CONT_MASK | LPSPI_TCR_CONTC_MASK | LPSPI_TCR_RXMSK_MASK |LPSPI_TCR_TXMSK_MASK | LPSPI_TCR_PCS_MASK)) |
     		//LPSPI_TCR_CPHA(kLPSPI_ClockPhaseSecondEdge) | LPSPI_TCR_CPOL(kLPSPI_ClockPolarityActiveLow) | LPSPI_TCR_CONT(0) | LPSPI_TCR_CONTC(0) | LPSPI_TCR_RXMSK(0) | LPSPI_TCR_TXMSK(0) | LPSPI_TCR_PCS(spiDevicePCS);

    //Write the word into transmit register
    LPSPI_WriteData(MCLS_LPSPI_MASTER_BASEADDR, writeData);
    //Enable interrupt
	NVIC_SetPriority(LPSPI_IRQN, 4);
    //LPSPI_EnableInterrupts(MCLS_LPSPI_MASTER_BASEADDR, kLPSPI_RxInterruptEnable|kLPSPI_FrameCompleteInterruptEnable );
	LPSPI_EnableInterrupts(MCLS_LPSPI_MASTER_BASEADDR, kLPSPI_FrameCompleteInterruptEnable );
    EnableIRQ(LPSPI_IRQN);

    /* Wait for transfer to finish */
    /*TODO*/
    //Add a timeout instead of indefinite waite. Change portMAX_DELAY to some value
    (void)xSemaphoreTake(spihandle.semaphore, portMAX_DELAY);

    readData = LPSPI_ReadData(MCLS_LPSPI_MASTER_BASEADDR);
    /* Unlock resource mutex */
    (void)xSemaphoreGive(spihandle.mutex);


	return DG_SUCCESS;
}

int spiReadTDC1000(uint8_t spiDeviceID, uint8_t regAddr, uint8_t *data)
{
	uint32_t writeData, readData;
	uint8_t spiDevicePCS;

	if(data == NULL)
	{
		return DG_INVALID_PARAM;
	}

	//lock mutex
    if(xSemaphoreTake(spihandle.mutex, portMAX_DELAY) != pdTRUE)
    {
        return DG_BUSY;
    }

	switch(spiDeviceID)
	{
	case DRV89XX_1:
		spiDevicePCS = (kLPSPI_Pcs1);
		break;
	case DRV89XX_2:
		spiDevicePCS = (kLPSPI_Pcs2);
		break;
	case TDC1000_1:
		spiDevicePCS = (kLPSPI_Pcs0);
		break;
	case TDC1000_2:
		spiDevicePCS = (kLPSPI_Pcs3);
		break;

	case DRV89XX_3:
	default:
	    /* Unlock resource mutex */
	    xSemaphoreGive(spihandle.mutex);
		return DG_INVALID_PARAM;
		break;
	}


    //Enable LPSPI2
    LPSPI_Enable(MCLS_LPSPI_MASTER_BASEADDR, true);

    /*Flush FIFO , clear status , disable all the inerrupts.*/
    LPSPI_FlushFifo(MCLS_LPSPI_MASTER_BASEADDR, true, true);
    LPSPI_ClearStatusFlags(MCLS_LPSPI_MASTER_BASEADDR, kLPSPI_AllStatusFlag);
    LPSPI_DisableInterrupts(MCLS_LPSPI_MASTER_BASEADDR, kLPSPI_AllInterruptEnable);

    //Create a command word for read with regAddr
    writeData = (((uint32_t)regAddr << 8) & 0x00003F00);


	//Write Transmit Control register TCR
    MCLS_LPSPI_MASTER_BASEADDR->TCR = (MCLS_LPSPI_MASTER_BASEADDR->TCR & ~(LPSPI_TCR_CPHA_MASK | LPSPI_TCR_CPOL_MASK | LPSPI_TCR_CONT_MASK | LPSPI_TCR_CONTC_MASK | LPSPI_TCR_RXMSK_MASK |LPSPI_TCR_TXMSK_MASK | LPSPI_TCR_PCS_MASK)) |
    		LPSPI_TCR_CPHA(kLPSPI_ClockPhaseSecondEdge) | LPSPI_TCR_CPOL(kLPSPI_ClockPolarityActiveHigh) | LPSPI_TCR_CONT(0) | LPSPI_TCR_CONTC(0) | LPSPI_TCR_RXMSK(0) | LPSPI_TCR_TXMSK(0) | LPSPI_TCR_PCS(spiDevicePCS);

    //Write the word into transmit register
    LPSPI_WriteData(MCLS_LPSPI_MASTER_BASEADDR, writeData);
    //Enable interrupt
	NVIC_SetPriority(LPSPI_IRQN, 4);
    //LPSPI_EnableInterrupts(MCLS_LPSPI_MASTER_BASEADDR, kLPSPI_RxInterruptEnable|kLPSPI_FrameCompleteInterruptEnable );
    LPSPI_EnableInterrupts(MCLS_LPSPI_MASTER_BASEADDR, kLPSPI_FrameCompleteInterruptEnable );
    EnableIRQ(LPSPI_IRQN);

    /* Wait for transfer to finish */
    /*TODO*/
    //Add a timeout instead of indefinite waite. Change portMAX_DELAY to some value
    (void)xSemaphoreTake(spihandle.semaphore, portMAX_DELAY);

    readData = LPSPI_ReadData(MCLS_LPSPI_MASTER_BASEADDR);
    *data = (uint8_t)readData;
    /* Unlock resource mutex */
    xSemaphoreGive(spihandle.mutex);

	return DG_SUCCESS;
}

int spiWriteTDC1000(uint8_t spiDeviceID, uint8_t regAddr, uint8_t data)
{
	uint32_t writeData, readData;
	uint8_t spiDevicePCS;

	//lock mutex
    if(xSemaphoreTake(spihandle.mutex, portMAX_DELAY) != pdTRUE)
    {
        return DG_BUSY;
    }

	switch(spiDeviceID)
	{
	case DRV89XX_1:
		spiDevicePCS = (kLPSPI_Pcs1);
		break;
	case DRV89XX_2:
		spiDevicePCS = (kLPSPI_Pcs2);
		break;
	case TDC1000_1:
		spiDevicePCS = (kLPSPI_Pcs0);
		break;
	case TDC1000_2:
		spiDevicePCS = (kLPSPI_Pcs3);
		break;

	case DRV89XX_3:
	default:
	    /* Unlock resource mutex */
	    xSemaphoreGive(spihandle.mutex);
		return DG_INVALID_PARAM;
		break;
	}
    //Enable LPSPI2
    LPSPI_Enable(MCLS_LPSPI_MASTER_BASEADDR, true);

    /*Flush FIFO , clear status , disable all the inerrupts.*/
    LPSPI_FlushFifo(MCLS_LPSPI_MASTER_BASEADDR, true, true);
    LPSPI_ClearStatusFlags(MCLS_LPSPI_MASTER_BASEADDR, kLPSPI_AllStatusFlag);
    LPSPI_DisableInterrupts(MCLS_LPSPI_MASTER_BASEADDR, kLPSPI_AllInterruptEnable);

    //Create a command word to write with regAddr
    writeData = ((((uint32_t)regAddr << 8) & 0x00003F00) | 0x00004000) + (data & 0xFF);


	//Write Transmit Control register TCR
    MCLS_LPSPI_MASTER_BASEADDR->TCR = (MCLS_LPSPI_MASTER_BASEADDR->TCR & ~(LPSPI_TCR_CPHA_MASK | LPSPI_TCR_CPOL_MASK | LPSPI_TCR_CONT_MASK | LPSPI_TCR_CONTC_MASK | LPSPI_TCR_RXMSK_MASK |LPSPI_TCR_TXMSK_MASK | LPSPI_TCR_PCS_MASK)) |
    		LPSPI_TCR_CPHA(kLPSPI_ClockPhaseSecondEdge) | LPSPI_TCR_CPOL(kLPSPI_ClockPolarityActiveHigh) | LPSPI_TCR_CONT(0) | LPSPI_TCR_CONTC(0) | LPSPI_TCR_RXMSK(0) | LPSPI_TCR_TXMSK(0) | LPSPI_TCR_PCS(spiDevicePCS);

    //Write the word into transmit register
    LPSPI_WriteData(MCLS_LPSPI_MASTER_BASEADDR, writeData);
    //Enable interrupt
	NVIC_SetPriority(LPSPI_IRQN, 4);
    //LPSPI_EnableInterrupts(MCLS_LPSPI_MASTER_BASEADDR, kLPSPI_RxInterruptEnable|kLPSPI_FrameCompleteInterruptEnable );
	LPSPI_EnableInterrupts(MCLS_LPSPI_MASTER_BASEADDR, kLPSPI_FrameCompleteInterruptEnable );
    EnableIRQ(LPSPI_IRQN);

    /* Wait for transfer to finish */
    /*TODO*/
    //Add a timeout instead of indefinite waite. Change portMAX_DELAY to some value
    (void)xSemaphoreTake(spihandle.semaphore, portMAX_DELAY);

    readData = LPSPI_ReadData(MCLS_LPSPI_MASTER_BASEADDR);
    /* Unlock resource mutex */
    (void)xSemaphoreGive(spihandle.mutex);


	return DG_SUCCESS;
}


void LPSPI_IRQHandler(void)
{
	uint32_t lpspiStatus;

	//Read status register
	lpspiStatus = LPSPI_GetStatusFlags(MCLS_LPSPI_MASTER_BASEADDR);
	if(lpspiStatus & kLPSPI_FrameCompleteFlag)
	{
		//Transfer is complete. Disable interrupt
	    LPSPI_DisableInterrupts(MCLS_LPSPI_MASTER_BASEADDR, kLPSPI_AllInterruptEnable);

		//Inform the transfer API method
		xSemaphoreGiveFromISR(spihandle.semaphore, NULL);
	}
    SDK_ISR_EXIT_BARRIER;
}
