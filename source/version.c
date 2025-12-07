/*
 * version.c
 *
 *  Created on: 08-Sep-2024
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

/* Chewie Includes */
#include "dgCommon.h"
#include "modulecom.h"
#include "dgtimer.h"


/*******************Software version Number*************/

char swVer[12] ;
char productModel[32];
char asbHwVersion[16];

void initVersion(void)
{
strcpy(swVer,"0.3.2" );
strcpy(productModel,"CHEWIE");
strcpy(asbHwVersion, "V 2.1");  //TODO: Read from EEPROM
}

void getFwVersion(char * version)
{
	strcpy(version, swVer);
}

void getHwVersion(char * version)
{
	strcpy(version, asbHwVersion);
}


/*******************Software version History*************/

/************************Ver 0.3.0 30-11-2025********************
 * New Features
 * 1. First port from ASB V2.0 to ASB V2.1 Hardware
 *
 *******************************************************************/

/************************Ver 0.3.1 01-12-2025********************
 * New Features
 * 1. CLI Module integrated
 * 2. SysConfig and sysStart modules integrated
 * Known Issues
 * 1. Floating point does not work in printf(). Need to modify the project configuration
 *
 *******************************************************************/

/************************Ver 0.3.2 03-12-2025********************
 * New Features
 * 1. DRV89xx driver integrated.
 * 2. CLI added for controlling the DRV89xx connected devices
 *
 *******************************************************************/

/************************Ver 0.3.3 07-12-2025********************
 * New Features
 * 1. Limit Switch integrated (To be tested)
 * 2. TCS, ADCS integrated (To be tested)
 *
 *******************************************************************/

