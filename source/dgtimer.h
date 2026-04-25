/*
 * dgtimer.h
 *
 *  Created on: 30-May-2024
 *      Author: Jawahar Arumugam
 */

#ifndef DGTIMER_H_
#define DGTIMER_H_



/*******************Constants**********************************/
#define DGTIMER_BLOCKTIME	20 / portTICK_PERIOD_MS

/*******************Macros*********************************/

#define CONV_SEC_TO_TICKS(x)	x*1000 / portTICK_PERIOD_MS
#define CONV_MSEC_TO_TICKS(x)	x / portTICK_PERIOD_MS

/************************Function Prototypes*******************/

int dgtimer_init();
int dgtimerCreate();
//
int dgtimerStart(uint8_t moduleid, TickType_t timeoutValue);
int dgtimerStop(uint8_t moduleid);
int dgtimerReset();
int dgtimerDestroy();
void dgTimerCallback( TimerHandle_t xTimer);


#endif /* DGTIMER_H_ */
