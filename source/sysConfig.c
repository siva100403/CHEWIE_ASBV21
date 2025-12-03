/*
 * sysConfig.c
 *
 *  Created on: 22-Aug-2025
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


/*******************************************************************************
 * Global Variables
 ******************************************************************************/

dgConfigMem_t allConfig;

#ifdef SYSCONFIG_FILE

#include csConfig.c

#else   //Default Configuration for Chewie Control System



const dgCtWastecatProcessParam_t processTable = {{{35.0, 57.0, 0, 240, 19}, 	//M-Phase, Cat0
											{60.0, 53.0, 0, 240, 90},	//T-Phase, Cat0
											{40.0, 48.0, 0, 30, 50},		//P-Phase, Cat0
											{0,0,0,0,0},					//Dummy
											},
										   {{35.0, 60.0, 0, 210, 18},	//M-Phase, Cat1
											{58.0, 55.0, 0, 840, 11},	//T-Phase, Cat1
											{40.0, 50.0, 0, 30, 17},		//P-Phase, Cat1
											{0,0,0,0,0},					//Dummy
										   },
										   {{35.0, 60.0, 0, 240, 18},	//M-Phase, Cat2
											{55.0, 55.0, 0, 600, 12},	//T-Phase, Cat2
											{38.0, 50.0, 0, 240, 18},		//P-Phase, Cat2
											{0,0,0,0,0},					//Dummy
										   },
										   {{35.0, 58.0, 0, 180, 18},	//M-Phase, Cat3
											{58.0, 53.0, 0, 840, 10},	//T-Phase, Cat3
											{40.0, 48.0, 0, 300, 15},		//P-Phase, Cat3
											{0,0,0,0,0},					//Dummy
										   },
										   {{35.0, 55.0, 0, 240, 20},	//M-Phase, Cat4
											{63.0, 50.0, 0, 1140, 14},	//T-Phase, Cat4
											{45.0, 45.0, 0, 420, 18},		//P-Phase, Cat4
											{0,0,0,0,0},					//Dummy
										   },
										   {{35.0, 55.0, 0, 180, 20},	//M-Phase, Cat5
										   {60.0, 50.0, 0, 1020, 12},	//T-Phase, Cat5
										   {40.0, 48.0, 0, 360, 18},		//P-Phase, Cat5
										   {0,0,0,0,0},					//Dummy
										   	},
											{{35.0, 60.0, 0, 180, 18},	//M-Phase, Cat6
											{57.0, 55.0, 0, 720, 10},	//T-Phase, Cat6
											{40.0, 50.0, 0, 30, 15},		//P-Phase, Cat6
											 {0,0,0,0,0},					//Dummy
											},
											{{30.0, 50.0, 0, 18, 20},	//M-Phase, Cat7
											{55.0, 50.0, 0, 140, 10},	//T-Phase, Cat7
											{35.0, 45.0, 0, 180, 15},		//P-Phase, Cat7
											{0,0,0,0,0},					//Dummy
											},
                                            };




const dgChewieAbout_t chewieDefault = {"CHEWIE", "MVP3", "ASBV1.0", "AIB0.0","HMI0.0", 0, 0};


