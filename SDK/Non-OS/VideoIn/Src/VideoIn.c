/****************************************************************
 *                                                              *
 * Copyright (c) Nuvoton Technology Corp. All rights reserved.  *
 *                                                              *
 ****************************************************************/

#include <stdio.h>
#include <string.h>
#include "w55fa95_reg.h"
#include "wblib.h"
#include "VideoIn.h"
#define REAL_CHIP
PFN_VIDEOIN_CALLBACK (pfnVideoInIntHandlerTable)[2][4]={0};

/*---------------------------------------------------------------------------------------------------------*/
/* Function: VideoInIntHandler                                                                             */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      None                                                                                               */
/*                                                                                                         */
/* Returns:                                                                                                */
/*      None                                                                                               */
/*                                                                                                         */
/* Description:                                                                                            */
/*      Driver internal use API to process the interrupt of Video-IN									   */
/*      As interrupt occurrence, the register call back function will be executed                          */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
static UINT32 u32EscapeFrame = 0;
static UINT32 g_u32DeviceType = 0;
static UINT32 g_u32PortOffset = 0x0;
/*---------------------------------------------------------------------------------------------------------*/
/* Function: videoIn_Port                                                                             */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      None                                                                                               */
/*                                                                                                         */
/* Returns:                                                                                                */
/*      None                                                                                               */
/*                                                                                                         */
/* Description:                                                                                            */
/*      Using the function to initialize the both videoIn ports 										   */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
void videoIn_Port(UINT32 u32Port)
{
	if(u32Port==0)
		g_u32PortOffset = 0;	
	else
		g_u32PortOffset = 0x800;
}

static void 
videoInIntHandler(
	void
	)
{
	UINT32 u32CapInt;
	UINT32 uBuf=0;
	BOOL bIsFiled;
	UINT32 u32Offset;
	
	if((inp32(REG_AHBCLK)&VIN0_CKE)==VIN0_CKE)
	{//Check VideoIn0 Enable 
		bIsFiled = (inp32(REG_VPEPAR) & FLDID) >>20;			
		u32CapInt = inp32(REG_VPEINT);
		if( (u32CapInt & (VINTEN | VINT)) == (VINTEN | VINT))
		{
			if(pfnVideoInIntHandlerTable[0][0]!=0)	
				pfnVideoInIntHandlerTable[0][0](uBuf, uBuf, bIsFiled, u32EscapeFrame);
			outp32(REG_VPEINT, (u32CapInt & ~(MDINT | ADDRMINT | MEINT)));		/* Clear Frame end interrupt */
			u32EscapeFrame=u32EscapeFrame+1;
		}	
		else if((u32CapInt & (ADDRMEN|ADDRMINT)) == (ADDRMEN|ADDRMINT))
		{
			if(pfnVideoInIntHandlerTable[0][1]!=0)	
				pfnVideoInIntHandlerTable[0][1](uBuf, uBuf, bIsFiled, u32EscapeFrame);
			outp32(REG_VPEINT, (u32CapInt & ~(MDINT | VINT | MEINT)));			/* Clear Address match interrupt */
		}	
		else if ((u32CapInt & (MEINTEN|MEINT)) == (MEINTEN|MEINT))
		{
			if(pfnVideoInIntHandlerTable[0][2]!=0)	
				pfnVideoInIntHandlerTable[0][2](uBuf, uBuf, bIsFiled, u32EscapeFrame);	
			outp32(REG_VPEINT, (u32CapInt & ~(MDINT | VINT|ADDRMINT)));			/* Clear Memory error interrupt */	
		}	
		else if ((u32CapInt & (MDINTEN|MDINT)) == (MDINTEN|MDINT))
		{
			if(pfnVideoInIntHandlerTable[0][3]!=0)	
				pfnVideoInIntHandlerTable[0][3](uBuf, uBuf, bIsFiled, u32EscapeFrame);	
			outp32(REG_VPEINT, (u32CapInt & ~( VINT | MEINT | ADDRMINT)));			/* Clear Memory error interrupt */	
		}
	}	
	if((inp32(REG_AHBCLK2)&VIN1_CKE)==VIN1_CKE)
	{//Check VideoIn0 Enable 
		u32Offset = 0x800;
		bIsFiled = (inp32(REG_VPEPAR+u32Offset) & FLDID) >>20;			
		u32CapInt = inp32(REG_VPEINT+u32Offset);
		if( (u32CapInt & (VINTEN | VINT)) == (VINTEN | VINT))
		{
			if(pfnVideoInIntHandlerTable[1][0]!=0)	
				pfnVideoInIntHandlerTable[1][0](uBuf, uBuf, bIsFiled, u32EscapeFrame);
			outp32(REG_VPEINT+u32Offset, (u32CapInt & ~(MDINT | ADDRMINT | MEINT)));		/* Clear Frame end interrupt */
			u32EscapeFrame=u32EscapeFrame+1;
		}	
		else if((u32CapInt & (ADDRMEN|ADDRMINT)) == (ADDRMEN|ADDRMINT))
		{
			if(pfnVideoInIntHandlerTable[1][1]!=0)	
				pfnVideoInIntHandlerTable[1][1](uBuf, uBuf, bIsFiled, u32EscapeFrame);
			outp32(REG_VPEINT+u32Offset, (u32CapInt & ~(MDINT | VINT | MEINT)));			/* Clear Address match interrupt */
		}	
		else if ((u32CapInt & (MEINTEN|MEINT)) == (MEINTEN|MEINT))
		{
			if(pfnVideoInIntHandlerTable[1][2]!=0)	
				pfnVideoInIntHandlerTable[1][2](uBuf, uBuf, bIsFiled, u32EscapeFrame);	
			outp32(REG_VPEINT+u32Offset, (u32CapInt & ~(MDINT | VINT|ADDRMINT)));			/* Clear Memory error interrupt */	
		}	
		else if ((u32CapInt & (MDINTEN|MDINT)) == (MDINTEN|MDINT))
		{
			if(pfnVideoInIntHandlerTable[1][3]!=0)	
				pfnVideoInIntHandlerTable[1][3](uBuf, uBuf, bIsFiled, u32EscapeFrame);	
			outp32(REG_VPEINT+u32Offset, (u32CapInt & ~( VINT | MEINT | ADDRMINT)));			/* Clear Memory error interrupt */	
		}
	}
	//outp32(REG_VPECTL,  inp32(REG_VPECTL) | UPDATE);
} 
/*---------------------------------------------------------------------------------------------------------*/
/* Function: DrvVideoIn_SetInitFrame &  DrvVideoIn_GetSkipFrame                                            */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      None                                                                                               */
/*                                                                                                         */
/* Returns:                                                                                                */
/*      None                                                                                               */
/*                                                                                                         */
/* Description:                                                                                            */
/*     	If enable interrupt, there is internal counter that records how many frames have pass. 			   */
/*      Set the internal counters to zero. The internal counter may be not a constant                      */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
void 
videoIn_SetInitFrame(
	void
	)
{
	u32EscapeFrame = 0;
}

UINT32 
videoIn_GetSkipFrame(
	void
	)
{
	return u32EscapeFrame;
}



//#define OPT_FPGA	
/*
UINT32
DrvVideoIn_GetVersion(
	void
	)
{
	//return (DRVVIDEOIN_MAJOR_NUM << 16) | (DRVVIDEOIN_MINOR_NUM << 8) | DRVVIDEOIN_BUILD_NUM;
	return DRVVIDEOIN_VERSION_NUM;
}
*/

