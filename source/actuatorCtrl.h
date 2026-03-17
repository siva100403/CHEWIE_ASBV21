/*
 * actuatorCtrl.h
 *
 *  Created on: 24-Aug-2025
 *      Author: Jawahar Arumugam
 */

#ifndef ACTUATORCTRL_H_
#define ACTUATORCTRL_H_



int additiveDispenseOn();
int additiveDispenseOff();

int DCsprayerOn();
int DCsprayerOff();

int flushSprayerOn();
int flushSprayerOff();

int fanMotorCWR();
int fanMotorCCWR();
int fanMotorStop();


int airValve1On();
int airValve1Off();

int airValve2On();
int airValve2Off();

int airValve3On();
int airValve3Off();

//int augerMotorCWR();
//int augerMotorCCWR();
//int augerMotorStop();

int flapMotorCWR();
int flapMotorCCWR();
int flapMotorStop();

//int fanMotorCWR();
//int fanMotorCCWR();
//int fanMotorStop();

int shdAugMotorCWR();
int shdAugMotorCCWR();
int shdAugMotorStop();
int shdAugMotorSetspeed(uint8_t speed);





#endif /* ACTUATORCTRL_H_ */
