/*
 * drv89xxDriver.h
 *
 *  Created on: 18-Aug-2025
 *      Author: Jawahar Arumugam
 */

#ifndef DRV89XXDRIVER_H_
#define DRV89XXDRIVER_H_

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define DRV8912_IRQ	         	GPIO40_IRQn
#define DRV8912_IRQ_HANDLER 	GPIO40_IRQHandler

//DRV89XX related
#define SLEEP_DRV89XX()		GPIO_PinWrite(BOARD_INITPINS_SLEEP_DRV8912_GPIO, BOARD_INITPINS_SLEEP_DRV8912_PIN, GPIO_PIN_LOW);
#define WAKEUP_DRV89XX()		GPIO_PinWrite(BOARD_INITPINS_SLEEP_DRV8912_GPIO, BOARD_INITPINS_SLEEP_DRV8912_PIN, GPIO_PIN_HIGH);


//Device ID Verification related
#define DRV8908_ID					0x20
#define DRV8912_ID					0x00
#define DRV89XX_DEVICEID_MASK		0x70
#define NUM_HB_DRV8908				8
#define NUM_HB_DRV8912				12


//Half bridge ID
#define HALFBRIDGE_1		1
#define HALFBRIDGE_2		2
#define HALFBRIDGE_3		3
#define HALFBRIDGE_4		4
#define HALFBRIDGE_5		5
#define HALFBRIDGE_6		6
#define HALFBRIDGE_7		7
#define HALFBRIDGE_8		8
#define HALFBRIDGE_9		9
#define HALFBRIDGE_10		10
#define HALFBRIDGE_11		11
#define HALFBRIDGE_12		12

//OP_CTRL reg mask
#define OP_CTRL_HB1_MASK			0x03
#define OP_CTRL_HB2_MASK			0x0C
#define OP_CTRL_HB3_MASK			0x30
#define OP_CTRL_HB4_MASK			0xC0
#define OP_CTRL_HB5_MASK			0x03
#define OP_CTRL_HB6_MASK			0x0C
#define OP_CTRL_HB7_MASK			0x30
#define OP_CTRL_HB8_MASK			0xC0
#define OP_CTRL_HB9_MASK			0x03
#define OP_CTRL_HB10_MASK			0x0C
#define OP_CTRL_HB11_MASK			0x30
#define OP_CTRL_HB12_MASK			0xC0

//Half Bridge Enable/Disable code
#define HB_HSDIS_LSDIS		0
#define HB_HSDIS_LSENA		1
#define HB_HSENA_LSDIS		2
#define HB_HSENA_LSENA		3

//For Solenoid/Motor connector between OUTx and GND
#define SOLENOID_ON			HB_HSENA_LSDIS
#define SOLENOID_OFF		HB_HSDIS_LSDIS
#define MOTOR_UNIDIR_ON		HB_HSENA_LSDIS
#define MOTOR_UNIDIR_OFF	HB_HSDIS_LSDIS

//Full Bridge Control Code
//MP_HALF_BRIDGE (bits 3,2) MN_HALF_BRIDGE (bits 1,0)
#define MOTOR_COAST		0x00
#define MOTOR_FWD		0x09
#define MOTOR_REV		0x06
#define MOTOR_BRAKE_HS	0x0A
#define MOTOR_BRAKE_LS	0x05
#define MOTOR_COAST_AL	0x0F

/*********************** Register Configuration for DRV8908 *************************/
#define CONFIG_CTRL_VALUE		0x04	//POLD Disabled, OTW report on Fault pin
#define CONFIG_CTRL_CLEAR_FAULT	0x05    //For clearing Fault use this value

#define PWM_CTRL_1_VALUE				0x50   	//PWM disabled for all Hlaf bridges except HB5 and HB7
#define PWM_CTRL_2_VALUE				0xFE	//Disable PWM Generators except Channel 1
#define FW_CTRL_1_VALUE					0x00	//Passive Free wheeling enabled
#define SR_CTRL_1_VALUE					0x00	//0.6V/uSec Slew rate on all channels
#define DRV8908_PWM_MAP_CTRL_3_VALUE	0x00 	//PWM Channel_1 for HB5
#define DRV8908_PWM_MAP_CTRL_4_VALUE	0x00 	//PWM Channel_1 for HB7
#define DRV8908_PWM_FREQ_CTRL_1_VALUE	0x03	//PWM Channel_1 freq 2000Hz
#define DRV8908_PWM_DUTY_CTRL_1_VALUE	0x80	//PWM Channel_Duty 100  0xFF
#define DRV8908_OLD_CTRL_3_VALUE		0x90	//Negative Current OLD enabled
#define DRV8908_OLD_CTRL_1_VALUE		0xFF	//OLD disabled for all H-bridges
//#define OLD_CTRL_1_VALUE				//Enable only for HB being used


