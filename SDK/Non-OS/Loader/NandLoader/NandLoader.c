#include <string.h>
#include "wblib.h"
#include "turbowriter.h"
#include "w55fa95_vpost.h"
#include "w55fa95_reg.h"

#ifdef __KLE_DEMO__
    #include "usbd.h"
    extern INT32 adc_open(UINT32 type, UINT32 hr, UINT32 vr);
    extern void BatteryDetection(BOOL bIsExtraPower);

    #define CAMERA_POWER_OFF            // set the camera module power off sequence.
#endif

#define SUPPORT_NANDUPDATER

// define DATE CODE and show it when running to make maintaining easy.
#ifdef __KLE_ECO__
    #define DATE_CODE   "20170920 ECO"
#else
    #define DATE_CODE   "20170920"
#endif

#ifdef __KLE_ECO__
    #define SUPPORT_RTC_ON_OFF_ISSUE    // to fix RTC power on/off fail issue.
#endif

#define DBG_PRINTF  sysprintf
//#define DBG_PRINTF(...)

/* global variable */
typedef struct nand_info
{
    unsigned int startBlock;
    unsigned int endBlock;
    unsigned int fileLen;
    unsigned int executeAddr;
} NVT_NAND_INFO_T;

extern void spuDacOn(UINT8 level);
extern ERRCODE DrvSPU_Open(void);

#ifndef __DISABLE_RTC__
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

int RTC_Check(void)
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


UINT32 RTC_Init (void)
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
#endif  // end of #ifndef __DISABLE_RTC__

#ifdef __KLE_DEMO__
#ifdef CAMERA_POWER_OFF
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


void HI702_sensorSuspend(BOOL bIsSuspend)
{
    // Standby GPB4 = LOW
    outp32(REG_GPBFUN, inp32(REG_GPBFUN) & ~(MF_GPB4));
    //mdelay(1);  // Dealy 1ms
    if(bIsSuspend == TRUE)
    {   // suspend GPB4 = LOW.
        outp32(REG_GPIOB_DOUT , inp32(REG_GPIOB_DOUT) & ~(BIT4));
        //sysprintf("Sensor suspend\n");
    }
    else
    {   // Non suspend GPB4 = High.
        outp32(REG_GPIOB_DOUT , inp32(REG_GPIOB_DOUT) | (BIT4));
    }
    outp32(REG_GPIOB_OMD, inp32(REG_GPIOB_OMD) | (BIT4));
}


void HI702_sensorPoweron(BOOL bIsPowerOn)
{
    // GPD0 acts as CAM2V8(1: on, 0: off),
    // GPB4 acts as CHIP_ENABLE(1: Enable, 0: Suspend)
    // GPD12 acts as CAM1V8 (1: on, 0: off)
    outp32(REG_GPDFUN , inp32(REG_GPDFUN) & ~(MF_GPD0) );       // GPD0 switch to GPIO
    outp32(REG_GPDFUN , inp32(REG_GPDFUN) & ~(MF_GPD12) );      // GPD12 switch to GPIO

    if(bIsPowerOn == TRUE)
    {   // Turn on the 2.8v first then 1.8v
        outp32(REG_GPIOD_DOUT , inp32(REG_GPIOD_DOUT)| (BIT0) );    // Turn on 2,8v
        outp32(REG_GPIOD_OMD , (inp32(REG_GPIOD_OMD) | (BIT0)));    // GPD0 set to output mode
        // mdelay(15);     // Dealy 10ms
        outp32(REG_GPIOD_DOUT , inp32(REG_GPIOD_DOUT)| (BIT12) );   // Turn on 1,8v
        outp32(REG_GPIOD_OMD , (inp32(REG_GPIOD_OMD) | (BIT12)));   // GPD12 set to output mode
    }
    else
    {   // Turn off the 1.8v first the 2.8v
        delay_tick(21);  // LGE ask delay 20ms at least
        outp32(REG_GPIOD_DOUT , inp32(REG_GPIOD_DOUT)&~(BIT12) );   // 1.8v off
        outp32(REG_GPIOD_OMD , (inp32(REG_GPIOD_OMD) | (BIT12)));   // GPD12 set to output mode
        delay_tick(21);  // LGE ask delay 20ms at least
        outp32(REG_GPIOD_DOUT , inp32(REG_GPIOD_DOUT)&~(BIT0) );    // 2.8v off
        outp32(REG_GPIOD_OMD , (inp32(REG_GPIOD_OMD) | (BIT0)));    // GPD0 set to output mode
    }
}
#endif  // end of CAMERA_POWER_OFF
#endif  // end of __KLE_DEMO__


