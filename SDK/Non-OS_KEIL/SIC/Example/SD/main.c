/*-----------------------------------------------------------------------------
 * 2011/7/7 by CJChen1@nuvoton.com, To run FA95 emulation code with SIC driver default configuration.
 *---------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "wbio.h"
#include "wblib.h"
#include "wbtypes.h"

#include "w55fa95_reg.h"
#include "w55fa95_sic.h"
#include "nvtfat.h"
#include "fmi.h"

/*-----------------------------------------------------------------------------
 * For system configuration
 *---------------------------------------------------------------------------*/

// Define DBG_PRINTF to sysprintf to show more information about testing
#define DBG_PRINTF    sysprintf
//#define DBG_PRINTF(...)

#define OK      TRUE
#define FAIL    FALSE


/*-----------------------------------------------------------------------------
 * For global variables
 *---------------------------------------------------------------------------*/
#define SECTOR_SIZE         512
#define MAX_SECTOR_COUNT    512
#define BUF_SIZE        (SECTOR_SIZE * MAX_SECTOR_COUNT)
__align (32) UINT8 g_ram0[BUF_SIZE];
__align (32) UINT8 g_ram1[BUF_SIZE];

UINT32  totalSector = 0;

extern FMI_SD_INFO_T *pSD0; // define in SIC driver
FMI_SD_INFO_T *pSD;


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
 * ISR of Card detect interrupt for card insert
 *---------------------------------------------------------------------------*/
void isr_card_insert()
{
    UINT32 result;
    sysprintf("--- ISR: card inserted on SD port 0 ---\n\n");
    result = sicSdOpen0();
    if (result < FMI_ERR_ID)
    {
        sysprintf("    Detect card on port %d.\n", 0);
        fmiSD_Show_info(0);
    }
    else if (result == FMI_NO_SD_CARD)
    {
        sysprintf("WARNING: Don't detect card on port %d !\n", 0);
    }
    else
    {
        sysprintf("WARNING: Fail to initial SD/MMC card %d, result = 0x%x !\n", 0, result);
    }
    return;
}


/*-----------------------------------------------------------------------------
 * ISR of Card detect interrupt for card remove
 *---------------------------------------------------------------------------*/
void isr_card_remove()
{
    sysprintf("--- ISR: card removed on SD port 0 ---\n\n");
    sicSdClose0();
    return;
}


/*-----------------------------------------------------------------------------
 * To do test for SD card access. Write, read, and compare data on random sectors.
 *---------------------------------------------------------------------------*/
unsigned int sd_access_test()
{
    UINT32 ii, sectorIndex;
    UINT32 result;
    UINT32 u32SecCnt;
    UINT8  *ptr_g_ram0, *ptr_g_ram1;
    DISK_DATA_T info;

    ptr_g_ram0 = (UINT8 *)((UINT32)g_ram0 | 0x80000000);    // non-cache
    ptr_g_ram1 = (UINT8 *)((UINT32)g_ram1 | 0x80000000);    // non-cache

    for(ii=0; ii<BUF_SIZE; ii++)
    {
        ptr_g_ram0[ii] = rand() & 0xFF;
    }

    // get information about SD card
    fmiGet_SD_info(pSD0, &info);

    while(1)
    {
        sectorIndex = rand() % (info.totalSectorN - MAX_SECTOR_COUNT);
        u32SecCnt = (rand() % MAX_SECTOR_COUNT) + 1;    // cannot be 0!!

        result = sicSdWrite0(sectorIndex, u32SecCnt, (UINT32)ptr_g_ram0);
        // DBG_PRINTF("    Write g_ram0 to SD card, result = 0x%x\n", result);
        // show_hex_data(ptr_g_ram0, 16);

        memset(ptr_g_ram1, 0x5a, u32SecCnt * SECTOR_SIZE);
        result = sicSdRead0(sectorIndex, u32SecCnt, (UINT32)ptr_g_ram1);
        // DBG_PRINTF("    Read g_ram1 to SD card, result = 0x%x\n", result);
        // show_hex_data(ptr_g_ram1, 16);

        if(memcmp(ptr_g_ram0, ptr_g_ram1, u32SecCnt * SECTOR_SIZE) == 0)
        {
            result = OK;
            sysprintf("    Data compare OK at sector %d, sector count = %d\n", sectorIndex, u32SecCnt);
        }
        else
        {
            result = FAIL;
            sysprintf("    ERROR: Data compare ERROR at sector %d, sector count = %d\n", sectorIndex, u32SecCnt);
        }
    }
    return result;
}


