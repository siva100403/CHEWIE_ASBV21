/*
 * imagePreprocess.h
 *
 *  Created on: 16-Jul-2026

 */

#ifndef IMAGEPREPROCESS_H_
#define IMAGEPREPROCESS_H_

#ifndef IMAGE_RESIZE_H
#define IMAGE_RESIZE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IMAGE_SRC_WIDTH          	320U
#define IMAGE_SRC_HEIGHT         	480U

#define IMAGE_CROP_WIDTH_96X96	    288U
#define IMAGE_CROP_HEIGHT_96X96     288U
#define IMAGE_CROP_WIDTH_128X128    256U
#define IMAGE_CROP_HEIGHT_128X128   256U

#define IMAGE_DST_WIDTH_96X96       96U
#define IMAGE_DST_HEIGHT_96X96      96U
#define IMAGE_DST_WIDTH_128X128     128U
#define IMAGE_DST_HEIGHT_128X128    128U

#define IMAGE_RESIZE_FACTOR_96X96   3U
#define IMAGE_RESIZE_FACTOR_128X128 2U


#define IMAGE_RGB888_CHANNELS    	3U

#define IMAGE_SRC_PIXEL_COUNT    		(IMAGE_SRC_WIDTH * IMAGE_SRC_HEIGHT)
#define IMAGE_DST_PIXEL_COUNT_96X96    (IMAGE_DST_WIDTH_96X96 * IMAGE_DST_HEIGHT_96X96)
#define IMAGE_DST_SIZE_BYTES_96X96     (IMAGE_DST_PIXEL_COUNT_96X96 * IMAGE_RGB888_CHANNELS)


/**
 * @brief Crop, convert and resize an RGB565 image.
 *
 * The function:
 * 1. Selects the centered 288 x 288 region from a 320 x 480 image.
 * 2. Converts RGB565 pixels to RGB888.
 * 3. Averages each non-overlapping 3 x 3 block.
 * 4. Produces a 96 x 96 RGB888 image.
 *
 * Source layout:
 * - Row-major
 * - Top-left origin
 * - One uint16_t per RGB565 pixel
 *
 * Destination layout:
 * - Row-major
 * - RGB byte order: R, G, B
 *
 * @param src_rgb565 Pointer to 320 x 480 RGB565 image.
 * @param dst_rgb888 Pointer to at least IMAGE_DST_SIZE_BYTES bytes.
 *
 * @return IMAGE_STATUS_SUCCESS or IMAGE_STATUS_INVALID_ARGUMENT.
 */
void Image_CropResizeRgb565ToRgb888_96X96(const uint16_t *src_rgb565, uint8_t *dst_rgb888);


void Image_CropResizeRgb565ToRgb888_128X128(const uint16_t *src_rgb565, uint8_t *dst_rgb888);

#ifdef __cplusplus
}
#endif

#endif /* IMAGE_RESIZE_H */

#endif /* IMAGEPREPROCESS_H_ */
