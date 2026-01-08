/*
 * transferCSAPI.h
 *
 *  Created on: 31-Dec-2024
 *      Author: Jawahar Arumugam
 */

#ifndef TRANSFERCSAPI_H_
#define TRANSFERCSAPI_H_



int transferStart(uint8_t srcModule);
int transferAbort(uint8_t srcModule);
int checkTransferFeasibility(void);
int event_ls_stvalveClose(uint8_t srcModule);

#endif /* TRANSFERCSAPI_H_ */
