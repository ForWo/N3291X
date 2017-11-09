#include <string.h>
#include "wblib.h"
#include "turbowriter.h"
#include "w55fa95_vpost.h"
#include "w55fa95_reg.h"

// define DATE CODE and show it when running to make maintaining easy.
#define DATE_CODE   "20170822"

/* global variable */
typedef struct sd_info
{
    unsigned int startSector;
    unsigned int endSector;
    unsigned int fileLen;
    unsigned int executeAddr;
} NVT_SD_INFO_T;

extern void spuDacOn(UINT8 level);
extern ERRCODE DrvSPU_Open(void);

/*-----------------------------------------------------------------------------
 * For RTC feature
 *---------------------------------------------------------------------------*/
#define RTC_DELAY       500000
#define RTC_INIT_KEY    0xa5eb1357
#define RTC_WRITE_KEY   0xa965
#define RTC_WAIT_COUNT  1000000
#define RTC_INIT_RETRY  5
#define E_RTC_SUCCESS   0
#define E_RTC_ERR_EIO   6

int RTC_Check(VOID)
{
    UINT32 volatile i, Wait;

    i =0;
    Wait = inp32(REG_FLAG) & RTC_REG_FLAG;
    while(Wait != RTC_REG_FLAG)
    {
        Wait = inp32(REG_FLAG) & RTC_REG_FLAG;
        i++;
        if(i > RTC_DELAY)
        {
            sysprintf("RTC_Check(): Time out\n");
            return -1;
        }
    }
    return 0;
}


UINT32 RTC_Init (VOID)
{
    INT32 i32i;
    int i;

    sysprintf("RTC_Init(): inp32(INIR) is 0x%x, PWRON is 0x%x, RTC_SET is 0x%x.\n", inp32(INIR), inpw(PWRON), inpw(RTC_BA+0x038));
    outp32(INIR, RTC_INIT_KEY);
    for (i32i = 0; i32i < RTC_WAIT_COUNT; i32i++)
    {
        if (inp32(INIR) & Active)
        {   /* Check RTC_INIR[0] to find out RTC reset signal */
            break;
        }
    }

    if (i32i == RTC_WAIT_COUNT)
    {
        sysprintf("RTC_Init(): inp32(INIR) is 0x%x, timeout.\n", inp32(INIR));
        return E_RTC_ERR_EIO;
    }
    RTC_Check();

    /*-----------------------------------------------------------------------------------------------------*/
    /* Install RTC ISR                                                                                     */
    /*-----------------------------------------------------------------------------------------------------*/
    outp32(AER, RTC_WRITE_KEY);
    for (i32i = 0; i32i < RTC_WAIT_COUNT; i32i++)
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* check RTC_AER[16] to find out RTC write enable                                                  */
        /*-------------------------------------------------------------------------------------------------*/
        if (inp32(AER) & ENF)
        {
            break;
        }
    }

    if (i32i == RTC_WAIT_COUNT)
    {
        return E_RTC_ERR_EIO;
    }

    i = 0;
    while(1)
    {
        if (inp32(REG_FLAG) & RTC_REG_FLAG)
        {
            break;
        }
        if (i > 10000)
        {
            sysprintf("RTC_Init(): inp32(REG_FLAG) is 0x%x, timeout.\n", inp32(REG_FLAG));
            return E_RTC_ERR_EIO;
        }
        i++;
    }
    return E_RTC_SUCCESS;
}


INT MoveData(NVT_SD_INFO_T *image, BOOL IsExecute)
{
    int volatile sector_count;
    void    (*fw_func)(void);

    sysprintf("Load file length 0x%x, execute address 0x%x\n", image->fileLen, image->executeAddr);

    // read SD card
    sector_count = image->fileLen / 512;
    if ((image->fileLen % 512) != 0)
        sector_count++;

    fmiSD_Read(image->startSector, sector_count, (UINT32)image->executeAddr);

    if (IsExecute == TRUE)
    {
        outpw(REG_SDISR, 0x0000FFFF);
        outpw(REG_FMIIER, 0);
        outpw(REG_SDIER, 0);
        outpw(REG_FMICR, 0);

        // SD-0 pin dis-selected
        outpw(REG_GPEFUN, inpw(REG_GPEFUN)&(~0x0000FFF0));  // SD0_CLK/CMD/DAT0_3 pins dis-selected

        // disable SD Card Host Controller operation and driving clock.
        outpw(REG_AHBCLK, inpw(REG_AHBCLK) & ~SD_CKE);
        outpw(REG_AHBCLK, inpw(REG_AHBCLK) & ~SIC_CKE);

        sysprintf("SD Boot Loader exit. Jump to execute address 0x%x ...\n", image->executeAddr);

        fw_func = (void(*)(void))(image->executeAddr);
        fw_func();
    }
    return 0;
}


