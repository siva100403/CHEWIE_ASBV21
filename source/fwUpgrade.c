/*
 * fwUpgrade.c
 *
 *  Created on: 19-May-2026
 *      Author: Jawahar Arumugam
 */


//#include "fsl_debug_console.h"
#include "fsl_common.h"
#include "pin_mux.h"
#include "board.h"
#include "fsl_runbootloader.h"

/* Standard C includes */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"

#include "dgCommon.h"
#include "modulecom.h"
#include "csmModAPI.h"



/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define BOOT_ARG_TAG (0xEBu)

/* @brief Boot interface can be selected by user application
 * @note  These interfaces are invalid for ISP boot
 */
enum
{
    kUserAppBootPeripheral_FLASH   = 0u,
    kUserAppBootPeripheral_ISP     = 1u,
    kUserAppBootPeripheral_FLEXSPI = 2u,
    kUserAppBootPeripheral_AUTO    = 3u,
};

/* @brief Boot mode can be selected by user application
 */
enum
{
    kUserAppBootMode_MasterBoot = 0U,
    kUserAppBootMode_IspBoot    = 1U,
};

/* @brief ISP Peripheral definitions
 * @note  For ISP boot, valid boot interfaces for user application are USART I2C SPI USB-HID CAN
 */
//! ISP Peripheral definitions
enum isp_peripheral_constants
{
    kIspPeripheral_Auto     = 0,
    kIspPeripheral_UsbHid   = 1,
    kIspPeripheral_Uart     = 2,
    kIspPeripheral_SpiSlave = 3,
    kIspPeripheral_I2cSlave = 4,
    kIspPeripheral_Can      = 5,
};
/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

volatile uint32_t g_systickCounter;

/*******************************************************************************
 * Code
 ******************************************************************************/


void startIspMode(void)
{


    user_app_boot_invoke_option_t arg = {.option = {.B = {
                                                        .tag            = BOOT_ARG_TAG,
                                                        .mode           = kUserAppBootMode_IspBoot,
                                                        .boot_interface = kIspPeripheral_Auto,
                                                    }}}; // EB: represents Enter Boot; 12: represents enter ISP mode by
                                                         // UART only,13: represents enter ISP mode by SPI only

 /*
    if (arg.option.B.tag != BOOT_ARG_TAG)
    {
        printf("The runBootloader API arg is Invalid...\n");
        error_trap();
    }
    if (arg.option.B.mode == kUserAppBootMode_IspBoot)
    {
        printf("Calling the runBootloader API to force into the ISP mode: %x...\n", arg.option.B.boot_interface);
        printf("The runBootloader ISP interface is choosen from the following one:\n");
        printf("kIspPeripheral_Auto :     0\n");
        printf("kIspPeripheral_UsbHid :   1\n");
        printf("kIspPeripheral_Uart :     2\n");
        printf("kIspPeripheral_SpiSlave : 3\n");
        printf("kIspPeripheral_I2cSlave : 4\n");
        printf("kIspPeripheral_Can :      5\n");
    }
    else
    {
        printf("Not Calling the runBootloader API to force into the ISP mode\n");
    }
*/

    printf("Call the runBootloader API based on the arg : %x...\n", arg);
    bootloader_user_entry(&arg);

    while (1)
    {
    }
}

int safeStateForFwUpgrade(void)
{
	if(csmStop(UNKNOWN)!= DG_SUCCESS)
	{
		printf("fwUpgrade.c:safeStateForFwUpgrade():csmStop failed\r\n");
		return DG_FAIL;
	}
	taskENTER_CRITICAL();
	__disable_irq();
	SysTick->CTRL = 0;
	return DG_SUCCESS;
}
