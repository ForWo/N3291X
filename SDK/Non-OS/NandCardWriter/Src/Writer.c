#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "w55fa95_reg.h"
#include "wblib.h"
#include "w55fa95_sic.h"
#include "w55fa95_gnand.h"
#include "nvtfat.h"
#include "writer.h"
#include "w55fa95_gpio.h"

//--- Support LED and Buzzer control for NandCardWrter.
#define KLE_LED_BUZZER_MSG

#if 1
// Define DBG_PRINTF to sysprintf to show more information about testing.
    #define DBG_PRINTF  sysprintf
#else
    #define DBG_PRINTF(...)
#endif
#define ERR_PRINTF      sysprintf

extern INI_INFO_T Ini_Writer;
extern INT fmiMarkBadBlock(UINT32 block);

//======================================================
// GNAND used
//======================================================
//--- SIC API for CS0 1st on board NAND
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


__align(32) UINT8 infoBufArray[0x40000];
__align(32) UINT8 StorageBufferArray[0x50000];
__align(32) UINT8 CompareBufferArray[0x50000];
UINT32 infoBuf, StorageBuffer, CompareBuffer, BufferSize=0;

UINT32 u32TimerChannel = 0;

CHAR        suNvtFullName[512], suNvtTargetFullName[512];
static INT  hNvtFile = -1;
INT32       gCurBlock=0, gCurPage=0;
FW_UPDATE_INFO_T    FWInfo[3];
BOOL volatile       bIsAbort = FALSE;
IBR_BOOT_STRUCT_T   NandMark;
INT32               gNandLoaderSize;


#ifdef KLE_LED_BUZZER_MSG
/*-----------------------------------------------------------------------------
 * Define LED index for LEX_xxx routines.
 *---------------------------------------------------------------------------*/
enum LEG_INDEX { LED_READY, LED_PASS, LED_FAIL };

/*-----------------------------------------------------------------------------
 * Initial GPIO G2/G4/G5 for LED and turn off them. 1 to turn off, 0 to turn on.
 *      LED_READY use GPIO G2
 *      LED_PASS  use GPIO G4
 *      LED_FAIL  use GPIO G5
 *---------------------------------------------------------------------------*/
void LED_init()
{
    gpio_configure(GPIO_PORTG, 2);  // Set GPIO pin to GPIO mode. Also set it to digital mode if necessary.
    gpio_configure(GPIO_PORTG, 4);  // Set GPIO pin to GPIO mode. Also set it to digital mode if necessary.
    gpio_configure(GPIO_PORTG, 5);  // Set GPIO pin to GPIO mode. Also set it to digital mode if necessary.
    gpio_setportpull(GPIO_PORTG, (BIT2|BIT4|BIT5), 0xFF);   // set pull high for port number. 1 enable pull high, 0 disable.
    gpio_setportval (GPIO_PORTG, (BIT2|BIT4|BIT5), 0xFF);   // set output value for port number
    gpio_setportdir (GPIO_PORTG, (BIT2|BIT4|BIT5), 0xFF);   // set dir for port number. 1 for output, 0 for input.
}

/*-----------------------------------------------------------------------------
 * Set GPIO to 0 to Turn ON LED.
 *---------------------------------------------------------------------------*/
void LED_on(int led_idx)
{
    switch (led_idx)
    {
        case LED_READY:
            gpio_setportval (GPIO_PORTG, BIT2, 0x00);
            break;
        case LED_PASS:
            gpio_setportval (GPIO_PORTG, BIT4, 0x00);
            break;
        case LED_FAIL:
            gpio_setportval (GPIO_PORTG, BIT5, 0x00);
            break;
        default:
            ERR_PRINTF("ERROR: Wrong LED index to turn on LED !!\n");
            break;
    }
}

/*-----------------------------------------------------------------------------
 * Set GPIO to 1 to Turn OFF LED.
 *---------------------------------------------------------------------------*/
void LED_off(int led_idx)
{
    switch (led_idx)
    {
        case LED_READY:
            gpio_setportval (GPIO_PORTG, BIT2, 0xFF);
            break;
        case LED_PASS:
            gpio_setportval (GPIO_PORTG, BIT4, 0xFF);
            break;
        case LED_FAIL:
            gpio_setportval (GPIO_PORTG, BIT5, 0xFF);
            break;
        default:
            ERR_PRINTF("ERROR: Wrong LED index to turn off LED !!\n");
            break;
    }
}