void videoIn_Init(
	BOOL bIsEnableSnrClock,
	E_VIDEOIN_SNR_SRC eSnrSrc,	
	UINT32 u32SensorFreq,						//KHz unit
	E_VIDEOIN_DEV_TYPE eDevType
	)
{
	UINT32 u32PllClock, u32SenDiv, u32ExtFreq;
	UINT32 u32Div0, u32Div1; 
	UINT32 u32SenSrc; 
	
	g_u32DeviceType = eDevType;
	u32ExtFreq = sysGetExternalClock();
	u32SensorFreq = u32SensorFreq*1000;
	
	u32PllClock = sysGetPLLOutputHz(eSYS_UPLL, u32ExtFreq);
	if(g_u32PortOffset==0)
		u32SenSrc = 0x3<<21;	//UPLL for sensor 1
	else
		u32SenSrc = 0x3<<16;	//UPLL for sensor 2
	if(u32PllClock==0)
	{
		u32PllClock = sysGetPLLOutputHz(eSYS_APLL, u32ExtFreq);
		if(g_u32PortOffset==0)
			u32SenSrc = 0x2<<21;	//APLL for sensor 1
		else
			u32SenSrc = 0x2<<16;	//APLL for sensor 2	
	}
	u32SenDiv = u32PllClock/(u32SensorFreq); 
	if(u32PllClock%u32SensorFreq!=0)
		u32SenDiv = u32SenDiv+1;
	for(u32Div0=1; u32Div0<=8; u32Div0 = u32Div0+1)	
	{//u32Div0 should be start from 1
		for(u32Div1=1; u32Div1<=16; u32Div1 = u32Div1+1)	
		{
			if(u32SenDiv==u32Div0*u32Div1)
				break;
		}
		if(u32Div1>16)
			continue;
		if(u32SenDiv==u32Div0*u32Div1)
				break;
	}	
	u32Div0 = u32Div0-1;
	u32Div1 = u32Div1-1;
	

	if(g_u32PortOffset==0)
	{
		volatile UINT32 u32Divider;
		if(bIsEnableSnrClock)	
			outp32(REG_AHBCLK, inp32(REG_AHBCLK) | SEN_CKE );						/* Enable Sensor clock */
		else
			outp32(REG_AHBCLK, inp32(REG_AHBCLK) & ~SEN_CKE );					/* Disable Sensor clock */	
		u32Divider  = u32SenSrc | ((u32Div0<<18) | (u32Div1<<23)) ;	
		sysprintf("Sensor port 1 Div0 = %d, Div1 = %d\n", u32Div0, u32Div1);
		outp32(REG_CLKDIV0, (inp32(REG_CLKDIV0) & ~(SENSOR0_N1 | SENSOR0_S | SENSOR0_N0)) | 
				 u32Divider );	
	}	
	else 
	{
		volatile UINT32 u32Divider;
		if(bIsEnableSnrClock)	
			outp32(REG_AHBCLK2, inp32(REG_AHBCLK2) | SEN1_CKE );						/* Enable Sensor clock */
		else
			outp32(REG_AHBCLK2, inp32(REG_AHBCLK2) & ~SEN1_CKE );						/* Disable Sensor clock */	
		u32Divider = u32SenSrc | ((u32Div0<<13) | (u32Div1<<18));
		sysprintf("Sensor port 2 Div0 = %d, Div1 = %d\n", u32Div0, u32Div1);
		outp32(REG_CLKDIV5, (inp32(REG_CLKDIV5) & ~(SENSOR1_N1 | SENSOR1_S | SENSOR1_N0)) | 
				   u32Divider);		
	}	
	if(eDevType == eVIDEOIN_SNR_CCIR656)
	{//VSYNC and HSYN by data
	#if 0
		outp32(REG_GPBFUN, inp32(REG_GPBFUN) | ( MF_GPB0 | MF_GPB1 | 						//SCLKO, PCLK
										MF_GPB5 | MF_GPB6 | MF_GPB7 | MF_GPB8 | 	//VDATA[7:0]
										MF_GPB9 | MF_GPB10 | MF_GPB11 | MF_GPB12) );	
	#else
		outp32(REG_GPBFUN, inp32(REG_GPBFUN) | 0x03FFFC0F );									
	#endif																		   								   
	}
	else if(eDevType == eVIDEOIN_SNR_CCIR601)
	{//VSYNC and HSYN by pins
		outp32(REG_GPBFUN, inp32(REG_GPBFUN) | ( MF_GPB0 | MF_GPB1 | MF_GPB2 | MF_GPB3 |	//SCLKO, PCLK, HSYNC and VSYNC
										MF_GPB5 | MF_GPB6 | MF_GPB7 | MF_GPB8 | 	//VDATA[7:0]
										MF_GPB9 | MF_GPB10 | MF_GPB11 | MF_GPB12) );										
	}
	else if(eDevType == eVIDEOIN_TVD_CCIR601)
	{
		outp32(REG_GPBFUN, inp32(REG_GPBFUN) | ( MF_GPB0 | MF_GPB1 | MF_GPB2 | MF_GPB3 | MF_GPB4 |	//SCLKO, PCLK, HSYNC, VSYNC and SFILED
										MF_GPB5 | MF_GPB6 | MF_GPB7 | MF_GPB8 | 	//VDATA[7:0]
										MF_GPB9 | MF_GPB10 | MF_GPB11 | MF_GPB12) );								  									
	}
	else if(eDevType == eVIDEOIN_TVD_CCIR656)
	{
		outp32(REG_GPBFUN, inp32(REG_GPBFUN) | (  MF_GPB1 |					//PCLK
										MF_GPB5 | MF_GPB6 | MF_GPB7 | MF_GPB8 | 	//VDATA[7:0]
										MF_GPB9 | MF_GPB10 | MF_GPB11 | MF_GPB12) );												
	}
	else if(eDevType == eVIDEOIN_2ND_SNR_CCIR601)
	{
#if 0	//Old FPGA
		outp32(REG_GPEFUN, (inp32(REG_GPEFUN) & ~(MF_GPE1 | MF_GPE0)) 
										  | 0x5);	//Hsync & VSync
		outp32(REG_GPCFUN, (inp32(REG_GPCFUN) & ~(MF_GPC15 | MF_GPC14 | MF_GPC13 | MF_GPC12 |
										  MF_GPC11 | MF_GPC10 | MF_GPC9 | MF_GPC8)) 
										  | (0x5555<<16));	//Data
		outp32(REG_GPAFUN, (inp32(REG_GPAFUN) & ~(MF_GPA2 | MF_GPA1 | MF_GPA0)) | 
											0x35);										    			
#else				
		/* Sensor Data */								  
		outp32(REG_GPCFUN, (inp32(REG_GPCFUN) & ~(MF_GPC15 | MF_GPC14 | MF_GPC13 | MF_GPC12 |
										  MF_GPC11 | MF_GPC10 | MF_GPC9 | MF_GPC8)) 
										  | (0x5555<<16));											  
		/* Hsync & VSync */
		outp32(REG_GPEFUN, (inp32(REG_GPEFUN) & ~(MF_GPE1 | MF_GPE0)) 
										  | 0x5);									  
		/* SCLKO and SPCLK	*/							  
		outp32(REG_GPAFUN, (inp32(REG_GPAFUN) & ~( MF_GPA1 | MF_GPA0)) | 
											0x5);						
		/* SFIELD */
		outp32(REG_GPEFUN, (inp32(REG_GPEFUN) & ~MF_GPE13) | 
											(0x1<<16));	 													  
#endif 	
	}
	else if(eDevType == eVIDEOIN_2ND_SNR_CCIR601_2)
	{				
		/* Sensor Data */								  
		outp32(REG_GPCFUN, (inp32(REG_GPCFUN) & ~(MF_GPC15 | MF_GPC14 | MF_GPC13 | MF_GPC12 |
										  MF_GPC11 | MF_GPC10 | MF_GPC9 | MF_GPC8)) 
										  | (0x5555<<16));											  										  					 
		/* Dummy: Set GPIOD[4:1] as GPIO */								  					
		outp32(REG_GPDFUN, inp32(REG_GPDFUN) & ~(MF_GPD4 | MF_GPD3 | MF_GPD2 | MF_GPD1));											  
		/* Hsync & VSync */
		outp32(REG_GPFFUN, (inp32(REG_GPFFUN) & ~(MF_GPF11 | MF_GPF10)) 
										  | (0xA<<20) );									  									  										  
		/* SCLKO and SPCLK	*/							  
		outp32(REG_GPFFUN, (inp32(REG_GPFFUN) & ~(MF_GPF9 | MF_GPF8)) 
										  | (0xA<<16) );				
		/* SFIELD */
		outp32(REG_GPEFUN, (inp32(REG_GPEFUN) & ~MF_GPE13) | 
											(0x1<<16));										
	}
									
}
/*---------------------------------------------------------------------------------------------------------*/
/* Function: videoIn_Open 									                                           */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      u32EngFreqKHz : Engine working Freq                                                                */
/*      u32SensorFreq : Sensor working Freq                                                                */
/* Returns:                                                                                                */
/*      None                                                                                               */
/*                                                                                                         */
/* Description:                                                                                            */
/*     	Initialize the VIdeo-In engine. Register a call back for driver internal using                     */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
ERRCODE 
videoIn_Open(
	UINT32 u32EngFreqKHz, 									/* VideoIn eng working frequency */  
	UINT32 u32SensorFreq									/* Specify the sensor clock */ 
)
{


	UINT32 u32PllClock, u32ExtFreq;	
	UINT32 u32SenDiv; 
	UINT32 u32Div0, u32Div1; 
	UINT32 u32SenSrc; 
	u32EngFreqKHz = u32EngFreqKHz*1000;
	u32SensorFreq = u32SensorFreq*1000;
	u32ExtFreq = sysGetExternalClock();		
	if(g_u32PortOffset==0)
	{//VIN0
		outp32(REG_AHBCLK, inp32(REG_AHBCLK) |( HCLK4_CKE | VIN0_CKE));		/* Enable Cap clock */
		outp32(REG_AHBIPRST, inp32(REG_AHBIPRST) | VIN0RST);				/* Global reset Capture */
		outp32(REG_AHBIPRST, inp32(REG_AHBIPRST) & (~VIN0RST));								
		u32PllClock = sysGetPLLOutputHz(eSYS_UPLL, u32ExtFreq);
	}
	else
	{//VIN1
		outp32(REG_AHBCLK, inp32(REG_AHBCLK) | HCLK4_CKE);	
		outp32(REG_AHBCLK2, inp32(REG_AHBCLK2) | VIN1_CKE);				/* Enable Cap clock */
		outp32(REG_AHBIPRST, inp32(REG_AHBIPRST) | VIN1RST);				/* Global reset Capture */
		outp32(REG_AHBIPRST, inp32(REG_AHBIPRST) & (~VIN1RST));								
		u32PllClock = sysGetPLLOutputHz(eSYS_UPLL, u32ExtFreq);
	}	


	u32PllClock = sysGetPLLOutputHz(eSYS_UPLL, u32ExtFreq);
	if(g_u32PortOffset==0)
		u32SenSrc = 0x3<<21;	//UPLL for sensor 1
	else
		u32SenSrc = 0x3<<16;	//UPLL for sensor 2
	if(u32PllClock==0)
	{
		u32PllClock = sysGetPLLOutputHz(eSYS_APLL, u32ExtFreq);
		if(g_u32PortOffset==0)
			u32SenSrc = 0x2<<21;	//APLL for sensor 1
		else
			u32SenSrc = 0x2<<16;	//APLL for sensor 2	
	}
	u32SenDiv = u32PllClock/(u32SensorFreq); 
	if(u32PllClock%u32SensorFreq!=0)
		u32SenDiv = u32SenDiv+1;
		
	if( (u32SenDiv>16) && ((u32SenDiv%2)!=0))	/* To avoid 324MHz, and sensor clock = 20MHz, the sensor clock divider is 0x11 */ 
		u32SenDiv = u32SenDiv+1;
		
	for(u32Div0=1; u32Div0<=8; u32Div0 = u32Div0+1)	
	{//u32Div0 should be start from 1
		for(u32Div1=1; u32Div1<=16; u32Div1 = u32Div1+1)	
		{
			if(u32SenDiv==u32Div0*u32Div1)
				break;
		}
		if(u32Div1>16)						/* To u32SenDiv=17. And the  u32Div1 just equal to 17. But SEN_N1 is max only 16 */ 
			continue;
		if(u32SenDiv==u32Div0*u32Div1)
				break;
	}	
	
	u32Div0 = u32Div0-1;
	u32Div1 = u32Div1-1;

	if(g_u32PortOffset==0)
	{
		volatile UINT32 u32Divider;
		
		u32Divider = (u32PllClock/2/ u32EngFreqKHz)-1;
		outp32(REG_CLKDIV4, (inp32(REG_CLKDIV4) & ~VIN0_N) | (u32Divider<<12));	
		
		u32Divider  = u32SenSrc | ((u32Div0<<18) | (u32Div1<<23)) ;	
		sysprintf("Sensor port 1 Div0 = %d, Div1 = %d\n", u32Div0, u32Div1);
		outp32(REG_CLKDIV0, (inp32(REG_CLKDIV0) & ~(SENSOR0_N1 | SENSOR0_S | SENSOR0_N0)) | 
				 u32Divider );	
	}	
	else
	{
		volatile UINT32 u32Divider;
		u32Divider = (u32PllClock/2/u32EngFreqKHz)-1;
		outp32(REG_CLKDIV4, (inp32(REG_CLKDIV4) & ~VIN1_N) | (u32Divider<<27));	
		
		u32Divider = u32SenSrc | ((u32Div0<<13) | (u32Div1<<18));
		sysprintf("Sensor port 2 Div0 = %d, Div1 = %d\n", u32Div0, u32Div1);
		outp32(REG_CLKDIV5, (inp32(REG_CLKDIV5) & ~(SENSOR1_N1 | SENSOR1_S | SENSOR1_N0)) | 
				   u32Divider);		
	}	


		
	sysInstallISR(IRQ_LEVEL_1, 
						IRQ_VIN, 
						(PVOID)videoInIntHandler);
	sysEnableInterrupt(IRQ_VIN);	
	return Successful;
} // DrvVideoIn_Open
/*---------------------------------------------------------------------------------------------------------*/
/* Function: videoIn_Close 									                                           */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      Nonre: 													                                           */
/*      u32SensorFreq : Sensor working Freq                                                                */
/* Returns:                                                                                                */
/*      None                                                                                               */
/*                                                                                                         */
/* Description:                                                                                            */
/*     	Disable pin function,engine clock and interruot                 			                       */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
void
videoIn_Close(
	void
	)
{
	// 1. Disable IP's interrupt
	sysDisableInterrupt(IRQ_VIN);	
	// 2. Disable IP¡¦s clock
	outp32(REG_AHBCLK, inp32(REG_AHBCLK) & ~ SEN_CKE);	
	videoIn_Reset();
	outp32(REG_AHBCLK, inp32(REG_AHBCLK) & ~VIN_CKE);			
	// 3. Disable Capture pin function 
	if(g_u32DeviceType == eVIDEOIN_SNR_CCIR656)
	{//VSYNC and HSYN by data
		outp32(REG_GPBFUN, inp32(REG_GPBFUN) & ~( MF_GPB0 | MF_GPB1 | 						//SCLKO, PCLK
										MF_GPB5 | MF_GPB6 | MF_GPB7 | MF_GPB8 | 	//VDATA[7:0]
										MF_GPB9 | MF_GPB10 | MF_GPB11 | MF_GPB12) );		
																   										   		
	}								   
	else if(g_u32DeviceType == eVIDEOIN_SNR_CCIR601)
	{//VSYNC and HSYN by pins
		outp32(REG_GPBFUN, inp32(REG_GPBFUN) & ~( MF_GPB0 | MF_GPB1 | MF_GPB2 | MF_GPB3 |	//SCLKO, PCLK, HSYNC and VSYNC
										MF_GPB5 | MF_GPB6 | MF_GPB7 | MF_GPB8 | 	//VDATA[7:0]
										MF_GPB9 | MF_GPB10 | MF_GPB11 | MF_GPB12) );	
	}
	else if(g_u32DeviceType == eVIDEOIN_TVD_CCIR601)
	{
		outp32(REG_GPBFUN, inp32(REG_GPBFUN) & ~( MF_GPB0 | MF_GPB1 | MF_GPB2 | MF_GPB3 | MF_GPB4 |	//SCLKO, PCLK, HSYNC, VSYNC and SFILED
										MF_GPB5 | MF_GPB6 | MF_GPB7 | MF_GPB8 | 	//VDATA[7:0]
										MF_GPB9 | MF_GPB10 | MF_GPB11 | MF_GPB12) );									   					
	}
	else if(g_u32DeviceType == eVIDEOIN_TVD_CCIR656)
	{
		outp32(REG_GPBFUN, inp32(REG_GPBFUN) & ~( MF_GPB1 | 	//SCLKO and PCLK 
										MF_GPB5 | MF_GPB6 | MF_GPB7 | MF_GPB8 | 	//VDATA[7:0]
										MF_GPB9 | MF_GPB10 | MF_GPB11 | MF_GPB12) );																			   
	}else if(g_u32DeviceType == eVIDEOIN_2ND_SNR_CCIR601)
	{
	  	outp32(REG_GPBFUN, inp32(REG_GPBFUN) & 0xFC00000F);	  	//Disable 1st sensor interface	  
		outp32(REG_GPCFUN, (inp32(REG_GPCFUN) & (~0xFFFF0000)) );		
										
		outp32(REG_GPEFUN, (inp32(REG_GPEFUN) & (~0xF)) );			//Disable 2nd sensor interface 														  								 										  
		outp32(REG_GPBFUN, (inp32(REG_GPBFUN) & (~0xF)));			
										    			
	}					   							   	
} // DrvVideoIn_Close

