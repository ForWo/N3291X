/***************************************************************************
 *                                                                         *
 * Copyright (c) 2007 - 2009 Nuvoton Technology Corp. All rights reserved.*
 *                                                                         *
 ***************************************************************************/
 
/****************************************************************************
 * 
 * FILENAME
 *     W55FA95_VPOST_HIMAX_HX7033.c
 *
 * VERSION
 *     0.1 
 *
 * DESCRIPTION
 *
 *
 *
 *
 * DATA STRUCTURES
 *     None
 *
 * FUNCTIONS
 *
 *
 *     
 * HISTORY
 *     2009.03.16		Created by Shu-Ming Fan
 *
 *
 * REMARK
 *     None
 *
 *
 **************************************************************************/
#include "stdio.h"
#include "stdlib.h"
//#include "NUC930_VPOST_Regs.h"
//#include "w55fa93_vpost.h"
#include "w55fa95_vpost.h"
#include "w55fa95_i2c.h"

extern void LCDDelay(unsigned int nCount);

#if defined(HAVE_HIMAX_HX7033)

static UINT32 g_nScreenWidth;
static UINT32 g_nScreenHeight;

//#define OPT_FPGA

#ifdef OPT_FPGA
typedef enum 
{
	eEXT 	= 0,
	eX32K 	= 1,
	eAPLL  	= 2,
	eUPLL  	= 3
}E_CLK;


static UINT32 GetPLLOutputKhz(
	E_CLK eSysPll,
	UINT32 u32FinKHz
	)
{
	UINT32 u32Freq, u32PllCntlReg;
	UINT32 NF, NR, NO;
	
	UINT8 au8Map[4] = {1, 2, 2, 4};
	if(eSysPll==eSYS_APLL)
		u32PllCntlReg = inp32(REG_APLLCON);
	else if(eSysPll==eSYS_UPLL)	
		u32PllCntlReg = inp32(REG_UPLLCON);		
	
	NF = (u32PllCntlReg&FB_DV)+2;
	NR = ((u32PllCntlReg & IN_DV)>>9)+2;
	NO = au8Map[((u32PllCntlReg&OUT_DV)>>14)];
//	sysprintf("PLL regster = 0x%x\n", u32PllCntlReg);	
//	sysprintf("NF = %d\n", NF);
//	sysprintf("NR = %d\n", NR);
//	sysprintf("NOr = %d\n", NO);
		
	u32Freq = u32FinKHz*NF/NR/NO;
//	sysprintf("PLL Freq = %d\n", u32Freq);
	return u32Freq;
}
#endif

static void BacklightControl(int OnOff)
{	
	// GPA[11] set OUTPUT mode  => control the backlight
	outpw(REG_GPIOA_OMD, (inpw(REG_GPIOA_OMD) & 0x0000FFFF)| 0x00000800);
	if(OnOff==TRUE) {
		// GPA[11] turn on the backlight
		outpw(REG_GPIOA_DOUT, (inpw(REG_GPIOA_DOUT) & 0x0000FFFF)| 0x00000800);
	} else {
		// GPA[11] diable backlight
		outpw(REG_GPIOA_DOUT, (inpw(REG_GPIOA_DOUT) & 0x0000FFFF) & 0xFFFFF7FF);
	}
}

static void delay_1mS(UINT32 u32TimeCnt)
{
	UINT32 ii, jj;
	
//	u32TimeCnt *= 4800;		// for 240MHz CPU clock speed
	u32TimeCnt *= 7200;		// for 360MHz CPU clock speed
	for (ii=0; ii<u32TimeCnt; ii++)
		for (jj=0; jj<100; jj++)
			jj++;
}

static void I2C_delay(UINT32 u32TimeCnt)
{
	UINT32 ii, jj;
	
	for (ii=0; ii<u32TimeCnt; ii++)
		for (jj=0; jj<100; jj++)
			jj++;
}

static UINT32 I2C_WriteReg( UINT8 nRegAddr, UINT8 nRegValue )
{
	int j;
	
	i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, nRegAddr, 1);	
	j = 1000;	
	while(j-- > 0) 
	{
		if(i2cWrite(&nRegValue, 1) == 1)
			break;
	}						
//	if(j <= 0)
//		printf("WRITE ERROR !\n");
	
	I2C_delay(20);					
	return j;
}

