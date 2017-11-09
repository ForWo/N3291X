#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "w55fa95_reg.h"
#include "wblib.h"
#include "w55fa95_sic.h"
#include "w55fa95_gnand.h"
#include "nvtfat.h"
#include "Font.h"
#include "writer.h"

#ifdef __KLE_DEMO__
    extern void LcmBacklightInit(void);
    extern void LcmBacklightEnable(void);
#endif

extern S_DEMO_FONT s_sDemo_Font;

int font_x=0, font_y=16;
UINT32 u32SkipX;
FMI_SM_INFO_T *pNvtSM0;

#if 1
// Define DBG_PRINTF to sysprintf to show more information about testing.
    #define DBG_PRINTF  sysprintf
#else
    #define DBG_PRINTF(...)
#endif


/**********************************/
__align(32) UINT8 StorageBufferArray[0x50000];
__align(32) UINT8 CompareBufferArray[0x50000];
UINT32 StorageBuffer, CompareBuffer, BufferSize=0;

BOOL volatile bIsAbort = FALSE;

#ifdef __KLE_DEMO__
UINT32 u32TimerChannel = 0;         /* For back light */
void Timer0_300msCallback(void)
{
    LcmBacklightInit();
    LcmBacklightEnable();
    DBG_PRINTF("T300ms callback\n");
    sysClearTimerEvent(TIMER0, u32TimerChannel);
}
#endif


/*-----------------------------------------------------------------------------
 * Check system area and find out the start block of update area.
 * The start block of update area depend on really image file size.
 * INPUT:
 *      pSM: pointer to data stucture of CS0 or CS1
 * OUTPUT:
 *      None
 * RETURN:
 *      >0 : the index of Update Area start block
 *      =0 : invalid Update Area start block since has no image information
 *      <0 : Fail, reture value of nandpread0()
 *---------------------------------------------------------------------------*/
INT searchUpdateAreaStartBlock(FMI_SM_INFO_T *pSM)
{
    int fmiNandSysArea=0;
    int volatile status, imageCount, i, block;
    unsigned int *pImageList = (unsigned int *)CompareBuffer;
    volatile int ii;
    UINT32 startBlock = 0;
    UINT8 *ptr = (UINT8 *)CompareBuffer;
    UINT32 updateAreaStartBlock = 0;

    /* read physical block 0 last page for system area size that TurboWriter write into. */
    for (ii=0; ii<4; ii++)
    {
        status = nandpread0(ii-pSM->uLibStartBlock, pSM->uPagePerBlock-1, (UINT8 *)ptr);
        if (!status)
        {
            if (((*(pImageList+0)) == 0x574255aa) && ((*(pImageList+3)) == 0x57425963))
                break;
        }
    }
    if (status < 0)
        return status;  // nandpread0() fail

    if (((*(pImageList+0)) == 0x574255aa) && ((*(pImageList+3)) == 0x57425963))
    {
        fmiNandSysArea = *(pImageList+1);   // sectors for system area in NAND that TurboWriter write into.
    }
//    DBG_PRINTF("searchUpdateAreaStartBlock(): fmiNandSysArea = %d sectors\n", fmiNandSysArea);

    if ((fmiNandSysArea != 0xFFFFFFFF) && (fmiNandSysArea != 0))
    {
        //--- TurboWriter wrote a valid value about system area size into NAND,
        //      got it and convert unit from sector to block.
        startBlock = (fmiNandSysArea / pSM->uSectorPerBlock) + 1;

        //--- Note by CJChen1@nuvoton.com at 2012/3/14:
        //      startBlock is the block INDEX of first block of data area;
        //      (fmiNandSysArea / pSM->uSectorPerBlock) is the block COUNT of system area;
        //      Since the block index is 0-BASE, the correct value should be
        //          startBlock = (fmiNandSysArea / pSM->uSectorPerBlock) + 0;
        //      NOT + 1.
        //      Howevern, for BACKWARD COMPATIBLE, we should keep this issue and don't modify it.
        //      Please keep in mind that if you reserve n blocks for system area by TurboWriter,
        //          NAND driver will reserve n+1 blocks actually.

        if (fmiNandSysArea % pSM->uSectorPerBlock > 0)
            startBlock++;
//        DBG_PRINTF("searchUpdateAreaStartBlock(): according to fmiNandSysArea, startBlock = %d\n", startBlock);
    }

    //--- always scan the image table to find out the ending block that all images really used.
    //      If the scan result larger than TurboWriter wrote into, use the larger one.
    /* read physical block 0 second last page for image table */
    for (ii=0; ii<4; ii++)
    {
        status = nandpread0(ii-pSM->uLibStartBlock, pSM->uPagePerBlock-2, (UINT8 *)ptr);
        if (!status)
        {
            if (((*(pImageList+0)) == 0x574255aa) && ((*(pImageList+3)) == 0x57425963))
                break;
        }
    }
    if (status < 0)
        return status;

    if (((*(pImageList+0)) == 0x574255aa) && ((*(pImageList+3)) == 0x57425963))
    {
        imageCount = *(pImageList+1);

        /* pointer to image information */
        pImageList = pImageList+4;
        for (i=0; i<imageCount; i++)
        {
            block = (*(pImageList + 1) & 0xFFFF0000) >> 16;     // block INDEX of ending block of image file
            block++;                                            // block INDEX of start block of data area
            if (block > startBlock)    // choose the larger one as start block of data area
                startBlock = block;

            if (block > updateAreaStartBlock)    // choose the larger one as start block of update area
                updateAreaStartBlock = block;

            /* pointer to next image */
            pImageList = pImageList+12;
        }
    }

    DBG_PRINTF("Scan image information, updateAreaStartBlock = %d\n", updateAreaStartBlock);
    return updateAreaStartBlock;
}


