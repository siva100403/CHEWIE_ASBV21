/*
 * psramDriver.c
 *
 *  Created on: 07-Dec-2025
 *      Author: Jawahar Arumugam
 */

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"

/* Standard C includes */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* NXP includes */
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
//#include "fsl_debug_console.h"
#include "fsl_spc.h"
#include "fsl_lpi2c.h"
#include "fsl_lpuart.h"
#include "fsl_device_registers.h"
#include "fsl_lpspi.h"
#include "fsl_utick.h"
#include "fsl_flexspi.h"


/* Chewie Includes */
#include "dgCommon.h"
#include "version.h"
#include "modulecom.h"
#include "psramDriver.h"
#include "dgtimer.h"
#include "GPIOSignals.h"
#include "seqControlCommon.h"
#include "dgI2cDriver.h"
#include "eeConfig.h"
#include "rtc.h"
#include "ASB_HMI_common.h"
#include "sysStart.h"
#include "dgUartDriverCommon.h"
#include "CliUartDriver.h"
#include "hmiUartDriver.h"
#include "cliProc.h"
#include "sysConfig.h"
#include "mclsSPIDriver.h"
#include "drv89xxDriver.h"
#include "drv89xxRegisters.h"
#include "mclsSPIDriver.h"
#include "drv89xxDriver.h"
#include "drv89xxRegisters.h"
#include "actuatorCtrl.h"
#include "sensorMod.h"
#include "augerAPI.h"
#include "shredder.h"
#include "shredderAPI.h"
#include "adcs.h"
#include "limitSwitchMod.h"
#include "transferCS.h"



/*------------------- PSRAM LUT sequence definitions ----------------*/

#ifndef FSL_FEATURE_FLEXSPI_LUT_DEPTH
#define FSL_FEATURE_FLEXSPI_LUT_DEPTH 64U   // 64 32-bit LUT entries
#endif

#define PSRAM_LUT_SEQ_COUNT  (FSL_FEATURE_FLEXSPI_LUT_DEPTH / 4U)


#define PSRAM_LUT_SEQ_IDX_READ        0
#define PSRAM_LUT_SEQ_IDX_WRITE       1
#define PSRAM_LUT_SEQ_IDX_READ_ID     2
#define PSRAM_LUT_SEQ_IDX_RESET       3
#define PSRAM_LUT_SEQ_IDX_QPI_READ    4
#define PSRAM_LUT_SEQ_IDX_QPI_WRITE   5
#define PSRAM_LUT_SEQ_IDX_ENTER_QPI   6
#define PSRAM_LUT_SEQ_IDX_EXIT_QPI    7
#define PSRAM_LUT_SEQ_IDX_MR_READ     8
#define PSRAM_LUT_SEQ_IDX_MR_WRITE    9
#define PSRAM_LUT_LAST				  10

