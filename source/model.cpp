/*
 * Copyright 2019 The TensorFlow Authors. All Rights Reserved.
 * Copyright 2021-2023 NXP
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "model.h"
#include "model_data.h"
//#include "EmptyvsNonEmpty-npu.h"
#include <cr_section_macros.h>
#include "fsl_common.h"
#include "core_cm33.h"


static const tflite::Model* s_model = nullptr;
static tflite::MicroInterpreter* s_interpreter = nullptr;

extern tflite::MicroOpResolver &MODEL_GetOpsResolver();


// An area of memory to use for input, output, and intermediate arrays.
// (Can be adjusted based on the model needs.)

#ifdef TENSORARENA_NONCACHE

static uint8_t s_tensorArena[kTensorArenaSize]
    __ALIGNED(16)
    __attribute__((section("NonCacheable")));

#else

__BSS(RAM5)static uint8_t s_tensorArena[kTensorArenaSize] __ALIGNED(16);
//static uint8_t s_tensorArena[kTensorArenaSize] __ALIGNED(16);

#endif


static uint32_t s_tensorArenaSizeUsed = 0;


// ============================================================
// DWT TIMER INITIALIZATION
// ============================================================

static void Timer_Init(void)
{
    /* Enable trace/debug block */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    /* Reset cycle counter */
    DWT->CYCCNT = 0;

    /* Enable cycle counter */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

// MODEL INITIALIZATION

status_t MODEL_Init(void)
{
    /* Initialize DWT cycle counter */
    Timer_Init();


    // Map the model into a usable data structure.
    // This doesn't involve any copying or parsing,
    // it's a very lightweight operation.

    s_model = tflite::GetModel(model_data);

    if (s_model->version() != TFLITE_SCHEMA_VERSION)
    {
        printf("Model provided is schema version %d not equal "
               "to supported version %d!\r\n",
               s_model->version(),
               TFLITE_SCHEMA_VERSION);

        return kStatus_Fail;
    }


    // Pull in only the operation implementations we need.
    // This relies on a complete list of all the ops needed by this graph.

    tflite::MicroOpResolver &micro_op_resolver =
        MODEL_GetOpsResolver();


    // Build an interpreter to run the model with.

    static tflite::MicroInterpreter static_interpreter(
        s_model,
        micro_op_resolver,
        s_tensorArena,
        kTensorArenaSize);

    s_interpreter = &static_interpreter;


    // Allocate memory from the tensor_arena for the model's tensors.

    TfLiteStatus allocate_status =
        s_interpreter->AllocateTensors();

    if (allocate_status != kTfLiteOk)
    {
        printf("AllocateTensors() failed!\r\n");
        return kStatus_Fail;
    }


    // Get actual tensor arena usage.

    s_tensorArenaSizeUsed =
        s_interpreter->arena_used_bytes();


    // ========================================================
    // PRINT CLOCK INFORMATION
    // ========================================================

#if (defined(CPU_MIMXRT798SGAWAR_hifi4) || \
     defined(CPU_MIMXRT798SGFOA_hifi4))

    printf("Hifi4 DSP Frequency: %d MHz\r\n",
           CLOCK_GetFreq(kCLOCK_Hifi4CpuClk) / 1000000);

#elif (defined(CPU_MIMXRT798SGAWAR_hifi1) || \
       defined(CPU_MIMXRT798SGFOA_hifi1))

    printf("Hifi1 DSP Frequency: %d MHz\r\n",
           CLOCK_GetFreq(kCLOCK_Hifi1CpuClk) / 1000000);

#elif (defined(CPU_MIMXRT685SFAWBR_dsp) || \
       defined(CPU_MIMXRT685SFFOB_dsp) || \
       defined(CPU_MIMXRT685SFVKB_dsp) || \
       defined(CPU_MIMXRT685SVFVKB_dsp) || \
       defined(CPU_MIMXRT595SFAWC_dsp) || \
       defined(CPU_MIMXRT595SFFOC_dsp))

    printf("DSP Frequency: %d MHz\r\n",
           CLOCK_GetFreq(kCLOCK_DspCpuClk) / 1000000);

#else

    printf("Core/NPU Frequency: %d MHz\r\n",
           CLOCK_GetFreq(kCLOCK_CoreSysClk) / 1000000);

