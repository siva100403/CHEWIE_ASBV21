/*
 * rtc.c
 *
 *  Created on: 27-Dec-2024
 *      Author: Jawahar Arumugam
 */

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"

/*  Standard C Included Files */
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


/* DG Includes */
#include <dgCommon.h>
#include "version.h"
#include "modulecom.h"
#include "dgI2cDriver.h"
#include "rtc.h"

/********************** Global Variables****************/
//Global variables to store the power fail and up time
char chewiePowerfailTime[32];
char chewiePowerUpTime[32];

/***********Macro Definitions and Declarations**********/

uint8_t bcd_convert(uint8_t val);


/******************mcp7940 Init*************************
* This initializes the RTC IC MCP7940. This should be  *
* called on Power-ON reset                             *
********************************************************/
int  mcp7940_init(void)
{
	uint8_t reg_value;

	//Enable ST (=1)bit in RTCSEC register to run 32.768KHz
     // internal osciallator. Since we don't want to corrupt
     // other bits in the register do READ-MODIFY_WRITE

	if (mcp7940_reg_read(RTCSEC_REG, &reg_value) != DG_SUCCESS)
	{
		// Register read failed. Return error
		return(DG_FAIL);
	}
	//Set ST bit to 1 to enable 32.768KHz Osc
	reg_value = reg_value | ST_BITSETMASK;

	//Write the value back
	if(mcp7940_reg_write(RTCSEC_REG, reg_value) != DG_SUCCESS)
	{
		// Register read failed. Return error
		return(DG_FAIL);
	}

	//Read the power fail time
	if(powerDWTIME_stamp(chewiePowerfailTime) == DG_SUCCESS)
	{
		printf("mcp7940_init(): PowefailTime: %s\r\n",chewiePowerfailTime );
	}
	else
	{
		printf("mcp7940_init(): PowerfailTime read fail\r\n");
		chewiePowerfailTime[0]=0;
	}

	//Read power up time and store
	if(powerUPTIME_stamp(chewiePowerUpTime) == DG_SUCCESS)
	{
		printf("mcp7940_init(): PowerupTime: %s\r\n",chewiePowerUpTime);
	}
	else
	{
		printf("mcp7940_init(): PowerupTime read fail\r\n");
		chewiePowerUpTime[0]=0;
	}



	//Enable VBATEN bit in RTCWKDAY register to enable
	// battery backup. Do READ_MODIFY_WRITE
	if (mcp7940_reg_read(RTCWKDAY_REG, &reg_value) != DG_SUCCESS)
	{
		// Register read failed. Return error
		return(DG_FAIL);
	}
	if((reg_value & VBATEN_BITSETMASK) == 0)
	{

		//Set VABTEN bit to 1 to enable Battery backup
		reg_value = reg_value | VBATEN_BITSETMASK;

		//Write the value back
		if(mcp7940_reg_write(RTCWKDAY_REG, reg_value) != DG_SUCCESS)
		{
			// Register read failed. Return error
			return(DG_FAIL);
		}
	}
	return(DG_SUCCESS);

}


/*********************************getRTCtime()************************************
 *Function Description: This function reads the RTC registers to get the time
 * in BCD format, convert into string ( yyyy-mm-dd hh:mm:ss format) and returns
 * RTC IC can support only 2 digit year (0 to 99). "20" is added in the front before
 * forming string.
 *Input Parameters:
 * char pointer - with memory allocated with min size of 21 bytes for returing
 *                time and date in the above format
 * Return value:
 *        returns DG_SUCCESS if success
 *        returns DG_FAIL if there is a error reading RTC registers
 *********************************************************************************/

