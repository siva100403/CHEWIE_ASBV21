/*
 * actuatorStatusTracker.c
 *
 *  Created on: 13-Mar-2026
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
#include "actuatorStatusTracker.h"


/**************************** Global Variables **************************/

dgActuatorStatus_t 	allActuatorStatus;



/*************************** Implementations ****************************/

void initActuatorStatusTracker(void)
{
	allActuatorStatus.combinedActuatorStatus = 0;
}
void updateLidStatus(uint8_t lidStatus)
{
	allActuatorStatus.bits.lidStatus = lidStatus & 0x02;
}
void updateAugerStatus(uint8_t augerStatus)
{
	allActuatorStatus.bits.augerStatus = augerStatus & 0x02;
}
void updateFlapStatus(uint8_t flapStatus)
{
	allActuatorStatus.bits.flapStatus = flapStatus & 0x02;
}

void updateStValveStatus(uint8_t stValveStatus)
{
	allActuatorStatus.bits.stValveStatus = stValveStatus & 0x02;
}
void updateHtrFanStatus(uint8_t htrFan)
{
	allActuatorStatus.bits.htrFan = htrFan & 0x02;
}

void updateExhFanStatus(uint8_t exhFan)
{
	allActuatorStatus.bits.exhFan = exhFan & 0x01;
}

void updateAdditiveMotorStatus(uint8_t additiveMotorStatus)
{
	allActuatorStatus.bits.additiveMotorStatus = additiveMotorStatus & 0x01;
}
void updateDcSprayerStatus(uint8_t dcSprayer)
{
	allActuatorStatus.bits.dcSprayer = dcSprayer & 0x01;
}
void updateFlushSprayerStatus(uint8_t flushSprayer)
{
	allActuatorStatus.bits.flushSprayer = flushSprayer & 0x01;
}
void updateAirValve1Status(uint8_t airValve1)
{
	allActuatorStatus.bits.airValve1 = airValve1 & 0x01;
}
void updateAirValve2Status(uint8_t airValve2)
{
	allActuatorStatus.bits.airValve2 = airValve2 & 0x01;
}
void updateAirValve3Status(uint8_t airValve3)
{
	allActuatorStatus.bits.airValve3 = airValve3 & 0x01;
}
void updateHeaterStatus(uint8_t heaterStatus)
{
	allActuatorStatus.bits.heaterStatus = heaterStatus & 0x01;
}

uint32_t getActuatorStatus(void)
{
	return allActuatorStatus.combinedActuatorStatus;
}
