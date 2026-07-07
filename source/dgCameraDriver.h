/*
 * dgCameraDriver.h
 *
 *  Created on: 21-Sep-2025
 *      Author: rahaw
 */

#ifndef DGCAMERADRIVER_H_
#define DGCAMERADRIVER_H_

#define DEMO_BUFFER_WIDTH  480U
#define DEMO_BUFFER_HEIGHT 320U

int initCamera(void);
int captureImage(void);
int getImagePart(uint8_t *payload,uint16_t startByte,uint8_t size );

#endif /* DGCAMERADRIVER_H_ */
