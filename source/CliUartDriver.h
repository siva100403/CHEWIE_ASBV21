/*
 * rs232CliDriver.h
 *
 *  Created on: 24-Nov-2024
 *      Author: Jawahar Arumugam
 */

#ifndef CLIUARTDRIVER_H_
#define CLIUARTDRIVER_H_



/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define LPFLEXCOMM_INSTANCE1 1U


/* CLI UART PORT Definitions */
#define CLI_LPUART	           	LPUART1
#define CLI_LPUART_CLK_FREQ  	CLOCK_GetLPFlexCommClkFreq(1u)
#define CLI_LPUART_IRQn       	LP_FLEXCOMM1_IRQn
#define CLI_LPUART_IRQHandler 	LP_FLEXCOMM1_IRQHandler


#define CLI_UART_BAUDRATE	9600U
/*! @brief Ring buffer size (Unit: Byte). */
#define DEMO_RING_BUFFER_SIZE 16



/****************Prototypes**************/

int initCliUart();




#endif /* CLIUARTDRIVER_H_ */
