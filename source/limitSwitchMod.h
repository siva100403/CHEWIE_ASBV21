/*
 * limitSwitch.h
 *
 *  Created on: 16-May-2025
 *      Author: Jawahar Arumugam
 */

#ifndef LIMITSWITCHMOD_H_
#define LIMITSWITCHMOD_H_

/******************************** Limit Switch Sense Module ***********************
 * Four Limit switch sensors
 *   - LS_SENSE_LIDCLOSE: Monitors the LID Close status
 *        - HIGH - when Lid reached CLOSE position
 *        - LOW - when Lid has not reached CLOSE position
 *  - LS_SENSE_LIDOPEN: Monitors the LID open status
 *        - HIGH - when Lid reached OPEN position
 *        - LOW  - when Lid has not reached OPEN position
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

#define DEBOUNCE_COUNT		2

/*----------------------Raw status of limit switch sensors-----------------*/
//Raw status of Limit switch sensor based on RS232 Driver
//RS232 driver based LS sensing has current limit and short circuit protection
#define LS232DRVR_OPEN		1			//RS232C driver will read logic HIGH when the input is OPEN
#define LS232DRVR_CLOSE		0			//When the Limit switch is CLOSED, driver will read logic LOW as the limit switch other terminal is driven LOW.

//Raw status of Limit switch sensor based on 12V sensing using transistor circuit
//Does not have short circuit or current limit protection
#define LIMITSWITCH_OPEN		0		//When the Limit switch is in OPEN condition
#define LIMITSWITCH_CLOSE		1		//When the limit switch is in CLOSE condition

/*-------------------Status of limit switch sensors after debouncing-------*/
//Lid close status
#define LID_NOTCLOSED			0
#define LID_CLOSED				1

//Lid Open Status
#define LID_NOTOPEN				0
#define LID_OPEN				1

//Flap position status
#define FLAP_POSITION_NOTCLOSED	0
#define FLAP_POSITION_CLOSED	1

//STTV (Storage Tray Transfer Valve position status
#define STTV_POSITION_NOTCLOSED	0
#define STTV_POSITION_CLOSED	1

/*------Lid Status after combining CLOSE and OPEN limit switches---------*/
#define LID_STATUS_OPEN			0
#define LID_STATUS_INBETWEEN	1
#define LID_STATUS_CLOSED		2
#define LID_STATUS_UNKNOWN		3
#define LID_STATUS_OPENING		4
#define LID_STATUS_CLOSING		5
#define LID_STATUS_ERROR		6

/*-------Lid Sensing Enable/Disable Macro--------------------------------*/
#define LID_SENSING_ENA			1
#define LID_SENSING_DIS			0

/*************************** Function Prototypes *************************/


uint8_t getLidSwicthStatus(void);
void enableLidStatusSensing(void);
void disableLidStatusSensing(void);




uint8_t getStorageTraySwicthStatus(void);
uint8_t getFlapStatus(void);
#define STORAGE_TRAY_CLOSE	0
#define STORAGE_TRAY_OPEN	1


int initLimitSwitchModule(void);

#endif /* LIMITSWITCHMOD_H_ */