UINT32 uCapIntCallback = eVIDEOIN_VINT;
/*---------------------------------------------------------------------------------------------------------*/
/* Function: videoIn_SetPacketFrameBufferControl 									                   */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      bFrameSwitch: Software mode buffer select  				                                           */
/*					0: Packet buffer 0																	   */
/*					1: Packet buffer 1																	   */
/*      bFrameBufferSel :   Buffer control by FSC IP                                                       */
/* Returns:                                                                                                */
/*      None                                                                                               */
/*                                                                                                         */
/* Description:                                                                                            */
/*     	Disable pin function,engine clock and interruot                 			                       */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
void videoIn_SetPacketFrameBufferControl(
	BOOL bFrameSwitch,
	BOOL bFrameBufferSel
	)
{
	UINT32 u32Ctl = inp32(REG_VPECTL+g_u32PortOffset) & ~(ADDRSW | FBMODE);
	outp32(REG_VPECTL+g_u32PortOffset, u32Ctl |
					((bFrameBufferSel?FBMODE:0) | 	
					(bFrameSwitch?ADDRSW:0)));	
}

void 
videoIn_GetPacketFrameBufferControl(
	PBOOL pbFrameSwitch,
	PBOOL pbFrameBufferSel
	)
{
	UINT32 u32Ctl = inp32(REG_VPECTL+g_u32PortOffset);
	*pbFrameBufferSel = (u32Ctl & FBMODE) >> 2;
	*pbFrameSwitch = (u32Ctl & ADDRSW) >> 3;
}
/*---------------------------------------------------------------------------------------------------------*/
/* Function: videoIn_InstallCallback			  									                       */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      eIntType: Interrupt type				  				                                           */
/*      pvIsr: Call back fucntion					                                                       */
/* Returns:                                                                                                */
/*      None                                                                                               */
/*                                                                                                         */
/* Description:                                                                                            */
/*     	Disable pin function,engine clock and interrupt                 			                       */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
ERRCODE 
videoIn_InstallCallback(
	E_VIDEOIN_INT_TYPE eIntType, 
	PFN_VIDEOIN_CALLBACK pfnCallback,
	PFN_VIDEOIN_CALLBACK *pfnOldCallback
	)
{
	UINT32 u32Port;
	if(g_u32PortOffset==0)
		u32Port = 0;
	else
		u32Port = 1;		
	if(eIntType == eVIDEOIN_VINT)
	{
		*pfnOldCallback = pfnVideoInIntHandlerTable[u32Port][0];
		pfnVideoInIntHandlerTable[u32Port][0] = (PFN_VIDEOIN_CALLBACK)(pfnCallback);
	}	
	else if(eIntType == eVIDEOIN_ADDRMINT)
	{
		*pfnOldCallback = pfnVideoInIntHandlerTable[u32Port][1];
		pfnVideoInIntHandlerTable[u32Port][1] = (PFN_VIDEOIN_CALLBACK)(pfnCallback);
	}
	else if(eIntType == eVIDEOIN_MEINT)
	{
		*pfnOldCallback = pfnVideoInIntHandlerTable[u32Port][2];
		pfnVideoInIntHandlerTable[u32Port][2] = (PFN_VIDEOIN_CALLBACK)(pfnCallback);
	}	
	else if(eIntType == eVIDEOIN_MDINT)
	{
		*pfnOldCallback = pfnVideoInIntHandlerTable[u32Port][3];
		pfnVideoInIntHandlerTable[u32Port][3] = (PFN_VIDEOIN_CALLBACK)(pfnCallback);
	}	
	else
		return E_VIDEOIN_INVALID_INT;			
	return Successful;	
}



