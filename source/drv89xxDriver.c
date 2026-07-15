/*
 * drv89xxDriver.c
 *
 *  Created on: 18-Aug-2025
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

/* Chewie Includes */
#include "dgCommon.h"
#include "version.h"
#include "modulecom.h"
#include "rtc.h"
#include "dgtimer.h"
#include "GPIOSignals.h"
#include "eeConfig.h"

#include "ASB_HMI_common.h"
#include "sysStart.h"

#include "sysConfig.h"
#include "mclsSPIDriver.h"
#include "drv89xxDriver.h"
#include "drv89xxRegisters.h"



/*******************************************************************************
 * Global Variables
 ******************************************************************************/
//For DRV89XX_1 device
uint8_t shadow_OP_CTRL_1_Reg;  	//Keeps a local copy of the register
uint8_t shadow_OP_CTRL_2_Reg;	//Keeps a local copy pf OP_CTRL_2 register

//For DRV89XX_2 device
uint8_t shadow2_OP_CTRL_1_Reg;  //Keeps a local copy of the register
uint8_t shadow2_OP_CTRL_2_Reg;	//Keeps a local copy pf OP_CTRL_2 register
uint8_t shadow2_OP_CTRL_3_Reg;	//Keeps a local copy pf OP_CTRL_2 register


SemaphoreHandle_t  drv89xxFHsemaphore;		//Semaphore is to synchronize between DRV89xxFault ISR and Fault handler
SemaphoreHandle_t  drv89xxMutex;			//mutex is to ensure shadow Op control registers are not currupted by ensuring one API at a time

/*******************************************************************************
 * Implementation
 ******************************************************************************/

/******************************************************************************/
/*!
* @brief Interrupt service fuction of FAULT pin of DRV8912.
*
*/
void DRV8912_IRQ_HANDLER(void)
{
	//Get the interrupt status of DRV89XX_FAULT Pin
	if(GPIO_PinGetInterruptFlag(BOARD_INITPINS_FAULT_DRV8912_GPIO, BOARD_INITPINS_FAULT_DRV8912_PIN))
	{
		   /* Clear external interrupt flag. */
		   GPIO_GpioClearInterruptFlags(BOARD_INITPINS_FAULT_DRV8912_GPIO, 1U << BOARD_INITPINS_FAULT_DRV8912_PIN);
		   GREEN_LED_TOGGLE();
		   //xSemaphoreGiveFromISR(drv89xxFHsemaphore, NULL);

	}
	SDK_ISR_EXIT_BARRIER;
}

int initDRV89xxFaultInt()
{
	//Select interrupt channel 0
	GPIO_SetPinInterruptChannel(BOARD_INITPINS_FAULT_DRV8912_GPIO, BOARD_INITPINS_FAULT_DRV8912_PIN, kGPIO_InterruptOutput0);
	NVIC_SetPriority(DRV8912_IRQ, 4);
    EnableIRQ(DRV8912_IRQ);
    return DG_SUCCESS;
}

int readRegDRV89XX_1(uint8_t regAddr, uint16_t *regValue)
{
	return spiReadDrv89xx(DRV89XX_1, regAddr, regValue);
}

int readRegDRV89XX_2(uint8_t regAddr, uint16_t *regValue)
{
	return spiReadDrv89xx(DRV89XX_2, regAddr, regValue);
}


int writeRegDRV89xx_1(uint8_t regAddr, uint8_t regValue)
{
	return spiWriteDrv89xx(DRV89XX_1, regAddr, regValue);
}

int writeRegDRV89xx_2(uint8_t regAddr, uint8_t regValue)
{
	return spiWriteDrv89xx(DRV89XX_2, regAddr, regValue);
}


static void drv89xxFaultHandler(void *pvParameters)
{
	uint16_t regValue;

	while(1)
	{
		//Wait for DRV89xx interrupt
	    xSemaphoreTake(drv89xxFHsemaphore, portMAX_DELAY);
	    printf("drv89xxDriver.c:drv89xxFaultHandler():received signal from DRV89xx ISR\r\n");

	    //Read status register to understand the reason for the interrupt
	    readRegDRV89XX_1(DRV8908_IC_STAT, &regValue);
	    printf("drv89xxDriver.c:drv89xxFaultHandler():DRV89xx_1 IC_STAT Reg = 0x%X\r\n",regValue);
	    if((regValue & 0x003E) != 0x00)
	    {
		    printf("drv89xxDriver.c:drv89xxFaultHandler():DRV89xx_1 IC_STAT Reg = 0x%X\r\n",regValue);
	    	//DRV89XX_1 has generated the interrupt.Check further
	    	if(regValue&0x0010)
	    	{
	    		//Open load condition detected
	    		readRegDRV89XX_1(DRV8908_OLD_STAT_1, &regValue);
	    		switch(regValue&0x00FF)
	    		{
	    		case 0x01:
	    			printf("drv89xxDriver.c:drv89xxFaultHandler(): Open Load detected on HB1 LS\r\n");
	    			break;
	    		case 0x02:
	    			printf("drv89xxDriver.c:drv89xxFaultHandler(): Open Load detected on HB1 HS\r\n");
	    			break;
	    		case 0x04:
	    			printf("drv89xxDriver.c:drv89xxFaultHandler(): Open Load detected on HB2 LS\r\n");
	    			break;
	    		case 0x08:
	    			printf("drv89xxDriver.c:drv89xxFaultHandler(): Open Load detected on HB2 HS\r\n");
	    			break;
	    		case 0x10:
	    			printf("drv89xxDriver.c:drv89xxFaultHandler(): Open Load detected on HB3 LS\r\n");
	    			break;
	    		case 0x20:
	    			printf("drv89xxDriver.c:drv89xxFaultHandler(): Open Load detected on HB3 HS\r\n");
	    			break;
	    		case 0x40:
	    			printf("drv89xxDriver.c:drv89xxFaultHandler(): Open Load detected on HB4 LS\r\n");
	    			break;
	    		case 0x80:
	    			printf("drv89xxDriver.c:drv89xxFaultHandler(): Open Load detected on HB4 HS\r\n");
	    			break;
	    		default:
	    			printf("drv89xxDriver.c:drv89xxFaultHandler(): Open Load detected = 0x%X\r\n",regValue);
	    		}
	    		//Clear open load fault
	    	    writeRegDRV89xx_1(DRV8908_CONFIG_CTRL, CONFIG_CTRL_CLEAR_FAULT);
	    	}
	    	else if(regValue&0x0004)
	    	{
	    		//Over current Condition detected
    			printf("drv89xxDriver.c:drv89xxFaultHandler(): Over current condition detected in DRV89xx_1\r\n");

	    	}
    	    //writeRegDRV89xx_1(DRV8908_CONFIG_CTRL, CONFIG_CTRL_CLEAR_FAULT);
	    }

	    //Read DRV89XX_2
	    readRegDRV89XX_2(DRV8912_IC_STAT, &regValue);
	    if((regValue & 0x003E) != 0x00)
	    {
	    	//DRV89XX_2 has generated the interrupt.Check further
		    printf("drv89xxDriver.c:drv89xxFaultHandler():DRV89xx_2 IC_STAT Reg = 0x%X\r\n",regValue);
    	    writeRegDRV89xx_2(DRV8912_CONFIG_CTRL, CONFIG_CTRL_CLEAR_FAULT);
	    }
	}
}

