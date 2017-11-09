/****************************************************************************
*                                                                           *
* Copyright (c) 2009 Nuvoton Tech. Corp. All rights reserved.               *
*                                                                           *
*****************************************************************************/

/****************************************************************************
* FILENAME
*   kpi.c
*
* VERSION
*   1.0
*
* DESCRIPTION
*   KPI sample application
*
* DATA STRUCTURES
*   None
*
* FUNCTIONS
*
* HISTORY
*
* REMARK
*   None
****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "wblib.h"

#include "w55fa95_kpi.h"
#include "w55fa95_vpost.h"

LCDFORMATEX lcdFormat;

__align(32) UINT8 Vpost_Frame[]=
{
	#include "..\..\VPOST\Example\ASIC\river_480x272_rgb565.dat"
};

int main(void)
{

	WB_UART_T uart;	
	UINT32 u32ExtFreq;
	unsigned int key;
/*
	sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
						240000000,		//UINT32 u32PllKHz,
						240000000);		//UINT32 u32SysKHz,									
	sysSetCPUClock(240000000);
	sysSetAPBClock(48000000);	
*/	
	u32ExtFreq = sysGetExternalClock();    	/* Hz unit */	
	uart.uiFreq = u32ExtFreq;
	uart.uiBaudrate = 115200;
	uart.uiDataBits = WB_DATA_BITS_8;
	uart.uiStopBits = WB_STOP_BITS_1;
	uart.uiParity = WB_PARITY_NONE;
	uart.uiRxTriggerLevel = LEVEL_1_BYTE;
	sysInitializeUART(&uart);	

	sysprintf("start kpi test...\n");	
	lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_RGB565;
	lcdFormat.nScreenWidth = 480;
	lcdFormat.nScreenHeight = 272;

	vpostLCMInit(&lcdFormat, (UINT32*)Vpost_Frame);

	outpw(REG_LCM_TCON1, (inpw(REG_LCM_TCON1) |0x3A0000));
	outpw(0xB100209C, 0x00320038);
	
	kpi_init();
	kpi_open(0); 
	
	while(1) {
#ifdef _KPI_NONBLOCK_
		key = kpi_read(KPI_NONBLOCK);
#else
		key = kpi_read(KPI_BLOCK);
#endif		
		sysprintf("key is %d\n", key);
		sysDelay(20);
	
	}
	kpi_close();
	sysprintf("quit kpi test...\n");
	return(0);
	
}