/* Device opcodes (APS1604M) */
enum
{
    PSRAM_CMD_RESET_EN   = 0x66,
    PSRAM_CMD_RESET      = 0x99,
    PSRAM_CMD_ENTER_QPI  = 0x35,
    PSRAM_CMD_EXIT_QPI   = 0xF5,
    PSRAM_CMD_FAST_READQ = 0xEB, /* QPI fast read */
    PSRAM_CMD_WRITEQ     = 0x38, /* QPI quad write */
    PSRAM_CMD_MR_READ    = 0xB5,
    PSRAM_CMD_MR_WRITE   = 0xB1,
	PSRAM_CMD_FAST_READ	 = 0x0B,
	PSRAM_CMD_READID	 = 0x9F,
	PSRAM_CMD_READ		 = 0x03,
	PSRAM_CMD_WRITE		 = 0x02,
};
//FSL_FEATURE_FLEXSPI_LUT_DEPTH
/* LUT table: 64 entries (16 sequences * 4 instructions) */
static const uint32_t s_psramLUT[64] =
{
    /* ----------------- READ (FAST, 0x0B) ----------------- */
    [4 * PSRAM_LUT_SEQ_IDX_READ + 0] =
        FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR,  kFLEXSPI_1PAD, PSRAM_CMD_FAST_READ,
                        kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 24),
    [4 * PSRAM_LUT_SEQ_IDX_READ + 1] =
        FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DUMMY_SDR, kFLEXSPI_1PAD, 8,
                        kFLEXSPI_Command_READ_SDR,  kFLEXSPI_1PAD, 0),
    [4 * PSRAM_LUT_SEQ_IDX_READ + 2] =
    				FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0,
    						kFLEXSPI_Command_STOP,  kFLEXSPI_1PAD, 0),
    [4 * PSRAM_LUT_SEQ_IDX_READ + 3] = 0,

    /* ----------------- WRITE (QPI, 0x38 linear) --------- */
    [4 * PSRAM_LUT_SEQ_IDX_WRITE + 0] =
        FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR,  kFLEXSPI_1PAD, PSRAM_CMD_WRITEQ,
                        kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_4PAD, 24),
    [4 * PSRAM_LUT_SEQ_IDX_WRITE + 1] =
        FLEXSPI_LUT_SEQ(kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_4PAD, 0,
                        kFLEXSPI_Command_STOP,      kFLEXSPI_4PAD, 0),
    [4 * PSRAM_LUT_SEQ_IDX_WRITE + 2] = 0,
    [4 * PSRAM_LUT_SEQ_IDX_WRITE + 3] = 0,

    /* ----------- Read ID (optional, SPI 1-1-1) ----------- */
	[4 * PSRAM_LUT_SEQ_IDX_READ_ID + 0] =
	        	FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR,  kFLEXSPI_1PAD, PSRAM_CMD_READID,
	        					kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 24),
	[4 * PSRAM_LUT_SEQ_IDX_READ_ID + 1] =
	            FLEXSPI_LUT_SEQ(kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0,
	            				kFLEXSPI_Command_STOP,  kFLEXSPI_1PAD, 0),
	[4 * PSRAM_LUT_SEQ_IDX_READ_ID + 2] = 0,
	[4 * PSRAM_LUT_SEQ_IDX_READ_ID + 3] = 0,


    /* -------------- Software Reset (RSTEN+RST) ----------- */
    /* RSTEN (0x66) + RST (0x99) on 1-1-1 SPI */
    [4 * PSRAM_LUT_SEQ_IDX_RESET + 0] =
        FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, PSRAM_CMD_RESET_EN,
                        kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, PSRAM_CMD_RESET),
    [4 * PSRAM_LUT_SEQ_IDX_RESET + 1] = 0,
    [4 * PSRAM_LUT_SEQ_IDX_RESET + 2] = 0,
    [4 * PSRAM_LUT_SEQ_IDX_RESET + 3] = 0,

	/* ---- 4: QPI FAST READ (0xEB) – 4-4-4 ---- */
	[4 * PSRAM_LUT_SEQ_IDX_QPI_READ + 0] =
	    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR,       kFLEXSPI_4PAD, PSRAM_CMD_FAST_READQ,
	                    kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_4PAD, 24),  /* 24-bit addr */

	[4 * PSRAM_LUT_SEQ_IDX_QPI_READ + 1] =
	    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DUMMY_SDR, kFLEXSPI_4PAD, 6,   /* 6 dummy cycles */
	                    kFLEXSPI_Command_READ_SDR,  kFLEXSPI_4PAD, 0),  /* data, size from xfer.dataSize */

	[4 * PSRAM_LUT_SEQ_IDX_QPI_READ + 2] = 0,
	[4 * PSRAM_LUT_SEQ_IDX_QPI_READ + 3] = 0,

	/* ---- 5: QPI WRITE (0x38) – 4-4-4 ---- */
	[4 * PSRAM_LUT_SEQ_IDX_QPI_WRITE + 0] =
	    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR,       kFLEXSPI_4PAD, PSRAM_CMD_WRITEQ,
	                    kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_4PAD, 24),

	[4 * PSRAM_LUT_SEQ_IDX_QPI_WRITE + 1] =
	    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_4PAD, 0,
	                    kFLEXSPI_Command_STOP,      kFLEXSPI_4PAD, 0),

	[4 * PSRAM_LUT_SEQ_IDX_QPI_WRITE + 2] = 0,
	[4 * PSRAM_LUT_SEQ_IDX_QPI_WRITE + 3] = 0,

	/* ---- 6: ENTER QPI (0x35) – still in SPI, 1-1-1 ---- */
	[4 * PSRAM_LUT_SEQ_IDX_ENTER_QPI + 0] =
	    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR,       kFLEXSPI_1PAD, PSRAM_CMD_ENTER_QPI,
	                    kFLEXSPI_Command_STOP,      kFLEXSPI_1PAD, 0),
	[4 * PSRAM_LUT_SEQ_IDX_ENTER_QPI + 1] = 0,
	[4 * PSRAM_LUT_SEQ_IDX_ENTER_QPI + 2] = 0,
	[4 * PSRAM_LUT_SEQ_IDX_ENTER_QPI + 3] = 0,

	/* ---- 7: EXIT QPI (0xF5) – 4-4-4 (device in QPI) ---- */
	[4 * PSRAM_LUT_SEQ_IDX_EXIT_QPI + 0] =
	    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR,       kFLEXSPI_4PAD, PSRAM_CMD_EXIT_QPI,
	                    kFLEXSPI_Command_STOP,      kFLEXSPI_4PAD, 0),
	[4 * PSRAM_LUT_SEQ_IDX_EXIT_QPI + 1] = 0,
	[4 * PSRAM_LUT_SEQ_IDX_EXIT_QPI + 2] = 0,
	[4 * PSRAM_LUT_SEQ_IDX_EXIT_QPI + 3] = 0,

	/* ---- 8: MODE REGISTER READ (0xB5) – 4-4-4 ---- */
	[4 * PSRAM_LUT_SEQ_IDX_MR_READ + 0] =
	    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR,       kFLEXSPI_4PAD, PSRAM_CMD_MR_READ,
	                    kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_4PAD, 8),   /* MA[3:0] */

	[4 * PSRAM_LUT_SEQ_IDX_MR_READ + 1] =
	    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_READ_SDR,  kFLEXSPI_4PAD, 1,   /* 1 byte MR data */
	                    kFLEXSPI_Command_STOP,      kFLEXSPI_4PAD, 0),

	[4 * PSRAM_LUT_SEQ_IDX_MR_READ + 2] = 0,
	[4 * PSRAM_LUT_SEQ_IDX_MR_READ + 3] = 0,

	/* ---- 9: MODE REGISTER WRITE (0xB1) – 4-4-4 ---- */
	[4 * PSRAM_LUT_SEQ_IDX_MR_WRITE + 0] =
	    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR,       kFLEXSPI_4PAD, PSRAM_CMD_MR_WRITE,
	                    kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_4PAD, 8),

	[4 * PSRAM_LUT_SEQ_IDX_MR_WRITE + 1] =
	    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_4PAD, 1,
	                    kFLEXSPI_Command_STOP,      kFLEXSPI_4PAD, 0),

	[4 * PSRAM_LUT_SEQ_IDX_MR_WRITE + 2] = 0,
	[4 * PSRAM_LUT_SEQ_IDX_MR_WRITE + 3] = 0,

};






