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
#define TCS_STATE_TRANSFERRING	1
#define TCS_ERROR				2

#define TCS_TIMER_DEFAULT		100





int initTCS(void);

#endif /* TRANSFERCS_H_ */
