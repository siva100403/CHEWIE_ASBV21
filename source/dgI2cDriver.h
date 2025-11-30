/*
 * dgI2cDriver.h
 *
 *  Created on: 23-Aug-2025
 *      Author: Jawahar Arumugam
 */

/*************************************I2C Driver ***********************************
 * I2C Port Allocation details
 *  Flexcomm FC6 in I2C mode - interfaces MCXN547 with RTC and EEPROM
 *  Flexcomm FC3 in I2C mode - interfaces MCXN547 with Camera and Temperature/Humidity sensor (CCS Board)
 *                           - I2C repeater/buffer TCA9803 is used to connect CCS board
 *                           - TCA9803_EN signal should be used to enable the TCA9803 before accessing CCS board
 *
 **********************************************************************************/

#ifndef DGI2CDRIVER_H_
#define DGI2CDRIVER_H_


/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define I2C_IRQ_PRIORITY    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1)

//I2C buffer for transactions
#define LPI2C_DATA_LENGTH       	33U
#define LPI2C_DATA_LENGTH_EE       128U   // For matching EEPROM page size

//LP Flexcomm related FC6 for RTC and Serial EEPROM
#define LPFLEXCOMM_INSTANCE6 	6U		//LPI2C port for RTC and EEPROM
#define RTC_I2C_MASTER_BASEADDR (LPI2C6)
#define RTC_I2C_MASTER 			((LPI2C_Type *)RTC_I2C_MASTER_BASEADDR)


#define RTC_I2C_BAUDRATE 		100000U 	/*! Transfer baudrate - 100KHz */


//I2C slave devices connected to FC7
#define EEPROM_SLAVE_MEM_ADDR		0x50
#define EEPROM_SLAVE_CTL_ADDR		0x58
#define MCP7940_SLAVE_ADDR			0x6F


//EEPROM Device Specific Constants M24512E
#define EEPROM_PAGE_SIZE		0x80
#define EEPROM_PAGE_SIZE_MASK	0xFF80


/************************** FC3 -I2C Port *************************************/
//LP Flexcomm related FC3 for Camera and Temperature/Humoidity sensor (CCS board)
#define LPFLEXCOMM_INSTANCE3 		3U		//LPI2C for Camera and Humidity/Temperature sensor
#define CAMERA_I2C_MASTER_BASEADDR 	(LPI2C3)
#define CAMERA_I2C_MASTER 			((LPI2C_Type *)CAMERA_I2C_MASTER_BASEADDR)


#define CAMERA_I2C_BAUDRATE 	100000U 	/*! Transfer baudrate - 100kHz */

//I2C slave devices connected to FC3
#define SHT4X_SLAVE_ADDR		0x44



/*******************************************************************************
 * Structure definitions
 ******************************************************************************/

typedef struct i2cHandle
{
	lpi2c_master_handle_t g_m_handle;
	SemaphoreHandle_t  semaphore;
	SemaphoreHandle_t  mutex;
	status_t	status;
}dgI2cHandle_t;


/*******************************************************************************
 * Function prototypes
 ******************************************************************************/

int initRTC_I2C();
int deinitRTC_I2C();
int mcp7940_reg_read(uint8_t reg_addr, uint8_t* reg_value);
int mcp7940_reg_write(uint8_t reg_addr, uint8_t reg_value);

int eeprom_mem_write_within_page(uint16_t addr, uint8_t* buffer, uint8_t size);
int eeprom_mem_read_within_page(uint16_t addr, uint8_t* rdBuffer, uint8_t size);

int initCamera_I2C();
int deinitCamera_I2C();
int sht4x_reg_read(uint8_t reg_addr, uint8_t* reg_value);
int sht4x_reg_write(uint8_t reg_addr, uint8_t reg_value);
int sht4x_setAddress(uint8_t reg_addr);
#endif /* DGI2CDRIVER_H_ */
