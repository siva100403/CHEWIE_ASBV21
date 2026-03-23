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
#include "fsl_ctimer.h"

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


/*******************************************************************************
 * Variables
 ******************************************************************************/
volatile uint32_t g_pwmPeriod   = 0U;
volatile uint32_t g_pulsePeriod = 0U;

/*******************************************************************************
 * Code
 ******************************************************************************/
status_t CTIMER_GetPwmPeriodValue(uint32_t pwmFreqHz, uint8_t dutyCyclePercent, uint32_t timerClock_Hz)
{
    /* Calculate PWM period match value */
    g_pwmPeriod = (timerClock_Hz / pwmFreqHz) - 1U;

    /* Calculate pulse width match value */
    g_pulsePeriod = (g_pwmPeriod + 1U) * (100 - dutyCyclePercent) / 100;

    return kStatus_Success;
}

//CTIMER3 is for PWM signal generation for TC78H660FNG
//MAT0 output of CT3 should be connected to P4_16, Pin 38

#define CTIMER_MAT_PWM_PERIOD_CHANNEL kCTIMER_Match_0
#define CTIMER          	CTIMER3         /* Timer 3 */
#define CTIMER_MAT_OUT  	kCTIMER_Match_0 /* Match output 0 */
#define CTIMER_CLK_FREQ 	CLOCK_GetCTimerClkFreq(3U)
#define CTIMER_FREQUENCY	20000
#define CTIMER_DUTY_CYCLE_DEFAULT	50
void initCTimer3()
{
    ctimer_config_t config;
    uint32_t srcClock_Hz;
    uint32_t timerClock;

    /* Use FRO HF clock for  Ctimer3 */
    CLOCK_SetClkDiv(kCLOCK_DivCtimer3Clk, 1u);
    CLOCK_AttachClk(kFRO_HF_to_CTIMER3);

    /* CTimer3 counter */
    srcClock_Hz = CTIMER_CLK_FREQ;

    CTIMER_GetDefaultConfig(&config);
    timerClock = srcClock_Hz / (config.prescale + 1);

    CTIMER_Init(CTIMER, &config);

    /* Get the PWM period match value and pulse width match value of 20Khz PWM signal with 50% dutycycle */
    CTIMER_GetPwmPeriodValue(CTIMER_FREQUENCY, (uint8_t)CTIMER_DUTY_CYCLE_DEFAULT, timerClock);
    CTIMER_SetupPwmPeriod(CTIMER, CTIMER_MAT_PWM_PERIOD_CHANNEL, CTIMER_MAT_OUT, g_pwmPeriod, g_pulsePeriod, false);
}

void pwmStart(uint8_t dutyCycle)
{
    ctimer_config_t config;
    uint32_t srcClock_Hz;
    uint32_t timerClock;

    /* CTimer3 counter */
    srcClock_Hz = CTIMER_CLK_FREQ;
    CTIMER_GetDefaultConfig(&config);

    timerClock = srcClock_Hz / (config.prescale + 1);
    /* Get the PWM period match value and pulse width match value of 20Khz PWM signal with 50% dutycycle */
    CTIMER_GetPwmPeriodValue(CTIMER_FREQUENCY, dutyCycle, timerClock);
    CTIMER_SetupPwmPeriod(CTIMER, CTIMER_MAT_PWM_PERIOD_CHANNEL, CTIMER_MAT_OUT, g_pwmPeriod, g_pulsePeriod, false);
    CTIMER_StartTimer(CTIMER);
}

void pwmStop()
{
    CTIMER_StopTimer(CTIMER);
}
void stMotorCWR(void)
{
	//It assumes motor1 is in OFF condition
	MOTOR1_FORWARD();
	//MOTOR1_START();
	pwmStart(80);
}

void stMotorCCWR(void)
{
	//It assumes motor is in OFF condition
	MOTOR1_REVERSE();
	pwmStart(80);
	//MOTOR1_START();
}

void stMotorStop(void)
{
	//MOTOR1_STOP();
	pwmStop();
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

