/*
 * CTurnerAPI.h
 *
 *  Created on: 26-Aug-2024
 *      Author: Jawahar Arumugam
 */

#ifndef AUGERAPI_H_
#define AUGERAPI_H_

/************************************************************************
 *                 			   Definitions								*
 ************************************************************************/







/************************************************************************
 *                 			Function Prototypes							*
 ************************************************************************/

int initCt(void);
int ctStop(uint8_t srcModule);
int ctStart(uint8_t srcModule);
#endif /* AUGERAPI_H_ */
