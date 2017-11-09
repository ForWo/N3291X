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

//--- Define UPDATE_NANDLOADER to copy NandLoader.bin from disk NAND1-2 to block 0 ~ 1/3/7 and overwrite original NandLoader.
//--- Please note it is a HIGH RISK action. Device will crash if NandLoader update fail since power off or any other issue.
#define UPDATE_NANDLOADER

#ifdef UPDATE_NANDLOADER
    //--- Define UPDATE_BOOT_CODE_OPTIONAL_SETTING to parse TurboWriter.ini and config Boot Code Optional Setting.
    #define UPDATE_BOOT_CODE_OPTIONAL_SETTING
#endif

extern INT fmiMarkBadBlock(UINT32 block);

#ifdef __KLE_DEMO__
    extern void LcmBacklightInit(void);
    extern void LcmBacklightEnable(void);
#endif

extern S_DEMO_FONT s_sDemo_Font;

int font_x=0, font_y=16;
UINT32 u32SkipX;

UINT8 *pInfo;
FMI_SM_INFO_T *pNvtSM0;

#if 1
// Define DBG_PRINTF to sysprintf to show more information about testing.
    #define DBG_PRINTF  sysprintf
#else
    #define DBG_PRINTF(...)
#endif


//======================================================
// GNAND used
//======================================================
NDRV_T _nandDiskDriver0 =
{
    nandInit0,
    nandpread0,
    nandpwrite0,
    nand_is_page_dirty0,
    nand_is_valid_block0,
    nand_ioctl,
    nand_block_erase0,
    nand_chip_erase0,
    0
};

/**********************************/
__align(32) UINT8 infoBufArray[0x40000];
__align(32) UINT8 StorageBufferArray[0x50000];
__align(32) UINT8 CompareBufferArray[0x50000];
UINT32 infoBuf, StorageBuffer, CompareBuffer, BufferSize=0;

CHAR        suNvtFullName[512], suNvtTargetFullName[512];
static INT  hNvtFile = -1;
INT32       gCurBlock=0, gCurPage=0;
FW_UPDATE_INFO_T    FWInfo[3];
BOOL volatile       bIsAbort = FALSE;
IBR_BOOT_STRUCT_T   NandMark;


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
        //status = nvtSMpread(ii, pSM->uPagePerBlock-1, (UINT8 *)ptr);
        status = nandpread0(ii-pSM->uLibStartBlock, pSM->uPagePerBlock-1, (UINT8 *)ptr);
        if (!status)
        {
            if (((*(pImageList+0)) == 0x574255aa) && ((*(pImageList+3)) == 0x57425963))
                break;
        }
    }
    if (status < 0)
        return status;  // nvtSMpread() fail

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


int File_Copy(CHAR *suSrcName, CHAR *suDstName)
{
    INT     hFileSrc, hFileDst, nByteCnt, nStatus;

    hFileSrc = fsOpenFile(suSrcName, NULL, O_RDONLY);
    if (hFileSrc < 0)
        return hFileSrc;

    hFileDst = fsOpenFile(suDstName, NULL, O_CREATE);
    if (hFileDst < 0)
    {
        fsCloseFile(hFileSrc);
        return hFileDst;
    }

    while (1)
    {
        Draw_Wait_Status(font_x+ u32SkipX*g_Font_Step, font_y);

        nStatus = fsReadFile(hFileSrc, (UINT8 *)StorageBuffer, BufferSize, &nByteCnt);
        if (nStatus < 0)
            break;

        nStatus = fsWriteFile(hFileDst, (UINT8 *)StorageBuffer, nByteCnt, &nByteCnt);
        if (nStatus < 0)
            break;
    }
    fsCloseFile(hFileSrc);
    fsCloseFile(hFileDst);

    if (nStatus == ERR_FILE_EOF)
        nStatus = 0;
    return nStatus;
}


