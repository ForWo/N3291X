#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wblib.h"
#include "w55fa95_edma.h"
#include "w55fa95_vpost.h"

__align(32) UINT8 LoadAddr[]=
{
	#include "..\..\..\VPOST\Example\ASIC\sea_800x480_RGB565.dat"
};

LCDFORMATEX lcdFormat;
extern void TransferLengthTest(void);
extern void ColorSpaceTransformTest(void);
extern void SPIFlashTest(void);
extern void UARTTest(void);

int main()
{
	WB_UART_T uart;
	UINT32 u32ExtFreq, u32Item;
	
	sysSetSystemClock(eSYS_UPLL,
	                300000000,      // Specified the APLL/UPLL clock, unit Hz
	                300000000);     // Specified the system clock, unit Hz
	sysSetCPUClock (300000000);     // Unit Hz
	sysSetAPBClock ( 75000000);     // Unit Hz

	outp32(REG_APBCLK, inp32(REG_APBCLK) | ADC_CKE);    // enable ADC clock in order to set register REG_AUDADC_CTL that belong to ADC.
	outp32(REG_AUDADC_CTL, inp32(REG_AUDADC_CTL) | APB2AHB);
	outp32(REG_APBCLK, inp32(REG_APBCLK) & ~ADC_CKE);   // disable ADC clock to save power.
    
	u32ExtFreq = sysGetExternalClock();    	/* Hz unit */	
	uart.uiFreq = u32ExtFreq;
	uart.uiBaudrate = 115200;
	uart.uiDataBits = WB_DATA_BITS_8;
	uart.uiStopBits = WB_STOP_BITS_1;
	uart.uiParity = WB_PARITY_NONE;
	uart.uiRxTriggerLevel = LEVEL_1_BYTE;
	sysInitializeUART(&uart);	

	sysEnableCache(CACHE_WRITE_BACK);

	sysSetLocalInterrupt(ENABLE_IRQ);

	lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_RGB565;
	lcdFormat.nScreenWidth = 800;
	lcdFormat.nScreenHeight = 480;	
	vpostLCMInit(&lcdFormat, (UINT32*)LoadAddr);
	
	EDMA_Init();

	do
	{
		sysprintf("==================================================================\n");
		sysprintf("[1] Transfer Length and Direction Test \n");
		sysprintf("[2] Color Space Transform Test \n");			
		sysprintf("[3] PDMA+SPIFlash Test \n");	
		sysprintf("[4] PDMA+UART Test \n");	
		sysprintf("[5] PDMA+ADC Test \n");
		sysprintf("==================================================================\n");

		u32Item = sysGetChar();
		
		switch(u32Item) 
		{
			case '1': 
				TransferLengthTest();			
				break;
					
			case '2':
				ColorSpaceTransformTest();					
				break;		
				
			case '3':
				SPIFlashTest();					
				break;

			case '4':
				UARTTest();					
				break;
				
			case '5':
				sysprintf("Please refer to adc sample code \n");											
				break;
				
			case 'Q':
			case 'q': 
				u32Item = 'Q';
				sysprintf("quit edma test...\n");				
				break;	
				
			}
		
	}while(u32Item!='Q');
	
}








