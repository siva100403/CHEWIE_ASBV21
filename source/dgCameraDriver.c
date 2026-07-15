/*
 * dgCameraDriver.c
 *
 *  Created on: 20-Sep-2025
 *      Author: Jawahar Arumugam
 */

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"

/* Standard C includes */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* NXP includes */
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
//#include "fsl_debug_console.h"
#include "fsl_spc.h"
#include "fsl_lpi2c.h"
#include "fsl_lpuart.h"
#include "fsl_device_registers.h"
#include "fsl_lpspi.h"
#include "fsl_utick.h"
#include "fsl_flexspi.h"
#include "fsl_gpio.h"
#include "fsl_smartdma.h"
#include "fsl_smartdma_mcxn.h"
#include "fsl_smartdma_prv.h"
#include "fsl_inputmux.h"
#include "fsl_ov7670.h"
#include "fsl_clock.h"
#include "fsl_spc.h"
#include "fsl_lpi2c.h"
#include <cr_section_macros.h>
//#include "fsl_camera_device.h"
//#include "fsl_video_common.h"

/* Chewie Includes */
/* Chewie Includes */
#include "dgCommon.h"
#include "version.h"
#include "modulecom.h"
#include "psramDriver.h"
#include "dgtimer.h"
#include "GPIOSignals.h"

#include "dgI2cDriver.h"
#include "eeConfig.h"

#include "sysStart.h"

#include "mclsSPIDriver.h"

#include "dgCameraDriver.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/


#define DEMO_CAMERA_RESOLUTION kVIDEO_ResolutionVGA /* 640*480 */
#define DEMO_SMARTDMA_API kSMARTDMA_CameraWholeFrame320_480

#define DEMO_BUFFER_WIDTH  480U
#define DEMO_BUFFER_HEIGHT 320U


/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

/* Camera resource and handle */
const ov7670_resource_t resource = {
    .i2cSendFunc    = BOARD_Camera_I2C_SendSCCB,
    .i2cReceiveFunc = BOARD_Camera_I2C_ReceiveSCCB,
    .xclock         = kOV7670_InputClock24MHZ,
};

camera_device_handle_t handle = {
    .resource = (void *)&resource,
    .ops      = &ov7670_ops,
};

smartdma_camera_param_t smartdmaParam;
static volatile bool g_camera_complete_flag = false;
//static uint16_t g_camera_buffer[DEMO_BUFFER_WIDTH * DEMO_BUFFER_HEIGHT];
uint16_t *g_camera_buffer;
//__BSS(RAM5) static uint16_t g_camera_buffer[DEMO_BUFFER_WIDTH * DEMO_BUFFER_HEIGHT];
volatile uint8_t g_samrtdma_stack[64] = {0};
/*******************************************************************************
 * Code
 ******************************************************************************/


static void SDMA_CompleteCallback(void *param)
{
	static uint8_t count=0;

    count++;

    GREEN_LED_TOGGLE();

/*    if((g_camera_buffer[0]!= 0)|| (g_camera_buffer[1]!= 0) || (g_camera_buffer[2]!= 0))
    {
    	RED_LED_TOGGLE();
    }*/
    if(count> 2)
    {
        g_camera_complete_flag = true;
        SMARTDMA_Reset();
        count =0;
    }

}




static int DEMO_InitCamera(void)
{
	status_t status;
    /* Init ov7670 module with default setting. */
    camera_config_t camconfig = {
        .pixelFormat                = kVIDEO_PixelFormatRGB565,
        //.pixelFormat                = kVIDEO_PixelFormatYUYV,
        .resolution                 = DEMO_CAMERA_RESOLUTION,
        .framePerSec                = 30,   //changing from 30 to 14
        .interface                  = kCAMERA_InterfaceGatedClock,
        .frameBufferLinePitch_Bytes = 0, /* Not used. */
        .controlFlags               = 0, /* Not used. */
        .bytesPerPixel              = 0, /* Not used. */
        .mipiChannel                = 0, /* Not used. */
        .csiLanes                   = 0, /* Not used. */
    };

   // BOARD_Camera_I2C_Init();
    status = CAMERA_DEVICE_Init(&handle, &camconfig);
    if(status== kStatus_Success)
    {
    	printf("dgCameraDriver.c:Camera Device Init success\r\n");
    	return DG_SUCCESS;
    }
    else
    {
    	printf("dgCameraDriver.c:Camera Device Init fail code=%d\r\n",status);
    	return DG_FAIL;
    }
}


