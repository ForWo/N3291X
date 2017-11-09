/***************************************************************************
 *                                                                         									     *
 * Copyright (c) 2008 Nuvoton Technolog. All rights reserved.              					     *
 *                                                                         									     *
 ***************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "wblib.h"
#include "demo.h"



#define MEM_SIZE			0x100000
#define NON_CACHE_BIT		0x80000000
void DemoClockMemCpy(UINT32 u32Count)
{
	UINT32 tmp;
	char *pUINT8Buf0, *pUINT8Buf1;
	volatile unsigned int  u32Btime, u32Etime;
	
	pUINT8Buf0 = (char*)0x800000;//(char*)malloc(MEM_SIZE);
	pUINT8Buf1 = (char*)0x1000000;//(char*)malloc(MEM_SIZE);
	
	//pUINT8Buf0 = (UINT8*)MEM_BUF1;
	for(tmp=0;tmp<MEM_SIZE; tmp = tmp+1)
	{
		*pUINT8Buf0++ = u32Count+tmp*123;
	}	
	sysSetTimerReferenceClock(TIMER0, 12000000); 		//External Crystal
	sysStartTimer(TIMER0, 100, PERIODIC_MODE);			/* 100 ticks/per sec ==> 1tick/10ms */
	
	sysSetLocalInterrupt(ENABLE_IRQ);	
	pUINT8Buf0 = (char*)0x800000;
	DBG_PRINTF("Src Buf = 0x%x.  Dst Buf= 0x%x\n",(UINT32)pUINT8Buf0 , (UINT32)pUINT8Buf1);
	u32Btime = sysGetTicks(TIMER0);	
	memcpy((UINT8*) ((UINT32)pUINT8Buf0 | NON_CACHE_BIT), 
			(UINT8*)((UINT32)pUINT8Buf1 | NON_CACHE_BIT), 
			MEM_SIZE);
	u32Etime = sysGetTicks(TIMER0);
	DBG_PRINTF("Copy 0x%x bytes memory takes %d ms\n",MEM_SIZE, (u32Etime-u32Btime)*10);
				
}			
void	DemoClockMemCmp(UINT32 u32Count)		
{
	UINT32 tmp;
	char *pUINT8Buf0, *pUINT8Buf1;
	volatile unsigned int  u32Btime, u32Etime;
	pUINT8Buf0 = (char*)0x800000;//(char*)malloc(MEM_SIZE);
	pUINT8Buf1 = (char*)0x1000000;//(char*)malloc(MEM_SIZE);						
	
	/* Compare time */
	u32Btime = sysGetTicks(TIMER0);
	if(memcmp((UINT8*) ((UINT32)pUINT8Buf0 | NON_CACHE_BIT), 
				(UINT8*)((UINT32)pUINT8Buf1 | NON_CACHE_BIT), 
				MEM_SIZE)!=0)
	{
		DBG_PRINTF("Compare error\n");
		while(1);
	}
	u32Etime = sysGetTicks(TIMER0);
	DBG_PRINTF("Compare 0x%x bytes memory takes %d ms\n", MEM_SIZE, (u32Etime-u32Btime)*10);

}

void DemoAPI_ChangeSystemClockDivider(void)
{
	UINT32 u32Count=0;
	UINT32 u32Integ, u32Fract;
	sysSetSystemClock(eSYS_UPLL, 		//E_SYS_SRC_CLK eSrcClk,	
						240000000,		//UINT32 u32PllHz, 	
						240000000);		//UINT32 u32SysHz
	sysDelay(10);		//Delay .1 sec
	sysEnableCache(CACHE_WRITE_BACK);
	sysprintf("The function-sysSetSystemDivider() will change system clock in SRAM\n");						
	for(u32Integ=0; u32Integ<=7; u32Integ=u32Integ+1)			
	{	  
		
		sysSetSystemDivider(240000000/(u32Integ+1), u32Integ);		
		sysprintf("SYS DIV 0x%x,\n", u32Integ);			
		DemoClockMemCmp(u32Count);
		sysprintf("System Clock = %d\n", sysGetSystemClock());
	}			  					  
}

/* =============================================================
	This enumation is only for clock test mode. 
	Specified clock will be outputed through pin SEN_CLK 
	The register is located CLK_BA[0x30]
==============================================================*/


