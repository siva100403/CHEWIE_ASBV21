/*
 * transferCS.h
 *
 *  Created on: 31-Dec-2024
 *      Author: Jawahar Arumugam
 */

#ifndef TRANSFERCS_H_
#define TRANSFERCS_H_




/*******************Constants**********************************/

//TCS states
#define TCS_STATE_IDLE			0
#define TCS_STATE_OPENING		1
#define TCS_STATE_TRANSFERRING	2
#define TCS_STATE_CLOSING		3
#define TCS_STATE_ERROR			4


//Control System parameters
//#define STV_OPEN_DURATION		600  	// in mSec
//#define STV_CLOSE_DURATION		600  	// in mSec
#define TRANSFER_DUR_DEFAULT	40		// in Seconds
#define TRANSFER_DUR_MAX		600 	// in Seconds




int initTCS(void);

#endif /* TRANSFERCS_H_ */
