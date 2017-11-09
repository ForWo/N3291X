/****************************************************************************
 *
 **************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "wbio.h"
#include "wblib.h"
#include "wbtypes.h"
//#include "WBChipDef.h"
//#include "usbd.h"

#include "w55fa95_vpost.h"
#include "w55fa95_reg.h"
//#include "w55fa95_kpi.h"

__align (32) UINT8 g_ram0[512*16*16];
__align (32) UINT8 g_ram1[512*16*16];
UINT32 u32SecCnt;
UINT32 u32backup[10];


__align(32) UINT8 Vpost_Frame[]=
{
//	#include "roof_800x600_rgb565.dat"		// for SVGA size test
//	#include "sea_800x480_rgb565.dat"		
//	#include "roof_800x480_rgb565.dat"			
//	#include "roof_720x480_rgb565.dat"		// for D1 size test
//	#include "lake_720x480_rgb565.dat"		// for D1 size test
	#include "mountain_640x480_rgb565.dat"	// for VGA size test	
//	#include "river_480x272_rgb565.dat"
//	#include "roof_320x240_rgb565.dat"	
//	#include "demo_rgb565.dat"	
//	#include "roof_320x240_yuv422.dat"	
//	#include "roof_320x240_rgbx888.dat"		
};


LCDFORMATEX lcdFormat;

int volatile gTotalSectors_SD = 0, gTotalSectors_SM = 0;


void initPLL(void)
{
	WB_UART_T 	uart;
	UINT32 		u32ExtFreq;	    	    	
	UINT32 u32Cke = inp32(REG_AHBCLK);

	/* init timer */	
	u32ExtFreq = sysGetExternalClock();    	/* Hz unit */	
	sysSetTimerReferenceClock (TIMER0, 
								u32ExtFreq);	/* Hz unit */
	sysStartTimer(TIMER0, 100, PERIODIC_MODE);
					
	sysSetLocalInterrupt(ENABLE_IRQ);
}


int count = 0;
int STATUS;

int main(void)
{
	int color_slct = 0, tick, key;
	
//	InitUART();
//	g_SetupTOK =0;
//	g_SetupPKT=0;
//	g_OutTOK = 0;
//	g_SetupInTOK=0;
//	g_TXD=0;
//	g_RXD = 0;
	 
//	DrvSIO_printf("TEST %d\n",count);

	
	initPLL();
	
	lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_RGB565;
//	lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_YCBYCR;
//	lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_RGBx888;

//	lcdFormat.nScreenWidth = 800;
//	lcdFormat.nScreenHeight = 480;

//	lcdFormat.nScreenWidth = 800;
//	lcdFormat.nScreenHeight = 600;

	lcdFormat.nScreenWidth = 640;
	lcdFormat.nScreenHeight = 480;
	
//	lcdFormat.nScreenWidth = 720;
//	lcdFormat.nScreenHeight = 480;
	
//	lcdFormat.nScreenWidth = 480;
//	lcdFormat.nScreenHeight = 272;
//	lcdFormat.nScreenWidth = 320;
//	lcdFormat.nScreenHeight = 240;
	
	vpostLCMInit(&lcdFormat, (UINT32*)Vpost_Frame);
	
#if 0
 	kpi_init();
	kpi_open(0); // use nIRQ0 as external interrupt source
	key = kpi_read(KPI_NONBLOCK);
	
	outpw(REG_LCM_TVCtl, (inpw(REG_LCM_TVCtl) & ~TVCtl_LCDSrc) | (0x02 << 10));
	while(1)
	{
		switch(color_slct)
		{
			case 0:
				outpw(REG_LCM_COLORSET, 0xFF0000);		// red color
				break;

			case 1:
				outpw(REG_LCM_COLORSET, 0x00FF00);		// green color
				break;

			case 2:
				outpw(REG_LCM_COLORSET, 0x0000FF);		// blue color
				break;

			case 3:
				outpw(REG_LCM_COLORSET, 0x000000);		// black color
				break;
		
			case 4:
			default:			
				outpw(REG_LCM_COLORSET, 0xFFFFFF);		// white color
				color_slct = -1;				
				break;
		}
		while(!kpi_read(KPI_NONBLOCK));
		tick = sysGetTicks(TIMER0);
		while ((sysGetTicks(TIMER0) - tick) < 100);
		color_slct++;	
	}
#endif	
	while(1);

}


