/***************************************************************************
 *                                                                         									     *
 * Copyright (c) 2008 Nuvoton Technolog. All rights reserved.              					     *
 *                                                                         									     *
 ***************************************************************************/

#include <stdio.h>
#include "wblib.h"
#include "demo.h"
//#pragma import (__use_no_semihosting_swi)

extern int DemoAPI_AIC(void);
extern void DemoAPI_UART(void);
extern void DemoAPI_Timer0(void);
extern void DemoAPI_Timer1(void);
extern void DemoAPI_WDT(void);
extern void DemoAPI_Cache(BOOL bIsCacheOn);
extern void DemoAPI_CLK(void);
extern void DemoAPI_CLKRandom(void);
extern void DemoAPI_CLKRandom(void);

BOOL bIsFirstApll=TRUE; 
UINT32 u32SysDiv0, u32SysDiv1;
void DemoAPI_AdjustApllDivider(void)
{
	UINT32  i,j; 
	if(bIsFirstApll==TRUE)
	{
		UINT32 u32Reg;
		u32Reg = inp32(REG_CLKDIV0) & 0xF07;		
		u32SysDiv0 = u32Reg & 0x7;	 
		u32SysDiv1 = u32Reg >>8;
		bIsFirstApll = FALSE;
	}
	for(i=u32SysDiv1; i<16;i=i+1)
	{
		for(j=u32SysDiv0; j<8;j=j+1)
		{
			outp32(REG_CLKDIV0, (inp32(REG_CLKDIV0) & ~0xF07) | 
								((i<<8) | j));
			DBG_PRINTF("SYS divider1 %d,  divider0 %d\n", i, j);												
		}
	}	
}

int main()
{
	//unsigned int volatile i;
	WB_UART_T uart;
	UINT32 u32Item, u32ExtFreq;
	UINT32 u32PllOutHz;
	u32ExtFreq = sysGetExternalClock();
#if 1	
	/* Normal speed UART */
	sysUartPort(1);
	uart.uiFreq = u32ExtFreq;	
    uart.uiBaudrate = 115200;
#else
	/* High speed UART */
	sysUartPort(0);
	uart.uiFreq = u32ExtFreq;
    uart.uiBaudrate = 921600;
#endif	
    	
    	uart.uiDataBits = WB_DATA_BITS_8;
    	uart.uiStopBits = WB_STOP_BITS_1;
    	uart.uiParity = WB_PARITY_NONE;
    	uart.uiRxTriggerLevel = LEVEL_1_BYTE;
    	uart.uart_no = WB_UART_0;
    	sysInitializeUART(&uart);
    	sysSetLocalInterrupt(ENABLE_FIQ_IRQ);	
    	
    	u32PllOutHz = sysGetPLLOutputHz(eSYS_UPLL, u32ExtFreq);
	DBG_PRINTF("PLL out frequency %d Hz\n", u32PllOutHz);	
	do
	{    	
		DBG_PRINTF("================================================================================\n");
		DBG_PRINTF("                               System library demo code                         |\n");
		DBG_PRINTF(" [1] UART demo                                                                  |\n");
		DBG_PRINTF(" [2] Timer0 demo                                                                |\n");
		DBG_PRINTF(" [3] Timer1 demo                                                                |\n");
		DBG_PRINTF(" [4] Watch dog                                                                  |\n");		
		DBG_PRINTF(" [5] Cache demo disable                                                         |\n");
		DBG_PRINTF(" [6] Cache demo enable                                                          |\n");
		DBG_PRINTF(" [7] AIC demo                                                                   |\n");				
		DBG_PRINTF(" [8] Clock switch                                                               |\n");
		DBG_PRINTF(" [9] High speed UART                                                            |\n");
		DBG_PRINTF(" [A] System clock divider                                                       |\n");
		DBG_PRINTF(" [B] Power down then wake up by GPIO                                            |\n");
		DBG_PRINTF("================================================================================\n");
		
		//outp32(0xb0000084, 0x03);
		//outp32(0xb0000230, 0x85);
	
		u32Item = sysGetChar();
		switch(u32Item)
		{
			case '1': 	DemoAPI_UART();		break; 	//OK-sysprintf
	    		case '2': 	DemoAPI_Timer0();		break;
	    		case '3': 	DemoAPI_Timer1();		break;	
	    		case '4':	DemoAPI_WDT();		break;
	    		case '5':	DemoAPI_Cache(FALSE);	break;	
	    		case '6':	DemoAPI_Cache(TRUE);	break;	
	    		case '7': 	DemoAPI_AIC(); 		break;
	    		case '8':	DemoAPI_CLK();		break;	  	
	    		case '9':	
	    				sysprintf("Please modify UART code to use UART port 0 and set baudrate to 921600 bps\n");
	    				DemoAPI_HUART();		break;		    		
	    		case 'A':	DemoAPI_ChangeSystemClockDivider(); break;
	    		
	    		case 'B':	/* Remember that to meet tRP>15ns spec, the system clock need slower down than 66MHz */ 		
    					sysSetSystemClock(eSYS_UPLL,
    									144000000,
    									144000000);	    									
    					sysSetSystemDivider(144000000/4, 4); 				
    					DBG_PRINTF("Register GPAB Int = 0x%x\n", inp32(REG_IRQTGSRC0));
		    			DBG_PRINTF("Register GPCD Int= 0x%x\n", inp32(REG_IRQTGSRC1));
		    			DBG_PRINTF("Register GPEF Int = 0x%x\n", inp32(REG_IRQTGSRC2));
		    			DBG_PRINTF("Register MISSR = 0x%x\n", inp32(REG_MISSR));
    					Demo_PowerDownWakeUp();		break;	
	    		case 'Q':	break;
	    		case 'q':	break;
		}
	}while((u32Item!= 'q') || (u32Item!= 'Q'));	
    	return 0;
} /* end main */