INT MoveData(NVT_NAND_INFO_T *image, BOOL IsExecute)
{
    unsigned int page_count, block_count, curBlock, addr;
    int volatile i, j;
    void    (*fw_func)(void);

    sysprintf("Load file length 0x%x, execute address 0x%x\n", image->fileLen, image->executeAddr);

    page_count = image->fileLen / pSM0->nPageSize;
    if ((image->fileLen % pSM0->nPageSize) != 0)
        page_count++;

    block_count = page_count / pSM0->uPagePerBlock;

    curBlock = image->startBlock;
    addr = image->executeAddr;
    j=0;
    while(1)
    {
        if (j >= block_count)
            break;

        if (pSM0->nPageSize == NAND_PAGE_512B)
        {
            if (CheckBadBlockMark_512(curBlock) == Successful)
            {
                for (i=0; i<pSM0->uPagePerBlock; i++)
                {
                    sicSMpread(0, curBlock, i, (UINT8 *)addr);
                    addr += pSM0->nPageSize;
                }
                j++;
            }
        }
        else
        {
            if (CheckBadBlockMark(curBlock) == Successful)
            {
                for (i=0; i<pSM0->uPagePerBlock; i++)
                {
                    sicSMpread(0, curBlock, i, (UINT8 *)addr);
                    addr += pSM0->nPageSize;
                }
                j++;
            }
        }
        curBlock++;
    }

    if ((page_count % pSM0->uPagePerBlock) != 0)
    {
        page_count = page_count - block_count * pSM0->uPagePerBlock;
_read_:
        if (CheckBadBlockMark(curBlock) == Successful)
        {
            for (i=0; i<page_count; i++)
            {
                sicSMpread(0, curBlock, i, (UINT8 *)addr);
                addr += pSM0->nPageSize;
            }
        }
        else
        {
            curBlock++;
            goto _read_;
        }
    }

    if (IsExecute == TRUE)
    {
        /* disable NAND control pin used */
        outpw(REG_GPDFUN, inpw(REG_GPDFUN) & ~0x0003FC00);      // disable NAND NWR/NRD/RB0/RB1 pins
        outpw(REG_GPEFUN, inpw(REG_GPEFUN) & ~0x00FF0000);      // disable NAND ALE/CLE/CS0/CS1 pins

        // disable SD Card Host Controller operation and driving clock.
        outpw(REG_GPEFUN, inpw(REG_GPEFUN)&(~0x0000FFF0));      // disable SD0_CLK/CMD/DAT0_3 pins selected

#ifdef __KLE_DEMO__
        udcClose();
#endif

        sysprintf("Nand Boot Loader exit. Jump to execute address 0x%x ...\n", image->executeAddr);

        fw_func = (void(*)(void))(image->executeAddr);
        fw_func();
    }
    return 0;
}


UINT8 image_buffer[8192];
unsigned char *imagebuf;
unsigned int *pImageList;

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
    sysprintf("NAND Loader DONOT set anything and follow IBR setting !!\n");
#endif  // __UPLL_NOT_SET__

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