/*-----------------------------------------------------------------------------
 * Toggle LED.
 *---------------------------------------------------------------------------*/
void LED_toggle(int led_idx)
{
    unsigned short reg_value;

    gpio_readport(GPIO_PORTG, &reg_value);
    switch (led_idx)
    {
        case LED_READY:
            if ((reg_value & BIT2) == 0)
                LED_off(LED_READY);
            else
                LED_on(LED_READY);
            break;
        case LED_PASS:
            if ((reg_value & BIT4) == 0)
                LED_off(LED_PASS);
            else
                LED_on(LED_PASS);
            break;
        case LED_FAIL:
            if ((reg_value & BIT5) == 0)
                LED_off(LED_FAIL);
            else
                LED_on(LED_FAIL);
            break;
        default:
            ERR_PRINTF("ERROR: Wrong LED index to toggle LED !!\n");
            break;
    }
}

/*-----------------------------------------------------------------------------
 * Initial GPIO A3 for Buzzer and turn off them. 1 to beep, 0 to mute.
 *---------------------------------------------------------------------------*/
void buzzer_init()
{
    gpio_configure(GPIO_PORTA, 3);  // Set GPIO pin to GPIO mode. Also set it to digital mode if necessary.
    gpio_setportpull(GPIO_PORTA, BIT3, 0xFF);   // set pull high for port number. 1 enable pull high, 0 disable.
    gpio_setportval (GPIO_PORTA, BIT3, 0x00);   // set output value for port number
    gpio_setportdir (GPIO_PORTA, BIT3, 0xFF);   // set dir for port number. 1 for output, 0 for input.
}

/*-----------------------------------------------------------------------------
 * Set GPIO A3 to 1 to turn on buzzer
 *---------------------------------------------------------------------------*/
void buzzer_on()
{
    gpio_setportval (GPIO_PORTA, BIT3, 0xFF);   // set output value to 1
}

/*-----------------------------------------------------------------------------
 * Set GPIO A3 to 0 to turn off buzzer
 *---------------------------------------------------------------------------*/
void buzzer_off()
{
    gpio_setportval (GPIO_PORTA, BIT3, 0X00);   // set output value to 0
}

/*-----------------------------------------------------------------------------
 * Toggle buzzer.
 *---------------------------------------------------------------------------*/
void buzzer_toggle()
{
    unsigned short reg_value;

    gpio_readport(GPIO_PORTA, &reg_value);
    if ((reg_value & BIT3) == 0)
        buzzer_on();
    else
        buzzer_off();
}

#define SW_START_BUTTON_DOWN    0
#define SW_START_BUTTON_UP      1

/*-----------------------------------------------------------------------------
 * Initial GPIO G3 to input mode for SW_START
 *---------------------------------------------------------------------------*/
void sw_start_init()
{
    gpio_configure(GPIO_PORTG, 3);  // Set GPIO pin to GPIO mode. Also set it to digital mode if necessary.
    gpio_setportpull(GPIO_PORTG, BIT3, 0xFF);   // set pull high for port number. 1 enable pull high, 0 disable.
    gpio_setportdir (GPIO_PORTG, BIT3, 0x00);   // set dir for port number. 1 for output, 0 for input.
}

/*-----------------------------------------------------------------------------
 * Get SW_START status
 * Return: SW_START_BUTTON_DOWN : button down
 *         SW_START_BUTTON_UP   : button up
 *---------------------------------------------------------------------------*/
int get_sw_start()
{
    unsigned short reg_value;

    gpio_readport(GPIO_PORTG, &reg_value);
    if ((reg_value & BIT3) == 0)
        return SW_START_BUTTON_DOWN;    // button down
    else
        return SW_START_BUTTON_UP;      // button up
}

/*-----------------------------------------------------------------------------
 * Timer Interrupt handler.
 *---------------------------------------------------------------------------*/
void TIMER0_LED_toggle(void)
{
    LED_toggle(LED_READY);
}

#endif  // end of KLE_LED_BUZZER_MSG


