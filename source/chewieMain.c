/*
 * Copyright 2016-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file    CHEWIE_ASBV21.c
 * @brief   Application entry point.
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
#include "motorControl.h"
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
#include "csmMod.h"
#include "csmModAPI.h"
#include "measure.h"




/* TODO: insert other definitions and declarations here. */
int initPrintMod(void);

/*
 * @brief   Application entry point.
 */
int main(void) {

	/*Configure LDO */
	SPC_EnableDCDCRegulator(SPC0, false);

    /* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();

    /* Init FSL debug console. */
    //BOARD_InitDebugConsole();

    //Initialize Module store
    initModuleStore();
    if(PSRAM_Init()==kStatus_Success)
    {
    	printf("chewieMain.c:initSysStart(): PSRAM Init success\r\n");
    }
    else
    {
    	printf("chewieMain.c:initSysStart(): PSRAM Init Fail\r\n");
    }


    if(initSysStart()==DG_SUCCESS)
    {
    	printf("chewieMain.c:initSysStart(): Success\r\n");
    }
    else
    {
    	printf("chewieMain.c:initSysStart(): Fail\r\n");
    }

    if(initPrintMod() != DG_SUCCESS)
    {
        printf("chewieMain.c:InitPrintMod(): failed\r\n");
    }
    else
    {
        printf("chewieMain.c:InitPrintMod(): success\r\n");

    }

    vTaskStartScheduler();
    //PRINTF("After Task scheduler started!.\r\n");
    for (;;)
    {
        //PRINTF("Inside main thread!.\r\n");
        vTaskDelay( 1000 );

    }

    /* Force the counter to be placed into memory. */
    volatile static int i = 0 ;
    /* Enter an infinite loop, just incrementing a counter. */
    while(1) {
        i++ ;
        /* 'Dummy' NOP to allow source level single stepping of
            tight while() loop */
        __asm volatile ("nop");
    }
    return 0 ;
}


static void print_task(void *pvParameters)
{


/*    GREEN_LED_ON();
    RED_LED_ON();
    BLUE_LED_ON();*/


    vTaskDelay( 1000 ); //For other tasks to get started


    //initLimitSwitchModule();
	//printf("chewieMain.c:():print_task():Limit switch initialized\r\n");


	uint8_t lidStatusLocal;
	float temp, hum;
	uint8_t buffer[128], outbuffer[128];
	int i,countr=0, countw=0;

	printf("Chewiemain.c:size of shredder Control param=%d\r\n", sizeof(dgShdConfigParams_t));

    while(1)
    {
/*
    	for(i=0; i<128; i++)
    	{
    		buffer[i]=countw++;
    	}

    	if(eeprom_mem_write_within_page(0x1000, buffer, 10)==DG_SUCCESS)
    	{
    		printf("EEPROM write success\r\n");
       	}
    	else
    	{
    		printf("EEPROM write failed\r\n");
    	}

    	if(eeprom_mem_read_within_page(0x1000, outbuffer, 10) == DG_SUCCESS)
    	{
    		printf("EEPROM read success\r\n");
    	}
    	else
    	{
    		printf("EEPROM read failed\r\n");
    	}

    	for(i=0;i<10; i++)
    	{
    		if(buffer[i]==outbuffer[i]) continue;

    		printf("EEPROM compare failed at i=%d\r\n",i);
    	}
*/

    	//storeWorkingConfigEEPROM()
    	//getLidSwicthStatus();
    	vTaskDelay(200);
    	//getLidSwicthStatus();

    	//printf("ChewieMain.c:lidSwitchStatus=%d, LSCLOSE=%d, LSOPEN=%d\r\n",getLidSwicthStatus(), READ_LS_SENSE_LIDCLOSE(), READ_LS_SENSE_LIDOPEN());


    }


}

int initPrintMod(void)
{
	//Create print_task
	TaskHandle_t printTaskHandle;

    if (xTaskCreate(print_task, "print_task", configMINIMAL_STACK_SIZE + 100, NULL, task_PRIORITY, &printTaskHandle) !=
        pdPASS)
    {
        printf("initPrintMod():Print Task creation failed!.\r\n");
        return DG_FAIL;
    }

    return DG_SUCCESS;
}

