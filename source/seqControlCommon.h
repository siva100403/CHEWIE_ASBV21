/*
 * seqControlCommon.h
 *
 *  Created on: 15-Aug-2025
 *      Author: Jawahar Arumugam
 */

#ifndef SEQCONTROLCOMMON_H_
#define SEQCONTROLCOMMON_H_

/*********** Sequence Execution engine control codes *************/

//Seq execution engine commands
#define SEQ_ENGINE_START			0
#define SEQ_ENGINE_CONTINUE			1
#define SEQ_ENGINE_STOP				2

//Sequence Control codes to control the flow during execution

#define SEQ_CTRL_START			0
#define SEQ_CTRL_MID			1
#define SEQ_CTRL_END			2

/******** Device control codes used by seq execution engine ******/

//CT Motor Control Codes
#define SEQ_CTRL_CTMOTOR_OFF	0
#define SEQ_CTRL_CTMOTOR_CCW	1
#define SEQ_CTRL_CTMOTOR_CCWR	2

//Sprayer Control Codes
#define SEQ_CTRL_SPRAYER_OFF	0
#define SEQ_CTRL_SPRAYER_ONCE	1

//Flush Sprayer Control Codes
#define SEQ_CTRL_FLUSH_OFF		0
#define SEQ_CTRL_FLUSH_ON		1

//ST Motor control codes
#define SEQ_CTRL_STMOTOR_OFF	0
#define SEQ_CTRL_STMOTOR_ON		1

//Fan Control Codes
#define SEQ_CTRL_FAN_OFF		0
#define SEQ_CTRL_FAN_CWR		1
#define SEQ_CTRL_FAN_CCWR		2

//Heater Control codes
#define SEQ_CTRL_HTR_OFF		0
#define SEQ_CTRL_HTR_ON			1

//Shredder Motor Control Codes
#define SEQ_CTRL_SHD_MOTOR_OFF		0
#define SEQ_CTRL_SHD_MOTOR_CWR		1
#define SEQ_CTRL_SHD_MOTOR_CCWR		2

//Shredder Auger Motor Control Codes
#define SEQ_CTRL_SHDAUG_MOTOR_OFF		0
#define SEQ_CTRL_SHDAUG_MOTOR_CWR		1
#define SEQ_CTRL_SHDAUG_MOTOR_CCWR		2

//Shredder Flap Control Codes
#define SEQ_CTRL_SHD_FLAP_OFF		0
#define SEQ_CTRL_SHD_FLAP_OPEN		1
#define SEQ_CTRL_SHD_FLAP_CLOSE		2

//Air Circulation control within Chewie
#define SEQ_CTRL_AIR_OFF			0			//S1-OFF, S2-NA, S3 - OFF
#define SEQ_CTRL_AIR_IN				1			//S1-ON,  S2-NA, S3 - OFF
#define SEQ_CTRL_AIR_OUT			2			//S1-ON,  S2-NA, S3 - ON
#define SEQ_CTRL_AIR_RECIRC			3			//S1-OFF, S2-NA, S3 - ON

/******************** CS Control Code used by Seq Engine ******************/
//CTCS Control Codes
#define SEQ_CTRL_CTCS_OFF		0
#define SEQ_CTRL_CTCS_ON		1
#define SEQ_CTRL_CTCS_ONCE		2


#endif /* SEQCONTROLCOMMON_H_ */