const dgActuatorCtrlSeq_t controlSeq = {
	{ //temp_wr_hum_wr
		{5, SEQ_CTRL_CTCS_ON, SEQ_CTRL_HTR_OFF, SEQ_CTRL_SPRAYER_OFF, SEQ_CTRL_FAN_CWR,   SEQ_CTRL_START, SEQ_CTRL_AIR_IN,0},
		{10, SEQ_CTRL_CTCS_OFF, SEQ_CTRL_HTR_OFF, SEQ_CTRL_SPRAYER_OFF, SEQ_CTRL_FAN_CWR, SEQ_CTRL_END,   SEQ_CTRL_AIR_OFF, 0}
	},
	{  //temp_wr_hum_ar
		{5, SEQ_CTRL_CTCS_ON, SEQ_CTRL_HTR_OFF, SEQ_CTRL_SPRAYER_OFF, SEQ_CTRL_FAN_CCWR,  SEQ_CTRL_START, SEQ_CTRL_AIR_OUT,0},
		{10, SEQ_CTRL_CTCS_OFF, SEQ_CTRL_HTR_OFF, SEQ_CTRL_SPRAYER_OFF, SEQ_CTRL_FAN_CWR, SEQ_CTRL_END,   SEQ_CTRL_AIR_OFF, 0}
	},
	{  //temp_wr_hum_br
		{10, SEQ_CTRL_CTCS_ON, SEQ_CTRL_HTR_OFF, SEQ_CTRL_SPRAYER_ONCE, SEQ_CTRL_FAN_CWR, SEQ_CTRL_START, SEQ_CTRL_AIR_OFF,0},
		{10, SEQ_CTRL_CTCS_OFF, SEQ_CTRL_HTR_OFF, SEQ_CTRL_SPRAYER_OFF, SEQ_CTRL_FAN_CWR, SEQ_CTRL_END,   SEQ_CTRL_AIR_OFF, 0}
	},
	{  //temp_ar_hum_wr
		{1, SEQ_CTRL_CTCS_ON, SEQ_CTRL_HTR_OFF, SEQ_CTRL_SPRAYER_OFF, SEQ_CTRL_FAN_CWR, SEQ_CTRL_START, SEQ_CTRL_AIR_OFF,0},
		{9, SEQ_CTRL_CTCS_OFF, SEQ_CTRL_HTR_OFF, SEQ_CTRL_SPRAYER_OFF, SEQ_CTRL_FAN_CWR, SEQ_CTRL_END,   SEQ_CTRL_AIR_OFF, 0}
	},
	{  //temp_ar_hum_ar
		{1, SEQ_CTRL_CTCS_ON, SEQ_CTRL_HTR_OFF, SEQ_CTRL_SPRAYER_OFF, SEQ_CTRL_FAN_CCWR, SEQ_CTRL_START, SEQ_CTRL_AIR_OUT,0},
		{9, SEQ_CTRL_CTCS_OFF, SEQ_CTRL_HTR_OFF, SEQ_CTRL_SPRAYER_OFF, SEQ_CTRL_FAN_CCWR, SEQ_CTRL_END,   SEQ_CTRL_AIR_OFF, 0}
	},
	{  //temp_ar_hum_br
		{10, SEQ_CTRL_CTCS_ON, SEQ_CTRL_HTR_OFF, SEQ_CTRL_SPRAYER_ONCE, SEQ_CTRL_FAN_CWR, SEQ_CTRL_START, SEQ_CTRL_AIR_IN,0},
		{10, SEQ_CTRL_CTCS_OFF, SEQ_CTRL_HTR_OFF, SEQ_CTRL_SPRAYER_OFF, SEQ_CTRL_FAN_CWR, SEQ_CTRL_END,  SEQ_CTRL_AIR_OFF, 0}
	},
	{  //temp_br_hum_wr
		{3, SEQ_CTRL_CTCS_ON, SEQ_CTRL_HTR_ON, SEQ_CTRL_SPRAYER_OFF, SEQ_CTRL_FAN_CWR, SEQ_CTRL_START,  SEQ_CTRL_AIR_RECIRC, 0},
		{1, SEQ_CTRL_CTCS_OFF, SEQ_CTRL_HTR_OFF, SEQ_CTRL_SPRAYER_OFF, SEQ_CTRL_FAN_CWR, SEQ_CTRL_END,  SEQ_CTRL_AIR_RECIRC, 0}
	},
	{  //temp_br_hum_ar
		{3, SEQ_CTRL_CTCS_ON, SEQ_CTRL_HTR_ON, SEQ_CTRL_SPRAYER_OFF, SEQ_CTRL_FAN_CWR, SEQ_CTRL_START,  SEQ_CTRL_AIR_RECIRC, 0},
		{1, SEQ_CTRL_CTCS_OFF, SEQ_CTRL_HTR_OFF, SEQ_CTRL_SPRAYER_OFF, SEQ_CTRL_FAN_CWR, SEQ_CTRL_END,  SEQ_CTRL_AIR_RECIRC, 0}
	},
	{  //temp_br_hum_br
		{3, SEQ_CTRL_CTCS_ON,  SEQ_CTRL_HTR_ON, SEQ_CTRL_SPRAYER_ONCE, SEQ_CTRL_FAN_CWR, SEQ_CTRL_START, SEQ_CTRL_AIR_RECIRC, 0},
		{1, SEQ_CTRL_CTCS_OFF, SEQ_CTRL_HTR_OFF,SEQ_CTRL_SPRAYER_OFF,  SEQ_CTRL_FAN_CWR, SEQ_CTRL_MID,  SEQ_CTRL_AIR_RECIRC, 0},
		{3, SEQ_CTRL_CTCS_ON,  SEQ_CTRL_HTR_ON, SEQ_CTRL_SPRAYER_OFF,  SEQ_CTRL_FAN_CWR, SEQ_CTRL_MID,  SEQ_CTRL_AIR_RECIRC, 0},
		{1, SEQ_CTRL_CTCS_OFF, SEQ_CTRL_HTR_OFF,SEQ_CTRL_SPRAYER_OFF,  SEQ_CTRL_FAN_CWR, SEQ_CTRL_MID,  SEQ_CTRL_AIR_RECIRC, 0},
		{3, SEQ_CTRL_CTCS_ON,  SEQ_CTRL_HTR_ON, SEQ_CTRL_SPRAYER_OFF,  SEQ_CTRL_FAN_CWR, SEQ_CTRL_MID, SEQ_CTRL_AIR_RECIRC, 0},
		{3, SEQ_CTRL_CTCS_OFF, SEQ_CTRL_HTR_OFF,SEQ_CTRL_SPRAYER_OFF,  SEQ_CTRL_FAN_CWR, SEQ_CTRL_END,  SEQ_CTRL_AIR_RECIRC, 0},
	},
};