#define __DDR2__
// Clock Skew for FA95 MCP version C
#define E_CLKSKEW   0x0088ff00

void initClock(void)
{
    UINT32 u32ExtFreq;
    UINT32 reg_tmp;

    u32ExtFreq = sysGetExternalClock();     // Hz unit
    if(u32ExtFreq==12000000)
    {
        outp32(REG_SDREF, 0x805A);
    }
    else
    {
        outp32(REG_SDREF, 0x80C0);
    }

#ifdef __UPLL_NOT_SET__
    // 2012/3/7 by CJChen1@nuvoton.com, don't change anything that include
    //      System clock, clock skew, REG_DQSODS and REG_SDTIME.
    //      System clock will follow IBR setting that should be UPLL/System/CPU 264MHz, HCLK1 132MHz, APB 33MHz.
    sysprintf("SD Loader DONOT set anything and follow IBR setting !!\n");
#endif  // __UPLL_NOT_SET__

#ifdef __UPLL_264__
    // Follow all IBR setting but REG_DQSODS and Clock skew.
    outp32(REG_DQSODS, 0x1010);
    outp32(REG_CKDQSDS, E_CLKSKEW);
#endif  // __UPLL_264__

#ifdef __UPLL_288__
    outp32(REG_DQSODS, 0x1010);
    outp32(REG_CKDQSDS, E_CLKSKEW);
    #ifdef __DDR2__
        outp32(REG_SDTIME, 0x2A62F74A);     // REG_SDTIME for 288M/300MHz system clock
        outp32(REG_SDMR, 0x00000432);
        outp32(REG_MISC_SSEL, 0x00000155);  // set MISC_SSEL to Reduced Strength to improve EMI
    #endif

    // initial system clock
    sysSetSystemClock(eSYS_UPLL,
                    288000000,      // Specified the APLL/UPLL clock, unit Hz
                    288000000);     // Specified the system clock, unit Hz
    sysSetCPUClock (288000000);     // Unit Hz
    sysSetAPBClock ( 72000000);     // Unit Hz
#endif  // __UPLL_288__

#ifdef __UPLL_300__
    outp32(REG_DQSODS, 0x1010);
    outp32(REG_CKDQSDS, E_CLKSKEW);
    #ifdef __DDR2__
        outp32(REG_SDTIME, 0x2A62F74A);     // REG_SDTIME for 300MHz system clock
        outp32(REG_SDMR, 0x00000432);
        outp32(REG_MISC_SSEL, 0x00000155);  // set MISC_SSEL to Reduced Strength to improve EMI
    #endif

    // initial system clock
    sysSetSystemClock(eSYS_UPLL,
                    300000000,      // Specified the APLL/UPLL clock, unit Hz
                    300000000);     // Specified the system clock, unit Hz
    sysSetCPUClock (300000000);     // Unit Hz
    sysSetAPBClock ( 75000000);     // Unit Hz
#endif  // __UPLL_300__

    // always set APLL to 432MHz
    sysSetPllClock(eSYS_APLL, 432000000);

    // always set HCLK234 to 0
    reg_tmp = inp32(REG_CLKDIV4) | CHG_APB;     // MUST set CHG_APB to HIGH when configure CLKDIV4
    outp32(REG_CLKDIV4, reg_tmp & (~HCLK234_N));

    // for FA95c, set Watchdog Timer Prescale to Normal mode
    outp32(REG_WTCR, (inp32(REG_WTCR) | 0x0100));
}


UINT8 dummy_buffer[512];
unsigned char *buf;
unsigned int *pImageList;

