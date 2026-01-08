/*
 * limitSwitchMod.c
 *
 *  Created on: 16-May-2025
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



/******************************** Limit Switch Sense Module ***********************
 * Four Limit switch sensors
 *   - LS_SENSE_LIDCLOSE: Monitors the LID Close status
 *        - HIGH - when Lid reached CLOSE position
 *        - LOW - when Lid has not reached CLOSE position
 *  - LS_SENSE_LIDOPEN: Monitors the LID open status
 *        - HIGH - when Lid reached OPEN position
 *        - LOW  - when Lid has not reached OPEN positiom
 *  - LS_SENSE_FLAP: Indicates FLAP is in CLOSE(REST) position.
 *  				There is only one limit switch for FLAP. This requires LS_DRIVE to be driven
 *        - HIGH - when FLAP reached the CLOSE(REST)
 *        - LOW  - when FLAP has not reached CLOSE position
 *  - LS_SENSE_STTV: Indicates Storage Tray Transfer Valve is CLOSE position. This requires LS_DRIVE to be driven
 *        - HIGH - when STTV reached the CLOSE(REST) position
 *        - LOW  - when STTV has not reached CLOSE position
 *
 *
 ***********************************************************************************/

//Sensing method for LS_SENSE_STTV and LS_SENSE_FLAP
// Limit switch wiring: One terminal connected to Output of RS232 driver and another terminal connected to the Input of RS232 Driver
// When an RS232 input is open it's corresponding logic level output is HIGH.
// RS232 Output is driven LOW normally. When the Limit switch is OPEN, the logic level output will read HIGH and when the limit switch is CLOSED the logic level output will read LOW
// De-bouncing is done by checking successive reading at 2 mSec interval give the same logic level.

/************************Global Variables*******************************************/

uint8_t lidCloseStatus;
uint8_t lidOpenStatus;
uint8_t lidStatus;
uint8_t sttvStatus;
uint8_t flapStatus;
uint8_t lidCloseDbCount;
uint8_t lidOpenDbCount;
uint8_t sttvDbCount;
uint8_t flapDbCount;
uint8_t lidSensingEnable;


#define TWO_MS_TIMER_PERIOD		(2000-1)     // uSec-1

/******************************** Modules ******************************************/


