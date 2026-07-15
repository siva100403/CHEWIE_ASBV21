/*
 * sht40Driver.c
 *
 *  Created on: 25-Aug-2025
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



int readShtSerialNumber(uint32_t *serialNum)
{
	uint8_t regValue[6];
	//Send command to read serial number
	if(sht4x_setAddress(CMD_READ_SERIAL_NO) != DG_SUCCESS)
	{
		return DG_FAIL;
	}

	if(sht4x_reg_read(CMD_READ_SERIAL_NO, &regValue[0]) != DG_SUCCESS)
	{
		return DG_FAIL;
	}
	*serialNum = regValue[5]*256*256*256 + regValue[4]*256*256 + regValue[2]*256 + regValue[1];

	return DG_SUCCESS;
}

int readShtTempHumidityHighPrecision(float *temperature, float *humidity)
{
	uint8_t regValue[6];
	uint16_t tempCount, humidityCount;

	//Send command to start the measurement
	if(sht4x_setAddress(CMD_MEASURE_TRH_HP) != DG_SUCCESS)
	{
		return DG_FAIL;
	}
	//10 mSec timeout for the measurement
	 vTaskDelay( 3 );

	if(sht4x_reg_read(CMD_MEASURE_TRH_HP, &regValue[0]) != DG_SUCCESS)
	{
		return DG_FAIL;
	}
	//Measurement is success. Convert the data to temperature and humidity
	tempCount = regValue[0]*256 + regValue[1];
	humidityCount = regValue[3]*256 + regValue[4];

	//Checksum to be verified
	/*TODO*/

	*temperature = -45.0 + 175.0 * ((float)tempCount)/65535.0;
	//*temperature = (float)(-45 + (175 * tempCount)/65535);
	*humidity = -6.0 + 125.0 * ((float)humidityCount)/65535.0;
	//*humidity = (float)(-6 + (125 * humidityCount)/65535);
	if(*humidity > 100.0)
	{
		*humidity = 100.0;
	}
	if(*humidity <0)
	{
		*humidity = 0.0;
	}
	return DG_SUCCESS;
}

int initSht40(void)
{
	uint32_t serialNum;
	//float temperature, humidity;

    TCA9803_ENA();

    vTaskDelay( 1);  //SHT40 requires 1uSec to enable

    //if(readShtTempHumidityHighPrecision(&temperature, &humidity)==DG_SUCCESS)
    //Read SHT serial number
    if(readShtSerialNumber(&serialNum) == DG_SUCCESS)
    {
    	printf("sht40Driver.c:initSht40():Device serial number:0x%X\r\n", serialNum);
    	return DG_SUCCESS;
    }
    else
    {
    	printf("sht40Driver.c:initSht40():init failed\r\n");
    	return DG_FAIL;
    }
}
