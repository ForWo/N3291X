/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                        	 					*/
/* Copyright (c) Nuvoton Technology Corp. All rights reserved.                                             		*/
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* Includes of system headers                                                                              				*/
/*---------------------------------------------------------------------------------------------------------*/
#include "wblib.h"
#include "w55fa95_reg.h"
#include "w55fa95_adc.h"
#include "adc.h"
#define NULL	0
volatile BOOL bIsWaitForTriggerCallback=FALSE, bIsConverseCallback = FALSE;
UINT32 bIsTouchOpen = FALSE;

static void DrvADC_Conversion(void)
{
	outp32(REG_ADC_CON, inp32(REG_ADC_CON) | ADC_CONV);	
}
static void DrvADC_GetNormalData(PUINT16 pu16Data)
{  		
	*pu16Data = inp32(REG_ADC_XDATA);					     				     											     											     											     				
}
static  VOID DrvADC_PollingADC(VOID)
{
	while( (inp32(REG_ADC_CON) & ADC_INT) != ADC_INT);	/* Wait until ADC INT */ 
	 outp32(REG_ADC_CON, (inp32(REG_ADC_CON) & 	  	/* Clean the ADC interrupt */     	
		        				~WT_INT ) |  
		        				(WT_INT |ADC_INT));
}

void WaitForTriggerCallback(void)
{
	bIsWaitForTriggerCallback = TRUE;
}
void ConverseCallback(void)
{
	bIsConverseCallback = TRUE;	
}

INT32 adc_open(UINT32 type, UINT32 hr, UINT32 vr)
{
	PFN_DRVADC_CALLBACK pfnOldCallback;

	DrvADC_TS_Open(eADC_NORMAL, NULL, NULL);
	//DrvADC_EnableInt(eDRVADC_WT_INT);  
	DrvADC_InstallCallback(eDRVADC_WT_INT,
							WaitForTriggerCallback,
							&pfnOldCallback);

	/* For eDRVADC_TSCREEN_AUTO mode, ADC_INT */
	DrvADC_EnableInt(eDRVADC_ADC_INT);							
	DrvADC_InstallCallback(eDRVADC_ADC_INT,
							ConverseCallback,
							&pfnOldCallback);
	bIsTouchOpen = TRUE;						
	return Successful;	
}
VOID adc_close(VOID)
{
	/* Touch off touch Clock */
	outp32(REG_APBCLK, inp32(REG_APBCLK) & ~ TOUCH_CKE);
	sysDisableInterrupt(IRQ_ADC);
	bIsTouchOpen = FALSE;
}
//int adc_read(unsigned char mode, unsigned short *x, unsigned short *y)
INT32 adc_read(UINT32 mode, PUINT16 x, PUINT16 y)
{
/*
	if(mode != ADC_NONBLOCK && mode != ADC_BLOCK) 
	{
		sysDisableInterrupt(IRQ_ADC);
		return(-1);
	}
*/		
	INT32 i32ErrCode = 1; 
	INT32 i32Delay = 100;
	DrvADC_SetTouchScreen(eDRVADC_TSCREEN_TRIG,			//E_DRVADC_TSC_MODE eTscMode,
							eDRVADC_TSCREEN_4WIRE,		//E_DRVADC_TSC_TYPE eTscWire,
							150,
							TRUE,						//BOOL bIsPullup,
							TRUE);						//BOOL bMAVFilter
	while(i32Delay--);						
	bIsWaitForTriggerCallback = 0;	
	DrvADC_EnableInt(eDRVADC_WT_INT);	
	if(mode==ADC_BLOCK)		
		while(bIsWaitForTriggerCallback==0);
	else
	{
		if(bIsWaitForTriggerCallback==0)
			i32ErrCode =  0; 	/* Pen up state */
	}		
								
	/*After here, Wait for trigger will be disable in ISR*/
	DrvADC_SetTouchScreen(eDRVADC_TSCREEN_AUTO,			//E_DRVADC_TSC_MODE eTscMode,
							eDRVADC_TSCREEN_4WIRE,		//E_DRVADC_TSC_TYPE eTscWire,
							150,
							TRUE,						//BOOL bIsPullup,
							TRUE);						//BOOL bMAVFilter
	
	bIsConverseCallback = 0;
	DrvADC_EnableInt(eDRVADC_ADC_INT);	
	outp32(0xb800E000, inp32(0xb800E000) | BIT13);	//Force ADC to message Position
	while(bIsConverseCallback==0);	
	if((inp32(REG_TSC_MAV_X)&TS_VALUE)==TS_VALUE)
	{
		//sysprintf("Position = %d (X, Y) = %d, %d\n", inp32(REG_TSC_MAV_X)&X_MAV_AVG, inp32(REG_TSC_MAV_Y));
		*x =  inp32(REG_TSC_MAV_X)&X_MAV_AVG;
		*y =  inp32(REG_TSC_MAV_Y);
		i32ErrCode = 1;	/* Pen down state */
	}	
	else
	{
		//sysprintf("Sample 4= %d. (X, Y) = %d, %d\n",(inp32(REG_TSC_SORT4)&TS_VALUE)>>31, inp32(REG_TSC_SORT4)&X_MAV, (inp32(REG_TSC_SORT4)&Y_MAV)>>16);
		//sysprintf("Sample 3= %d. (X, Y) = %d, %d\n",(inp32(REG_TSC_SORT3)&TS_VALUE)>>31, inp32(REG_TSC_SORT4)&X_MAV, (inp32(REG_TSC_SORT3)&Y_MAV)>>16);
		//sysprintf("Sample 2= %d. (X, Y) = %d, %d\n",(inp32(REG_TSC_SORT2)&TS_VALUE)>>31, inp32(REG_TSC_SORT4)&X_MAV, (inp32(REG_TSC_SORT2)&Y_MAV)>>16);
		//sysprintf("Sample 1= %d. (X, Y) = %d, %d\n",(inp32(REG_TSC_SORT1)&TS_VALUE)>>31, inp32(REG_TSC_SORT4)&X_MAV, (inp32(REG_TSC_SORT1)&Y_MAV)>>16);
		//sysprintf("\n");
		i32ErrCode =  0; 	/* Pen up state */
	}
	DrvADC_SetTouchScreen(eDRVADC_TSCREEN_TRIG,				//E_DRVADC_TSC_MODE eTscMode,
								eDRVADC_TSCREEN_4WIRE,	//E_DRVADC_TSC_TYPE eTscWire,
								150,
								TRUE,					//BOOL bIsPullup,
								TRUE);					//BOOL bMAVFilter	
	return i32ErrCode;									
	
}

