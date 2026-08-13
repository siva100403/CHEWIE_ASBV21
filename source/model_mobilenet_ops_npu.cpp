/*
 * Copyright 2022-2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/kernels/neutron/neutron.h"

tflite::MicroOpResolver &MODEL_GetOpsResolver()
{
    static tflite::MicroMutableOpResolver<9> s_microOpResolver;

    s_microOpResolver.AddConv2D();
    s_microOpResolver.AddDepthwiseConv2D();
    s_microOpResolver.AddAdd();
    s_microOpResolver.AddPad();
    s_microOpResolver.AddMean();
    s_microOpResolver.AddFullyConnected();
    s_microOpResolver.AddSoftmax();
    s_microOpResolver.AddDequantize();
    s_microOpResolver.AddReshape();

    return s_microOpResolver;
}
