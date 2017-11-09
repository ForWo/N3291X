/***************************************************************************
 *                                                                         *
 * Copyright (c) 2007 - 2009 Nuvoton Technology Corp. All rights reserved.*
 *                                                                         *
 ***************************************************************************/
 
/****************************************************************************
 * 
 * FILENAME
 *     W55FA95_VPOST_ILITEK_ILI9322.c
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
#include "w55fa95_vpost.h"

extern void LCDDelay(unsigned int nCount);

#if defined(HAVE_ILITEK_ILI9322)

static UINT32 g_nScreenWidth;
static UINT32 g_nScreenHeight;

//#define OPT_FPGA

#ifndef OPT_FPGA
typedef enum 
{
	eEXT 	= 0,
	eX32K 	= 1,
	eAPLL  	= 2,
	eUPLL  	= 3
}E_CLK;


#define SpiEnable()		outp32(REG_GPIOD_DOUT, inp32(REG_GPIOD_DOUT) & ~0x2000)
#define SpiDisable()	outp32(REG_GPIOD_DOUT, inp32(REG_GPIOD_DOUT) |  0x2000)	
#define SpiHighSCK()	outp32(REG_GPIOD_DOUT, inp32(REG_GPIOD_DOUT) |  0x4000)	
#define SpiLowSCK()		outp32(REG_GPIOD_DOUT, inp32(REG_GPIOD_DOUT) & ~0x4000)
#define SpiHighSDA()	outp32(REG_GPIOD_DOUT, inp32(REG_GPIOD_DOUT) |  0x8000)	
#define SpiLowSDA()		outp32(REG_GPIOD_DOUT, inp32(REG_GPIOD_DOUT) & ~0x8000)

///	#define SpiEnable()		outp32(REG_GPIOD_DOUT, inp32(REG_GPIOD_DOUT) & ~0x0800)
///	#define SpiDisable()	outp32(REG_GPIOD_DOUT, inp32(REG_GPIOD_DOUT) |  0x0800)
///	#define SpiHighSCK()	outp32(REG_GPIOB_DOUT, inp32(REG_GPIOB_DOUT) |  0x2000)	
///	#define SpiLowSCK()		outp32(REG_GPIOB_DOUT, inp32(REG_GPIOB_DOUT) & ~0x2000)
///	#define SpiHighSDA()	outp32(REG_GPIOB_DOUT, inp32(REG_GPIOB_DOUT) |  0x4000)	
///	#define SpiLowSDA()		outp32(REG_GPIOB_DOUT, inp32(REG_GPIOB_DOUT) & ~0x4000)

void SpiDelay(UINT32 u32TimeCnt)
{
	UINT32 ii, jj;
	
	for (ii=0; ii<u32TimeCnt; ii++)
		for (jj=0; jj<200; jj++)
			jj++;
}

static void timer_10ms(int count)
{
	volatile unsigned int btime, etime;
	
	btime = sysGetTicks(TIMER0);
	while (1)
	{			
		etime = sysGetTicks(TIMER0);
		if ((etime - btime) >= count)
			break;			
	}
}


void SpiInit()
{	
	outp32(REG_GPIOD_OMD, inp32(REG_GPIOD_OMD) | 0xE000);
	outp32(	REG_GPIOD_PUEN, inp32(REG_GPIOD_PUEN) | 0x8000);		
}

void ILI_RdWtRegInit()
{
	SpiInit();
	SpiLowSDA();		// serial data pin low
	SpiHighSCK();		// serial clock pin low
	SpiDisable();
}

void ILITek_WtReg(UINT8 nRegAddr, UINT8 nData)
{
	UINT32 i;
	
	nRegAddr <<= 1;
	SpiEnable();
	SpiHighSCK();		
	
	// send WR bit		
	SpiLowSCK();
	SpiDelay(2);		
	SpiLowSDA();		
	SpiDelay(2);
	SpiHighSCK();		
	SpiDelay(2);		
	
	// Send register address, MSB first
	for( i = 0; i < 7; i ++ )
	{
		SpiLowSCK();				
		if ( nRegAddr&0x80 )
			SpiHighSDA();
		else
			SpiLowSDA();
		
		SpiDelay(3);
		SpiHighSCK();
		nRegAddr<<=1;
		SpiDelay(20);
	}
	// Send register data LSB first
	for( i = 0; i < 8; i ++ )
	{
		SpiLowSCK();						
		if ( nData&0x80 )
			SpiHighSDA();
		else
			SpiLowSDA();
		
		SpiDelay(3);
		SpiHighSCK();
		nData<<=1;
		SpiDelay(20);
	}
	SpiDisable();
}

void ILI_I9322_CCIR656_720Y_NTSC_INIT(void)
{

//		outp32(REG_LCM_LCDCCtl, 0);			// Disable VPOST engine

	ILI_RdWtRegInit();

	ILITek_WtReg(0x04,0x00);		// Reset to all registers to default values
	ILITek_WtReg(0x04,0x01);		// Normal operation
	
	ILITek_WtReg(0x06,0xAC);		// Bit[7:4] = 0xA --> CCIR656 720x480	
//	ILITek_WtReg(0x06,0xBF);		// Bit[7:4] = 0xB --> CCIR656 640x480
									// Bit[3:2] = 0x3 --> auto detection mode for NTSC/PAL
									// Bit[1:1] = 0x1 --> up-to-down scan
									// Bit[0:0] = 0x1 --> left-to-right scan										
									
//	ILITek_WtReg(0x09,0x7f);		

	ILITek_WtReg(0x0A,0x49);			
	
	timer_10ms(2);												
	ILITek_WtReg(0x07,0xEE);		// turn off charge-pump
	timer_10ms(4);														
	ILITek_WtReg(0x07,0xEF);		// turn off charge-pump		
	timer_10ms(5);					// delay at least 80 ms															
}			

void ILI_I9322_CCIR656_640Y_NTSC_INIT(void)
{

//		outp32(REG_LCM_LCDCCtl, 0);			// Disable VPOST engine

	ILI_RdWtRegInit();

	ILITek_WtReg(0x04,0x00);		// Reset to all registers to default values
	ILITek_WtReg(0x04,0x01);		// Normal operation
	
//	ILITek_WtReg(0x06,0xAC);		// Bit[7:4] = 0xA --> CCIR656 720x480	
//	ILITek_WtReg(0x06,0xBF);		// Bit[7:4] = 0xB --> CCIR656 640x480
	ILITek_WtReg(0x06,0xBC);		// Bit[7:4] = 0xB --> CCIR656 640x480	
									// Bit[3:2] = 0x3 --> auto detection mode for NTSC/PAL
									// Bit[1:1] = 0x1 --> up-to-down scan
									// Bit[0:0] = 0x1 --> left-to-right scan										
									
	ILITek_WtReg(0x0A,0x49);		
	
	timer_10ms(2);												
	ILITek_WtReg(0x07,0xEE);		// turn off charge-pump
	timer_10ms(4);														
	ILITek_WtReg(0x07,0xEF);		// turn off charge-pump		
	timer_10ms(5);					// delay at least 80 ms															
}			

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

INT vpostLCMInit_ILITEK_ILI9322(PLCDFORMATEX plcdformatex, UINT32 *pFramebuf)
{
	volatile S_DRVVPOST_SYNCLCM_HTIMING sHTiming = {3, 0xF0,(UINT8)(1560-1280-241)};	// Horizontal direction is decreased by 1
	volatile S_DRVVPOST_SYNCLCM_VTIMING sVTiming = {2,21,1};			// Vertical direction is not decreased by 1
	volatile S_DRVVPOST_SYNCLCM_WINDOW sWindow = {640*2,480, 640};	
	volatile S_DRVVPOST_SYNCLCM_POLARITY sPolarity = {TRUE,TRUE,FALSE,TRUE};
	UINT32 nBytesPixel, u32PLLclk, u32ClockDivider;


//#define CCIR656_640Y
#define CCIR656_720Y


	// reset LCD panel
	{
		outp32(REG_GPIOD_OMD, inp32(REG_GPIOD_OMD) | 0x1000);	
		outp32(REG_GPIOD_DOUT, inp32(REG_GPIOD_DOUT) | 0x1000);	
		timer_10ms(1);
		outp32(REG_GPIOD_DOUT, inp32(REG_GPIOD_DOUT) & ~0x1000);		
		timer_10ms(2);
		outp32(REG_GPIOD_DOUT, inp32(REG_GPIOD_DOUT) | 0x1000);	
	}		
	
#ifdef CCIR656_640Y
	  	ILI_I9322_CCIR656_640Y_NTSC_INIT();		
#endif
#ifdef CCIR656_720Y
	  	ILI_I9322_CCIR656_720Y_NTSC_INIT();		
#endif

	// VPOST clock control
	outpw(REG_AHBCLK, inpw(REG_AHBCLK) | VPOST_CKE | HCLK4_CKE);
	outpw(REG_AHBIPRST, inpw(REG_AHBIPRST) | VPOSTRST);
	outpw(REG_AHBIPRST, inpw(REG_AHBIPRST) & ~VPOSTRST);	

	
	u32PLLclk = sysGetPLLOutputHz(eSYS_UPLL, sysGetExternalClock());
	u32ClockDivider = u32PLLclk / 27000000;
	
	u32ClockDivider /= 2;
	u32ClockDivider--;
	outpw(REG_CLKDIV1, (inpw(REG_CLKDIV1) & ~VPOST_N0) | 1);						
	outpw(REG_CLKDIV1, (inpw(REG_CLKDIV1) & ~VPOST_N1) | ((u32ClockDivider & 0xFF) << 8));						
	outpw(REG_CLKDIV1, inpw(REG_CLKDIV1) & ~VPOST_S);
	outpw(REG_CLKDIV1, inpw(REG_CLKDIV1) | (3<<3));		// VPOST clock from UPLL
	
	// Enable VPOST function pins
	vpostSetDataBusPin(eDRVVPOST_DATA_8BITS);	

	// configure LCD timing sync or async with TV timing	
	vpostsetLCM_TimingType(eDRVVPOST_SYNC_TV);

	// LCD image source select
	vpostSetLCM_ImageSource(eDRVVPOST_FRAME_BUFFER);

	// LCD type (sync/MPU/High definition)
	vpostSetLCM_TypeSelect(eDRVVPOST_SYNC);	

	// set Horizontal scanning pixel timing for Syn type LCD   
    vpostSetSyncLCM_HTiming((S_DRVVPOST_SYNCLCM_HTIMING *)&sHTiming);

	// set Vertical scanning line timing for Syn type LCD   
    vpostSetSyncLCM_VTiming((S_DRVVPOST_SYNCLCM_VTIMING *)&sVTiming);
	
	// set both "active pixel per line" and "active lines per screen" for Syn type LCD   
	vpostSetSyncLCM_ImageWindow((S_DRVVPOST_SYNCLCM_WINDOW *)&sWindow);

	// set frame buffer data format
	vpostSetSerialSyncLCM_Interface(eDRVVPOST_SRGB_CCIR656);


	// TV display position adjustment
//  	outpw(REG_TVOUT_ADJ, 0x10000000);		


#ifdef CCIR656_640Y
    // TV control register
    vpostSetTVEnableConfig(	eDRVVPOST_LCD_VGA, 				/* Frame Buffer Size in TV */
    						eDRVVPOST_FRAME_BUFFER, 		/* LCD Color Source */
    						eDRVVPOST_FRAME_BUFFER, 		/* TV Color Source */
    						0,								/* TV DAC 1:Disable 0:Enable */
    						1, 								/* 1:Interlance 0:Non-Interlance */
    						0, 								/* TV System Select 1:PAL 0:NTSC */
    						1								/* TV Encoder 1:enable 0:disable */
    						);
