/*
 * dgI2cDriver.c
 *
 *  Created on: 23-Aug-2025
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
#include "GPIOSignals.h"
#include "dgI2cDriver.h"





/*******************************************************************************
 * Global Variables
 ******************************************************************************/

dgI2cHandle_t rtcI2cHandle;     	//RTC and EEPROM slave devices

dgI2cHandle_t cameraI2cHandle;     	//Camera and Humidity/temperature sensor

uint8_t rtcI2cTxBuff[LPI2C_DATA_LENGTH_EE];
uint8_t rtcI2cRxBuff[LPI2C_DATA_LENGTH_EE];

uint8_t cameraI2cTxBuff[LPI2C_DATA_LENGTH];
uint8_t cameraI2cRxBuff[LPI2C_DATA_LENGTH];


/*******************************************************************************
 * Implementation
 ******************************************************************************/

static void rtcI2c_master_callback(LPI2C_Type *base, lpi2c_master_handle_t *handle, status_t status, void *userData)
{
	dgI2cHandle_t *i2chandle;

	i2chandle = (dgI2cHandle_t *)userData;
	i2chandle->status = status;
	xSemaphoreGiveFromISR(i2chandle->semaphore, NULL);
}


int initRTC_I2C()
{

     lpi2c_master_config_t masterConfig;
     uint32_t srcClock_Hz;


     /* attach FRO 12M to FLEXCOMM6 */
     CLOCK_SetClkDiv(kCLOCK_DivFlexcom6Clk, 1u);
     CLOCK_AttachClk(kFRO12M_to_FLEXCOMM6);


    (void)memset(&rtcI2cHandle, 0, sizeof(dgI2cHandle_t));

    rtcI2cHandle.mutex = xSemaphoreCreateMutex();
    if (rtcI2cHandle.mutex == NULL)
    {
        return DG_FAIL;
    }

    rtcI2cHandle.semaphore = xSemaphoreCreateBinary();
    if (rtcI2cHandle.semaphore == NULL)
    {
        vSemaphoreDelete(rtcI2cHandle.mutex);
        return DG_FAIL;
    }
    rtcI2cHandle.status = kStatus_Success;

	//Initialize LP_FLEXCOMM6 as LPI2C
	 if(LP_FLEXCOMM_Init(LPFLEXCOMM_INSTANCE6, LP_FLEXCOMM_PERIPH_LPI2C) != kStatus_Success)
	 {
	 	printf("Flexcomm6 as I2C init fail, code:%d\r\n",kStatus_Success);
	 }
	 else
	    	printf("Flexcomm6 as LPI2C (RTC, EEPROM) init Success\r\n");

    /*
     * masterConfig.debugEnable = false;
     * masterConfig.ignoreAck = false;
     * masterConfig.pinConfig = kLPI2C_2PinOpenDrain;
     * masterConfig.baudRate_Hz = 100000U;
     * masterConfig.busIdleTimeout_ns = 0;
     * masterConfig.pinLowTimeout_ns = 0;
     * masterConfig.sdaGlitchFilterWidth_ns = 0;
     * masterConfig.sclGlitchFilterWidth_ns = 0;
     */
    LPI2C_MasterGetDefaultConfig(&masterConfig);

    /* Change the default baudrate configuration */
    masterConfig.baudRate_Hz = RTC_I2C_BAUDRATE;

    srcClock_Hz = CLOCK_GetLPFlexCommClkFreq(LPFLEXCOMM_INSTANCE6);
    /* Initialize the LPI2C master peripheral */
    LPI2C_MasterInit(RTC_I2C_MASTER, &masterConfig, srcClock_Hz);
	NVIC_SetPriority(LP_FLEXCOMM6_IRQn, I2C_IRQ_PRIORITY);
    /* Create the LPI2C handle for the non-blocking transfer */
    LPI2C_MasterTransferCreateHandle(RTC_I2C_MASTER, &rtcI2cHandle.g_m_handle, rtcI2c_master_callback, (void*)&rtcI2cHandle);
    //EnableIRQ(LP_FLEXCOMM6_IRQn);
    return DG_SUCCESS;
}

/*!
 * brief Deinitializes the LPI2C.
 *
 * This function deinitializes the LPI2C module and related RTOS context.
 *
 * param handle The RTOS LPI2C handle.
 */
