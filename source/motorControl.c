/*
 * motorControl.c
 *
 *  Created on: 09-Dec-2025
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
#include "fsl_debug_console.h"
#include "fsl_spc.h"
#include "fsl_lpi2c.h"
#include "fsl_lpuart.h"
#include "fsl_device_registers.h"
#include "fsl_lpspi.h"
#include "fsl_utick.h"
#include "fsl_flexspi.h"

/* Chewie Includes */
#include "dgCommon.h"
#include "version.h"
#include "modulecom.h"
#include "psramDriver.h"
#include "dgtimer.h"
#include "GPIOSignals.h"
#include "seqControlCommon.h"
#include "dgI2cDriver.h"
#include "eeConfig.h"
#include "rtc.h"
#include "ASB_HMI_common.h"
#include "sysStart.h"
#include "dgUartDriverCommon.h"
#include "CliUartDriver.h"
#include "hmiUartDriver.h"
#include "cliProc.h"
#include "sysConfig.h"
#include "mclsSPIDriver.h"
#include "drv89xxDriver.h"
#include "drv89xxRegisters.h"
#include "mclsSPIDriver.h"
#include "drv89xxDriver.h"
#include "drv89xxRegisters.h"
#include "actuatorCtrl.h"
#include "sensorMod.h"
#include "augerAPI.h"
#include "shredder.h"
#include "shredderAPI.h"
#include "adcs.h"
#include "limitSwitchMod.h"
#include "transferCS.h"
#include "transferCSAPI.h"
#include "lidModule.h"
#include "lidModuleAPI.h"
#include "motorControl.h"

/**************************Motor Driver TC78H660FNG***************************
 * Operated in PHASE MODE -> MODE = HIGH
 * 	MOTOR1: ENABLE_A (ON/OFF) PHASE_A (Motor Direction)
 * 	MOTOR2: ENABLE_B (ON/OFF) PHASE_B (Motor Direction)
 *
 * 	Current Limit:1.35A set using external voltage reference IC, which is adjustable
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define ONE_SEC_DELAY	1000 / portTICK_PERIOD_MS
#define TWO_SEC_DELAY	2000 / portTICK_PERIOD_MS
#define THREE_SEC_DELAY	3000 / portTICK_PERIOD_MS

/*******************************************************************************
 * Variables
 ******************************************************************************/


/*******************************************************************************
 * Code
 ******************************************************************************/

//Use the method below to configure the MODE pin of TC78H660 to input/output
//While making TC78H660 active from STBY, MODE pin has to be configured as Output and after
//becomes active, this has to be configured as Input to receive the ERR
void modePinInit(uint8_t io)
{
	gpio_pin_config_t config;

	if(io == DG_PIN_OUTPUT)
	{
		config.pinDirection = kGPIO_DigitalOutput;
		config.outputLogic = 1;
	}
	else
	{
		config.pinDirection = kGPIO_DigitalInput;
		config.outputLogic = 0;
	}
	GPIO_PinInit(BOARD_INITPINS_TC78H660_MODE_GPIO, BOARD_INITPINS_TC78H660_MODE_PIN, &config);
}


/*This method puts the TC78H660 IC to STBY mode*/
void TC78H660_Stby(void)
{
	//Stop both motors and make the Controller Inactive

	MOTOR1_STOP();
	MOTOR2_STOP();
	vTaskDelay(1);
	TC78H660_STBY();

}

/*This method makes the TC78H660 IC ACTIVE mode*/
void TC78H660_Active(void)
{
	modePinInit(DG_PIN_OUTPUT);
	//Configured for MODE HIGH
	TC78H660_PHASE_MODE();
	// For the setup delay, 1uSec, for the Mode input.
	// uSec delay is not possible with vTaskDelay
	vTaskDelay(1);
	//MODE pin is made as an input with a pullup and hence MODE pin will  be default HIGH
	TC78H660_ACTIVE();
	// For the HOLD time delay, 100uSec, for the Mode input.
	// uSec delay is not possible with vTaskDelay
	vTaskDelay(2);
	modePinInit(DG_PIN_INPUT);
}



void fanMotorCWR(void)
{
	//It assumes motor1 is in OFF condition
	MOTOR1_FORWARD();
	MOTOR1_START();
}

void fanMotorCCWR(void)
{
	//It assumes motor is in OFF condition
	MOTOR1_REVERSE();
	MOTOR1_START();
}

void fanMotorStop(void)
{
	MOTOR1_STOP();
}

void augerMotorCWR(void)
{
	//It assumes motor1 is in OFF condition
	MOTOR2_FORWARD();
	MOTOR2_START();
}

void augerMotorCCWR(void)
{
	//It assumes motor is in OFF condition
	MOTOR2_REVERSE();
	MOTOR2_START();
}

void augerMotorStop(void)
{
	MOTOR2_STOP();
}

