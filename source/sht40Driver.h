/*
 * sht40Driver.h
 *
 *  Created on: 25-Aug-2025
 *      Author: Jawahar Arumugam
 */

#ifndef SHT40DRIVER_H_
#define SHT40DRIVER_H_


/***************************** SHT40 Register Addresses ***************************/
#define CMD_MEASURE_TRH_HP			0xFD			//High Precision Temp /Humidity measurement
#define CMD_MEASURE_TRH_MP			0xF6			//Medium Precision Temp /Humidity measurement
#define CMD_MEASURE_TRH_LP			0xE0			//LOW Precision Temp /Humidity measurement
#define CMD_READ_SERIAL_NO			0x89			//Command to read serial number
#define CMD_SOFT_RESET				0x94			//Command to reset SHT40
#define CMD_MEASURE_TRH_WH_200_1S	0x39			//Heater 200mW for 1 Sec and HP Temp /Humidity measurement
#define CMD_MEASURE_TRH_WH_200_P1S	0x39			//Heater 200mW for .1 Sec and HP Temp /Humidity measurement
#define CMD_MEASURE_TRH_WH_110_1S	0x2F			//Heater 110mW for 1 Sec and HP Temp /Humidity measurement
#define CMD_MEASURE_TRH_WH_110_P1S	0x24			//Heater 110mW for .1 Sec and HP Temp /Humidity measurement
#define CMD_MEASURE_TRH_WH_20_1S	0x1E			//Heater 20mW for 1 Sec and HP Temp /Humidity measurement
#define CMD_MEASURE_TRH_WH_20_P1S	0x15			//Heater 20mW for .1 Sec and HP Temp /Humidity measurement



#define TCA9803_DIS()			GPIO_PinWrite(BOARD_INITPINS_TCA9803_EN_GPIO, BOARD_INITPINS_TCA9803_EN_PIN, GPIO_PIN_LOW);
#define TCA9803_ENA()			GPIO_PinWrite(BOARD_INITPINS_TCA9803_EN_GPIO, BOARD_INITPINS_TCA9803_EN_PIN, GPIO_PIN_HIGH);


int readShtSerialNumber(uint32_t *serialNum);
int readShtTempHumidityHighPrecision(float *temperature, float *humidity);
int initSht40(void);
#endif /* SHT40DRIVER_H_ */