int File_Compare(CHAR *suFileNameS, CHAR *suFileNameD)
{
    INT     hFileS, hFileD;
    INT     nLenS, nLenD, nStatusS, nStatusD;

    hFileS = fsOpenFile(suFileNameS, NULL, O_RDONLY);
    if (hFileS < 0)
        return hFileS;

    hFileD = fsOpenFile(suFileNameD, NULL, O_RDONLY);
    if (hFileD < 0)
        return hFileD;

    while (1)
    {
        Draw_Wait_Status(font_x+ u32SkipX*g_Font_Step, font_y);
        nStatusS = fsReadFile(hFileS, (UINT8 *)StorageBuffer, BufferSize, &nLenS);
        nStatusD = fsReadFile(hFileD, (UINT8 *)CompareBuffer, BufferSize, &nLenD);

        if ((nStatusS == ERR_FILE_EOF) && (nStatusD == ERR_FILE_EOF))
        {
            fsCloseFile(hFileS);
            fsCloseFile(hFileD);
            return 0;
        }

        if (nLenS != nLenD)
            break;

        if (memcmp((UINT8 *)StorageBuffer, (UINT8 *)CompareBuffer, nLenS))
            break;
    }

    fsCloseFile(hFileS);
    fsCloseFile(hFileD);

    return -1;
}


int nandCopyContent(CHAR *suDirName, CHAR *suTargetName)
{
    INT             nLenS, nLenD, nStatus;
    CHAR            suSrcLongName[MAX_FILE_NAME_LEN];
    CHAR            suDstLongName[MAX_FILE_NAME_LEN];
    CHAR            suSlash[6] = { '\\', 0, 0, 0 };
    FILE_FIND_T     tFileInfo;
    CHAR            Array1[64], FolderName[64];

    fsUnicodeStrCat(suDirName, suSlash);    /* append '\' */
    fsUnicodeStrCat(suTargetName, suSlash); /* append '\' */

    memset((UINT8 *)&tFileInfo, 0, sizeof(tFileInfo));

    nStatus = fsFindFirst(suDirName, NULL, &tFileInfo);
    if (nStatus < 0)
    {
        fsUnicodeToAscii(suDirName,FolderName, TRUE);
        sprintf(Array1, "No %s Folder", FolderName);
        Draw_CurrentOperation(Array1,nStatus);
        return nStatus;
    }

    do
    {
        if (tFileInfo.ucAttrib & A_DIR)
        {
            if ((strcmp(tFileInfo.szShortName, "..") == 0) ||
                (strcmp(tFileInfo.szShortName, ".") == 0))
                continue;

            fsUnicodeCopyStr(suSrcLongName, suDirName);
            fsUnicodeCopyStr(suDstLongName, suTargetName);
            nLenS = fsUnicodeStrLen(suDirName);
            nLenD = fsUnicodeStrLen(suTargetName);
            if ( !((suDirName[nLenS-2] == '\\') && (suDirName[nLenS-1] == 0)) )
                fsUnicodeStrCat(suSrcLongName, suSlash);    /* append '\' */
            if ( !((suTargetName[nLenD-2] == '\\') && (suTargetName[nLenD-1] == 0)) )
                fsUnicodeStrCat(suDstLongName, suSlash);    /* append '\' */
            fsUnicodeStrCat(suSrcLongName, tFileInfo.suLongName);
            fsUnicodeStrCat(suDstLongName, tFileInfo.suLongName);

            nStatus = fsMakeDirectory(suDstLongName, NULL);
            if (nStatus < 0)
            {
                Draw_CurrentOperation("Unable to Create Directory",nStatus);
                return nStatus;
            }

            nStatus = nandCopyContent(suSrcLongName, suDstLongName);
            if (nStatus < 0)
            {
                sysprintf("===> 22\n");
                bIsAbort = TRUE;
                return nStatus;
            }
        }
        else
        {
            fsUnicodeCopyStr(suSrcLongName, suDirName);
            fsUnicodeCopyStr(suDstLongName, suTargetName);
            fsUnicodeStrCat(suSrcLongName, tFileInfo.suLongName);
            fsUnicodeStrCat(suDstLongName, tFileInfo.suLongName);

            DBG_PRINTF("Copying file %s\n", tFileInfo.szShortName);
            sprintf(Array1, "Copying %s", tFileInfo.szShortName);

            nStatus = File_Copy(suSrcLongName, suDstLongName);
            if (nStatus < 0)
            {
                Draw_CurrentOperation(Array1,nStatus);
                DBG_PRINTF("    Copying file %s fail ! Status = 0x%x\n", tFileInfo.szShortName, nStatus);
                return nStatus;
            }
            else
                Draw_CurrentOperation(Array1,nStatus);

            sprintf(Array1, "Comparing %s", tFileInfo.szShortName);
            nStatus = File_Compare(suSrcLongName, suDstLongName);
            if (nStatus < 0)
            {
                Draw_CurrentOperation(Array1,nStatus);
                return nStatus;
            }
            Draw_CurrentOperation(Array1,nStatus);
        }
    } while (!fsFindNext(&tFileInfo));

    fsFindClose(&tFileInfo);
    return 0;
}


