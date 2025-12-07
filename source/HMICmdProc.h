/*
 * HMICmdProc.h
 *
 *  Created on: 05-May-2025
 *      Author: Jawahar Arumugam
 */

#ifndef HMICMDPROC_H_
#define HMICMDPROC_H_

/*************************** HMI Command Processor states ********************/
#define HMICMDPROC_STATE_IDLE					0
#define HMICMDPROC_STATE_WAIT_FOR_CMD			1
#define HMICMDPROC_STATE_PROCESSING				2
#define HMICMDPROC_STATE_SENDING_RESPONSE		3


/******************************** Function Prototypes ************************/

int initHMICmdProc(void);

#endif /* HMICMDPROC_H_ */