int deinitRTC_I2C()
{
    LPI2C_MasterDeinit(RTC_I2C_MASTER);

    vSemaphoreDelete(rtcI2cHandle.semaphore);
    vSemaphoreDelete(rtcI2cHandle.mutex);

    return DG_SUCCESS;
}


int mcp7940_reg_read(uint8_t reg_addr, uint8_t* reg_value)
{

	lpi2c_master_transfer_t rtcByteXfer;
	 status_t status;

	//lock mutex

    if (xSemaphoreTake(rtcI2cHandle.mutex, portMAX_DELAY) != pdTRUE)
    {
        return DG_BUSY;
    }

	//EEPROM Xfer buffer for byte write
    rtcByteXfer.slaveAddress 	= MCP7940_SLAVE_ADDR;
    rtcByteXfer.direction      = kLPI2C_Read;
    rtcByteXfer.subaddress     = (uint32_t)reg_addr;
    rtcByteXfer.subaddressSize = 1;
    rtcByteXfer.data           = rtcI2cRxBuff;
    rtcByteXfer.dataSize       = 1;
    rtcByteXfer.flags          = kLPI2C_TransferDefaultFlag;

    status = LPI2C_MasterTransferNonBlocking(RTC_I2C_MASTER, &rtcI2cHandle.g_m_handle, &rtcByteXfer);
    if (status != kStatus_Success)
    {
        (void)xSemaphoreGive(rtcI2cHandle.mutex);
        return DG_FAIL;
    }

    /* Wait for transfer to finish */
    (void)xSemaphoreTake(rtcI2cHandle.semaphore, portMAX_DELAY);

    /* Unlock resource mutex */
    (void)xSemaphoreGive(rtcI2cHandle.mutex);

    /* Return status captured by callback function */
    if(rtcI2cHandle.status ==kStatus_Success)
    {
    	*reg_value = rtcI2cRxBuff[0];
    	return DG_SUCCESS;
    }

    return DG_FAIL;

}

int mcp7940_reg_write(uint8_t reg_addr, uint8_t reg_value)
{
	lpi2c_master_transfer_t rtcByteWriteXfer;
	 status_t status;

	//lock mutex

    if (xSemaphoreTake(rtcI2cHandle.mutex, portMAX_DELAY) != pdTRUE)
    {
        return DG_BUSY;
    }

	//EEPROM Xfer buffer for byte write
	rtcI2cTxBuff[0] = reg_value;
	rtcByteWriteXfer.slaveAddress 	= MCP7940_SLAVE_ADDR;
	rtcByteWriteXfer.direction      = kLPI2C_Write;
	rtcByteWriteXfer.subaddress     = (uint32_t)reg_addr;
	rtcByteWriteXfer.subaddressSize = 1;
	rtcByteWriteXfer.data           = rtcI2cTxBuff;
	rtcByteWriteXfer.dataSize       = 1;
	rtcByteWriteXfer.flags          = kLPI2C_TransferDefaultFlag;

    status = LPI2C_MasterTransferNonBlocking(RTC_I2C_MASTER, &rtcI2cHandle.g_m_handle, &rtcByteWriteXfer);
    if (status != kStatus_Success)
    {
        (void)xSemaphoreGive(rtcI2cHandle.mutex);
        return DG_FAIL;
    }

    /* Wait for transfer to finish */
    (void)xSemaphoreTake(rtcI2cHandle.semaphore, portMAX_DELAY);

    /* Unlock resource mutex */
    (void)xSemaphoreGive(rtcI2cHandle.mutex);

    /* Return status captured by callback function */
    if(rtcI2cHandle.status ==kStatus_Success)
    {
    	return DG_SUCCESS;
    }
    return DG_FAIL;
}