UINT32 adc_normalread(UINT32 u32Channel, PUINT16 pu16Data)
{
	BOOL bIsEnableWtInt=FALSE;

	if( (u32Channel>4)&&(u32Channel<2) )
		return -1; //return E_DRVADC_INVALID_CHANNEL;
	//if(_state != ADC_STATE_WT)	
	//	return -1; //return E_DRVADC_INVALID_TIMING;
	if( inp32(REG_ADC_CON) & WT_INT_EN )
	{
		DrvADC_DisableInt(eDRVADC_WT_INT);	
		bIsEnableWtInt = TRUE;
	}	
	DrvADC_SetTouchScreen(eDRVADC_TSCREEN_AUTO,			//E_DRVADC_TSC_MODE eTscMode,
								eDRVADC_TSCREEN_4WIRE,
								0x150,
								TRUE,					//BOOL bIsPullup,
								TRUE);
    	DrvADC_EnableInt(eDRVADC_ADC_INT);	
	
	outp32( REG_ADC_CON,( ADC_CON_ADC_EN | ADC_CONV | ADC_INT 
						| (u32Channel<<9)) );
	DrvADC_Conversion();	
	//while( (inp32(REG_ADC_CON) & ADC_INT) != ADC_INT);	/* Wait until ADC INT */ 
	DrvADC_PollingADC();
	DrvADC_GetNormalData(pu16Data);		
	
	outp32(REG_ADC_CON, (inp32(REG_ADC_CON) & ~(ADC_TSC_MODE|ADC_MUX))|(ADC_INT | WT_INT)); 	
	/* set to WT mode */
	DrvADC_SetTouchScreen(eDRVADC_TSCREEN_TRIG,					//E_DRVADC_TSC_MODE eTscMode,
								eDRVADC_TSCREEN_4WIRE,
								0x150,
								TRUE,						//BOOL bIsPullup,
								TRUE);						//BOOL bMAVFilter
	DrvADC_EnableInt(eDRVADC_ADC_INT);									
	if(bIsEnableWtInt)
		DrvADC_EnableInt(eDRVADC_WT_INT);
	return Successful;					
}

