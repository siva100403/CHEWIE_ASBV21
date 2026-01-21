/*
 * sysStart.c
 *
 *  Created on: 05-May-2025
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
#include "motorControl.h"
#include "seqControlCommon.h"
#include "dgI2cDriver.h"
#include "eeConfig.h"
#include "sht40Driver.h"
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
#include "motorControl.h"
#include "sensorMod.h"
#include "sensorModAPI.h"
#include "sht40Driver.h"
#include "augerAPI.h"
#include "shredder.h"
#include "shredderAPI.h"
#include "adcs.h"
#include "limitSwitchMod.h"
#include "transferCS.h"
#include "transferCSAPI.h"
#include "lidModule.h"
#include "lidModuleAPI.h"
#include "hatcsMod.h"
#include "hatcsModAPI.h"
#include "HMICmdProc.h"
#include "HMICmdProcAPI.h"
#include "csmMod.h"
#include "csmModAPI.h"
#include "measure.h"
#include "alert.h"




static void sysStart_task(void *pvParameters)
{
	QueueHandle_t sysStartQHandle;
	dgMsg_t rcvMsg;					//Holds the currently received message
	//static int sysStartState;		// This stores the state of system start and health module

	//Health register allocation
	dgHealthStatus_t 	devHealthReg[LAST_DEVICE];
	dgHealthStatus_t 	moduleHealthReg[LAST_MODULE];


	//Initialize devHealthReg and moduleHealthReg
	memset((void*)devHealthReg, 0, sizeof(devHealthReg));
	memset((void*)moduleHealthReg, 0, sizeof(moduleHealthReg));

	//Wait till module registration is complete
	sysStartQHandle = NULL;
	while(sysStartQHandle== NULL)
	{
		vTaskDelay(100 / portTICK_PERIOD_MS);
		sysStartQHandle = getQHandle(SYSSTART_MOD);
	}

	initVersion();

    BLUE_LED_ON();
    RED_LED_OFF();
    GREEN_LED_OFF();
	//Initialize the state
	//sysStartState = SYSSTART_STATE_START;

	printf("sysStart_task(): Started\r\n");

	char version[16];
	getHwVersion(version);
	printf("\r\n******************* ASB HW Version:%s ***************\r\n\r\n",version);
	getFwVersion(version);
	printf("\r\n******************* ASB FW Version:%s ***************\r\n\r\n",version);



	//Initialize I2C port used by RTC and EEPROM
	if(initRTC_I2C() == DG_SUCCESS)
	{
		printf("sysStart_task(): initRTC_I2C() passed\r\n");
		devHealthReg[RTC_DEV].presenceStatus = CNI_DEVICE_PRESENCE;
		devHealthReg[RTC_DEV].operationStatus = UNKOWN_DEVICE_STATUS;
		devHealthReg[EEPROM_DEV].presenceStatus = CNI_DEVICE_PRESENCE;
		devHealthReg[EEPROM_DEV].operationStatus = UNKOWN_DEVICE_STATUS;
	}
	else
	{
		printf("sysStart_task(): initRTC_I2C() failed\r\n");
		//This I2C bus is used by RTC and EEPROM. Mark both devices not working
		devHealthReg[RTC_DEV].presenceStatus = DEVICE_NOTPRESENT;
		devHealthReg[RTC_DEV].operationStatus = DEVICE_NOTWORKING;
		devHealthReg[EEPROM_DEV].presenceStatus = DEVICE_NOTPRESENT;
		devHealthReg[EEPROM_DEV].operationStatus = DEVICE_NOTWORKING;
	}
	//Initialize I2C port used by Camera and Temperature/Humidity sensor
	if(initCamera_I2C() == DG_SUCCESS)
	{
		printf("sysStart_task(): initCamera_I2C() passed\r\n");
		devHealthReg[TEMPSENSOR_DEV].presenceStatus = CNI_DEVICE_PRESENCE;
		devHealthReg[TEMPSENSOR_DEV].operationStatus = UNKOWN_DEVICE_STATUS;
		devHealthReg[CAMERA_DEV].presenceStatus = CNI_DEVICE_PRESENCE;
		devHealthReg[CAMERA_DEV].operationStatus = UNKOWN_DEVICE_STATUS;
	}
	else
	{
		printf("sysStart_task(): initCamera_I2C() failed\r\n");
		//This I2C bus is used by Camera and Temp sensor. Mark both devices not working
		devHealthReg[TEMPSENSOR_DEV].presenceStatus = DEVICE_NOTPRESENT;
		devHealthReg[TEMPSENSOR_DEV].operationStatus = DEVICE_NOTWORKING;
		devHealthReg[CAMERA_DEV].presenceStatus = DEVICE_NOTPRESENT;
		devHealthReg[CAMERA_DEV].operationStatus = DEVICE_NOTPRESENT;
	}

	//Initialize RTC
	if(devHealthReg[RTC_DEV].presenceStatus == CNI_DEVICE_PRESENCE)
	{
		if(mcp7940_init()== DG_SUCCESS)
		{
			//RTC initialization is success. Update Health Register
			devHealthReg[RTC_DEV].presenceStatus = DEVICE_PRESENT;
			devHealthReg[RTC_DEV].operationStatus = DEVICE_WORKING;

		}
		else
		{
			//RTC initialization has failed. Update Health Register
			devHealthReg[RTC_DEV].presenceStatus = DEVICE_PRESENT;
			devHealthReg[RTC_DEV].operationStatus = DEVICE_NOTWORKING;
		}

	}
	else
	{
		//I2C bus initialization failed. Hence don't initialize RTC

	}

	//Initialize Chewie System Configuration from EEPROM
	if(initSysConfig()==DG_SUCCESS)
	{
		printf("sysStart_task(): initSysConfig() passed\r\n");
	}
	else
	{
		printf("sysStart_task(): initSysConfig() failed\r\n");
	}




	//Initialize CLI Module
	if(initCli()==DG_SUCCESS)
	{
		printf("sysStart_task(): initCli() passed\r\n");
		moduleHealthReg[CLI_MOD].presenceStatus = MODULE_PRESENT;
		moduleHealthReg[CLI_MOD].operationStatus = MODULE_WORKING;

	}
	else
	{
		printf("sysStart_task(): initCli() failed\r\n");
		moduleHealthReg[CLI_MOD].presenceStatus = MODULE_PRESENT;
		moduleHealthReg[CLI_MOD].operationStatus = MODULE_NOTWORKING;
	}

	//If initCLI is successful initialize the UART for CLI
	if(moduleHealthReg[CLI_MOD].operationStatus == MODULE_WORKING)
	{
		if(initCliUart() == DG_SUCCESS)
		{
			printf("sysStart_task(): initCliUart() passed\r\n");
			//CLI UART initialization is success. Update Health Register
			devHealthReg[CLI_UART_DEV].presenceStatus = DEVICE_PRESENT;
			devHealthReg[CLI_UART_DEV].operationStatus = DEVICE_WORKING;
		}
		else
		{
			printf("sysStart_task(): initCliUart() failed\r\n");
			//CLI UART initialization failed. Update Health Register
			devHealthReg[CLI_UART_DEV].presenceStatus = DEVICE_PRESENT;
			devHealthReg[CLI_UART_DEV].operationStatus = DEVICE_NOTWORKING;
		}
	}

	//Initialize and make TC78H660 active
	TC78H660_Active();
	printf("sysStart_task():TC78H660 made active r\n");

	//Initialize LPSPI Device which is used by TDC1000 and DRV89XX devices
	if(initSPI_MCLS() == DG_SUCCESS)
	{
		printf("sysStart_task(): initSPI_MCLS()) passed\r\n");
	}
	else
	{
		printf("sysStart_task(): initSPI_MCLS()) failed\r\n");
	}
	//Initialize DRV89XX_1 Motor Driver Device Module
	if(initDRV89XX_1()==DG_SUCCESS)
	{
		printf("sysStart_task(): initDRV89XX_1() passed\r\n");
		devHealthReg[DRV89XX_1_DEV].presenceStatus = DEVICE_PRESENT;
		devHealthReg[DRV89XX_1_DEV].operationStatus = DEVICE_WORKING;

	}
	else
	{
		printf("sysStart_task(): initDRV89XX_1() failed\r\n");
		devHealthReg[DRV89XX_1_DEV].presenceStatus = DEVICE_NOTPRESENT;
		devHealthReg[DRV89XX_1_DEV].operationStatus = DEVICE_NOTWORKING;
	}

	//Initialize DRV89XX_2 Motor Driver Device Module
	if(initDRV89XX_2()==DG_SUCCESS)
	{
		printf("sysStart_task(): initDRV89XX_2() passed\r\n");
		devHealthReg[DRV89XX_2_DEV].presenceStatus = DEVICE_PRESENT;
		devHealthReg[DRV89XX_2_DEV].operationStatus = DEVICE_WORKING;

	}
	else
	{
		printf("sysStart_task(): initDRV89XX_2() failed\r\n");
		devHealthReg[DRV89XX_2_DEV].presenceStatus = DEVICE_NOTPRESENT;
		devHealthReg[DRV89XX_2_DEV].operationStatus = DEVICE_NOTWORKING;
	}


	if(initDrv89xxFaulthandler() == DG_SUCCESS)
	{
		printf("sysStart_task():initDrv89xxFaulthandler() Success\r\n");
	}
	else
	{
		printf("sysStart_task():initDrv89xxFaulthandler() failed\r\n");
	}

	//Initialize SHT40 Temperature, Humidity sensor
	if(initSht40()==DG_SUCCESS)
	{
		printf("sysStart_task(): initSht40() passed\r\n");
		devHealthReg[TEMPSENSOR_DEV].presenceStatus = DEVICE_PRESENT;
		devHealthReg[TEMPSENSOR_DEV].operationStatus = DEVICE_WORKING;

	}
	else
	{
		printf("sysStart_task(): initSht40() failed\r\n");
		devHealthReg[TEMPSENSOR_DEV].presenceStatus = DEVICE_NOTPRESENT;
		devHealthReg[TEMPSENSOR_DEV].operationStatus = DEVICE_NOTWORKING;
	}

	//Initialize HMI Command Processing Module
	if(initHMICmdProc()==DG_SUCCESS)
	{
		printf("sysStart_task(): initHMICmdProc() passed\r\n");
		moduleHealthReg[HMICMDPROC_MOD].presenceStatus = MODULE_PRESENT;
		moduleHealthReg[HMICMDPROC_MOD].operationStatus = MODULE_WORKING;

	}
	else
	{
		printf("sysStart_task(): initHMICmdProc() failed\r\n");
		moduleHealthReg[HMICMDPROC_MOD].presenceStatus = MODULE_PRESENT;
		moduleHealthReg[HMICMDPROC_MOD].operationStatus = MODULE_NOTWORKING;
	}


	//Initialize UART used for HMI communication
    if(initHMIUart() == DG_SUCCESS)
    {
		devHealthReg[HMI_UART_DEV].presenceStatus = DEVICE_PRESENT;
		devHealthReg[HMI_UART_DEV].operationStatus = DEVICE_WORKING;
    }
    else
    {
		devHealthReg[HMI_UART_DEV].presenceStatus = DEVICE_PRESENT;
		devHealthReg[HMI_UART_DEV].operationStatus = DEVICE_NOTWORKING;
    }

    //Initialize Lid module and associated components
    if(initLidModule() == DG_SUCCESS)
	{
		printf("sysStart_task(): initLidModule() passed\r\n");
		moduleHealthReg[LID_MOD].presenceStatus = MODULE_PRESENT;
		moduleHealthReg[LID_MOD].operationStatus = MODULE_IDLE;
	}
	else
	{
		printf("sysStart_task(): initLidModule() failed\r\n");
		moduleHealthReg[LID_MOD].presenceStatus = MODULE_PRESENT;
		moduleHealthReg[LID_MOD].operationStatus = MODULE_NOTWORKING;
	}

    //vTaskDelay(500 / portTICK_PERIOD_MS);
    //Initialize shredder module and associated components
    if(initShd() == DG_SUCCESS)
	{
		printf("sysStart_task(): initShd() passed\r\n");
		moduleHealthReg[SHREDDER_MOD].presenceStatus = MODULE_PRESENT;
		moduleHealthReg[SHREDDER_MOD].operationStatus = MODULE_WORKING;
	}
	else
	{
		printf("sysStart_task(): initShd() failed\r\n");
		moduleHealthReg[SHREDDER_MOD].presenceStatus = MODULE_NOTPRESENT;
		moduleHealthReg[SHREDDER_MOD].operationStatus = MODULE_NOTWORKING;
	}
    //Initialize Limit switch module
    if(initLimitSwitchModule() == DG_SUCCESS)
	{
		printf("sysStart_task(): initLimitSwitchModule() passed\r\n");
		devHealthReg[LIMITSWITCH_MOD].presenceStatus = DEVICE_PRESENT;
		devHealthReg[LIMITSWITCH_MOD].operationStatus = DEVICE_WORKING;
	}
	else
	{
		printf("sysStart_task(): initLimitSwitchModule() failed\r\n");
		devHealthReg[LIMITSWITCH_MOD].presenceStatus = DEVICE_PRESENT;
		devHealthReg[LIMITSWITCH_MOD].operationStatus = DEVICE_NOTWORKING;
	}

	if(initSensor() == DG_SUCCESS)
	{
		printf("sysStart_task(): initSensor() passed\r\n");
		moduleHealthReg[SENSOR_MOD].presenceStatus = MODULE_PRESENT;
		moduleHealthReg[SENSOR_MOD].operationStatus = MODULE_WORKING;
	}
	else
	{
		printf("sysStart_task(): initSensor() failed\r\n");
		moduleHealthReg[SENSOR_MOD].presenceStatus = MODULE_PRESENT;
		moduleHealthReg[SENSOR_MOD].operationStatus = MODULE_NOTWORKING;
	}

    if(initAdcs() == DG_SUCCESS)
    {
    	printf("sysStart_task(): initAdcs success\r\n");
		moduleHealthReg[ADCS_MOD].presenceStatus = MODULE_PRESENT;
		moduleHealthReg[ADCS_MOD].operationStatus = MODULE_WORKING;
    }
    else
    {
    	printf("sysStart_task(): initAdcs fail\r\n");
		moduleHealthReg[ADCS_MOD].presenceStatus = MODULE_PRESENT;
		moduleHealthReg[ADCS_MOD].operationStatus = MODULE_NOTWORKING;
    }

	//Initialize Composting Module
	if(initCSM() == DG_SUCCESS)
	{
		printf("sysStart_task(): initCSM() passed\r\n");
		moduleHealthReg[CSM_MOD].presenceStatus = MODULE_PRESENT;
		moduleHealthReg[CSM_MOD].operationStatus = MODULE_IDLE;
	}
	else
	{
		printf("sysStart_task(): initCSM() failed\r\n");
		moduleHealthReg[CSM_MOD].presenceStatus = MODULE_PRESENT;
		moduleHealthReg[CSM_MOD].operationStatus = MODULE_NOTWORKING;
	}

	if(initHatcs() == DG_SUCCESS)
	{
		printf("sysStart_task():initHatcs() success!.\r\n");
		moduleHealthReg[HATCS_MOD].presenceStatus = MODULE_PRESENT;
		moduleHealthReg[HATCS_MOD].operationStatus = MODULE_WORKING;
	}
	else
	{
		printf("sysStart_task():initHatcs() failed!.\r\n");
		moduleHealthReg[HATCS_MOD].presenceStatus = MODULE_PRESENT;
		moduleHealthReg[HATCS_MOD].operationStatus = MODULE_NOTWORKING;
	}
	if(initCt() == DG_SUCCESS)
	{
		printf("sysStart_task():initCt success!.\r\n");
		moduleHealthReg[CT_MOD].presenceStatus = MODULE_PRESENT;
		moduleHealthReg[CT_MOD].operationStatus = MODULE_WORKING;
	}
	else
	{
		printf("sysStart_task():initCt failed!.\r\n");
		moduleHealthReg[CT_MOD].presenceStatus = MODULE_PRESENT;
		moduleHealthReg[CT_MOD].operationStatus = MODULE_NOTWORKING;
	}

	if(initTCS() == DG_SUCCESS)
	{
		printf("sysStart_task():initTCS success!.\r\n");
		moduleHealthReg[TCS_MOD].presenceStatus = MODULE_PRESENT;
		moduleHealthReg[TCS_MOD].operationStatus = MODULE_WORKING;
	}
	else
	{
		printf("sysStart_task():initTCS failed!.\r\n");
		moduleHealthReg[TCS_MOD].presenceStatus = MODULE_PRESENT;
		moduleHealthReg[TCS_MOD].operationStatus = MODULE_NOTWORKING;
	}

	//Read the CSM state stored in RTC RAM
	dgDateTime_t updateTime;
	dgCtStateVar_t csmStateVar;

	getChewieStateStore(&updateTime,&csmStateVar);
	printf("sysStart_task():csm state:%d, phase:%d, remDur:%d, schDur:%d\r\n",csmStateVar.state,csmStateVar.curPhase, csmStateVar.remDur, csmStateVar.schDur);
	//Validate csm state variables read from RTC RAM
	if((csmStateVar.state > CSM_STATE_ERROR)||(csmStateVar.curPhase > TRANSFER_PHASE) || (csmStateVar.remDur > csmStateVar.schDur ))
	{
		//Data in RTC RAM is improper. No need to start CSM
		printf("sysStart_task():csmState variables read from RTC RAM is improper. CSM not started\r\n");
		csmStart(SYSSTART_MOD, CSM_STATE_MPHASE, MESOPHILIC_PHASE, 240, WASTE_CAT0);
	}
	else
	{
		//Start csm module
		//vTaskDelay(1000);
		if(moduleHealthReg[CSM_MOD].operationStatus == MODULE_IDLE)
		{
			csmStart(SYSSTART_MOD, csmStateVar.state, csmStateVar.curPhase, csmStateVar.remDur, csmStateVar.curWasteCat);
			moduleHealthReg[CSM_MOD].operationStatus = MODULE_WORKING;
		}
	}
	vTaskDelay(2);  //10 mSec delay for the Limit switch module to get the Lid status after de-bouncing
	//Start Lid Module
	if(moduleHealthReg[LID_MOD].operationStatus == MODULE_IDLE)
	{
		moduleStart(LID_MOD);
		moduleHealthReg[LID_MOD].operationStatus = MODULE_WORKING;
	}



	while(1)
	{
		//Receive event from Queue. Block until event is available
		if(xQueueReceive(sysStartQHandle, &rcvMsg, portMAX_DELAY ) != pdPASS )
		{
			// Queue did not return an event. Hence go back
			continue;
		}

	}
}



int initSysStart(void)
{

	TaskHandle_t sysStartTaskHandle;
	TimerHandle_t sysStartTimerHandle;
	BaseType_t result;

	//Create sysStart task
	result = xTaskCreate(sysStart_task, "sysStart_task", configMINIMAL_STACK_SIZE + 300, NULL, task_PRIORITY, &sysStartTaskHandle);
    if ( result !=    pdPASS)
    {
        printf("sysStart_task creation failed!.\r\n");
        return DG_FAIL;
    }
    //Sys Start moduke requires single shot timer and hence create FreeRTOS SW timer
    sysStartTimerHandle = xTimerCreate("sysStartTimer",100, pdFALSE, (void*)SYSSTART_MOD, dgTimerCallback);
    if(sysStartTimerHandle == NULL)
    {
        printf("Timer creation failed!.\r\n");
    	vTaskDelete(sysStartTaskHandle);
        return DG_FAIL;
    }
    if(registerModule(SYSSTART_MOD, sysStartTaskHandle, sysStartTimerHandle)!= DG_SUCCESS)
    {
    	//Registering the module failed. Hence kill the task and return error
        printf("sysStartModule_task registration failed!.\r\n");
        xTimerDelete(sysStartTimerHandle,100 / portTICK_PERIOD_MS);
    	vTaskDelete(sysStartTaskHandle);

    	return DG_FAIL;
    }
    return DG_SUCCESS;
}