static void callback2mSec(void)
{
	static uint8_t starting = 1;
	uint8_t lidStatusChangeFlag;


	lidStatusChangeFlag = 0;  //Default - No change in lid status

	RED_LED_TOGGLE();

	if(lidSensingEnable == LID_SENSING_ENA)
	{
	    //Read LIDCLOSE sensor and debounce
	    if(READ_LS_SENSE_LIDCLOSE() == LIMITSWITCH_CLOSE)
	    {
	    	lidCloseDbCount++;
	    	lidCloseDbCount = (lidCloseDbCount >DEBOUNCE_COUNT)? DEBOUNCE_COUNT: lidCloseDbCount;
	    	//Check whether there is a change in switch status
	    	if((lidCloseDbCount == DEBOUNCE_COUNT) && (lidCloseStatus != LID_CLOSED))
	    	{
	    		// Change in Limit switch status and hence update the status variable.
	    		lidCloseStatus = LID_CLOSED;
	    		lidStatusChangeFlag = 1; //Lid status changed
	    	}
	    }
	    else
	    {
	    	lidCloseDbCount--;
	    	lidCloseDbCount = (lidCloseDbCount <0)? 0: lidCloseDbCount;
	    	//Check whether there is a change in switch status
	    	if((lidCloseDbCount == 0) && (lidCloseStatus != LID_NOTCLOSED))
	    	{
	    		// Change in Limit switch status and hence update the status variable.
	    		lidCloseStatus = LID_NOTCLOSED;
	    		lidStatusChangeFlag = 1; //Lid status changed
	    	}
	    }

	    //Read LIDOPEN sensor and debounce
	    if(READ_LS_SENSE_LIDOPEN() == LIMITSWITCH_CLOSE)
	    {
	    	lidOpenDbCount++;
	    	lidOpenDbCount = (lidOpenDbCount >DEBOUNCE_COUNT)? DEBOUNCE_COUNT: lidOpenDbCount;
	    	//Check whether there is a change in switch status
	    	if((lidOpenDbCount == DEBOUNCE_COUNT) && (lidOpenStatus != LID_OPEN))
	    	{
	    		// Change in Limit switch status and hence update the status variable.
	    		lidOpenStatus = LID_OPEN;
	    		lidStatusChangeFlag = 1; //Lid status changed
	    	}
	    }
	    else
	    {
	    	lidOpenDbCount--;
	    	lidOpenDbCount = (lidOpenDbCount <0)? 0: lidOpenDbCount;
	    	//Check whether there is a change in switch status
	    	if((lidOpenDbCount == 0) && (lidOpenStatus != LID_NOTOPEN))
	    	{
	    		// Change in Limit switch status and hence update the status variable.
	    		lidOpenStatus = LID_NOTOPEN;
	    		lidStatusChangeFlag = 1; //Lid status changed
	    	}
	    }
	    //Check whether there is a change in lid status and send event to Lid module and shredder Module
	    if((lidStatusChangeFlag == 1)||(starting == 1))
	    {
	    	uint8_t combinedStatus;


	    	//Combine lidOpenStatus and lidCloseStatus to lidStatus
	    	combinedStatus = ((lidCloseStatus<<1) & 0x02)+(lidOpenStatus & 0x01);
	    	switch(combinedStatus)
	    	{
	    	case 0x00:
	    		//Both limit switch can not be in open condition.
	    		//printf("limitSwitchMod.c:callback2mSec():Both limit switch in OPEN condition\r\n");
	    		lidStatus = LID_STATUS_ERROR;
	    	case 0x01:
	    		lidStatus = LID_STATUS_OPEN;
	    		//send event to shredder module
	        	//event_lid_open(LIMITSWITCH_MOD);
	    		break;
	    	case 0x02:
	    		lidStatus = LID_STATUS_CLOSED;
	    		//send Lid_close event to shredder module
	    		event_lid_close(LIMITSWITCH_MOD);
	    		break;
	    	case 0x03:
	    		lidStatus = LID_STATUS_INBETWEEN;
	    		//send Lid_open event to shredder module
	        	event_lid_open(LIMITSWITCH_MOD);
	    		break;
	    	default:
	    		break;
	    	}
	    	//send event to lidModule
	    	if(starting == 0)
	    	{
		        sendLidSwitchEvent(lidStatus);
	    	}


	    	starting = 0;
	    }
	}


    //Read FLAP sensor and debounce
    if(READ_LS_SENSE_FLAP() == LS232DRVR_CLOSE)
    {
    	flapDbCount++;
    	flapDbCount = (flapDbCount >DEBOUNCE_COUNT)? DEBOUNCE_COUNT: flapDbCount;
    	//Check whether there is a change in switch status
    	if((flapDbCount == DEBOUNCE_COUNT) && (flapStatus != FLAP_POSITION_CLOSED))
    	{
    		// Change in Limit switch status and hence update the status variable.
    		flapStatus = FLAP_POSITION_CLOSED;
    		//send event to transferModule
    		event_ls_flapclose(UNKNOWN);
    	}
    }
    else
    {
    	flapDbCount--;
    	flapDbCount = (flapDbCount <0)? 0: flapDbCount;
    	//Check whether there is a change in switch status
    	if((flapDbCount == 0) && (flapStatus != FLAP_POSITION_NOTCLOSED))
    	{
    		// Change in Limit switch status and hence update the status variable.
    		flapStatus = FLAP_POSITION_NOTCLOSED;
    		//send event to transferModule
    		///*TODO*/
    	}
    }

    //Read ST sensor and debounce
    if(READ_LS_SENSE_ST() == LS232DRVR_CLOSE)
    {
    	sttvDbCount++;
    	sttvDbCount = (sttvDbCount >DEBOUNCE_COUNT)? DEBOUNCE_COUNT: sttvDbCount;
    	//Check whether there is a change in switch status
    	if((sttvDbCount == DEBOUNCE_COUNT) && (sttvStatus != STTV_POSITION_CLOSED))
    	{
    		// Change in Limit switch status and hence update the status variable.
    		sttvStatus = STTV_POSITION_CLOSED;
    		//send event to transferModule
    		event_ls_stvalveClose(UNKNOWN);
    	}
    }
    else
    {
    	sttvDbCount--;
    	sttvDbCount = (sttvDbCount <0)? 0: sttvDbCount;
    	//Check whether there is a change in switch status
    	if((sttvDbCount == 0) && (sttvStatus != STTV_POSITION_NOTCLOSED))
    	{
    		// Change in Limit switch status and hence update the status variable.
    		sttvStatus = STTV_POSITION_NOTCLOSED;
    		//send event to transferModule
    		///*TODO*/
    	}
    }
}


void start2mSecTimer(void)
{
    UTICK_Init(UTICK0);
    UTICK_SetTick(UTICK0, kUTICK_Repeat, TWO_MS_TIMER_PERIOD, callback2mSec);
}

void stop2mSecTimer(void)
{

    UTICK_Deinit(UTICK0);
}

void enableLidStatusSensing(void)
{
	lidSensingEnable = LID_SENSING_ENA;
}

void disableLidStatusSensing(void)
{
	lidSensingEnable = LID_SENSING_DIS;
}


uint8_t getLidSwicthStatus(void)
{
	printf("lsMod.c:lidstatus=0x%X, lidCloseSt=0x%X, LidOpenSt=0x%X\r\n",lidStatus,lidCloseStatus,lidOpenStatus);
	return lidStatus;
}

uint8_t getStorageTraySwicthStatus(void)
{
	return sttvStatus;
}



int initLimitSwitchModule(void)
{
	//Initialize the variables used
	lidCloseStatus = READ_LS_SENSE_LIDCLOSE();
	lidOpenStatus = READ_LS_SENSE_LIDOPEN();
	sttvStatus = READ_LS_SENSE_ST();
	flapStatus = READ_LS_SENSE_FLAP();
	lidStatus = LID_STATUS_UNKNOWN;
	lidSensingEnable = LID_SENSING_DIS;

	lidCloseDbCount = 0;
	lidOpenDbCount = 0;
	sttvDbCount = 0;
	flapDbCount = 0;

    //Initialize the Limit switch driving Pin
	LS_DRIVE_HIGH();

    /* Enable FRO 1MHz clock for UTICK */
    SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_FRO1MHZ_ENA_MASK;
    //Start the timer
	start2mSecTimer();
    return DG_SUCCESS;
}
