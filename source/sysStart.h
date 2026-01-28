/*
 * sysStart.h
 *
 *  Created on: 05-May-2025
 *      Author: Jawahar Arumugam
 */

#ifndef SYSSTART_H_
#define SYSSTART_H_


/****************************SYSSTART Module states ****************************/
#define SYSSTART_STATE_START	1

/*******************Values for module/Device presence status *******************/
#define UNKOWN_MODULE_PRESENCE	0
#define MODULE_PRESENT			1
#define MODULE_NOTPRESENT		2
#define CNI_MODULE_PRESENCE		3		//Could Not Identify Module presence

#define UNKOWN_DEVICE_PRESENCE	0
#define DEVICE_PRESENT			1
#define DEVICE_NOTPRESENT		2
#define CNI_DEVICE_PRESENCE		3		//Could Not Identify Device presence

/*******************Values for Module/Device Operation Status ******************/
#define UNKOWN_MODULE_STATUS	0
#define MODULE_WORKING			1
#define MODULE_NOTWORKING		2
#define MODULE_PARTIAL_WORKING	3
#define MODULE_DISABLED			4
#define MODULE_IDLE				5

#define UNKOWN_DEVICE_STATUS	0
#define DEVICE_WORKING			1
#define DEVICE_NOTWORKING		2
#define DEVICE_PARTIAL_WORKING	3
#define DEVICE_DISABLED			4
#define DEVICE_IDLE				5

/******************** Structure to store individual module health *******/
typedef struct healthStatus
{
	uint8_t presenceStatus;
	uint8_t operationStatus;
}dgHealthStatus_t;


/************************ System Health Repository **********************/

/*typedef struct devHealthRegister
{
	dgHealthStatus_t	rtc;
	dgHealthStatus_t	eeprom;
	dgHealthStatus_t	hmiUART;
	dgHealthStatus_t	tempSensor;
	dgHealthStatus_t	humSensor;
	dgHealthStatus_t	dcLevelSensor; 		//Digestion Chamber
	dgHealthStatus_t	stLevelSensor;		//Storage Tray Level Sensor
	dgHealthStatus_t	enzymeLevelSensor;
	dgHealthStatus_t	augerMotor;
	dgHealthStatus_t	shredderMotor;


}dgDevHealthRegister_t;


typedef struct modHealthRegister
{
	dgHealthStatus_t	hmiComm;
	dgHealthStatus_t	lidModule;
	dgHealthStatus_t	limitSwitchModule;
	dgHealthStatus_t	augerModule;				//CT Module
	dgHealthStatus_t 	HMICmdProcModule;
	dgHealthStatus_t 	compostingModule;
	dgHealthStatus_t 	hatCSModule;

}dgModHealthRegister_t;*/


/******************************* Function Prototypes ********************/
int initSysStart(void);
uint8_t setDeviceHealth(uint8_t deviceId, uint8_t opStatus, uint8_t presenceStatus);
uint8_t getDeviceHealth(uint8_t deviceId, uint8_t* opStatus, uint8_t* presenceStatus);

#endif /* SYSSTART_H_ */
