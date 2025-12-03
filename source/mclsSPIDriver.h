/*
 * mclsSPIDriver.h
 *
 *  Created on: 21-Aug-2025
 *      Author: Jawahar Arumugam
 */

/**********************Motor Control / Level Sensor SPI Module*****************
 *Two motor/solenoid driver ICs (DRV89XX) and two Levels sensor ICs (TDC1000)
 *share the same SPI.
 *Flexcomm Port FC2 is used in SPI mode for interfacing to these devices
 *Following PCS are used for these devices
 * 	- DRV89XX_1 - PCS2
 * 	- DRV89XX_2 - PCS1
 * 	- TDC1000_1 - PCS0
 * 	- TDC1000_2 - PCS3
 *
 ******************************************************************************/


#ifndef MCLSSPIDRIVER_H_
#define MCLSSPIDRIVER_H_

/*******************************************************************************
 * Definitions
 ******************************************************************************/
//LP Flexcomm related
#define MCLS_LPSPI_MASTER_BASEADDR (LPSPI2)
#define TRANSFER_BAUDRATE 2000000U /*! Transfer baudrate - 2MHz */
#define LPFLEXCOMM_INSTANCE2 	2U		//LPSPI port for DRV8912 and TDC1000

//SPI Device IDs
#define DRV89XX_1     	0
#define DRV89XX_2		1
#define DRV89XX_3		2
#define TDC1000_1		3
#define TDC1000_2		4

#define LPSPI_IRQN       (LP_FLEXCOMM2_IRQn)
#define LPSPI_IRQHandler (LP_FLEXCOMM2_IRQHandler)


/*******************************************************************************
 * Structure definitions
 ******************************************************************************/

typedef struct spiHandle
{
	SemaphoreHandle_t  semaphore;		//Semaphore is to synchronize between SPI_ISR and SPI API
	SemaphoreHandle_t  mutex;			//mutex is to ensure only one SPI transfer is active
}dgSpiHandle_t;



/*******************************************************************************
 * Function prototypes
 ******************************************************************************/

int initSPI_MCLS();
int spiReadDrv89xx(uint8_t spiDeviceID, uint8_t regAddr, uint16_t *data);
int spiWriteDrv89xx(uint8_t spiDeviceID, uint8_t regAddr, uint8_t data);

int spiReadTDC1000(uint8_t spiDeviceID, uint8_t regAddr, uint8_t *data);
int spiWriteTDC1000(uint8_t spiDeviceID, uint8_t regAddr, uint8_t data);


#endif /* MCLSSPIDRIVER_H_ */