/*------------------------- EEPROM Driver methods ----------------------------------*/
int eeprom_mem_read_within_page(uint16_t addr, uint8_t* buffer, uint8_t size)
{

	lpi2c_master_transfer_t eepromByteXfer;
	status_t status;

	 //Validate input parameters
	 if((buffer==NULL)||(size==0)||(size > EEPROM_PAGE_SIZE))
	 {
	        return DG_INVALID_PARAM;
	 }
	 //Entire write area has to  be within a page
	 if(((addr+(size-1))& EEPROM_PAGE_SIZE_MASK) != (addr & EEPROM_PAGE_SIZE_MASK))
	 {
	        return DG_INVALID_PARAM;
	 }

	//lock mutex

    if (xSemaphoreTake(rtcI2cHandle.mutex, portMAX_DELAY) != pdTRUE)
    {
        return DG_BUSY;
    }

	//EEPROM Xfer buffer for byte write
    eepromByteXfer.slaveAddress 	= EEPROM_SLAVE_MEM_ADDR;
    eepromByteXfer.direction      	= kLPI2C_Read;
    eepromByteXfer.subaddress     	= (uint32_t)addr;
    eepromByteXfer.subaddressSize 	= 2;
    eepromByteXfer.data           	= (void*)buffer;
    eepromByteXfer.dataSize       	= size;
    eepromByteXfer.flags          	= kLPI2C_TransferDefaultFlag;

    status = LPI2C_MasterTransferNonBlocking(RTC_I2C_MASTER, &rtcI2cHandle.g_m_handle, &eepromByteXfer);
    if (status != kStatus_Success)
    {
        (void)xSemaphoreGive(rtcI2cHandle.mutex);
        return DG_FAIL;
    }

    /* Wait for transfer to finish */
    (void)xSemaphoreTake(rtcI2cHandle.semaphore, portMAX_DELAY);

    /* Unlock resource mutex */
    (void)xSemaphoreGive(rtcI2cHandle.mutex);

    /* Return status captured by callback function */
    if(rtcI2cHandle.status ==kStatus_Success)
    {
    	return DG_SUCCESS;
    }

    return DG_FAIL;

}

int eeprom_mem_write_within_page(uint16_t addr, uint8_t* buffer, uint8_t size)
{

	lpi2c_master_transfer_t eepromByteXfer;
	status_t status;

	 //Validate input parameters
	 if((buffer==NULL)||(size==0)||(size > EEPROM_PAGE_SIZE))
	 {
	        return DG_INVALID_PARAM;
	 }
	 //Entire write area has to  be within a page
	 if(((addr+(size-1))& EEPROM_PAGE_SIZE_MASK) != (addr & EEPROM_PAGE_SIZE_MASK))
	 {
	        return DG_INVALID_PARAM;
	 }

	//lock mutex

    if (xSemaphoreTake(rtcI2cHandle.mutex, portMAX_DELAY) != pdTRUE)
    {
        return DG_BUSY;
    }

	//EEPROM Xfer buffer for byte write
    eepromByteXfer.slaveAddress 	= EEPROM_SLAVE_MEM_ADDR;
    eepromByteXfer.direction      	= kLPI2C_Write;
    eepromByteXfer.subaddress     	= (uint32_t)addr;
    eepromByteXfer.subaddressSize 	= 2;
    eepromByteXfer.data           	= (void*)buffer;
    eepromByteXfer.dataSize       	= size;
    eepromByteXfer.flags          	= kLPI2C_TransferDefaultFlag;

    status = LPI2C_MasterTransferNonBlocking(RTC_I2C_MASTER, &rtcI2cHandle.g_m_handle, &eepromByteXfer);
    if (status != kStatus_Success)
    {
        (void)xSemaphoreGive(rtcI2cHandle.mutex);
        return DG_FAIL;
    }

    /* Wait for transfer to finish */
    (void)xSemaphoreTake(rtcI2cHandle.semaphore, portMAX_DELAY);

    /* Unlock resource mutex */
    (void)xSemaphoreGive(rtcI2cHandle.mutex);

    /* Return status captured by callback function */
    if(rtcI2cHandle.status ==kStatus_Success)
    {
    	return DG_SUCCESS;
    }

    return DG_FAIL;

}


/*----------------Driver Initialization and APIs for Camera I2C---------------------*/

static void cameraI2c_master_callback(LPI2C_Type *base, lpi2c_master_handle_t *handle, status_t status, void *userData)
{
	dgI2cHandle_t *i2chandle;

	i2chandle = (dgI2cHandle_t *)userData;
	i2chandle->status = status;
	xSemaphoreGiveFromISR(i2chandle->semaphore, NULL);
}


