/*
 * sensorMod.h
 *
 *  Created on: 02-Jan-2025
 *      Author: Jawahar Arumugam
 */

#ifndef SENSORMOD_H_
#define SENSORMOD_H_

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define SENSOR_SAMPLING_TIME		4 			// in seconds
//#define SENSOR_SAMPLING_TICKS   	(SENSOR_SAMPLING_TIME)/ portTICK_PERIOD_MS

//Sensor module state

#define SENSOR_STATE_IDLE      	0
#define SENSOR_STATE_READY		1
#define SENSOR_STATE_ACTIVE		2

/*******************************************************************************
 *  Sensor states
 ******************************************************************************/
#define HAT_SENSOR_TEMP_WR_HUM_WR	0
#define HAT_SENSOR_TEMP_WR_HUM_AR	1
#define HAT_SENSOR_TEMP_WR_HUM_BR	2
#define HAT_SENSOR_TEMP_AR_HUM_WR	3
#define HAT_SENSOR_TEMP_AR_HUM_AR	4
#define HAT_SENSOR_TEMP_AR_HUM_BR	5
#define HAT_SENSOR_TEMP_BR_HUM_WR	6
#define HAT_SENSOR_TEMP_BR_HUM_AR	7
#define HAT_SENSOR_TEMP_BR_HUM_BR	8
#define AIR_IN_CTRL_SEQ				9



typedef struct sensorThreshold
{
	float		targetTemp;       	    	//set temperature
	float		targetTempLow;				//Lower temperature range
											// tempTarget - tempRange
	float		targetTempHigh;				//Higher temperature range
											// tempTarget+tempRange
	float		targetHumidity;				//Set Humidity value
	float		targetHumidityLow;			//Lower Humidity value
	float		targetHumidityHigh;			//Higher Humidity value
}dgSensorThreshold_t;


//This structure will be used to pass parameters THCS module.

typedef struct sensorParam
{
	float		tempSetValue;           	//set temperature
	float		humiditySetValue;			//Set Humidity value
	float		tempRange;					//Temperature range +/-
	float		humidityRange;				//Humidity range +/-

} dgSensorParam_t;



int initSensor(void);

#endif /* SENSORMOD_H_ */