/**********************************/
int main()
{
    DateTime_T ltime;
    NDISK_T *ptNDisk;
    INT  status=0, nReadLen;
    CHAR szNvtFullName[64];
    CHAR Array1[64];
    UINT Next_Font_Height;
    INT  FileInfoIdx=0;
    UINT32 u32ExtFreq;
    UINT32 uBlockSize=0, uFreeSize=0, uDiskSize=0;
    WB_UART_T uart;
    BOOL bIsExecute = FALSE;    // FALSE means no execute image updated
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

    infoBuf = (UINT32) &infoBufArray[0] | 0x80000000;
    StorageBuffer = (UINT32)&StorageBufferArray[0] | 0x80000000;
    CompareBuffer = (UINT32)&CompareBufferArray[0] | 0x80000000;

    pInfo = (UINT8 *)((UINT32)infoBuf | 0x80000000);

    /* configure Timer0 for FMI library */
    sysSetTimerReferenceClock(TIMER0, 12000000);
    sysStartTimer(TIMER0, 100, PERIODIC_MODE);

#ifdef __KLE_DEMO__
    u32TimerChannel = sysSetTimerEvent(TIMER0, 30, (PVOID)Timer0_300msCallback);
#endif

    sysprintf("\n=====> W55FA95 NandUpdater (v%d.%d) Begin [%d] <=====\n", MAJOR_VERSION_NUM, MINOR_VERSION_NUM, sysGetTicks(0));

    ltime.year = 2012;
    ltime.mon  = 04;
    ltime.day  = 19;
    ltime.hour = 8;
    ltime.min  = 40;
    ltime.sec  = 0;
    sysSetLocalTime(ltime);

    fsInitFileSystem();
    fsAssignDriveNumber('D', DISK_TYPE_SMART_MEDIA, 0, 1);      // NAND1-1, 2 partitions
    fsAssignDriveNumber('E', DISK_TYPE_SMART_MEDIA, 0, 2);      // NAND1-2, 2 partitions

    Draw_Init();
    font_y = g_Font_Height;
    Next_Font_Height = g_Font_Height-6;

    sicOpen();
    Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, font_x, font_y, "Mount GNAND:");
    u32SkipX = 12;
    ptNDisk = (NDISK_T *)malloc(sizeof(NDISK_T));
    GNAND_InitNAND(&_nandDiskDriver0, ptNDisk, TRUE);
    status = GNAND_MountNandDisk(ptNDisk);
    if (status)
    {
        Draw_CurrentOperation("Mount GNAND",status);
        Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
    }
    else
        Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Successful);
    font_y += Next_Font_Height;

    //--- show disk free space
    fsDiskFreeSpace('D', &uBlockSize, &uFreeSize, &uDiskSize);
    sysprintf("Disk D Size: %d Kbytes, Free Space: %d KBytes\n", (INT)uDiskSize, (INT)uFreeSize);

    uBlockSize=0, uFreeSize=0, uDiskSize=0;
    fsDiskFreeSpace('E', &uBlockSize, &uFreeSize, &uDiskSize);
    sysprintf("Disk E Size: %d Kbytes, Free Space: %d KBytes\n", (INT)uDiskSize, (INT)uFreeSize);

    //--- always use CS0
    pNvtSM0 = pSM0;     // pSM0 defined by Non-OS SIC driver library

    sprintf(Array1, "Nand:%d(Blk)*%d(Pg)*%d(Size)",
            pNvtSM0->uBlockPerFlash, pNvtSM0->uPagePerBlock, pNvtSM0->nPageSize);
    Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, 0,  _LCM_HEIGHT_ -1-g_Font_Height,Array1);

    BufferSize = pNvtSM0->uPagePerBlock * pNvtSM0->nPageSize;

    if (BufferSize >= 0x50000)
        BufferSize = 0x40000;

    //--- found the start block of Update Area
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
        Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Successful);
    font_y += Next_Font_Height;