/******************* Parameters for Auger Control System *********************************/

const dgCtConfigParam_t ctParam = {30, 30, 3, 5, 60};    //Parameters are in sec


/*********** Control Sequences for Storage Tray transfer Control System ******************/

const dgTransferCtrlSeq_t trfCtrlSeqDefault[] = { {2, SEQ_CTRL_CTMOTOR_CCW, SEQ_CTRL_STMOTOR_ON, SEQ_CTRL_START },
		                                   {1, SEQ_CTRL_CTMOTOR_OFF, SEQ_CTRL_STMOTOR_OFF,SEQ_CTRL_MID },
										   {2, SEQ_CTRL_CTMOTOR_CCW, SEQ_CTRL_STMOTOR_ON, SEQ_CTRL_MID },
										   {1, SEQ_CTRL_CTMOTOR_OFF, SEQ_CTRL_STMOTOR_OFF,SEQ_CTRL_MID },
										   {2, SEQ_CTRL_CTMOTOR_CCW, SEQ_CTRL_STMOTOR_ON, SEQ_CTRL_MID },
										   {1, SEQ_CTRL_CTMOTOR_OFF, SEQ_CTRL_STMOTOR_OFF,SEQ_CTRL_END },
};

//Duration specified in seconds
const dgShredderCtrlSeq_t shdCtrlSeqDefault[] = {
		{ 10,  SEQ_CTRL_SHD_MOTOR_CCWR,  SEQ_CTRL_SHD_FLAP_OFF, SEQ_CTRL_FLUSH_OFF, SEQ_CTRL_SHDAUG_MOTOR_CCWR, SEQ_CTRL_START },
		{ 4,  SEQ_CTRL_SHD_MOTOR_OFF,  SEQ_CTRL_SHD_FLAP_OFF, SEQ_CTRL_FLUSH_OFF, SEQ_CTRL_SHDAUG_MOTOR_OFF, SEQ_CTRL_MID },
		{ 2,  SEQ_CTRL_SHD_MOTOR_CWR, SEQ_CTRL_SHD_FLAP_OFF, SEQ_CTRL_FLUSH_OFF, SEQ_CTRL_SHDAUG_MOTOR_OFF, SEQ_CTRL_MID },
		{ 4,  SEQ_CTRL_SHD_MOTOR_OFF,  SEQ_CTRL_SHD_FLAP_OFF, SEQ_CTRL_FLUSH_OFF, SEQ_CTRL_SHDAUG_MOTOR_OFF, SEQ_CTRL_MID },
		{ 2,  SEQ_CTRL_SHD_MOTOR_CWR, SEQ_CTRL_SHD_FLAP_OFF, SEQ_CTRL_FLUSH_ON,  SEQ_CTRL_SHDAUG_MOTOR_OFF, SEQ_CTRL_MID },
		{ 4,  SEQ_CTRL_SHD_MOTOR_OFF,  SEQ_CTRL_SHD_FLAP_OFF, SEQ_CTRL_FLUSH_OFF, SEQ_CTRL_SHDAUG_MOTOR_OFF, SEQ_CTRL_MID },
		{ 6,  SEQ_CTRL_SHD_MOTOR_CCWR,  SEQ_CTRL_SHD_FLAP_OFF, SEQ_CTRL_FLUSH_ON,  SEQ_CTRL_SHDAUG_MOTOR_OFF, SEQ_CTRL_MID },
		{ 4,  SEQ_CTRL_SHD_MOTOR_OFF,  SEQ_CTRL_SHD_FLAP_OFF, SEQ_CTRL_FLUSH_OFF, SEQ_CTRL_SHDAUG_MOTOR_OFF, SEQ_CTRL_MID },
		{ 10, SEQ_CTRL_SHD_MOTOR_OFF,  SEQ_CTRL_SHD_FLAP_OFF, SEQ_CTRL_FLUSH_OFF, SEQ_CTRL_SHDAUG_MOTOR_OFF, SEQ_CTRL_MID },
		{ 13, SEQ_CTRL_SHD_MOTOR_OFF,  SEQ_CTRL_SHD_FLAP_OPEN,  SEQ_CTRL_FLUSH_OFF, SEQ_CTRL_SHDAUG_MOTOR_OFF, SEQ_CTRL_MID },
		{ 1,  SEQ_CTRL_SHD_MOTOR_OFF,  SEQ_CTRL_SHD_FLAP_CLOSE, SEQ_CTRL_FLUSH_OFF, SEQ_CTRL_SHDAUG_MOTOR_OFF, SEQ_CTRL_END },
		};