/*-----------------------------------------------------------------------------*/

int main(void)
{
    int result, i;

	UINT32 u32ExtFreq;
	UINT32 u32PllOutHz;
	WB_UART_T uart;

    //--- initial UART
    u32ExtFreq = sysGetExternalClock();
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
    /*
    u32ErrCode = sysSetSystemClock(eSYS_UPLL,   //E_SYS_SRC_CLK eSrcClk,
                            240000000,          //UINT32 u32PllKHz,
                            120000000);         //UINT32 u32SysKHz,
    */
    u32PllOutHz = sysGetPLLOutputHz(eSYS_UPLL, u32ExtFreq);
    DBG_PRINTF("PLL out frequency %d Hz\n", u32PllOutHz);

    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);

    srand(time(NULL));

    //--- Initial system clock
#ifdef OPT_FPGA_DEBUG
    sicIoctl(SIC_SET_CLOCK, 27000, 0, 0);               // clock from FPGA clock in
#else
    sicIoctl(SIC_SET_CLOCK, u32PllOutHz/1000, 0, 0);    // clock from PLL
#endif

    //--- Enable AHB clock for SIC/SD/NAND, interrupt ISR, DMA, and FMI engineer
    sicOpen();

    //--- Initial callback function for card detection interrupt
    sicIoctl(SIC_SET_CALLBACK, FMI_SD_CARD, (INT32)isr_card_remove, (INT32)isr_card_insert);

    //--- Initial SD card on port 0
    result = sicSdOpen0();

    if (result < 0)
    {
        sysprintf("ERROR: Open SD card on port %d fail. Return = 0x%x.\n", 0, result);
        sicSdClose0();

        //--- Example code to enable/disable SD0 Card Detect feature.
        sysprintf("SD0 card detect feature is DISABLED by default.\n");
        sysprintf("The card status will always be INSERTED if card detect feature disabled.\n");
        sicIoctl(SIC_GET_CARD_STATUS, (INT32)&i, 0, 0);
        sysprintf("Check SD0 card status ..... is %s\n", i ? "INSERTED" : "REMOVED");

        sysprintf("Now, try to enable SD0 card detect feature ... Done!\n");
        sicIoctl(SIC_SET_CARD_DETECT, TRUE, 0, 0);  // MUST call sicIoctl() BEFORE sicSdOpen0()
        sicSdOpen0();

        i = FALSE;
        sysprintf("Wait SD0 card insert ... \n");
        while (i == FALSE)
        {
            sicIoctl(SIC_GET_CARD_STATUS, (INT32)&i, 0, 0);
        }
        sysprintf("Card inserted !!\n");
        sicSdClose0();

        sysprintf("Now, Try to disable SD0 card detect feature ... Done!\n");
        sicIoctl(SIC_SET_CARD_DETECT, FALSE, 0, 0); // MUST call sicIoctl() BEFORE sicSdOpen0()
        sicSdOpen0();
    }
    else
    {
        totalSector = result;
        sysprintf("    Detect SD card on port 0 with %d sectors.\n", totalSector);
        fmiSD_Show_info(0);
    
        //--- Do test items for SD card on port 0
        DBG_PRINTF("Do basic SD card access test ...\n");
        result = sd_access_test();
        if (result == OK)
            DBG_PRINTF("SD card access test is SUCCESSFUL!!!\n");
        else
            sysprintf("SD card access test is FAIL!!!\n");
    
        sicSdClose0();
    }

    sysprintf("\n===== THE END =====\n");
    return OK;
}
