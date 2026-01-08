/*
 * alert.h
 *
 *  Created on: 29-Dec-2024
 *      Author: Jawahar Arumugam
 */

#ifndef ALERT_H_
#define ALERT_H_



/************************************************************************
 * 							Chewie Alert Definitions 					*
 *
 ************************************************************************/

#define ALERT_WASTE_ADDITION_TOO_LONG		0
#define ALERT_TRANSFER_PENDING				1			//This is due to STORAGE_TRAY_FULL or STORAGE_TRAY_OPEN
#define ALERT_STORAGE_TRAY_FULL				2
#define ALERT_STORAGE_TRAY_OPEN				3
#define ALERT_TRANSFER_TIMEOUT				4			//Transfer taking longer than the timeout





void generateAlert(uint16_t alert);


#endif /* ALERT_H_ */
