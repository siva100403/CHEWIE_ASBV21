/*
 * image_decode_raw.h
 *
 *  Created on: 15-Jul-2026
 *      Author: rahaw
 */

#ifndef IMAGE_DECODE_RAW_H_
#define IMAGE_DECODE_RAW_H_

#include "fsl_common.h"
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus*/

/*******************************************************************************
 * Definitions
 ******************************************************************************/

status_t IMAGE_Decode(const uint8_t* srcData, uint8_t* dstData,
                      int32_t dstWidth, int32_t dstHeight, int32_t dstChannels);

#if defined(__cplusplus)
}
#endif /* __cplusplus*/

#endif /* IMAGE_DECODE_RAW_H_ */
