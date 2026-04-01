/*
 * alert.c
 *
 *  Created on: 29-Dec-2024
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
#include "alarm.h"


/*----------------------- Global Variables -------------------------*/

dgActiveAlarm_t activeAlarmTable[LAST_ALARM];

/*
const dgAlarmDefintion_t alarmDefTable[]= {
		{HW_ALARM_TEMP_HUM_SESNOR, ALARM_GROUP_HWFAULT, ALARM_CAT_CRITICAL, "Temp/Humidity sensor fault"},
		{HW_ALARM_RTC, ALARM_GROUP_HWFAULT, ALARM_CAT_MINOR, "Real time clock error"},
		{HW_ALARM_EEPROM, ALARM_GROUP_HWFAULT, ALARM_CAT_MAJOR, "Config EEPROM Error"},
		{HW_ALARM_CAMERA, ALARM_GROUP_HWFAULT, ALARM_CAT_MAJOR, "Camera Error"},
		{HW_ALARM_CLI_UART, ALARM_GROUP_HWFAULT, ALARM_CAT_MAJOR, "Camera Error"},
		{HW_ALARM_HMI_UART, ALARM_GROUP_HWFAULT, ALARM_CAT_MAJOR, "Camera Error"},
		{HW_ALARM_DRV89XX_2, ALARM_GROUP_HWFAULT, ALARM_CAT_MAJOR, "Camera Error"},
		{HW_ALARM_TDC1000_1, ALARM_GROUP_HWFAULT, ALARM_CAT_MAJOR, "Camera Error"},
		{HW_ALARM_TDC1000_2, ALARM_GROUP_HWFAULT, ALARM_CAT_MAJOR, "Camera Error"},
		{HW_ALARM_LID, ALARM_GROUP_HWFAULT, ALARM_CAT_MAJOR, "Camera Error"}

};
*/

/*************************** Implementation ***********************************/

int initAlarm()
{

}
//Chewie functional modules raise alert using this method, when they find
//errors or abnormal conditions.
//This module will try to recover from the error/abnormal conditions, if not it notify
//the stake-holders

void generateAlert(uint16_t alert)
{

}
