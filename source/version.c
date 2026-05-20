/*
 * version.c
 *
 *  Created on: 08-Sep-2024
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

/* Chewie Includes */
#include "dgCommon.h"
#include "modulecom.h"
#include "dgtimer.h"


/*******************Software version Number*************/

char swVer[12] ;
char productModel[32];
char asbHwVersion[16];

void initVersion(void)
{

	strcpy(swVer,"0.6.3" );

	strcpy(productModel,"CHEWIE");
	strcpy(asbHwVersion, "V 2.2");  //TODO: Read from EEPROM
}

void getFwVersion(char * version)
{
	strcpy(version, swVer);
}

void getHwVersion(char * version)
{
	strcpy(version, asbHwVersion);
}


/*******************Software version History*************/

/************************Ver 0.3.0 30-11-2025********************
 * New Features
 * 1. First port from ASB V2.0 to ASB V2.1 Hardware
 *
 *******************************************************************/

/************************Ver 0.3.1 01-12-2025********************
 * New Features
 * 1. CLI Module integrated
 * 2. SysConfig and sysStart modules integrated
 * Known Issues
 * 1. Floating point does not work in printf(). Need to modify the project configuration
 *
 *******************************************************************/

/************************Ver 0.3.2 03-12-2025********************
 * New Features
 * 1. DRV89xx driver integrated.
 * 2. CLI added for controlling the DRV89xx connected devices
 *
 *******************************************************************/

/************************Ver 0.3.3 07-12-2025********************
 * New Features
 * 1. Limit Switch integrated (To be tested)
 * 2. TCS, ADCS integrated (To be tested)
 *
 *******************************************************************/

/************************Ver 0.3.4 08-12-2025********************
 * New Features
 * 1. PSRAM Driver added
 *
 *******************************************************************/

/************************Ver 0.3.5 09-12-2025********************
 * New Features
 * 1. FD_CONFIG CLI added to initialize config EEPROM with default values
 * 2. sht40 initialization is working
 *
 *******************************************************************/

/************************Ver 0.3.6 19-12-2025********************
 * New Features
 * 1. Lid integrated and modified for Lid motor power ON/OFF
 * 2. TC78H660 Motor Driver included
 * 3. HMICmdProc integrated
 *
 *******************************************************************/

/************************Ver 0.3.7 22-12-2025********************
 * New Features
 * 1. Motor Driver pin assignment modified
 *
 *******************************************************************/

/************************Ver 0.3.8 24-12-2025********************
 * New Features
 * 1. All Control functions integrated including CSM
 * 2. Shredder CS: Duration parameter unit changed from seconds to 10s mSec
 * 3. TCS CS: Duration parameter unit changed from seconds to 100s mSec
 *
 *******************************************************************/

/************************Ver 0.3.9 21-01-2026********************
 * Bug fixes
 * 1. Issue: ASM was always loading default config due checksum error. This was
 *      dues to shredder config space exceeded the assigned 128 bytes when shredder
 *      CS duration incresed from 8 bit to 16 bit.
 *   Fix: Reduce the max shredder sequence from 16 to 15. (Temperory fix)
 *
 *******************************************************************/

/************************Ver 0.4.0 28-01-2026********************
 * Bug fixes
 * 1. Issue: When FLAP is not in CLOSE condition, CLOSE event is getting generated
 *      vice versa.
 *   Fix: limitSwitchMod.c:2mSecCallback(): Now checking for LS232DRVR_OPEN.
 *
 * New Features:
 *
 *******************************************************************/

/************************Ver 0.4.1 03-02-2026********************
 *
 * New Features:
 *   - TCS control system task has been modified to support new transfer mechanism
 *     i.e from auger transfer to OPEN/CLOSE mechanism
 *   - CSMSTOP command was not closing Airvalves. That has been fixed
 *
 *******************************************************************/

/************************Ver 0.4.2 05-02-2026********************
 *
 * Bug fixes:
 *   - Shredder.c:executeSHDSeqControl(): When sequence engine is stopped, shdAugur was not stopped
 *     i.e from auger transfer to OPEN/CLOSE mechanism
 *   - CSMSTOP command was not closing Airvalves. That has been fixed
 *   - Lid Module: Values for Timer. Unit mismatch fixed
 *   - GPIOSignals.h: LIDOPEN, LIDCLOSE LS signals interchanged. Fixed
 *
 *******************************************************************/

/************************Ver 0.4.3 12-02-2026********************
 *
 * Modifications:
 *   - hatcs.c:executeActuatorControl(): Code modified to control two fans.
 *      When CWR direction specified in seq -> Heater FAN will be ON
 *      When CCWR direction is specified in seq -> Airvalve FAN will be ON
 *      When OFF in seq -> both FANs will be OFF
 *      Heater FAN connected to SPARE2_RELAY CTRL
 *      Airvalve FAN connected to FAN
 *   - hatcs.c:executeActuatorSafeState(): Included heater fan OFF
 *
 *   - adcs.c: Modified the code to store adcs state in RTC RAM and
 *      read&restore after power returns
 *   - measure.c/measure.h: getAdcsState() and updateAdcsState() added for
 *      storing and retrieving ADCS state in RTC RAM
 *   -shredder.c:shredder_task():adcsStart() method called at the end of shredder
 *
 *******************************************************************/

/************************Ver 0.4.4 14-02-2026********************
 *
 * Bugfix:
 *   - limitswitchMod.c:callback2mSec():Lid Event message is not sent during
 *      initialization. i.e. when starting==1
 *
 *******************************************************************/