/*------------------------- FLEXSPI0 for PSRAM -------------------*/
#define PSRAM_FLEXSPI          FLEXSPI0
#define PSRAM_FLEXSPI_PORT     kFLEXSPI_PortA1
#define PSRAM_SIZE_KB          (2048U)    /* 2 MB PSRAM */

static flexspi_device_config_t s_psramDevCfg =
{
    .flexspiRootClk       = 0,   /* fill at runtime with CLOCK_GetFlexspiClkFreq() */
    .isSck2Enabled        = false,
    .flashSize            = PSRAM_SIZE_KB,

    .CSIntervalUnit       = kFLEXSPI_CsIntervalUnit1SckCycle,
    .CSInterval           = 2,
    .CSHoldTime           = 1,
    .CSSetupTime          = 1,
    .dataValidTime        = 1,   /* tune if timing issues show up */

    .columnspace          = 0,
    .enableWordAddress    = false,

    /* AHB read/write use the QPI read/write sequences */
    .AWRSeqIndex          = PSRAM_LUT_SEQ_IDX_QPI_WRITE,
    .AWRSeqNumber         = 1,
    .ARDSeqIndex          = PSRAM_LUT_SEQ_IDX_QPI_READ,
    .ARDSeqNumber         = 1,

    .AHBWriteWaitUnit     = kFLEXSPI_AhbWriteWaitUnit2AhbCycle,
    .AHBWriteWaitInterval = 0,
};



