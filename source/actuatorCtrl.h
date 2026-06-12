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

/*
int flapMotorCWR();
int flapMotorCCWR();
int flapMotorStop();
*/

int stMotorCWR(void);
int stMotorCCWR(void);
int stMotorStop(void);

int mkValveCWR();
int mkValveCCWR();
int mkValveStop();



#endif /* ACTUATORCTRL_H_ */
