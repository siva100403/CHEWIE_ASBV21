/*
 * rtc.h
 *
 *  Created on: 27-Dec-2024
 *      Author: Jawahar Arumugam
 */

#ifndef RTC_H_
#define RTC_H_


//MCP7940N RTC Register Addresses
#define RTCSEC_REG		0x00
#define RTCMIN_REG		0x01
#define RTCHOUR_REG		0x02
#define RTCWKDAY_REG	0x03
#define RTCDATE_REG		0x04
#define RTCMTH_REG		0x05
#define RTCYEAR_REG		0x06
#define CONTROL_REG		0x07
#define OSCTRIM_REG		0x08
#define PWRUPMIN_REG    0x1C
#define PWRUPHOUR_REG   0x1D
#define PWRUPDATE_REG   0x1E
#define PWRUPMTH_REG    0x1F
#define PWRDWMIN_REG    0x18
#define PWRDWHOUR_REG   0x19
#define PWRDWDATE_REG   0x1A
#define PWRDWMTH_REG    0x1B


//Bit Masks
#define ST_BITSETMASK		0x80 //OR this value to set ST bit
#define VBATEN_BITSETMASK	0x08 //



typedef struct dateTime
{
	uint16_t year;
	uint8_t month;
	uint8_t date;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
}dgDateTime_t;



int mcp7940_init(void);
int getRTCtime(char *string);
int setrtctime(uint8_t year,uint8_t month,uint8_t date,uint8_t hour, uint8_t min,uint8_t sec);
int powerUPTIME_stamp( char *string);
int powerDWTIME_stamp( char *string);
int getRTCtimeMMDDHHMM(dgDateTime_t *time);

#define LOWER_RTC_RAM		0x20
#define HIGHER_RTC_RAM		0x5F

int rtcRAMRead(uint8_t addr, uint8_t *value);
int rtcRAMWrite(uint8_t addr, uint8_t value);

#endif /* RTC_H_ */