/*-------------------------- Implementation-----------------------*/



status_t PSRAM_Init(void)
{
    flexspi_config_t config;

    //Flexspi frequency 150MHz / 2 = 75MHz
   CLOCK_SetClkDiv(kCLOCK_DivFlexspiClk, 2U);
   CLOCK_AttachClk(kPLL0_to_FLEXSPI); //!< Switch FLEXSPI to PLL0

    /* 1. Get default FlexSPI config and tweak it */
    FLEXSPI_GetDefaultConfig(&config);
    config.rxSampleClock                  = kFLEXSPI_ReadSampleClkLoopbackInternally;
    config.ahbConfig.enableAHBPrefetch    = true;
    config.ahbConfig.enableAHBCachable    = true;

    FLEXSPI_Init(PSRAM_FLEXSPI, &config);

    //To be safe pass current FlexSPI clk freq
    s_psramDevCfg.flexspiRootClk = CLOCK_GetFlexspiClkFreq();

    FLEXSPI_SetFlashConfig(PSRAM_FLEXSPI, &s_psramDevCfg, PSRAM_FLEXSPI_PORT);

    /* 3. Program LUT */
    FLEXSPI_UpdateLUT(PSRAM_FLEXSPI, 0, s_psramLUT, PSRAM_LUT_LAST*4 /*PSRAM_LUT_SEQ_COUNT*/);

    /* 4. Do a software reset of PSRAM (optional but nice) */
    //psram_ipcmd(PSRAM_LUT_SEQ_IDX_RESET, 0, NULL, 0);
    flexspi_transfer_t xfer = {0};

    xfer.deviceAddress = 0;
    xfer.port          = PSRAM_FLEXSPI_PORT;
    xfer.cmdType       = kFLEXSPI_Command;
    xfer.SeqNumber     = 1;
    xfer.seqIndex = PSRAM_LUT_SEQ_IDX_RESET;
    xfer.data          = NULL;
    xfer.dataSize      = 0;

    if (FLEXSPI_TransferBlocking(PSRAM_FLEXSPI, &xfer) != kStatus_Success)
        return kStatus_Fail;

    uint8_t id[8];
    PSRAM_ReadID(id);
    printf("psramDriver.c:psram id = %d,%d,%d,%d,%d,%d,%d,%d\r\n",id[0],id[1],id[2],id[3],id[4],id[5],id[6],id[7] );
    //enter QPI mode
    PSRAM_EnterQPI();
    /* 5. Clear AHB buffers and make sure bus is idle */
    FLEXSPI_SoftwareReset(PSRAM_FLEXSPI);


    return kStatus_Success;
}

static status_t psram_send_cmd(uint8_t lutIndex)
{
    flexspi_transfer_t xfer = {0};

    xfer.deviceAddress = 0;
    xfer.port          = PSRAM_FLEXSPI_PORT;
    xfer.cmdType       = kFLEXSPI_Command;
    xfer.seqIndex      = lutIndex;
    xfer.SeqNumber     = 1;
    xfer.data          = NULL;
    xfer.dataSize      = 0;

    return FLEXSPI_TransferBlocking(PSRAM_FLEXSPI, &xfer);
}


int PSRAM_EnterQPI(void)
{
    status_t st = psram_send_cmd(PSRAM_LUT_SEQ_IDX_ENTER_QPI);
    if (st != kStatus_Success)
    {
        printf("PSRAM_EnterQPI: error = %d\r\n", st);
        return DG_FAIL;
    }

    /* small guard time after mode change */
    //vTaskDelay(pdMS_TO_TICKS(1));
    return DG_SUCCESS;
}

int PSRAM_ExitQPI(void)
{
    status_t st = psram_send_cmd(PSRAM_LUT_SEQ_IDX_EXIT_QPI);
    if (st != kStatus_Success)
    {
        printf("PSRAM_ExitQPI: error = %d\r\n", st);
        return DG_FAIL;
    }

    //vTaskDelay(pdMS_TO_TICKS(1));
    return DG_SUCCESS;
}



