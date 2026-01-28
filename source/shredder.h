/*
 * shredder.h
 *
 *  Created on: 22-Jun-2025
 *      Author: Jawahar Arumugam
 */

#ifndef SHREDDER_H_
#define SHREDDER_H_


/***************************** Shredder States *******************************/
#define SHD_STATE_IDLE				0     	//Default state after powerup
#define SHD_STATE_STARTDELAY		1
#define SHD_STATE_ACTIVE			2
#define SHD_STATE_WAITFORCLOSE		3
#define SHD_STATE_FLAPSYNC			4



#define SHD_START_DELAY_DEFAULT 		5   	//Secs. Shredding start delay after Lid close
#define FLAP_OPEN_DURATION_DEFAULT  	4   	//mSec. Flap open duration
#define FLAP_CLOSE_DURATION_DEFAULT   	4   	//mSec. Flap open duration



#endif /* SHREDDER_H_ */
