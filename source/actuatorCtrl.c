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

/*******************************************************************************
 * Global Variables
 ******************************************************************************/

/***** HBridge Port assignment for Solenoid and Uni-directional DC motor ******/
const dgSolenoidHbAlloc_t airValve1 = {DRV89XX_1, HALFBRIDGE_1};
const dgSolenoidHbAlloc_t airValve2 = {DRV89XX_1, HALFBRIDGE_2};
const dgSolenoidHbAlloc_t additiveDispensor = {DRV89XX_1, HALFBRIDGE_6};
const dgSolenoidHbAlloc_t DCsprayer = {DRV89XX_1, HALFBRIDGE_8};

const dgSolenoidHbAlloc_t flushSprayer = {DRV89XX_2, HALFBRIDGE_1};
const dgSolenoidHbAlloc_t airValve3 = {DRV89XX_2, HALFBRIDGE_2};
const dgSolenoidHbAlloc_t stMotor = {DRV89XX_2, HALFBRIDGE_3};



/************ HBridge Port assignment for bi-directional DC motor ************/
const dgBiDirMotorHbAlloc_t shdAugMotor = {DRV89XX_1, HALFBRIDGE_3, HALFBRIDGE_4};
const dgBiDirMotorHbAlloc_t flapMotor = {DRV89XX_1, HALFBRIDGE_5, HALFBRIDGE_7};
const dgBiDirMotorHbAlloc_t fanMotor = {DRV89XX_2, HALFBRIDGE_5, HALFBRIDGE_7};
const dgBiDirMotorHbAlloc_t augerMotor = {DRV89XX_2, HALFBRIDGE_11, HALFBRIDGE_12};

/*****HBRidge Port Assignment for parallel(2) driving of bi-directional motor ***/
//const dgBiDirMotorPar2HbAlloc_t fanMotor = {DRV89XX_2, HALFBRIDGE_5, HALFBRIDGE_7, HALFBRIDGE_6, HALFBRIDGE_8};
/*******************************************************************************
 * Implementation
 ******************************************************************************/
int additiveDispenseOn()
{
	return halfBridgeCtrl(additiveDispensor.spiDeviceId , additiveDispensor.solHbridgeId, SOLENOID_ON);
}

int additiveDispenseOff()
{
	return halfBridgeCtrl(additiveDispensor.spiDeviceId , additiveDispensor.solHbridgeId, SOLENOID_OFF);
}


int DCsprayerOn()
{
	return halfBridgeCtrl(DCsprayer.spiDeviceId , DCsprayer.solHbridgeId, SOLENOID_ON);
}

int DCsprayerOff()
{
	return halfBridgeCtrl(DCsprayer.spiDeviceId , DCsprayer.solHbridgeId, SOLENOID_OFF);
}

int flushSprayerOn()
{
	return halfBridgeCtrl(flushSprayer.spiDeviceId , flushSprayer.solHbridgeId, SOLENOID_ON);
}

int flushSprayerOff()
{
	return halfBridgeCtrl(flushSprayer.spiDeviceId , flushSprayer.solHbridgeId, SOLENOID_OFF);
}


int stMotorOn()
{
	return halfBridgeCtrl(stMotor.spiDeviceId , stMotor.solHbridgeId, SOLENOID_ON);
}

int stMotorOff()
{
	return halfBridgeCtrl(stMotor.spiDeviceId , stMotor.solHbridgeId, SOLENOID_OFF);
}


int airValve1On()
{
	return halfBridgeCtrl(airValve1.spiDeviceId , airValve1.solHbridgeId, SOLENOID_ON);
}

int airValve1Off()
{
	return halfBridgeCtrl(airValve1.spiDeviceId , airValve1.solHbridgeId, SOLENOID_OFF);
}


int airValve2On()
{
	return halfBridgeCtrl(airValve2.spiDeviceId , airValve2.solHbridgeId, SOLENOID_ON);
}