int PSRAM_ReadID(void* buffer )
{
    if (buffer == NULL)
    {
        return DG_INVALID_PARAM;
    }

    flexspi_transfer_t xfer = {0};
    status_t retValue;

    xfer.deviceAddress = 0x0000;
    xfer.port          = PSRAM_FLEXSPI_PORT;
    xfer.cmdType       = kFLEXSPI_Read;
    xfer.seqIndex      = PSRAM_LUT_SEQ_IDX_READ_ID;
    xfer.SeqNumber     = 1;
    xfer.data          = (uint32_t *)buffer;   /* FLEXSPI expects uint32_t* */
    xfer.dataSize      = 8;

    retValue = FLEXSPI_TransferBlocking(PSRAM_FLEXSPI, &xfer);
    if(retValue == kStatus_Success)
    {
    	return DG_SUCCESS;
    }
    else
    {
    	printf("psramDriver.c:psram_readID: error code=%d\r\n", retValue);
    	return DG_FAIL;
    }

}



int PSRAM_Read_QPI(uint32_t addr, void *dst, size_t len)
{
     //Device is byte-addressable; limit to 2 MB
    if ((addr + len) > (PSRAM_SIZE_KB * 1024U))
    {
        return DG_INVALID_PARAM;
    }

    flexspi_transfer_t xfer = {0};
    status_t retValue;

    xfer.deviceAddress = addr;
    xfer.port          = PSRAM_FLEXSPI_PORT;
    xfer.cmdType       = kFLEXSPI_Read;
    xfer.seqIndex      = PSRAM_LUT_SEQ_IDX_QPI_READ;
    xfer.SeqNumber     = 1;
    xfer.data          = (uint32_t *)dst;   /* FLEXSPI expects uint32_t* */
    xfer.dataSize      = len;

    retValue = FLEXSPI_TransferBlocking(PSRAM_FLEXSPI, &xfer);
    if(retValue == kStatus_Success)
    {
    	return DG_SUCCESS;
    }
    else
    {
    	printf("psramDriver.c:psram_read_QPI: error code=%d\r\n", retValue);
    	return DG_FAIL;
    }
}

int PSRAM_Read(uint32_t addr, void *dst, size_t len)
{
     //Device is byte-addressable; limit to 2 MB
    if ((addr + len) > (PSRAM_SIZE_KB * 1024U))
    {
        return DG_INVALID_PARAM;
    }

    flexspi_transfer_t xfer = {0};
    status_t retValue;

    xfer.deviceAddress = addr;
    xfer.port          = PSRAM_FLEXSPI_PORT;
    xfer.cmdType       = kFLEXSPI_Read;
    xfer.seqIndex      = PSRAM_LUT_SEQ_IDX_READ;
    xfer.SeqNumber     = 1;
    xfer.data          = (uint32_t *)dst;   /* FLEXSPI expects uint32_t* */
    xfer.dataSize      = len;

    retValue = FLEXSPI_TransferBlocking(PSRAM_FLEXSPI, &xfer);
    if(retValue == kStatus_Success)
    {
    	return DG_SUCCESS;
    }
    else
    {
    	printf("psramDriver.c:psram_read: error code=%d\r\n", retValue);
    	return DG_FAIL;
    }
}

int PSRAM_Write_QPI(uint32_t addr, const void *src, size_t len)
{
    if ((addr + len) > (PSRAM_SIZE_KB * 1024U))
    {
        return DG_INVALID_PARAM;
    }

    flexspi_transfer_t xfer = {0};
    status_t retValue;

    xfer.deviceAddress = addr;
    xfer.port          = PSRAM_FLEXSPI_PORT;
    xfer.cmdType       = kFLEXSPI_Write;
    xfer.seqIndex      = PSRAM_LUT_SEQ_IDX_QPI_WRITE;
    xfer.SeqNumber     = 1;
    xfer.data          = (uint32_t *)src;   /* FLEXSPI expects uint32_t* */
    xfer.dataSize      = len;

    retValue = FLEXSPI_TransferBlocking(PSRAM_FLEXSPI, &xfer);
    if(retValue == kStatus_Success)
    {
    	return DG_SUCCESS;
    }
    else
    {
    	printf("psramDriver.c:psram_write_QPI: error code=%d\r\n", retValue);
    	return DG_FAIL;
    }

}