static void DEMO_InitSmartDma(void)
{
    //static smartdma_camera_param_t smartdmaParam;

    /* Clear camera buffer. */
    memset((void *)g_camera_buffer, 0x33, sizeof(g_camera_buffer));
    //CLOCK_EnableClock(kCLOCK_Smartdma);
    /* Init smartdma for camera. */
    SMARTDMA_InitWithoutFirmware();
    SMARTDMA_InstallFirmware(SMARTDMA_CAMERA_MEM_ADDR, s_smartdmaCameraFirmware, SMARTDMA_CAMERA_FIRMWARE_SIZE);
    SMARTDMA_InstallCallback(SDMA_CompleteCallback, NULL); /* Set camera call back. */
    NVIC_EnableIRQ(SMARTDMA_IRQn);
    NVIC_SetPriority(SMARTDMA_IRQn, 3);

    /* Boot smartdma. */
    //smartdmaParam.smartdma_stack = (uint32_t *)g_samrtdma_stack;
    //smartdmaParam.p_buffer       = (uint32_t *)g_camera_buffer;
    /* Make sure the frame size that the firmware fetches is smaller than or equal to the camera resolution.
       In this case it is half of the camera resolution. */
    //SMARTDMA_Boot(DEMO_SMARTDMA_API, &smartdmaParam, 0x2);
}

/*!
 * @brief Main function
 */
