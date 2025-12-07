/*
 * psramDriver.h
 *
 *  Created on: 07-Dec-2025
 *      Author: rahaw
 */

#ifndef PSRAMDRIVER_H_
#define PSRAMDRIVER_H_



status_t PSRAM_Init(void);
int PSRAM_Read(uint32_t addr, void *dst, size_t len);
int PSRAM_Write(uint32_t addr, const void *src, size_t len);
int PSRAM_ReadID(void* buffer );

#endif /* PSRAMDRIVER_H_ */