int getRTCtime(char *string)
{
	int ret;
	uint8_t reg_value;
	int i=0;
	string[i] = '2';
	i++;
	string[i] = '0';
	i++;

		/*year register */
		ret=mcp7940_reg_read(RTCYEAR_REG, &reg_value);
		if(ret == DG_SUCCESS)
		{
			//printf("%d\n",ret);
			string[i]= (char)((reg_value >> 4) + (0x30));
			i++;
			string[i]= ((reg_value & 0x0f) + (0x30));
			i++;
		}
		else
		{
			return(DG_FAIL);
		}
		string[i] = '-';
		i++;
		/*month register */
		ret=mcp7940_reg_read(RTCMTH_REG, &reg_value);

		if(ret == DG_SUCCESS)
		{
			string[i]= (char)(((reg_value & 0x10)>> 4) + (0x30));
			i++;
			string[i]= ((reg_value & 0x0f) + (0x30));
			i++;
		}
		else
		{
			return(DG_FAIL);
		}
		string[i++] = '-';

		/*date register */
		ret=mcp7940_reg_read(RTCDATE_REG , &reg_value); //
		if(ret == DG_SUCCESS)
		{
			string[i++]= (char)(((reg_value & 0x30)>> 4) + (0x30)); // 0011 0000
			string[i++]= ((reg_value & 0x0f) + 0x30);
		}
		else
		{
			return(DG_FAIL);
		}
		string[i++] = ' ';
		/*hour register */
		ret=mcp7940_reg_read(RTCHOUR_REG , &reg_value);
		if(ret == DG_SUCCESS)
		{
			 string[i++]= (char)(((reg_value & 0x30) >> 4) + (0x30)); //0011 0000
			 string[i++] = ((reg_value & 0x0f) + 0x30);
		}
		else
		{
			return(DG_FAIL);
		}
		string[i++] = ':';
		/*minute  register */
		ret=mcp7940_reg_read(RTCMIN_REG , &reg_value);
		if(ret == DG_SUCCESS)
		{
			string[i++]=(char)(((reg_value & 0x70) >> 4) + (0x30)); //0001 0000
			string[i++] =((reg_value & 0x0f) + 0x30);
		}
		else
		{
			return(DG_FAIL);
		}
		string[i++] = ':';
		/*sec register */
		ret=mcp7940_reg_read(RTCSEC_REG , &reg_value);
		if(ret == DG_SUCCESS)
		{
			string[i++]= (char)(((reg_value & 0x70) >> 4) + (0x30));
			string[i++]=((reg_value & 0x0f) + 0x30);
		}
		else
		{
			return(DG_FAIL);
		}

		string[i++]='\0';

		return (DG_SUCCESS);
}



/*********************************getRTCtimeMMDDHHMM()************************************
 *Function Description: This function reads the RTC registers to get the time
 * in dgDateTime format
 *Input Parameters:
 * char pointer - with memory allocated with min size of 21 bytes for returing
 *                time and date in the above format
 * Return value:
 *        returns DG_SUCCESS if success
 *        returns DG_FAIL if there is a error reading RTC registers
 *********************************************************************************/

int getRTCtimeMMDDHHMM(dgDateTime_t *time)
{
	int ret;
	uint8_t month, date, hour, minutes;

	if(time==NULL)
	{
		return DG_INVALID_PARAM;
	}

	/*month register */
	ret=mcp7940_reg_read(RTCMTH_REG, &month);
	if(ret != DG_SUCCESS)
	{
		return(DG_FAIL);
	}


	/*date register */
	ret=mcp7940_reg_read(RTCDATE_REG , &date); //
	if(ret != DG_SUCCESS)
	{
		return(DG_FAIL);
	}
	/*hour register */
	ret=mcp7940_reg_read(RTCHOUR_REG , &hour);
	if(ret != DG_SUCCESS)
	{
		return(DG_FAIL);
	}
	/*minute  register */
	ret=mcp7940_reg_read(RTCMIN_REG , &minutes);
	if(ret != DG_SUCCESS)
	{
		return(DG_FAIL);
	}

	//Formating the time
	time->minute = (minutes & 0x0F) + ((minutes >>4) & 0x07)*10;
	if(hour & 0x40)
	{
		//12 hour format
		time->hour = (hour & 0x0F) + ((hour >>4) & 0x01)*10;
		if(hour & 0x20)
		{
			//PM, hence add 12
			time->hour +=12;
		}
	}
	else
	{
		//24 hr format
		time->hour = (hour & 0x0F) + ((hour >>4) & 0x03)*10;
	}

	time->date = (date & 0x0F) + ((date>>4) & 0x03)*10;

	time->month = (month & 0x0F) + ((month>>4) & 0x03)*10;

	return (DG_SUCCESS);
}


/*********************************setRTCtime()************************************
 *Function Description: This function can be used to set the correct time in RTC.
 * Input parameters are validated to chekc whether they are within range, converted
 * into BCD format before writing into the RTC register
 *Input Parameters:
 * year, month, date, hour, min and sec- as 8 bit unsigned integer
 * Return value:
 *        returns DG_SUCCESS if success
 *        returns DG_FAIL if there is a error in writing RTC registers or invalid paramters
 *********************************************************************************/

