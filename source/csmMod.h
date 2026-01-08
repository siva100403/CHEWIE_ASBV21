/*
 * compostingModule.h
 *
 *  Created on: 15-Nov-2024
 *      Author: Jawahar Arumugam
 */

#ifndef CSMMOD_H_
#define CSMMOD_H_


/*******************************************************************************
 * Definitions
 ******************************************************************************/

//Composting State machine states

#define CSM_STATE_IDLE			0		//IDLE state: Chewie not doing anything
#define CSM_STATE_MPHASE		1		//MPHASE (Mesophilic phase)
#define CSM_STATE_TPHASE		2		//TPHASE (Thermophilic phase)
#define CSM_STATE_PPHASE		3		// Pathogen killing phase
#define CSM_STATE_SMPHASE		4		//SMPHASE - Short Mphase, shorter duration MPhase
#define CSM_STATE_TRANSFER		5		//Transferring content from compost chamber to Storage chamber
#define CSM_STATE_ADDWASTE_I	6		//Adding waste at IDLE state
#define CSM_STATE_ADDWASTE_M	7		//Adding waste at MPHASE (Mesophilic phase)
#define CSM_STATE_ADDWASTE_T	8		//Adding waste at TPHASE
#define CSM_STATE_ERROR			9		//Error condition where it cannot proceed with composting process




//Timeouts used by Chewie

#define ADDWASTE_DURATION_TIMEOUT		10 			//in min; This includes the dumping and shredding
#define CSM_TIMER_DEFAULT				1			// in Timerticks
#define ONE_MIN_TIMEOUT					60			//60 seconds
#define TRANFER_TIMEOUT					10			//in min; transfer timeout





typedef struct cheiweStateVar
{
	uint16_t 	schDur;		//in minutes
	uint16_t	expDur;		//in minutes. Time spent in this state so far
	uint16_t	remDur;		//in minutes. Time remaining to complete the phase
	uint8_t		curPhase;
	uint8_t 	prevPhase;
	uint8_t		curWasteCat;
	uint8_t		state;


}dgCtStateVar_t;
/*******************************************************************************
 * Function prototypes
 ******************************************************************************/

int initCSM(void);
#endif /* CSMMOD_H_ */