void HIMAX_HX7033_Init(void)
{
	int rtval,j;
	UINT8 value;
	
	i2cInit();
	rtval = i2cOpen();
//	if(rtval < 0)
//	{
//		printf("Open I2C error!\n");
//		while(1);
//	}	

	i2cIoctl(I2C_IOC_SET_DEV_ADDRESS, 0x48, 0);  
	i2cIoctl(I2C_IOC_SET_SPEED, 100, 0);
//	i2cIoctl(I2C_IOC_SET_SPEED, 50, 0);
	i2cIoctl(I2C_IOC_SET_SINGLE_MASTER, 1, 0); 

#define OPT_GPD2_CONTROL
#ifdef OPT_GPD2_CONTROL
	outpw(REG_GPDFUN, inpw(REG_GPDFUN) & ~MF_GPD2);			// GPD2 becomes GPIO mode
	outpw(REG_GPIOD_OMD, inpw(REG_GPIOD_OMD) | BIT2);		// output mode
//	outpw(REG_GPIOD_PUEN, inpw(REG_GPIOD_PUEN) | BIT2);		// pull-up resistor enabled
	outpw(REG_GPIOD_DOUT, inpw(REG_GPIOD_DOUT) & ~BIT2);	// GPD2 = LOW
#endif	

	I2C_WriteReg(0x00, 0x56);	// up/dowm reversed
	I2C_WriteReg(0x01, 0x00);	// SC disabled
#if 1
	I2C_WriteReg(0x05, 0xF5);
	I2C_WriteReg(0x06, 0x55);	
	I2C_WriteReg(0x07, 0x55);		
	I2C_WriteReg(0x08, 0x55);	
	I2C_WriteReg(0x09, 0x55);	

	// Gamma 2.2 and COM Q28E
	I2C_WriteReg(0x0A, 0xF2);	
	I2C_WriteReg(0x0B, 0x80);	
	I2C_WriteReg(0x0C, 0x00);		
	I2C_WriteReg(0x0D, 0x52);	
	I2C_WriteReg(0x0E, 0x88);	
	I2C_WriteReg(0x0F, 0xAF);		
	I2C_WriteReg(0x10, 0xDF);	
	I2C_WriteReg(0x11, 0xE5);	
	I2C_WriteReg(0x12, 0x1A);		
	I2C_WriteReg(0x13, 0x20);	
	I2C_WriteReg(0x14, 0x50);	
	I2C_WriteReg(0x15, 0x77);		
	I2C_WriteReg(0x16, 0xAD);	
	I2C_WriteReg(0x17, 0xFF);	
	
	// FPC control

#ifdef OPT_GPD2_CONTROL
	// VDDA control	
	delay_1mS(1);		// delay 1 mS
	outpw(REG_GPIOD_DOUT, inpw(REG_GPIOD_DOUT) | BIT2);	// GPD2 = HIGH
	delay_1mS(1);		// delay 1 mS
#endif	
		
	// SC control
	I2C_WriteReg(0x01, 0x80);	// SC enabled
	delay_1mS(5);		// delay 5 mS	
	I2C_WriteReg(0x01, 0x00);	// SC enabled	
	delay_1mS(1);		// delay 1 mS
	
	// backlight turned on
	
#else

	// Gamma 2.2 and COM Q28E
	I2C_WriteReg(0x0A, 0xF2);	
	I2C_WriteReg(0x0B, 0x80);	
	I2C_WriteReg(0x0C, 0x00);		
	I2C_WriteReg(0x0D, 0x56);	
	I2C_WriteReg(0x0E, 0x8B);	
	I2C_WriteReg(0x0F, 0xB1);		
	I2C_WriteReg(0x10, 0xE0);	
	I2C_WriteReg(0x11, 0xE5);	
	I2C_WriteReg(0x12, 0x1A);		
	I2C_WriteReg(0x13, 0x1F);	
	I2C_WriteReg(0x14, 0x4E);	
	I2C_WriteReg(0x15, 0x74);		
	I2C_WriteReg(0x16, 0xA9);	
	I2C_WriteReg(0x17, 0xFF);		
#endif
}

static INT Clock_Control(void)
{
}

