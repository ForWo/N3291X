/****************************************************************************
*                                                                           *
* Copyright (c) 2009 Nuvoton Tech. Corp. All rights reserved.               *
*                                                                           *
*****************************************************************************/

/****************************************************************************
* FILENAME
*   adc.c
*
* VERSION
*   1.0
*
* DESCRIPTION
*   ADC sample application using ADC library
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
#include <stdlib.h>
#include <string.h>

#include "wblib.h"
#include "W55FA95_adc.h"
#include "W55FA95_SIC.h"
#include "w55fa95_vpost.h"
#include "nvtfat.h"
#include "DrvEDMA.h"

extern INT16 g_pi16SampleBuf[];

extern void Smpl_TscPanel_WT_Powerdown_Wakeup(void);
extern void AudioRecord(UINT32 u32SampleRate);

/* Every 10ms call back */
UINT32 u32Flag_10ms = 0;
/* Assume VREF always = 3.3V */
void Timer0_1_Callback(void) 
{/* LVD check */
	UINT16 u16Vol;
	float voltage;
	u32Flag_10ms = u32Flag_10ms+1;
	voltage = 3.3/1024.;  
	if(u32Flag_10ms>=3000)
	{		
		if(adc_normalread(2, &u16Vol)==Successful)
		{
			UINT32 u32Dec;
			voltage = voltage*u16Vol;
			sysprintf("Battery value = %x\n", u16Vol);
			sysprintf("Battery Voltage = ");			
			u32Dec = voltage;	
			sysprintf("%d.",u32Dec); 	
			voltage = voltage - u32Dec;
			while(voltage!=0.)
			{		
				voltage = voltage*10.;	
				u32Dec = voltage;	
				sysprintf("%d",u32Dec); 	
				voltage = voltage - u32Dec;
			}
			sysprintf("\n"); 
		}	
		u32Flag_10ms = 0;
	}	
}

UINT32 u32Flag_10s = 0;
void Timer0_2_Callback(void)
{
	u32Flag_10s = u32Flag_10s+1;
}
UINT32 u32Flag_20ms = 0;
void Timer0_3_Callback(void)
{
	u32Flag_20ms = 1;
}

LCDFORMATEX lcdInfo;
#define FB_ADDR	0x500000

#define ADC_NONBLOCK 	0
#define ADC_BLOCK		1

extern void TouchPanel_Powerdown_Wakeup(void);