int initCamera_I2C()
{

     lpi2c_master_config_t masterConfig;
     uint32_t srcClock_Hz;


     /* attach FRO 12M to FLEXCOMM7 */
     CLOCK_SetClkDiv(kCLOCK_DivFlexcom3Clk, 1u);
     CLOCK_AttachClk(kFRO12M_to_FLEXCOMM3);


    (void)memset(&cameraI2cHandle, 0, sizeof(dgI2cHandle_t));

    cameraI2cHandle.mutex = xSemaphoreCreateMutex();
    if (cameraI2cHandle.mutex == NULL)
    {
        return DG_FAIL;
    }

    cameraI2cHandle.semaphore = xSemaphoreCreateBinary();
    if (cameraI2cHandle.semaphore == NULL)
    {
        vSemaphoreDelete(cameraI2cHandle.mutex);
        return DG_FAIL;
    }
    cameraI2cHandle.status = kStatus_Success;

	//Initialize LP_FLEXCOMM3 as LPI2C
	 if(LP_FLEXCOMM_Init(LPFLEXCOMM_INSTANCE3, LP_FLEXCOMM_PERIPH_LPI2C) != kStatus_Success)
	 {
	 	printf("Flexcomm3 as I2C init fail, code:%d\r\n",kStatus_Success);
	 }
	 else
	    	printf("Flexcomm3 as LPI2C init Success\r\n");

    /*
     * masterConfig.debugEnable = false;
     * masterConfig.ignoreAck = false;
     * masterConfig.pinConfig = kLPI2C_2PinOpenDrain;
     * masterConfig.baudRate_Hz = 100000U;
     * masterConfig.busIdleTimeout_ns = 0;
     * masterConfig.pinLowTimeout_ns = 0;
     * masterConfig.sdaGlitchFilterWidth_ns = 0;
     * masterConfig.sclGlitchFilterWidth_ns = 0;
     */
    LPI2C_MasterGetDefaultConfig(&masterConfig);

    /* Change the default baudrate configuration */
    masterConfig.baudRate_Hz = CAMERA_I2C_BAUDRATE;

    srcClock_Hz = CLOCK_GetLPFlexCommClkFreq(LPFLEXCOMM_INSTANCE3);
    /* Initialize the LPI2C master peripheral */
    LPI2C_MasterInit(CAMERA_I2C_MASTER, &masterConfig, srcClock_Hz);
	NVIC_SetPriority(LP_FLEXCOMM3_IRQn, I2C_IRQ_PRIORITY);
    /* Create the LPI2C handle for the non-blocking transfer */
    LPI2C_MasterTransferCreateHandle(CAMERA_I2C_MASTER, &cameraI2cHandle.g_m_handle, cameraI2c_master_callback, (void*)&cameraI2cHandle);
    //EnableIRQ(LP_FLEXCOMM3_IRQn);
    return DG_SUCCESS;
}

/*!
 * brief Deinitializes the LPI2C.
 *
 * This function deinitializes the LPI2C module and related RTOS context.
 *
 * param handle The RTOS LPI2C handle.
 */
int deinitCamera_I2C()
{
    LPI2C_MasterDeinit(CAMERA_I2C_MASTER);

    vSemaphoreDelete(cameraI2cHandle.semaphore);
    vSemaphoreDelete(cameraI2cHandle.mutex);

    return DG_SUCCESS;
}

//Register value should have a storage for 6 bytes
int sht4x_reg_read(uint8_t reg_addr, uint8_t* reg_value)
{

	lpi2c_master_transfer_t cameraByteXfer;
	status_t status;

	//lock mutex

    if (xSemaphoreTake(cameraI2cHandle.mutex, portMAX_DELAY) != pdTRUE)
    {
        return DG_BUSY;
    }

	//EEPROM Xfer buffer for byte write
    cameraByteXfer.slaveAddress 	= SHT4X_SLAVE_ADDR;
    cameraByteXfer.direction      	= kLPI2C_Read;
    cameraByteXfer.subaddress     	= (uint32_t)reg_addr;
    cameraByteXfer.subaddressSize 	= 0;
    cameraByteXfer.data           	= cameraI2cRxBuff;
    cameraByteXfer.dataSize       	= 6;
    cameraByteXfer.flags          	= kLPI2C_TransferDefaultFlag;

    status = LPI2C_MasterTransferNonBlocking(CAMERA_I2C_MASTER, &cameraI2cHandle.g_m_handle, &cameraByteXfer);
    if (status != kStatus_Success)
    {
        (void)xSemaphoreGive(cameraI2cHandle.mutex);
        return DG_FAIL;
    }

    /* Wait for transfer to finish */
    (void)xSemaphoreTake(cameraI2cHandle.semaphore, portMAX_DELAY);

    /* Unlock resource mutex */
    (void)xSemaphoreGive(cameraI2cHandle.mutex);

    /* Return status captured by callback function */
    if(cameraI2cHandle.status ==kStatus_Success)
    {
    	//Copy 6 bytes
    	for(uint8_t i=0; i<6; i++)
    	{
        	reg_value[i] = cameraI2cRxBuff[i];
    	}

    	return DG_SUCCESS;
    }

    return DG_FAIL;

}

