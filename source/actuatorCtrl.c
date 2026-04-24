/*
 * actuatorCtrl.c
 *
 *  Created on: 24-Aug-2025
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
#include "dgtimer.h"
#include "GPIOSignals.h"
#include "sysStart.h"
#include "sysConfig.h"
#include "mclsSPIDriver.h"
#include "drv89xxDriver.h"
#include "drv89xxRegisters.h"
#include "mclsSPIDriver.h"
#include "drv89xxDriver.h"
#include "drv89xxRegisters.h"
#include "actuatorCtrl.h"
#include "actuatorStatusTracker.h"

/*******************************************************************************
 * Global Variables
 ******************************************************************************/

/*---- HBridge Port assignment for Solenoid and Uni-directional DC motor -----*/
const dgSolenoidHbAlloc_t airValve1 = {DRV89XX_1, HALFBRIDGE_4};
const dgSolenoidHbAlloc_t airValve2 = {DRV89XX_1, HALFBRIDGE_2};
const dgSolenoidHbAlloc_t additiveDispensor = {DRV89XX_1, HALFBRIDGE_3};
const dgSolenoidHbAlloc_t DCsprayer = {DRV89XX_1, HALFBRIDGE_8};

const dgSolenoidHbAlloc_t flushSprayer = {DRV89XX_2, HALFBRIDGE_8};
const dgSolenoidHbAlloc_t airValve3 = {DRV89XX_2, HALFBRIDGE_2};
//const dgSolenoidHbAlloc_t stMotor = {DRV89XX_2, HALFBRIDGE_10};




/************ HBridge Port assignment for bi-directional DC motor ************/
const dgBiDirMotorHbAlloc_t stMotor = {DRV89XX_2, HALFBRIDGE_3, HALFBRIDGE_4};
const dgBiDirMotorHbAlloc_t mkValve = {DRV89XX_2, HALFBRIDGE_12, HALFBRIDGE_11};

const dgBiDirMotorHbAlloc_t flapMotor = {DRV89XX_1, HALFBRIDGE_5, HALFBRIDGE_7};
const dgBiDirMotorHbAlloc_t fanMotor = {DRV89XX_2, HALFBRIDGE_5, HALFBRIDGE_7};
//dgBiDirMotorHbAlloc_t stMotor = {DRV89XX_2, HALFBRIDGE_11, HALFBRIDGE_12};



/*****HBRidge Port Assignment for parallel(2) driving of bi-directional motor ***/
//const dgBiDirMotorPar2HbAlloc_t fanMotor = {DRV89XX_2, HALFBRIDGE_5, HALFBRIDGE_7, HALFBRIDGE_6, HALFBRIDGE_8};


/*******************************************************************************
 * Implementation
 ******************************************************************************/

int additiveDispenseOn()
{
	updateAdditiveMotorStatus(ON);
	return halfBridgeCtrl(additiveDispensor.spiDeviceId , additiveDispensor.solHbridgeId, SOLENOID_ON);
}

int additiveDispenseOff()
{
	updateAdditiveMotorStatus(OFF);
	return halfBridgeCtrl(additiveDispensor.spiDeviceId , additiveDispensor.solHbridgeId, SOLENOID_OFF);
}


int DCsprayerOn()
{
	updateDcSprayerStatus(ON);
	return halfBridgeCtrl(DCsprayer.spiDeviceId , DCsprayer.solHbridgeId, SOLENOID_ON);
}

int DCsprayerOff()
{
	//updateDcSprayerStatus(OFF);
	return halfBridgeCtrl(DCsprayer.spiDeviceId , DCsprayer.solHbridgeId, SOLENOID_OFF);
}

int flushSprayerOn()
{
	updateFlushSprayerStatus(ON);
	return halfBridgeCtrl(flushSprayer.spiDeviceId , flushSprayer.solHbridgeId, SOLENOID_ON);
}

int flushSprayerOff()
{
	updateFlushSprayerStatus(OFF);
	return halfBridgeCtrl(flushSprayer.spiDeviceId , flushSprayer.solHbridgeId, SOLENOID_OFF);
}

int airValve1On()
{
	updateAirValve1Status(ON);
	return halfBridgeCtrl(airValve1.spiDeviceId , airValve1.solHbridgeId, SOLENOID_ON);
}