void
videoIn_Reset(
	void
	)
{	
	outp32(REG_VPECTL+g_u32PortOffset, inp32(REG_VPECTL+g_u32PortOffset) | VPRST);			
	outp32(REG_VPECTL+g_u32PortOffset, inp32(REG_VPECTL+g_u32PortOffset) & (~VPRST));			
} // VideoIn_Reset

ERRCODE 
videoIn_EnableInt(
	E_VIDEOIN_INT_TYPE eIntType
	)
{
	switch(eIntType)
	{
		case eVIDEOIN_MDINT:
		case eVIDEOIN_ADDRMINT:							
		case eVIDEOIN_MEINT:				
		case eVIDEOIN_VINT:				
				outp32(REG_VPEINT+g_u32PortOffset, inp32(REG_VPEINT+g_u32PortOffset) | eIntType);
				break;		
		default: 				 		
			return E_VIDEOIN_INVALID_INT;
	}
	return Successful;
} // VideoIn_EnableInt

ERRCODE
videoIn_DisableInt(
	E_VIDEOIN_INT_TYPE eIntType
	)
{
	switch(eIntType)
	{
		case eVIDEOIN_MDINT:
		case eVIDEOIN_ADDRMINT:							
		case eVIDEOIN_MEINT:				
		case eVIDEOIN_VINT:				
				outp32(REG_VPEINT+g_u32PortOffset, inp32(REG_VPEINT+g_u32PortOffset) & ~eIntType );
				break;		
		default: 				 		
			return E_VIDEOIN_INVALID_INT;
	}
	return Successful;
} // videoIn_DisableInt

BOOL
DrvVideoIn_IsIntEnabled(
	E_VIDEOIN_INT_TYPE eIntType
	)
{
	UINT32 u32IntEnable = inp32(REG_VPEINT+g_u32PortOffset);	
	switch(eIntType)
	{
		case eVIDEOIN_MDINT:
				u32IntEnable = u32IntEnable & eVIDEOIN_MDINT;
				break;		
		case eVIDEOIN_ADDRMINT:	
				u32IntEnable = u32IntEnable & eVIDEOIN_ADDRMINT;
				break;					
		case eVIDEOIN_MEINT:			
				u32IntEnable = u32IntEnable & eVIDEOIN_MEINT;
				break;	
		case eVIDEOIN_VINT:				
				u32IntEnable = u32IntEnable & eVIDEOIN_VINT;
				break;		
	}
	return (u32IntEnable?TRUE:FALSE);	
} // DrvVideoIn_IsIntEnabled
//===================================================================================
//
//	Clear the interrupt status. //Write one clear 
//
//
//
//
//
//
//
//
//
//
//===================================================================================
ERRCODE
videoIn_ClearInt(
	E_VIDEOIN_INT_TYPE eIntType
	)
{
	UINT32 u32IntChannel = eIntType >>16;
	switch(eIntType)
	{
		case eVIDEOIN_MDINT:
				outp32(REG_VPEINT+g_u32PortOffset, (inp32(REG_VPEINT+g_u32PortOffset) & ~((eVIDEOIN_ADDRMINT | eVIDEOIN_MEINT | eVIDEOIN_VINT)>>16)) | 
								u32IntChannel);
				break;					
		case eVIDEOIN_ADDRMINT:		
				outp32(REG_VPEINT+g_u32PortOffset, (inp32(REG_VPEINT+g_u32PortOffset) & ~((eVIDEOIN_MDINT | eVIDEOIN_MEINT | eVIDEOIN_VINT)>>16)) | 
								u32IntChannel);
				break;					
		case eVIDEOIN_MEINT:
				outp32(REG_VPEINT+g_u32PortOffset, (inp32(REG_VPEINT+g_u32PortOffset) & ~((eVIDEOIN_MDINT | eVIDEOIN_ADDRMINT | eVIDEOIN_VINT)>>16)) | 
								u32IntChannel);
				break;					
		case eVIDEOIN_VINT:				
				outp32(REG_VPEINT+g_u32PortOffset, (inp32(REG_VPEINT+g_u32PortOffset) & ~((eVIDEOIN_MDINT | eVIDEOIN_MEINT | eVIDEOIN_ADDRMINT)>>16)) | 
								u32IntChannel);
				break;	
		default: 				 		
			return E_VIDEOIN_INVALID_INT;
	}
	return Successful;


} // DrvVideoIn_ClearInt

//===================================================================================
//
//	Polling the interrupt status 
//
//
//	return value could be 
//	
//	VIDEOIN_ADDRMINT | VIDEOIN_MEINT | VIDEOIN_VINT
//
//
//
//
//
//===================================================================================
BOOL
DrvVideoIn_PollInt(
	E_VIDEOIN_INT_TYPE eIntType
	)
{
	UINT32 u32IntStatus = inp32(REG_VPEINT+g_u32PortOffset);	
	switch(eIntType)
	{
		case eVIDEOIN_MDINT:
				u32IntStatus = u32IntStatus & (eVIDEOIN_MDINT>>16);
				break;		
		case eVIDEOIN_ADDRMINT:	
				u32IntStatus = u32IntStatus & (eVIDEOIN_ADDRMINT>>16);
				break;					
		case eVIDEOIN_MEINT:			
				u32IntStatus = u32IntStatus & (eVIDEOIN_MEINT>>16);
				break;	
		case eVIDEOIN_VINT:				
				u32IntStatus = u32IntStatus & (eVIDEOIN_VINT>>16);
				break;		
	}
	return (u32IntStatus?TRUE:FALSE);
} // DrvVideoIn_PollInt

//===================================================================================
//
//	Enable engine and turn on the pipe.
//
//
//
//
//
//
//	u32PipeEnable = 0. Both pipe disable.
//					1. Packet pipe enable.	
//					2. Planar pipe enable.
//					3. Both pipe enable.
//===================================================================================
void DrvVideoIn_SetPipeEnable(
	BOOL bEngEnable,     				// TRUE: Enable, FALSE: Disable
	E_VIDEOIN_PIPE ePipeEnable    
	)
{
	outp32(REG_VPECTL+g_u32PortOffset, (inp32(REG_VPECTL+g_u32PortOffset) & ~(VPEEN | PKEN | PNEN)) 
    | (((bEngEnable ? VPEEN : 0x0))
    // | ((ePipeEnable & ~(PKEN | PNEN))<<5)) );
    | ((ePipeEnable & 0x03)<<5)) );
} // DrvVideoIn_SetPipeEnable

