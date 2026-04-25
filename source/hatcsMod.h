/*
 * hatcsMod.h
 *
 *  Created on: 31-Dec-2024
 *      Author: Jawahar Arumugam
 */

#ifndef HATCSMOD_H_
#define HATCSMOD_H_

/**********Humidity-Aeration-Temperature (HAT) Control System************
 * HATCS, when enabled, maintains the temperature and humidity of 		*
 * the compost chamber within the specified range. Ensures Aeration		*
 ************************************************************************/


/********************************************************************
 * Function prototypes												*
 ********************************************************************/





/********************************************************************
 * Data structures													*
 ********************************************************************/

//Process control variables
typedef struct hatsProcessVar
{
	float		targetTemp;       	    	//set temperature
	float		targetTempLow;				//Lower temperature range
											// tempTarget - tempRange
	float		targetTempHigh;				//Higher temperature range
											// tempTarget+tempRange
	float		targetHumidity;				//Set Humidity value
	float		targetHumidityLow;			//Lower Humidity value
	float		targetHumidityHigh;			//Higher Humidity value
}hatcsProcessVar_t;





/********************************************************************
 * Variables and constants											*
 ********************************************************************/


#define AIR_RECIRC		0
#define AIR_OUT			1

//HATCS states

#define HATCS_STATE_IDLE		0
#define HATCS_STATE_ACTIVE		1
#define HATCS_STATE_ERROR		2

/*
#define HATCS_STATE_TWR_HWR		0		//Temperature and Humidity within Range
#define HATCS_STATE_TWR_HAR		1		//Temperature within Range & Humidity Above Range
#define HATCS_STATE_TWR_HBR		2		//Temperature within Range & Humidity Below Range
#define HATCS_STATE_TAR_HWR		3		//Temperature Above Range & Humidity within Range
#define HATCS_STATE_TAR_HAR		4		//Temperature Above Range & Humidity Above Range
#define HATCS_STATE_TAR_HBR		5		//Temperature Above Range & Humidity Below Range
#define HATCS_STATE_TBR_HWR		6		//Temperature Below Range & Humidity within Range
#define HATCS_STATE_TBR_HAR		7		//Temperature Below Range & Humidity above Range
#define HATCS_STATE_TBR_HBR		8		//Temperature Below Range & Humidity below Range
*/



int initHatcs(void);
int sprayOnceDC(uint8_t duration);
int sprayOnceDCmSec(uint16_t duration);
int setMkValveStatus(uint8_t status);

#endif /* HATCSMOD_H_ */
