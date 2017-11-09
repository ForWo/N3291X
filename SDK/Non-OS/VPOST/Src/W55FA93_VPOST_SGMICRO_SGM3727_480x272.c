/***************************************************************************
 *                                                                         *
 * Copyright (c) 2007 - 2009 Nuvoton Technology Corp. All rights reserved.*
 *                                                                         *
 ***************************************************************************/
 
/****************************************************************************
 * 
 * FILENAME
 *     W55FA93_VPOST_HANNSTAR_HSD043I9W1.c
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
 *     2010.09.21		Created by xxx
 *
 *
 * REMARK
 *     None
 *
 * 
 **************************************************************************/
#include "stdio.h"
#include "stdlib.h"
#include "wblib.h"

#include "w55fa95_vpost.h"

extern void LCDDelay(unsigned int nCount);

#ifdef HAVE_SGMICRO_SGM3727

static UINT32 g_nScreenWidth;
static UINT32 g_nScreenHeight;


static INT Clock_Control(void)
{
}

INT vpostLCMInit_SGMICRO_SGM3727(PLCDFORMATEX plcdformatex, UINT32 *pFramebuf)
{
	volatile S_DRVVPOST_SYNCLCM_WINDOW sWindow = {480,272,480};
	volatile S_DRVVPOST_SYNCLCM_HTIMING sHTiming = {5,0x20,(UINT8)0x08};
	volatile S_DRVVPOST_SYNCLCM_VTIMING sVTiming = {2,0x8,0x6};
	volatile S_DRVVPOST_SYNCLCM_POLARITY sPolarity = {TRUE,TRUE,FALSE,TRUE};	// Vsync, VDEN, Hsync, Clock
	
	UINT32 nBytesPixel, u32PLLclk, u32ClockDivider, u32Clkin;
//	WB_CLKFREQ_T pllClock;

#define OPT_24BIT_MODE

	// VPOST clock control
//	Clock_Control();
	outpw(REG_AHBCLK, inpw(REG_AHBCLK) | VPOST_CKE | HCLK4_CKE);
	outpw(REG_AHBIPRST, inpw(REG_AHBIPRST) | VPOSTRST);
	outpw(REG_AHBIPRST, inpw(REG_AHBIPRST) & ~VPOSTRST);	
	
	u32PLLclk = sysGetPLLOutputHz(eSYS_UPLL, sysGetExternalClock());
	u32ClockDivider = u32PLLclk / 9000000;
	u32ClockDivider --;
	
	outpw(REG_CLKDIV1, inpw(REG_CLKDIV1) & ~VPOST_N0);						
	outpw(REG_CLKDIV1, (inpw(REG_CLKDIV1) & ~VPOST_N1) | ((u32ClockDivider & 0xFF) << 8));						
	outpw(REG_CLKDIV1, inpw(REG_CLKDIV1) & ~VPOST_S);
	outpw(REG_CLKDIV1, inpw(REG_CLKDIV1) | (3<<3));		// VPOST clock from UPLL		

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
	
	// Configure LCD interface
	vpostSetLCM_TypeSelect(eDRVVPOST_HIGH_RESOLUTINO_SYNC);

	// Configure LCD timing sync or async with TV timing	
	vpostsetLCM_TimingType(eDRVVPOST_ASYNC_TV);
	
    // Configure Parallel LCD interface (16/18/24-bit data bus)
#ifdef	OPT_24BIT_MODE		
    vpostSetParalelSyncLCM_Interface(eDRVVPOST_PRGB_24BITS);
#else    
//	vpostSetParalelSyncLCM_Interface(eDRVVPOST_PRGB_16BITS);
    vpostSetParalelSyncLCM_Interface(eDRVVPOST_PRGB_18BITS);    
#endif    
    
//	HANNSTAR_HSD043I9W1_Init();
	
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
	
	// for KPI
	outpw(REG_LCM_TCON1, (inpw(REG_LCM_TCON1) & 0xFF000000) | 0x003A1F07);
	outpw(REG_LCM_KPI_HS_DLY, 0x00320038);
	
	// enable LCD controller
	vpostVAStartTrigger();
	
	// Backlight enabled
	
	outpw(REG_GPFFUN, inpw(REG_GPFFUN) & ~MF_GPF10);	// TDI for GPD3 
	outpw(REG_GPDFUN, inpw(REG_GPDFUN) & ~MF_GPD3);		// GPD3 in GPIO mode
	outpw(REG_GPIOD_OMD, inpw(REG_GPIOD_OMD) | BIT3);	// GPD3 in output mode
	outpw(REG_GPIOD_DOUT, inpw(REG_GPIOD_DOUT) | BIT3);	// GPD3 output to High

	return 0;
}

INT vpostLCMDeinit_SGMICRO_SGM3727(void)
{
	vpostVAStopTrigger();
	vpostFreeVABuffer();
	outpw(REG_AHBCLK, inpw(REG_AHBCLK) & ~VPOST_CKE);	
}
#endif    //HAVE_HANNSTAR_HSD043I9W1