int initDrv89xxFaulthandler(void)
{
	//Create HMICmdProc task task
	//TaskHandle_t drv89xxFhTaskHandle;
	//BaseType_t result;

/*	result = xTaskCreate(drv89xxFaultHandler, "drv89xxFaultHandler", configMINIMAL_STACK_SIZE + 100, NULL, task_PRIORITY, &drv89xxFhTaskHandle);
    if ( result !=    pdPASS)
    {
        printf("drv89xxDriver.c:initDrv89xxFaulthandler():Drv89xxFaulthandler task creation failed!.\r\n");
        return DG_FAIL;
    }

    drv89xxFHsemaphore = xSemaphoreCreateBinary();
    if(drv89xxFHsemaphore == NULL)
    {
        printf("drv89xxDriver.c:initDrv89xxFaulthandler(): semaphore creation failed!.\r\n");
    	vTaskDelete(drv89xxFhTaskHandle);
        return DG_FAIL;
    }*/

    //Create mutex
    drv89xxMutex = xSemaphoreCreateMutex();
	if (drv89xxMutex == NULL)
	{
        printf("drv89xxDriver.c:initDrv89xxFaulthandler(): mutex creation failed!.\r\n");
		vSemaphoreDelete(drv89xxFHsemaphore);
    	//vTaskDelete(drv89xxFhTaskHandle);
		return DG_FAIL;
	}

    //Enabling DRV89xx fault Interrupt
/*    initDRV89xxFaultInt();*/

    return DG_SUCCESS;
}




int initDRV89XX_1()
{
	uint16_t data;
	//DRV89XX_1 is DRV8908 (8 Channel) Device

	//Wakeup DRV8908 from Sleep
	WAKEUP_DRV89XX();

    vTaskDelay( 1 ); //DRV89XX takes 8 uSec come out of sleep

    //Read Device ID and confirm it is DRV8908
    if(readRegDRV89XX_1(DRV8908_CONFIG_CTRL, &data) != DG_SUCCESS)
    {
    	return DG_FAIL;
    }
    else
    {
    	//Check the Device ID
    	if((data & DRV89XX_DEVICEID_MASK) != DRV8908_ID)
    	{
    		return DG_FAIL;
    	}
    }
    //Initialise OP_CTRL Reg 1 & 2 and corresponding shadow registers
    shadow_OP_CTRL_1_Reg = 0x00;  	//Half bridges 1-4 are in Coast mode (OFF)
    writeRegDRV89xx_1(DRV8908_OP_CTRL_1, shadow_OP_CTRL_1_Reg);
    shadow_OP_CTRL_2_Reg = 0x00;  	//Half bridges 5-8 are in Coast mode (OFF)
    writeRegDRV89xx_1(DRV8908_OP_CTRL_2, shadow_OP_CTRL_2_Reg);

    //Initialise Config Control Reg
    writeRegDRV89xx_1(DRV8908_CONFIG_CTRL, CONFIG_CTRL_CLEAR_FAULT);


    //Map PWM channel 1 HB5 and HB7
    writeRegDRV89xx_1(DRV8908_PWM_MAP_CTRL_3, DRV8908_PWM_MAP_CTRL_3_VALUE);
    writeRegDRV89xx_1(DRV8908_PWM_MAP_CTRL_4, DRV8908_PWM_MAP_CTRL_4_VALUE);

    //Set PWM Freq to 2000Hz for PWM Channel1
    writeRegDRV89xx_1(DRV8908_PWM_FREQ_CTRL_1, DRV8908_PWM_FREQ_CTRL_1_VALUE);

    //Set defaault duty cycle to 100 for PWM Channel 1
    writeRegDRV89xx_1(DRV8908_PWM_DUTY_CTRL_1, DRV8908_PWM_DUTY_CTRL_1_VALUE);

    //Enable Negative current OLD to avoid false OLD during PWM
    writeRegDRV89xx_1(DRV8908_OLD_CTRL_3, DRV8908_OLD_CTRL_3_VALUE);
    //Diasble OLD for all
    writeRegDRV89xx_1(DRV8908_OLD_CTRL_1, DRV8908_OLD_CTRL_1_VALUE);

    //Initialise PWM Control reg
    writeRegDRV89xx_1(DRV8908_PWM_CTRL_1, PWM_CTRL_1_VALUE);
    writeRegDRV89xx_1(DRV8908_PWM_CTRL_2, PWM_CTRL_2_VALUE);

    //Free wheel diode configuration
    writeRegDRV89xx_1(DRV8908_FW_CTRL_1, FW_CTRL_1_VALUE);  //Passive free wheeling enabled

    //Slew Rate control configuration
    writeRegDRV89xx_1(DRV8908_SR_CTRL_1, SR_CTRL_1_VALUE);  //Slew rate 0.6/uSec for all HBs

    return DG_SUCCESS;
}

int setDutyCycleChl1_DRV89XX_1(uint8_t duty)
{
	 writeRegDRV89xx_1(DRV8908_PWM_DUTY_CTRL_1, duty);
	 return DG_SUCCESS;
}


int initDRV89XX_2()
{
	uint16_t data;
	//DRV89XX_2 is DRV8912 (12 Channel) Device

	//Wakeup DRV8912_2 from Sleep
	WAKEUP_DRV89XX();

    vTaskDelay( 1 ); //DRV89XX takes 8 uSec come out of sleep

    //Read Device ID and confirm it is DRV8908
    if(readRegDRV89XX_2(DRV8912_CONFIG_CTRL, &data) != DG_SUCCESS)
    {
    	return DG_FAIL;
    }
    else
    {
    	//Check the Device ID
    	if((data & DRV89XX_DEVICEID_MASK) != DRV8912_ID)
    	{
    		return DG_FAIL;
    	}
    }
    //Initialise OP_CTRL Reg 1 & 2 and corresponding shadow registers
    shadow2_OP_CTRL_1_Reg = 0x00;  	//Half bridges 1-4 are in Coast mode (OFF)
    writeRegDRV89xx_2(DRV8912_OP_CTRL_1, shadow2_OP_CTRL_1_Reg);

    shadow2_OP_CTRL_2_Reg = 0x00;  	//Half bridges 5-8 are in Coast mode (OFF)
    writeRegDRV89xx_2(DRV8912_OP_CTRL_2, shadow2_OP_CTRL_2_Reg);

    shadow2_OP_CTRL_3_Reg = 0x00;  	//Half bridges 9-12 are in Coast mode (OFF)
    writeRegDRV89xx_2(DRV8912_OP_CTRL_3, shadow2_OP_CTRL_3_Reg);

    //Initialise Config Control Reg
    writeRegDRV89xx_2(DRV8912_CONFIG_CTRL, CONFIG_CTRL_CLEAR_FAULT_8912);

    writeRegDRV89xx_2(DRV8912_OLD_CTRL_1, OLD_CTRL_1_VALUE_8912);
    writeRegDRV89xx_2(DRV8912_OLD_CTRL_2, OLD_CTRL_2_VALUE_8912);
    writeRegDRV89xx_2(DRV8912_OLD_CTRL_3, OLD_CTRL_3_VALUE_8912);


    //Initialise PWM Control reg
    writeRegDRV89xx_2(DRV8912_PWM_CTRL_1, PWM_CTRL_1_VALUE_8912_2);
    writeRegDRV89xx_2(DRV8912_PWM_CTRL_2, PWM_CTRL_2_VALUE_8912_2);


    //Map PWM channel 1 HB5 and HB7
    writeRegDRV89xx_2(DRV8912_PWM_MAP_CTRL_2, PWM_MAP_CTRL_2_VALUE_8912_2);
    writeRegDRV89xx_2(DRV8912_PWM_MAP_CTRL_3, PWM_MAP_CTRL_3_VALUE_8912_2);

    //Set PWM Freq to 2000Hz for PWM Channel1
    writeRegDRV89xx_2(DRV8912_PWM_FREQ_CTRL, PWM_FREQ_CTRL_VALUE_8912_2);

    //Set defaault duty cycle to 100 for PWM Channel 1
    writeRegDRV89xx_2(DRV8912_PWM_DUTY_CTRL_1, PWM_DUTY_CTRL_1_VALUE_8912_2);

    //Free wheel diode configuration
    writeRegDRV89xx_2(DRV8912_FW_CTRL_1, FW_CTRL_1_VALUE_8912);  //Passive free wheeling enabled
    writeRegDRV89xx_2(DRV8912_FW_CTRL_2, FW_CTRL_2_VALUE_8912);  //Passive free wheeling enabled

    //Slew Rate control configuration
    writeRegDRV89xx_2(DRV8912_SR_CTRL_1, SR_CTRL_1_VALUE_8912);
    writeRegDRV89xx_2(DRV8912_SR_CTRL_2, SR_CTRL_2_VALUE_8912);

    return DG_SUCCESS;
}





