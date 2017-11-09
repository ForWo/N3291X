#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wblib.h"
#include "w55fa95_reg.h"
#include "usbd.h"
#include "HID.h"
#ifdef HID_KEYPAD  

#include "w55fa95_kpi.h"
#include "w55fa95_vpost.h"

LCDFORMATEX lcdFormat;	
__align(32) UINT8 Vpost_Frame[]=
{
	#include "..\..\VPOST\Example\ASIC\river_480x272_rgb565.dat"
};

#endif
	
int main(void)
{
	WB_UART_T uart;
	UINT32 u32ExtFreq;
	
	u32ExtFreq = sysGetExternalClock();    	/* Hz unit */		
	uart.uiFreq = u32ExtFreq;
    uart.uiBaudrate = 115200;
    uart.uiDataBits = WB_DATA_BITS_8;
    uart.uiStopBits = WB_STOP_BITS_1;
    uart.uiParity = WB_PARITY_NONE;
    uart.uiRxTriggerLevel = LEVEL_1_BYTE;
    sysInitializeUART(&uart);	

    sysprintf("Sample code Start\n");	

#ifdef HID_KEYPAD  
	lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_RGB565;
	lcdFormat.nScreenWidth = 480;
	lcdFormat.nScreenHeight = 272;

	vpostLCMInit(&lcdFormat, (UINT32*)Vpost_Frame);

	outpw(REG_LCM_TCON1, (inpw(REG_LCM_TCON1) |0x3A0000));
	outpw(0xB100209C, 0x00320038);
	
	kpi_init();
	kpi_open(0); 
#endif
	/* Enable USB */
	udcOpen();  
	
	hidInit();
	udcInit();
	
	while(1)
	{
#ifdef HID_KEYBOARD
		HID_SetInReport();
#endif
#ifdef HID_MOUSE
		HID_UpdateMouseData();
#endif	
	};
}

