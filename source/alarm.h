/*
 * alarm.h
 *
 *  Created on: 29-Dec-2024
 *      Author: Jawahar Arumugam
 */

#ifndef ALARM_H_
#define ALARM_H_



/************************************************************************
 * 							Chewie Alarm Definitions 					*
 *
 ************************************************************************/





//Alarm Groups
#define ALARM_GROUP_HWFAULT			1		//Hardware fault
#define ALARM_GROUP_OP				2		//Operation related like Storage tray full, water not available, etc
#define ALARM_GROUP_CONFIG			3    	//System Configuration related alarm
#define ALARM_VER_MISMATCH			4		// Version mismatch with other sub-systems or API


//Alarm Category
#define ALARM_CAT_CRITICAL
#define ALARM_CAT_MAJOR
#define ALARM_CAT_MINOR
#define ALARM_CAT_INFO

//Alarm status
#define ALARM_GENERATED		1
#define ALARM_CLEARED		2

//Alarm Number
#define HW_ALARM_TEMP_HUM_SESNOR	1
#define HW_ALARM_RTC				2
#define HW_ALARM_EEPROM				3
#define HW_ALARM_CAMERA				4
#define HW_ALARM_CLI_UART			5
#define HW_ALARM_HMI_UART			6
#define HW_ALARM_DRV89XX_1			7
#define HW_ALARM_DRV89XX_2			8
#define HW_ALARM_TDC1000_1			9
#define HW_ALARM_TDC1000_2			10
#define HWS_ALARM_LID				11

#define OPS_ALARM_TRAY_FULL			20
#define OPS_ALARM_ADDITIVE_LOW		21
#define OPS_ALARM_ODOURSHIELD		22
#define OPS_ALARM_HUMIDITY_CTRL		23
#define OPS_ALARM_TEMP_CTRL			24
#define OPS_ALARM_FOREIGN_MAT		25
#define OPS_ALARM_SHD_OVERLOAD		26
#define LAST_ALARM					27


typedef struct alarmDefinition
{
	uint8_t alarmNumber;
	uint8_t alarmGroup;
	uint8_t alarmCat;
	char	alarmDescription[64];
}dgAlarmDefintion_t;

typedef struct activeAlarm
{
	uint8_t alarmNumber;
	dgDateTime_t alarmTime;
	uint8_t alarmStatus;
	char	alarmComment[32];

}dgActiveAlarm_t;

void generateAlert(uint16_t alert);


#endif /* ALARM_H_ */