#ifdef UPDATE_NANDLOADER
{
    FW_UPDATE_INFO_T *ptr_image;
    int NandFlag, i, j;
    extern INT32 gNandLoaderSize;
    extern INT nvtSMpwrite(INT PBA, INT page, UINT8 *buff);

#ifdef UPDATE_BOOT_CODE_OPTIONAL_SETTING
    int optional_ini_size;
    extern IBR_BOOT_OPTIONAL_STRUCT_T optional_ini_file;
    extern int ProcessOptionalINI(char *fileName);

    // Get the Boot Code Optional Setting from INI file (TurboWriter.ini) to optional_ini_file
    sysprintf("=====> Parsing TurboWriter.ini as Boot Code Optional Setting INI file [%d] <=====\n", sysGetTicks(0));
    ProcessOptionalINI("E:\\update\\TurboWriter.ini");
    if (optional_ini_file.Counter == 0)
        optional_ini_size = 0;
    else
    {
        optional_ini_size = 8 * (optional_ini_file.Counter + 1);
        if (optional_ini_file.Counter % 2 == 0)
            optional_ini_size += 8;     // for dummy pair to make sure 16 bytes alignment.
    }
#endif

    sysprintf("=====> Update NandLoader to System Area [%d] <=====\n", sysGetTicks(0));

    //--- update NandLoader if NandLoader.bin exist in disk NAND1-2
    strcpy(szNvtFullName, "E:\\update\\NandLoader.bin");
    fsAsciiToUnicode(szNvtFullName, suNvtFullName, TRUE);
    hNvtFile = fsOpenFile(suNvtFullName, NULL, O_RDONLY);
    if (hNvtFile < 0)
    {
        sysprintf("===> File name NandLoader.bin don't exist, don't update NandLoader.\n");
        goto WriteUpdateArea;   // don't update NandLoader.bin if the file not exist
    }

    sprintf(Array1, "Open %s", szNvtFullName);
    Draw_CurrentOperation(Array1, hNvtFile);

    //--- initial Boot Code Mark for NandLoader
    NandMark.BootCodeMarker = 0x57425AA5;
    NandMark.ExeAddr = 0x900000;
    NandMark.ImageSize = (UINT32)fsGetFileSize(hNvtFile);
    NandMark.Reserved = 0x00000000;
    gNandLoaderSize = NandMark.ImageSize;
    nIsSysImage = 0x5A;

    //--- Duplicate NandLoader in block 0 ~ (pNvtSM0->uIBRBlock - 1)
    for (i=0; i < pNvtSM0->uIBRBlock; i++)
    {
        gCurBlock = i;

        //--- backup image information from last 2 pages of block
        //    and update NandLoader image file size since it could be changed.
        status = nandpread0(gCurBlock - pNvtSM0->uLibStartBlock, pNvtSM0->uPagePerBlock - 2, (UINT8 *)CompareBuffer);
        if (status < 0)
        {
            sysprintf("Read image information from block %d page %d fail ! Ignore it.\n", gCurBlock, pNvtSM0->uPagePerBlock - 2);
            continue;
        }
        ptr_image = (FW_UPDATE_INFO_T *)(CompareBuffer+16);
        for (j=0; j<3; j++)     // search 3 images within image information table
        {
            // sysprintf("[block%d-%d] imageNo=%d, imageFlag=%d, fileLen=%d\n", gCurBlock, j, ptr_image->imageNo, ptr_image->imageFlag, ptr_image->fileLen);
            if (ptr_image->imageFlag == 3)  // found NandLoader image
            {
                ptr_image->fileLen = NandMark.ImageSize;
                strcpy(ptr_image->imageName, "NandLoader_update.bin");
                break;
            }
            ptr_image++;   // go to next image
        }

        //--- backup reserve size information from last pages of block
        status = nandpread0(gCurBlock - pNvtSM0->uLibStartBlock, pNvtSM0->uPagePerBlock - 1, (UINT8 *)(CompareBuffer + pNvtSM0->nPageSize));
        if (status < 0)
        {
            sysprintf("Read reserve size information from block %d page %d fail ! Ignore it.\n", gCurBlock, pNvtSM0->uPagePerBlock - 1);
            continue;
        }

        //--- erase block
        nand_block_erase0(gCurBlock - pNvtSM0->uLibStartBlock);

        //--- write NandLoader.bin to block
        fsFileSeek(hNvtFile, 0, SEEK_SET);
        gCurPage = 0;
        NandFlag = 1;   // 1 to write 1st Page with NAND Marker
        while(1)
        {
            memset((UINT8 *)StorageBuffer, 0xff, BufferSize);
            if (NandFlag)
            {   // Write 1st Page with NAND Marker
#ifdef UPDATE_BOOT_CODE_OPTIONAL_SETTING
                status = fsReadFile(hNvtFile, (UINT8 *)(StorageBuffer+sizeof(IBR_BOOT_STRUCT_T)+optional_ini_size),
                                    pNvtSM0->nPageSize - sizeof(IBR_BOOT_STRUCT_T) - optional_ini_size, &nReadLen);
                memcpy((UINT8 *)StorageBuffer, (UINT8 *)&NandMark, sizeof(IBR_BOOT_STRUCT_T));
                memcpy((UINT8 *)(StorageBuffer+sizeof(IBR_BOOT_STRUCT_T)), (UINT8 *)&optional_ini_file, optional_ini_size);
#else
                status = fsReadFile(hNvtFile, (UINT8 *)(StorageBuffer+sizeof(IBR_BOOT_STRUCT_T)),
                                    pNvtSM0->nPageSize - sizeof(IBR_BOOT_STRUCT_T), &nReadLen);
                memcpy((UINT8 *)StorageBuffer, (UINT8 *)&NandMark, sizeof(IBR_BOOT_STRUCT_T));
#endif
                nvtSMpwrite(gCurBlock, gCurPage, (UINT8 *)StorageBuffer);
                NandFlag = 0;
            }
            else
            {
                status = fsReadFile(hNvtFile, (UINT8 *)StorageBuffer, pNvtSM0->nPageSize, &nReadLen);
                nvtSMpwrite(gCurBlock, gCurPage, (UINT8 *)StorageBuffer);
            }
            gCurPage++;
            if (status == ERR_FILE_EOF)
                break;
            else if (status < 0)
            {
                Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
                sysprintf("===> 2.3 fail to read NandLoader file\n");
                bIsAbort = TRUE;
                goto _end_;
            }
        }   // end of while(1) to write NandLoader to one block

        //--- restore the image information at last 2 page
        nvtSMpwrite(gCurBlock, pNvtSM0->uPagePerBlock - 2, (UINT8 *)CompareBuffer);

        //--- restore the reserve size information at last page
        nvtSMpwrite(gCurBlock, pNvtSM0->uPagePerBlock - 1, (UINT8 *)(CompareBuffer + pNvtSM0->nPageSize));
    }   // end of for(i) to write NandLoader to all blocks

    Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Successful);
    font_y += Next_Font_Height;

    fsCloseFile(hNvtFile);
    nIsSysImage = 0xFF;
}

