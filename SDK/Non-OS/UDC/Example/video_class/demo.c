/***************************************************************************
 *                                                                         *
 * Copyright (c) 2008 Nuvoton Technolog. All rights reserved.              *
 *                                                                         *
 ***************************************************************************/

#include <stdio.h>
#include "wblib.h"

#include "W55FA95_GPIO.h"
#include "W55FA95_VideoIn.h"
#include "demo.h"
#include "jpegcodec.h"
#include "usbd.h"
#include "videoclass.h"
	
void init(void)
{
	WB_UART_T uart;
	UINT32 u32ExtFreq;
	
	/* Cache on */ 
	sysInvalidCache();
	sysDisableCache();
	sysEnableCache(CACHE_WRITE_BACK);
	
	/* Init UART */
	u32ExtFreq = sysGetExternalClock();
	sysUartPort(1);
	uart.uiFreq = u32ExtFreq;	//use Ext clock
	uart.uiBaudrate = 115200;
	uart.uiDataBits = WB_DATA_BITS_8;
	uart.uiStopBits = WB_STOP_BITS_1;
	uart.uiParity = WB_PARITY_NONE;
	uart.uiRxTriggerLevel = LEVEL_1_BYTE;
	uart.uart_no = WB_UART_0;
	sysInitializeUART(&uart);
	sysprintf("UART Init\n");
	/* Init Timer */
	u32ExtFreq = sysGetExternalClock();	
	sysSetTimerReferenceClock(TIMER0, u32ExtFreq); //External Crystal	
	sysStartTimer(TIMER0, 100, PERIODIC_MODE);		/* 100 ticks/per sec ==> 1tick/10ms */	
	sysSetLocalInterrupt(ENABLE_FIQ_IRQ);			
		
}			
int main()
{
	/*Due to I2C share pins with UART, the SerialIO can not be used*/
	init();	
	
	uvcPuInfo.PU_BACKLIGHT_COMPENSATION_MIN = 0;
	uvcPuInfo.PU_BACKLIGHT_COMPENSATION_MAX = 255;
	uvcPuInfo.PU_BACKLIGHT_COMPENSATION_DEF = 4;
	uvcPuInfo.PU_BRIGHTNESS_MIN = 0;
	uvcPuInfo.PU_BRIGHTNESS_MAX = 255;
	uvcPuInfo.PU_BRIGHTNESS_DEF = 8;
	uvcPuInfo.PU_CONTRAST_MIN = 0;
	uvcPuInfo.PU_CONTRAST_MAX = 255;
	uvcPuInfo.PU_CONTRAST_DEF = 16;
	uvcPuInfo.PU_HUE_MIN = 0;
	uvcPuInfo.PU_HUE_MAX = 255;
	uvcPuInfo.PU_HUE_DEF = 32;
	uvcPuInfo.PU_SATURATION_MIN = 0;
	uvcPuInfo.PU_SATURATION_MAX = 255;
	uvcPuInfo.PU_SATURATION_DEF = 64;
	uvcPuInfo.PU_SHARPNESS_MIN = 0;
	uvcPuInfo.PU_SHARPNESS_MAX =255;
	uvcPuInfo.PU_SHARPNESS_DEF = 128;
	uvcPuInfo.PU_GAMMA_MIN = 0;
	uvcPuInfo.PU_GAMMA_MAX = 255;
	uvcPuInfo.PU_GAMMA_DEF = 192;
	uvcPuInfo.PU_POWER_LINE_FREQUENCY_MIN = 0;
	uvcPuInfo.PU_POWER_LINE_FREQUENCY_MAX = 255;
	uvcPuInfo.PU_POWER_LINE_FREQUENCY_DEF = 255; 		
	
#ifdef __UVC_VIN__	

	sysprintf("Plug in sensor OV7225 to GPB\n");	
	videoIn_Port(0); 
		
	Smpl_NT99141_VGA(u8PacketFrameBuffer0, u8PacketFrameBuffer1);		 	
	jpegOpen ();    
#endif		
	uvc_main();		
	while(1);
    return 0;
} /* end main */