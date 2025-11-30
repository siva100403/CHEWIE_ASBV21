/*
 * pin_mux_local.c
 *
 *  Created on: 13-Aug-2025
 *      Author: Jawahar Arumugam
 */


#include "fsl_common.h"
#include "fsl_port.h"
#include "fsl_gpio.h"
#include "pin_mux.h"
#include "pin_mux_local.h"

/**********************************************************************
 * This file contains the workaround code for GPIO_PinInit() for PORT5
 */

void GPIO_PinInit_Port5(GPIO_Type *base, uint32_t pin, const gpio_pin_config_t *config)
{
    assert(NULL != config);

//    GPIO_PortClockEnable(base, true);



    if (config->pinDirection == kGPIO_DigitalInput)
    {
        base->PDDR &= GPIO_FIT_REG(~(1UL << pin));
    }
    else
    {
        GPIO_PinWrite(base, pin, config->outputLogic);
        base->PDDR |= GPIO_FIT_REG((1UL << pin));
    }
}