void DemoAPI_CLK(void)
{
	while(1)
	{//Adjust PLL   
		UINT32 u32Clk;					    	
	   
	    	DBG_PRINTF("***UPLL = 240MHz								  	\n");			
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							240000000,		//UINT32 u32PllHz,
							240000000);		//UINT32 u32SysHz,
				
		DBG_PRINTF("***UPLL = 300MHz								  	\n");			
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							300000000,		//UINT32 u32PllHz,
							300000000);		//UINT32 u32SysHz,					
							
	    	u32Clk = 300000000;
	    	while(1)
	    	{
	    		UINT32 u32SysClock;
	    		DBG_PRINTF("***UPLL = %d MHz\n", u32Clk/1000000);
	    		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							u32Clk,		//UINT32 u32PllHz,
							u32Clk/2);		//UINT32 u32SysHz,				
			u32SysClock = sysGetSystemClock();
			sysprintf("HCLK clock = %d Hz\n", u32SysClock/2);
			u32Clk = u32Clk - 16000000;	
			if(u32Clk<=124000000)
				break;					
		}
		u32Clk = 124000000;
	    	while(1)
	    	{
	    		DBG_PRINTF("***UPLL = %d MHz\n", u32Clk/1000000);
	    		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							u32Clk,		//UINT32 u32PllHz,
							u32Clk/5);		//UINT32 u32SysHz,	
			u32Clk = u32Clk + 16000000;	
			if(u32Clk>300000000)
				break;					
		}
							 						
		DBG_PRINTF("***UPLL = 300MHz								  	\n");			
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							300000000,		//UINT32 u32PllHz,
							300000000);		//UINT32 u32SysHz,		
		
		DBG_PRINTF("***UPLL = 264MHz								  	\n");			
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							264000000,		//UINT32 u32PllHz,
							264000000);		//UINT32 u32SysHz,												
	 
	 	DBG_PRINTF("***UPLL = 300MHz								  	\n");			
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							300000000,		//UINT32 u32PllHz,
							300000000);		//UINT32 u32SysHz,		
				
		DBG_PRINTF("***UPLL = 264MHz								  	\n");			
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							264000000,		//UINT32 u32PllHz,
							264000000);		//UINT32 u32SysHz,								
					

		DBG_PRINTF("***UPLL = 216MHz								  	\n");			
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							216000000,		//UINT32 u32PllHz,
							216000000);		//UINT32 u32SysHz,
													
	  	DBG_PRINTF("***UPLL = 264MHz								  	\n");			
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							264000000,		//UINT32 u32PllHz,
							264000000);		//UINT32 u32SysHz,

		DBG_PRINTF("***UPLL = 240MHz								  	\n");			
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							240000000,		//UINT32 u32PllHz,
							240000000);		//UINT32 u32SysHz,
		
		DBG_PRINTF("***UPLL = 300MHz								  	\n");			
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							300000000,		//UINT32 u32PllHz,
							300000000);		//UINT32 u32SysHz,
							
		DBG_PRINTF("***UPLL = 264MHz								  	\n");			
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							264000000,		//UINT32 u32PllHz,
							264000000);		//UINT32 u32SysHz,			
							
		DBG_PRINTF("***UPLL = 216MHz								  	\n");			
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							216000000,		//UINT32 u32PllHz,
							216000000);		//UINT32 u32SysHz,												
		
		DBG_PRINTF("***UPLL = 264MHz								  	\n");			
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							264000000,		//UINT32 u32PllHz,
							264000000);		//UINT32 u32SysHz,					

		DBG_PRINTF("***UPLL = 216MHz								  	\n");				
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							216000000,		//UINT32 u32PllHz,
							216000000);		//UINT32 u32SysHz,	
							
		DBG_PRINTF("***UPLL = 300MHz								  	\n");				
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							300000000,		//UINT32 u32PllHz,
							300000000);		//UINT32 u32SysHz,	
		
		DBG_PRINTF("***UPLL = 216MHz								  	\n");				
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							216000000,		//UINT32 u32PllHz,
							216000000);		//UINT32 u32SysHz,						
		
		DBG_PRINTF("***UPLL = 264MHz								  	\n");			
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							264000000,		//UINT32 u32PllHz,
							264000000);		//UINT32 u32SysHz,
		
		DBG_PRINTF("***UPLL = 192MHz								  	\n");				
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							192000000,		//UINT32 u32PllHz,
							19200000);		//UINT32 u32SysHz,																
		DBG_PRINTF("Done								  	\n");	
	}									    		

}
void Delay(UINT32 u32DelaySec)
{
	volatile unsigned int btime, etime, tmp, tsec;
	btime = sysGetTicks(TIMER0);
	tsec = 0;
	tmp = btime;
	while (1)
	{			
		etime = sysGetTicks(TIMER0);
		if ((etime - btime) >= (100*u32DelaySec))
		{	
			break;		
		}
	}
}
void EMU_DemoAPI_SystemClockRatio(void)
{	
	UINT32 u32ExtFreq;
	UINT32 u32PllOutHz;
	UINT32 i, j=0;
	sysSetTimerReferenceClock(TIMER0, 27000000); //External Crystal	
	sysStartTimer(TIMER0, 100, PERIODIC_MODE);			/* 100 ticks/per sec ==> 1tick/10ms */	
	sysSetLocalInterrupt(ENABLE_IRQ);	
	u32ExtFreq = sysGetExternalClock();
	u32PllOutHz = sysGetPLLOutputHz(eSYS_UPLL, u32ExtFreq);
	while(1)
    	{//Adjust system divider    		
	    	for(i=0;i<=5;i=i+1)
	    	{
	    		UINT32 u32Hclk = u32PllOutHz/(i+1)/2;
	    		sysprintf("HCLK = %d\n", u32Hclk);
	    		sysSetSystemDivider(u32Hclk, i);		
			sysprintf("SYS DIV %d done\n", i);
			sysDelay(1);		    					
		}
		j = j+1;
		if(j>5){
			sysSetSystemDivider(u32PllOutHz, 0);
			sysprintf("Restore SYS DIV = 0\n");
			break;
		}	
	}

}