INT vpostLCMInit_HIMAX_HX7033(PLCDFORMATEX plcdformatex, UINT32 *pFramebuf)
{
	volatile S_DRVVPOST_SYNCLCM_WINDOW sWindow = {320,240,320};	
	volatile S_DRVVPOST_SYNCLCM_HTIMING sHTiming = {2,0x27,(UINT8)0x07};
	volatile S_DRVVPOST_SYNCLCM_VTIMING sVTiming = {2,0x8,0x09};
//	volatile S_DRVVPOST_SYNCLCM_POLARITY sPolarity = {FALSE,FALSE,TRUE,FALSE};
	volatile S_DRVVPOST_SYNCLCM_POLARITY sPolarity = {TRUE,TRUE,FALSE,FALSE};

	UINT32 nBytesPixel, u32PLLclk, u32ClockDivider;

//#define OPT_24BIT_MODE

	// VPOST clock control
	outpw(REG_AHBCLK, inpw(REG_AHBCLK) | VPOST_CKE | HCLK4_CKE);
	outpw(REG_AHBIPRST, inpw(REG_AHBIPRST) | VPOSTRST);
	outpw(REG_AHBIPRST, inpw(REG_AHBIPRST) & ~VPOSTRST);	
	
#ifndef OPT_FPGA
	u32PLLclk = sysGetPLLOutputHz(eSYS_UPLL, sysGetExternalClock());
//	u32ClockDivider = u32PLLclk / 15000000;
//	u32ClockDivider = u32PLLclk / 9000000;
//	u32ClockDivider = u32PLLclk / 22000000;
	u32ClockDivider = u32PLLclk / 13000000;
	
	u32ClockDivider /= 3;
	u32ClockDivider--;
//	outpw(REG_CLKDIV1, (inpw(REG_CLKDIV1) & ~VPOST_N0) | 1);						
	outpw(REG_CLKDIV1, (inpw(REG_CLKDIV1) & ~VPOST_N0) | 2);						
	outpw(REG_CLKDIV1, (inpw(REG_CLKDIV1) & ~VPOST_N1) | ((u32ClockDivider & 0xFF) << 8));						
	outpw(REG_CLKDIV1, inpw(REG_CLKDIV1) & ~VPOST_S);
	outpw(REG_CLKDIV1, inpw(REG_CLKDIV1) | (3<<3));		// VPOST clock from UPLL		
#else
//	u32PLLclk = sysGetPLLOutputKhz(eSYS_UPLL, 27000);	// CLK_IN = 27 MHz
//	u32PLLclk = sysGetPLLOutputKhz(eSYS_UPLL, 12000);	// CLK_IN = 12 MHz
	u32PLLclk = GetPLLOutputKhz(eUPLL, 12000);			// CLK_IN = 12 MHz
	u32ClockDivider = u32PLLclk / 9000;
	
	u32ClockDivider /= 2;
	outpw(REG_CLKDIV1, (inpw(REG_CLKDIV1) & ~VPOST_N0) | 1);						
	outpw(REG_CLKDIV1, (inpw(REG_CLKDIV1) & ~VPOST_N1) | ((u32ClockDivider & 0xFF) << 8));						
	outpw(REG_CLKDIV1, inpw(REG_CLKDIV1) & ~VPOST_S);
	outpw(REG_CLKDIV1, inpw(REG_CLKDIV1) | (3<<3));		// VPOST clock from UPLL		
#endif

	vpostVAStopTrigger();	

	// Enable VPOST function pins
#ifdef	OPT_24BIT_MODE		
	vpostSetDataBusPin(eDRVVPOST_DATA_24BITS);
#else
//	vpostSetDataBusPin(eDRVVPOST_DATA_16BITS);
	vpostSetDataBusPin(eDRVVPOST_DATA_18BITS);	
#endif	
		  
	// LCD image source select
	vpostSetLCM_ImageSource(eDRVVPOST_FRAME_BUFFER);
	
	// configure LCD interface
	vpostSetLCM_TypeSelect(eDRVVPOST_HIGH_RESOLUTINO_SYNC);

	// configure LCD timing sync or async with TV timing	
	vpostsetLCM_TimingType(eDRVVPOST_ASYNC_TV);
	
    // Configure Parallel LCD interface (16/18/24-bit data bus)
#ifdef	OPT_24BIT_MODE		
    vpostSetParalelSyncLCM_Interface(eDRVVPOST_PRGB_24BITS);
#else    
//    vpostSetParalelSyncLCM_Interface(eDRVVPOST_PRGB_16BITS);
    vpostSetParalelSyncLCM_Interface(eDRVVPOST_PRGB_18BITS);    
#endif    
    
	HIMAX_HX7033_Init();
	
    // set Horizontal scanning line timing for Syn type LCD 
    vpostSetSyncLCM_HTiming(&sHTiming);

	// set Vertical scanning line timing for Syn type LCD   
    vpostSetSyncLCM_VTiming(&sVTiming);
	
	// set both "active pixel per line" and "active lines per screen" for Syn type LCD   
	vpostSetSyncLCM_ImageWindow(&sWindow);

  	// set Hsync/Vsync/Vden/Pclk poalrity
	vpostSetSyncLCM_SignalPolarity(&sPolarity);  	
    
#if 0
	vpostSetFrameBuffer_BaseAddress(pFramebuf);
#else    
    // set frambuffer base address
    if(pFramebuf != NULL) {
		vpostAllocVABufferFromAP(pFramebuf);
	} else {
    	if( vpostAllocVABuffer(plcdformatex, nBytesPixel)==FALSE)
    		return ERR_NULL_BUF;
    }
#endif    
	
	// set frame buffer data format
	vpostSetFrameBuffer_DataType(plcdformatex->ucVASrcFormat);
	
	vpostSetYUVEndianSelect(eDRVVPOST_YUV_LITTLE_ENDIAN);

	outpw(REG_LCM_TCON1, inpw(REG_LCM_TCON1) | 0x3a0000);
	outpw(REG_LCM_KPI_HS_DLY, 0x00320038);	

	// enable LCD controller
	vpostVAStartTrigger();
	
//	BacklightControl(TRUE);			
	return 0;
}

INT32 vpostLCMDeinit_HIMAX_HX7033(VOID)
{
	vpostVAStopTrigger();
	vpostFreeVABuffer();
	outpw(REG_AHBCLK, inpw(REG_AHBCLK) & ~VPOST_CKE);	
	return 0;
}
#endif    //HAVE_HIMAX_HX7033