//==============================================================================================================
//==============================================================================================================
//==============================================================================================================
//==============================================================================================================
//==============================================================================================================
extern void ADC_IntHandler(void);
extern BOOL bIsAUDEnable;
BOOL bIsTSEnable = FALSE;
INT32 
DrvADC_TS_Open(
	E_ADC_MODE eADC_Mode,
	UINT32 u32SrcClock,
	UINT32 u32ConvClock
)
{//Wait for ST add the GCR clock controll bit.
	/* Enable clock */
	UINT32 u32ExtClock;
	UINT32 u32N1;
	/* Gloabal Reset IP */	
	
	/* Touch Panel Set Clock */
	outp32(REG_APBCLK, inp32(REG_APBCLK) | TOUCH_CKE | ADC_CKE);	/* Issue! Touch has to enable ADC_CKE and TOUCH_CKE */
	outp32(REG_AUDADC_CTL, inp32(REG_AUDADC_CTL) | APB2AHB);	
	
	//outp32(REG_APBCLK, inp32(REG_APBCLK) | TOUCH_CKE);			/* Issue */
	u32ExtClock = sysGetExternalClock();
	
	for(u32N1=1; u32N1<=32; u32N1=u32N1+1)
	{
		if((u32ExtClock/u32N1)<=2000000 )
			break;
	}
	u32N1 = u32N1-1;
	sysprintf("N1 = %d\n", u32N1);
	outp32(REG_CLKDIV5, (inp32(REG_CLKDIV5) & ~(TOUCH_N1|TOUCH_S|TOUCH_N0))  | (u32N1<<27));
	
	/* Local Reset IP */
	outp32(REG_ADC_CON, inp32(REG_ADC_CON)|(ADC_CON_ADC_EN | ADC_RST));
	outp32(REG_ADC_CON, inp32(REG_ADC_CON)&~ADC_RST);
	outp32(REG_AUDADC_CTL, (inp32(REG_AUDADC_CTL)& ~TP_STRTIME)| (0x10<<8));
	/* Hook TS interrupt */
	if(bIsAUDEnable == FALSE)
	{//Check bIsAUDEnable ??
		sysInstallISR(IRQ_LEVEL_1, 
						IRQ_ADC, 
						(PVOID)ADC_IntHandler);
		sysEnableInterrupt(IRQ_ADC);
	}
	else
	{
		sysInstallISR(IRQ_LEVEL_1, 
						IRQ_ADC, 
						(PVOID)ADC_IntHandler);
		sysEnableInterrupt(IRQ_ADC);	
	}
	bIsTSEnable = TRUE;	
	return Successful;
}	

/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         						*/
/* FUNCTION                                                                                                					*/
/*      DrvADC_SetChannel()		 		                                                               			*/
/*                                                                                                         						*/
/* DESCRIPTION                                                                                             					*/
/*      To set the ADC clock source.                                                                      	 			*/
/*                                                                                                         						*/
/* INPUTS                                                                                                  					*/
/*      u32Channel   									        		                                */																				 
/*                                                                                                         						*/
/* OUTPUTS                                                                                                 					*/
/*      none                            				                                                   				*/
/*                                                                                                         						*/
/* RETURN                                                                                                  					*/
/*      none				                                                                               				*/
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/
void 
DrvADC_SetChannel(
	UINT32 u32Channel
	)
{
	UINT32 u32RegData = (inp32(REG_ADC_CON) & (~ADC_MUX));	
	u32RegData = u32RegData | (u32Channel << 9);

	outp32(REG_ADC_CON, u32RegData);
}