#endif


    // PRINT TENSOR ARENA INFORMATION

    printf("TensorArena Addr: 0x%x - 0x%x\r\n",
           s_tensorArena,
           s_tensorArena + kTensorArenaSize);

    printf("TensorArena Size: Total 0x%x (%d B); "
           "Used 0x%x (%d B)\r\n",
           kTensorArenaSize,
           kTensorArenaSize,
           s_tensorArenaSizeUsed,
           s_tensorArenaSizeUsed);

    // PRINT MODEL INFORMATION

    printf("Model Addr: 0x%x - 0x%x\r\n",
           model_data,
           model_data + sizeof(model_data));

    printf("Model Size: 0x%x (%d B)\r\n",
           sizeof(model_data),
           sizeof(model_data));


    printf("Total Size Used: %d B "
           "(Model (%d B) + TensorArena (%d B))\r\n",
           (sizeof(model_data) + s_tensorArenaSizeUsed),
           sizeof(model_data),
           s_tensorArenaSizeUsed);


    return kStatus_Success;
}


// ============================================================
// MODEL INFERENCE
// ============================================================

status_t MODEL_RunInference(void)
{
    uint32_t start;
    uint32_t end;

    /*
     * Read DWT cycle counter before inference.
     */
    //start = DWT->CYCCNT;


    /*
     * Actual TensorFlow Lite Micro inference.
     */
    TfLiteStatus status = s_interpreter->Invoke();


    /*
     * Read DWT cycle counter after inference.
     */
   // end = DWT->CYCCNT;


    /*
     * Calculate number of CPU cycles used.
     *
     * uint32_t is used because DWT->CYCCNT
     * is a 32-bit cycle counter.
     */
    //uint32_t cycles = end - start;


   /* printf("Invoke cycles = %lu\r\n",
           (unsigned long)cycles);

*/
    /*
     * Calculate approximate inference time in milliseconds.
     */
  /*  uint32_t frequency =
        CLOCK_GetFreq(kCLOCK_CoreSysClk);

    if (frequency != 0)
    {
        uint32_t time_ms =
            (uint32_t)(((uint64_t)cycles * 1000ULL) /
                       frequency);

        printf("Invoke time = %lu ms\r\n",
               (unsigned long)time_ms);
    }
*/

    if (status != kTfLiteOk)
    {
        printf("Invoke failed!\r\n");
        return kStatus_Fail;
    }


    return kStatus_Success;
}

// GET TENSOR DATA


uint8_t* GetTensorData(
    TfLiteTensor* tensor,
    tensor_dims_t* dims,
    tensor_type_t* type)
{
    switch (tensor->type)
    {
        case kTfLiteFloat32:
            *type = kTensorType_FLOAT32;
            break;

        case kTfLiteUInt8:
            *type = kTensorType_UINT8;
            break;

        case kTfLiteInt8:
            *type = kTensorType_INT8;
            break;

        default:
            assert("Unknown input tensor data type!\r\n");
            break;
    }


    dims->size = tensor->dims->size;

    assert(dims->size <= MAX_TENSOR_DIMS);


    for (int i = 0; i < tensor->dims->size; i++)
    {
        dims->data[i] = tensor->dims->data[i];
    }


    return tensor->data.uint8;
}

// GET INPUT TENSOR


uint8_t* MODEL_GetInputTensorData(
    tensor_dims_t* dims,
    tensor_type_t* type)
{
    TfLiteTensor* inputTensor =
        s_interpreter->input(0);

    return GetTensorData(
        inputTensor,
        dims,
        type);
}



// GET OUTPUT TENSOR


uint8_t* MODEL_GetOutputTensorData(
    tensor_dims_t* dims,
    tensor_type_t* type)
{
    TfLiteTensor* outputTensor =
        s_interpreter->output(0);

    return GetTensorData(
        outputTensor,
        dims,
        type);
}



// CONVERT INPUT
// Convert unsigned 8-bit image data to model input format in-place.

void MODEL_ConvertInput(
    uint8_t* data,
    tensor_dims_t* dims,
    tensor_type_t type)
{
    int size =
        dims->data[2] *
        dims->data[1] *
        dims->data[3];


    switch (type)
    {
        case kTensorType_UINT8:

            break;


        case kTensorType_INT8:

            for (int i = size - 1; i >= 0; i--)
            {
                reinterpret_cast<int8_t*>(data)[i] =
                    static_cast<int8_t>(data[i]) - 127;
            }

            break;


        case kTensorType_FLOAT32:

            for (int i = size - 1; i >= 0; i--)
            {
                reinterpret_cast<float*>(data)[i] =
                    (static_cast<float>(data[i]) -
                     MODEL_INPUT_MEAN) /
                    MODEL_INPUT_STD;
            }

            break;


        default:

            assert("Unknown input tensor data type!\r\n");
            break;
    }
}

// GET MODEL NAME


const char* MODEL_GetModelName(void)
{
    return MODEL_NAME;
}
