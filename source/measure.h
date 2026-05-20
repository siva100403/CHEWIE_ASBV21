/*
 * config.h
 *
 *  Created on: 27-Dec-2024
 *      Author: Jawahar Arumugam
 */

#ifndef MEASURE_H_
#define MEASURE_H_


#include "csmMod.h"


/*********************** Battery Backup RAM usage *****************/

#define CHEWIE_STATE_STORE_ADDR		0x20     //Address in the RTC RAM
#define CHEWIE_STATE_STORE_SIZE		0x10	//16 bytes
#define CSM_SCHDUR_LB_ADDR		0x20
#define CSM_SCHDUR_HB_ADDR		0x21
#define CSM_EXPDUR_LB_ADDR		0x22
#define CSM_EXPDUR_HB_ADDR		0x23
#define CSM_REMDUR_LB_ADDR		0x24
#define CSM_REMDUR_HB_ADDR		0x25
#define CSM_CURPHASE_ADDR		0x26
#define CSM_PREVPHASE_ADDR		0x27
#define CSM_WASTECAT_ADDR		0x28
#define CSM_STATE_ADDR			0x29


#define MATERIAL_LEVEL_STORE_ADDR	0x30	//Address in RTC RAM
#define MATERIAL_LEVEL_STORE_SIZE	0x8		//8 bytes

//Stores the state of ADCS. After power up this location is checked
// to decide the state of ADCS
#define ADCS_STATE_ADDR				0x39	//1 byte

//Stores the state of MKValve
#define MKVALVE_POSITION_ADDR		0x3A    //1 Byte



#define USAGE_COUNTER_STORE_ADDR	0x40	//Address in RTC RAM
#define USAGE_COUNTER_STORE_SIZE	0x20	//32 bytes




typedef struct chewieStateStore
{
	dgDateTime_t timeOfUpdate;
	uint16_t	schDur;		//in minutes. Duration as per configuration
	uint16_t	expDur;		//in minutes. Time spent in this state so far
	uint16_t	remDur;		//in minutes. Time remaining to complete the phase
	uint8_t		curPhase;	//Composting Phase
	uint8_t 	prevPhase;
	uint8_t		wasteCat;
	uint8_t		state;		//composting state machine state

}dgChewieStateStore_t;

typedef struct matLevelStore
{
	dgDateTime_t timeOfUpdate;
	uint8_t  ccLevel;				//in cm - compost chamber level
	uint8_t  scLevel;				//in cm - storage chmaber level
	uint8_t  enzymeLevel;			//in cm - enzyme storage level
	uint8_t  shredderLevel;			//in cm - material level in shredder
}dgMatLevelStore_t;


typedef struct usageCounter
{
	dgDateTime_t timeOfStart;
	uint16_t ctmotorCounter;		//in seconds-seconds ctmotor was ON
	uint16_t shredderCounter;		//in seconds - seconds shredder was ON
	uint16_t lidCounter;			//Count - number of open/close cycles of lid
}dgUsageCounter_t;





int updateChewieStateStoreExpDur(uint16_t expDur, uint16_t remDur);
int updateCounterCtmotor(uint16_t ctmotor);
int updateCounterShmotor(uint16_t shmotor);
int updateCounterLid(uint16_t lidCycles);
int updateChewieStateStore(dgCtStateVar_t *ctVar);
int getChewieStateStore(dgDateTime_t *updateTime,dgCtStateVar_t *ctVar);

int getAdcsState(uint8_t *state);
int updateAdcsState(uint8_t state);

int getMkValvePosition(uint8_t *state);
int updateMkValvePosition(uint8_t state);


#endif /* MEASURE_H_ */
