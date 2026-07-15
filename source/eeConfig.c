/*
 * eeConfig.c
 *
 *  Created on: 12-Nov-2025
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

/* Freescale includes */
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
//#include "fsl_debug_console.h"
#include "fsl_spc.h"
#include "fsl_lpi2c.h"
#include "fsl_lpuart.h"

/* Chewie Includes */
#include "dgCommon.h"
#include "modulecom.h"
#include "version.h"
#include "dgtimer.h"
#include "dgI2cDriver.h"
#include "GPIOSignals.h"
#include "sysConfig.h"
#include "cliProc.h"
#include "rtc.h"
#include "eeConfig.h"



int writeWorkingConfigEEPROM(uint8_t *buffer)
{
	uint8_t numPages, pageCount;
	uint16_t addr;
	uint8_t *bufferPtr;
	int retValue;

	//Validate parameters
	if(buffer == NULL)
	{
		return DG_INVALID_PARAM;
	}
	retValue = DG_SUCCESS;
	bufferPtr = buffer;
	//Calculate the number of pages to be written
	numPages = CHEWIE_CONFIG_WORKING_SIZE/EEPROM_PAGE_SIZE;
	addr = CHEWIE_CONFIG_WORKING_START;
	for(pageCount=0; pageCount<numPages; pageCount++)
	{
		if(eeprom_mem_write_within_page(addr, bufferPtr, 128) != DG_SUCCESS)
		{
			printf("eeConfig.c:writeWorkingConfigEEPROM() fail. Page=%X, bufferPtr=%X\r\n",addr,bufferPtr);
			retValue = DG_FAIL;
			break;
		}
		bufferPtr+=EEPROM_PAGE_SIZE;
		addr+=EEPROM_PAGE_SIZE;
		//This delay is required for the EEPROM programming to work for multiple pages immediately
		vTaskDelay(2);
		//printf("eeConfig.c:writeWorkingConfigEEPROM() success. Page=%X, bufferPtr=%X\r\n",addr,bufferPtr);
	}
	return retValue;
}

int readWorkingConfigEEPROM(uint8_t *buffer)
{
	uint8_t numPages, pageCount;
	uint16_t  addr;
	uint8_t *bufferPtr;
	int retValue;

	//Validate parameters
	if(buffer == NULL)
	{
		return DG_INVALID_PARAM;
	}

	retValue = DG_SUCCESS;
	bufferPtr = buffer;
	//Calculate the number of pages to be written
	numPages = CHEWIE_CONFIG_WORKING_SIZE/EEPROM_PAGE_SIZE;
	addr = CHEWIE_CONFIG_WORKING_START;
	for(pageCount=0; pageCount<numPages; pageCount++)
	{
		if(eeprom_mem_read_within_page(addr, bufferPtr, EEPROM_PAGE_SIZE) != DG_SUCCESS)
		{
			retValue = DG_FAIL;
			break;
		}
		bufferPtr+=EEPROM_PAGE_SIZE;
		addr+=EEPROM_PAGE_SIZE;
	}
	return retValue;
}
