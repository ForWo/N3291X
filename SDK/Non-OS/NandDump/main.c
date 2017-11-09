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

#define MAJOR_VERSION_NUM   1
#define MINOR_VERSION_NUM   2

/*-----------------------------------------------------------------------------
 * for system configuration
 *---------------------------------------------------------------------------*/
// define DBG_PRINTF to sysprintf to show more information about emulation testing
#define DBG_PRINTF    sysprintf
//#define DBG_PRINTF(...)

#define DUMP_FILENAME_PREFIX    "C:\\NAND1_2_SD"

#define OK      TRUE
#define FAIL    FALSE

/*-----------------------------------------------------------------------------
 * Globle Variables
 *---------------------------------------------------------------------------*/
#define BUF_SIZE    (1024*8+512)
__align (32) UINT8 g_ram0[BUF_SIZE];
__align (32) UINT8 g_ram1[BUF_SIZE];
UINT8 *ptr_g_ram0;
UINT8 *ptr_g_ram1;

// for NAND
NDISK_T *ptMassNDisk;
NDISK_T MassNDisk;

char gDump_filename[64];


int initial_SD_card_0()
{
    int result;
    result = sicSdOpen0();  // default disk name is "C:"
    sysprintf("Detect and initial SD/MMC card on port 0, result = 0x%08X ... \n", result);
    if (result < FMI_ERR_ID)
    {
        sysprintf("    SD card detectd on port 0.\n");
        fmiSD_Show_info(0);
    }
    else if (result == FMI_NO_SD_CARD)
    {
        sysprintf("ERROR: Not found any SD card on port 0 !\n");
        return -1;
    }
    else
    {
        sysprintf("ERROR: Fail to initial SD/MMC card on port 0, result = 0x%x !\n", result);
        return -1;
    }
    return 0;
}


int open_dump_file(int file_no)
{
    int file_handler;
    char unicode_path[128];

    sprintf(gDump_filename, "%s_%d.dat", DUMP_FILENAME_PREFIX, file_no);
//    fsAsciiToUnicode((VOID*)"C:\\NAND1_2_SD.dat", (VOID*)unicode_path, TRUE);
    fsAsciiToUnicode((VOID*)gDump_filename, (VOID*)unicode_path, TRUE);
    file_handler = fsOpenFile(unicode_path, 0, O_CREATE | O_TRUNC);
    if (file_handler < 0)
        sysprintf("ERROR: open file %s fail and return 0x%X !! BYE !!\n", gDump_filename, file_handler);
    else
        sysprintf("Dump data to file %s ...\n", gDump_filename);
    return file_handler;
}


int close_dump_file(int file_handler)
{
    int result;
    result = fsCloseFile(file_handler);
    if (result == FS_OK)
    {
        sysprintf("\nClose file %s.\n", gDump_filename);
        return 0;
    }
    else
    {
        sysprintf("ERROR: close file %s fail and return 0x%X !! BYE !!\n", gDump_filename, result);
        return -1;
    }
}