int halfBridgeCtrl(uint8_t spiDeviceID, uint8_t halfBridgeId, uint8_t enaDis)
{
	uint8_t hbCtrlByte;
	int retvalue;
/*

	printf("fullBridgeCtrl():DRV1_OP1=%d,DRV1_OP2=%d\r\n", shadow_OP_CTRL_1_Reg, shadow_OP_CTRL_2_Reg);
	printf("fullBridgeCtrl():DRV2_OP1=%d,DRV2_OP2=%d,DRV2_OP3=%d\r\n", shadow2_OP_CTRL_1_Reg, shadow2_OP_CTRL_2_Reg, shadow2_OP_CTRL_3_Reg);
	printf("fullBridgeCtrl():DRV3_OP1=%d,DRV3_OP2=%d,DRV3_OP3=%d\r\n", shadow3_OP_CTRL_1_Reg, shadow3_OP_CTRL_2_Reg, shadow3_OP_CTRL_3_Reg);
*/

	//lock mutex
    if(xSemaphoreTake(drv89xxMutex, portMAX_DELAY) != pdTRUE)
    {
        return DG_BUSY;
    }

	switch(spiDeviceID)
	{
	case DRV89XX_1:
		//DRV8908 device
		switch(halfBridgeId)
		{
		case HALFBRIDGE_1:
			hbCtrlByte = (enaDis & 0x03);
			shadow_OP_CTRL_1_Reg = (shadow_OP_CTRL_1_Reg & (~OP_CTRL_HB1_MASK)) | (hbCtrlByte);
			retvalue =  spiWriteDrv89xx(DRV89XX_1, DRV8908_OP_CTRL_1, shadow_OP_CTRL_1_Reg);
			break;
		case HALFBRIDGE_2:
			hbCtrlByte = (enaDis & 0x03)<<2;
			shadow_OP_CTRL_1_Reg = (shadow_OP_CTRL_1_Reg & (~OP_CTRL_HB2_MASK)) | (hbCtrlByte);
			retvalue =  spiWriteDrv89xx(DRV89XX_1, DRV8908_OP_CTRL_1, shadow_OP_CTRL_1_Reg);
			break;
		case HALFBRIDGE_3:
			hbCtrlByte = (enaDis & 0x03)<<4;
			shadow_OP_CTRL_1_Reg = (shadow_OP_CTRL_1_Reg & (~OP_CTRL_HB3_MASK)) | (hbCtrlByte);
			retvalue =  spiWriteDrv89xx(DRV89XX_1, DRV8908_OP_CTRL_1, shadow_OP_CTRL_1_Reg);
			break;
		case HALFBRIDGE_4:
			hbCtrlByte = (enaDis & 0x03)<<6;
			shadow_OP_CTRL_1_Reg = (shadow_OP_CTRL_1_Reg & (~OP_CTRL_HB4_MASK)) | (hbCtrlByte);
			retvalue =  spiWriteDrv89xx(DRV89XX_1, DRV8908_OP_CTRL_1, shadow_OP_CTRL_1_Reg);
			break;
		case HALFBRIDGE_5:
			hbCtrlByte = (enaDis & 0x03);
			shadow_OP_CTRL_2_Reg = (shadow_OP_CTRL_2_Reg & (~OP_CTRL_HB5_MASK)) | (hbCtrlByte);
			retvalue =  spiWriteDrv89xx(DRV89XX_1, DRV8908_OP_CTRL_2, shadow_OP_CTRL_2_Reg);
			break;
		case HALFBRIDGE_6:
			hbCtrlByte = (enaDis & 0x03)<<2;
			shadow_OP_CTRL_2_Reg = (shadow_OP_CTRL_2_Reg & (~OP_CTRL_HB6_MASK)) | (hbCtrlByte);
			retvalue =  spiWriteDrv89xx(DRV89XX_1, DRV8908_OP_CTRL_2, shadow_OP_CTRL_2_Reg);
			break;
		case HALFBRIDGE_7:
			hbCtrlByte = (enaDis & 0x03)<<4;
			shadow_OP_CTRL_2_Reg = (shadow_OP_CTRL_2_Reg & (~OP_CTRL_HB7_MASK)) | (hbCtrlByte);
			retvalue =  spiWriteDrv89xx(DRV89XX_1, DRV8908_OP_CTRL_2, shadow_OP_CTRL_2_Reg);
			break;
		case HALFBRIDGE_8:
			hbCtrlByte = (enaDis & 0x03)<<6;
			shadow_OP_CTRL_2_Reg = (shadow_OP_CTRL_2_Reg & (~OP_CTRL_HB8_MASK)) | (hbCtrlByte);
			retvalue =  spiWriteDrv89xx(DRV89XX_1, DRV8908_OP_CTRL_2, shadow_OP_CTRL_2_Reg);
			break;
		default:
		    xSemaphoreGive(drv89xxMutex);
			return DG_INVALID_PARAM;
			break;
		}
	    xSemaphoreGive(drv89xxMutex);
	    return retvalue;
		break;
	case DRV89XX_2:       //DRV8912 Device
		switch(halfBridgeId)
		{
		case HALFBRIDGE_1:
			hbCtrlByte = (enaDis & 0x03);
			shadow2_OP_CTRL_1_Reg = (shadow2_OP_CTRL_1_Reg & (~OP_CTRL_HB1_MASK)) | (hbCtrlByte);
			retvalue =  spiWriteDrv89xx(DRV89XX_2, DRV8912_OP_CTRL_1, shadow2_OP_CTRL_1_Reg);
			break;
		case HALFBRIDGE_2:
			hbCtrlByte = (enaDis & 0x03)<<2;
			shadow2_OP_CTRL_1_Reg = (shadow2_OP_CTRL_1_Reg & (~OP_CTRL_HB2_MASK)) | (hbCtrlByte);
			retvalue =  spiWriteDrv89xx(DRV89XX_2, DRV8912_OP_CTRL_1, shadow2_OP_CTRL_1_Reg);
			break;
		case HALFBRIDGE_3:
			hbCtrlByte = (enaDis & 0x03)<<4;
			shadow2_OP_CTRL_1_Reg = (shadow2_OP_CTRL_1_Reg & (~OP_CTRL_HB3_MASK)) | (hbCtrlByte);
			retvalue =  spiWriteDrv89xx(DRV89XX_2, DRV8912_OP_CTRL_1, shadow2_OP_CTRL_1_Reg);
			break;
		case HALFBRIDGE_4:
			hbCtrlByte = (enaDis & 0x03)<<6;
			shadow2_OP_CTRL_1_Reg = (shadow2_OP_CTRL_1_Reg & (~OP_CTRL_HB4_MASK)) | (hbCtrlByte);
			retvalue =  spiWriteDrv89xx(DRV89XX_2, DRV8912_OP_CTRL_1, shadow2_OP_CTRL_1_Reg);
			break;
		case HALFBRIDGE_5:
			hbCtrlByte = (enaDis & 0x03);
			shadow2_OP_CTRL_2_Reg = (shadow2_OP_CTRL_2_Reg & (~OP_CTRL_HB5_MASK)) | (hbCtrlByte);
			retvalue =  spiWriteDrv89xx(DRV89XX_2, DRV8912_OP_CTRL_2, shadow2_OP_CTRL_2_Reg);
			break;
		case HALFBRIDGE_6:
			hbCtrlByte = (enaDis & 0x03)<<2;
			shadow2_OP_CTRL_2_Reg = (shadow2_OP_CTRL_2_Reg & (~OP_CTRL_HB6_MASK)) | (hbCtrlByte);
			retvalue =  spiWriteDrv89xx(DRV89XX_2, DRV8912_OP_CTRL_2, shadow2_OP_CTRL_2_Reg);
			break;
		case HALFBRIDGE_7:
			hbCtrlByte = (enaDis & 0x03)<<4;
			shadow2_OP_CTRL_2_Reg = (shadow2_OP_CTRL_2_Reg & (~OP_CTRL_HB7_MASK)) | (hbCtrlByte);
			retvalue =  spiWriteDrv89xx(DRV89XX_2, DRV8912_OP_CTRL_2, shadow2_OP_CTRL_2_Reg);
			break;
		case HALFBRIDGE_8:
			hbCtrlByte = (enaDis & 0x03)<<6;
			shadow2_OP_CTRL_2_Reg = (shadow2_OP_CTRL_2_Reg & (~OP_CTRL_HB8_MASK)) | (hbCtrlByte);
			retvalue =  spiWriteDrv89xx(DRV89XX_2, DRV8912_OP_CTRL_2, shadow2_OP_CTRL_2_Reg);
			break;
		case HALFBRIDGE_9:
			hbCtrlByte = (enaDis & 0x03);
			shadow2_OP_CTRL_3_Reg = (shadow2_OP_CTRL_3_Reg & (~OP_CTRL_HB9_MASK)) | (hbCtrlByte);
			retvalue =  spiWriteDrv89xx(DRV89XX_2, DRV8912_OP_CTRL_3, shadow2_OP_CTRL_3_Reg);
			break;
		case HALFBRIDGE_10:
			hbCtrlByte = (enaDis & 0x03)<<2;
			shadow2_OP_CTRL_3_Reg = (shadow2_OP_CTRL_3_Reg & (~OP_CTRL_HB10_MASK)) | (hbCtrlByte);
			retvalue =  spiWriteDrv89xx(DRV89XX_2, DRV8912_OP_CTRL_3, shadow2_OP_CTRL_3_Reg);
			break;
		case HALFBRIDGE_11:
			hbCtrlByte = (enaDis & 0x03)<<4;
			shadow2_OP_CTRL_3_Reg = (shadow2_OP_CTRL_3_Reg & (~OP_CTRL_HB11_MASK)) | (hbCtrlByte);
			retvalue =  spiWriteDrv89xx(DRV89XX_2, DRV8912_OP_CTRL_3, shadow2_OP_CTRL_3_Reg);
			break;
		case HALFBRIDGE_12:
			hbCtrlByte = (enaDis & 0x03)<<6;
			shadow2_OP_CTRL_3_Reg = (shadow2_OP_CTRL_3_Reg & (~OP_CTRL_HB12_MASK)) | (hbCtrlByte);
			retvalue =  spiWriteDrv89xx(DRV89XX_2, DRV8912_OP_CTRL_3, shadow2_OP_CTRL_3_Reg);
			break;
		default:
		    xSemaphoreGive(drv89xxMutex);
			return DG_INVALID_PARAM;
			break;
		}
	    xSemaphoreGive(drv89xxMutex);
	    return retvalue;
		break;
	default:
		printf("drv89xxDriver.c:halfBridgeCtrl(): invalid spiDeviceId\r\n");
	    xSemaphoreGive(drv89xxMutex);
		return DG_INVALID_PARAM;
		break;
	}
    xSemaphoreGive(drv89xxMutex);
    return DG_SUCCESS;
}

