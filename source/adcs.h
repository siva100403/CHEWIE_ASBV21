/*
 * ADCS.h
 *
 *  Created on: 06-Sep-2025
 *      Author: Jawahar Arumugam
 */

#ifndef ADCS_H_
#define ADCS_H_


/******************************* ADCS states *********************************/
#define ADCS_STATE_IDLE				0     	//Default state after powerup
#define ADCS_STATE_DELIVERING		1		//Currently delivering additives
#define ADCS_STATE_QUIET_PERIOD		2		//Period during which Additives should not be delivered eventhough waste has been added


int adcsStart(uint8_t srcModule);
int adcsAbort(uint8_t srcModule);
int initAdcs(void);

#endif /* ADCS_H_ */
