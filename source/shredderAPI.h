/*
 * ShredderAPI.h
 *
 *  Created on: 28-Apr-2025
 *      Author: Anusha
 */

#ifndef SHREDDERAPI_H_
#define SHREDDERAPI_H_


int initShd(void);
int event_lid_close(uint8_t);
int event_lid_open(uint8_t);
int event_lid_aiinferencecomplete(uint8_t infresult);
int event_lid_inbetween(uint8_t srcModule);
int event_ls_flapclose(uint8_t srcModule);
int shdFlapSync(uint8_t srcModule);

#endif /* SHREDDERAPI_H_ */