/**********************************/
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
    CHAR            FolderName[64];

    fsUnicodeStrCat(suDirName, suSlash);    /* append '\' */
    fsUnicodeStrCat(suTargetName, suSlash); /* append '\' */

    memset((UINT8 *)&tFileInfo, 0, sizeof(tFileInfo));
    nStatus = fsFindFirst(suDirName, NULL, &tFileInfo);
    if (nStatus < 0)
    {
        fsUnicodeToAscii(suDirName,FolderName, TRUE);
        sysprintf("No %s Folder", FolderName);
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

            nStatus = File_Copy(suSrcLongName, suDstLongName);
            if (nStatus < 0)
            {
                ERR_PRINTF("    Copying file %s fail ! Status = 0x%x\n", tFileInfo.szShortName, nStatus);
                return nStatus;
            }

            nStatus = File_Compare(suSrcLongName, suDstLongName);
            if (nStatus < 0)
            {
                return nStatus;
            }
        }
    } while (!fsFindNext(&tFileInfo));

    fsFindClose(&tFileInfo);
    return 0;
}


/*-----------------------------------------------------------------------------
 * Keep idle for some ticks.
 * Please make sure the TIMER0 had defined before use delay_tick().
 *---------------------------------------------------------------------------*/
void delay_tick(unsigned int counter)
{
    unsigned begin_tick;
    begin_tick = sysGetTicks(0);
    while (sysGetTicks(0) - begin_tick < counter)
        ;
}


