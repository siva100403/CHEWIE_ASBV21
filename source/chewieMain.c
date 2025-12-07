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

/* Chewie Includes */
#include "dgCommon.h"
#include "version.h"
#include "modulecom.h"
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
#include "augerAPI.h"
#include "shredder.h"
#include "shredderAPI.h"
#include "adcs.h"
#include "limitSwitchMod.h"
#include "transferCS.h"


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


    GREEN_LED_ON();
    RED_LED_ON();
    BLUE_LED_ON();


    vTaskDelay( 1000 ); //For other tasks to get started




    initLimitSwitchModule();
	printf("chewieMain.c:():print_task():Limit switch initialized\r\n");

	uint8_t lidStatusLocal;

    while(1)
    {

/*    	LS_DRIVE_HIGH();
    	HEATER_OFF();
    	SHD_STOP();
    	SHD_DIR_CW();
    	LID_MOTOR_DIR_CLOSE();
    	SPARE1_RELAY_OFF();
    	HMI_INT_INACTIVE();
    	LID_POWER_OFF();
    	SPARE2_RELAY_OFF();*/

    	lidStatusLocal = getLidSwicthStatus();
    	printf("chewieMain.c:():print_task():Lidstatus=%d\r\n",lidStatusLocal);


        vTaskDelay( 200 );

/*    	LS_DRIVE_LOW();
    	HEATER_ON();
    	SHD_START();
    	SHD_DIR_CCW();
    	LID_MOTOR_DIR_OPEN();
    	SPARE1_RELAY_ON();
    	HMI_INT_ACTIVE();
    	LID_POWER_ON();
    	SPARE2_RELAY_ON();*/

        //vTaskDelay( 500);

    	//printf("chewieMain.c:(): inside print_task\r\n");
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

