/*
 * hmiUartDriver.h
 *
 *  Created on: 14-Aug-2025
 *      Author: Jawahar Arumugam
 */

#ifndef HMIUARTDRIVER_H_
#define HMIUARTDRIVER_H_



/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define LPFLEXCOMM_INSTANCE0 0U


/* HMI UART PORT Definitions */
#define HMI_LPUART	           LPUART0
#define HMI_LPUART_CLK_FREQ   CLOCK_GetLPFlexCommClkFreq(0u)
#define HMI_LPUART_IRQn       LP_FLEXCOMM0_IRQn
#define HMI_LPUART_IRQHandler LP_FLEXCOMM0_IRQHandler


#define HMI_UART_BAUDRATE	115200U


/****************Prototypes**************/

int initHMIUart();
void interruptHMI(uint8_t reason);
uint8_t getIntReason(void);

#endif /* HMIUARTDRIVER_H_ */