/**********************************/
int main()
{
    DateTime_T ltime;

    NDISK_T *ptNDisk;
    PDISK_T *pDisk_nand = NULL;
    int status=0, nReadLen;
    CHAR szNvtFullName[64];

    LDISK_T *ptLDisk;
    PDISK_T *ptPDisk;
    PARTITION_T *ptPart;
    int LogicSectorD=-1, i;

    UINT32 u32ExtFreq;
    UINT32 u32PllOutHz;
    WB_UART_T uart;

    UINT32 uBlockSize=0, uFreeSize=0, uDiskSize=0;

#if 1
    // show clock setting
    DBG_PRINTF("NandCardWriter entry.\n");
    DBG_PRINTF("APLL   Clock = %dHz\n", sysGetPLLOutputHz(eSYS_APLL, sysGetExternalClock()));
    DBG_PRINTF("UPLL   Clock = %dHz\n", sysGetPLLOutputHz(eSYS_UPLL, sysGetExternalClock()));
    DBG_PRINTF("System Clock = %dHz\n", sysGetSystemClock());
    DBG_PRINTF("CPU    Clock = %dHz\n", sysGetCPUClock());
    DBG_PRINTF("HCLK1  Clock = %dHz\n", sysGetHCLK1Clock());
    DBG_PRINTF("APB    Clock = %dHz\n", sysGetAPBClock());
#endif

    //--- Reset SIC engine to make sure it under normal status.
    outp32(REG_AHBCLK, inp32(REG_AHBCLK) | (SIC_CKE | NAND_CKE | SD_CKE));
    outp32(REG_AHBIPRST, inp32(REG_AHBIPRST) | SICRST);     // SIC engine reset is avtive
    outp32(REG_AHBIPRST, inp32(REG_AHBIPRST) & ~SICRST);    // SIC engine reset is no active. Reset completed.

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
    sysSetTimerReferenceClock(TIMER0, u32ExtFreq);
    sysStartTimer(TIMER0, 100, PERIODIC_MODE);

    sysprintf("\n=====> W55FA95 Nand Card Writer (v%d.%d) Begin [%d] <=====\n", MAJOR_VERSION_NUM, MINOR_VERSION_NUM, sysGetTicks(0));

#ifdef KLE_LED_BUZZER_MSG
    LED_init();
    buzzer_init();
    sw_start_init();

    //--- when initial, turn on buzzer and all LED 1 second, and then turn off all but LED_READY.
    LED_on(LED_READY);
    LED_on(LED_PASS);
    LED_on(LED_FAIL);
    buzzer_on();
    delay_tick(100);    // delay 100 ticks = 1s
    buzzer_off();
    LED_off(LED_PASS);
    LED_off(LED_FAIL);
#endif

    ltime.year = 2012;
    ltime.mon  = 04;
    ltime.day  = 19;
    ltime.hour = 8;
    ltime.min  = 40;
    ltime.sec  = 0;
    sysSetLocalTime(ltime);

    fsInitFileSystem();
    fsAssignDriveNumber('X', DISK_TYPE_SD_MMC, 0, 1);           // SD0, single partition
    fsAssignDriveNumber('D', DISK_TYPE_SMART_MEDIA, 0, 1);      // NAND1-1, 2 partitions
    fsAssignDriveNumber('E', DISK_TYPE_SMART_MEDIA, 0, 2);      // NAND1-2, 2 partitions

#ifdef KLE_LED_BUZZER_MSG
    DBG_PRINTF("Waiting for SW_START become 0 or button down...\n");
    while (get_sw_start() == SW_START_BUTTON_UP)
        ;   // waiting SW_START become 0 or button down

    // Ready LED blinking per 200ms to indicate NAND is programming
    u32TimerChannel = sysSetTimerEvent(TIMER0, 20, (PVOID)TIMER0_LED_toggle);   // interrupt per n ticks
#endif

    u32PllOutHz = sysGetPLLOutputHz(eSYS_UPLL, sysGetExternalClock());
    sicIoctl(SIC_SET_CLOCK, u32PllOutHz/1000, 0, 0);
    sicOpen();
    status = sicSdOpen0();
    if (status < 0)
    {
        sysprintf("===> 1 (No SD Card)\n");
        bIsAbort = TRUE;
        goto _end_;
    }

    // Get the NandCardWriter setting from INI file (NandCardWriter.ini)
    status = ProcessINI("X:\\NandCardWriter.ini");
    if (status < 0)
    {
        sysprintf("===> 1.1 (Wrong INI file)\n");
        bIsAbort = TRUE;
        goto _end_;
    }

    BufferSize = 0x40000;

    /************************************/
    /* Program NAND card on CS1         */
    /************************************/
    if (Ini_Writer.NANDCARD_FAT != FAT_MODE_SKIP)
    {
        sysprintf("\n=====> Create GNAND for NAND on CS1 [%d] <=====\n", sysGetTicks(0));

        ptNDisk = (NDISK_T *)malloc(sizeof(NDISK_T));

        //--- erase GNAND P2LN table to force GNAND_InitNAND() to erase whole NAND
        nandInit0(ptNDisk);
        for (i=0; i<8; i++)
        {
            nand_block_erase0(i);
        }
        fmiSMClose(0);

        status = GNAND_InitNAND(&_nandDiskDriver0, ptNDisk, TRUE);
        if (status)
        {
            sysprintf("ERROR: GNAND initial fail for CS0 !! Return = 0x%x\n", status);
            bIsAbort = TRUE;
            goto _end_;
        }

        status = GNAND_MountNandDisk(ptNDisk);

        pDisk_nand = (PDISK_T *)ptNDisk->pDisk;

        if ((Ini_Writer.NANDCARD_FAT == FAT_MODE_IMAGE_NO_MBR) || (Ini_Writer.NANDCARD_FAT == FAT_MODE_FILE))
        {
            // partition and format Nandcard
            sysprintf("=====> partition and format [%d] <=====\n", sysGetTicks(0));
            status = fsFormatFlashMemoryCard((PDISK_T *)pDisk_nand);    // only one partition
            if (status < 0)
            {
                sysprintf("===> 5 (Format NAND card fail)\n");
                bIsAbort = TRUE;
                goto _end_;
            }

            fsSetVolumeLabel('D', "NANDCARD\n", strlen("NANDCARD"));

            // Get disk information
            uBlockSize=0, uFreeSize=0, uDiskSize=0;
            fsDiskFreeSpace('D', &uBlockSize, &uFreeSize, &uDiskSize);
            sysprintf("Disk D (Nandcard) Size: %d Kbytes, Free Space: %d KBytes\n", (INT)uDiskSize, (INT)uFreeSize);

            // Get the Start sector for F partition
            ptLDisk = (LDISK_T *)malloc(sizeof(LDISK_T));
            if (get_vdisk('D', &ptLDisk) <0)
            {
                sysprintf(" ===> 6 (vdisk fail)\n");
                bIsAbort = TRUE;
                goto _end_;
            }
            ptPDisk = ptLDisk->ptPDisk;   // get the physical disk structure pointer of NAND disk
            ptPart = ptPDisk->ptPartList;  // Get the partition of NAND disk
            while (ptPart != NULL)
            {
        #if 1
                sysprintf("Driver %c -- Start sector : %d, Total sector : %d\n",
                        ptPart->ptLDisk->nDriveNo, ptPart->uStartSecN, ptPart->uTotalSecN);
        #endif
                if  (ptPart->ptLDisk->nDriveNo == 'D')
                    LogicSectorD = ptPart->uStartSecN;
                ptPart = ptPart->ptNextPart;
            }
        }
        else if (Ini_Writer.NANDCARD_FAT == FAT_MODE_IMAGE_WITH_MBR)    // don't need partition disk since image with MBR
        {
            LogicSectorD = 0;   // write image with MBR to sector 0
        }

        /*********************************/
        /* copy first partition content  */
        /*********************************/
        sysprintf("\n=====> copy Partition Content [%d] <=====\n", sysGetTicks(0));
        if ((Ini_Writer.NANDCARD_FAT == FAT_MODE_IMAGE_NO_MBR) || (Ini_Writer.NANDCARD_FAT == FAT_MODE_IMAGE_WITH_MBR))
        {
            // Copy File Through FAT like Binary ISO
            if (LogicSectorD == -1)
            {
                sysprintf("===> 7.1 (Wrong start sector for NANDCARD)\n");
                bIsAbort = TRUE;
                goto _end_;
            }

            strcpy(szNvtFullName, "X:\\NANDCARD\\content.bin");
            fsAsciiToUnicode(szNvtFullName, suNvtFullName, TRUE);
            hNvtFile = fsOpenFile(suNvtFullName, NULL, O_RDONLY);
            sysprintf("Copying file %s\n", szNvtFullName);
            if (hNvtFile < 0)
            {
                sysprintf("===> 7.2 (Open content.bin fail)\n");
                bIsAbort = TRUE;
                goto _end_;
            }

            while(1)
            {
                sysprintf(".");
                status = fsReadFile(hNvtFile, (UINT8 *)StorageBuffer, BufferSize, &nReadLen);
                GNAND_write(ptNDisk, LogicSectorD, nReadLen/512, (UINT8 *)StorageBuffer);
                LogicSectorD += nReadLen/512;
                if (status == ERR_FILE_EOF)
                    break;
                else if (status < 0)
                {
                    sysprintf("\n===> 7.3 (Read content.bin fail) [0x%x]\n", status);
                    bIsAbort = TRUE;
                    goto _end_;
                }
            }
            sysprintf("\n");
        }
        else if (Ini_Writer.NANDCARD_FAT == FAT_MODE_FILE)
        {
            // Copy File through FAT
            strcpy(szNvtFullName, "D:");
            fsAsciiToUnicode(szNvtFullName, suNvtTargetFullName, TRUE);
            strcpy(szNvtFullName, "X:\\NANDCARD");
            fsAsciiToUnicode(szNvtFullName, suNvtFullName, TRUE);
            status = nandCopyContent(suNvtFullName, suNvtTargetFullName);
            if (status < 0)
            {
                sysprintf("===> 7.4 (Copy files in NANDCARD fail) [0x%x]\n", status);
                bIsAbort = TRUE;
                goto _end_;
            }
        }
    }   // end of Ini_Writer.NANDCARD_FAT != FAT_MODE_SKIP
    else
    {
        bIsAbort = TRUE;
    }

_end_:

    sysprintf("\n=====> Finish [%d] <=====\n", sysGetTicks(0));

#ifdef KLE_LED_BUZZER_MSG
    sysClearTimerEvent(TIMER0, u32TimerChannel);    // stop LED_READY blinking
    LED_on(LED_READY);
    LED_off(LED_FAIL);
    LED_off(LED_PASS);

    if (bIsAbort)
    {
        // control LED and buzzer for Fail : LED_FAIL on, buzzer on 200ms and off 100ms forever.
        LED_on(LED_FAIL);
        while(1)
        {
            buzzer_on();
            delay_tick(20);
            buzzer_off();
            delay_tick(10);
            if (get_sw_start() == SW_START_BUTTON_DOWN) // press button to stop buzzer and exit program.
                break;
        }
    }
    else
    {
        // control LED and buzzer for Pass : LED_PASS on, buzzer on 300ms one time.
        LED_on(LED_PASS);
        buzzer_on();
        delay_tick(30);
        buzzer_off();
    }
#endif

    while(1);
}