#endif


/*******************************************************************************
 * Sys Config API implementation
 ******************************************************************************/

//This method returns Auger CS parameters in the specified buffer from Flash

/*int	loadCtParam(dgCtConfigParam_t *ctParamCur)
{
	//Validate input parameters
	if(ctParamCur==NULL)
	{
		return DG_INVALID_PARAM;
	}

	memcpy((void*)ctParamCur, (void*)&ctParam, sizeof(dgCtConfigParam_t));

	return DG_SUCCESS;
}*/

//This method reads process parameter for a specified phase and waste category from Flash
//and returns in the specified buffer
/*int	loadProcessParam(dgCtProcessParam_t *processParam, uint8_t wasteCat, uint8_t phase)
{
	uint8_t *addr, *startAddr;
	//Validate input parameters
	if((processParam==NULL)||(wasteCat >=WASTE_CAT_MAX)||(phase >= PHASE_COUNT_MAX))
	{
		return DG_INVALID_PARAM;
	}
	addr = (uint8_t*)&processTable;
	startAddr = addr +wasteCat*sizeof(dgCtPhaseProcessParam_t) + phase * sizeof(dgCtProcessParam_t);
	memcpy((void*)processParam, startAddr, sizeof(dgCtProcessParam_t));
	return DG_SUCCESS;
}*/


//This method reads Actuator Control sequence for a HAT state

/*int	loadActuatorSeq(dgActuatorCtrlCode_t *controlCode, uint8_t hatSensorState)
{
	uint8_t *addr;
	//Validate input parameters
	if(controlCode==NULL)
	{
		return DG_INVALID_PARAM;
	}
	addr = (uint8_t*)&controlSeq;
	addr = addr + sizeof(dgActuatorCtrlCode_t)*hatSensorState*MAX_CTRL_CODE_PER_SEQ;
	memcpy((void*)controlCode, (void*)addr, sizeof(dgActuatorCtrlCode_t)*MAX_CTRL_CODE_PER_SEQ);
	return DG_SUCCESS;
}*/