/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         						*/
/* FUNCTION                                                                                                					*/
/* 		DrvADC_SetTouchScreen()	                      		                                               		*/
/*                                                                                                         						*/
/* DESCRIPTION                                                                                             					*/
/*     	Set the parameter for TSC													*/
/*                                                                                                         						*/
/* INPUTS                                                                                                  					*/
/*      eTscMode    	Normal mode, Semi-Auto, Auto or Wait for trigger   					        */
/*      eTscWire 		4 wire, 5 wire, 8 wire or unused   					          			*/
/*      bIsPullup		Control the internal pull up PMOS in switch box							*/
/*      bMAVFilter      Enable or disable MAV filter in TSC auto mode                                      		*/
/* OUTPUTS                                                                                                 					*/
/*      none 														                        */
/*                                                                                                         						*/
/* RETURN                                                                                                  					*/
/*      none																	*/
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/
void DrvADC_SetTouchScreen(
	E_DRVADC_TSC_MODE eTscMode,
	E_DRVADC_TSC_TYPE eTscWire,
	UINT32 u32DleayCycle,
	BOOL bIsPullup,
	BOOL bMAVFilter	
	)
{  				
    	outp32(REG_ADC_CON, (inp32(REG_ADC_CON) & ~(ADC_TSC_MODE)) | 	
    				(eTscMode << 14) );
   	outp32(REG_ADC_DLY, u32DleayCycle); 				
    	outp32(REG_ADC_TSC, (inp32(REG_ADC_TSC) & ~(ADC_TSC_TYPE | ADC_PU_EN | ADC_TSC_MAV_EN)) | 	
    				( (((eTscWire << 1) & ADC_TSC_TYPE) | ((bIsPullup <<3) & ADC_PU_EN)) |
    				((bMAVFilter<<16) & ADC_TSC_MAV_EN) ));				
}    
void DrvADC_GetTouchScreen(
	E_DRVADC_TSC_MODE* peTscMode,
	E_DRVADC_TSC_TYPE* peTscWire,
	PUINT32 pu32DleayCycle, 
	PBOOL pbIsPullup,
	PBOOL pbMAVFilter	
	)
{  		
	UINT32 u32RegData = inp32(REG_ADC_TSC);		
	
	*peTscMode = (inp32(REG_ADC_CON) & ADC_TSC_MODE) >> 14;
    	*peTscWire = (u32RegData & ADC_TSC_TYPE) >> 1;
    	*pu32DleayCycle = inp32(REG_ADC_DLY);
    	*pbIsPullup = (u32RegData & ADC_PU_EN) >> 3;
    	*pbMAVFilter = (u32RegData & ADC_TSC_MAV_EN) >> 9;						     				     											     											     											     				
}    

/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         						*/
/* FUNCTION                                                                                                					*/
/* 		DrvADC_GetMovingAverage()	                  		                                               		*/
/*                                                                                                         						*/
/* DESCRIPTION                                                                                             					*/
/*     	Get the moving average for TSC if MAV filter enable						              	*/
/*                                                                                                         						*/
/* INPUTS                                                                                                  					*/
/*      pu32AverageX    10 bit moving average for TSC auto mode if MAV enable				       	*/
/*      pu32AverageY 	10 bit moving average for TSC auto mode if MAV enable  				*/
/*                                                                                                         						*/
/* OUTPUTS                                                                                                 					*/
/*      none 														                      	*/
/*                                                                                                         						*/
/* RETURN                                                                                                  					*/
/*      none																	*/
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/
void DrvADC_GetMovingAverage(
	PUINT16 	pu16AverageX,
	PUINT16 	pu16AverageY
	)
{  		
	*pu16AverageX = inp32(REG_TSC_MAV_X) & X_MAV_AVG;
	*pu16AverageY = inp32(REG_TSC_MAV_Y) & Y_MAV_AVG;						     				     											     											     											     				
}   

BOOL 
DrvADC_GetTouchScreenUpDownState(void)
{
	return (inp32(REG_ADC_TSC)&ADC_UD);
}

void DrvADC_GetTscData(
	PUINT16 	pu16XData,
	PUINT16 	pu16YData
	)
{  		
	*pu16XData = inp32(REG_ADC_XDATA);
	*pu16YData = inp32(REG_ADC_YDATA);					     				     											     											     											     				
}