int PSRAM_Write(uint32_t addr, const void *src, size_t len)
{
    if ((addr + len) > (PSRAM_SIZE_KB * 1024U))
    {
        return DG_INVALID_PARAM;
    }

    flexspi_transfer_t xfer = {0};
    status_t retValue;

    xfer.deviceAddress = addr;
    xfer.port          = PSRAM_FLEXSPI_PORT;
    xfer.cmdType       = kFLEXSPI_Write;
    xfer.seqIndex      = PSRAM_LUT_SEQ_IDX_WRITE;
    xfer.SeqNumber     = 1;
    xfer.data          = (uint32_t *)src;   /* FLEXSPI expects uint32_t* */
    xfer.dataSize      = len;

    retValue = FLEXSPI_TransferBlocking(PSRAM_FLEXSPI, &xfer);
    if(retValue == kStatus_Success)
    {
    	return DG_SUCCESS;
    }
    else
    {
    	printf("psramDriver.c:psram_write: error code=%d\r\n", retValue);
    	return DG_FAIL;
    }

}


int PSRAM_TestByteOrder(volatile uint8_t *psram)
{
    for (uint32_t i = 0; i < 16U; i++)
    {
        psram[i] = (uint8_t)i;
    }

    for (uint32_t i = 0; i < 16U; i++)
    {
        if (psram[i] != (uint8_t)i)
        {
            printf("Byte order test failed @ 0x%08X: "
                   "expected %02X, got %02X\r\n",
                   i, (unsigned)i, psram[i]);
            return DG_FAIL;
        }
    }
    printf("psramDriver.c:PSRAM_TestByteOrder() Test passed\r\n");
    return DG_SUCCESS;
}

int PSRAM_TestAccessWidth(uint32_t psramBase)
{
    volatile uint8_t  *p8;
    volatile uint16_t *p16;
    volatile uint32_t *p32;
    uint32_t i;

    p8  = (volatile uint8_t *)psramBase;
    p16 = (volatile uint16_t *)psramBase;
    p32 = (volatile uint32_t *)psramBase;

    /* ---------------------------------
     * Write 16 bytes using 8-bit writes
     * --------------------------------- */
    for (i = 0U; i < 16U; i++)
    {
        p8[i] = (uint8_t)i;
    }

    __DSB();
    __ISB();

    /* ---------------------------------
     * Verify using 8-bit reads
     * --------------------------------- */
    printf("8-bit reads:\r\n");

    for (i = 0U; i < 16U; i++)
    {
        printf("%02X ", (unsigned int)p8[i]);
        if((p8[i]) != i)
        {
        	return DG_FAIL;
        }
    }

    printf("\r\n");

    /* ---------------------------------
     * Verify using 16-bit reads
     * --------------------------------- */
    printf("16-bit reads:\r\n");

    for (i = 0U; i < 8U; i++)
    {
        uint16_t expected;
        uint16_t actual;

        expected = (uint16_t)((2U * i) |
                            ((2U * i + 1U) << 8U));

        actual = p16[i];

        printf("[%lu] Expected=0x%04X Actual=0x%04X\r\n",
               (unsigned long)i,
               (unsigned int)expected,
               (unsigned int)actual);

        if (actual != expected)
        {
            printf("16-bit access test FAILED\r\n");
            return DG_FAIL;
        }
    }

    /* ---------------------------------
     * Verify using 32-bit reads
     * --------------------------------- */
    printf("32-bit reads:\r\n");

    for (i = 0U; i < 4U; i++)
    {
        uint32_t base;
        uint32_t expected;
        uint32_t actual;

        base = 4U * i;

        expected =
              (base + 0U)
            | ((base + 1U) << 8U)
            | ((base + 2U) << 16U)
            | ((base + 3U) << 24U);

        actual = p32[i];

        printf("[%lu] Expected=0x%08lX Actual=0x%08lX\r\n",
               (unsigned long)i,
               (unsigned long)expected,
               (unsigned long)actual);

        if (actual != expected)
        {
            printf("32-bit access test FAILED\r\n");
            return DG_FAIL;
        }
    }

    printf("PSRAM access-width test PASSED\r\n");

    return DG_SUCCESS;
}