int	loadActuatorSeq(dgActuatorCtrlCode_t *controlCode, uint8_t sensorState)
{
	uint8_t index;
	//Validate input parameters
    if ((controlCode == NULL)||(sensorState>8)) return DG_INVALID_PARAM;

    switch(sensorState)
    {
    case HAT_SENSOR_TEMP_WR_HUM_WR:
    	for(index=0; index<MAX_CTRL_CODE_PER_SEQ; index++)
    	{
            controlCode[index] = allConfig.hatcsConfig.hatcsActuatorCtrlSeq.temp_wr_hum_wr[index];
    	}
    	break;
    case HAT_SENSOR_TEMP_WR_HUM_AR:
    	for(index=0; index<MAX_CTRL_CODE_PER_SEQ; index++)
    	{
            controlCode[index] = allConfig.hatcsConfig.hatcsActuatorCtrlSeq.temp_wr_hum_ar[index];
    	}
    	break;
    case HAT_SENSOR_TEMP_WR_HUM_BR:
    	for(index=0; index<MAX_CTRL_CODE_PER_SEQ; index++)
    	{
            controlCode[index] = allConfig.hatcsConfig.hatcsActuatorCtrlSeq.temp_wr_hum_br[index];
    	}
		break;
    case HAT_SENSOR_TEMP_AR_HUM_WR:
    	for(index=0; index<MAX_CTRL_CODE_PER_SEQ; index++)
    	{
            controlCode[index] = allConfig.hatcsConfig.hatcsActuatorCtrlSeq.temp_ar_hum_wr[index];
    	}
    	break;
    case HAT_SENSOR_TEMP_AR_HUM_AR:
    	for(index=0; index<MAX_CTRL_CODE_PER_SEQ; index++)
    	{
            controlCode[index] = allConfig.hatcsConfig.hatcsActuatorCtrlSeq.temp_ar_hum_ar[index];
    	}
    	break;
    case HAT_SENSOR_TEMP_AR_HUM_BR:
    	for(index=0; index<MAX_CTRL_CODE_PER_SEQ; index++)
    	{
            controlCode[index] = allConfig.hatcsConfig.hatcsActuatorCtrlSeq.temp_ar_hum_br[index];
    	}
    	break;
    case HAT_SENSOR_TEMP_BR_HUM_WR:
    	for(index=0; index<MAX_CTRL_CODE_PER_SEQ; index++)
    	{
            controlCode[index] = allConfig.hatcsConfig.hatcsActuatorCtrlSeq.temp_br_hum_wr[index];
    	}
    	break;
    case HAT_SENSOR_TEMP_BR_HUM_AR:
    	for(index=0; index<MAX_CTRL_CODE_PER_SEQ; index++)
    	{
            controlCode[index] = allConfig.hatcsConfig.hatcsActuatorCtrlSeq.temp_br_hum_ar[index];
    	}
    	break;
    case HAT_SENSOR_TEMP_BR_HUM_BR:
    	for(index=0; index<MAX_CTRL_CODE_PER_SEQ; index++)
    	{
            controlCode[index] = allConfig.hatcsConfig.hatcsActuatorCtrlSeq.temp_br_hum_br[index];
    	}
    	break;
    default:
    	return DG_FAIL;
    	break;
    }
	return DG_SUCCESS;
}


/*--------------------------- Config through Chewie Tool ------------------*/

int verifyConfigAreaChecksum()
{
	uint16_t configAreaSize;
	uint16_t count;
	uint16_t checksum;
	uint16_t *bufferPtr;

	configAreaSize = (sizeof(dgConfigMem_t))/2;
	bufferPtr = (uint16_t *)&allConfig;
	checksum = 0;

	for(count=0; count<configAreaSize; count++)
	{
		checksum = checksum + bufferPtr[count];
	}
	if(checksum == 0) return DG_SUCCESS;
	else return DG_FAIL;
}

void computeConfigAreaChecksum()
{
	uint16_t configAreaSize;
	uint16_t count;
	uint16_t checksum;
	uint16_t *bufferPtr;

	configAreaSize = (sizeof(dgConfigMem_t))/2;
	bufferPtr = (uint16_t *)&allConfig;
	checksum = 0;

	for(count=0; count<configAreaSize-1; count++)
	{
		checksum = checksum + bufferPtr[count];
	}
	checksum = - checksum;
	allConfig.checksumArea[126] = checksum&0xFF;
	allConfig.checksumArea[127] = (checksum>>8) & 0xFF;

}

int loadWorkingConfigEEPROM()
{
	if(readWorkingConfigEEPROM((uint8_t*)&allConfig) != DG_SUCCESS)
	{
		printf("sysConfig.c:readSysConfig():read config eeprom failed\r\n");
		return DG_FAIL;
	}
	//verify checksum
	if(verifyConfigAreaChecksum() != DG_SUCCESS)
	{
		printf("sysConfig.c:readSysConfig():configArea checksum failure\r\n");
		return DG_CHECKSUM_FAIL;
	}
	return DG_SUCCESS;
}

int storeWorkingConfigEEPROM()
{
	//verify checksum
	if(verifyConfigAreaChecksum() != DG_SUCCESS)
	{
		printf("sysConfig.c:storeSysConfig():configArea checksum failure\r\n");
		return DG_FAIL;
	}
	if(writeWorkingConfigEEPROM((uint8_t*)&allConfig) != DG_SUCCESS)
	{
		printf("sysConfig.c:storeSysConfig():write config eeprom failed\r\n");
		return DG_FAIL;
	}

	return DG_SUCCESS;
}


