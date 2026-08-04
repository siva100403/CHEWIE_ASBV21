/*
 * inference.h
 *
 *  Created on: 10-Jul-2026
 *      Author: Jawahar Arumugam
 */

#ifndef INFERENCE_H_
#define INFERENCE_H


/*------------------ Inference Module States----------------*/
#define INFMOD_STATE_IDLE		0
#define INFMOD_STATE_READY		1
#define INFMOD_STATE_INFERENCE	2
#define INFMOD_STATE_ERROR		3


int initInferenceModule(void);
int doInference();
int getImageData128X128(char *buffer, int pixelOffset, uint8_t size);
int getImageData96X96(char *buffer, int pixelOffset, uint8_t size);

#endif /* INFERENCE_H_ */
