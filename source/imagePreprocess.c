/*
 * imagePreprocess.c
 *
 *  Created on: 16-Jul-2026
 */


#include "imagePreprocess.h"

#define CROP_X_96X96 				16U
#define CROP_Y_96X96 				96U

#define CROP_X_128X128              32U
#define CROP_Y_128_128              112U

static inline void AccumulateRgb565(
    uint16_t pixel,
    uint32_t *red_sum,
    uint32_t *green_sum,
    uint32_t *blue_sum)
{
    /*
     * RGB565 format:
     *
     * Bits 15:11 = Red   (5 bits)
     * Bits 10:5  = Green (6 bits)
     * Bits 4:0   = Blue  (5 bits)
     */

	//swap bytes
	uint16_t temp = (pixel<<8)&0xFF00;
	uint16_t temp1 = (pixel>>8)&0x00FF;
	pixel = temp | temp1;
    const uint32_t red5 = ((uint32_t)pixel >> 11U) & 0x1FU;
    const uint32_t green6 = ((uint32_t)pixel >> 5U) & 0x3FU;
    const uint32_t blue5 = (uint32_t)pixel & 0x1FU;

/*	const uint32_t blue5 = ((uint32_t)pixel >> 11U) & 0x1FU;
	const uint32_t green6 = ((uint32_t)pixel >> 5U) & 0x3FU;
	const uint32_t red5 = (uint32_t)pixel & 0x1FU;
*/
    /*
     * Convert RGB565 channel values to RGB888
     * before accumulating.
     *
     * 5-bit -> 8-bit:
     * abcde -> abcdeabc
     *
     * 6-bit -> 8-bit:
     * abcdef -> abcdefab
     */

    *red_sum += (red5 << 3U) | (red5 >> 2U);
    *green_sum += (green6 << 2U) | (green6 >> 4U);
    *blue_sum += (blue5 << 3U) | (blue5 >> 2U);
}


void Image_CropResizeRgb565ToRgb888_128X128(const uint16_t *restrict src, uint8_t *restrict dst)
{
    uint32_t out_y;

    /*
     * Each output pixel represents the average
     * of one 2 x 2 source pixel block.
     */

    for (out_y = 0U; out_y < IMAGE_DST_HEIGHT_128X128; ++out_y)
    {
        uint32_t out_x;

        /*
         * Find the first source row for this
         * 2 x 2 block.
         *
         * Crop begins at Y = 112.
         *
         * Output row 0 -> source rows 112, 113
         * Output row 1 -> source rows 114, 115
         * ...
         */

        const uint32_t source_y = CROP_Y_128_128 + (out_y * 2U);
        const uint16_t *row0 = &src[(source_y + 0U) * IMAGE_SRC_WIDTH];
        const uint16_t *row1 = &src[(source_y + 1U) * IMAGE_SRC_WIDTH];

        for (out_x = 0U; out_x < IMAGE_DST_WIDTH_128X128; ++out_x)
        {
            /*
             * Find first source column for
             * this 2 x 2 block.
             *
             * Crop begins at X = 32.
             */

            const uint32_t source_x = CROP_X_128X128 + (out_x * 2U);

            uint32_t red_sum   = 0U;
            uint32_t green_sum = 0U;
            uint32_t blue_sum  = 0U;

            /*
             * Process:
             *
             * P00 P01
             * P10 P11
             */

            AccumulateRgb565(row0[source_x + 0U], &red_sum, &green_sum, &blue_sum);
            AccumulateRgb565(row0[source_x + 1U], &red_sum, &green_sum, &blue_sum);
            AccumulateRgb565(row1[source_x + 0U], &red_sum, &green_sum, &blue_sum);
            AccumulateRgb565(row1[source_x + 1U], &red_sum, &green_sum, &blue_sum);

            /*
             * Average four pixels.
             *
             * Add 2 before division by 4
             * for round-to-nearest.
             *
             * Since division is by 4, use >> 2.
             */

            *dst++ = (uint8_t)((red_sum + 2U) >> 2U);
            *dst++ = (uint8_t)((green_sum + 2U) >> 2U);
            *dst++ = (uint8_t)((blue_sum + 2U) >> 2U);
        }
    }
}


void Image_CropResizeRgb565ToRgb888_96X96(const uint16_t *restrict src, uint8_t *restrict dst)
{
    uint32_t out_y;

    for (out_y = 0U; out_y < 96U; ++out_y)
    {
        uint32_t out_x;

        const uint32_t source_y = CROP_Y_96X96 + (out_y * 3U);

        const uint16_t *row0 = &src[(source_y + 0U) * IMAGE_SRC_WIDTH];
        const uint16_t *row1 = &src[(source_y + 1U) * IMAGE_SRC_WIDTH];
        const uint16_t *row2 = &src[(source_y + 2U) * IMAGE_SRC_WIDTH];

        for (out_x = 0U; out_x < 96U; ++out_x)
        {
            const uint32_t source_x = CROP_X_96X96 + (out_x * 3U);
            uint32_t red_sum   = 0U;
            uint32_t green_sum = 0U;
            uint32_t blue_sum  = 0U;

            AccumulateRgb565(row0[source_x + 0U], &red_sum, &green_sum, &blue_sum);

            AccumulateRgb565(row0[source_x + 1U], &red_sum, &green_sum, &blue_sum);

            AccumulateRgb565(row0[source_x + 2U], &red_sum, &green_sum, &blue_sum);

            AccumulateRgb565(row1[source_x + 0U],  &red_sum, &green_sum, &blue_sum);

            AccumulateRgb565(row1[source_x + 1U], &red_sum, &green_sum, &blue_sum);

            AccumulateRgb565(row1[source_x + 2U], &red_sum, &green_sum, &blue_sum);

            AccumulateRgb565(row2[source_x + 0U], &red_sum, &green_sum, &blue_sum);

            AccumulateRgb565(row2[source_x + 1U], &red_sum, &green_sum, &blue_sum);

            AccumulateRgb565(row2[source_x + 2U], &red_sum, &green_sum, &blue_sum);


            *dst++ = (uint8_t)((red_sum   + 4U) / 9U);
            *dst++ = (uint8_t)((green_sum + 4U) / 9U);
            *dst++ = (uint8_t)((blue_sum  + 4U) / 9U);
        }
    }
}