// Shredder
int getShredderCtrlSeq(uint8_t index, dgShredderCtrlSeq_t *out) {
    if (index >= 16 || out == NULL) return DG_INVALID_PARAM;
    *out = allConfig.shredderConfig.shdParams.shdActCtrlSeq[index];
    return DG_SUCCESS;
}

int setShredderCtrlSeq(uint8_t index, const dgShredderCtrlSeq_t *value) {
    if (index >= 16 || value == NULL) return DG_INVALID_PARAM;
    allConfig.shredderConfig.shdParams.shdActCtrlSeq[index] = *value;
    return DG_SUCCESS;
}

int getShredderTimingParam(dgShredderTimingVar_t *out) {
    *out = allConfig.shredderConfig.shdParams.shdTimingVars;
    return DG_SUCCESS;
}

int setShredderTimingParam(const dgShredderTimingVar_t *value) {
	allConfig.shredderConfig.shdParams.shdTimingVars = *value;
    return DG_SUCCESS;
}

// Transfer
int getTransferCtrlSeq(dgTransferCtrlSeq_t *out) {
    if (out == NULL) return DG_INVALID_PARAM;
    *out = allConfig.transferConfig.transferParam;
    return DG_SUCCESS;
}

int setTransferCtrlSeq(const dgTransferCtrlSeq_t *value) {
    if (value == NULL) return DG_INVALID_PARAM;
    allConfig.transferConfig.transferParam = *value;
    return DG_SUCCESS;
}

// Auger
int getAugerConfig(dgCtConfigParam_t *out) {
    if (out == NULL) return DG_INVALID_PARAM;
    *out = allConfig.augerConfig.augerParam;
    return DG_SUCCESS;
}

int setAugerConfig(const dgCtConfigParam_t *value) {
    if (value == NULL) return DG_INVALID_PARAM;
    allConfig.augerConfig.augerParam = *value;
    return DG_SUCCESS;
}

// HATCS
int getHatcsActuatorCtrlSeq(uint8_t sensorState, uint8_t index, dgActuatorCtrlCode_t *out)
{
    if ((out == NULL)||(sensorState>8)||(index >= MAX_CTRL_CODE_PER_SEQ)) return DG_INVALID_PARAM;
    switch(sensorState)
    {
    case HAT_SENSOR_TEMP_WR_HUM_WR:
        *out = allConfig.hatcsConfig.hatcsActuatorCtrlSeq.temp_wr_hum_wr[index];
    	break;
    case HAT_SENSOR_TEMP_WR_HUM_AR:
        *out = allConfig.hatcsConfig.hatcsActuatorCtrlSeq.temp_wr_hum_ar[index];
    	break;
    case HAT_SENSOR_TEMP_WR_HUM_BR:
        *out = allConfig.hatcsConfig.hatcsActuatorCtrlSeq.temp_wr_hum_br[index];
		break;
    case HAT_SENSOR_TEMP_AR_HUM_WR:
        *out = allConfig.hatcsConfig.hatcsActuatorCtrlSeq.temp_ar_hum_wr[index];
    	break;
    case HAT_SENSOR_TEMP_AR_HUM_AR:
        *out = allConfig.hatcsConfig.hatcsActuatorCtrlSeq.temp_ar_hum_ar[index];
    	break;
    case HAT_SENSOR_TEMP_AR_HUM_BR:
        *out = allConfig.hatcsConfig.hatcsActuatorCtrlSeq.temp_ar_hum_br[index];
    	break;
    case HAT_SENSOR_TEMP_BR_HUM_WR:
        *out = allConfig.hatcsConfig.hatcsActuatorCtrlSeq.temp_br_hum_wr[index];
    	break;
    case HAT_SENSOR_TEMP_BR_HUM_AR:
        *out = allConfig.hatcsConfig.hatcsActuatorCtrlSeq.temp_br_hum_ar[index];
    	break;
    case HAT_SENSOR_TEMP_BR_HUM_BR:
        *out = allConfig.hatcsConfig.hatcsActuatorCtrlSeq.temp_br_hum_br[index];
    	break;
    default:
    	return DG_FAIL;
    	break;
    }

    return DG_SUCCESS;
}

