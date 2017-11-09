/****************************************************************************
*                                                                           *
* Copyright (c) 2009 Nuvoton Tech. Corp. All rights reserved.               *
*                                                                           *
*****************************************************************************/

/* PLL clock option */
//#define __UPLL_NOT_SET__
//#define __UPLL_300__
#define __UPLL_288__
//#define __UPLL_264__

#define __DAC_ON__


/* Start for option for VPOST frame buffer */
#if defined(__TV__)
	#ifdef __TV_QVGA__ 
	#define PANEL_WIDTH		320
	#define PANEL_HEIGHT	240
	#else
	#define PANEL_WIDTH		640
	#define PANEL_HEIGHT	480
	#endif
#elif defined( __LCM_800x600__) 
	#define PANEL_WIDTH		800
	#define PANEL_HEIGHT	600
#elif defined( __LCM_480x272__)
	#define PANEL_WIDTH		480
	#define PANEL_HEIGHT	272
#elif defined( __LCM_800x480__)
	#define PANEL_WIDTH		800
	#define PANEL_HEIGHT	480
#elif defined( __LCM_QVGA__)
	#define PANEL_WIDTH		320
	#define PANEL_HEIGHT	240
#elif defined( __LCM_128x64__)
	#define PANEL_WIDTH		128
	#define PANEL_HEIGHT	64	
#else 	
	#define PANEL_WIDTH	480
	#define PANEL_HEIGHT	272
#endif
/* End for option for VPOST frame buffer */

#define PANEL_BPP		2
#define FB_ADDR		0x500000


#ifdef __DEBUG__
#define DBG_PRINTF		sysprintf
#else
#define DBG_PRINTF(...)		
#endif
 
 VOID SPI_OpenSPI(VOID);
 int SPIReadFast(BOOL bEDMAread, UINT32 addr, UINT32 len, UINT32 *buf);
 
 /* Turn on the optional. Back light enable */
 /* Turn off the optional, ICE can connect to */
 /* Default Demo Board  GPD1 keep pull high */
 /* 								*/ 
//#define __BACKLIGHT__
 

/* NAND1-1 Size */
#define NAND1_1_SIZE	 32	/* MB unit */
#define SD1_1_SIZE	 	128 /* MB unit */ 