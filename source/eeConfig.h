/*
 * eeConfig.h
 *
 *  Created on: 12-Nov-2025
 *      Author: Jawahar Arumugam
 */

#ifndef EECONFIG_H_
#define EECONFIG_H_


//EEPROM specific
#define EEPROM_PAGE_SIZE	0x80


/***************************** Config EEPROM Memory Allocation ****************
 *
 * Address Range     |  EEPROM Pages     |   Stored Date
 * ---------------------------------------------------------------------------*
 * 0x0000 - 0x07FF   |     00-15		 | Configuration data - Working Copy
 * 0x0800 - 0x0FFF	 |     16-31         | Configuration data - Factory default
 * 0x1000 - 0x10FF   |     32-33         | ASB HW version, serial number, batch
 * 0x1100 - 0x11FF   |     34-35         | Chewie Installation Information
 *
 ******************************************************************************/

#define CHEWIE_CONFIG_WORKING_START		0x0000
#define CHEWIE_CONFIG_WORKING_SIZE		0x0800




int writeWorkingConfigEEPROM(uint8_t *buffer);
int readWorkingConfigEEPROM(uint8_t *buffer);

#endif /* EECONFIG_H_ */