int setHatcsActuatorCtrlSeq(uint8_t sensorState, uint8_t index, const dgActuatorCtrlCode_t *value)
{

    if ((value == NULL)||(sensorState>8)||(index >= MAX_CTRL_CODE_PER_SEQ)) return DG_INVALID_PARAM;

    switch(sensorState)
    {
    case HAT_SENSOR_TEMP_WR_HUM_WR:
        allConfig.hatcsConfig.hatcsActuatorCtrlSeq.temp_wr_hum_wr[index]=*value;
    	break;
    case HAT_SENSOR_TEMP_WR_HUM_AR:
        allConfig.hatcsConfig.hatcsActuatorCtrlSeq.temp_wr_hum_ar[index]=*value;
    	break;
    case HAT_SENSOR_TEMP_WR_HUM_BR:
        allConfig.hatcsConfig.hatcsActuatorCtrlSeq.temp_wr_hum_br[index]=*value;
		break;
    case HAT_SENSOR_TEMP_AR_HUM_WR:
        allConfig.hatcsConfig.hatcsActuatorCtrlSeq.temp_ar_hum_wr[index]=*value;
    	break;
    case HAT_SENSOR_TEMP_AR_HUM_AR:
        allConfig.hatcsConfig.hatcsActuatorCtrlSeq.temp_ar_hum_ar[index]=*value;
    	break;
    case HAT_SENSOR_TEMP_AR_HUM_BR:
        allConfig.hatcsConfig.hatcsActuatorCtrlSeq.temp_ar_hum_br[index]=*value;
    	break;
    case HAT_SENSOR_TEMP_BR_HUM_WR:
        allConfig.hatcsConfig.hatcsActuatorCtrlSeq.temp_br_hum_wr[index]=*value;
    	break;
    case HAT_SENSOR_TEMP_BR_HUM_AR:
        allConfig.hatcsConfig.hatcsActuatorCtrlSeq.temp_br_hum_ar[index]=*value;
    	break;
    case HAT_SENSOR_TEMP_BR_HUM_BR:
        allConfig.hatcsConfig.hatcsActuatorCtrlSeq.temp_br_hum_br[index]=*value;
    	break;
    default:
    	return DG_FAIL;
    }
    return DG_SUCCESS;
}

// CSM
int getCsmPhaseParam(uint8_t wasteCat, uint8_t phase, dgCtProcessParam_t *out) {
    if ((out == NULL)||(wasteCat>=WASTE_CAT_MAX)||(phase >= TRANSFER_PHASE)) return DG_INVALID_PARAM;
    dgCtPhaseProcessParam_t *param;
    //Get the process param table for the req wastecat
    switch (wasteCat)
    {
        case WASTE_CAT0: param = &allConfig.csmConfig.csmProcessParam.cat0; break;
        case WASTE_CAT1: param = &allConfig.csmConfig.csmProcessParam.cat1; break;
        case WASTE_CAT2: param = &allConfig.csmConfig.csmProcessParam.cat2; break;
        case WASTE_CAT3: param = &allConfig.csmConfig.csmProcessParam.cat3; break;
        case WASTE_CAT4: param = &allConfig.csmConfig.csmProcessParam.cat4; break;
        case WASTE_CAT5: param = &allConfig.csmConfig.csmProcessParam.cat5; break;
        case WASTE_CAT6: param = &allConfig.csmConfig.csmProcessParam.cat6; break;
        case WASTE_CAT7: param = &allConfig.csmConfig.csmProcessParam.cat7; break;
        default: param = NULL;
    }

    //Get the process param for the specified phase
    switch (phase)
    {
        case MESOPHILIC_PHASE: *out = param->mPhase; break;
        case THERMOPHILIC_PHASE: *out = param->tPhase; break;
        case PATHOGEN_ELM_PHASE: *out = param->pPhase; break;
    }

    return DG_SUCCESS;
}

int setCsmPhaseParam(uint8_t wasteCat, uint8_t phase, dgCtProcessParam_t *value) {
    dgCtPhaseProcessParam_t *param;

    if ((value == NULL)||(wasteCat>=WASTE_CAT_MAX)||(phase >= TRANSFER_PHASE)) return DG_INVALID_PARAM;

    //Get the process param table for the req wastecat
    switch (wasteCat)
    {
        case WASTE_CAT0: param = &allConfig.csmConfig.csmProcessParam.cat0; break;
        case WASTE_CAT1: param = &allConfig.csmConfig.csmProcessParam.cat1; break;
        case WASTE_CAT2: param = &allConfig.csmConfig.csmProcessParam.cat2; break;
        case WASTE_CAT3: param = &allConfig.csmConfig.csmProcessParam.cat3; break;
        case WASTE_CAT4: param = &allConfig.csmConfig.csmProcessParam.cat4; break;
        case WASTE_CAT5: param = &allConfig.csmConfig.csmProcessParam.cat5; break;
        case WASTE_CAT6: param = &allConfig.csmConfig.csmProcessParam.cat6; break;
        case WASTE_CAT7: param = &allConfig.csmConfig.csmProcessParam.cat7; break;
        default: param = NULL;

    }
    //Get the process param for the specified phase
    switch (phase)
    {
        case MESOPHILIC_PHASE: param->mPhase = *value; break;
        case THERMOPHILIC_PHASE: param->tPhase = *value; break;
        case PATHOGEN_ELM_PHASE: param->pPhase = *value; break;
    }

    return DG_SUCCESS;
}