/*********************** Register Configuration for DRV8912 *************************/
#define CONFIG_CTRL_VALUE_8912			0x04	//POLD Disabled, OTW report on Fault pin
#define CONFIG_CTRL_CLEAR_FAULT_8912	0x05    //For clearing Fault use this value

#define PWM_CTRL_1_VALUE_8912_2			0x50   	//PWM disabled for all Hlaf bridges except HB5 and HB7
#define PWM_CTRL_2_VALUE_8912_2			0xE0	//PWM Disabled for chl 9-12 and Disable all PWM Generators

#define PWM_MAP_CTRL_2_VALUE_8912_2		0x00 	//PWM Channel_1 for HB5
#define PWM_MAP_CTRL_3_VALUE_8912_2		0x00 	//PWM Channel_1 for HB5

#define PWM_FREQ_CTRL_VALUE_8912_2		0x03	//PWM Channel_1 freq 2000Hz
#define PWM_DUTY_CTRL_1_VALUE_8912_2	0xA0	//PWM Channel_Duty   0xFF

#define PWM_CTRL_1_VALUE_8912			0x00   	//PWM disabled for all Hlaf bridges
#define PWM_CTRL_2_VALUE_8912			0xF0	//PWM Disabled for chl 9-12 and Disable all PWM Generators

#define FW_CTRL_1_VALUE_8912			0x00	//Passive Free wheeling enabled for chl 1-8
#define FW_CTRL_2_VALUE_8912			0x00	//Passive Free wheeling enabled for chl 9-12
#define SR_CTRL_1_VALUE_8912			0x00	//0.6V/uSec Slew rate for channels 1-8
#define SR_CTRL_2_VALUE_8912			0x00	//0.6V/uSec Slew rate for chl 9-12
#define OLD_CTRL_1_VALUE_8912			0xFF	//Disable OLD for 1-8 H-bridges
#define OLD_CTRL_2_VALUE_8912			0x0F	//Disable OLD for 9-12 H-bridges
#define OLD_CTRL_3_VALUE_8912			0x80    //60uSec Deglitch time


//HBridge Allocation structure for solenoid and uni-directional motor
//High side switching considered for solenoid driving and Uni-directional 12VDC motor
//Solenoid connection:
//   - One end to spHbridgeId output
//   - Other end to 12V GND
//Uni-directional motor connection:
//   - Motor Positive Terminal to spHbridgeId output
//   - Motor Negative Terminal to 12V GND
typedef struct solenoidHbAlloc
{
	uint8_t spiDeviceId;			//Which DRV89XX?
	uint8_t solHbridgeId;			//HBridge ID to be used to drive solenoid or Uni-directional motor
}dgSolenoidHbAlloc_t;

typedef struct biDirMotorHbAlloc
{
	uint8_t spiDeviceId;			//Which DRV89XX/
	uint8_t mpHbridgeId;			//HBridge output for motor positive terminal
	uint8_t mnHbridgeId;			//HBridge output for motor negative terminal
}dgBiDirMotorHbAlloc_t;

//Bi-driectional mptor with 2 HB parallel driving
typedef struct biDirMotorPar2HbAlloc
{
	uint8_t spiDeviceId;			//Which DRV89XX/
	uint8_t mp1HbridgeId;			//HBridge output1 for motor positive terminal
	uint8_t mp2HbridgeId;			//HBridge output2 for motor positive terminal
	uint8_t mn1HbridgeId;			//HBridge output1 for motor negative terminal
	uint8_t mn2HbridgeId;			//HBridge output2 for motor negative terminal
}dgBiDirMotorPar2HbAlloc_t;


int initDRV89XX_1();
int initDRV89XX_2();
int initDRV89XX_3();
int initDrv89xxFaulthandler(void);
int halfBridgeCtrl(uint8_t spiDeviceID, uint8_t halfBridgeId, uint8_t enaDis);
int fullBridgeCtrl(uint8_t spiDeviceID, uint8_t mpHalfBridgeId, uint8_t mnHalfBridegId, uint8_t mcControl);
int fullBridgePar2Ctrl(uint8_t spiDeviceID, uint8_t mp1HalfBridgeId, uint8_t mp2HalfBridgeId,uint8_t mn1HalfBridegId, uint8_t mn2HalfBridegId, uint8_t mcControl);
int readRegDRV89XX_1(uint8_t regAddr, uint16_t *regValue);
int readRegDRV89XX_2(uint8_t regAddr, uint16_t *regValue);
int readRegDRV89XX_3(uint8_t regAddr, uint16_t *regValue);
int setDutyCycleChl1_DRV89XX_1(uint8_t duty);
#endif /* DRV89XXDRIVER_H_ */