int main()
{
    NVT_SD_INFO_T image;
    int count, i;
    int ibr_boot_sd_port;
#ifdef __UPLL_NOT_SET__
    UINT32 u32PllHz, u32SysHz, u32CpuHz, u32Hclk1Hz, u32ApbHz, u32APllHz;
#endif

    sysprintf("W55FA95 SD Boot Loader entry (%s).\n", DATE_CODE);

    //--- Re-initial RTC if RTC initial fail when power on.
    //--- Need FA95 demo board 1M54 or later to support this solution.
    if (RTC_Check() == -1)
    {
        for (i = 0; i < RTC_INIT_RETRY; i++)
        {
            if ((RTC_Init() == E_RTC_SUCCESS) && (inp32(RTC_FCR) != 0))
            {
                sysprintf("Since IBR initial RTC fail, retry RTC_Init() %d times and successful finally.\n", i+1);
                break;
            }
            else
            {
                sysprintf("Since IBR initial RTC fail, retry RTC_Init() %d times but still fail.\n", i+1);
            }
        }
    }

    //--- initial SPU
    DrvSPU_Open();
    spuDacOn(2);

    /* PLL clock setting */
    initClock();

    //--- 2013/9/6, enable REG_AUDADC_CTL[APB2AHB] since APB clock < 1/3 System Clock
    outp32(REG_APBCLK, inp32(REG_APBCLK) | ADC_CKE);    // enable ADC clock in order to set register REG_AUDADC_CTL that belong to ADC.
    outp32(REG_AUDADC_CTL, inp32(REG_AUDADC_CTL) | APB2AHB);
    outp32(REG_APBCLK, inp32(REG_APBCLK) & ~ADC_CKE);   // disable ADC clock to save power.

    sysprintf("REG_CKDQSDS = 0x%08X\n", inp32(REG_CKDQSDS));
    sysprintf("System clock = %dHz\n", sysGetSystemClock());
    sysprintf("AHB clock = %dHz\n", sysGetHCLK1Clock());

#ifdef __UPLL_NOT_SET__
    sysprintf("REG_SDTIME = 0x%08X\n", inp32(REG_SDTIME));
    u32APllHz = sysGetPLLOutputHz(eSYS_APLL, sysGetExternalClock());
    u32PllHz = sysGetPLLOutputHz(eSYS_UPLL, sysGetExternalClock());
    u32SysHz = sysGetSystemClock();
    u32CpuHz = sysGetCPUClock();
    u32Hclk1Hz = sysGetHCLK1Clock();
    u32ApbHz = sysGetAPBClock();
    sysprintf("APLL Clock = %d\n", u32APllHz);
    sysprintf("UPLL Clock = %d\n", u32PllHz);
    sysprintf("System Clock = %d\n", u32SysHz);
    sysprintf("CPU Clock = %d\n", u32CpuHz);
    sysprintf("HCLK1 Clock = %d\n", u32Hclk1Hz);
    sysprintf("APB Clock = %d\n", u32ApbHz);
    sysprintf("REG_CLKDIV4=0x%08X, HCLK234=0x%X\n", inp32(REG_CLKDIV4), (inp32(REG_CLKDIV4) & HCLK234_N)>>4);
#endif

    /* RTC H/W Power Off Function Configuration */
    RTC_Check();    // waiting for RTC regiesters ready for access
    outp32(PWRON, (inp32(PWRON) & ~PCLR_TIME) | 0x60005);   // Press Power Key during 6 sec to Power off (0x'6'0005)
    RTC_Check();
    outp32(RIIR, 0x4);
    RTC_Check();
    outp32(REG_APBCLK, inp32(REG_APBCLK) & ~RTC_CKE);   // disable RTC clock to save power

    buf = (UINT8 *)((UINT32)dummy_buffer | 0x80000000);
    pImageList=((unsigned int *)(((unsigned int)dummy_buffer)|0x80000000));

    // IBR keep the booting SD port number on register SDCR.
    // SDLoader should load image from same SD port.
    ibr_boot_sd_port = (inpw(REG_SDCR) & SDCR_SDPORT) >> 29;

    /* Initial DMAC and NAND interface */
    fmiInitDevice();

    sysprintf("Boot from SD%d ...\n", ibr_boot_sd_port);
    i = fmiInitSDDevice(ibr_boot_sd_port);
    if (i < 0)
        sysprintf("SD init fail <%d>\n", i);

    memset((char *)&image, 0, sizeof(NVT_SD_INFO_T));

    /* read image information */
    fmiSD_Read(33, 1, (UINT32)buf);

    if (((*(pImageList+0)) == 0x574255aa) && ((*(pImageList+3)) == 0x57425963))
    {
        count = *(pImageList+1);

        /* logo */
        pImageList = pImageList+4;
        for (i=0; i<count; i++)
        {
            if (((*(pImageList) >> 16) & 0xffff) == 4)  // logo
            {
                image.startSector = *(pImageList + 1) & 0xffff;
                image.endSector = (*(pImageList + 1) & 0xffff0000) >> 16;
                image.executeAddr = *(pImageList + 2);
                image.fileLen = *(pImageList + 3);
                MoveData(&image, FALSE);
                break;
            }
            /* pointer to next image */
            pImageList = pImageList+12;
        }

        pImageList = ((unsigned int*)(((unsigned int)dummy_buffer)|0x80000000));
        memset((char *)&image, 0, sizeof(NVT_SD_INFO_T));
        /* execution file */
        pImageList = pImageList+4;
        for (i=0; i<count; i++)
        {
            if (((*(pImageList) >> 16) & 0xffff) == 1)  // execute
            {
                image.startSector = *(pImageList + 1) & 0xffff;
                image.endSector = (*(pImageList + 1) & 0xffff0000) >> 16;
                image.executeAddr = *(pImageList + 2);
                // sysprintf("executing address = 0x%x\n", image.executeAddr);
                image.fileLen = *(pImageList + 3);
                MoveData(&image, TRUE);
                break;
            }
            /* pointer to next image */
            pImageList = pImageList+12;
        }
        while(1);
    }
    return 0;
}