int loadDefaultConfig()
{
	//allConfig

	//Copy hatcsConfig default
	dgHatcsConfig_t *hatcsConfigPtr;
	hatcsConfigPtr = &(allConfig.hatcsConfig);
	memcpy((void*)(hatcsConfigPtr),(void*)&controlSeq, sizeof(controlSeq));

	//Copy csmConfig default
	dgCsmConfig_t *csmConfigPtr;
	csmConfigPtr = &(allConfig.csmConfig);
	memcpy((void*)(csmConfigPtr),(void*)&processTable, sizeof(processTable));
	//Copy augerConfig default
	dgAugerConfig_t *augerConfigPtr;
	augerConfigPtr = &(allConfig.augerConfig);
	memcpy((void*)(augerConfigPtr),(void*)&ctParam, sizeof(ctParam));

	//Copy transferConfig default
	dgTransferConfig_t *transferConfigPtr;
	transferConfigPtr = &(allConfig.transferConfig);
	memcpy((void*)(transferConfigPtr),(void*)&trfCtrlSeqDefault, sizeof(trfCtrlSeqDefault));

	//Copy additiveConfig default

	//Copy shredderConfig default
	allConfig.shredderConfig.shdParams.shdTimingVars.flapCloseDur = FLAP_CLOSE_DURATION_DEFAULT;
	allConfig.shredderConfig.shdParams.shdTimingVars.flapOpenDur  = FLAP_OPEN_DURATION_DEFAULT;
	allConfig.shredderConfig.shdParams.shdTimingVars.shdStartDlyFmLidclose = SHD_START_DELAY_DEFAULT;
	allConfig.shredderConfig.shdParams.shdTimingVars.numOfCtrlSeq = 11;

	dgShredderCtrlSeq_t *shredderConfigPtr;
	shredderConfigPtr = &(allConfig.shredderConfig.shdParams.shdActCtrlSeq[0]);

	memcpy((void*)(shredderConfigPtr),(void*)&shdCtrlSeqDefault, sizeof(dgShredderCtrlSeq_t)*MAX_SHD_SEQ);

	//Compute checksum
	computeConfigAreaChecksum();

	//Store in EEPROM config area
	if(storeWorkingConfigEEPROM() == DG_SUCCESS)
	{
		printf("sysConfig.c:loadDefaultConfig():storeWorkingConfigEEPROM():Success");
		return DG_SUCCESS;
	}
	printf("sysConfig.c:loadDefaultConfig():storeWorkingConfigEEPROM():Fail\r\n");
	return DG_FAIL;
}

int storeAllconfigEEPROM()
{
	//Compute and update checksum for the allconfig area
	computeConfigAreaChecksum();

	if(storeWorkingConfigEEPROM() != DG_SUCCESS)
	{
		printf("sysConfig.c:storeAllconfigEEPROM(): EEPROM write error\r\n");
		return DG_FAIL;
	}
	return DG_SUCCESS;
}

int initSysConfig()
{
	int retValue;

/*	loadDefaultConfig();
	return DG_SUCCESS;*/

	retValue = loadWorkingConfigEEPROM();
	if(retValue == DG_SUCCESS)
	{
		printf("sysConfig.c:initSysConfig(): loadWorkingConfigEEPROM(): Success\r\n");
		return DG_SUCCESS;
	}
	else if(retValue == DG_CHECKSUM_FAIL)
	{
		printf("sysConfig.c:initSysConfig(): loadWorkingConfigEEPROM()':Fail. Loading default config\r\n");
		//Config data is not written. Write default values
		loadDefaultConfig();
	}
	else return DG_FAIL;

	return DG_SUCCESS;
}