int initCamera(void)
{

    /* Enable clock for PCLK. */
    CLOCK_AttachClk(kMAIN_CLK_to_CLKOUT);
    CLOCK_SetClkDiv(kCLOCK_DivClkOut, 25U);   //25U changed to 6 for 25 MHz --> 150 MHz/6


    /* Init camera I2C clock. */
/*    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM7);
    CLOCK_EnableClock(kCLOCK_LPFlexComm7);
    CLOCK_EnableClock(kCLOCK_LPI2c7);
    CLOCK_SetClkDiv(kCLOCK_DivFlexcom7Clk, 1u);*/

    /* Attach camera signals. P0_4/P0_11/P0_5 is set to EZH_CAMERA_VSYNC/EZH_CAMERA_HSYNC/EZH_CAMERA_PCLK. */
/*    INPUTMUX_Init(INPUTMUX0);
    INPUTMUX_AttachSignal(INPUTMUX0, 0, kINPUTMUX_GpioPort0Pin4ToSmartDma);
    INPUTMUX_AttachSignal(INPUTMUX0, 1, kINPUTMUX_GpioPort0Pin11ToSmartDma);
    INPUTMUX_AttachSignal(INPUTMUX0, 2, kINPUTMUX_GpioPort0Pin5ToSmartDma);*/
    /* Turn off clock to inputmux to save power. Clock is only needed to make changes */
    //INPUTMUX_Deinit(INPUTMUX0);
    TCA9803_DIS();
    printf("SmartDma camera init start.\r\n");
    vTaskDelay(100);
    CAMERA_PDWN_ACTIVE();
    CAMERA_RST_ACTIVE();
    vTaskDelay(100);
    CAMERA_PDWN_INACTIVE();
    CAMERA_RST_INACTIVE();

    vTaskDelay(200);


    DEMO_InitCamera();
    uint8_t xsc, ysc;
    // Preserve lower scaling bits.
    //uint8_t xsc = ov7670_read(0x70);
    //ov7670_reg_read(OV7670_SCALING_XSC_REG, &xsc);
    //OV7670_ReadReg(handle, OV7670_SCALING_XSC_REG, &xsc);
    //uint8_t ysc = ov7670_read(0x71);
    //ov7670_reg_read(OV7670_SCALING_YSC_REG, &ysc);
    //OV7670_ReadReg(handle, OV7670_SCALING_YSC_REG, &ysc);

    // Test_pattern[0:1] = 10 => 8-bar color bar
    //ov7670_write(0x70, xsc | 0x80);
    //OV7670_WriteReg(handle, OV7670_SCALING_XSC_REG, xsc | 0x80);
    //ov7670_reg_write(OV7670_SCALING_XSC_REG, xsc & 0x7F);
    //ov7670_write(0x71, ysc & 0x7F);
    //OV7670_WriteReg(handle, OV7670_SCALING_YSC_REG, ysc & 0x7F);
    //ov7670_reg_write(OV7670_SCALING_YSC_REG, ysc | 0x80);

/*    ov7670_reg_read(OV7670_COM7_REG, &xsc);
    printf("dgCameraDriver.c:initCamera():COM7 Value=0x%x\r\n",xsc);
    ov7670_reg_read(OV7670_COM15_REG, &xsc);
    printf("dgCameraDriver.c:initCamera():COM15 Value=0x%x\r\n",xsc);
    ov7670_reg_read(OV7670_TSLB_REG, &xsc);
    printf("dgCameraDriver.c:initCamera():TSLB Value=0x%x\r\n",xsc);
    ov7670_reg_read(OV7670_COM13_REG, &xsc);
    printf("dgCameraDriver.c:initCamera():COM13 Value=0x%x\r\n",xsc);*/
/*    for(int i=0; i< 0xCA; i++)
    {
        ov7670_reg_read(i, &xsc);
        printf("dgCameraDriver.c:initCamera():RegAddr = 0x%x, Value =0x%x\r\n",i, xsc);
    }*/


   DEMO_InitSmartDma();
   //startSmartDma();

    return DG_SUCCESS;

/*    while (1)
    {
        while (!g_camera_complete_flag)
        {
        }
        g_camera_complete_flag = false;
        //DEMO_DrawWindow(0U, 0U, DEMO_BUFFER_HEIGHT - 1U, DEMO_BUFFER_WIDTH - 1U, (const uint8_t *)g_camera_buffer,
                        //DEMO_BUFFER_WIDTH * DEMO_BUFFER_HEIGHT);
    }*/
}

void startSmartDma()
{
    /* Boot smartdma. */
    smartdmaParam.smartdma_stack = (uint32_t *)g_samrtdma_stack;
    smartdmaParam.p_buffer       = (uint32_t *)g_camera_buffer;
    /* Make sure the frame size that the firmware fetches is smaller than or equal to the camera resolution.
       In this case it is half of the camera resolution. */
    SMARTDMA_Boot(DEMO_SMARTDMA_API, &smartdmaParam, 0x2);
}
int captureImage()
{
	g_camera_complete_flag = false;
	startSmartDma();
	while(g_camera_complete_flag == false)
	{
		vTaskDelay(1);
	}
	printf("camera capture completed\r\n");
	g_camera_complete_flag = false;
	return DG_SUCCESS;
}


int getImageData(char *buffer, int pixelOffset, uint8_t size)
{
	uint8_t count;
	uint16_t pixel;
	char *p;
	//char tempbuff[10];


	//Validate parameters
	if((buffer == NULL)||(size>64)||((pixelOffset+size)>DEMO_BUFFER_WIDTH*DEMO_BUFFER_HEIGHT))
	{
		return DG_FAIL;
	}
	p = buffer;
	p += sprintf(p, "CC:");
	//strcpy(buffer, "CC:");
	for(count=0;count<size;count++)
	{
		pixel = g_camera_buffer[pixelOffset+count];
		p += sprintf(p, "%04X ", pixel);
		//itoa(pixel, tempbuff, 16);
		//strcat(buffer, tempbuff);
		//strcat(buffer, " ");
	}
	//strcat(buffer, "\r\n");
	p += sprintf(p, "\r\n");
	return DG_SUCCESS;
}