int sht4x_setAddress(uint8_t reg_addr)
{
	lpi2c_master_transfer_t cameraByteWriteXfer;
	 status_t status;

	//lock mutex

    if (xSemaphoreTake(cameraI2cHandle.mutex, portMAX_DELAY) != pdTRUE)
    {
        return DG_BUSY;
    }

	//EEPROM Xfer buffer for byte write
	cameraI2cTxBuff[0] = reg_addr;
	cameraByteWriteXfer.slaveAddress 	= SHT4X_SLAVE_ADDR;
	cameraByteWriteXfer.direction      	= kLPI2C_Write;
	cameraByteWriteXfer.subaddress     	= (uint32_t)reg_addr;
	cameraByteWriteXfer.subaddressSize 	= 0;
	cameraByteWriteXfer.data           	= cameraI2cTxBuff;
	cameraByteWriteXfer.dataSize       	= 1;
	cameraByteWriteXfer.flags          	= kLPI2C_TransferDefaultFlag;

    status = LPI2C_MasterTransferNonBlocking(CAMERA_I2C_MASTER, &cameraI2cHandle.g_m_handle, &cameraByteWriteXfer);
    if (status != kStatus_Success)
    {
        (void)xSemaphoreGive(cameraI2cHandle.mutex);
        return DG_FAIL;
    }

    /* Wait for transfer to finish */
    (void)xSemaphoreTake(cameraI2cHandle.semaphore, portMAX_DELAY);

    /* Unlock resource mutex */
    (void)xSemaphoreGive(cameraI2cHandle.mutex);

    /* Return status captured by callback function */
    if(cameraI2cHandle.status ==kStatus_Success)
    {
    	return DG_SUCCESS;
    }
    return DG_FAIL;
}

int sht4x_reg_write(uint8_t reg_addr, uint8_t reg_value)
{
	lpi2c_master_transfer_t cameraByteWriteXfer;
	 status_t status;

	//lock mutex

    if (xSemaphoreTake(cameraI2cHandle.mutex, portMAX_DELAY) != pdTRUE)
    {
        return DG_BUSY;
    }

	//EEPROM Xfer buffer for byte write
	cameraI2cTxBuff[0] = reg_value;
	cameraByteWriteXfer.slaveAddress 	= SHT4X_SLAVE_ADDR;
	cameraByteWriteXfer.direction      	= kLPI2C_Write;
	cameraByteWriteXfer.subaddress     	= (uint32_t)reg_addr;
	cameraByteWriteXfer.subaddressSize 	= 1;
	cameraByteWriteXfer.data           	= cameraI2cTxBuff;
	cameraByteWriteXfer.dataSize       	= 1;
	cameraByteWriteXfer.flags          	= kLPI2C_TransferDefaultFlag;

    status = LPI2C_MasterTransferNonBlocking(CAMERA_I2C_MASTER, &cameraI2cHandle.g_m_handle, &cameraByteWriteXfer);
    if (status != kStatus_Success)
    {
        (void)xSemaphoreGive(cameraI2cHandle.mutex);
        return DG_FAIL;
    }

    /* Wait for transfer to finish */
    (void)xSemaphoreTake(cameraI2cHandle.semaphore, portMAX_DELAY);

    /* Unlock resource mutex */
    (void)xSemaphoreGive(cameraI2cHandle.mutex);

    /* Return status captured by callback function */
    if(cameraI2cHandle.status ==kStatus_Success)
    {
    	return DG_SUCCESS;
    }
    return DG_FAIL;
}


/************************** OV7670 SCCB Register Access methods ******************/