int fullBridgeCtrl(uint8_t spiDeviceID, uint8_t mpHalfBridgeId, uint8_t mnHalfBridegId, uint8_t mcControl)
{
	uint8_t hbCtrlByte;
	uint8_t opCtrlReg1Modified, opCtrlReg2Modified, opCtrlReg3Modified;
	uint8_t mpHBenaDis, mnHBenaDis;
/*

	printf("fullBridgeCtrl():DRV1_OP1=%d,DRV1_OP2=%d\r\n", shadow_OP_CTRL_1_Reg, shadow_OP_CTRL_2_Reg);
	printf("fullBridgeCtrl():DRV2_OP1=%d,DRV2_OP2=%d,DRV2_OP3=%d\r\n", shadow2_OP_CTRL_1_Reg, shadow2_OP_CTRL_2_Reg, shadow2_OP_CTRL_3_Reg);
	printf("fullBridgeCtrl():DRV3_OP1=%d,DRV3_OP2=%d,DRV3_OP3=%d\r\n", shadow3_OP_CTRL_1_Reg, shadow3_OP_CTRL_2_Reg, shadow3_OP_CTRL_3_Reg);
*/

	//lock mutex
    if(xSemaphoreTake(drv89xxMutex, portMAX_DELAY) != pdTRUE)
    {
        return DG_BUSY;
    }

	opCtrlReg1Modified = DG_BOOL_FALSE;
	opCtrlReg2Modified = DG_BOOL_FALSE;
	opCtrlReg3Modified = DG_BOOL_FALSE;

	mpHBenaDis = (mcControl >> 2) & 0x03;
	mnHBenaDis = mcControl & 0x03;

	switch(spiDeviceID)
	{
	case DRV89XX_1:  		//DRV8908 device

		switch(mpHalfBridgeId)
		{
		case HALFBRIDGE_1:
			hbCtrlByte = (mpHBenaDis);
			shadow_OP_CTRL_1_Reg = (shadow_OP_CTRL_1_Reg & (~OP_CTRL_HB1_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_2:
			hbCtrlByte = (mpHBenaDis)<<2;
			shadow_OP_CTRL_1_Reg = (shadow_OP_CTRL_1_Reg & (~OP_CTRL_HB2_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_3:
			hbCtrlByte = (mpHBenaDis)<<4;
			shadow_OP_CTRL_1_Reg = (shadow_OP_CTRL_1_Reg & (~OP_CTRL_HB3_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_4:
			hbCtrlByte = (mpHBenaDis)<<6;
			shadow_OP_CTRL_1_Reg = (shadow_OP_CTRL_1_Reg & (~OP_CTRL_HB4_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_5:
			hbCtrlByte = (mpHBenaDis);
			shadow_OP_CTRL_2_Reg = (shadow_OP_CTRL_2_Reg & (~OP_CTRL_HB5_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_6:
			hbCtrlByte = (mpHBenaDis)<<2;
			shadow_OP_CTRL_2_Reg = (shadow_OP_CTRL_2_Reg & (~OP_CTRL_HB6_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_7:
			hbCtrlByte = (mpHBenaDis)<<4;
			shadow_OP_CTRL_2_Reg = (shadow_OP_CTRL_2_Reg & (~OP_CTRL_HB7_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_8:
			hbCtrlByte = (mpHBenaDis)<<6;
			shadow_OP_CTRL_2_Reg = (shadow_OP_CTRL_2_Reg & (~OP_CTRL_HB8_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		default:
		    xSemaphoreGive(drv89xxMutex);
			return DG_INVALID_PARAM;
			break;
		}

		switch(mnHalfBridegId)
		{
		case HALFBRIDGE_1:
			hbCtrlByte = (mnHBenaDis);
			shadow_OP_CTRL_1_Reg = (shadow_OP_CTRL_1_Reg & (~OP_CTRL_HB1_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_2:
			hbCtrlByte = (mnHBenaDis)<<2;
			shadow_OP_CTRL_1_Reg = (shadow_OP_CTRL_1_Reg & (~OP_CTRL_HB2_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_3:
			hbCtrlByte = (mnHBenaDis)<<4;
			shadow_OP_CTRL_1_Reg = (shadow_OP_CTRL_1_Reg & (~OP_CTRL_HB3_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_4:
			hbCtrlByte = (mnHBenaDis)<<6;
			shadow_OP_CTRL_1_Reg = (shadow_OP_CTRL_1_Reg & (~OP_CTRL_HB4_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_5:
			hbCtrlByte = (mnHBenaDis);
			shadow_OP_CTRL_2_Reg = (shadow_OP_CTRL_2_Reg & (~OP_CTRL_HB5_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_6:
			hbCtrlByte = (mnHBenaDis)<<2;
			shadow_OP_CTRL_2_Reg = (shadow_OP_CTRL_2_Reg & (~OP_CTRL_HB6_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_7:
			hbCtrlByte = (mnHBenaDis)<<4;
			shadow_OP_CTRL_2_Reg = (shadow_OP_CTRL_2_Reg & (~OP_CTRL_HB7_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_8:
			hbCtrlByte = (mnHBenaDis)<<6;
			shadow_OP_CTRL_2_Reg = (shadow_OP_CTRL_2_Reg & (~OP_CTRL_HB8_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		default:
		    xSemaphoreGive(drv89xxMutex);
			return DG_INVALID_PARAM;
			break;
		}
		if(opCtrlReg1Modified == DG_BOOL_TRUE)
		{
			spiWriteDrv89xx(DRV89XX_1, DRV8908_OP_CTRL_1, shadow_OP_CTRL_1_Reg);
		}
		if(opCtrlReg2Modified == DG_BOOL_TRUE)
		{
			spiWriteDrv89xx(DRV89XX_1, DRV8908_OP_CTRL_2, shadow_OP_CTRL_2_Reg);
		}
	    xSemaphoreGive(drv89xxMutex);
		return DG_SUCCESS;
		break;
	case DRV89XX_2:     //8912 Device
	    //writeRegDRV89xx_2(DRV8912_CONFIG_CTRL, CONFIG_CTRL_CLEAR_FAULT);
		switch(mpHalfBridgeId)
		{
		case HALFBRIDGE_1:
			hbCtrlByte = (mpHBenaDis);
			shadow2_OP_CTRL_1_Reg = (shadow2_OP_CTRL_1_Reg & (~OP_CTRL_HB1_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_2:
			hbCtrlByte = (mpHBenaDis)<<2;
			shadow2_OP_CTRL_1_Reg = (shadow2_OP_CTRL_1_Reg & (~OP_CTRL_HB2_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_3:
			hbCtrlByte = (mpHBenaDis)<<4;
			shadow2_OP_CTRL_1_Reg = (shadow2_OP_CTRL_1_Reg & (~OP_CTRL_HB3_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_4:
			hbCtrlByte = (mpHBenaDis)<<6;
			shadow2_OP_CTRL_1_Reg = (shadow2_OP_CTRL_1_Reg & (~OP_CTRL_HB4_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_5:
			hbCtrlByte = (mpHBenaDis);
			shadow2_OP_CTRL_2_Reg = (shadow2_OP_CTRL_2_Reg & (~OP_CTRL_HB5_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_6:
			hbCtrlByte = (mpHBenaDis)<<2;
			shadow2_OP_CTRL_2_Reg = (shadow2_OP_CTRL_2_Reg & (~OP_CTRL_HB6_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_7:
			hbCtrlByte = (mpHBenaDis)<<4;
			shadow2_OP_CTRL_2_Reg = (shadow2_OP_CTRL_2_Reg & (~OP_CTRL_HB7_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_8:
			hbCtrlByte = (mpHBenaDis)<<6;
			shadow2_OP_CTRL_2_Reg = (shadow2_OP_CTRL_2_Reg & (~OP_CTRL_HB8_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_9:
			hbCtrlByte = (mpHBenaDis);
			shadow2_OP_CTRL_3_Reg = (shadow2_OP_CTRL_3_Reg & (~OP_CTRL_HB5_MASK)) | (hbCtrlByte);
			opCtrlReg3Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_10:
			hbCtrlByte = (mpHBenaDis)<<2;
			shadow2_OP_CTRL_3_Reg = (shadow2_OP_CTRL_3_Reg & (~OP_CTRL_HB6_MASK)) | (hbCtrlByte);
			opCtrlReg3Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_11:
			hbCtrlByte = (mpHBenaDis)<<4;
			shadow2_OP_CTRL_3_Reg = (shadow2_OP_CTRL_3_Reg & (~OP_CTRL_HB7_MASK)) | (hbCtrlByte);
			opCtrlReg3Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_12:
			hbCtrlByte = (mpHBenaDis)<<6;
			shadow2_OP_CTRL_3_Reg = (shadow2_OP_CTRL_3_Reg & (~OP_CTRL_HB8_MASK)) | (hbCtrlByte);
			opCtrlReg3Modified = DG_BOOL_TRUE;
			break;

		default:
		    xSemaphoreGive(drv89xxMutex);
			return DG_INVALID_PARAM;
			break;
		}

		switch(mnHalfBridegId)
		{
		case HALFBRIDGE_1:
			hbCtrlByte = (mnHBenaDis);
			shadow2_OP_CTRL_1_Reg = (shadow2_OP_CTRL_1_Reg & (~OP_CTRL_HB1_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_2:
			hbCtrlByte = (mnHBenaDis)<<2;
			shadow2_OP_CTRL_1_Reg = (shadow2_OP_CTRL_1_Reg & (~OP_CTRL_HB2_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_3:
			hbCtrlByte = (mnHBenaDis)<<4;
			shadow2_OP_CTRL_1_Reg = (shadow2_OP_CTRL_1_Reg & (~OP_CTRL_HB3_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_4:
			hbCtrlByte = (mnHBenaDis)<<6;
			shadow2_OP_CTRL_1_Reg = (shadow2_OP_CTRL_1_Reg & (~OP_CTRL_HB4_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_5:
			hbCtrlByte = (mnHBenaDis);
			shadow2_OP_CTRL_2_Reg = (shadow2_OP_CTRL_2_Reg & (~OP_CTRL_HB5_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_6:
			hbCtrlByte = (mnHBenaDis)<<2;
			shadow2_OP_CTRL_2_Reg = (shadow2_OP_CTRL_2_Reg & (~OP_CTRL_HB6_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_7:
			hbCtrlByte = (mnHBenaDis)<<4;
			shadow2_OP_CTRL_2_Reg = (shadow2_OP_CTRL_2_Reg & (~OP_CTRL_HB7_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_8:
			hbCtrlByte = (mnHBenaDis)<<6;
			shadow2_OP_CTRL_2_Reg = (shadow2_OP_CTRL_2_Reg & (~OP_CTRL_HB8_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_9:
			hbCtrlByte = (mnHBenaDis);
			shadow2_OP_CTRL_3_Reg = (shadow2_OP_CTRL_3_Reg & (~OP_CTRL_HB5_MASK)) | (hbCtrlByte);
			opCtrlReg3Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_10:
			hbCtrlByte = (mnHBenaDis)<<2;
			shadow2_OP_CTRL_3_Reg = (shadow2_OP_CTRL_3_Reg & (~OP_CTRL_HB6_MASK)) | (hbCtrlByte);
			opCtrlReg3Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_11:
			hbCtrlByte = (mnHBenaDis)<<4;
			shadow2_OP_CTRL_3_Reg = (shadow2_OP_CTRL_3_Reg & (~OP_CTRL_HB7_MASK)) | (hbCtrlByte);
			opCtrlReg3Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_12:
			hbCtrlByte = (mnHBenaDis)<<6;
			shadow2_OP_CTRL_3_Reg = (shadow2_OP_CTRL_3_Reg & (~OP_CTRL_HB8_MASK)) | (hbCtrlByte);
			opCtrlReg3Modified = DG_BOOL_TRUE;
			break;

		default:
		    xSemaphoreGive(drv89xxMutex);
			return DG_INVALID_PARAM;
			break;
		}
		if(opCtrlReg1Modified == DG_BOOL_TRUE)
		{
			spiWriteDrv89xx(DRV89XX_2, DRV8912_OP_CTRL_1, shadow2_OP_CTRL_1_Reg);
		}
		if(opCtrlReg2Modified == DG_BOOL_TRUE)
		{
			spiWriteDrv89xx(DRV89XX_2, DRV8912_OP_CTRL_2, shadow2_OP_CTRL_2_Reg);
		}
		if(opCtrlReg3Modified == DG_BOOL_TRUE)
		{
			spiWriteDrv89xx(DRV89XX_2, DRV8912_OP_CTRL_3, shadow2_OP_CTRL_3_Reg);
		}
	    xSemaphoreGive(drv89xxMutex);
		return DG_SUCCESS;
		break;

	default:
		printf("drv89xxDriver.c:halfBridgeCtrl(): invalid spiDeviceId\r\n");
	    xSemaphoreGive(drv89xxMutex);
		return DG_INVALID_PARAM;
		break;
	}

}

int fullBridgePar2Ctrl(uint8_t spiDeviceID, uint8_t mp1HalfBridgeId, uint8_t mp2HalfBridgeId,uint8_t mn1HalfBridegId, uint8_t mn2HalfBridegId, uint8_t mcControl)
{
	uint8_t hbCtrlByte;
	uint8_t opCtrlReg1Modified, opCtrlReg2Modified, opCtrlReg3Modified;
	uint8_t mpHBenaDis, mnHBenaDis;


	//lock mutex
    if(xSemaphoreTake(drv89xxMutex, portMAX_DELAY) != pdTRUE)
    {
        return DG_BUSY;
    }

	opCtrlReg1Modified = DG_BOOL_FALSE;
	opCtrlReg2Modified = DG_BOOL_FALSE;
	opCtrlReg3Modified = DG_BOOL_FALSE;

	mpHBenaDis = (mcControl >> 2) & 0x03;
	mnHBenaDis = mcControl & 0x03;

	switch(spiDeviceID)
	{
	case DRV89XX_1:  		//DRV8908 device

		switch(mp1HalfBridgeId)
		{
		case HALFBRIDGE_1:
			hbCtrlByte = (mpHBenaDis);
			shadow_OP_CTRL_1_Reg = (shadow_OP_CTRL_1_Reg & (~OP_CTRL_HB1_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_2:
			hbCtrlByte = (mpHBenaDis)<<2;
			shadow_OP_CTRL_1_Reg = (shadow_OP_CTRL_1_Reg & (~OP_CTRL_HB2_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_3:
			hbCtrlByte = (mpHBenaDis)<<4;
			shadow_OP_CTRL_1_Reg = (shadow_OP_CTRL_1_Reg & (~OP_CTRL_HB3_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_4:
			hbCtrlByte = (mpHBenaDis)<<6;
			shadow_OP_CTRL_1_Reg = (shadow_OP_CTRL_1_Reg & (~OP_CTRL_HB4_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_5:
			hbCtrlByte = (mpHBenaDis);
			shadow_OP_CTRL_2_Reg = (shadow_OP_CTRL_2_Reg & (~OP_CTRL_HB5_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_6:
			hbCtrlByte = (mpHBenaDis)<<2;
			shadow_OP_CTRL_2_Reg = (shadow_OP_CTRL_2_Reg & (~OP_CTRL_HB6_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_7:
			hbCtrlByte = (mpHBenaDis)<<4;
			shadow_OP_CTRL_2_Reg = (shadow_OP_CTRL_2_Reg & (~OP_CTRL_HB7_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_8:
			hbCtrlByte = (mpHBenaDis)<<6;
			shadow_OP_CTRL_2_Reg = (shadow_OP_CTRL_2_Reg & (~OP_CTRL_HB8_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		default:
		    xSemaphoreGive(drv89xxMutex);
			return DG_INVALID_PARAM;
			break;
		}
		switch(mp2HalfBridgeId)
		{
		case HALFBRIDGE_1:
			hbCtrlByte = (mpHBenaDis);
			shadow_OP_CTRL_1_Reg = (shadow_OP_CTRL_1_Reg & (~OP_CTRL_HB1_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_2:
			hbCtrlByte = (mpHBenaDis)<<2;
			shadow_OP_CTRL_1_Reg = (shadow_OP_CTRL_1_Reg & (~OP_CTRL_HB2_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_3:
			hbCtrlByte = (mpHBenaDis)<<4;
			shadow_OP_CTRL_1_Reg = (shadow_OP_CTRL_1_Reg & (~OP_CTRL_HB3_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_4:
			hbCtrlByte = (mpHBenaDis)<<6;
			shadow_OP_CTRL_1_Reg = (shadow_OP_CTRL_1_Reg & (~OP_CTRL_HB4_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_5:
			hbCtrlByte = (mpHBenaDis);
			shadow_OP_CTRL_2_Reg = (shadow_OP_CTRL_2_Reg & (~OP_CTRL_HB5_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_6:
			hbCtrlByte = (mpHBenaDis)<<2;
			shadow_OP_CTRL_2_Reg = (shadow_OP_CTRL_2_Reg & (~OP_CTRL_HB6_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_7:
			hbCtrlByte = (mpHBenaDis)<<4;
			shadow_OP_CTRL_2_Reg = (shadow_OP_CTRL_2_Reg & (~OP_CTRL_HB7_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_8:
			hbCtrlByte = (mpHBenaDis)<<6;
			shadow_OP_CTRL_2_Reg = (shadow_OP_CTRL_2_Reg & (~OP_CTRL_HB8_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		default:
		    xSemaphoreGive(drv89xxMutex);
			return DG_INVALID_PARAM;
			break;
		}

		switch(mn1HalfBridegId)
		{
		case HALFBRIDGE_1:
			hbCtrlByte = (mnHBenaDis);
			shadow_OP_CTRL_1_Reg = (shadow_OP_CTRL_1_Reg & (~OP_CTRL_HB1_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_2:
			hbCtrlByte = (mnHBenaDis)<<2;
			shadow_OP_CTRL_1_Reg = (shadow_OP_CTRL_1_Reg & (~OP_CTRL_HB2_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_3:
			hbCtrlByte = (mnHBenaDis)<<4;
			shadow_OP_CTRL_1_Reg = (shadow_OP_CTRL_1_Reg & (~OP_CTRL_HB3_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_4:
			hbCtrlByte = (mnHBenaDis)<<6;
			shadow_OP_CTRL_1_Reg = (shadow_OP_CTRL_1_Reg & (~OP_CTRL_HB4_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_5:
			hbCtrlByte = (mnHBenaDis);
			shadow_OP_CTRL_2_Reg = (shadow_OP_CTRL_2_Reg & (~OP_CTRL_HB5_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_6:
			hbCtrlByte = (mnHBenaDis)<<2;
			shadow_OP_CTRL_2_Reg = (shadow_OP_CTRL_2_Reg & (~OP_CTRL_HB6_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_7:
			hbCtrlByte = (mnHBenaDis)<<4;
			shadow_OP_CTRL_2_Reg = (shadow_OP_CTRL_2_Reg & (~OP_CTRL_HB7_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_8:
			hbCtrlByte = (mnHBenaDis)<<6;
			shadow_OP_CTRL_2_Reg = (shadow_OP_CTRL_2_Reg & (~OP_CTRL_HB8_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		default:
		    xSemaphoreGive(drv89xxMutex);
			return DG_INVALID_PARAM;
			break;
		}
		switch(mn2HalfBridegId)
		{
		case HALFBRIDGE_1:
			hbCtrlByte = (mnHBenaDis);
			shadow_OP_CTRL_1_Reg = (shadow_OP_CTRL_1_Reg & (~OP_CTRL_HB1_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_2:
			hbCtrlByte = (mnHBenaDis)<<2;
			shadow_OP_CTRL_1_Reg = (shadow_OP_CTRL_1_Reg & (~OP_CTRL_HB2_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_3:
			hbCtrlByte = (mnHBenaDis)<<4;
			shadow_OP_CTRL_1_Reg = (shadow_OP_CTRL_1_Reg & (~OP_CTRL_HB3_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_4:
			hbCtrlByte = (mnHBenaDis)<<6;
			shadow_OP_CTRL_1_Reg = (shadow_OP_CTRL_1_Reg & (~OP_CTRL_HB4_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_5:
			hbCtrlByte = (mnHBenaDis);
			shadow_OP_CTRL_2_Reg = (shadow_OP_CTRL_2_Reg & (~OP_CTRL_HB5_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_6:
			hbCtrlByte = (mnHBenaDis)<<2;
			shadow_OP_CTRL_2_Reg = (shadow_OP_CTRL_2_Reg & (~OP_CTRL_HB6_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_7:
			hbCtrlByte = (mnHBenaDis)<<4;
			shadow_OP_CTRL_2_Reg = (shadow_OP_CTRL_2_Reg & (~OP_CTRL_HB7_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_8:
			hbCtrlByte = (mnHBenaDis)<<6;
			shadow_OP_CTRL_2_Reg = (shadow_OP_CTRL_2_Reg & (~OP_CTRL_HB8_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		default:
		    xSemaphoreGive(drv89xxMutex);
			return DG_INVALID_PARAM;
			break;
		}
		if(opCtrlReg1Modified == DG_BOOL_TRUE)
		{
			spiWriteDrv89xx(DRV89XX_1, DRV8908_OP_CTRL_1, shadow_OP_CTRL_1_Reg);
		}
		if(opCtrlReg2Modified == DG_BOOL_TRUE)
		{
			spiWriteDrv89xx(DRV89XX_1, DRV8908_OP_CTRL_2, shadow_OP_CTRL_2_Reg);
		}
	    xSemaphoreGive(drv89xxMutex);
		return DG_SUCCESS;
		break;
	case DRV89XX_2:     //8912 Device

		switch(mp1HalfBridgeId)
		{
		case HALFBRIDGE_1:
			hbCtrlByte = (mpHBenaDis);
			shadow2_OP_CTRL_1_Reg = (shadow2_OP_CTRL_1_Reg & (~OP_CTRL_HB1_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_2:
			hbCtrlByte = (mpHBenaDis)<<2;
			shadow2_OP_CTRL_1_Reg = (shadow2_OP_CTRL_1_Reg & (~OP_CTRL_HB2_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_3:
			hbCtrlByte = (mpHBenaDis)<<4;
			shadow2_OP_CTRL_1_Reg = (shadow2_OP_CTRL_1_Reg & (~OP_CTRL_HB3_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_4:
			hbCtrlByte = (mpHBenaDis)<<6;
			shadow2_OP_CTRL_1_Reg = (shadow2_OP_CTRL_1_Reg & (~OP_CTRL_HB4_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_5:
			hbCtrlByte = (mpHBenaDis);
			shadow2_OP_CTRL_2_Reg = (shadow2_OP_CTRL_2_Reg & (~OP_CTRL_HB5_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_6:
			hbCtrlByte = (mpHBenaDis)<<2;
			shadow2_OP_CTRL_2_Reg = (shadow2_OP_CTRL_2_Reg & (~OP_CTRL_HB6_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_7:
			hbCtrlByte = (mpHBenaDis)<<4;
			shadow2_OP_CTRL_2_Reg = (shadow2_OP_CTRL_2_Reg & (~OP_CTRL_HB7_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_8:
			hbCtrlByte = (mpHBenaDis)<<6;
			shadow2_OP_CTRL_2_Reg = (shadow2_OP_CTRL_2_Reg & (~OP_CTRL_HB8_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_9:
			hbCtrlByte = (mpHBenaDis);
			shadow2_OP_CTRL_3_Reg = (shadow2_OP_CTRL_3_Reg & (~OP_CTRL_HB5_MASK)) | (hbCtrlByte);
			opCtrlReg3Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_10:
			hbCtrlByte = (mpHBenaDis)<<2;
			shadow2_OP_CTRL_3_Reg = (shadow2_OP_CTRL_3_Reg & (~OP_CTRL_HB6_MASK)) | (hbCtrlByte);
			opCtrlReg3Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_11:
			hbCtrlByte = (mpHBenaDis)<<4;
			shadow2_OP_CTRL_3_Reg = (shadow2_OP_CTRL_3_Reg & (~OP_CTRL_HB7_MASK)) | (hbCtrlByte);
			opCtrlReg3Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_12:
			hbCtrlByte = (mpHBenaDis)<<6;
			shadow2_OP_CTRL_3_Reg = (shadow2_OP_CTRL_3_Reg & (~OP_CTRL_HB8_MASK)) | (hbCtrlByte);
			opCtrlReg3Modified = DG_BOOL_TRUE;
			break;

		default:
		    xSemaphoreGive(drv89xxMutex);
			return DG_INVALID_PARAM;
			break;
		}
		switch(mp2HalfBridgeId)
		{
		case HALFBRIDGE_1:
			hbCtrlByte = (mpHBenaDis);
			shadow2_OP_CTRL_1_Reg = (shadow2_OP_CTRL_1_Reg & (~OP_CTRL_HB1_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_2:
			hbCtrlByte = (mpHBenaDis)<<2;
			shadow2_OP_CTRL_1_Reg = (shadow2_OP_CTRL_1_Reg & (~OP_CTRL_HB2_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_3:
			hbCtrlByte = (mpHBenaDis)<<4;
			shadow2_OP_CTRL_1_Reg = (shadow2_OP_CTRL_1_Reg & (~OP_CTRL_HB3_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_4:
			hbCtrlByte = (mpHBenaDis)<<6;
			shadow2_OP_CTRL_1_Reg = (shadow2_OP_CTRL_1_Reg & (~OP_CTRL_HB4_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_5:
			hbCtrlByte = (mpHBenaDis);
			shadow2_OP_CTRL_2_Reg = (shadow2_OP_CTRL_2_Reg & (~OP_CTRL_HB5_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_6:
			hbCtrlByte = (mpHBenaDis)<<2;
			shadow2_OP_CTRL_2_Reg = (shadow2_OP_CTRL_2_Reg & (~OP_CTRL_HB6_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_7:
			hbCtrlByte = (mpHBenaDis)<<4;
			shadow2_OP_CTRL_2_Reg = (shadow2_OP_CTRL_2_Reg & (~OP_CTRL_HB7_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_8:
			hbCtrlByte = (mpHBenaDis)<<6;
			shadow2_OP_CTRL_2_Reg = (shadow2_OP_CTRL_2_Reg & (~OP_CTRL_HB8_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_9:
			hbCtrlByte = (mpHBenaDis);
			shadow2_OP_CTRL_3_Reg = (shadow2_OP_CTRL_3_Reg & (~OP_CTRL_HB5_MASK)) | (hbCtrlByte);
			opCtrlReg3Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_10:
			hbCtrlByte = (mpHBenaDis)<<2;
			shadow2_OP_CTRL_3_Reg = (shadow2_OP_CTRL_3_Reg & (~OP_CTRL_HB6_MASK)) | (hbCtrlByte);
			opCtrlReg3Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_11:
			hbCtrlByte = (mpHBenaDis)<<4;
			shadow2_OP_CTRL_3_Reg = (shadow2_OP_CTRL_3_Reg & (~OP_CTRL_HB7_MASK)) | (hbCtrlByte);
			opCtrlReg3Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_12:
			hbCtrlByte = (mpHBenaDis)<<6;
			shadow2_OP_CTRL_3_Reg = (shadow2_OP_CTRL_3_Reg & (~OP_CTRL_HB8_MASK)) | (hbCtrlByte);
			opCtrlReg3Modified = DG_BOOL_TRUE;
			break;

		default:
		    xSemaphoreGive(drv89xxMutex);
			return DG_INVALID_PARAM;
			break;
		}


		switch(mn1HalfBridegId)
		{
		case HALFBRIDGE_1:
			hbCtrlByte = (mnHBenaDis);
			shadow2_OP_CTRL_1_Reg = (shadow2_OP_CTRL_1_Reg & (~OP_CTRL_HB1_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_2:
			hbCtrlByte = (mnHBenaDis)<<2;
			shadow2_OP_CTRL_1_Reg = (shadow2_OP_CTRL_1_Reg & (~OP_CTRL_HB2_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_3:
			hbCtrlByte = (mnHBenaDis)<<4;
			shadow2_OP_CTRL_1_Reg = (shadow2_OP_CTRL_1_Reg & (~OP_CTRL_HB3_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_4:
			hbCtrlByte = (mnHBenaDis)<<6;
			shadow2_OP_CTRL_1_Reg = (shadow2_OP_CTRL_1_Reg & (~OP_CTRL_HB4_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_5:
			hbCtrlByte = (mnHBenaDis);
			shadow2_OP_CTRL_2_Reg = (shadow2_OP_CTRL_2_Reg & (~OP_CTRL_HB5_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_6:
			hbCtrlByte = (mnHBenaDis)<<2;
			shadow2_OP_CTRL_2_Reg = (shadow2_OP_CTRL_2_Reg & (~OP_CTRL_HB6_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_7:
			hbCtrlByte = (mnHBenaDis)<<4;
			shadow2_OP_CTRL_2_Reg = (shadow2_OP_CTRL_2_Reg & (~OP_CTRL_HB7_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_8:
			hbCtrlByte = (mnHBenaDis)<<6;
			shadow2_OP_CTRL_2_Reg = (shadow2_OP_CTRL_2_Reg & (~OP_CTRL_HB8_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_9:
			hbCtrlByte = (mnHBenaDis);
			shadow2_OP_CTRL_3_Reg = (shadow2_OP_CTRL_3_Reg & (~OP_CTRL_HB5_MASK)) | (hbCtrlByte);
			opCtrlReg3Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_10:
			hbCtrlByte = (mnHBenaDis)<<2;
			shadow2_OP_CTRL_3_Reg = (shadow2_OP_CTRL_3_Reg & (~OP_CTRL_HB6_MASK)) | (hbCtrlByte);
			opCtrlReg3Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_11:
			hbCtrlByte = (mnHBenaDis)<<4;
			shadow2_OP_CTRL_3_Reg = (shadow2_OP_CTRL_3_Reg & (~OP_CTRL_HB7_MASK)) | (hbCtrlByte);
			opCtrlReg3Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_12:
			hbCtrlByte = (mnHBenaDis)<<6;
			shadow2_OP_CTRL_3_Reg = (shadow2_OP_CTRL_3_Reg & (~OP_CTRL_HB8_MASK)) | (hbCtrlByte);
			opCtrlReg3Modified = DG_BOOL_TRUE;
			break;

		default:
		    xSemaphoreGive(drv89xxMutex);
			return DG_INVALID_PARAM;
			break;
		}
		switch(mn2HalfBridegId)
		{
		case HALFBRIDGE_1:
			hbCtrlByte = (mnHBenaDis);
			shadow2_OP_CTRL_1_Reg = (shadow2_OP_CTRL_1_Reg & (~OP_CTRL_HB1_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_2:
			hbCtrlByte = (mnHBenaDis)<<2;
			shadow2_OP_CTRL_1_Reg = (shadow2_OP_CTRL_1_Reg & (~OP_CTRL_HB2_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_3:
			hbCtrlByte = (mnHBenaDis)<<4;
			shadow2_OP_CTRL_1_Reg = (shadow2_OP_CTRL_1_Reg & (~OP_CTRL_HB3_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_4:
			hbCtrlByte = (mnHBenaDis)<<6;
			shadow2_OP_CTRL_1_Reg = (shadow2_OP_CTRL_1_Reg & (~OP_CTRL_HB4_MASK)) | (hbCtrlByte);
			opCtrlReg1Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_5:
			hbCtrlByte = (mnHBenaDis);
			shadow2_OP_CTRL_2_Reg = (shadow2_OP_CTRL_2_Reg & (~OP_CTRL_HB5_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_6:
			hbCtrlByte = (mnHBenaDis)<<2;
			shadow2_OP_CTRL_2_Reg = (shadow2_OP_CTRL_2_Reg & (~OP_CTRL_HB6_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_7:
			hbCtrlByte = (mnHBenaDis)<<4;
			shadow2_OP_CTRL_2_Reg = (shadow2_OP_CTRL_2_Reg & (~OP_CTRL_HB7_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_8:
			hbCtrlByte = (mnHBenaDis)<<6;
			shadow2_OP_CTRL_2_Reg = (shadow2_OP_CTRL_2_Reg & (~OP_CTRL_HB8_MASK)) | (hbCtrlByte);
			opCtrlReg2Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_9:
			hbCtrlByte = (mnHBenaDis);
			shadow2_OP_CTRL_3_Reg = (shadow2_OP_CTRL_3_Reg & (~OP_CTRL_HB5_MASK)) | (hbCtrlByte);
			opCtrlReg3Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_10:
			hbCtrlByte = (mnHBenaDis)<<2;
			shadow2_OP_CTRL_3_Reg = (shadow2_OP_CTRL_3_Reg & (~OP_CTRL_HB6_MASK)) | (hbCtrlByte);
			opCtrlReg3Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_11:
			hbCtrlByte = (mnHBenaDis)<<4;
			shadow2_OP_CTRL_3_Reg = (shadow2_OP_CTRL_3_Reg & (~OP_CTRL_HB7_MASK)) | (hbCtrlByte);
			opCtrlReg3Modified = DG_BOOL_TRUE;
			break;

		case HALFBRIDGE_12:
			hbCtrlByte = (mnHBenaDis)<<6;
			shadow2_OP_CTRL_3_Reg = (shadow2_OP_CTRL_3_Reg & (~OP_CTRL_HB8_MASK)) | (hbCtrlByte);
			opCtrlReg3Modified = DG_BOOL_TRUE;
			break;

		default:
		    xSemaphoreGive(drv89xxMutex);
			return DG_INVALID_PARAM;
			break;
		}
		if(opCtrlReg1Modified == DG_BOOL_TRUE)
		{
			spiWriteDrv89xx(DRV89XX_2, DRV8912_OP_CTRL_1, shadow2_OP_CTRL_1_Reg);
		}
		if(opCtrlReg2Modified == DG_BOOL_TRUE)
		{
			spiWriteDrv89xx(DRV89XX_2, DRV8912_OP_CTRL_2, shadow2_OP_CTRL_2_Reg);
		}
		if(opCtrlReg3Modified == DG_BOOL_TRUE)
		{
			spiWriteDrv89xx(DRV89XX_2, DRV8912_OP_CTRL_3, shadow2_OP_CTRL_3_Reg);
		}
	    xSemaphoreGive(drv89xxMutex);
		return DG_SUCCESS;
		break;
	default:
		printf("drv89xxDriver.c:halfBridgeCtrl(): invalid spiDeviceId\r\n");
	    xSemaphoreGive(drv89xxMutex);
		return DG_INVALID_PARAM;
		break;
	}

}