void DrvVideoIn_GetPipeEnable(
	PBOOL pbEngEnable,      			// TRUE: Enable, FALSE: Disable
	E_VIDEOIN_PIPE* pePipeEnable     // TRUE: Enable, FALSE: Disable
	)
{
	UINT32 u32Temp = inp32(REG_VPECTL+g_u32PortOffset);
	
	*pbEngEnable = (u32Temp & VPEEN) ? TRUE : FALSE;
	*pePipeEnable = (u32Temp & (PKEN | PNEN))>>5;
} // DrvVideoIn_GetPipeEnable


void 
DrvVideoIn_SetShadowRegister(
	void
	)
{
	outp32(REG_VPECTL+g_u32PortOffset,  inp32(REG_VPECTL+g_u32PortOffset) | UPDATE);
} // DrvVideoIn_SetShadowRegister

void 
DrvVideoIn_SetSensorPolarity(
	BOOL bVsync,       					// TRUE: High Active, FALSE: Low Active
	BOOL bHsync,       					// TRUE: High Active, FALSE: Low Active
	BOOL bPixelClk     					// TRUE: Falling Edge, FALSE: Rising Edge
	)
{
	outp32(REG_VPEPAR+g_u32PortOffset, (inp32(REG_VPEPAR+g_u32PortOffset) & ~(VSP| HSP | PCLKP)) 
    | (((bVsync ? VSP : 0x0)
    | (bHsync ? HSP : 0x0))
    | (bPixelClk ? PCLKP : 0x0)) );
} // DrvVideoIn_SetSensorPolarity

void 
DrvVideoIn_GetSensorPolarity(
	PBOOL pbVsync,      				// TRUE: High Active, FALSE: Low Active
	PBOOL pbHsync,      				// TRUE: High Active, FALSE: Low Active
	PBOOL pbPixelClk    				// TRUE: Falling Edge, FALSE: Rising Edge
	)
{
	UINT32 u32Temp = inp32(REG_VPEPAR+g_u32PortOffset);
	
	*pbVsync = (u32Temp & VSP)  ? TRUE : FALSE;
	*pbHsync = (u32Temp & HSP) ? TRUE : FALSE;
	*pbPixelClk = (u32Temp & PCLKP) ? TRUE : FALSE;
} // DrvVideoIn_GetSensorPolarity
/*---------------------------------------------------------------------------------------------------------*/
/* Function: DrvVideoIn_InstallInterrupt			  									                   */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      eInputOrder: Data order for input format 														   */ 
/*			00 = Y0 U0 Y1 V0																			   */
/*			01 = Y0 V0 Y1 U0																			   */
/*			10 = U0 Y0 V0 Y1																			   */
/*			11 = V0 Y0 U0 Y1    	  				                                          			   */
/*      eInputFormat: 																					   */
/*			0 = YUV.	1=RGB			 					                                               */
/* 		eOutputFormat: 																				       */
/*			00=YCbCr422.	01=only output Y.   10=RGB555	11=RGB565									   */
/* Returns:                                                                                                */
/*      None                                                                                               */
/*                                                                                                         */
/* Description:                                                                                            */
/*     	Set the input format, input order and output format               			                       */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
void 
DrvVideoIn_SetDataFormatAndOrder(
	E_VIDEOIN_ORDER eInputOrder, 
	E_VIDEOIN_IN_FORMAT eInputFormat, 
	E_VIDEOIN_OUT_FORMAT eOutputFormat
	)
{
	outp32(REG_VPEPAR+g_u32PortOffset, (inp32(REG_VPEPAR+g_u32PortOffset) & ~(OUTFMT | PDORD | INFMT)) 
    | ((((eInputOrder << 2) & PDORD)
    | (eInputFormat & INFMT))
    | ((eOutputFormat <<4 ) & OUTFMT)) );
} // DrvVideoIn_SetDataFormatAndOrder

void DrvVideoIn_GetDataFormatAndOrder(
	E_VIDEOIN_ORDER* peInputOrder, 
	E_VIDEOIN_IN_FORMAT* peInputFormat, 
	E_VIDEOIN_OUT_FORMAT* peOutputFormat
	)
{
	UINT32 u32Temp = inp32(REG_VPEPAR);
	
	*peInputOrder = (u32Temp & PDORD) >> 2;
	*peInputFormat = u32Temp & INFMT;
	*peOutputFormat = (u32Temp & OUTFMT) >> 4;
} // DrvVideoIn_GetDataFormatAndOrder

/*----------------------------------------------------------------------------------------------------------*/
/* Function: DrvVideoIn_SetPlanarFormat			  									                	   	*/
/*                                                                                                         	*/
/* Parameters:                                                                                             	*/
/*      bIsYUV420: Planar Format																		   	*/ 
/*			FALSE : Planar YUV422																		   	*/
/*			TRUE : Planar YUV420																		   	*/
/* Returns:                                                                                                	*/
/*      None                                                                                               	*/
/*                                                                                                         	*/
/* Description:                                                                                            	*/
/*     	Set the planar output format, 						               			                       	*/
/*                                                                                                         	*/
/*----------------------------------------------------------------------------------------------------------*/
void DrvVideoIn_SetPlanarFormat(
	BOOL bIsYUV420
)
{
	outp32(REG_VPEPAR+g_u32PortOffset, (inp32(REG_VPEPAR+g_u32PortOffset) & ~PNFMT) | 
					(bIsYUV420 ? PNFMT : 0));
}

BOOL 
DrvVideoIn_GetPlanarFormat(void)
{
	return ((inp32(REG_VPEPAR+g_u32PortOffset) & PNFMT)>>7);
}


/*----------------------------------------------------------------------------------------------------------*/
/* Function: DrvVideoIn_SetMotionDet			  									                	   	*/
/*                                                                                                         	*/
/* Parameters:                                                                                             	*/
/*      bEnable: Enable Motion Detection																	*/ 
/*			FALSE : Disable MD																			   	*/
/*			TRUE : Enable MD																			   	*/
/*      bBlockSize: Motion Detection Block Size																*/ 
/*			FALSE : 16x16																				   	*/
/*			TRUE : 8x8																					   	*/
/*      bSaveMode: Motion Detection Save Mode  																*/ 
/*			FALSE : 1 bit DIFF + 7 Y Differential															*/
/*			TRUE : 	1 bit DIFF only 													   					*/
/* Returns:                                                                                                	*/
/*      None                                                                                               	*/
/*                                                                                                         	*/
/* Description:                                                                                            	*/
/*     	Set the motion detection parameter					               			                       	*/
/*                                                                                                         	*/
/*----------------------------------------------------------------------------------------------------------*/	
void DrvVideoIn_SetMotionDet(
	BOOL bEnable,
	BOOL bBlockSize,	
	BOOL bSaveMode
)
{
	outp32(REG_VPEMD+g_u32PortOffset, (inp32(REG_VPEMD+g_u32PortOffset) & ~(MDSM | MDBS | MDEN)) |
			(((bEnable?MDEN:0) | (bBlockSize?MDBS:0)) | 
				(bSaveMode?MDSM:0)));	
}
	
