/*
 * hatcsModAPI.h
 *
 *  Created on: 31-Dec-2024
 *      Author: Jawahar Arumugam
 */

#ifndef HATCSMODAPI_H_
#define HATCSMODAPI_H_


/************Temperature & Humidity Control System API***************
 * This file contains the API implemention to configure and control *
 * the HAT Control system		 									*
 ********************************************************************/


/********************************************************************
 * Function prototypes												*
 ********************************************************************/
int hatcsStart(uint8_t srcModule);
int hatcsStop(uint8_t srcModule);
int hatcsNotifySensorState(uint8_t srcModule, uint8_t sensorState);
int hatcsInstructAirInlet(void);
//int hatcsInstructMixing(void);



/********************************************************************
 * Data structures													*
 ********************************************************************/






#endif /* HATCSMODAPI_H_ */
