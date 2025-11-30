/*
 * dgCommon.h
 *
 *  Created on: 24-May-2024
 *      Author: Jawahar Arumugam
 */

#ifndef DGCOMMON_H_
#define DGCOMMON_H_

/********************Constant Definitions **********************/

#define task_PRIORITY (configMAX_PRIORITIES - 2)
#define TIMERTASK_PRIORITY  (configMAX_PRIORITIES - 1)

/********************Return values **********************/


#define DG_INVALID_PARAM	-2
#define DG_FAIL				-1
#define DG_SUCCESS			0
#define DG_INPROGRESS		1
#define DG_INVALID_CMD		2
#define DG_BUSY				3
#define DG_INVALID_STATE	4
#define DG_ACTION_COMPLETE 	5
#define DG_CHECKSUM_FAIL	6



#define DG_BOOL_TRUE		1
#define DG_BOOL_FALSE		0

#define DG_CSENABLE			1
#define DG_CSDISABLE		0

//Pin direction
#define DG_PIN_INPUT 		0
#define DG_PIN_OUTPUT		1

#endif /* DGCOMMON_H_ */
