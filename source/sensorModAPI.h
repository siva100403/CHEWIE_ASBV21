/*
 * sensorModAPI.h
 *
 *  Created on: 06-Jan-2025
 *      Author: Jawahar Arumugam
 */

#ifndef SENSORMODAPI_H_
#define SENSORMODAPI_H_

//This structure is used to get the status of Sensor Module

typedef struct sensorModStatus
{
	uint8_t		sensorModStatus;		//Modul status
	uint8_t		sensorStatus;			//Sensor status
	float		curTemp;				//Cur temperature
	float		curHumidity; 			//Cur humidity
	float 		avgTemp;				//Average Temperature
	float		avgHumidity;			//Average Humidity
	float		setTemp;				//Set temperature
	float 		setHumidity;			//set temperature
}dgSensorModStatus_t;




int sensorStart(uint8_t srcModule);
int sensorStop(uint8_t srcModule);
int sensorSetParam(uint8_t srcModule, float setvalueTemp, float setvalueHumidity, float tempRange, float humidityRange);

int getSensorStatus(uint8_t srcModule, float *curTemp, float *curHumidity, float *sT, float *sH, uint8_t *sensorStatus, uint8_t *moduleStatus);
#endif /* SENSORMODAPI_H_ */