int airValve1Off()
{
	updateAirValve1Status(OFF);
	return halfBridgeCtrl(airValve1.spiDeviceId , airValve1.solHbridgeId, SOLENOID_OFF);
}


int airValve2On()
{
	updateAirValve2Status(ON);
	return halfBridgeCtrl(airValve2.spiDeviceId , airValve2.solHbridgeId, SOLENOID_ON);
}



int airValve2Off()
{
	updateAirValve2Status(OFF);
	return halfBridgeCtrl(airValve2.spiDeviceId , airValve2.solHbridgeId, SOLENOID_OFF);
}

int airValve3On()
{
	updateAirValve3Status(ON);
	return halfBridgeCtrl(airValve3.spiDeviceId , airValve3.solHbridgeId, SOLENOID_ON);
}

int airValve3Off()
{
	updateAirValve3Status(OFF);
	return halfBridgeCtrl(airValve3.spiDeviceId , airValve3.solHbridgeId, SOLENOID_OFF);
}

int fanMotorCWR()
{
	//updateHtrFanStatus(CWR);
	return fullBridgeCtrl(fanMotor.spiDeviceId, fanMotor.mpHbridgeId, fanMotor.mnHbridgeId, MOTOR_FWD);
}

int fanMotorCCWR()
{
	//updateHtrFanStatus(CCWR);
	return fullBridgeCtrl(fanMotor.spiDeviceId, fanMotor.mpHbridgeId, fanMotor.mnHbridgeId, MOTOR_REV);
}

int fanMotorStop()
{
	//updateHtrFanStatus(OFF);
	return fullBridgeCtrl(fanMotor.spiDeviceId, fanMotor.mpHbridgeId, fanMotor.mnHbridgeId, MOTOR_COAST);
}

int stMotorCWR()
{
	updateStValveStatus(CWR);
	return fullBridgeCtrl(stMotor.spiDeviceId, stMotor.mpHbridgeId, stMotor.mnHbridgeId, MOTOR_FWD);
}

int stMotorCCWR()
{
	updateStValveStatus(CCWR);
	return fullBridgeCtrl(stMotor.spiDeviceId, stMotor.mpHbridgeId, stMotor.mnHbridgeId, MOTOR_REV);
}

int stMotorStop()
{
	updateStValveStatus(OFF);
	return fullBridgeCtrl(stMotor.spiDeviceId, stMotor.mpHbridgeId, stMotor.mnHbridgeId, MOTOR_COAST);
}




int flapMotorCWR()
{
	printf("FlapMotor CWR\r\n");
	updateFlapStatus(CWR);
	return fullBridgeCtrl(flapMotor.spiDeviceId, flapMotor.mpHbridgeId, flapMotor.mnHbridgeId, MOTOR_FWD);
}

int flapMotorCCWR()
{
	printf("FlapMotor CCWR\r\n");
	updateFlapStatus(CCWR);
	return fullBridgeCtrl(flapMotor.spiDeviceId, flapMotor.mpHbridgeId, flapMotor.mnHbridgeId, MOTOR_REV);
}

int flapMotorStop()
{
	printf("FlapMotor OFF\r\n");
	updateFlapStatus(OFF);
	return fullBridgeCtrl(flapMotor.spiDeviceId, flapMotor.mpHbridgeId, flapMotor.mnHbridgeId, MOTOR_COAST);
}



//MK Valve Control
int mkValveCWR()
{
	printf("mkValve CWR\r\n");
	return fullBridgeCtrl(mkValve.spiDeviceId, mkValve.mpHbridgeId, mkValve.mnHbridgeId, MOTOR_FWD);
}

int mkValveCCWR()
{
	printf("mkValve CCWR\r\n");
	return fullBridgeCtrl(mkValve.spiDeviceId, mkValve.mpHbridgeId, mkValve.mnHbridgeId, MOTOR_REV);
}

int mkValveStop()
{
	printf("mkValve OFF\r\n");
	return fullBridgeCtrl(mkValve.spiDeviceId, mkValve.mpHbridgeId, mkValve.mnHbridgeId, MOTOR_COAST);
}