/**********************************/
int main()
{
    NDISK_T *ptNDisk;
    INT  status=0;
    CHAR Array1[64];
    UINT Next_Font_Height;
    UINT32 u32ExtFreq;
    WB_UART_T uart;
    UINT32 UpdateAreaStartBlock = 0;    // the index of start block of Updata Area

#if 0
    // show clock setting
    DBG_PRINTF("NandWriter extry.\n");
    DBG_PRINTF("APLL   Clock = %dHz\n", sysGetPLLOutputHz(eSYS_APLL, sysGetExternalClock()));
    DBG_PRINTF("UPLL   Clock = %dHz\n", sysGetPLLOutputHz(eSYS_UPLL, sysGetExternalClock()));
    DBG_PRINTF("System Clock = %dHz\n", sysGetSystemClock());
    DBG_PRINTF("CPU    Clock = %dHz\n", sysGetCPUClock());
    DBG_PRINTF("HCLK1  Clock = %dHz\n", sysGetHCLK1Clock());
    DBG_PRINTF("APB    Clock = %dHz\n", sysGetAPBClock());
#endif

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

    /* enable cache */
    sysDisableCache();
    sysInvalidCache();
    sysEnableCache(CACHE_WRITE_BACK);

    /* check SDRAM size and buffer address */
    StorageBuffer = (UINT32)&StorageBufferArray[0] | 0x80000000;
    CompareBuffer = (UINT32)&CompareBufferArray[0] | 0x80000000;

    /* configure Timer0 for FMI library */
    sysSetTimerReferenceClock(TIMER0, 12000000);
    sysStartTimer(TIMER0, 100, PERIODIC_MODE);

#ifdef __KLE_DEMO__
    u32TimerChannel = sysSetTimerEvent(TIMER0, 30, (PVOID)Timer0_300msCallback);
#endif

    sysprintf("\n=====> W55FA95 NandUpdater Clean Tool (v%d.%d) Begin [%d] <=====\n", MAJOR_VERSION_NUM, MINOR_VERSION_NUM, sysGetTicks(0));

    Draw_Init();
    font_y = g_Font_Height;
    Next_Font_Height = g_Font_Height-6;

    //nvtSMInit();
    sicOpen();
    ptNDisk = (NDISK_T *)malloc(sizeof(NDISK_T));
    nandInit0(ptNDisk);
    //--- always use CS0
    pNvtSM0 = pSM0;     // pSM0 defined by Non-OS SIC driver library

    sprintf(Array1, "Nand:%d(Blk)*%d(Pg)*%d(Size)",
            pNvtSM0->uBlockPerFlash, pNvtSM0->uPagePerBlock, pNvtSM0->nPageSize);
    Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, 0,  _LCM_HEIGHT_ -1-g_Font_Height,Array1);

    BufferSize = pNvtSM0->uPagePerBlock * pNvtSM0->nPageSize;
    if (BufferSize >= 0x50000)
        BufferSize = 0x40000;

    // found the start block of Update Area
    Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, font_x, font_y, "Check Update Area size:");
    u32SkipX = 23;
    UpdateAreaStartBlock = searchUpdateAreaStartBlock(pNvtSM0);
    if (UpdateAreaStartBlock >= pNvtSM0->uLibStartBlock)
    {
        sysprintf("===> ERROR: Update Area (block %d) over the System Area (block %d).\n", UpdateAreaStartBlock, pNvtSM0->uLibStartBlock);
        Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
        bIsAbort = TRUE;
        goto _end_;
    }
    else
        Draw_Status(font_x+ 12*g_Font_Step, font_y, Successful);
    font_y += Next_Font_Height;

    // erase Update Image information block
    Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, font_x, font_y, "Clean Update Image Information:");
    u32SkipX = 31;
    status = nand_block_erase0(UpdateAreaStartBlock - pNvtSM0->uLibStartBlock);
    if (status == 0)
        Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Successful);
    else
    {
        sysprintf("===> ERROR: fail to clean Update Image Information!\n");
        Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
        bIsAbort = TRUE;
    }
    font_y += Next_Font_Height;

_end_:

    sysprintf("=====> Finish [%d] <=====\n", sysGetTicks(0));
    Draw_FinalStatus(bIsAbort);

    while(1);
}