int airValve2Off()
{
	return halfBridgeCtrl(airValve2.spiDeviceId , airValve2.solHbridgeId, SOLENOID_OFF);
}

int airValve3On()
{
	return halfBridgeCtrl(airValve3.spiDeviceId , airValve3.solHbridgeId, SOLENOID_ON);
}

int airValve3Off()
{
	return halfBridgeCtrl(airValve3.spiDeviceId , airValve3.solHbridgeId, SOLENOID_OFF);
}

int augerMotorCWR()
{
	return fullBridgeCtrl(augerMotor.spiDeviceId, augerMotor.mpHbridgeId, augerMotor.mnHbridgeId, MOTOR_FWD);
}

int augerMotorCCWR()
{
	return fullBridgeCtrl(augerMotor.spiDeviceId, augerMotor.mpHbridgeId, augerMotor.mnHbridgeId, MOTOR_REV);
}

int augerMotorStop()
{
	return fullBridgeCtrl(augerMotor.spiDeviceId, augerMotor.mpHbridgeId, augerMotor.mnHbridgeId, MOTOR_COAST);
}

int shdAugMotorCWR()
{
	return fullBridgeCtrl(shdAugMotor.spiDeviceId, shdAugMotor.mpHbridgeId, shdAugMotor.mnHbridgeId, MOTOR_FWD);
}

int shdAugMotorCCWR()
{
	return fullBridgeCtrl(shdAugMotor.spiDeviceId, shdAugMotor.mpHbridgeId, shdAugMotor.mnHbridgeId, MOTOR_REV);
}



int shdAugMotorStop()
{
	return fullBridgeCtrl(shdAugMotor.spiDeviceId, shdAugMotor.mpHbridgeId, shdAugMotor.mnHbridgeId, MOTOR_COAST);
}

int shdAugMotorSetspeed(uint8_t speed)
{
	return setDutyCycleChl1_DRV89XX_1(speed);
}


int flapMotorCWR()
{
	return fullBridgeCtrl(flapMotor.spiDeviceId, flapMotor.mpHbridgeId, flapMotor.mnHbridgeId, MOTOR_FWD);
}

int flapMotorCCWR()
{
	return fullBridgeCtrl(flapMotor.spiDeviceId, flapMotor.mpHbridgeId, flapMotor.mnHbridgeId, MOTOR_REV);
}

int flapMotorStop()
{
	return fullBridgeCtrl(flapMotor.spiDeviceId, flapMotor.mpHbridgeId, flapMotor.mnHbridgeId, MOTOR_COAST);
}

int fanMotorCWR()
{
	return fullBridgeCtrl(fanMotor.spiDeviceId, fanMotor.mpHbridgeId, fanMotor.mnHbridgeId, MOTOR_FWD);
	//return fullBridgePar2Ctrl(fanMotor.spiDeviceId, fanMotor.mp1HbridgeId,  fanMotor.mp2HbridgeId, fanMotor.mn1HbridgeId, fanMotor.mn2HbridgeId, MOTOR_FWD);
}

int fanMotorCCWR()
{
	return fullBridgeCtrl(fanMotor.spiDeviceId, fanMotor.mpHbridgeId, fanMotor.mnHbridgeId, MOTOR_REV);
	//return fullBridgePar2Ctrl(fanMotor.spiDeviceId, fanMotor.mp1HbridgeId,  fanMotor.mp2HbridgeId, fanMotor.mn1HbridgeId, fanMotor.mn2HbridgeId, MOTOR_REV);
}

int fanMotorStop()
{
	return fullBridgeCtrl(fanMotor.spiDeviceId, fanMotor.mpHbridgeId, fanMotor.mnHbridgeId, MOTOR_COAST);
	//return fullBridgePar2Ctrl(fanMotor.spiDeviceId, fanMotor.mp1HbridgeId,  fanMotor.mp2HbridgeId, fanMotor.mn1HbridgeId, fanMotor.mn2HbridgeId, MOTOR_COAST);
}