WriteUpdateArea:
#endif

    /********************************************************************************************/
    /* don't copy NandLoader, just store update image information in first block of Update Area */
    /********************************************************************************************/
    /* nand information */
    FWInfo[FileInfoIdx].imageNo = FileInfoIdx;
    FWInfo[FileInfoIdx].imageFlag = 3;
    FWInfo[FileInfoIdx].startBlock = UpdateAreaStartBlock;  // first block of Update Area
    FWInfo[FileInfoIdx].endBlock = UpdateAreaStartBlock;
    FWInfo[FileInfoIdx].executeAddr = 0;
    FWInfo[FileInfoIdx].fileLen = 0;
    memcpy(&FWInfo[FileInfoIdx].imageName[0], "NANDLoader.bin", 32);

    // initial Boot Code Mark for NandLoader
    NandMark.BootCodeMarker = 0x57425AA5;
    NandMark.ExeAddr = 0x900000;
    NandMark.ImageSize = FWInfo[FileInfoIdx].fileLen;
    NandMark.Reserved = 0x00000000;

    // Write 1st Page of block UpdateAreaStartBlock with NAND Marker
    memcpy((UINT8 *)StorageBuffer, (UINT8 *)&NandMark, sizeof(IBR_BOOT_STRUCT_T));
    gCurPage = 0;
    gCurBlock = UpdateAreaStartBlock;
    nand_block_erase0(UpdateAreaStartBlock - pNvtSM0->uLibStartBlock);
    nandpwrite0(UpdateAreaStartBlock - pNvtSM0->uLibStartBlock, 0, (UINT8 *)StorageBuffer);
    FileInfoIdx++;
    sysprintf("=====> write mark to Update Area start block %d [%d] <=====\n", gCurBlock, sysGetTicks(0));

    /************************************/
    /* copy update image to Update Area */
    /************************************/

    /*************/
    /* copy logo */
    /*************/
    strcpy(szNvtFullName, "E:\\update\\Logo.bin");
    fsAsciiToUnicode(szNvtFullName, suNvtFullName, TRUE);
    hNvtFile = fsOpenFile(suNvtFullName, NULL, O_RDONLY);
    if (hNvtFile < 0)
    {
        goto WriteNVTLoader;    // don't update Logo.bin if the file not exist
    }

    Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, font_x, font_y, "Writing Logo:");
    u32SkipX = 13;

    sprintf(Array1, "Open %s", szNvtFullName);
    Draw_CurrentOperation(Array1,hNvtFile);

    /* nand information */
    FWInfo[FileInfoIdx].imageNo = FileInfoIdx;
    FWInfo[FileInfoIdx].imageFlag = 4;  // Logo image
    FWInfo[FileInfoIdx].startBlock = FWInfo[FileInfoIdx-1].endBlock + 1;
    FWInfo[FileInfoIdx].executeAddr = 0x500000;
    FWInfo[FileInfoIdx].fileLen = (UINT32)fsGetFileSize(hNvtFile);
    memcpy(&FWInfo[FileInfoIdx].imageName[0], "Logo.bin", 32);
    gCurPage = 0;
    gCurBlock = FWInfo[FileInfoIdx].startBlock;

    sysprintf("=====> copy and verify logo to block %d [%d] <=====\n", gCurBlock, sysGetTicks(0));

    while(1)
    {
        status = fsReadFile(hNvtFile, (UINT8 *)StorageBuffer, pNvtSM0->nPageSize, &nReadLen);
        if (gCurPage == 0)
        {
            if (gCurBlock >= pNvtSM0->uLibStartBlock)
            {
                sysprintf("===> ERROR: Update Area (block %d) over the System Area (block %d).\n", gCurBlock, pNvtSM0->uLibStartBlock);
                Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
                bIsAbort = TRUE;
                goto _end_;
            }
            nand_block_erase0(gCurBlock - pNvtSM0->uLibStartBlock);
        }
        nandpwrite0(gCurBlock - pNvtSM0->uLibStartBlock, gCurPage, (UINT8 *)StorageBuffer);
        gCurPage++;
        if (gCurPage == pNvtSM0->uPagePerBlock)
        {
            gCurPage = 0;
            gCurBlock++;
        }

        if (status == ERR_FILE_EOF)
            break;
        else if (status < 0)
        {
            Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
            sysprintf("===> 3.2 fail to read Logo file\n");
            bIsAbort = TRUE;
            goto _end_;
        }
    }

    if (gCurPage == 0)
        FWInfo[FileInfoIdx].endBlock = gCurBlock - 1;
    else
        FWInfo[FileInfoIdx].endBlock = gCurBlock;

    /* verify logo */
    gCurPage = 0;
    gCurBlock = FWInfo[FileInfoIdx].startBlock;
    fsFileSeek(hNvtFile, 0, SEEK_SET);

    while(1)
    {
        status = fsReadFile(hNvtFile, (UINT8 *)StorageBuffer, pNvtSM0->nPageSize, &nReadLen);
        nandpread0(gCurBlock - pNvtSM0->uLibStartBlock, gCurPage, (UINT8 *)CompareBuffer);
        gCurPage++;
        if (gCurPage == pNvtSM0->uPagePerBlock)
        {
            gCurPage = 0;
            gCurBlock++;
        }

        if (memcmp((UINT8 *)StorageBuffer, (UINT8 *)CompareBuffer, nReadLen))
        {
            Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
            sysprintf("===> 3.3 fail to verify Logo file\n");
            bIsAbort = TRUE;
            goto _end_;
        }
        if (status == ERR_FILE_EOF)
            break;
        else if (status < 0)
        {
            Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
            sysprintf("===> 3.4 fail to read Logo file\n");
            bIsAbort = TRUE;
            goto _end_;
        }
    }
    Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Successful);
    font_y += Next_Font_Height;

    fsCloseFile(hNvtFile);
    hNvtFile = -1;

    FileInfoIdx++;

    /******************/
    /* copy nvtloader */
    /******************/