int main()
{
    NVT_NAND_INFO_T image;
    int volatile count, i;
#ifdef __UPLL_NOT_SET__
    UINT32 u32PllHz, u32SysHz, u32CpuHz, u32Hclk1Hz, u32ApbHz, u32APllHz;
#endif

#ifdef SUPPORT_RTC_ON_OFF_ISSUE
    int volatile wait;
    UINT32 volatile u32Tmp;
#endif

#ifdef SUPPORT_NANDUPDATER
    int fmiNandSysArea = 0;
    UINT32 imageCount, block, dataAreaStartBlock = 0, updateAreaStartBlock = 0;
    UINT32 imageInfoBlock = 0;
#endif

    sysprintf("W55FA95 Nand Boot Loader entry (%s).\n", DATE_CODE);

#ifdef __DISABLE_RTC__
    sysprintf("Disable RTC feature.\n");
#else
#ifdef SUPPORT_RTC_ON_OFF_ISSUE

    // pull high GPD15 to simulate power button DOWN in order to enable Level shift.
    outpw(REG_GPDFUN, inpw(REG_GPDFUN) & ~MF_GPD15);        // set GPD15 as GPIO pin
    outpw(REG_GPIOD_PUEN, inpw(REG_GPIOD_PUEN) | BIT15);    // set GPD15 internal resistor to pull up
    outpw(REG_GPIOD_DOUT, inpw(REG_GPIOD_DOUT) | BIT15);    // output 1 to GPD15 for power key down
    outpw(REG_GPIOD_OMD,  inpw(REG_GPIOD_OMD)  | BIT15);    // set GPD15 to OUTPUT mode

    // use GPE0 to supply core power
    outpw(REG_GPEFUN, inpw(REG_GPEFUN) & ~MF_GPE0);         // set GPE0 as GPIO pin
    outpw(REG_GPIOE_PUEN, inpw(REG_GPIOE_PUEN) | BIT0);     // set GPE0 internal resistor to pull up
    outpw(REG_GPIOE_DOUT, inpw(REG_GPIOE_DOUT) | BIT0);     // output 1 to GPE0 to supply core power (0810)
    outpw(REG_GPIOE_OMD,  inpw(REG_GPIOE_OMD)  | BIT0);     // set GPE0 to OUTPUT mode

    // pull low GPA6 to supply RTC power
    outpw(REG_GPAFUN, inpw(REG_GPAFUN) & ~MF_GPA6);         // set GPA6 as GPIO pin
    outpw(REG_GPIOA_PUEN, inpw(REG_GPIOA_PUEN) | BIT6);     // set GPA6 internal resistor to pull up
    outpw(REG_GPIOA_DOUT, inpw(REG_GPIOA_DOUT) & (~BIT6));  // output 0 to GPA6 to force RTC power on
    outpw(REG_GPIOA_OMD,  inpw(REG_GPIOA_OMD)  | BIT6);     // set GPA6 to OUTPUT mode

    sysprintf("REG_CHIPCFG = 0x%x.\n", inpw(REG_CHIPCFG));      // for power on setting debug
    sysprintf("REG 0xB0000038 = 0x%x.\n", inpw(0xB0000038));    // for power on setting debug

    // waiting for RTC regiesters ready for access : RTC must initial successfully and Level shift must enable.
    if ((RTC_Check() == -1) || (inp32(RTC_FCR) == 0))
    {
        // First RTC_Check() timeout means IBR initial RTC fail.
        // RTC_FCR == 0 means level shift not ready.
        for (i = 0; i < RTC_INIT_RETRY; i++)
        {
            if ((RTC_Init() == E_RTC_SUCCESS) && (inp32(RTC_FCR) != 0))
            {
                sysprintf("Since IBR initial RTC fail, retry RTC_Init() %d times and successful finally.\n", i+1);
                sysprintf("    PWRON = 0x%X, RTC_FCR = 0x%X \n", inp32(PWRON), inp32(RTC_FCR));
                break;
            }
            else
            {
                sysprintf("Since IBR initial RTC fail, retry RTC_Init() %d times but still fail.\n", i+1);
                sysprintf("    PWRON = 0x%X, RTC_FCR = 0x%X \n", inp32(PWRON), inp32(RTC_FCR));
                // reset RTC and power off system since RTC cannot be recovered.
                if ((i+1) == RTC_INIT_RETRY)
                {
                    //--- RTC fail. Re-initial RTC here.
                    outpw(REG_GPIOA_DOUT, inpw(REG_GPIOA_DOUT) | BIT6);     // output 1 to GPA6 to force RTC power off
                    for (wait = 0; wait < 20000; wait++)                    // delay for stability. 10000 loop = 5ms
                        ;
                    outpw(REG_GPIOA_DOUT, inpw(REG_GPIOA_DOUT) & (~BIT6));  // output 0 to GPA6 to force RTC power on

                    outpw(REG_GPIOD_DOUT, inpw(REG_GPIOD_DOUT) & (~BIT15)); // output 0 to GPD15 for power key up
                    outpw(REG_GPIOE_DOUT, inpw(REG_GPIOE_DOUT) & (~BIT0));  // output 0 to GPE0 to disable core power (0810)
                    while(1);   // power off when user release power key
                }
            }
        }
    }

    // RTC ready to access
    // Wait until PWR_KEY become 0 (power key down)
    i = 0;
    while ((inpw(PWRON) & PWR_KEY) == PWR_KEY)
        i++;
    sysprintf("Waiting PWRON[7] become 0 since GPD15 pull high, retry %d times.\n", i);

    // Power cycle RTC if fisrt time to boot system (RTC time is 2005/01/01 00h00m0Xs)
    // System is first time to boot if RTC CLR/TLR registers are default value (ignore 0~9 second).
    if ((inpw(CLR) == 0x00050101) && ((inpw(TLR) & 0xFFFFFFF0) == 0))
    {
        outpw(REG_GPIOA_DOUT, inpw(REG_GPIOA_DOUT) | BIT6);     // output 1 to GPA6 to force RTC power off
        for (wait = 0; wait < 20000; wait++)                    // delay for stability. 10000 loop = 5ms
            ;
        outpw(REG_GPIOA_DOUT, inpw(REG_GPIOA_DOUT) & (~BIT6));  // output 0 to GPA6 to force RTC power on
        sysprintf("Pull GPIO A6 high and then low to re-initial RTC.\n");
    }

    // Support solution that RTC_Init() retry and turn off core power if retry RTC_INIT_RETRY times
    i = 1;
    while (1)
    {
        if (RTC_Init() == E_RTC_SUCCESS)
        {
            if (i > RTC_INIT_RETRY)
            {
                sysprintf("Retry RTC_Init() %d times and successful finally. Turn on core power (GPE0) !\n", i);
                outpw(REG_GPIOE_DOUT, inpw(REG_GPIOE_DOUT) | BIT0);     // output 1 to GPE0 to turn on core power (0810)
            }
            break;
        }
        if (i == RTC_INIT_RETRY)
        {
            sysprintf("Retry RTC_Init() 5 times and still fail. Turn off core power (GPE0) !\n");
            outpw(REG_GPIOD_DOUT, inpw(REG_GPIOD_DOUT) & (~BIT15)); // output 0 to GPD15 for power key up
            outpw(REG_GPIOE_DOUT, inpw(REG_GPIOE_DOUT) & (~BIT0));  // output 0 to GPE0 to disable core power (0810)
            while(1);
            // break while (1) loop by RTC_Init() successful or end user release power key.
        }
        i++;
    }   // end of while loop

    // set RTC_PWRON to 1 to pull high RTC PWRCE and
    // set HW_PCLR_EN to 0 to disable H/W power off function
    u32Tmp = (inpw(PWRON) | RTC_PWRON) & (~HW_PCLR_EN);
    if ((u32Tmp & PCLR_TIME) == 0)
        u32Tmp |= 0x00070000;   // set PCLR_TIME to default value 7
    outpw(PWRON, u32Tmp);
    RTC_Check();
    sysprintf("After re-initial RTC, PWRON = 0x%X.\n", inpw(PWRON));
    if (((inpw(PWRON) & RTC_PWRON) != RTC_PWRON) ||
         (inpw(PWRON) & HW_PCLR_EN) != 0)
    {
        sysprintf("Write RTC_PWRON fail. Turn off core power (GPE0) !\n");
        outpw(REG_GPIOD_DOUT, inpw(REG_GPIOD_DOUT) & (~BIT15)); // output 0 to GPD15 for power key up
        outpw(REG_GPIOE_DOUT, inpw(REG_GPIOE_DOUT) & (~BIT0));  // output 0 to GPE0 to turn off core power (0810)
        while(1);
    }

    // pull low GPD15 to simulate the power button UP
    outpw(REG_GPIOD_DOUT, inpw(REG_GPIOD_DOUT) & (~BIT15));

    sysprintf("After pull low GPD15, PWRON = 0x%X.\n", inpw(PWRON));
#endif  // end of SUPPORT_RTC_ON_OFF_ISSUE

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
#endif  // end of #else __DISABLE_RTC__

#ifdef __KLE_DEMO__
    // call udcOpen() as early as possible for USB VBus stable.
    // Otherwise, USB library can not judge VBus correct, for example, udcIsAttached().
    udcOpen();
#endif

    //--- initial SPU
    DrvSPU_Open();
    spuDacOn(2);

    /* PLL clock setting */
    initClock();

    //--- 2013/9/6, enable REG_AUDADC_CTL[APB2AHB] since APB clock < 1/3 System Clock
    outp32(REG_APBCLK, inp32(REG_APBCLK) | ADC_CKE);    // enable ADC clock in order to set register REG_AUDADC_CTL that belong to ADC.
    outp32(REG_AUDADC_CTL, inp32(REG_AUDADC_CTL) | APB2AHB);
    outp32(REG_APBCLK, inp32(REG_APBCLK) & ~ADC_CKE);   // disable ADC clock to save power.

#ifdef __KLE_DEMO__
#ifdef CAMERA_POWER_OFF
    // set the camera control and data pins to Low to power off camera
    sysSetTimerReferenceClock(TIMER0, sysGetExternalClock());
    sysStartTimer(TIMER0, 1000, PERIODIC_MODE);     // set 1 ticks = 1 ms

    HI702_sensorSuspend(TRUE);
    // Sensor clock & Pixel clock to low
    outpw(REG_GPBFUN, inp32(REG_GPBFUN) & ~( MF_GPB0 | MF_GPB1 | MF_GPB2 | MF_GPB3 |   // SCLKO, PCLK, HSYNC and VSYNC
                                             MF_GPB5 | MF_GPB6 | MF_GPB7 | MF_GPB8 |   // VDATA[7:0]
                                             MF_GPB9 | MF_GPB10 | MF_GPB11 | MF_GPB12) );
    outpw(REG_GPIOB_DOUT, inp32(REG_GPIOB_DOUT) & ~(BIT0|BIT1|BIT2|BIT3|BIT5|BIT6|BIT7|BIT8|BIT9|BIT10|BIT11|BIT12));  // output 0
    outpw(REG_GPIOB_OMD,  inp32(REG_GPIOB_OMD)  |  (BIT0|BIT1|BIT2|BIT3|BIT5|BIT6|BIT7|BIT8|BIT9|BIT10|BIT11|BIT12));  // output mode
    HI702_sensorPoweron(FALSE);
#endif  // end of CAMERA_POWER_OFF
#endif  // end of __KLE_DEMO__

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

#ifndef __DISABLE_RTC__
    RTC_Check();    // waiting for RTC regiesters ready for access

#ifndef SUPPORT_RTC_ON_OFF_ISSUE
    // RTC H/W power off function is invalid if support solution for RTC on/off issue. So, don't config it.
    // RTC H/W Power Off Function Configuration
    outp32(PWRON, (inp32(PWRON) & ~PCLR_TIME) | 0x60005);     /* Press Power Key during 6 sec to Power off (0x'6'0005) */
    RTC_Check();
    outp32(RIIR, 0x4);
    RTC_Check();
    outp32(REG_APBCLK, inp32(REG_APBCLK) & ~RTC_CKE);
#endif
#endif  // end of #ifndef __DISABLE_RTC__

    imagebuf = (UINT8 *)((UINT32)image_buffer | 0x80000000);
    pImageList=((unsigned int *)(((unsigned int)image_buffer)|0x80000000));

    /* Initial DMAC and NAND interface */
    fmiInitDevice();
    sicSMInit();
    memset((char *)&image, 0, sizeof(NVT_NAND_INFO_T));

#ifdef __KLE_DEMO__
    //--- shutdown system if both USB cable unplug and battery voltage too low.
    adc_open(NULL, 320, 240);
    // 2012/9/12 by CJChen1, change shutdown policy by customer's request.
    if (udcIsAttached())
    {
        sysprintf("USB cable plug in.\n");
        BatteryDetection(TRUE);
    }
    else
    {
        sysprintf("USB cable un-plug.\n");
        BatteryDetection(FALSE);
    }
#endif  // end of __KLE_DEMO__

#ifdef SUPPORT_NANDUPDATER

    // read physical block 0~3 last page for system area size
    for (i=0; i<4; i++)
    {
        if (!sicSMpread(0, i, pSM0->uPagePerBlock-1, imagebuf))
        {
            if (((*(pImageList+0)) == 0x574255aa) && ((*(pImageList+3)) == 0x57425963))
            {
                imageInfoBlock = i;
//                DBG_PRINTF("Get system area size from block 0x%x ..\n", imageInfoBlock);
                break;
            }
        }
    }

    if (((*(pImageList+0)) == 0x574255aa) && ((*(pImageList+3)) == 0x57425963))
    {
        fmiNandSysArea = *(pImageList+1);   // sectors for system area in NAND that TurboWriter write into.
        dataAreaStartBlock = (fmiNandSysArea / pSM0->uSectorPerBlock) + 1;
        if (fmiNandSysArea % pSM0->uSectorPerBlock > 0)
            dataAreaStartBlock++;
    }
//    DBG_PRINTF("System area size is %d sectors, need %d blocks\n", fmiNandSysArea, dataAreaStartBlock);

    if ((fmiNandSysArea != 0xFFFFFFFF) && (fmiNandSysArea != 0))
    {
        // System area size difined. It is possible to run update image.
        // Search the start block of Update Area
        sicSMpread(0, imageInfoBlock, pSM0->uPagePerBlock-2, imagebuf);
        if (((*(pImageList+0)) == 0x574255aa) && ((*(pImageList+3)) == 0x57425963))
        {
            imageCount = *(pImageList+1);

            /* pointer to image information */
            pImageList = pImageList+4;
            for (i=0; i<imageCount; i++)
            {
                block = (*(pImageList + 1) & 0xFFFF0000) >> 16;  // block INDEX of ending block of image file
                block++;                                         // block INDEX of start block of Update Area
                if (block > updateAreaStartBlock)    // choose the larger one as start block of Updatea Area
                    updateAreaStartBlock = block;

                /* pointer to next image */
                pImageList = pImageList+12;
            }
        }
//        DBG_PRINTF("Update Area start block is %d. Search update image information...\n", updateAreaStartBlock);

        // search update image information in Update Area
        pImageList=((unsigned int *)(((unsigned int)image_buffer)|0x80000000));
        if (!sicSMpread(0, updateAreaStartBlock, pSM0->uPagePerBlock-2, imagebuf))
        {
            if (((*(pImageList+0)) == 0x574255aa) && ((*(pImageList+3)) == 0x57425963))
            {
                sysprintf("Found update image information in Update Area block %d.\n", updateAreaStartBlock);
                sysprintf("Booting from update system image ...\n");
            }
            else
            {
                sysprintf("Not found update image information in Update Area.\n");
                sysprintf("Booting from original system image ...\n");
                sicSMpread(0, imageInfoBlock, pSM0->uPagePerBlock-2, imagebuf);
            }
        }
    }
    else
    {
        // System area size not defined. Normal booting...
        sysprintf("System Area size not defined. Booting from original system image ...\n");
        // read physical block 0~3 - image information.
        for (i=0; i<4; i++)
        {
            if (!sicSMpread(0, i, pSM0->uPagePerBlock-2, imagebuf))
            {
                if (((*(pImageList+0)) == 0x574255aa) && ((*(pImageList+3)) == 0x57425963))
                {
                    sysprintf("Get image information from block 0x%x ..\n", i);
                    break;
                }
            }
        }
    }

#else

    /* read physical block 0~3 - image information */
    for (i=0; i<4; i++)
    {
        if (!sicSMpread(0, i, pSM0->uPagePerBlock-2, imagebuf))
        {
            if (((*(pImageList+0)) == 0x574255aa) && ((*(pImageList+3)) == 0x57425963))
            {
                sysprintf("Get image information from block 0x%x ..\n", i);
                break;
            }
        }
    }

#endif  // end of #ifdef SUPPORT_NANDUPDATER

    if (((*(pImageList+0)) == 0x574255aa) && ((*(pImageList+3)) == 0x57425963))
    {
        count = *(pImageList+1);

        /* load logo first */
        pImageList = pImageList+4;
        for (i=0; i<count; i++)
        {
            if (((*(pImageList) >> 16) & 0xffff) == 4)  // logo
            {
                image.startBlock = *(pImageList + 1) & 0xffff;
                image.endBlock = (*(pImageList + 1) & 0xffff0000) >> 16;
                image.executeAddr = *(pImageList + 2);
                image.fileLen = *(pImageList + 3);
                MoveData(&image, FALSE);
                break;
            }
            /* pointer to next image */
            pImageList = pImageList+12;
        }

        pImageList=((unsigned int*)(((unsigned int)image_buffer)|0x80000000));
        memset((char *)&image, 0, sizeof(NVT_NAND_INFO_T));

        /* load execution file */
        pImageList = pImageList+4;
        for (i=0; i<count; i++)
        {
            if (((*(pImageList) >> 16) & 0xffff) == 1)  // execute
            {
                image.startBlock = *(pImageList + 1) & 0xffff;
                image.endBlock = (*(pImageList + 1) & 0xffff0000) >> 16;
                image.executeAddr = *(pImageList + 2);
                image.fileLen = *(pImageList + 3);
                MoveData(&image, TRUE);
                break;
            }
            /* pointer to next image */
            pImageList = pImageList+12;
        }
    }
    return 0;
}
