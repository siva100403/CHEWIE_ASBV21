/*
 * config.c
 *
 *  Created on: 27-Dec-2024
 *      Author: rahaw
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
#include "alarm.h"


int updateChewieStateStore(dgCtStateVar_t *ctVar)
{
	/*TODO: Error check for all write*/
	rtcRAMWrite(CSM_SCHDUR_LB_ADDR, (uint8_t)((ctVar->schDur)&0xFF));
	rtcRAMWrite(CSM_SCHDUR_HB_ADDR, (uint8_t)(((ctVar->schDur)>>8)&0xFF));
	rtcRAMWrite(CSM_EXPDUR_LB_ADDR, (uint8_t)((ctVar->expDur)&0xFF));
	rtcRAMWrite(CSM_EXPDUR_HB_ADDR, (uint8_t)(((ctVar->expDur)>>8)&0xFF));
	rtcRAMWrite(CSM_REMDUR_LB_ADDR, (uint8_t)((ctVar->remDur)&0xFF));
	rtcRAMWrite(CSM_REMDUR_HB_ADDR, (uint8_t)(((ctVar->remDur)>>8)&0xFF));

	rtcRAMWrite(CSM_CURPHASE_ADDR, ctVar->curPhase);
	rtcRAMWrite(CSM_PREVPHASE_ADDR, ctVar->prevPhase);
	rtcRAMWrite(CSM_WASTECAT_ADDR, ctVar->curWasteCat);
	rtcRAMWrite(CSM_STATE_ADDR, ctVar->state);

	/*TODO: Current date has to be read and written to RTC RAM*/

	return DG_SUCCESS;
}
int getChewieStateStore(dgDateTime_t *updateTime,dgCtStateVar_t *ctVar)
{
	/*TODO: Time needs to filled*/

	uint8_t lowByte, highByte;

	//Read CSM variables
	rtcRAMRead(CSM_SCHDUR_LB_ADDR, &lowByte);
	rtcRAMRead(CSM_SCHDUR_HB_ADDR, &highByte);
	ctVar->schDur = (highByte <<8) + lowByte;

	rtcRAMRead(CSM_EXPDUR_LB_ADDR, &lowByte);
	rtcRAMRead(CSM_EXPDUR_HB_ADDR, &highByte);
	ctVar->expDur = (highByte <<8) + lowByte;

	rtcRAMRead(CSM_REMDUR_LB_ADDR, &lowByte);
	rtcRAMRead(CSM_REMDUR_HB_ADDR, &highByte);
	ctVar->remDur = (highByte <<8) + lowByte;

	rtcRAMRead(CSM_CURPHASE_ADDR, &ctVar->curPhase);
	rtcRAMRead(CSM_PREVPHASE_ADDR, &ctVar->prevPhase);
	rtcRAMRead(CSM_WASTECAT_ADDR, &ctVar->curWasteCat);
	rtcRAMRead(CSM_STATE_ADDR, &ctVar->state);

	return DG_SUCCESS;
}
int updateChewieStateStoreExpDur(uint16_t expDur, uint16_t remDur)
{
	rtcRAMWrite(CSM_EXPDUR_LB_ADDR, (uint8_t)(expDur&0xFF));
	rtcRAMWrite(CSM_EXPDUR_HB_ADDR, (uint8_t)((expDur>>8)&0xFF));
	rtcRAMWrite(CSM_REMDUR_LB_ADDR, (uint8_t)(remDur&0xFF));
	rtcRAMWrite(CSM_REMDUR_HB_ADDR, (uint8_t)((remDur>>8)&0xFF));
	return DG_SUCCESS;
}
int updateCounterCtmotor(uint16_t ctmotor)
{
	return DG_SUCCESS;
}
int updateCounterShmotor(uint16_t shmotor)
{
	return DG_SUCCESS;
}
int updateCounterLid(uint16_t lidCycles)
{
	return DG_SUCCESS;
}

int updateAdcsState(uint8_t state)
{
	rtcRAMWrite(ADCS_STATE_ADDR, state);
	return DG_SUCCESS;
}

int getAdcsState(uint8_t *state)
{
	rtcRAMRead(ADCS_STATE_ADDR, state);
	return DG_SUCCESS;
}


