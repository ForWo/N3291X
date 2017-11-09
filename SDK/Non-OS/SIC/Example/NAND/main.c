/***************************************************************************
 * Copyright (c) 2011 Nuvoton Technology. All rights reserved.
 *
 * FILENAME
 *     main.c
 * DESCRIPTION
 *     The main file for SIC/NAND demo code.
 * FUNCTIONS
 *     None
 **************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "wbio.h"
#include "wblib.h"
#include "wbtypes.h"
#include "w55fa95_reg.h"
#include "w55fa95_sic.h"


/*-----------------------------------------------------------------------------
 * For system configuration
 *---------------------------------------------------------------------------*/

// Define DBG_PRINTF to sysprintf to show more information about testing.
#define DBG_PRINTF    sysprintf
//#define DBG_PRINTF(...)

#define OK      TRUE
#define FAIL    FALSE


/*-----------------------------------------------------------------------------
 * For global variables
 *---------------------------------------------------------------------------*/
NDISK_T *ptMassNDisk;
NDISK_T MassNDisk;

// Define number and size for data buffer
#define SECTOR_SIZE     512
#define BUF_SIZE    (SECTOR_SIZE*16*16*2)
__align (32) UINT8 g_ram0[BUF_SIZE];
__align (32) UINT8 g_ram1[BUF_SIZE];


/*-----------------------------------------------------------------------------
 * show data by hex format
 *---------------------------------------------------------------------------*/
void show_hex_data(unsigned char *ptr, unsigned int length)
{
    unsigned int line_len = 8;
    unsigned int i;

    for (i=0; i<length; i++)
    {
        if (i % line_len == 0)
            DBG_PRINTF("        ");
        DBG_PRINTF("0x%02x ", *(ptr+i));
        if (i % line_len == line_len-1)
            DBG_PRINTF("\n");
    }
    if (i % line_len != 0)
        DBG_PRINTF("\n");
}


/*-----------------------------------------------------------------------------
 * To do test for NAND access. Write, read, and compare data.
 *---------------------------------------------------------------------------*/
int nand_access_test()
{
    int ii;
    UINT32  result;
    UINT32  block, page;
    UINT8   *ptr_g_ram0, *ptr_g_ram1;

#ifdef CACHE_ON
    ptr_g_ram0 = g_ram0;
    ptr_g_ram1 = g_ram1;
#else
    ptr_g_ram0 = (UINT8 *)((UINT32)g_ram0 | 0x80000000);    // non-cache
    ptr_g_ram1 = (UINT8 *)((UINT32)g_ram1 | 0x80000000);    // non-cache
#endif

    //--- initial random data, select random block index and page index
    for(ii=0; ii<ptMassNDisk->nPageSize; ii++)
    {
        ptr_g_ram0[ii] = rand() & 0xFF;
    }
    // select random block except first 4 blocks.
    //      First 4 blocks is booting block and control by IBR. Don't touch it.
    block = (rand() % (ptMassNDisk->nBlockPerZone - 4)) + 4;
    page  = rand() % ptMassNDisk->nPagePerBlock;

    //--- do write and read back test
    result = nand_block_erase0(block);
    if (result != 0)
    {
        // Erase block fail. Could be bad block. Ignore it.
        DBG_PRINTF("    Ignore since nand_block_erase0() fail, block = %d, page = %d, result = 0x%x\n", block, page, result);
        return OK;
    }

    result = nandpwrite0(block, page, ptr_g_ram0);
    DBG_PRINTF("    Write g_ram0 to NAND, result = 0x%x\n", result);
    show_hex_data(ptr_g_ram0, 16);

    memset(ptr_g_ram1, 0x5a, BUF_SIZE);
    result = nandpread0(block, page, ptr_g_ram1);
    DBG_PRINTF("    Read NAND to g_ram1,  result = 0x%x\n", result);
    show_hex_data(ptr_g_ram1, 16);

    //--- compare data
    if(memcmp(ptr_g_ram0, ptr_g_ram1, ptMassNDisk->nPageSize) == 0)
    {
        DBG_PRINTF("    Data compare OK at block %d page %d\n", block, page);
        return OK;
    }
    else
    {
        DBG_PRINTF("    ERROR: Data compare ERROR at block %d page %d\n", block, page);
        return FAIL;
    }
}


/*-----------------------------------------------------------------------------*/
int main(void)
{
    int result;

    UINT32 u32ExtFreq;
    UINT32 u32PllOutHz;
    WB_UART_T uart;

    //--- initial system clock
/*
    // for FA95 syslib. CANNOT RUN on ICE
    sysSetSystemClock(eSYS_UPLL,    // Specified the system clock come from external clock, APLL or UPLL
                      192000000,    // Specified the APLL/UPLL clock, unit Hz
                      192000000);   // Specified the system clock, unit Hz
    sysSetCPUClock(192000000);      // unit Hz
    sysSetAPBClock( 48000000);      // unit Hz
*/

    //--- initial UART
    u32ExtFreq = sysGetExternalClock();     // KHz unit
    sysUartPort(1);
    uart.uiFreq = u32ExtFreq;   //use APB clock
    uart.uiBaudrate = 115200;
    uart.uiDataBits = WB_DATA_BITS_8;
    uart.uiStopBits = WB_STOP_BITS_1;
    uart.uiParity = WB_PARITY_NONE;
    uart.uiRxTriggerLevel = LEVEL_1_BYTE;
    uart.uart_no = WB_UART_0;
    sysInitializeUART(&uart);

    sysSetLocalInterrupt(ENABLE_FIQ_IRQ);

    u32PllOutHz = sysGetPLLOutputHz(eSYS_UPLL, u32ExtFreq);
    DBG_PRINTF("PLL out frequency %d Hz\n", u32PllOutHz);

    //--- enable cache feature
    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);

    srand(time(NULL));

    //--- initial system clock
    sysSetTimerReferenceClock(TIMER0, 12000000);    // External Crystal
    sysStartTimer(TIMER0, 100, PERIODIC_MODE);      // 100 ticks/per sec ==> 1tick/10ms
#ifdef OPT_FPGA_DEBUG
    sicIoctl(SIC_SET_CLOCK, 27000, 0, 0);               // clock from FPGA clock in
#else
    sicIoctl(SIC_SET_CLOCK, u32PllOutHz/1000, 0, 0);    // clock from PLL
#endif

    //--- initial SIC/NAND driver for port 0
    sicOpen();

    ptMassNDisk = (NDISK_T*)&MassNDisk;
    if (nandInit0((NDISK_T *)ptMassNDisk))
    {
        sysprintf("ERROR: NAND initial fail !!\n");
        return FAIL;
    }

    //--- do basic NAND access test
    DBG_PRINTF("Do basic NAND access test ...\n");
    result = nand_access_test();
    if (result == OK)
        DBG_PRINTF("NAND access test is SUCCESSFUL!!!\n");
    else
        DBG_PRINTF("NAND access test is FAIL!!!\n");

    sysprintf("\n===== THE END =====\n\n");
    return OK;
}