int setrtctime(uint8_t year,uint8_t month,uint8_t date,uint8_t hour, uint8_t min,uint8_t sec)
 {

	uint8_t year_bcd, month_bcd, date_bcd, hour_bcd, min_bcd, sec_bcd;

	//Validating year; We expect only the last two digts of the year. First two digits are assumed
	// to be 20.
	if(year<=99)
	{
		year_bcd = bcd_convert(year);
	}
	else
	{
		return(DG_FAIL);
	}
	//Validating month
	if(month>=1 && month<=12)
	{
		month_bcd = bcd_convert(month);
	}
	else
	{
		return(DG_FAIL);
	}

	//Validate date and write. We are validating whether the date is 31 or less. We are not validating
	// date based on month and leap year.
	// Strict validation based on month and leap year will be done later
	if(date>=1 && date<=31)
	{
		date_bcd = bcd_convert(date);
	}
	else
	{
		return(DG_FAIL);
	}

	//Validating hour. It is assumed that time is in 24 hour format
	if(hour<=23)
	{
		hour_bcd = bcd_convert(hour);
	}
	else
	{
		return(DG_FAIL);
	}

	//Validating minutes
	if(min<=59)
	{
		min_bcd = bcd_convert(min);
	}
	else
	{
		return(DG_FAIL);
	}
	//Validating seconds and setting ST bit before writing. If ST bit is zero RTC will not count
	if(sec<=59)
	{
		sec_bcd = bcd_convert(sec);
		sec_bcd=(sec_bcd | 0x80);
	}
	else
	{
		return(DG_FAIL);
	}


	if(mcp7940_reg_write(RTCYEAR_REG, year_bcd) != DG_SUCCESS)
	{
		return(DG_FAIL);
	}
	if (mcp7940_reg_write(RTCMTH_REG, month_bcd)!= DG_SUCCESS)
	{
		return(DG_FAIL);
	}
	if(mcp7940_reg_write(RTCDATE_REG, date_bcd) != DG_SUCCESS)
	{
		return(DG_FAIL);
	}
	if (mcp7940_reg_write(RTCHOUR_REG, hour_bcd)!= DG_SUCCESS)
	{
		return(DG_FAIL);
	}
	if(mcp7940_reg_write(RTCMIN_REG, min_bcd) != DG_SUCCESS)
	{
		return(DG_FAIL);
	}
	if (mcp7940_reg_write(RTCSEC_REG, sec_bcd) != DG_SUCCESS)
	{
		return(DG_FAIL);
	}
	return(DG_SUCCESS);
}


uint8_t bcd_convert(uint8_t val)
{
	if(val>=100)
	{
		//Error to be handled
		/*
*/
	}

	else
	{
		val= (((val/10)<<4)+(val%10));
	}

	return val;
}

/******************POWER_UP/DOWN FUNCTIONS**********************/
//Read powerDWTIME_stamp() first and then call powerUPTIME_stamp().
//powerUPTIME_stamp() method clears the timestamp registers at the end

