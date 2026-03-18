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


#define ALERT_TRANSFER_PENDING				1			//This is due to STORAGE_TRAY_FULL or STORAGE_TRAY_OPEN
#define ALERT_STORAGE_TRAY_FULL				2
#define ALERT_STORAGE_TRAY_OPEN				3
#define ALERT_TRANSFER_TIMEOUT				4			//Transfer taking longer than the timeout


//Alarm Groups
#define ALARM_GROUP_HWFAULT			1		//Hardware fault
#define ALARM_GROUP_OP				2		//Operation related like Storage tray full, water not available, etc
#define ALARM_GROUP_CONFIG			3    	//System Configuration related alarm

typedef struct alarmDefinition
{
	uint8_t alarmGroup;
	uint8_t alarmNumber;
	unit8_t alarmCat
};

void generateAlert(uint16_t alert);


#endif /* ALARM_H_ */
