/*
 * Copyright 2020-2022 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "output_postproc.h"
#include "get_top_n.h"
#include "stdio.h"
#include "labels.h"

#define DETECTION_TRESHOLD 23
#define NUM_RESULTS 1


status_t MODEL_ProcessOutput(
    const uint8_t* data,
    const tensor_dims_t* dims,
    tensor_type_t type,
    int inferenceTime)
{
    const float threshold =(float)DETECTION_TRESHOLD / 100;
    result_t topResults[NUM_RESULTS];
    const char* label ="No label detected";

    /* Find best label candidates. */
    MODEL_GetTopN(data,dims->data[dims->size - 1],type,NUM_RESULTS,threshold, topResults);

    float confidence = 0;

    if (topResults[0].index >= 0)
    {
        auto result = topResults[0];
        confidence = result.score;
        int index = result.index;

        if (confidence * 100 > DETECTION_TRESHOLD)
        {
            label = labels[index];
        }
    }

    int score =
        (int)(confidence * 100);


    printf("----------------------------------------\r\n");
    printf("     Detected: %s (%d%%)\r\n",label,score);
    //printf("     Inference time: %d ms\r\n",inferenceTime);
    printf("----------------------------------------\r\n");

    return kStatus_Success;
}
