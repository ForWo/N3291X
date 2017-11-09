/****************************************************************************
 *                                                                                    
 * Copyright (c) Nuvoton Technology Corp. All rights reserved.           
 *                                                                                    
 ***************************************************************************/
 
#include <stdio.h>

#include "w55fa95_reg.h"
#include "wblib.h"
#include "wbio.h"
#include "vdodef.h"
#include "vdoapi.h"
#include "avctest.h"
#include "nvtfat.h" 
#include "w55fa95_sic.h"
#include "w55fa95_vpost.h"


void InitClockIC(void)
{	
    // initial system clock
    sysSetSystemClock(eSYS_UPLL,
                    300000000,      // Specified the APLL/UPLL clock, unit Hz
                    300000000);     // Specified the system clock, unit Hz
    sysSetCPUClock (300000000);     // Unit Hz
    sysSetAPBClock ( 75000000);     // Unit Hz
}	



int main(void)
{
	UINT32 u32ExtFreq;
	UINT32 u32PllOutKHz;
	WB_UART_T uart;	
	LCDFORMATEX lcdFormat;

	outp32(REG_AHBCLK,inp32(REG_AHBCLK) | VPOST_CKE | HCLK4_CKE | SIC_CKE | SD_CKE |GVE_CKE | HCLK3_CKE |VDE_CKE);	// enable  IP clock
	
    //--- initial UART
    u32ExtFreq = sysGetExternalClock();
    sysUartPort(1);
    uart.uiFreq = u32ExtFreq;   //use APB clock
    uart.uiBaudrate = 115200;
    uart.uiDataBits = WB_DATA_BITS_8;
    uart.uiStopBits = WB_STOP_BITS_1;
    uart.uiParity = WB_PARITY_NONE;
    uart.uiRxTriggerLevel = LEVEL_1_BYTE;
    uart.uart_no = WB_UART_0;
    sysInitializeUART(&uart);

	//InitClockIC();
	
    u32PllOutKHz = sysGetPLLOutputHz(eSYS_UPLL, u32ExtFreq);
    Console_Printf("PLL out frequency %d Khz\n", u32PllOutKHz);	

    sysInvalidCache();
	sysEnableCache(CACHE_WRITE_BACK);
	
	sysSetTimerReferenceClock(TIMER0, 12000000);	// 12M Xtl In
	sysStartTimer(TIMER0, 1000, PERIODIC_MODE);	
           
    vdoInit();    

	CheckVersion();                             // Get Video Codec F/W version
	
    sicIoctl(SIC_SET_CLOCK, u32PllOutKHz/1000, 0, 0);  	/* clock from PLL */

    
	fsInitFileSystem();
	fmiInitDevice();
	if (sicSdOpen0() <=0)
	{
		Console_Printf("Error in initialize SD card !!\n");
		while(1);
	}


   	/* initialize attached LCM module */
	lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_RGB565;
	vpostLCMInit(&lcdFormat, (UINT32*)VPOSDISPLAYBUFADDR);

	vpeInit();
    
	vdoInstallCallBackFunction(INT_BUF_EMPTY_NUM,       (PVOID)CallBackBufferEmpty);        // Buffer Empty callback
	vdoInstallCallBackFunction(INT_PIC_RUN_NUM,         (PVOID)CallBackPicRun);		        // Pic Run callback
    vdoInstallISR();   
    
	sysEnableInterrupt(IRQ_VDE);			                                        // Enable MP4 Interrupt	    
	sysSetLocalInterrupt(ENABLE_IRQ);		                                        // enable CPSR I bit 
	    
    H264TestPattern();

	sysprintf("All done\n");
	while(1);
    return TRUE;

}