void DrvVideoIn_GetMotionDet(
	PBOOL pbEnable,
	PBOOL pbBlockSize,	
	PBOOL pbSaveMode
)
{
	UINT32 u32RegData = inp32(REG_VPEMD+g_u32PortOffset);
	*pbEnable = (u32RegData & MDEN);
	*pbBlockSize = (u32RegData & MDBS)>>8; 
	*pbSaveMode = (u32RegData & MDSM)>>9; 		
}
/*----------------------------------------------------------------------------------------------------------*/
/* Function: DrvVideoIn_SetMotionDetEx			  									                	   	*/
/*                                                                                                         	*/
/* Parameters:                                                                                             	*/
/*      u32DetFreq: MD detect frequency	(0~3)																*/ 
/*			0 : Each frame  																			   	*/
/*			1 :	Every 2 frame																	   			*/
/*			2 : every 3 frame																				*/
/*			3 : every 4 frame																				*/
/*      u32Threshold: Motion detection threshold (0~31)														*/ 
/*			(Value << 1) : Differential threshold 															*/
/*			Value 16 meaning threshold = 32																   	*/
/*      u32OutBuffer: Output buffer addree		  															*/ 
/* Returns:                                                                                                	*/
/*      None                                                                                               	*/
/*                                                                                                         	*/
/* Description:                                                                                            	*/
/*     	Set the motion detection parameter extention 					               			                       	*/
/*                                                                                                         	*/
/*----------------------------------------------------------------------------------------------------------*/			
void DrvVideoIn_SetMotionDetEx(	
	UINT32 u32Threshold,
	UINT32 u32OutBuffer,
	UINT32 u32LumBuffer
)
{
	outp32(REG_VPEMD+g_u32PortOffset, (inp32(REG_VPEMD+g_u32PortOffset) & ~MDTHR) |	
				((u32Threshold <<16) & MDTHR) );
	outp32(REG_MDADDR+g_u32PortOffset, u32OutBuffer);	
	outp32(REG_MDYADDR+g_u32PortOffset, u32LumBuffer);		
}
void DrvVideoIn_GetMotionDetEx(	
	PUINT32 pu32Threshold,
	PUINT32 pu32OutBuffer,
	PUINT32 pu32LumBuffer
)
{
	UINT32 u32RegData;
	u32RegData = inp32(REG_VPEMD+g_u32PortOffset); 
	//*pu32DetFreq = u32RegData & MDDF;
	*pu32Threshold = u32RegData & MDTHR;
	*pu32OutBuffer = inp32(REG_MDADDR+g_u32PortOffset);
	*pu32LumBuffer = inp32(REG_MDYADDR+g_u32PortOffset);
}	
void DrvVideoIn_SetMotionDetFreq(UINT32 u32DetFreq)
{
	outp32(REG_VPEMD+g_u32PortOffset, (inp32(REG_VPEMD+g_u32PortOffset) & ~MDDF) |
				((u32DetFreq <<10) & MDDF) );
}	
void DrvVideoIn_GetMotionDetFreq(PUINT32 pu32DetFreq)
{
	UINT32 u32RegData;
	u32RegData = inp32(REG_VPEMD+g_u32PortOffset); 
	*pu32DetFreq = u32RegData & MDDF;
}
//===================================================================================
//
//	One shutte or continuous mode
//
//
//
//
//
//
//	bIsOneSutterMode =  0. Continuous mode 
//		     			1. One shutte mode
//===================================================================================
void 
DrvVideoIn_SetOperationMode(
	BOOL bIsOneSutterMode
	)
{
	outp32(REG_VPECTL+g_u32PortOffset, (inp32(REG_VPECTL+g_u32PortOffset) & ~CAPONE) | 
			((bIsOneSutterMode <<16) & CAPONE) );
} // DrvVideoIn_SetOperationMode

BOOL 
DrvVideoIn_GetOperationMode(
	void
	)
{
	return ( (inp32(REG_VPECTL+g_u32PortOffset) & CAPONE) ? TRUE : FALSE );
} // DrvVideoIn_GetOperationMode


UINT32 
DrvVideoIn_GetProcessedDataCount(    // Read Only
	E_VIDEOIN_PIPE ePipe	 		 // Planar or packet pipe		
	)
{
	if(ePipe == eVIDEOIN_PACKET)
		return inp32(REG_CURADDRP+g_u32PortOffset);				/* Packet pipe */
	else if (ePipe == eVIDEOIN_PLANAR)
		return inp32(REG_CURADDRY+g_u32PortOffset);				/* Planar pipe */
	else 
		return 0;		
} // DrvVideoIn_GetProcessedDataCount


void 
DrvVideoIn_SetCropWinStartAddr(
	UINT32 u32VerticalStart, 
	UINT32 u32HorizontalStart
	)
{
	outp32(REG_VPECWSP+g_u32PortOffset, (inp32(REG_VPECWSP+g_u32PortOffset) & ~(CWSPV | CWSPH)) //(Y|X)
    | ((u32VerticalStart << 16)
    | u32HorizontalStart));
} // DrvVideoIn_SetCropWinStartAddr


void 
DrvVideoIn_GetCropWinStartAddr(
	PUINT32 pu32VerticalStart, 
	PUINT32 pu32HorizontalStart
	)
{
	UINT32 u32Temp = inp32(REG_VPECWSP+g_u32PortOffset);
	
	*pu32VerticalStart = (u32Temp & CWSPV) >> 16;
	*pu32HorizontalStart = u32Temp & CWSPH;
} // DrvVideoIn_GetCropWinStartAddr

void 
DrvVideoIn_SetCropWinSize(
	UINT32 u32Height, 
	UINT32 u32Width
	)
{
	outp32(REG_VPECWS+g_u32PortOffset, (inp32(REG_VPECWS+g_u32PortOffset) & ~(CWSH | CWSW)) 
    | ((u32Height << 16)
    | u32Width));
} // DrvVideoIn_SetCropWinSize


void 
DrvVideoIn_GetCropWinSize(
	PUINT32 pu32Height, 
	PUINT32 pu32Width
	)
{
	UINT32 u32Temp = inp32(REG_VPECWS+g_u32PortOffset);
	
	*pu32Height = (u32Temp & CWSH) >> 16;
	*pu32Width = u32Temp & CWSW;
} // DrvVideoIn_GetCropWinSize

ERRCODE 
DrvVideoIn_SetVerticalScaleFactor(
	E_VIDEOIN_PIPE ePipe,
	UINT16 u16Numerator, 
	UINT16 u16Denominator
	)
{//Y direction 
#if 1
	UINT8 u8NumeratorL = u16Numerator&0xFF, u8NumeratorH=u16Numerator>>8;
	UINT8 u8DenominatorL = u16Denominator&0xFF, u8DenominatorH = u16Denominator>>8;
	if(ePipe == eVIDEOIN_PACKET)
	{
		outp32(REG_VPEPKDSL+g_u32PortOffset, (inp32(REG_VPEPKDSL+g_u32PortOffset) & ~(DSVNL | DSVML)) 
    	| ((u8NumeratorL << 24)
    	| (u8DenominatorL << 16)));
    	outp32(REG_VPEPKDSH+g_u32PortOffset, (inp32(REG_VPEPKDSH+g_u32PortOffset) & ~(DSVNH | DSVMH)) 
    	| ((u8NumeratorH << 24)
    	| (u8DenominatorH << 16)));    	
    }	
    else if(ePipe == eVIDEOIN_PLANAR)
	{	
		outp32(REG_VPEPNDSL+g_u32PortOffset, (inp32(REG_VPEPNDSL+g_u32PortOffset) & ~(DSVNL | DSVML)) 
    	| ((u8NumeratorL << 24)
    	| (u8DenominatorL << 16)));
    	outp32(REG_VPEPNDSH+g_u32PortOffset, (inp32(REG_VPEPNDSH+g_u32PortOffset) & ~(DSVNH | DSVMH)) 
    	| ((u8NumeratorH << 24)
    	| (u8DenominatorH << 16)));	
	}
	else
		return E_VIDEOIN_INVALID_PIPE;
#else
	if(ePipe == eVIDEOIN_PACKET)
	{
		outp32(REG_VPEPKDS+g_u32PortOffset, (inp32(REG_VPEPKDS+g_u32PortOffset) & ~(DSVN | DSVM)) 
    	| ((u8Numerator << 24)
    	| (u8Denominator << 16)));    	    	
    }	
    else if(ePipe == eVIDEOIN_PLANAR)
    {
    	outp32(REG_VPEPNDS+g_u32PortOffset, (inp32(REG_VPEPNDS+g_u32PortOffset) & ~(DSVN | DSVM)) 
    	| ((u8Numerator << 24)
    	| (u8Denominator << 16)));	
    }
    else
		return E_VIDEOIN_INVALID_PIPE;
#endif
	return Successful;	
} // DrvVideoIn_SetVerticalScaleFactor

ERRCODE 
DrvVideoIn_GetVerticalScaleFactor(
	E_VIDEOIN_PIPE ePipe,
	PUINT16 pu16Numerator, 
	PUINT16 pu16Denominator
	)
{
	
#if 1	
	UINT32 u32Temp1, u32Temp2; 
	if(ePipe == eVIDEOIN_PACKET)
	{
		u32Temp1 = inp32(REG_VPEPKDSL+g_u32PortOffset);
		u32Temp2 = inp32(REG_VPEPKDSH+g_u32PortOffset);
	}	
	else if(ePipe == eVIDEOIN_PLANAR)
	{
		u32Temp1 = inp32(REG_VPEPNDSL+g_u32PortOffset);
		u32Temp2 = inp32(REG_VPEPNDSH+g_u32PortOffset);
	}	
	else
		return E_VIDEOIN_INVALID_PIPE;	
	*pu16Numerator = ((u32Temp1 & DSVNL) >> 24) | (((u32Temp2 & DSVNH) >> 24)<<8);
	*pu16Denominator = (u32Temp1 & DSVML) >> 16 | (((u32Temp2 & DSVMH) >> 16)<<8);			
#else
	UINT32 u32Temp; 
	if(ePipe == eVIDEOIN_PACKET)
		u32Temp = inp32(REG_VPEPKDS+g_u32PortOffset);
	else if(ePipe == eVIDEOIN_PLANAR)
		u32Temp = inp32(REG_VPEPNDS+g_u32PortOffset);
	else
		return E_VIDEOIN_INVALID_PIPE;	
	*pu8Numerator = (u32Temp & DSVN) >> 24;
	*pu8Denominator = (u32Temp & DSVM) >> 16;
#endif	
	return Successful;
} // DrvVideoIn_GetVerticalScaleFactor