int ov7670_reg_write(uint8_t reg_addr, uint8_t reg_value)
{
	lpi2c_master_transfer_t cameraByteWriteXfer;
	 status_t status;

	//lock mutex

    if (xSemaphoreTake(cameraI2cHandle.mutex, portMAX_DELAY) != pdTRUE)
    {
    	printf("dgI2CDriver.c:ov7670_reg_write():Bus busy\r\n");
        return DG_BUSY;
    }

	//EEPROM Xfer buffer for byte write
	cameraI2cTxBuff[0] = reg_value;
	cameraByteWriteXfer.slaveAddress 	= OV7670_SLAVE_ADDR;
	cameraByteWriteXfer.direction      	= kLPI2C_Write;
	cameraByteWriteXfer.subaddress     	= (uint32_t)reg_addr;
	cameraByteWriteXfer.subaddressSize 	= 1;
	cameraByteWriteXfer.data           	= cameraI2cTxBuff;
	cameraByteWriteXfer.dataSize       	= 1;
	cameraByteWriteXfer.flags          	= kLPI2C_TransferDefaultFlag;

    status = LPI2C_MasterTransferNonBlocking(CAMERA_I2C_MASTER, &cameraI2cHandle.g_m_handle, &cameraByteWriteXfer);
    if (status != kStatus_Success)
    {
        xSemaphoreGive(cameraI2cHandle.mutex);
    	printf("dgI2CDriver.c:ov7670_reg_write():Transact fail\r\n");
        return DG_FAIL;
    }

    /* Wait for transfer to finish */
    xSemaphoreTake(cameraI2cHandle.semaphore, portMAX_DELAY);
    vTaskDelay(1); //Camera sensor requires this delay. Otherwise it does not work
    /* Unlock resource mutex */
    xSemaphoreGive(cameraI2cHandle.mutex);

    /* Return status captured by callback function */
    if(cameraI2cHandle.status ==kStatus_Success)
    {
    	//uint8_t readbackValue;
    	//ov7670_reg_read(reg_addr, &readbackValue);
    	//printf("dgI2CDriver.c:ov7670_reg_write():writeValue=0x%x, readback=0x%x\r\n",reg_value, readbackValue);
    	return DG_SUCCESS;
    }
	printf("dgI2CDriver.c:ov7670_reg_write():write fail\r\n");
    return DG_FAIL;
}

//Register value should have a storage for 1 bytes
int ov7670_reg_read(uint8_t reg_addr, uint8_t* reg_value)
{

	lpi2c_master_transfer_t cameraByteXfer, cameraByteWriteXfer;
	status_t status;

	//lock mutex

    if (xSemaphoreTake(cameraI2cHandle.mutex, portMAX_DELAY) != pdTRUE)
    {
    	printf("dgI2CDriver.c:ov7670_reg_read():Bus Busy\r\n");
        return DG_BUSY;
    }

    //Do a write cycle to set address
	cameraI2cTxBuff[0] = reg_addr;
	cameraByteWriteXfer.slaveAddress 	= OV7670_SLAVE_ADDR;
	cameraByteWriteXfer.direction      	= kLPI2C_Write;
	cameraByteWriteXfer.subaddress     	= 0;
	cameraByteWriteXfer.subaddressSize 	= 0;
	cameraByteWriteXfer.data           	= cameraI2cTxBuff;
	cameraByteWriteXfer.dataSize       	= 1;
	cameraByteWriteXfer.flags          	= kLPI2C_TransferDefaultFlag;

    status = LPI2C_MasterTransferNonBlocking(CAMERA_I2C_MASTER, &cameraI2cHandle.g_m_handle, &cameraByteWriteXfer);
    if (status != kStatus_Success)
    {
        xSemaphoreGive(cameraI2cHandle.mutex);
    	printf("dgI2CDriver.c:ov7670_reg_read():Transact Error\r\n");
        return DG_FAIL;
    }

    /* Wait for transfer to finish */
    (void)xSemaphoreTake(cameraI2cHandle.semaphore, portMAX_DELAY);
    vTaskDelay(1);    //Camera sensor requires this delay. Otherwise it does not work
    //Check the result of address write operation
    if(cameraI2cHandle.status !=kStatus_Success)
    {
    	printf("dgI2CDriver.c:ov7670_reg_read():Address set failed\r\n");
        xSemaphoreGive(cameraI2cHandle.mutex);
        return DG_FAIL;
    }

    //Now read the data from the register

    cameraByteXfer.slaveAddress 	= OV7670_SLAVE_ADDR;
    cameraByteXfer.direction      	= kLPI2C_Read;
    cameraByteXfer.subaddress     	= 0;
    cameraByteXfer.subaddressSize 	= 0;
    cameraByteXfer.data           	= cameraI2cRxBuff;
    cameraByteXfer.dataSize       	= 1;
    cameraByteXfer.flags          	= kLPI2C_TransferDefaultFlag;

    status = LPI2C_MasterTransferNonBlocking(CAMERA_I2C_MASTER, &cameraI2cHandle.g_m_handle, &cameraByteXfer);
    if (status != kStatus_Success)
    {
        xSemaphoreGive(cameraI2cHandle.mutex);
    	printf("dgI2CDriver.c:ov7670_reg_read():read failed\r\n");
        return DG_FAIL;
    }

    /* Wait for transfer to finish */
    (void)xSemaphoreTake(cameraI2cHandle.semaphore, portMAX_DELAY);
    vTaskDelay(1); //Camera sensor requires this delay. Otherwise it does not work
    /* Unlock resource mutex */
    (void)xSemaphoreGive(cameraI2cHandle.mutex);

    /* Return status captured by callback function */
    if(cameraI2cHandle.status ==kStatus_Success)
    {
    	//Copy 1 byte
    	*reg_value = cameraI2cRxBuff[0];

    	return DG_SUCCESS;
    }
	printf("dgI2CDriver.c:ov7670_reg_read():read fail\r\n");
    return DG_FAIL;

}