int powerUPTIME_stamp( char *string)
{
	int ret;
	uint8_t reg_value;
	int i=0;
	string[i] = '2';
	i++;
	string[i] = '0';
	i++;


		/*year register */
		ret=mcp7940_reg_read(RTCYEAR_REG, &reg_value);
		if(ret == DG_SUCCESS)
		{
			//printf("%d\n",ret);
			string[i]= (char)((reg_value >> 4) + (0x30));
			i++;
			string[i]= ((reg_value & 0x0f) + (0x30));
			i++;
		}
		else
		{
			return(DG_FAIL);
		}
		string[i] = '-';
		i++;
		/*month register */
		ret=mcp7940_reg_read(PWRUPMTH_REG, &reg_value);

		if(ret == DG_SUCCESS)
		{
			string[i]= (char)(((reg_value & 0x10)>> 4) + (0x30));
			i++;
			string[i]= ((reg_value & 0x0f) + (0x30));
			i++;
		}
		else
		{
			return(DG_FAIL);
		}

		string[i++] = '-';

		/*date register */
		ret=mcp7940_reg_read(PWRUPDATE_REG, &reg_value); //
		if(ret == DG_SUCCESS)
		{
			string[i++]= (char)(((reg_value & 0x30)>> 4) + (0x30)); // 0011 0000
			string[i++]= ((reg_value & 0x0f) + 0x30);
		}
		else
		{
			return(DG_FAIL);
		}
		string[i++] = ' ';
		/*hour register */
		ret=mcp7940_reg_read(PWRUPHOUR_REG, &reg_value);
		if(ret == DG_SUCCESS)
		{
			 string[i++]= (char)(((reg_value & 0x30) >> 4) + (0x30)); //0011 0000
			 string[i++] = ((reg_value & 0x0f) + 0x30);
		}
		else
		{
			return(DG_FAIL);
		}
		string[i++] = ':';
		/*minute  register */
		ret=mcp7940_reg_read(PWRUPMIN_REG, &reg_value);
		if(ret == DG_SUCCESS)
		{
			string[i++]=(char)(((reg_value & 0x70) >> 4) + (0x30)); //0001 0000
			string[i++] =((reg_value & 0x0f) + 0x30);
		}
		else
		{
			return(DG_FAIL);
		}
		string[i++] = ':';
		/*sec register */
		ret=mcp7940_reg_read(RTCSEC_REG , &reg_value);
		if(ret == DG_SUCCESS)
		{
			string[i++]= (char)(((reg_value & 0x70) >> 4) + (0x30));
			string[i++]=((reg_value & 0x0f) + 0x30);
		}
		else
		{
			return(DG_FAIL);
		}
		string[i++]='\0';
		printf("%s\n",string);

		ret=mcp7940_reg_read(RTCWKDAY_REG, &reg_value);
		if(ret == DG_SUCCESS)
		{
			reg_value = reg_value & 0xef;
			ret=mcp7940_reg_write(RTCWKDAY_REG,reg_value);

		}
		else
		{
			return(DG_FAIL);
		}
		return DG_SUCCESS;
}
/*************POWER DOWN**************/
int powerDWTIME_stamp( char *string)
{
	int ret;
	uint8_t reg_value;
	int i=0;
	string[i] = '2';
	i++;
	string[i] = '0';
	i++;

	/*year register */
	ret=mcp7940_reg_read(RTCYEAR_REG, &reg_value);
	if(ret == DG_SUCCESS)
	{
		//printf("%d\n",ret);
		string[i]= (char)((reg_value >> 4) + (0x30));
		i++;
		string[i]= ((reg_value & 0x0f) + (0x30));
		i++;
	}
	else
	{
		return(DG_FAIL);
	}
	string[i] = '-';
	i++;
	/*month register */
	ret=mcp7940_reg_read(PWRDWMTH_REG, &reg_value);

	if(ret == DG_SUCCESS)
	{
		string[i]= (char)(((reg_value & 0x10)>> 4) + (0x30));
		i++;
		string[i]= ((reg_value & 0x0f) + (0x30));
		i++;
	}
	else
	{
		return(DG_FAIL);
	}
	string[i++] = '-';

	/*date register */
	ret=mcp7940_reg_read(PWRDWDATE_REG, &reg_value); //
	if(ret == DG_SUCCESS)
	{
		string[i++]= (char)(((reg_value & 0x30)>> 4) + (0x30)); // 0011 0000
		string[i++]= ((reg_value & 0x0f) + 0x30);
	}
	else
	{
		return(DG_FAIL);
	}
	string[i++] = ' ';
	/*hour register */
	ret=mcp7940_reg_read(PWRDWHOUR_REG, &reg_value);
	if(ret == DG_SUCCESS)
	{
		string[i++]= (char)(((reg_value & 0x30) >> 4) + (0x30)); //0011 0000
		string[i++] = ((reg_value & 0x0f) + 0x30);
	}
	else
	{
		return(DG_FAIL);
	}
	string[i++] = ':';
	/*minute  register */
	ret=mcp7940_reg_read(PWRDWMIN_REG, &reg_value);
	if(ret == DG_SUCCESS)
	{
		string[i++]=(char)(((reg_value & 0x70) >> 4) + (0x30)); //0001 0000
		string[i++] =((reg_value & 0x0f) + 0x30);
	}
	else
	{
		return(DG_FAIL);
	}
	string[i++] = ':';
	/*sec register: Seconds are not stored by MCP7940N. Hence make it zero*/
	string[i++] = '0';
	string[i++] = '0';

/*	ret=mcp7940_reg_read(RTCSEC_REG , &reg_value);
	if(ret == DG_SUCCESS)
	{
		string[i++]= (char)(((reg_value & 0x70) >> 4) + (0x30));
		string[i++]=((reg_value & 0x0f) + 0x30);
	}
	else
	{
		return(DG_FAIL);
	}*/

	string[i++]='\0';
	//printf("%s\n",string);
	return DG_SUCCESS;
}



int rtcRAMRead(uint8_t addr, uint8_t *value)
{
	//Validate addr
	if((addr < LOWER_RTC_RAM)||(addr > HIGHER_RTC_RAM))
	{
		return DG_INVALID_PARAM;
	}
	return mcp7940_reg_read( addr,  value);
}

int rtcRAMWrite(uint8_t addr, uint8_t value)
{
	//Validate addr
	if((addr < LOWER_RTC_RAM)||(addr > HIGHER_RTC_RAM))
	{
		return DG_INVALID_PARAM;
	}
	return mcp7940_reg_write(addr,  value);
}