ERRCODE 
DrvVideoIn_SetHorizontalScaleFactor(
	E_VIDEOIN_PIPE bPipe,
	UINT16 u16Numerator, 
	UINT16 u16Denominator
	)
{
#if 1
	UINT8 u8NumeratorL = u16Numerator&0xFF, u8NumeratorH=u16Numerator>>8;
	UINT8 u8DenominatorL = u16Denominator&0xFF, u8DenominatorH = u16Denominator>>8;
	if(bPipe == eVIDEOIN_PACKET)
	{
		outp32(REG_VPEPKDSL+g_u32PortOffset, (inp32(REG_VPEPKDSL+g_u32PortOffset) & ~(DSHNL | DSHML)) 
	    | ((u8NumeratorL << 8)
	    | u8DenominatorL));	
	    outp32(REG_VPEPKDSH+g_u32PortOffset, (inp32(REG_VPEPKDSH+g_u32PortOffset) & ~(DSHNH | DSHMH)) 
	    | ((u8NumeratorH << 8)
	    | u8DenominatorH));    
	}
	else  if(bPipe == eVIDEOIN_PLANAR)
	{
		outp32(REG_VPEPNDSL+g_u32PortOffset, (inp32(REG_VPEPNDSL+g_u32PortOffset) & ~(DSHNL | DSHML)) 
	    | ((u8NumeratorL << 8)
	    | u8DenominatorL));	
		outp32(REG_VPEPNDSH+g_u32PortOffset, (inp32(REG_VPEPNDSH+g_u32PortOffset) & ~(DSHNH | DSHMH)) 
	    | ((u8NumeratorH << 8)
	    | u8DenominatorH));	
	}	    
	else
		return E_VIDEOIN_INVALID_PIPE;	
#else
	if(bPipe == eVIDEOIN_PACKET)
	{
		outp32(REG_VPEPKDS+g_u32PortOffset, (inp32(REG_VPEPKDS+g_u32PortOffset) & ~(DSHN | DSHM)) 
	    | ((u8Numerator << 8)
	    | u8Denominator));
	}
	else  if(bPipe == eVIDEOIN_PLANAR)
	{
		outp32(REG_VPEPNDS+g_u32PortOffset, (inp32(REG_VPEPNDS+g_u32PortOffset) & ~(DSHN | DSHM)) 
	    | ((u8Numerator << 8)
	    | u8Denominator));
	}	    
	else
		return E_VIDEOIN_INVALID_PIPE;	
#endif		
	return Successful;
} // DrvVideoIn_SetHorizontalScaleFactor

ERRCODE 
DrvVideoIn_GetHorizontalScaleFactor(
	E_VIDEOIN_PIPE bPipe,
	PUINT16 pu16Numerator, 
	PUINT16 pu16Denominator
	)
{
	
#if 1
	UINT32 u32Temp1, u32Temp2;
	if(bPipe == eVIDEOIN_PACKET)
	{
		u32Temp1 = inp32(REG_VPEPKDSL+g_u32PortOffset);
		u32Temp2 = inp32(REG_VPEPKDSH+g_u32PortOffset);
	}	
	else  if(bPipe == eVIDEOIN_PLANAR)
	{
		u32Temp1 = inp32(REG_VPEPNDSL+g_u32PortOffset);
		u32Temp2 = inp32(REG_VPEPNDSH+g_u32PortOffset);
	}	
	else
		return E_VIDEOIN_INVALID_PIPE;				
	*pu16Numerator = ((u32Temp1 & DSHNL) >> 8) | (u32Temp2 & DSHNH);
	*pu16Denominator = (u32Temp1 & DSHML) | ((u32Temp2 & DSHMH)<<8);
#else	
	UINT32 u32Temp;
	if(bPipe == eVIDEOIN_PACKET)
		u32Temp = inp32(REG_VPEPKDS+g_u32PortOffset);
	else  if(bPipe == eVIDEOIN_PLANAR)
		u32Temp = inp32(REG_VPEPNDS+g_u32PortOffset);
	else
		return E_VIDEOIN_INVALID_PIPE;			
	*pu8Numerator = (u32Temp & DSHN) >> 8;
	*pu8Denominator = u32Temp & DSHM;
#endif	
	return Successful;
} // DrvVideoIn_GetHorizontalScaleFactor

void 
DrvVideoIn_SetFrameRateScaleFactor(
	UINT8 u8Numerator, 
	UINT8 u8Denominator
	)
{
	outp32(REG_VPEFRC+g_u32PortOffset, (inp32(REG_VPEFRC+g_u32PortOffset) & ~(FRCN | FRCM)) 
    | (((u8Numerator << 8) & FRCN)
    | (u8Denominator & FRCM)));
} // DrvVideoIn_SetFrameRateScaleFactor

void 
DrvVideoIn_GetFrameRateScaleFactor(
	PUINT8 pu8Numerator, 
	PUINT8 pu8Denominator
	)
{
	UINT32 u32Temp = inp32(REG_VPEFRC+g_u32PortOffset);
	
	*pu8Numerator = (u32Temp & FRCN) >> 8;
	*pu8Denominator = u32Temp & FRCM;
} // DrvVideoIn_GetFrameRateScaleFactor

void 
DrvVideoIn_SetAddressMatch(
	UINT32 u32AddressMatch
	)
{
	outp32(REG_CMPADDR+g_u32PortOffset, u32AddressMatch);
}

void 
DrvVideoIn_GetAddressMatch(
	PUINT32 pu32AddressMatch
	)
{
	*pu32AddressMatch = inp32(REG_CMPADDR+g_u32PortOffset);
}

void 
DrvVideoIn_SetStride(
	UINT32 u32PacketStride,
	UINT32 u32PlanarStride	
	)
{
	outp32(REG_VSTRIDE+g_u32PortOffset, ((u32PlanarStride<<16) & PNSTRIDE) | 
					(u32PacketStride & PKSTRIDE) );
}

void 
DrvVideoIn_GetStride(
	PUINT32	pu32PacketStride,
	PUINT32 pu32PlanarStride	
	)
{
	UINT32 u32Tmp =  inp32(REG_VSTRIDE+g_u32PortOffset);
	*pu32PlanarStride = (u32Tmp & PNSTRIDE) >>16;
	*pu32PacketStride = u32Tmp & PKSTRIDE;	
}

ERRCODE 
DrvVideoIn_SetBaseStartAddress(
	E_VIDEOIN_PIPE ePipe,
	E_VIDEOIN_BUFFER eBuf,
	UINT32 u32BaseStartAddr
	)	
{
	if(ePipe==eVIDEOIN_PACKET)
	{
		if(eBuf>eVIDEOIN_BUF1)
			return E_VIDEOIN_INVALID_BUF;
		outp32(REG_PACBA0+g_u32PortOffset+eBuf*4, u32BaseStartAddr);				
	}	
	else if(ePipe==eVIDEOIN_PLANAR)
	{
		if(eBuf>eVIDEOIN_BUF2)
			return E_VIDEOIN_INVALID_BUF;
		outp32(REG_YBA0+g_u32PortOffset+eBuf*4, u32BaseStartAddr);						
	}	
	else
		return E_VIDEOIN_INVALID_PIPE;
	return Successful;	
}

ERRCODE 
DrvVideoIn_GetBaseStartAddress(
	E_VIDEOIN_PIPE ePipe,
	E_VIDEOIN_BUFFER eBuf,
	PUINT32 pu32BaseStartAddr
	)
{
	if(ePipe==eVIDEOIN_PACKET)
	{
		if(eBuf>eVIDEOIN_BUF1)
			return E_VIDEOIN_INVALID_BUF;
		*pu32BaseStartAddr = inp32(REG_PACBA0+g_u32PortOffset+eBuf*4);		
	}	
	else if(ePipe==eVIDEOIN_PLANAR)
	{
		if(eBuf>eVIDEOIN_BUF2)
			return E_VIDEOIN_INVALID_BUF;	
		*pu32BaseStartAddr = inp32(REG_YBA0+g_u32PortOffset+eBuf*4);						
	}	
	else
		return E_VIDEOIN_INVALID_PIPE;
	return Successful;		
}


/*---------------------------------------------------------------------------------------------------------*/
/* Function: DrvVideoIn_InstallInterrupt			  									                   */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      u32FieldEnable:  																				   */ 
/*			00 = Both fields are disabled																   */
/*			01 = Field 1 enable																			   */
/*			10 = Field 2 enable																			   */
/*			11 = Both fields are enabled  	  		                                          			   */
/*      eInputType: 																					   */
/*			0 = CCIR601.	1=CCIR656	 					                                               */
/* 		bFieldSwap: 																					   */
/*			Swap field 1 and field 2																	   */
/* Returns:                                                                                                */
/*      None                                                                                               */
/*                                                                                                         */
/* Description:                                                                                            */
/*     	Set the input type, fileds enable or disable and fields swap       			                       */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

