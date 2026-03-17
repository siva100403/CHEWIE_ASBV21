/*
 * actuatorStatusTracker.h
 *
 *  Created on: 13-Mar-2026
 *      Author: Jawahar Arumugam
 */

#ifndef ACTUATORSTATUSTRACKER_H_
#define ACTUATORSTATUSTRACKER_H_



#define ON		1
#define OFF		0
#define CWR		1
#define CCWR	2
//For Lid ST and FLAP
#define CLOSE	0
#define CLOSING	1
#define OPENING	2
#define OPEN 	3


typedef union actuatorStatus {
    struct {
        // Bit fields should be of an integer type (unsigned int, signed int, int)
    	uint32_t lidStatus			: 2; //OPEN, OPENING, CLOSE, CLOSING
    	uint32_t augerStatus		: 2; //OFF, CWR, CCWR
    	uint32_t flapStatus			: 2; //
    	uint32_t stValveStatus		: 2;
    	uint32_t htrFan				: 2; // 2 CWR, CCWR, OFF
        uint32_t exhFan  			: 1; // 1 ON, OFF bit for Additive Motor Status
        uint32_t additiveMotorStatus  	: 1; // 1 ON, OFF bit for Additive Motor Status
        uint32_t dcSprayer  		: 1; // 1 bit for Digestion Chamber sprayer status
        uint32_t flushSprayer 		: 1; // 1 bit for timeout flag
        uint32_t airValve1   		: 1; // 1 bit for airvalve1
        uint32_t airValve2   		: 1; // 1 bit for airvalve2
        uint32_t airValve3   		: 1; // 1 bit for airvalve3
        uint32_t heaterStatus		: 1; // 1 bit

    } bits;
    // An alternative way to access the entire 8 bits as a single byte
    uint32_t combinedActuatorStatus;
}dgActuatorStatus_t;


/************************* Function Prototypes ************************/

void initActuatorStatusTracker(void);
void updateLidStatus(uint8_t);
void updateStValveStatus(uint8_t);
void updateAugerStatus(uint8_t);
void updateFlapStatus(uint8_t);
void updateHtrFanStatus(uint8_t);
void updateExhFanStatus(uint8_t);
void updateAdditiveMotorStatus(uint8_t);
void updateDcSprayerStatus(uint8_t);
void updateFlushSprayerStatus(uint8_t);
void updateAirValve1Status(uint8_t);
void updateAirValve2Status(uint8_t);
void updateAirValve3Status(uint8_t);
void updateHeaterStatus(uint8_t);

uint32_t getActuatorStatus(void);


#endif /* ACTUATORSTATUSTRACKER_H_ */
