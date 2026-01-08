/*
 * lidModule.h
 *
 *  Created on: 15-May-2025
 *      Author: Jawahar Arumugam
 */

#ifndef LIDMODULE_H_
#define LIDMODULE_H_

/************************ LID Module Constants ********************/
#define PROXIMITY_LIDCLOSE_TIMEOUT		15 			//in Seconds
#define NONPROXIMITY_LIDCLOSE_TIMEOUT	8			//in seconds
#define LIDOPENING_DURATION				4000			//in mSec
#define LIDCLOSING_DURATION				4000			//in mSec
#define LID_OPEN_DELAY					20			//in Ticks (100 mSec)
#define LID_OPEN_WAIT					1000			//in mSec Time: To ensure motor goes to OFF before reversal
#define LID_CLOSE_WAIT					1000			//in mSec: To ensure motor goes to OFF before reversal


//Lid Module states
#define LIDMOD_STATE_IDLE			0			//Module is initialized and not started
#define LIDMOD_STATE_OPEN			1			//Lid is in OPEN condition
#define LIDMOD_STATE_CLOSE			2			//Lid is in CLOSED condition
#define LIDMOD_STATE_OPENING		3			//Lid is currently opening. Motor ON
#define LIDMOD_STATE_CLOSING		4			//Lid is currently closing. Motor ON
#define LIDMOD_STATE_OPEN_WAIT		5			//To ensure motor goes to OFF before reversing
#define LIDMOD_STATE_CLOSE_WAIT		6			//To ensure motor goes to OFF before reversing
#define LIDMOD_STATE_OPEN_DELAY		7			//Lid closed just now. Hence delay is required to OPEN
#define LIDMOD_STATE_READY			8			//After MODULE_START it goes to READY state
#define LIDMOD_STATE_ERROR			9


/************************ Function Prototypes ********************/

int initLidModule(void);

#endif /* LIDMODULE_H_ */