int main(void)
{
	UINT32 u32ExtFreq, u32Item, u32PllFreq, u32SampleRate;
	INT32 i32TimerChanel1, i32TimerChanel2;
	UINT16 x, y;
	WB_UART_T uart;
	outp32(REG_POR_LVRD, 0x41);
	
	u32ExtFreq = sysGetExternalClock();
	sysUartPort(1);

	uart.uiFreq = u32ExtFreq;	//use APB clock
    	uart.uiBaudrate = 115200;
	uart.uiDataBits = WB_DATA_BITS_8;
	uart.uiStopBits = WB_STOP_BITS_1;
	uart.uiParity = WB_PARITY_NONE;
	uart.uiRxTriggerLevel = LEVEL_1_BYTE;
	sysInitializeUART(&uart);
		
	u32PllFreq = sysGetPLLOutputHz(eSYS_UPLL, u32ExtFreq);
	
	sysprintf("External clock = %dHz\n", u32ExtFreq);    	
	sysprintf("PLL clock = %d Hz\n", u32PllFreq);	
	
#if 0
	lcdInfo.ucVASrcFormat = DRVVPOST_FRAME_RGB565;	
	lcdInfo.nScreenWidth = 480;	
	lcdInfo.nScreenHeight = 272;
	vpostLCMInit(&lcdInfo, (UINT32*)FB_ADDR);
#endif	
	
	fsInitFileSystem();	
	sicIoctl(SIC_SET_CLOCK, u32PllFreq/1000, 0, 0);	
	sicOpen();	
	if (sicSdOpen0()<=0)
	{
		sysprintf("Error in initializing SD card !! \n");						
		//while(1);
	}			
	fsAssignDriveNumber('C', DISK_TYPE_SD_MMC, 0, 1);		
	
	/* For 30 sec: Low battery checking from channel 2*/
	sysSetTimerReferenceClock (TIMER0, u32ExtFreq);
	sysStartTimer(TIMER0, 100, PERIODIC_MODE);	
	sysEnableCache(CACHE_WRITE_BACK);	
	sysSetLocalInterrupt(ENABLE_IRQ);	
	do
	{
		sysprintf("==================================================================\n");
		sysprintf("[1] Power Down Wake Up by TSC  (Test 20 sec if nonblocking) \n");
		sysprintf("[2] Position and Low battery \n");	
		sysprintf("[3] Audio Recording to SD card \n");	
		sysprintf("==================================================================\n");

		u32Item = sysGetChar();
		
		switch(u32Item) 
		{
			case '1': 			
				adc_open(NULL, 320, 240);
				sysprintf("!!!!!!Remember to disable all of IPs that request memory. For example, VPOST....!!!!!!\n"); 			
				TouchPanel_Powerdown_Wakeup();	
				break;	
			case '2':			
				adc_open(NULL, 320, 240);	
				i32TimerChanel1 = sysSetTimerEvent(TIMER0, 1, (PVOID)Timer0_1_Callback);	/* For  battery detection */
				if(i32TimerChanel1==0)
				{
					sysprintf("Out of timer channel\n");
					exit(-1);
				}
				
				u32Flag_10s = 0;		
				i32TimerChanel2 = sysSetTimerEvent(TIMER0, 1000, (PVOID)Timer0_2_Callback);	/* For  escape touch panel test 10 sec*/
				
				u32Flag_20ms = 0;		
				i32TimerChanel2 = sysSetTimerEvent(TIMER0, 1, (PVOID)Timer0_3_Callback);		/* For touch*/	
								
				if(i32TimerChanel2==0)
				{
					sysprintf("Out of timer channel\n");
					exit(-1);
				}											
				while(1) 
				{

				/* Touch pannel */
				if(u32Flag_20ms==1)
				{	
					u32Flag_20ms = 0;
	#ifdef _ADC_NONBLOCK_
					if(adc_read(ADC_NONBLOCK, &x, &y))
	#else
					if(adc_read(ADC_BLOCK, &x, &y))
	#endif
						sysprintf("x = %d, y = %d\n", x, y);
					else
						sysprintf("pen up");
					if(u32Flag_10s==100)
						break;					
				}	
				}
				sysClearTimerEvent(TIMER0, i32TimerChanel1); 
				sysClearTimerEvent(TIMER0, i32TimerChanel2); 	
			break;
			case '3':
				sysprintf("!!!! Please specified the APLL to 432MHz for below sampling rate\n");
				sysprintf("[0] Sample Rate 11025 \n");
				sysprintf("[1] Sample Rate 16K \n");
				sysprintf("[2] Sample Rate 12K \n");	
				sysprintf("[3] Sample Rate 8K \n");	
				
				u32Item = sysGetChar();
				/* Use APLL for audio recording from ADC. The sample rate support 8K and 12K samples/s */
				if(u32Item=='0')
				{
					sysCheckPllConstraint(FALSE);						
				//	sysSetPllClock(eSYS_APLL, 	//Another PLL,	
				//				432000000);	//UINT32 u32PllKHz, 	
					sysCheckPllConstraint(TRUE);
					AudioRecord(11025);	
				}	
				else if(u32Item=='1')
				{
					sysCheckPllConstraint(FALSE);						
				//	sysSetPllClock(eSYS_APLL, 	//Another PLL,	
				//				432000000);		//UINT32 u32PllKHz, 	
					sysCheckPllConstraint(TRUE);				
					AudioRecord(16000);		
				}else if(u32Item=='2')
				{
					sysCheckPllConstraint(FALSE);	
				//	sysSetPllClock(eSYS_APLL, 	//Another PLL,	
				//				432000000);		//UINT32 u32PllKHz, 										
					sysCheckPllConstraint(TRUE);						
					AudioRecord(12000);	
				}else if(u32Item=='3')
				{
					sysCheckPllConstraint(FALSE);	
				//	sysSetPllClock(eSYS_APLL, 	//Another PLL,	
				//				432000000);		//UINT32 u32PllKHz, 										
					sysCheckPllConstraint(TRUE);				
					u32SampleRate = 8000;
					AudioRecord(8000);	
				}				
											
				break;		
			case	'Q':
			case 'q': 
				u32Item = 'Q';
				sysprintf("quit adc test...\n");
				adc_close();
				break;	
			}
	}while(u32Item!='Q');		
}