/******************* Camera I2C methods to work with NXP Camera driver *************/



status_t BOARD_LPI2C_ReceiveSCCB(LPI2C_Type *base,
                             uint8_t deviceAddress,
                             uint32_t subAddress,
                             uint8_t subAddressSize,
                             uint8_t *rxBuff,
                             uint8_t rxBuffSize)
{
    lpi2c_master_transfer_t xfer;
	status_t status;

    xSemaphoreTake(cameraI2cHandle.mutex, portMAX_DELAY);
    vTaskDelay(1); //Camera sensor requires this delay. Otherwise it does not work
    //printf("dgI2cDriver.c:BOARD_LPI2C_ReceiveSCCB():Semaphore taken\r\n");

    xfer.flags          = kLPI2C_TransferDefaultFlag;
    xfer.slaveAddress   = deviceAddress;
    xfer.direction      = kLPI2C_Write;
    xfer.subaddress     = subAddress;
    xfer.subaddressSize = subAddressSize;
    xfer.data           = NULL;
    xfer.dataSize       = 0;


    status = LPI2C_MasterTransferNonBlocking(base, &cameraI2cHandle.g_m_handle, &xfer);
    if (status != kStatus_Success)
    {
        xSemaphoreGive(cameraI2cHandle.mutex);
        return status;
    }
    //printf("dgI2cDriver.c:BOARD_LPI2C_ReceiveSCCB():Address transfer initiated\r\n");
    /* Wait for transfer to finish */
    xSemaphoreTake(cameraI2cHandle.semaphore, portMAX_DELAY);

    //printf("dgI2cDriver.c:BOARD_LPI2C_ReceiveSCCB():Address transfer completed\r\n");
    vTaskDelay(1);    //Camera sensor requires this delay. Otherwise it does not work

    if(cameraI2cHandle.status != kStatus_Success)
    {
        printf("dgI2cDriver.c:BOARD_LPI2C_ReceiveSCCB():Address transfer error\r\n");
    	return cameraI2cHandle.status;
    }
    xfer.flags          = kLPI2C_TransferDefaultFlag;
    xfer.slaveAddress   = deviceAddress;
    xfer.direction      = kLPI2C_Read;
    xfer.subaddress     = subAddress;
    xfer.subaddressSize = 0;
    xfer.data           = rxBuff;
    xfer.dataSize       = rxBuffSize;

    status = LPI2C_MasterTransferNonBlocking(base, &cameraI2cHandle.g_m_handle, &xfer);
    if (status != kStatus_Success)
    {
        xSemaphoreGive(cameraI2cHandle.mutex);
        printf("dgI2cDriver.c:BOARD_LPI2C_ReceiveSCCB():Transact error\r\n");
        return status;
    }
    //printf("dgI2cDriver.c:BOARD_LPI2C_ReceiveSCCB():Data read initiated\r\n");
    vTaskDelay(1); //Camera sensor requires this delay. Otherwise it does not work
    /* Unlock resource mutex */
    xSemaphoreGive(cameraI2cHandle.mutex);

    //printf("dgI2cDriver.c:BOARD_LPI2C_ReceiveSCCB():Data received\r\n");
    if(cameraI2cHandle.status != kStatus_Success)
    {
        printf("dgI2cDriver.c:BOARD_LPI2C_ReceiveSCCB():Transact error\r\n");
    }
    /* Return status captured by callback function */
    return cameraI2cHandle.status;
}