WriteNVTLoader:

    strcpy(szNvtFullName, "E:\\update\\NVTLoader.bin");
    fsAsciiToUnicode(szNvtFullName, suNvtFullName, TRUE);
    hNvtFile = fsOpenFile(suNvtFullName, NULL, O_RDONLY);
    if (hNvtFile < 0)
    {
        goto WriteSysteInfo;
    }

    Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, font_x, font_y, "Writing NvtLoader:");
    u32SkipX = 18;

    sprintf(Array1, "Open %s", szNvtFullName);
    Draw_CurrentOperation(Array1,hNvtFile);

    /* nand information */
    FWInfo[FileInfoIdx].imageNo = FileInfoIdx;
    FWInfo[FileInfoIdx].imageFlag = 1;  // Execute image
    FWInfo[FileInfoIdx].startBlock = FWInfo[FileInfoIdx-1].endBlock + 1;
    FWInfo[FileInfoIdx].executeAddr = 0x800000;
    FWInfo[FileInfoIdx].fileLen = (UINT32)fsGetFileSize(hNvtFile);
    memcpy(&FWInfo[FileInfoIdx].imageName[0], "NVTLoader.bin", 32);
    gCurPage = 0;
    gCurBlock = FWInfo[FileInfoIdx].startBlock;

    sysprintf("=====> copy and verify nvtloader to block %d [%d] <=====\n", gCurBlock, sysGetTicks(0));

    while(1)
    {
        status = fsReadFile(hNvtFile, (UINT8 *)StorageBuffer, pNvtSM0->nPageSize, &nReadLen);
        if (gCurPage == 0)
        {
            if (gCurBlock >= pNvtSM0->uLibStartBlock)
            {
                sysprintf("===> ERROR: Update Area (block %d) over the System Area (block %d).\n", gCurBlock, pNvtSM0->uLibStartBlock);
                Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
                bIsAbort = TRUE;
                goto _end_;
            }
            nand_block_erase0(gCurBlock - pNvtSM0->uLibStartBlock);
        }
        nandpwrite0(gCurBlock - pNvtSM0->uLibStartBlock, gCurPage, (UINT8 *)StorageBuffer);
        gCurPage++;
        if (gCurPage == pNvtSM0->uPagePerBlock)
        {
            gCurPage = 0;
            gCurBlock++;
        }

        if (status == ERR_FILE_EOF)
            break;
        else if (status < 0)
        {
            Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
            sysprintf("===> 4.2 fail to read NvtLoader file\n");
            bIsAbort = TRUE;
            goto _end_;
        }
    }

    if (gCurPage == 0)
        FWInfo[FileInfoIdx].endBlock = gCurBlock - 1;
    else
        FWInfo[FileInfoIdx].endBlock = gCurBlock;


    /* verify nvtloader */
    gCurPage = 0;
    gCurBlock = FWInfo[FileInfoIdx].startBlock;
    fsFileSeek(hNvtFile, 0, SEEK_SET);

    while(1)
    {
        status = fsReadFile(hNvtFile, (UINT8 *)StorageBuffer, pNvtSM0->nPageSize, &nReadLen);
        nandpread0(gCurBlock - pNvtSM0->uLibStartBlock, gCurPage, (UINT8 *)CompareBuffer);
        gCurPage++;
        if (gCurPage == pNvtSM0->uPagePerBlock)
        {
            gCurPage = 0;
            gCurBlock++;
        }

        if (memcmp((UINT8 *)StorageBuffer, (UINT8 *)CompareBuffer, nReadLen))
        {
            Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
            sysprintf("===> 4.3 fail to verify NvtLoader file\n");
            bIsAbort = TRUE;
            goto _end_;
        }

        if (status == ERR_FILE_EOF)
            break;
        else if (status < 0)
        {
            Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
            sysprintf("===> 4.4 fail to read NvtLoader file\n");
            bIsAbort = TRUE;
            goto _end_;
        }
    }

    Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Successful);
    font_y += Next_Font_Height;

    bIsExecute = TRUE;
    fsCloseFile(hNvtFile);
    hNvtFile = -1;