/*-----------------------------------------------------------------------------*/
int main(void)
{
    unsigned int result, ib, ip, ir;
    DateTime_T local_time;
    UINT32 u32ExtFreq, u32PllOutHz;
    FMI_SM_INFO_T *pSM = 0;
    char buffer[48];
    INT file_handler;
    INT nWriteLen;
    INT totalBlockNumber;
    INT nandDumpUnit;
    INT spareSize;
    UINT32 blockSize, freeSize, diskSize;
    UINT32 dump_file_no, dump_file_size;

    DBG_PRINTF("\n=====> W55FA95 NandDump (v%d.%d) Begin [%d] <=====\n", MAJOR_VERSION_NUM, MINOR_VERSION_NUM, sysGetTicks(0));

    //--- enable system cache feature
    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);

    //--- initial system timer
    u32ExtFreq = sysGetExternalClock();     // Hz unit
	sysSetTimerReferenceClock(TIMER0, u32ExtFreq); 			// External Crystal
	sysStartTimer(TIMER0, 100, PERIODIC_MODE);				// 100 ticks/per sec ==> 1tick/10ms

	//--- initialize SIC/FMI (Flash memory interface controller)
    u32PllOutHz = sysGetPLLOutputHz(eSYS_UPLL, u32ExtFreq);
    DBG_PRINTF("UPLL out frequency %d Hz\n", u32PllOutHz);
    sicIoctl(SIC_SET_CLOCK, u32PllOutHz/1000, 0, 0);        // clock from PLL

    //--- Enable AHB clock for SIC/SD/NAND, interrupt ISR, DMA, and FMI engineer
    sicOpen();

    ptr_g_ram0 = (UINT8 *)((UINT32)g_ram0 | 0x80000000);
    ptr_g_ram1 = (UINT8 *)((UINT32)g_ram1 | 0x80000000);

    // (option) set local time for system
    local_time.year = 2012;
	local_time.mon  = 3;
	local_time.day  = 1;
	local_time.hour = 16;
	local_time.min  = 20;
	local_time.sec  = 30;
	sysSetLocalTime(local_time);

    //--- initial NVTFAT file system
    fsInitFileSystem();     // MUST call fsInitFileSystem() before sicSdOpenx()

    //--- initial SD card on port 0
    if (initial_SD_card_0() != 0)
        goto _exit_;

    //--- initial NAND driver
	ptMassNDisk = (NDISK_T*)&MassNDisk;
    if (nandInit0((NDISK_T *)ptMassNDisk))
    {
    	sysprintf("ERROR: NAND initial fail !!\n");
        goto _exit_;
    }

    pSM = pSM0;

    //--- open file on SD card
    dump_file_no = 0;
    dump_file_size = 0;

    file_handler = open_dump_file(dump_file_no++);
    if (file_handler < 0)
        goto _exit_;

    //--- write data to dump file on SD card

    // detect NAND chip and write 512 bytes header information to dump file
    memset(ptr_g_ram0, '*',  512);

    // +uLibStartBlock since sicSMInit() always minu it from uBlockPerFlash for pure data block
    // +1 since fmiSM_ReadID() always set uBlockPerFlash = really block - 1
    totalBlockNumber = pSM->uBlockPerFlash + pSM->uLibStartBlock + 1;

    memset(ptr_g_ram0, 0x00, 0x10);
    memcpy(ptr_g_ram0, "BlockPerFlash=", 14);
    ptr_g_ram0[14] = totalBlockNumber&0xFF;	        // little-endian
    ptr_g_ram0[15] = (totalBlockNumber >> 0x08)&0xFF;

    memset(ptr_g_ram0+0x10, 0x00, 0x10);
	memcpy(ptr_g_ram0+0x10, "PagePerBlock=", 13);
	ptr_g_ram0[0x10+13] = (pSM->uPagePerBlock)&0xFF;
	ptr_g_ram0[0x10+14] = ((pSM->uPagePerBlock) >> 0x08)&0xFF;

    memset(ptr_g_ram0+0x20, 0x00, 0x10);
	memcpy(ptr_g_ram0+0x20, "PageSize=", 9);
	ptr_g_ram0[0x20+9] = (pSM->nPageSize)&0xFF;
	ptr_g_ram0[0x20+10] = ((pSM->nPageSize) >> 0x08)&0xFF;

    memset(ptr_g_ram0+0x30, 0x00, 0x10);
	memcpy(ptr_g_ram0+0x30, "HiddenBlock=", 12);
	ptr_g_ram0[0x30+12] = (pSM->uLibStartBlock)&0xFF;
	ptr_g_ram0[0x30+13] = ((pSM->uLibStartBlock) >> 0x08)&0xFF;

    sysprintf("Detect NAND chip ...\n");
    sysprintf("    Page size = %d\n", pSM->nPageSize);
    sysprintf("    Page number per block = %d\n", pSM->uPagePerBlock);
    sysprintf("    Data   block number = %d\n", pSM->uBlockPerFlash + 1);  // +1 since fmiSM_ReadID() always set uBlockPerFlash = really block - 1
    sysprintf("    Hidden block number = %d\n", pSM->uLibStartBlock);
    sysprintf("    Total  block number = %d\n", totalBlockNumber);
    nandDumpUnit = (pSM->nPageSize + 512) * pSM->uPagePerBlock;     // 512 for redundancy area

    result = fsWriteFile(file_handler, ptr_g_ram0, 512, &nWriteLen);
    if (result != FS_OK)
    {
        sysprintf("ERROR: write file fail and return 0x%X !! BYE !!\n", result);
        fsCloseFile(file_handler);
        goto _exit_;
    }

    spareSize = (*(UINT32 *)(REG_SMREAREA_CTL)) & SMRE_REA128_EXT;
    sysprintf("Begin to dump data. Total %d blocks, spear size = %d...\n", totalBlockNumber, spareSize);
    for (ib = 0; ib < totalBlockNumber; ib++)
    {
        if (ib % 50 == 0)
            sysprintf("\n    dump block %d ", ib);
        sysprintf(".");

        //--- dump one block
        for (ip = 0; ip < pSM->uPagePerBlock; ip++)
        {
            memset(ptr_g_ram0, 0xFF, BUF_SIZE);
            // "- pSM->uLibStartBlock" in order to dump hidden blocks
            nandpread0(ib - pSM->uLibStartBlock, ip, ptr_g_ram0);
            // read redundancy area from register SMRA0
			for(ir = 0; ir < spareSize; ir++)
				ptr_g_ram0[pSM->nPageSize+ir] = *(UINT8 *)(REG_SMRA_0+ir);

            // add block index and page index at end of redundancy area for notice
            memset(buffer, 0xFF, 48);
            sprintf(buffer, "End of real block=%d page=%d\n", ib, ip);
            memcpy(ptr_g_ram0+pSM->nPageSize+512-48, buffer, 48);

            result = fsWriteFile(file_handler, ptr_g_ram0, pSM->nPageSize+512, &nWriteLen);
            if (result != FS_OK)
            {
                sysprintf("ERROR: write file fail at block %d page %d, return 0x%X !! BYE !!\n", ib, ip, result);
                fsCloseFile(file_handler);
                goto _exit_;
            }
        }
        dump_file_size += nandDumpUnit;

        //--- if disk full, change SD card and open next file to dump.
        if (fsDiskFreeSpace ('C', &blockSize, &freeSize, &diskSize) == FS_OK)
        {
            // sysprintf("Disk C block size=%d, free space=%d MB, disk size=%d MB\n", blockSize, (INT)freeSize/1024, (INT)diskSize/1024);
        }
        if ((freeSize*1024) < nandDumpUnit)
        {
            dump_file_size = 0;
            if (close_dump_file(file_handler) != 0)
                goto _exit_;

            sysprintf("\n");
            sysprintf("SD card full !! Please change a SD card.\n");
            sysprintf("Press any key to continue...\n");
            sysGetChar();
            sicSdClose0();

            if (initial_SD_card_0() != 0)
                goto _exit_;
            file_handler = open_dump_file(dump_file_no++);
            if (file_handler < 0)
                goto _exit_;
        }

        //--- if file too large, close file and open next file to dump.
        if (dump_file_size + nandDumpUnit > 0xC0000000)    // 0x40000000 = 1GB; 0x100000 = 1MB
        {
            dump_file_size = 0;
            if (close_dump_file(file_handler) != 0)
                goto _exit_;
            file_handler = open_dump_file(dump_file_no++);
            if (file_handler < 0)
                goto _exit_;
        }
    }
    sysprintf("\n");

    //--- close file on SD card
    if (close_dump_file(file_handler) != 0)
        goto _exit_;

    //--- unmount file system
_exit_:
    sysprintf("\n===== THE END =====\n");
    while (1);
    return OK;
}