void 
DrvVideoIn_SetInputType(
	UINT32 	u32FieldEnable,						//0: Both fields are disabled. 1: Field 1 enable. 2: Field 2 enable. 3: Both fields are enable
	E_VIDEOIN_TYPE eInputType,				//0: CCIR601.	1: CCIR656 	
	BOOL 	bFieldSwap							//1: Swap two field 
	)	
{
	outp32(REG_VPEPAR+g_u32PortOffset, (inp32(REG_VPEPAR+g_u32PortOffset) & ~(FLDSWAP | FLD1EN | FLD0EN | FLDSWAP)) | 
									((u32FieldEnable << 16) & (FLD1EN | FLD0EN )) |
									((eInputType <<1 ) & SNRTYPE) | 									
    								((bFieldSwap << 13) & FLDSWAP) );
} // DrvVideoIn_SetInputType

void 
DrvVideoIn_GetInputType(
	PUINT32 pu32FieldEnable,					//0: Both fields are disabled. 1: Field 1 enable. 2: Field 2 enable. 3: Both fields are enable
	E_VIDEOIN_TYPE* peInputType,				//0: CCIR601.	1: CCIR656 	
	PBOOL 	pbFieldSwap							//1: Swap two field 
	)	
{
	UINT32 u32Tmp = inp32(REG_VPEPAR+g_u32PortOffset);	
	*pu32FieldEnable = (u32Tmp & ( FLD1EN | FLD0EN ))>>16;
	*peInputType = (u32Tmp & SNRTYPE )>>1;
	*pbFieldSwap = (u32Tmp & FLDSWAP )>>13;
} // DrvVideoIn_GetInputType


/*---------------------------------------------------------------------------------------------------------*/
/* Function: DrvVideoIn_IsStandardCCIR656			  								*/
/*                                                                                                         						*/
/* Parameters:                                                                                             					*/
/*      bIsStandard:           															*/ 
/*			0 = Non-Standard CCIR656 mode (OV7725 or Hynix 702) 					*/
/*			1 = Standard CCIR656 mode										  	*/
/* Returns:                                                                                                					*/
/*      None                                                                                              	 					*/
/*                                                                                                         						*/
/* Description:                                                                                            					*/
/*     	Set the CCIR656 mode  							    			                       	*/
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/
void DrvVideoIn_IsStandardCCIR656(BOOL bIsStandard)
{
	if(bIsStandard==TRUE)
		outp32(REG_VPEPAR, inp32(REG_VPEPAR) & ~BIT18);		// Standard
	else
		outp32(REG_VPEPAR, inp32(REG_VPEPAR) | BIT18);			// Non-Standard 
}
/*---------------------------------------------------------------------------------------------------------*/
/* Function: DrvVideoIn_SetFieldDetection			  									                   */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      bDetPosition: Available if "bFieldDetMethod"=0              									   */ 
/*			00 = Vsync start 																   			   */
/*			01 = Vsync end																			 	   */
/*      bFieldDetMethod: 																			       */
/*				0: Detect field by Vsync & Hsync(SA711113). 											   */
/*				1: Detect field by field pin(WT8861)													   */
/* Returns:                                                                                                */
/*      None                                                                                               */
/*                                                                                                         */
/* Description:                                                                                            */
/*     	Set the input type, fileds enable or disable and fields swap       			                       */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
void DrvVideoIn_SetFieldDetection(
	BOOL 	bDetPosition,					 	 	
	BOOL 	bFieldDetMethod						//0: Detect field by Vsync & Hsync(SA711113). 1:Detect field by field pin(WT8861)	 
)
{
	outp32(REG_VPEPAR+g_u32PortOffset, (inp32(REG_VPEPAR+g_u32PortOffset) & ~(FLDDETP | FLDDETM)) | 
									((bDetPosition << 15) & FLDDETP) |
    								((bFieldDetMethod << 14) & FLDDETM) );
} // DrvVideoIn_SetFieldDetection


void DrvVideoIn_GetFieldDetection(
	PBOOL 	pbDetPosition,						//0: Both fields are disabled. 1: Field 1 enable. 2: Field 2 enable. 3: Both fields are enable. It is only available as CCIR601 	 	
	PBOOL 	pbFieldDetMethod					//0: Detect field by Vsync & Hsync. 1:Detect field by CCIR656. It is only available as CCIR601	 
)
{
	UINT32 u32Tmp = inp32(REG_VPEPAR+g_u32PortOffset);		
	*pbDetPosition = (u32Tmp & FLDDETP) >> 15;
	*pbFieldDetMethod = (u32Tmp & FLDDETM) >> 14;	
} // DrvVideoIn_GetFieldDetection
/*----------------------------------------------------------------------------------------------------------*/
/* Function: DrvVideoIn_SetColorEffect			  									                   		*/
/*                                                                                                         	*/
/* Parameters:                                                                                             	*/
/*      eColorMode: Available as following             									  				 	*/ 
/*			eVIDEOIN_CEF_NORMAL = 0,																		*/
/*			eVIDEOIN_CEF_SEPIA = 1,																			*/
/*			eVIDEOIN_CEF_NEGATIVE = 2,																		*/
/*			eVIDEOIN_CEF_POSTERIZE = 3													   					*/
/* Returns:                                                                                                	*/
/*      None                                                                                               	*/
/*                                                                                                         	*/
/* Description:                                                                                            	*/
/*     	Set the special color mode     			                      						 				*/
/*                                                                                                         	*/
/*----------------------------------------------------------------------------------------------------------*/
ERRCODE DrvVideoIn_SetColorEffect(
	E_VIDEOIN_CEF 	eColorMode	
)					
{
	if(eColorMode>eVIDEOIN_CEF_POSTERIZE)
		return E_VIDEOIN_INVALID_COLOR_MODE;
	outp32(REG_VPEPAR+g_u32PortOffset, (inp32(REG_VPEPAR+g_u32PortOffset) & ~SCEF) | 
									(eColorMode<<11) );
	return Successful;								
} // DrvVideoIn_SetColorEffect

void DrvVideoIn_GetColorEffect(
	E_VIDEOIN_CEF* 	peColorMode						
)
{
	UINT32 u32Tmp = inp32(REG_VPEPAR+g_u32PortOffset);		
	*peColorMode = (u32Tmp & SCEF) >> 11;
} // DrvVideoIn_GetFieldDetection

ERRCODE 
DrvVideoIn_SetColorEffectParameter(
	UINT8 	u8YComp,
	UINT8  	u8UComp,
	UINT8 	u8VComp
	)
{
	UINT32 u32Tmp = inp32(REG_VPEPAR+g_u32PortOffset);		
	UINT32 u32ColorMode = (u32Tmp & SCEF) >> 11;
	if(u32ColorMode == eVIDEOIN_CEF_SEPIA)
	{
		outp32(REG_VSEPIA+g_u32PortOffset,  (((UINT32)u8UComp<<8) | u8VComp));	
	}
	else if(u32ColorMode == eVIDEOIN_CEF_POSTERIZE)
	{	
		outp32(REG_VPOSTERIZE+g_u32PortOffset,  (((UINT32)u8YComp<<16) | ((UINT32)u8UComp<<8) | u8VComp));	
	}else
	{
		return E_VIDEOIN_WRONG_COLOR_PARAMETER;
	}		
	return Successful;		
}

ERRCODE 
DrvVideoIn_GetColorEffectParameter(
	PUINT8 	pu8YComp,
	PUINT8  pu8UComp,
	PUINT8 	pu8VComp
	)
{
	UINT32 u32Tmp = inp32(REG_VPEPAR+g_u32PortOffset);		
	UINT32 u32ColorMode = (u32Tmp & SCEF) >> 11;
	if(u32ColorMode == eVIDEOIN_CEF_SEPIA)
	{
		u32Tmp = inp32(REG_VSEPIA+g_u32PortOffset);
		*pu8UComp = (u32Tmp & 0xFF00)>>8;	
		*pu8VComp = u32Tmp & 0xFF;	
	}
	else if(u32ColorMode == eVIDEOIN_CEF_POSTERIZE)
	{	
		u32Tmp = inp32(REG_VPOSTERIZE+g_u32PortOffset);
		*pu8YComp = (u32Tmp & 0xFF0000)>>16;
		*pu8UComp = (u32Tmp & 0xFF00)>>8;	
		*pu8VComp = u32Tmp & 0xFF;	
	}
	else
	{
		return E_VIDEOIN_WRONG_COLOR_PARAMETER;
	}		
	return Successful;		
}
