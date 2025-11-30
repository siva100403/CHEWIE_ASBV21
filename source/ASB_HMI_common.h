/*
 * ASB_HMI_common.h
 *
 *  Created on: 05-May-2025
 *      Author: Jawahar Arumugam
 */

#ifndef ASB_HMI_COMMON_H_
#define ASB_HMI_COMMON_H_

/******************************************************************
*                   ASB-HMI Commands                                  *
*******************************************************************/

#define ACMD_KEEP_ALIVE				0
#define GET_SENSOR_VALUES			1
#define PROXIMITY_EVENTS			2
#define ACTUATOR_CTRL_REQ			3
#define USER_REQUESTS				4
#define ALARM_STATUS_REQ			5
#define ALARM_CLEAR_REQ				6
#define GET_SYSTEM_STATUS			7
#define CS_CTRL_REQ					8
#define GET_ASB_VERSION				9
#define GET_ASB_DATETIME			10
#define ACMD_LAST					11       //For validation purpose. Should be one more than the last command

/******************************Packet structure*********************/

#define FLAG_BYTE				0
#define COMMAND_BYTE			1
#define SEQUENCE_BYTE			2
#define LENGTH_BYTE				3
#define PAYLOAD_START			4

#define MAX_PAYLOAD_SIZE		120
#define MAX_PACKET_SIZE			128   		//Payload + Packet header + Checksum
#define MAX_TX_BUFFER_SIZE		192			//Packet inserted with ESC seq (50% overhead added)

/********************************Flag bits ***********************/
#define HMI_TO_ASB				0x00
#define ASB_TO_HMI				0x01

#define CMD_EXECUTION_SUCCESS 	0x00
#define CMD_EXECUTION_ERROR		0x04
#define CMD_RCVD_WITH_ERROR		0x08
#define CMD_INVALID				0x0C

/************************Special characters used**********************/
#define START_CHR		'\002'         	//STX
#define END_CHR			'\003'			//ETX
#define ESCAPE_CHR		'\033'			//ESC


/****************************** Device IDs ***************************/
//On-board deives
#define	RTC_DEV					0
#define	EEPROM_DEV				1
#define	HMI_UART_DEV			2
#define CLI_UART_DEV			3
#define DRV89XX_1_DEV			4
#define DRV89XX_2_DEV			5
#define DRV89XX_3_DEV			6
#define TDC1000_1_DEV			7
#define TDC1000_2_DEV			8
//Relay controlled devices
#define	SHD_MOTOR_DEV			9
#define HEATER_DEV				10
//Bi-directional Motor
#define LID_MOTOR_DEV			11
#define	AUGER_MOTOR_DEV			12
#define FAN_MOTOR_DEV			13
#define FLAP_MOTOR_DEV			14
//Solenoid and Uni-directional motor
#define DCSPRAYER_DEV			15
#define ADDITIVE_MOTOR_DEV		16
#define ST_MOTOR_DEV			17
#define FLUSHSPRAYER_DEV		18
#define AIRVALVE1_DEV			19
#define AIRVALVE2_DEV			20
#define AIRVALVE3_DEV			21
//Sensor Device
#define	TEMPSENSOR_DEV			22
#define	CAMERA_DEV				23
#define	DC_LSENSE_DEV			24 		//Digestion Chamber
#define	ST_LSENSE_DEV			25		//Storage Tray Level Sensor CLI_UART_DEV
#define	ADDITIVE_LSENSE_DEV		26
#define LAST_DEVICE				27


/************************* Control System IDs **************************/
#define CSM_CS		0
#define CT_CS		1


/**************************Payload definitions for commands***********/
//Sensor values
typedef struct sensorPayload
{
	float	temp;
	float	humidity;
	float	storageLevel;
	float	enzymeLevel;
	float	digChamberLevel;
}dgSensorPayload_t;


//This structure is used to return the system status for the ASB-HMI command GET_SYSTEM_STATUS
typedef struct sysStatus
{
	float	curTemp;
	float 	setTemp;
	float	curHumidity;
	float	setHumidity;
	float	storageLevel;
	float	enzymeLevel;
	float	digChamberLevel;
	uint16_t mPhaseDur;
	uint16_t mPhaseElapsedDur;
	uint16_t tPhaseDur;
	uint16_t tPhaseElapsedDur;
	uint16_t pPhaseDur;
	uint16_t pPhaseElapsedDur;
	uint8_t wasteCat;
	uint8_t curPhase;

}dgSysStatusPayload_t;

//This structure is used for sending proximity evenets

typedef struct proximityEvents
{
	uint8_t proximity;
	uint8_t dummy;
}dgProximityEvents_t;

//Definition of proximity byte: 00cc bbaa
//   cc - indciates the capacitive proximity event
//   bb - indicates the IRHA25 proximity event
//   aa - indicates the IRHA10 proximity event

//interpretation of aa, bb, cc bits
//   00 - No event reported
//   01 - Reserved
//   10 - Proximity event removed
//   11 - Proximity event detected

#define CAP_PROXIMITY_EVENT_MASK		0x30
#define IRHA25_PROXIMITY_EVENT_MASK		0x0C
#define IRHA10_PROXIMITY_EVENT_MASK		0x03

#define CAP_PROXIMITY_EVENT_DETECTED	0x30
#define CAP_PROXIMITY_EVENT_REMOVED		0x20
#define CAP_PROXIMITY_NO_EVENT_REPORT	0x00

#define IRHA25_PROXIMITY_EVENT_DETECTED		0x0C
#define IRHA25_PROXIMITY_EVENT_REMOVED		0x08
#define IRHA25_PROXIMITY_NO_EVENT_REPORT	0x00

#define IRHA10_PROXIMITY_EVENT_DETECTED		0x03
#define IRHA10_PROXIMITY_EVENT_REMOVED		0x02
#define IRHA10_PROXIMITY_NO_EVENT_REPORT	0x00

// Structure used to pass parameters from HMI to ASB for actuator Control
//The interpretation of ctrlParam1 and ctrlParam2 depends on the Device ID
typedef struct actuatorCtrl
{
	int deviceID;
	int ctrlParam1;
	int ctrlParam2;
}dgactuatorCtrlCmd_t;

#define DEVICE_OFF			0
#define DEVICE_ON			1
#define DEVICE_ROTATE_CW	2
#define DEVICE_ROTATE_CCW	3
#define DEVICE_OPEN			4
#define DEVICE_CLOSE		5

// Structure used to pass parameters from HMI to ASB for getting ASB version info

typedef struct asbVersion
{
	char fwVersion[12];
	char hwVersion[32];

}dgAsbVersion_t;


/*****************************Function Prototypes*******************************/

int preparePacket(uint8_t flags, uint8_t* rcvPacket, uint8_t *payload, uint8_t payloadSize, uint8_t *packetBuff, uint8_t *packetSize);

int computeCheksum(uint8_t *packetBuff, uint16_t size, uint16_t *checksum);
int verifyCheksum(uint8_t *packetBuff, uint8_t packetSize);

int insertEsc(uint8_t *packetBuff, uint8_t packetSize, uint8_t *txBuff, uint16_t *txSize);
int stripEsc(uint8_t *rxBuff, uint8_t rxSize, uint8_t *packetBuff, uint8_t *packetSize);

#endif /* ASB_HMI_COMMON_H_ */