WriteSysteInfo:
    /* set information to block UpdateAreaStartBlock @(last page -1) */
    // bIsExecute is TRUE means execute image had updated.
    //      NandUpdater must modify Update Image Information here
    //      and then NandLoader will use Update Image to boot system.
    if (bIsExecute)
    {
        unsigned int *ptr;
        pInfo = (UINT8 *)((UINT32)infoBuf | 0x80000000);
        ptr = (unsigned int *)((UINT32)infoBuf | 0x80000000);

        memset(pInfo, 0xff,  pNvtSM0->nPageSize);

        /* update image information */
        *(ptr+0) = 0x574255AA;
        *(ptr+1) = FileInfoIdx+1;
        *(ptr+3) = 0x57425963;

        sysprintf("=====> write image information to Update Area start block %d [%d] <=====\n", UpdateAreaStartBlock, sysGetTicks(0));

        memcpy(pInfo+16, (char *)&FWInfo, pNvtSM0->nPageSize);
        nandpwrite0(UpdateAreaStartBlock - pNvtSM0->uLibStartBlock, pNvtSM0->uPagePerBlock - 2, pInfo);

        /* Verify information */
        nandpread0(UpdateAreaStartBlock - pNvtSM0->uLibStartBlock, pNvtSM0->uPagePerBlock - 2, (UINT8 *)CompareBuffer);

        if (memcmp((UINT8 *)pInfo, (UINT8 *)CompareBuffer, 112))
        {
            sysprintf("===> 4.5 fail to verify System Information\n");
            bIsAbort = TRUE;
            goto _end_;
        }
    }


    /****************************************************/
    /* copy files from NAND1-2 update folder to NAND1-1 */
    /****************************************************/
    sysprintf("\n=====> copy files to NAND1-1 [%d] <=====\n", sysGetTicks(0));

    // Copy File through FAT
    Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, font_x, font_y, "Copying NAND1-1:");
    u32SkipX = 16;

    strcpy(szNvtFullName, "D:");
    fsAsciiToUnicode(szNvtFullName, suNvtTargetFullName, TRUE);

    strcpy(szNvtFullName, "E:\\update\\NAND1-1");
    fsAsciiToUnicode(szNvtFullName, suNvtFullName, TRUE);

    status = nandCopyContent(suNvtFullName, suNvtTargetFullName);
    Draw_Clear_Wait_Status(font_x+ u32SkipX*g_Font_Step, font_y);
    if (status < 0)
    {
        Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
        sysprintf("===> 7.4 Copy files in NAND1-1 fail [0x%x]\n", status);
    }
    else
        Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Successful);
    font_y += Next_Font_Height;


    /****************************************************/
    /* copy files from NAND1-2 update folder to NAND1-2 */
    /****************************************************/
    sysprintf("\n=====> copy files to NAND1-2 [%d] <=====\n", sysGetTicks(0));

    // Copy File through FAT
    Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, font_x, font_y, "Copying NAND1-2:");
    u32SkipX = 16;

    strcpy(szNvtFullName, "E:");
    fsAsciiToUnicode(szNvtFullName, suNvtTargetFullName, TRUE);

    strcpy(szNvtFullName, "E:\\update\\NAND1-2");
    fsAsciiToUnicode(szNvtFullName, suNvtFullName, TRUE);

    status = nandCopyContent(suNvtFullName, suNvtTargetFullName);
    Draw_Clear_Wait_Status(font_x+ u32SkipX*g_Font_Step, font_y);
    if (status < 0)
    {
        Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
        sysprintf("===> 8.4 Copy files in NAND1-2 fail [0x%x]\n", status);
    }
    else
        Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Successful);
    font_y += Next_Font_Height;

_end_:

    /************************/
    /* Delete Update Folder */
    /************************/
    sysprintf("=====> Delete update folder in NAND1-2 [%d] <=====\n", sysGetTicks(0));
    Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, font_x, font_y, "Delete update folder:");
    u32SkipX = 21;

    strcpy(szNvtFullName, "E:\\update");
    fsAsciiToUnicode(szNvtFullName, suNvtFullName, TRUE);
    status = fsDeleteDirTree(suNvtFullName, NULL);
    if (status == 0)
        Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Successful);
    else
    {
        sysprintf("===> fail to delete update folder in NAND1-2 !\n");
        Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
    }
    font_y += Next_Font_Height;


    /*********************/
    /* Show Final Status */
    /*********************/
    sysprintf("=====> Finish [%d] <=====\n", sysGetTicks(0));
    Draw_FinalStatus(bIsAbort, FALSE);

    if (!bIsAbort)
    {
        //--- if update OK, enable Watchdog Timer but don't reset it to make system reboot.
        sysprintf("=====> System rebooting [%d] <=====\n", sysGetTicks(0));
        sysEnableWatchDogTimer();
        sysEnableWatchDogTimerReset();
    }

    while(1);
}
