/*
 * lidModuleAPI.h
 *
 *  Created on: 16-May-2025
 *      Author: Jawahar Arumugam
 */

#ifndef LIDMODULEAPI_H_
#define LIDMODULEAPI_H_


int sendLidSwitchEvent(uint8_t switchEvent);
int sendProximityEvent(dgProximityEvents_t *eventPtr);

#endif /* LIDMODULEAPI_H_ */
