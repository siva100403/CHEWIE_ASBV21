/*
 * motorControl.h
 *
 *  Created on: 11-Dec-2024
 *      Author: Jawahar Arumugam
 */

#ifndef MOTORCONTROL_H_
#define MOTORCONTROL_H_


#define GPIO_PIN_ZERO	0
#define GPIO_PIN_ONE	1

/*********************************************************************
 ****  		 MACRO to control Motor controller IC pins 			******
 *********************************************************************/

//TC78H660 control

#define TC78H660_ACTIVE()		GPIO_PinWrite(BOARD_INITPINS_TC78H660_STBY_GPIO, BOARD_INITPINS_TC78H660_STBY_PIN, GPIO_PIN_ONE);
#define TC78H660_STBY()			GPIO_PinWrite(BOARD_INITPINS_TC78H660_STBY_GPIO, BOARD_INITPINS_TC78H660_STBY_PIN, GPIO_PIN_ZERO);

#define TC78H660_PHASE_MODE()	GPIO_PinWrite(BOARD_INITPINS_TC78H660_MODE_GPIO, BOARD_INITPINS_TC78H660_MODE_PIN, GPIO_PIN_ONE);
#define TC78H660_IN_MODE()		GPIO_PinWrite(BOARD_INITPINS_TC78H660_MODE_GPIO, BOARD_INITPINS_TC78H660_MODE_PIN, GPIO_PIN_ZERO);

//#define MOTOR1_START()		GPIO_PinWrite(BOARD_INITPINS_MOTOR1_ONOFF_CTRL_GPIO, BOARD_INITPINS_MOTOR1_ONOFF_CTRL_PIN, GPIO_PIN_ONE);
//#define MOTOR1_STOP()			GPIO_PinWrite(BOARD_INITPINS_MOTOR1_ONOFF_CTRL_GPIO, BOARD_INITPINS_MOTOR1_ONOFF_CTRL_PIN, GPIO_PIN_ZERO);

#define MOTOR1_FORWARD()		GPIO_PinWrite(BOARD_INITPINS_MOTOR1_DIR_CTRL_GPIO, BOARD_INITPINS_MOTOR1_DIR_CTRL_PIN, GPIO_PIN_ONE);
#define MOTOR1_REVERSE()		GPIO_PinWrite(BOARD_INITPINS_MOTOR1_DIR_CTRL_GPIO, BOARD_INITPINS_MOTOR1_DIR_CTRL_PIN, GPIO_PIN_ZERO);

#define MOTOR2_START()			GPIO_PinWrite(BOARD_INITPINS_MOTOR2_ONOFF_CTRL_GPIO, BOARD_INITPINS_MOTOR2_ONOFF_CTRL_PIN, GPIO_PIN_ONE);
#define MOTOR2_STOP()			GPIO_PinWrite(BOARD_INITPINS_MOTOR2_ONOFF_CTRL_GPIO, BOARD_INITPINS_MOTOR2_ONOFF_CTRL_PIN, GPIO_PIN_ZERO);

#define MOTOR2_FORWARD()		GPIO_PinWrite(BOARD_INITPINS_MOTOR2_DIR_CTRL_GPIO, BOARD_INITPINS_MOTOR2_DIR_CTRL_PIN, GPIO_PIN_ONE);
#define MOTOR2_REVERSE()		GPIO_PinWrite(BOARD_INITPINS_MOTOR2_DIR_CTRL_GPIO, BOARD_INITPINS_MOTOR2_DIR_CTRL_PIN, GPIO_PIN_ZERO);



/*********************************************************************
 ****  		 Function Prototype						 			******
 *********************************************************************/

void TC78H660_Stby(void);

void TC78H660_Active(void);



void augerMotorCWR(void);
void augerMotorCCWR(void);
void augerMotorStop(void);

void pwmStop();
void pwmStart(uint8_t dutyCycle);
void initCTimer3();

void hFanCWR(uint8_t duty);
void hFanCCWR(uint8_t duty);
void hFanStop(void);

void flapMotorCWR();
void flapMotorCCWR();
void flapMotorStop();

#endif /* MOTORCONTROL_H_ */