#endif    						
#ifdef CCIR656_720Y    						
    // TV control register
    vpostSetTVEnableConfig(	eDRVVPOST_LCD_D1, 				/* Frame Buffer Size in TV */
    						eDRVVPOST_FRAME_BUFFER, 		/* LCD Color Source */
    						eDRVVPOST_FRAME_BUFFER, 		/* TV Color Source */
    						0,								/* TV DAC 1:Disable 0:Enable */
    						1, 								/* 1:Interlance 0:Non-Interlance */
    						0, 								/* TV System Select 1:PAL 0:NTSC */
    						1								/* TV Encoder 1:enable 0:disable */
    						);
#endif

	// set frame buffer data format
	vpostSetFrameBuffer_DataType(plcdformatex->ucVASrcFormat);
	
	vpostSetYUVEndianSelect(eDRVVPOST_YUV_LITTLE_ENDIAN);
	
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

#ifdef CCIR656_640Y
	  	outp32(REG_LCM_LCDCCtl, inp32(REG_LCM_LCDCCtl) | LCDCCtl_HAW_656);
#endif
#ifdef CCIR656_720Y
	  	outp32(REG_LCM_LCDCCtl, inp32(REG_LCM_LCDCCtl) & ~LCDCCtl_HAW_656);
#endif

	// trigger to display
	vpostVAStartTrigger();
	return 0;
}

INT32 vpostLCMDeinit_ILITEK_ILI9322(VOID)
{
	vpostVAStopTrigger();
	vpostFreeVABuffer();
	outpw(REG_AHBCLK, inpw(REG_AHBCLK) & ~VPOST_CKE);	
	return 0;
}
#endif    //HAVE_ILITEK_ILI9322