/*
 * compostingModAPI.h
 *
 *  Created on: 24-Dec-2024
 *      Author: Jawahar Arumugam
 */

#ifndef CSMMODAPI_H_
#define CSMMODAPI_H_


/********************************************************************
 * Data structures													*
 ********************************************************************/
//This structure will be used to pass parameters to composting module.

typedef struct csmParam
{
	uint8_t		wasteCat;           		//Set waste category - This will be set after the AI model analysis
	uint8_t		phase;						//Composting phase - This will be set after power fail recovery
	uint16_t 	remDur;
	uint8_t		state;						//csmState
} dgCsmParam_t;



int csmStart(uint8_t srcModule, uint8_t state, uint8_t phase, uint16_t remainingDuration, uint8_t wasteCat);
int csmStop(uint8_t srcModule);
//int csmSetWasteCat(uint8_t srcModule, uint8_t wasteCat);
int csmGetState(uint8_t srcModule, dgCsmParam_t *state);
int csmNotifyWasteAddStart(uint8_t srcModule);
int csmNotifyWasteAddEnd(uint8_t srcModule, uint8_t wasteCat);
int csmNotifyTransferComplete(uint8_t srcModule);


#endif /* CSMMODAPI_H_ */