/*status_t BOARD_Camera_I2C_Send(
    uint8_t deviceAddress, uint32_t subAddress, uint8_t subAddressSize, const uint8_t *txBuff, uint8_t txBuffSize)
{
    return BOARD_LPI2C_Send(CAMERA_I2C_MASTER, deviceAddress, subAddress, subAddressSize, (uint8_t *)txBuff,
                            txBuffSize);
}

status_t BOARD_Camera_I2C_Receive(
    uint8_t deviceAddress, uint32_t subAddress, uint8_t subAddressSize, uint8_t *rxBuff, uint8_t rxBuffSize)
{
    return BOARD_LPI2C_Receive(CAMERA_I2C_MASTER, deviceAddress, subAddress, subAddressSize, rxBuff,
                               rxBuffSize);
}*/

status_t BOARD_LPI2C_SendSCCB(LPI2C_Type *base,
                          uint8_t deviceAddress,
                          uint32_t subAddress,
                          uint8_t subAddressSize,
                          uint8_t *txBuff,
                          uint8_t txBuffSize)
{
    lpi2c_master_transfer_t xfer;
	status_t status;

    xSemaphoreTake(cameraI2cHandle.mutex, portMAX_DELAY);


    xfer.flags          = kLPI2C_TransferDefaultFlag;
    xfer.slaveAddress   = deviceAddress;
    xfer.direction      = kLPI2C_Write;
    xfer.subaddress     = subAddress;
    xfer.subaddressSize = subAddressSize;
    xfer.data           = txBuff;
    xfer.dataSize       = txBuffSize;

    //status =  LPI2C_MasterTransferBlocking(CAMERA_I2C_MASTER, &xfer);

    status = LPI2C_MasterTransferNonBlocking(base, &cameraI2cHandle.g_m_handle, &xfer);
    if (status != kStatus_Success)
    {
        xSemaphoreGive(cameraI2cHandle.mutex);
        return status;
    }

    /* Wait for transfer to finish */
    xSemaphoreTake(cameraI2cHandle.semaphore, portMAX_DELAY);

    /* Unlock resource mutex */
    xSemaphoreGive(cameraI2cHandle.mutex);
    if(cameraI2cHandle.status != kStatus_Success)
    {
        printf("dgI2cDriver.c:BOARD_LPI2C_SendSCCB():Transact error\r\n");
    }
    vTaskDelay(1);
    //uint8_t readbackValue;
    //BOARD_LPI2C_ReceiveSCCB(base,deviceAddress, subAddress,subAddressSize, &readbackValue, 1);
    //printf("dgI2cDriver.c:BOARD_LPI2C_SendSCCB():reg=0x%x,written=0x%x, readback=0x%x\r\n", subAddress, txBuff[0],readbackValue);
    /* Return status captured by callback function */
    return cameraI2cHandle.status;

}

status_t BOARD_Camera_I2C_SendSCCB(
    uint8_t deviceAddress, uint32_t subAddress, uint8_t subAddressSize, const uint8_t *txBuff, uint8_t txBuffSize)
{
    return BOARD_LPI2C_SendSCCB(CAMERA_I2C_MASTER, deviceAddress, subAddress, subAddressSize, (uint8_t *)txBuff,
                                txBuffSize);
}

status_t BOARD_Camera_I2C_ReceiveSCCB(
    uint8_t deviceAddress, uint32_t subAddress, uint8_t subAddressSize, uint8_t *rxBuff, uint8_t rxBuffSize)
{
    return BOARD_LPI2C_ReceiveSCCB(CAMERA_I2C_MASTER, deviceAddress, subAddress, subAddressSize, rxBuff,
                                   rxBuffSize);
}



