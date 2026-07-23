/*
 * psramDriver.h
 *
 *  Created on: 07-Dec-2025
 *      Author: Jawahar Arumugam
 */

#ifndef PSRAMDRIVER_H_
#define PSRAMDRIVER_H_

#define PSRAM_START_ADDR		0x80000000
#define PSRAM_SIZE				0x01000000

status_t PSRAM_Init(void);
int PSRAM_Read(uint32_t addr, void *dst, size_t len);
int PSRAM_Write(uint32_t addr, const void *src, size_t len);
int PSRAM_ReadID(void* buffer );
int PSRAM_EnterQPI(void);
int PSRAM_ExitQPI(void);
int PSRAM_Write_QPI(uint32_t addr, const void *src, size_t len);
int PSRAM_Read_QPI(uint32_t addr, void *dst, size_t len);

int PSRAM_TestByteOrder(volatile uint8_t *psram);
int PSRAM_TestAccessWidth(uint32_t psramBase);



#endif /* PSRAMDRIVER_H_ */