/************************Ver 0.4.5 15-02-2026********************
 *
 * Modifications:
 *   - ST Motor and FAN Motor (used for sucking air from DC)inter changed
 *      ST motor has been changed and it requires 2A motor driver
 *   - motorControl.c/h: fanMotorCWR(), fanMotorCCWR(), fanMotorStop() renamed to st
 *   - actuatorCtrl.c/h: stMotorCWR(), stMotorCCWR(), stMotorStop() renamed to fan
 *
 *******************************************************************/


/************************Ver 0.4.6 16-02-2026********************
 *
 * New Features:
 *   - TCS Control parameters can be modified from Config Tool
 *
 *******************************************************************/

/************************Ver 0.4.7 18-03-2026********************
 *
 * New Features:
 *   - SPARE2_RELAY ON OFF Cli command added for testing purpose
 *   - CLI added for getting limit switch status
 *
 * Bug fixes:
 *   - cliProc.c:TCS Start cmd: Due to coding error Sending proximityEvent to lid module. Removed
 *   - transferCS.c: DG_TIMER_EXPIRY event, TCS_STATE_OPENING:dgtimerStart() not returning. Increased stack size
 *
 *******************************************************************/

/************************Ver 0.4.8 23-03-2026********************
 *
 * New Features:
 *   - PWM enabled for 2A Motor driver 1
 *   - Motor ports interchanged: hFAN connected to port1 of 2A motor driver
 *                       		 stMotor driver ports moved to shdAugerMotor ports
 *                       		 shdAugerMotor removed. Not required
 *                       		 Spare2 Relay port is free
 *****************************************************************/

/************************Ver 0.4.8a 27-03-2026********************
 *
 * Bug Fix:
 *   - Shredder.c: LS_FLAP_CLOSE event was not handled in all states.
 *     As a result, flap motor was not stopped on LS_FLAP_CLOSE event for the last sequence.
 *****************************************************************/
//Merged comment conflict - Start
/************************Ver 0.4.7 12-03-2026********************
 *
 * New Features:
 *   - Cloud Integration:RTSS Stream Data API added
 *   - Actuator Status tracking added
 *
 *******************************************************************/

/************************Ver 0.4.8 17-03-2026********************
 *
 * New Features:
 *   - rtc.c:getRTCtimeMMDDHHMM(dgDateTime_t *time): modified to include year and seconds
 *   - actuatorStatusTracker(): Modified to include Lid, StValve, Flap status
 *
 *
 *******************************************************************/
//Merged comment conflict - End

/************************Ver 0.5.0 30-03-2026********************
 * Merged Branch - of cCloudIntegration and LabTestChanges branches
 *
 *
 *******************************************************************/



/************************Ver 0.5.0w 01-04-2026********************
 * 1. hatcs.c: Modified to run hFAN and exhaust FAN simultaneously when airCircCtrl is OUT
 *  Now airCircCtrl decides the exhaust FAN operation and not fanCtrl config
 * 2. transferCS.c: augerMotor rotation direction changed from CCWR to CWR
 *
 *******************************************************************/

 /************************Ver 0.5.1w 02-04-2026********************
 * 1.motorControl.h: Added method to read error status of TC78H660
 * 2.shredder.c:executeSHDSeqControl(): Flap is closed irrespective of the status when SEQ_ENGINE_STOP is received.
 *
 *******************************************************************/

/************************Ver 0.5.2w 02-04-2026********************
* 1.Alarm branch merged
*
*
*******************************************************************/

/************************Ver 0.5.3w 02-04-2026********************
* 1. alarmManager.c: Integrated with sysStart and sensing 2A driver overload error
* 2. hatcs.c: sensing 2A overload status and reseting the driver
* 3. shredder.c: Flap closed whenever the shredder sequence stopped in between
* 4. Alarm raised for Lid error and 2A motor overload
*
********************************************************************/

/************************Ver 0.5.4 08-04-2026********************
* 1. shredder.c: Flapmotor control CCWR and CWR definition changed. Now these commands rotate the
*                Flap motor in specified direction till Stop command is encountered by the execution
*                engine. This feature will enable to configure the flap to rotate continuously during
*                shredding and flushing.
* 2. Added few printf() for debugging and change the order of enabling flag stopFlapFlag
* 3. CliProc.c: DCSprayer command now supports ON/OFF/ONCE. ONCE requires duration parameter
* 4. MKValve (AIROUT) valve support added
********************************************************************/

/************************Ver 0.5.5 21-04-2026********************
* 1. ActuatorStatusTracker has been updated with all the ports except Lid.
* *****************************************************************/


/************************Ver 0.6.0 09-04-2026********************
 * CSM V2
 *  - sysConfig.c/h: Config version added
 *  - sysconfig.h/c: dgCtProcessParam_t definition modified. Default values modified
 *
 *******************************************************************/

/************************Ver 0.6.1 25-04-2026********************
 * CSM V2
 *  - Merged with branch RelayDrivenFlap
 *  - hatcs.c: Modified to support aeration control
 *  - csmMod.c: Code optimization reducing repetition
 *  - Support for CS and Actuator control from Chewie Tool V2.0
 *
 *******************************************************************/

/************************Ver 0.6.2 30-04-2026********************
 * CSM V2
 *  - MKValve timing adjusted
 *  - sysConfig.c: Bug fix: loadActuatorSeq()
 *  - hatcsConfig.c:Bug fix:
 *
 *******************************************************************/

/************************Ver 0.6.3 06-05-2026********************
 * CSM V2
 *  - hatcsmod.c: Mkvalve position stored in RTC RAM and used to restore position
 *                on power-up
 *  - ISP Boot code included. Not tested
 *
 *******************************************************************/
