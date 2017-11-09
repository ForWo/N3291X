/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         						*/
/* Copyright (c) Nuvoton Technology Corp. All rights reserved.                                             		*/
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/* Includes of system headers                                                                              				*/
/*---------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include "wblib.h"
#include "w55fa95_reg.h"
#include "w55fa95_adc.h"
#include "ADC.h"
/*---------------------------------------------------------------------------------------------------------*/
/* Global file scope (static) variables                                                                    				*/
/*---------------------------------------------------------------------------------------------------------*/
BOOL bIsAUDEnable = FALSE;
extern BOOL bIsTSEnable;
static PFN_DRVADC_CALLBACK g_psADCCallBack[3]={0, 0, 0};
static UINT8 g_u8IsRecord;
static UINT32 u32AudioCycle=1;
static UINT32 u32ADCReset = 0;

extern UINT32 bIsTouchOpen;

//#define DBG_PRINTF(...) 
#define DBG_PRINTF		sysprintf	

void DrvADC_SetSamplingRate(E_SYS_SRC_CLK eSrcClock,
						UINT32 u32SamplingRate,
						UINT32* pu32Cycle);

static void adc_reset(void)
{
	u32ADCReset = 1;
	outp32(REG_APBIPRST, inp32(REG_APBIPRST) | ADCRST);
    	outp32(REG_APBIPRST, inp32(REG_APBIPRST) & ~ADCRST);
}	
INT32 audio_Init(E_SYS_SRC_CLK eSrcClock, UINT32 u32ConvClock)
{			
	UINT32 u32Cycle;		
	DrvADC_SetSamplingRate(eSrcClock, u32ConvClock, &u32Cycle);
	outp32(REG_AGCP1, inp32(REG_AGCP1) | EDMA_MODE);
	DrvADC_FineTuneClock(u32Cycle-1);
	DrvADC_AUD_Open(eADC_RECORD,		//E_ADC_MODE eADC_Mode,
					eSrcClock,			//UINT32 u32SrcClock,
					u32ConvClock);	//UINT32 u32ConvClock	HZ Unit.
	DBG_PRINTF("Sample rate = %d\n",u32ConvClock);		
	return Successful;											
}						
INT32 audio_open(E_SYS_SRC_CLK eSrcClock, UINT32 u32ConvClock)
{
	
	audio_Init(eSrcClock, u32ConvClock);
#if 0	
	DrvADC_SetSamplingRate(eSrcClock, u32ConvClock, &u32Cycle);
	outp32(REG_AGCP1, inp32(REG_AGCP1) | EDMA_MODE);
	DrvADC_FineTuneClock(u32Cycle-1);
	DrvADC_AUD_Open(eADC_RECORD,		//E_ADC_MODE eADC_Mode,
					eSrcClock,			//UINT32 u32SrcClock,
					u32ConvClock);	//UINT32 u32ConvClock	HZ Unit.
#endif 
	DrvADC_OverFlowCtl(TRUE,			/* To avoid pop sound */
						0x5000);
					
	DrvADC_SetGainControl(eDRVADC_PRE_P14, 
								eDRVADC_POST_P0);
	DrvADC_SetNoiseGateThreshold(1,	//UINT32 u32GainStep, 
									2,	//UINT32 u32InSample,
									2);	//UINT32 u32OutSample	
	
	/* Sine wav is not gpod */
	DrvADC_SetAutoGainTiming(0x10,		//UINT32 u32Period,
								0x04,	//UINT32 u32Attack,
								0x04,	//UINT32 u32Recovery,
								0x04);	//UINT32 u32Hold
	
	/*Audio Recording is not good
	DrvADC_SetAutoGainTiming(0x40,		//UINT32 u32Period,
								0x08,	//UINT32 u32Attack,
								0x08,	//UINT32 u32Recovery,
								0x08);	//UINT32 u32Hold
	
	*/							
	
					   												    	    	    
    	DrvADC_SetNoiseGate(FALSE,     					
    						0,	
    						0); 		
		
	DrvADC_SetAutoGainControl(TRUE,
						//15, 		//Output target -6db
						11, 		//Output target -12db	
						//9,			//Output target -15db	
				     		eDRVADC_BAND_P0P5,
				     		eDRVADC_BAND_N0P5); 										
	return Successful;
}
void audio_close(void)
{
	if(bIsTouchOpen==FALSE)
	{/* If touch was close, turn off ADC_CKE */
	 	outp32(REG_APBCLK,inp32(REG_APBCLK)&~ADC_CKE);
	 }
}
void audio_stoprecord(void)
{
    	//Disable record function 
    	outp32(REG_AGCP1, inp32(REG_AGCP1) & ~EDMA_MODE);	
	DrvADC_StopRecord();
}
/*----------------------------------------------------------------------------------------------------------
 FUNCTION                                                                                                					
 		adc_StartRecord()		                                                                       			
                                                                                                         						
 DESCRIPTION                                                                                             					
     	Start to record Audio data. This function only can be used in                                      		
      ADC_RECORD mode.                                                                                   				
                                                                                                         						
 INPUTS                                                                                                  					
      none														                        
                                                                                                         						
 OUTPUTS                                                                                                 					
      none														                        
                                                                                                         						
 RETURN                                                                                                  					
      none				                                                                               				
                                                                                                         						
----------------------------------------------------------------------------------------------------------*/
void audio_startrecord(void)
{
    	//Enable Co-work with EDMA  	
    	outp32(REG_AGCP1, inp32(REG_AGCP1) | EDMA_MODE);	

   	// Clean INT status for safe 
    	outp32(REG_AUDIO_CON, inp32(REG_AUDIO_CON) | 
    				(AUDIO_RESET | AUDIO_INT) );
	outp32(REG_AUDIO_CON, inp32(REG_AUDIO_CON) & 
					~(AUDIO_RESET | AUDIO_INT) );
     	//Enable record function 
#if 1    
    	outp32(REG_AUDIO_CON, (inp32(REG_AUDIO_CON) & ~AUDIO_INT_MODE) |
    				(((1 << 30) &  AUDIO_INT_MODE) |
    			 	AUDIO_EN) );
#else
  	outp32(REG_AUDIO_CON, (inp32(REG_AUDIO_CON) & ~AUDIO_INT_MODE) |
    				(((0 << 30) &  AUDIO_INT_MODE) |
    			 	AUDIO_EN) );	
#endif    			 	    
}
/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         						*/
/* FUNCTION                                                                                                					*/
/* 		DrvADC_EnableInt()		                                   	   	                                   		*/
/*                                                                                                         						*/
/* DESCRIPTION                                                                                             					*/
/*     	enable ADC interrupt and setup callback function 	        	                                   	*/
/*                                                                                                         						*/
/* INPUTS                                                                                                  					*/
/*      callback                                                                                           					*/
/*          The callback funciton                                                                          					*/
/*                                                                                                         						*/
/*      u32UserData                                                                                        					*/
/*          The user's data to pass to the callback function                                               			*/
/*                                                                                                         						*/
/* OUTPUTS                                                                                                 					*/
/*      none 														                        */
/*                                                                                                         						*/
/* RETURN                                                                                                  					*/
/*      none														                        */
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/
INT32  
adc_enableInt(
	E_ADC_INT eIntType
	)
{
    /* Enable adc interrupt */
//#ifdef USING_INT    
    switch(eIntType)
    {
    		case eADC_ADC_INT: 
		 	outp32(REG_ADC_CON, inp32(REG_ADC_CON) | ADC_INT_EN);	    		                                     
    	   		 break; 
	   	case eADC_AUD_INT:
	   		outp32(REG_AUDIO_CON, inp32(REG_AUDIO_CON) | AUDIO_INT_EN);	 
	   		break;	
		case eADC_WT_INT:     
			outp32(REG_ADC_CON, inp32(REG_ADC_CON) | WT_INT_EN);	 
			break;	
		default:
			return ERR_ADC_INVALID_INT;
    }			
//#endif    
    return Successful;
}



INT32 
adc_disableInt(
	E_ADC_INT eIntType
	)
{
    /* Enable adc interrupt */
    switch(eIntType)
    {
    		case eADC_ADC_INT:
    			outp32(REG_ADC_CON, inp32(REG_ADC_CON) & ~ADC_INT_EN);	                                      
    	    	break; 
	   	case eADC_AUD_INT:
	   		outp32(REG_AUDIO_CON, inp32(REG_AUDIO_CON) & ~AUDIO_INT_EN);	 
	   		break;		
		case eADC_WT_INT:
			outp32(REG_ADC_CON, inp32(REG_ADC_CON) & ~WT_INT_EN);     
			break;	
		default:
			return ERR_ADC_INVALID_INT;
    }			
    return Successful;
}
INT32  
adc_installCallback(
	E_ADC_INT eIntType,
	PFN_ADC_CALLBACK pfnCallback,
	PFN_ADC_CALLBACK* pfnOldCallback
	)
{
	switch(eIntType)
    	{
    		case eADC_ADC_INT:
	    		*pfnOldCallback = g_psADCCallBack[eADC_ADC_INT];
	    		g_psADCCallBack[eADC_ADC_INT] = pfnCallback; 	                                      
	    	   	break; 
	   	case eADC_AUD_INT:
	   		*pfnOldCallback = g_psADCCallBack[eADC_AUD_INT];
    			g_psADCCallBack[eADC_AUD_INT] = pfnCallback; 	 
	   		break;
		case eADC_WT_INT:
			*pfnOldCallback = g_psADCCallBack[eADC_WT_INT];
    			g_psADCCallBack[eADC_WT_INT] = pfnCallback;  
			break;	
		default:
			return ERR_ADC_INVALID_INT;
    }			
    return Successful;
}

void ADC_SetAutoGainTiming(
	UINT32 u32Period,
	UINT32 u32Attack,
	UINT32 u32Recovery,
	UINT32 u32Hold
	)
{
	DrvADC_SetAutoGainTiming(u32Period,
						 	u32Attack,
							 u32Recovery,
							 u32Hold);
}	
void ADC_GetAutoGainTiming(
	PUINT32 pu32Period,
	PUINT32 pu32Attack,
	PUINT32 pu32Recovery,
	PUINT32 pu32Hold
	)
{  		
	DrvADC_GetAutoGainTiming(pu32Period,
							pu32Attack,
							pu32Recovery,
							pu32Hold)	;					     				     											     											     											     				
}  

/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         						*/
/* FUNCTION                                                                                                					*/
/*      DrvADC_SetClockSource()		 		                                                               		*/
/*                                                                                                         						*/
/* DESCRIPTION                                                                                             					*/
/*      To set the ADC clock source.                                                                       				*/
/*                                                                                                         						*/
/* INPUTS                                                                                                  					*/
/*      clkSrc   									        		                                       	*/
/*  E_DRVSYS_SYS_EXT = 0,														*/
/*	E_DRVSYS_SYS_X32K = 1,													*/
/*	E_DRVSYS_SYS_APLL = 2,													*/
/*	E_DRVSYS_SYS_UPLL = 3														*/
/*                                                                                                         						*/
/* OUTPUTS                                                                                                 					*/
/*      none                            				                                                   				*/
/*                                                                                                         						*/
/* RETURN                                                                                                  					*/
/*      none				                                                                               				*/
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/
void 
DrvADC_SetClockSource(
	UINT32 u32ClkSrc
	)
{
	UINT32 u32ClkSel = (inp32(REG_CLKDIV3) & (~ADC_S));	
	u32ClkSel = u32ClkSel | (u32ClkSrc << 14);

	outp32(REG_CLKDIV3, u32ClkSel);
}


/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         						*/
/* FUNCTION                                                                                                					*/
/*      DrvADC_GetVersion()		 		                                                                   		*/
/*                                                                                                         						*/
/* DESCRIPTION                                                                                             					*/
/*      To get the version number 								                                        */
/*                                                                                                         						*/
/* INPUTS                                                                                                  					*/
/*      none 														              		*/
/*                                                                                                         						*/
/* OUTPUTS                                                                                                 					*/
/*      none				                                                                               				*/
/*                                                                                                         						*/
/* RETURN                                                                                                  					*/
/*      the version number of ADC driver				                                                   		*/
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/
UINT32 
DrvADC_GetVersion(
	void
	)
{
	return ADC_VERSION_NUM;
}


/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         						*/
/* FUNCTION                                                                                                					*/
/*      ADC_IntHandler() 		                                                                       					*/
/*                                                                                                         						*/
/* DESCRIPTION                                                                                             					*/
/*      ADC Interrupt Service Routine 								                                       	*/
/*                                                                                                         						*/
/* INPUTS                                                                                                  					*/
/*      u32UserData     The user's parameter    					                                       		*/
/*                                                                                                         						*/
/* OUTPUTS                                                                                                 					*/
/*      none                    									                                       		*/
/*                                                                                                         						*/
/* RETURN                                                                                                  					*/
/*      none				                                                                               				*/
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/
void 
ADC_IntHandler(void)
{
	UINT32 u32Reg;

	u32Reg = inp32(REG_ADC_CON);

	/* Process ADC interrupt */
	u32Reg = inp32(REG_ADC_CON);
	if( (u32Reg & (ADC_INT_EN | ADC_INT)) == (ADC_INT_EN | ADC_INT))
	{//ADC interrupt
		if(g_psADCCallBack[eDRVADC_ADC_INT]!=0)
	        		g_psADCCallBack[eDRVADC_ADC_INT]();  
	    	/* Clean the ADC interrupt */     	
	    	outp32(REG_ADC_CON, (inp32(REG_ADC_CON) & 
	    						~WT_INT) |  
	    						ADC_INT);
			return;
	}        
	if( (u32Reg & (WT_INT_EN | WT_INT)) == (WT_INT_EN | WT_INT))
	{//Wait for trigger
			/*Disable WT int   */
			if((inp32(REG_ADC_CON) & ADC_TSC_MODE)==ADC_TSC_MODE)
	    			DrvADC_DisableInt(eDRVADC_WT_INT);	
	    	
			if(g_psADCCallBack[eDRVADC_WT_INT]!=0)
	        		g_psADCCallBack[eDRVADC_WT_INT]();
	    	/* Clean the touch panel interrupt */    
			outp32(REG_ADC_CON, (inp32(REG_ADC_CON) & 
	    					~ADC_INT) |  
	    					WT_INT);                
			return;
	}    
	u32Reg = inp32(REG_AUDIO_CON);
	if( (u32Reg & (AUDIO_INT_EN | AUDIO_INT)) == (AUDIO_INT_EN | AUDIO_INT))
	{//Audio record interrupt
			if(g_psADCCallBack[eDRVADC_AUD_INT]!=0)
	        		g_psADCCallBack[eDRVADC_AUD_INT]();    		
			/* Clean the record interrupt */
	    	outp32(REG_AUDIO_CON, inp32(REG_AUDIO_CON) | AUDIO_INT);
	}   
}


/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         						*/
/* FUNCTION                                                                                                					*/
/* 		DrvADC_StartRecord()		                                                                       			*/
/*                                                                                                         						*/
/* DESCRIPTION                                                                                             					*/
/*     	Start to record Audio data. This function only can be used in                                      		*/
/*      ADC_RECORD mode.                                                                                   				*/
/*                                                                                                         						*/
/* INPUTS                                                                                                  					*/
/*      none														                      	*/
/*                                                                                                         						*/
/* OUTPUTS                                                                                                 					*/
/*      none														                  	*/
/*                                                                                                         						*/
/* RETURN                                                                                                  					*/
/*      none				                                                                               				*/
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/

void 
DrvADC_StartRecord(
	E_ADC_RECORD_MODE eRecordMode
	)
{
    	/* Clean INT status for safe */
   	outp32(REG_AUDIO_CON,(inp32(REG_AUDIO_CON) & ~AUDIO_INT_MODE) |
    			   ((eRecordMode << 30) & AUDIO_INT_MODE));
	outp32(REG_AUDIO_CON, inp32(REG_AUDIO_CON) &
		    					~AUDIO_EN );
	outp32(REG_AUDIO_CON, inp32(REG_AUDIO_CON) | 
		    					(AUDIO_INT | AUDIO_EN) );
}


/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         						*/
/* FUNCTION                                                                                               					*/
/* 		DrvADC_StopRecord()                                                                               			*/
/*                                                                                                         						*/
/* DESCRIPTION                                                                                             					*/
/*     	Stop recording Audio data. This function only can be used in                                       		*/
/*      ADC_RECORD mode.                                                                                  		 		*/
/*                                                                                                         						*/
/* INPUTS                                                                                                  					*/
/*      none														                    	*/
/*                                                                                                         						*/
/* OUTPUTS                                                                                                 					*/
/*      none														                     	*/
/*                                                                                                         						*/
/* RETURN                                                                                                  					*/
/*      none				                                                                               				*/
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/
void 
DrvADC_StopRecord(
	void
	)
{
    /* Disable record function */
	outp32(REG_AUDIO_CON, inp32(REG_AUDIO_CON) & (~AUDIO_EN));
	outp32(REG_AUDIO_CON, inp32(REG_AUDIO_CON) & 
		    					~(AUDIO_INT | AUDIO_EN | AUDIO_INT_EN) );
}


/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         						*/
/* FUNCTION                                                                                                					*/
/* 		DrvADC_StartConvert()		                                                                       			*/
/*                                                                                                         						*/
/* DESCRIPTION                                                                                             					*/
/*     	Start to convert ADC data. This function only can be used in                                       	*/
/*      ADC_NORMAL mode.                                                                                   				*/
/*                                                                                                         						*/
/* INPUTS                                                                                                  					*/
/*      u32Channel: The analog input channel and it could be ch2~ch7.                                      		*/
/*                                                                                                         						*/
/* OUTPUTS                                                                                                 					*/
/*      none														        		*/
/*                                                                                                         						*/
/* RETURN                                                                                                  					*/
/*      none				                                                                               				*/
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/
void 
DrvADC_StartConvert(
	UINT32 u32Channel
	)
{
    if(u32Channel <= 7)
    {
        /* Clean INT flag for safe and trigger start */
	    outp32(REG_ADC_CON, (inp32(REG_ADC_CON)|ADC_CONV) | 
	    				((u32Channel << 9)| ADC_INT) );
    }
}


/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         						*/
/* FUNCTION                                                                                                					*/
/* 		DrvADC_IsConvertReady()	                                                                           		*/
/*                                                                                                         						*/
/* DESCRIPTION                                                                                             					*/
/*     	check if ADC (not audio) is converted OK       			    	                                   	*/
/*                                                                                                         						*/
/* INPUTS                                                                                                  					*/
/*      none														                        */
/*                                                                                                         						*/
/* OUTPUTS                                                                                                 					*/
/*      none 														                       	*/
/*                                                                                                         						*/
/* RETURN                                                                                                  					*/
/*      TURE : Conversion finished		    						                                       	*/
/*      FALSE: Under converting         							                                       		*/
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/
BOOL 
DrvADC_IsConvertReady(
	void
	)
{
	return ((inp32(REG_ADC_CON)&ADC_FINISH)? TRUE:FALSE);		//Check finished?	
}					

BOOL 
DrvADC_Polling_ADCIntStatus(
	void
	)
{
	return ((inp32(REG_ADC_CON)&ADC_INT)? TRUE:FALSE);			//Check Int?
}
/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                        	 					*/
/* FUNCTION                                                                                                					*/
/* 		DrvADC_GetConvertData()	                                                                           		*/
/*                                                                                                         						*/
/* DESCRIPTION                                                                                             					*/
/*     	Get the converted ADC value in ADC_NORMAL mode    				                       	*/
/*                                                                                                         						*/
/* INPUTS                                                                                                  					*/
/*      none												                                       	*/
/*                                                                                                         						*/
/* OUTPUTS                                                                                                 					*/
/*      none 														                        */
/*                                                                                                         						*/
/* RETURN                                                                                                  					*/
/*      The ADC value            		    						                                       		*/
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/
UINT32 
DrvADC_GetConvertData(
	void
	)
{
    return (inp32(REG_ADC_XDATA));
}


/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         						*/
/* FUNCTION                                                                                                					*/
/* 		DrvADC_GetRecordData()	                                                                           			*/
/*                                                                                                         						*/
/* DESCRIPTION                                                                                            					*/
/*     	Get the converted ADC value in ADC_RECORD mode    				                	*/
/*                                                                                                         						*/
/* INPUTS                                                                                                  					*/
/*      none														                        */
/*                                                                                                         						*/
/* OUTPUTS                                                                                                 					*/
/*      none 														                        */
/*                                                                                                         						*/
/* RETURN                                                                                                  					*/
/*      The pointer to the record data. The data length is 8 samples                                        		*/
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/
PINT16 
DrvADC_GetRecordData(
	void
	)
{
    /* Return the base address of converted data. There are 8 samples */
    return (PINT16)REG_AUDIO_BUF0;
}


void DrvADC_FineTuneClock(
	UINT8 	u8AdcClock
	)
{  			
	outp32(REG_ADC_CON, (inp32(REG_ADC_CON)& ~AUDCLKDIV) | 
			(u8AdcClock<<1));				     				     											     											     											     				
	u32AudioCycle = 	(u8AdcClock+1);	
}
/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         						*/
/* FUNCTION                                                                                                					*/
/* 		DrvADC_SetSamplingRate()	                                                                           		*/
/*                                                                                                         						*/
/* DESCRIPTION                                                                                            					*/
/*     	There are 4 parameters deteminate the sample rate. 								*/
/* 1. downsample =8/16. 2 Cycle. 3 divider. PLL										*/
/*                                                                                                         						*/
/* INPUTS                                                                                                  					*/
/*      none														                        */
/*                                                                                                         						*/
/* OUTPUTS                                                                                                 					*/
/*      none 														                        */
/*                                                                                                         						*/
/* RETURN                                                                                                  					*/
/*      pu32Cycle                                        											*/
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/
void DrvADC_SetSamplingRate(E_SYS_SRC_CLK eSrcClock,
						UINT32 u32SamplingRate,
						UINT32* pu32Cycle)
{
	UINT32 u32APll, u32ExtHz;
	UINT32 u32Div, u32Cycle, u32DownSample;
	float deviation;
	
	u32ExtHz = sysGetExternalClock();
	u32APll = sysGetPLLOutputHz(eSrcClock, u32ExtHz);
	DBG_PRINTF(" PLL  = %d Hz\n", u32APll);	
	DBG_PRINTF("%d\n", u32SamplingRate);
	for(u32Cycle=15; u32Cycle<=128;u32Cycle=u32Cycle+1)
	{
		if(u32Cycle==16)
			continue;
		if(u32SamplingRate>=11025)
		{
			u32DownSample = 8;
		}
		else
		{
			u32DownSample = 16;		
		}
		u32Div = u32APll/u32DownSample/u32Cycle/u32SamplingRate;
		if(((u32APll/u32DownSample/u32Cycle)%u32SamplingRate)== 0)
		{//Integrate 
			if(u32APll/u32Div<= 3000000)
			{
				DBG_PRINTF("Down sample = %d\n", u32DownSample);
				DBG_PRINTF("u32Cycle = %d\n", u32Cycle);
				DBG_PRINTF("u32Div = %d\n", u32Div);
				*pu32Cycle = u32Cycle;				
				break;
			}	
		}
		else
		{//Float			
			if((u32SamplingRate==11025) || (u32SamplingRate==22050))
			{
				UINT32 u32RealSampleRate;			
				if(u32APll/u32Div<= 3000000)
				{
					u32RealSampleRate = u32APll/u32DownSample/u32Cycle/u32Div;
					deviation = (float)(abs(u32SamplingRate-u32RealSampleRate))/u32RealSampleRate;
					if(deviation<0.0015)
					{
						DBG_PRINTF("u32Cycle = %d\n", u32Cycle);
						DBG_PRINTF("u32Div = %d\n", u32Div);
						DBG_PRINTF("deviation = %f\n\n", deviation);
						*pu32Cycle = u32Cycle;
						//break;
					}
				}
			}
		}
	}
}


INT32 
DrvADC_AUD_Open(
	E_ADC_MODE eADC_Mode,
	UINT32 u32SrcClock,
	UINT32 u32ConvClock	//HZ
	)
{
	UINT32 u32ExtHz;
	UINT32 u32PLLClockHz;
	UINT32 u32Tmp;	
	volatile UINT32 u32Delay, u32CalBri;
	UINT32 u32Reg;
	
	if((eADC_Mode != eADC_NORMAL) && (eADC_Mode != eADC_RECORD))
	{
	    return ERR_ADC_ARGUMENT;
	}       
	/* Commen settings */
	outp32(REG_APBCLK,inp32(REG_APBCLK)|ADC_CKE);
	if(u32ADCReset==0)
		adc_reset();
		
	
	/* Default to use conv bit to control conversion */
	u32Reg = 0;
	/* Enable ADC */
	u32Reg = u32Reg | (AUDADC_EN); 			
	/* Use the same clock source as system */
	outp32(REG_CLKDIV3, (inp32(REG_CLKDIV3) & ~ADC_S) | 
					(u32SrcClock << 19));
	outp32(REG_AUDADC_CTL, inp32(REG_AUDADC_CTL) | APB2AHB);									
	switch(u32SrcClock)
	{
		case eADC_X32K:	
					return ERR_ADC_CLOCK;			/* Wrong clock source */			
					break;
		case eADC_APLL:												
		case eADC_UPLL:			
					{
						UINT32 u32TotalDiv;
						UINT32 u32IdxN0, u32IdxN1; 
						UINT32 u32IdxN00, u32IdxN11; 
						//DrvSYS_GetPLLClock(eSrcClock,
						//						&u32PLLClockKHz);
						u32ExtHz = sysGetExternalClock();
						u32PLLClockHz = sysGetPLLOutputHz(u32SrcClock, u32ExtHz);
						DBG_PRINTF("PLL=%d,  clock = %d Hz\n", u32SrcClock, u32PLLClockHz);
																		
						if(eADC_Mode == eADC_RECORD)
						{
							if(u32ConvClock>=11025)					
								u32TotalDiv = (u32PLLClockHz)/(u32AudioCycle*8*u32ConvClock);		/* 8 samples */
							else
								u32TotalDiv = (u32PLLClockHz)/(u32AudioCycle*16*u32ConvClock);	/* 16 samples */	
						}		
						else
							u32TotalDiv = (u32PLLClockHz/(1000*50))/u32ConvClock;	
						DBG_PRINTF("Total Divider = %d\n", u32TotalDiv);
						if(u32TotalDiv>(8*256))
						{
							DBG_PRINTF("Error, Total divider = 0x%x\n", u32TotalDiv);
							return ERR_ADC_CLOCK;						
						}
																		
						DBG_PRINTF("Total divider = 0x%x\n", u32TotalDiv);						
						for(u32IdxN0=1;u32IdxN0 <= 8;u32IdxN0=u32IdxN0+1)	//BUG, From2 cause. 8 bit N1 dividers issue 							
						{
							for(u32IdxN1=1;u32IdxN1<=256;u32IdxN1=u32IdxN1+1)								
							{//u32IdxN0 != 1
								if(u32TotalDiv==(u32IdxN0*u32IdxN1))
								{
									u32IdxN00 = u32IdxN0;
									u32IdxN11 = u32IdxN1;
									DBG_PRINTF("2. ADC_N0 = %d, ADC_N1= %d\n", u32IdxN0, u32IdxN1);
									break; 
								}	
							}							
							if(u32TotalDiv==((u32IdxN00)*u32IdxN11))	
							{
								DBG_PRINTF("2. ADC_N0 = %d, ADC_N1= %d\n", u32IdxN00, u32IdxN11);							
								break;
							}		
						}	
						DBG_PRINTF("3. ADC_N0 = %d, ADC_N1= %d\n", u32IdxN00, u32IdxN11);
						u32Tmp = (inp32(REG_CLKDIV3) & ~(ADC_N1 | ADC_S | ADC_N0)) | 
										( (((u32IdxN11-1) <<24) | ((u32IdxN00-1) << 16) | (u32SrcClock<<19) ));
									
						DBG_PRINTF("Tmp = 0x%x\n", u32Tmp);
						outp32(REG_CLKDIV3, u32Tmp);																					
					}					
					break;
		case eADC_EXT:	
					{
						UINT32 u32ExtClk, u32AdcDivN1;
						u32ExtClk = sysGetExternalClock();										
						u32AdcDivN1 = (u32ExtClk)/u32ConvClock;	
						if(u32AdcDivN1>256)
							return ERR_ADC_CLOCK;
						outp32(REG_CLKDIV3, (inp32(REG_CLKDIV3) & ~(ADC_N1 | ADC_N0)) |
										((u32AdcDivN1-1) <<24) );																													
					}									
					break;
	}	
	if(u32ConvClock>=11025)
	{/* Sample rate >=11025. 11025/12K/16K sample rate use upsamle 8X. 8 ADC samples ==> 1 samples */
		outp32(REG_AGCP1, inp32(REG_AGCP1)| DOWNSMPSEL ); 
	}
	else
	{/* Sample rate <11025 .  sample rate use upsample 16X. 16 ADC samples ==> 1 samples */
		outp32(REG_AGCP1, inp32(REG_AGCP1)& ~DOWNSMPSEL);
	}																		
    	outp32(REG_ADC_CON, u32Reg);    
	outp32(REG_ADC_CON, (inp32(REG_ADC_CON)& ~AUDCLKDIV) | 
							((u32AudioCycle-1)<<1));				
    	g_u8IsRecord = 0;
        	  	         
       	/* Reset Record Function */       
        outp32(REG_AUDADC_CTL, (inp32(REG_AUDADC_CTL)& ~AUD_STRTIME)| (0x01<<16));
        outp32(REG_AUDIO_CON, inp32(REG_AUDIO_CON) & ~ AUDIO_CCYCLE);		//Choice 12 bit ADC
        outp32(REG_AUDIO_CON, inp32(REG_AUDIO_CON) | AUDIO_RESET);			//Choice 12 bit ADC
        outp32(REG_AUDIO_CON, inp32(REG_AUDIO_CON) & ~(AUDIO_RESET));
        
        /* Default to Audio Interrupt Mode 3, op offset:b'1000, interrupt enabled */             
        outp32(REG_AUDIO_CON, (AUDIO_HPEN | AUDIO_INT |AUDIO_VOL_EN | AUDIO_RESET));
        sysDelay(1);

	outp32(REG_AUDIO_CON, (inp32(REG_AUDIO_CON) & ~(AUDIO_CCYCLE | AUDIO_RESET)) | (0x2<<16));//2 samples as 0 
	if( u32ADCReset==1)
    	{	     
	        /* Hardware offset calibration */
	        outp32(REG_AUDIO_CON, inp32(REG_AUDIO_CON) & ~AUDIO_VOL_EN);
	        sysDelay(1);    /* Must !!!! */		
	        outp32(REG_AUDIO_CON, inp32(REG_AUDIO_CON) | AUDIO_VOL_EN);
	        sysDelay(12);    /* Must !!!! */		
	    	for(u32Delay=0; u32Delay<3; u32Delay=u32Delay+1)
	    	{   
	    		
	        	outp32(REG_AUDIO_CON, inp32(REG_AUDIO_CON) | 
			    			 ( AUDIO_INT | AUDIO_EN ) );
			sysDelay(1);    /* Must !!!! */			 
			DrvADC_SetOffsetCancellationEx(1,	//512 sample
											192);//Delay sample count       			 
			DrvADC_SetOffsetCancellation(FALSE,   //BOOL bIsMuteEnable, It is not need if hardware calibration mode
									0, //BOOL bIsOffsetCalibration,
									0, //BOOL bIsHardwareMode,
									0x10);//UINT32 u32Offset. It is useless in hardware calibration.    
			    			 
	        	DrvADC_SetOffsetCancellation(FALSE,   //BOOL bIsMuteEnable, It is not need if hardware calibration mode
									TRUE, //BOOL bIsOffsetCalibration,
									TRUE, //BOOL bIsHardwareMode,
									0x10);//UINT32 u32Offset. It is useless in hardware calibration.   	  		   				  		   										
			DBG_PRINTF("Hardware offset compension\n");
			while((inp32(REG_STATEFLAG)&OP_MUTE_FLAG)==OP_MUTE_FLAG);	//Wait until hardware calibration finish
			sysDelay(1);    /* Must !!!! */	
			outp32(REG_AUDIO_CON, inp32(REG_AUDIO_CON) & ~AUDIO_EN);
			u32CalBri=  inp32(REG_OPOCCALSUM);
			DBG_PRINTF("Register  = 0x%x\n", u32CalBri);
		}					
				
		/* if reported calibration over 0x7F. */
		/* It mean reported calibration value will map to 
			0 ~ -0x7F ==> hardware will auto calibration to + offset
			-0x80 ~ -0x1FF ==> overflow (Negative MAX). programmer should siwtch the calibrate value to software mode and calibration value equal to +32mv. 		
		*/
		u32CalBri=  inp32(REG_OPOCCALSUM);
		DBG_PRINTF("Register  = 0x%x\n", u32CalBri);
		u32CalBri = u32CalBri & CAL_SUM;
		DBG_PRINTF("u32CalBri = 0x%x\n", u32CalBri);
		
		switch((u32CalBri>>7))
		{
			case 4:
			case 5:
			case 6:
				DBG_PRINTF("Adjust and switch to software calibration mode\n");
				outp32(REG_OPOC, 0x0002000C);
				
		}	          					
        	g_u8IsRecord = 1;
    	}
    	
    	sysInstallISR(IRQ_LEVEL_1, 
					IRQ_ADC, 
					(PVOID)ADC_IntHandler);
	sysEnableInterrupt(IRQ_ADC);	
	if(bIsTSEnable == FALSE)
	{
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
	bIsAUDEnable = TRUE;
	return Successful;	
}


/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         						*/
/* FUNCTION                                                                                                					*/
/*      DrvADC_Close()			                                                                           			*/
/*                                                                                                         						*/
/* DESCRIPTION                                                                                             					*/
/*     	close ADC								 					                       	*/
/*                                                                                                         						*/
/* INPUTS                                                                                                  					*/
/*      none														                      	*/
/*                                                                                                         						*/
/* OUTPUTS                                                                                                 					*/
/*      none														                      	*/
/*                                                                                                         						*/
/* RETURN                                                                                                  					*/
/*      none				                                                                               				*/
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/
void 
DrvADC_Close(
	void
	)
{     
    while(!DrvADC_IsConvertReady());
    adc_reset();
    sysDisableInterrupt(IRQ_ADC);
    outp32(REG_ADC_CON, inp32(REG_ADC_CON)&~ADC_CON_ADC_EN);
    outp32(REG_APBCLK,inp32(REG_APBCLK)&~ADC_CKE);

}



void DrvADC_EnableHighPassFilter(
	BOOL bIsEnableFilter
	)
{
	UINT32 u32RegData = inp32(REG_AUDIO_CON);
	bIsEnableFilter?(u32RegData=u32RegData|AUDIO_HPEN):(u32RegData=u32RegData&(~AUDIO_HPEN));
	outp32(REG_AUDIO_CON, u32RegData);
}	

BOOL DrvADC_IsEnableHighPassFilter(void)
{
	return ((inp32(REG_AUDIO_CON) & AUDIO_HPEN)>>26);
}
/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         						*/
/* FUNCTION                                                                                                					*/
/* 		DrvADC_EnableInt()		                                   	   	                                   		*/
/*                                                                                                         						*/
/* DESCRIPTION                                                                                            					*/
/*     	enable ADC interrupt and setup callback function 	        	                                   	*/
/*                                                                                                         						*/
/* INPUTS                                                                                                  					*/
/*      callback                                                                                           					*/
/*          The callback funciton                                                                          					*/
/*                                                                                                         						*/
/* OUTPUTS                                                                                                 					*/
/*      none 														                        */
/*                                                                                                         						*/
/* RETURN                                                                                                 	 				*/
/*      none														                        */
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/
INT32 
DrvADC_EnableInt(
	E_DRVADC_INT eIntType
	)
{
    /* Enable adc interrupt */
    switch(eIntType)
    {
    	case eDRVADC_ADC_INT: 
		 	outp32(REG_ADC_CON, inp32(REG_ADC_CON) | ADC_INT_EN);	    		                                     
    	    break; 
	   	case eDRVADC_AUD_INT:
	   		outp32(REG_AUDIO_CON, inp32(REG_AUDIO_CON) | AUDIO_INT_EN);	 
	   		break;
		case eDRVADC_WT_INT:     
			outp32(REG_ADC_CON, inp32(REG_ADC_CON) | WT_INT_EN);	 
			break;	
		default:
			return ERR_ADC_INVALID_INT;
    }			
    return Successful;
}

INT32 
DrvADC_DisableInt(
	E_DRVADC_INT eIntType
	)
{
    /* Enable adc interrupt */
    switch(eIntType)
    {
    	case eDRVADC_ADC_INT:
    		outp32(REG_ADC_CON, inp32(REG_ADC_CON) & ~ADC_INT_EN);	                                      
    	    break; 
	   	case eDRVADC_AUD_INT:
	   		outp32(REG_AUDIO_CON, inp32(REG_AUDIO_CON) & ~AUDIO_INT_EN);	 
	   		break;
		case eDRVADC_WT_INT:
			outp32(REG_ADC_CON, inp32(REG_ADC_CON) & ~WT_INT_EN);     
			break;	
		default:
			return ERR_ADC_INVALID_INT;
    }			
    return Successful;
}

INT32 
DrvADC_ClearInt(
	E_DRVADC_INT eIntType
	)
{
    switch(eIntType)
    {
    	case eDRVADC_ADC_INT: 
		 	outp32(REG_ADC_CON, (inp32(REG_ADC_CON) & ~(ADC_INT|WT_INT)) | 
		 					ADC_INT);	    		                                     
    	    break; 
	   	case eDRVADC_AUD_INT:
	   		outp32(REG_AUDIO_CON, inp32(REG_AUDIO_CON) | AUDIO_INT);	 
	   		break;
		case eDRVADC_WT_INT:     
			outp32(REG_ADC_CON, (inp32(REG_ADC_CON) & ~(ADC_INT|WT_INT)) |  
							WT_INT);	 
			break;	
		default:
			return ERR_ADC_INVALID_INT;	
    }			
   	return Successful;    	
}	

BOOL 
DrvADC_PollInt(
	E_DRVADC_INT eIntType
	)
{
   	UINT32 u32IntSt = 0;
    switch(eIntType)
    {
    	case eDRVADC_ADC_INT: 
		 	u32IntSt = inp32(REG_ADC_CON) & ADC_INT;	    		                                     
    	    break; 
	   	case eDRVADC_AUD_INT:
	   		u32IntSt = inp32(REG_AUDIO_CON) & AUDIO_INT;	 
	   		break;
		case eDRVADC_WT_INT:     
			u32IntSt = inp32(REG_ADC_CON) & WT_INT;	 
			break;	
    }			
    if( u32IntSt != 0)
    	return TRUE;
	else
    	return FALSE;		    	
}


INT32 
DrvADC_InstallCallback(
	E_DRVADC_INT eIntType,
	PFN_DRVADC_CALLBACK pfnCallback,
	PFN_DRVADC_CALLBACK* pfnOldCallback
	)
{
	switch(eIntType)
    {
    	case eDRVADC_ADC_INT:
    		*pfnOldCallback = g_psADCCallBack[eDRVADC_ADC_INT];
    		g_psADCCallBack[eDRVADC_ADC_INT] = pfnCallback; 	                                      
    	    break; 
	   	case eDRVADC_AUD_INT:
	   		*pfnOldCallback = g_psADCCallBack[eDRVADC_AUD_INT];
    		g_psADCCallBack[eDRVADC_AUD_INT] = pfnCallback; 	 
	   		break;
		case eDRVADC_WT_INT:
			*pfnOldCallback = g_psADCCallBack[eDRVADC_WT_INT];
    		g_psADCCallBack[eDRVADC_WT_INT] = pfnCallback;  
			break;	
		default:
			return ERR_ADC_INVALID_INT;
    }			
    return Successful;
}


/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         						*/
/* FUNCTION                                                                                                					*/
/* 		DrvADC_IsAudioDataReady()	                                                                       			*/
/*                                                                                                         						*/
/* DESCRIPTION                                                                                             					*/
/*     	Check if the recording data is converted OK.        			                                   	*/
/*                                                                                                         						*/
/* INPUTS                                                                                                  					*/
/*      none														                        */
/*                                                                                                         						*/
/* OUTPUTS                                                                                                					*/
/*      none 														                        */
/*                                                                                                         						*/
/* RETURN                                                                                                  					*/
/*      TURE: data is ready											                                */
/*      FALSE: data is not ready									                                       	*/
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/
BOOL 
DrvADC_GetRecordReadyFlag(
	void
	)
{
	return ((inp32(REG_AUDIO_CON)&AUDIO_INT)? TRUE:FALSE);
}	


/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         						*/
/* FUNCTION                                                                                                					*/
/* 		DrvADC_ClearRecordReadyFlag()	                                                                 	  	*/
/*                                                                                                         						*/
/* DESCRIPTION                                                                                             					*/
/*     	To clear the recording ready flag.       	        			                                   	*/
/*                                                                                                         						*/
/* INPUTS                                                                                                  					*/
/*      None														                       	*/
/*                                                                                                         						*/
/* OUTPUTS                                                                                                 					*/
/*      None 														                	*/
/*                                                                                                         						*/
/* RETURN                                                                                                  					*/
/*      None	                									                                       	*/
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/
void DrvADC_ClearRecordReadyFlag(
	void
	)
{
    outp32(REG_AUDIO_CON, inp32(REG_AUDIO_CON) & ~AUDIO_INT);
    outp32(REG_AUDIO_CON, inp32(REG_AUDIO_CON) | AUDIO_INT);
}	


/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         						*/
/* FUNCTION                                                                                                					*/
/* 		DrvADC_SetMICGain()	                      		                                                   		*/
/*                                                                                                         						*/
/* DESCRIPTION                                                                                             					*/
/*     	Set record volume gain       									                        */
/*                                                                                                         						*/
/* INPUTS                                                                                                  					*/
/*      u16MicGainLevel						    					                                */
/*          The volume gain could be 0 ~ 31 dB.                                                            			*/
/*                                                                                                         						*/
/* OUTPUTS                                                                                                 					*/
/*      none 														                        */
/*                                                                                                         						*/
/* RETURN                                                                                                  					*/
/*      none														                       	*/
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/
void DrvADC_SetMICGain(
	UINT16 u16MicGainLevel
	)
{
	outp32(REG_AGCP1, (inp32(REG_AGCP1)&(~AUDIO_VOL)) |
	       (u16MicGainLevel & AUDIO_VOL));

	outp32(REG_AUDIO_CON, inp32(REG_AUDIO_CON) | AUDIO_VOL_EN);       
}	


/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         						*/
/* FUNCTION                                                                                                					*/
/* 		DrvADC_GetMICGain()	                      		                                                   		*/
/*                                                                                                         						*/
/* DESCRIPTION                                                                                             					*/
/*     	Get record volume gain.       									                        */
/*                                                                                                         						*/
/* INPUTS                                                                                                 			 		*/
/*      None        						    					                                       	*/
/*                                                                                                         						*/
/* OUTPUTS                                                                                                 					*/
/*      none 														                       	*/
/*                                                                                                         						*/
/* RETURN                                                                                                  					*/
/*      Recording gain in dB.										                                */
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/
INT16 DrvADC_GetMICGain(
	void
	)
{
    return (inp32(REG_AGCP1) & AUDIO_VOL);
}	

/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         						*/
/* FUNCTION                                                                                                					*/
/* 		DrvADC_SetGainControl()	                      		                                               			*/
/*                                                                                                         						*/
/* DESCRIPTION                                                                                             					*/
/*     	Set Pre-Amplifer and Post-Amplifer					                   				*/
/*                                                                                                         						*/
/* INPUTS                                                                                                  					*/
/*      None        						    					                                       	*/
/*                                                                                                         						*/
/* OUTPUTS                                                                                                 					*/
/*      none 														                  	*/
/*                                                                                                         						*/
/* RETURN                                                                                                  					*/
/*      Recording gain in dB.										                                */
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/
void DrvADC_SetGainControl(
	UINT32 u32PreGain, 
	UINT32 u32PostGain
	)
{
#ifdef TC5577_SPI
	 UINT8 u8SpiReg; 
	 spi_5577_SetSelect(0); //For Audion recording chip select
	 u8SpiReg = spi_5577_ReadByte(0x08)&(~0x6);
	 u8SpiReg = u8SpiReg | (u32PreGain<<1);
	 spi_5577_WriteByte(0x08, u8SpiReg);
	 u8SpiReg = (spi_5577_ReadByte(0x08)>>1)&0x03;	
	 if(u8SpiReg!=u32PreGain)
	 {
	 	DBG_PRINTF("SPI write fail\n");
	 	while(1);
	 }
	 outp32(REG_AGCP1, (inp32(REG_AGCP1) & ~(AUDIO_VOL|PRAGA)) | 
     				((u32PreGain<<8)|u32PostGain)); 
#else
     outp32(REG_AGCP1, (inp32(REG_AGCP1) & ~(AUDIO_VOL|PRAGA)) | 
     				((u32PreGain<<8)|u32PostGain)); 								     				
#endif     				
}	

void DrvADC_GetGainControl(
	UINT32* u32PreGain, 
	UINT32* u32PostGain
	)
{
	UINT32 u32RegData = inp32(REG_AGCP1);
	*u32PreGain =  (u32RegData & PRAGA)>>8;
	*u32PostGain = u32RegData & AUDIO_VOL;							     				
}	
/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         						*/
/* FUNCTION                                                                                                					*/
/* 		DrvADC_SetOffsetCancellation()	                      		                                      		*/
/*                                                                                                         						*/
/* DESCRIPTION                                                                                             					*/
/*      The function is only for OP offset callcellation				    						*/
/*                                                                                                         						*/
/* INPUTS                                                                                                  					*/
/*      None        						    					                                       	*/
/*                                                                                                         						*/
/* OUTPUTS                                                                                                 					*/
/*      none 														                     	*/
/*                                                                                                         						*/
/* RETURN                                                                                                  					*/
/*      Recording gain in dB.										                                */
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/
void DrvADC_SetOffsetCancellation(
	BOOL bIsMuteEnable,
	BOOL bIsOffsetCalibration,
	BOOL bIsHardwareMode,
	UINT32 u32Offset
	)
{
	outp32(REG_OPOC, (inp32(REG_OPOC) & ~(MUTE_SW | OOC | OPOCM |OPOC_SW)) |
				 (((bIsMuteEnable ? MUTE_SW:0) |
				 (bIsOffsetCalibration ? OOC:0)) |
				 (bIsHardwareMode ? OPOCM:0)) 
				  );
	if(bIsHardwareMode!=TRUE)
	{//Software mode setting. 
		outp32(REG_OPOC, (inp32(REG_OPOC)&(~OPOC_SW))|			  
		((u32Offset<<24)&OPOC_SW));			  
	}	
}	

void DrvADC_GetOffsetCancellation(
	PBOOL pbIsMuteEnable,
	PBOOL pbIsOffsetCalibration,
	PBOOL pbIsHardwareMode,
	PUINT32 pu32Offset
	)
{
	UINT32 u32RegData = inp32(REG_OPOC);
	*pbIsMuteEnable = (u32RegData & MUTE_SW)>>31;
	*pbIsOffsetCalibration = (u32RegData & OOC)>>30;
	*pbIsHardwareMode = (u32RegData & OPOCM)>>29;
	*pu32Offset = (u32RegData & OPOC_SW)>>24;
}	

void DrvADC_SetOffsetCancellationEx(
	UINT32 u32SampleNumber,
	UINT32 u32DelaySampleCount
	)
{
	outp32(REG_OPOC, (inp32(REG_OPOC) & ~(OPOC_TCSN | OPOC_DSC)) |
				 (((u32SampleNumber<<16) & OPOC_TCSN) |
				 (u32DelaySampleCount & OPOC_DSC)) );
}
void DrvADC_GetOffsetCancellationEx(
	PUINT32 pu32SampleNumber,
	PUINT32 pu32DelaySampleCount
	)
{
	UINT32 u32RegData = inp32(REG_OPOC);	
	*pu32SampleNumber = (u32RegData & OPOC_TCSN)>>16;
	*pu32DelaySampleCount = (u32RegData & OPOC_DSC);
}
/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         						*/
/* FUNCTION                                                                                                					*/
/* 		DrvADC_SetNoiseGate()	                      		                                                   		*/
/*                                                                                                         						*/
/* DESCRIPTION                                                                                             					*/
/*     	Set Pre-Amplifer, Post-Amplifer and offset(Offset Cancellation					       	*/
/*                                                                                                         						*/
/* INPUTS                                                                                                  					*/
/*      None        						    					                                       	*/
/*                                                                                                         						*/
/* OUTPUTS                                                                                                 					*/
/*      none 														                        */
/*                                                                                                         						*/
/* RETURN                                                                                                  					*/
/*      Noise gate Level gain in -24, -30, -36, -42dB.								               	*/
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/
void DrvADC_SetNoiseGate(
	BOOL bIsEnable, 
	UINT32 u32NoiseGateLevel,
	UINT32 u32NoiseGain		//If signal less the specified level, noise gain will replace the AGC target level
	)
{
	UINT32 u32RegData;
    outp32(REG_AGC_CON, (inp32(REG_AGC_CON) & ~(NG_EN)) | 
     				((bIsEnable <<31)& NG_EN) );
	u32RegData = inp32(REG_NG_CTRL)& ~(NG_GAIN | NG_LEVEL);	
	outp32(REG_NG_CTRL, u32RegData | (((u32NoiseGain << 8)& NG_GAIN) | (u32NoiseGateLevel & NG_LEVEL)) );     													     				    				
}	

/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         						*/
/* FUNCTION                                                                                                					*/
/* 		DrvADC_GetNoiseGate()	                      		                                                   		*/
/*                                                                                                         						*/
/* DESCRIPTION                                                                                             					*/
/*     	Set Pre-Amplifer, Post-Amplifer and offset(Offset Cancellation				                */
/*                                                                                                         						*/
/* INPUTS                                                                                                  					*/
/*      None        						    					                                       	*/
/*                                                                                                         						*/
/* OUTPUTS                                                                                                 					*/
/*      none 														                       	*/
/*                                                                                                         						*/
/* RETURN                                                                                                  					*/
/*      Noise gate Level gain in -24, -30, -36, -42dB.								               	*/
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/
void DrvADC_GetNoiseGate(
	PBOOL pbIsEnable, 
	UINT32* pu32NoiseGateLevel,
	UINT32* pu32NoiseGain	
	)
{
	UINT32 u32RegData = inp32(REG_AGC_CON);
	*pbIsEnable = (u32RegData & NG_EN)>>31;
	u32RegData = inp32(REG_NG_CTRL);
	*pu32NoiseGateLevel = u32RegData & NG_LEVEL;
    	*pu32NoiseGain = (u32RegData & NG_GAIN) >> 8;						     				
}	

/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         						*/
/* 			                                                                                               		 			*/										             
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/
void DrvADC_SetNoiseGateThreshold(
	UINT32 u32GainStep, 
	UINT32 u32InSample,
	UINT32 u32OutSample		//If signal less the specified level, noise gain will replace the AGC target level
	)
{
    outp32(REG_NG_CTRL, (inp32(REG_NG_CTRL) & ~(GAINCHGMOD | IN_NG_TIME | OUT_NG_TIME)) | 
     				((((u32GainStep <<24)& GAINCHGMOD)|
     				((u32InSample <<20)& IN_NG_TIME))|
     				((u32OutSample <<16)& OUT_NG_TIME)));    													     				    				
}	
/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         						*/
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/
void DrvADC_GetNoiseGateThreshold(
	PUINT32 pu32GainStep, 
	PUINT32 pu32InSample,
	PUINT32 pu32OutSample	
	)
{
	UINT32 u32RegData = inp32(REG_NG_CTRL);
	*pu32GainStep = (u32RegData & GAINCHGMOD)>>24;
	*pu32InSample = (u32RegData & IN_NG_TIME)>>20;
    	*pu32OutSample = (u32RegData & OUT_NG_TIME)>>16;					     				
}	

/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         						*/
/* FUNCTION                                                                                                					*/
/* 		DrvADC_SetAutoGainControl()	                      		                                               		*/
/*                                                                                                         						*/
/* DESCRIPTION                                                                                             					*/
/*     	Set the parameter for AGC										                */
/*                                                                                                         						*/
/* INPUTS                                                                                                  					*/
/*      bIsEnable    	Enable AGC    						    					                */
/*      u32OutputLevel  Output target level      						    					*/
/*      eUpBand        	A band in the uper side from u32OutputLevel+-eUpBand					*/
/*      eDownBand       A band in the buttom side from u32OutputLevel+-eUpBand			       	*/
/*                                                                                                         						*/
/* OUTPUTS                                                                                                 					*/
/*      none 														                       	*/
/*                                                                                                         						*/
/* RETURN                                                                                                  					*/
/*      none															               	*/
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/
void DrvADC_SetAutoGainControl(
	BOOL bIsEnable, 
	UINT32 u32OutputLevel,
	E_DRVADC_UPBAND eAdcUpBand,
	E_DRVADC_DOWNBAND eAdcDownBand
	)
{
     outp32(REG_AGC_CON, (inp32(REG_AGC_CON) & ~AGC_EN) | 
     				((bIsEnable <<30)& AGC_EN) );     				
     outp32(REG_AGCP1, ( (inp32(REG_AGCP1) & ~(OTL | UPBAND | DOWNBAND)) | 
     				((u32OutputLevel<<12) & OTL) ) |
     				(((eAdcUpBand <<11)& UPBAND) | 
     				((eAdcDownBand <<10)& DOWNBAND)) );											     				     											     											     											     				
}	
	
/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         						*/
/* FUNCTION                                                                                                					*/
/* 		DrvADC_GetAutoGainControl()	                      		                                           		*/
/*                                                                                                         						*/
/* DESCRIPTION                                                                                             					*/
/*     	Set Pre-Amplifer, Post-Amplifer and offset(Offset Cancellation						*/
/*                                                                                                         						*/
/* INPUTS                                                                                                  					*/
/*      None        						    					                                       	*/
/* OUTPUTS                                                                                                 					*/
/*      bIsEnable    	Enable AGC    						    					               	*/
/*      u32OutputLevel  Output target level      						    					*/
/*      eUpBand        	A band in the uper side from u32OutputLevel+-eUpBand					*/
/*      eDownBand       A band in the buttom side from u32OutputLevel+-eUpBand				*/
/*                                                                                                         						*/
/* RETURN                                                                                                  					*/
/*      None.										               						*/
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/
void DrvADC_GetAutoGainControl(
	PBOOL pbIsEnable, 
	PUINT32 pu32OutputLevel,
	E_DRVADC_UPBAND* peAdcUpBand,
	E_DRVADC_DOWNBAND* peAdcDownBand
	)
{
	UINT32 u32RegData = inp32(REG_AGC_CON);
	*pbIsEnable = (u32RegData & AGC_EN)>>30;
	u32RegData = inp32(REG_AGCP1);
	*pu32OutputLevel = (u32RegData & OTL)>>12; 
    	*peAdcUpBand = 	(u32RegData & UPBAND)>>11; 					     				
   	*peAdcDownBand = (u32RegData & DOWNBAND)>>10; 						     				    						     						     				
}	
 
/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         						*/
/* FUNCTION                                                                                                					*/
/* 		DrvADC_SetClampingAGC()	                      		                                               		*/
/*                                                                                                         						*/
/* DESCRIPTION                                                                                             					*/
/*     	Set the parameter for AGC										                */
/*                                                                                                         						*/
/* INPUTS                                                                                                  					*/
/*      eAdcMaxClamp    Clamp AGC gain. The output level will be  					                */
/*						1. Input level + Max gain(db) if the value less OTL				*/
/*						2. OTL if input level + Max gain(db) great OTL					*/						         						    					           
/*      eAdcMinClamp    A band in the uper side from u32OutputLevel+-eUpBand					*/
/*                                                                                                         						*/
/* OUTPUTS                                                                                                 					*/
/*      none 														                        */
/*                                                                                                         						*/
/* RETURN                                                                                                  					*/
/*      none																	*/
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/
void DrvADC_SetClampingAGC(
	E_DRVADC_MAX_CLAMP eAdcMaxClamp,
	E_DRVADC_MIN_CLAMP eAdcMinClamp
	)
{		
     outp32(REG_AGCP1, (inp32(REG_AGCP1) & ~(MAXGAIN | MINGAIN )) | 
     				(((eAdcMaxClamp << 20) & MAXGAIN)  |
     				((eAdcMinClamp << 16) & MINGAIN)) );											     				     											     											     											     				
}	

void DrvADC_GetClampingAGC(
	E_DRVADC_MAX_CLAMP* peAdcMaxClamp,
	E_DRVADC_MIN_CLAMP* peAdcMinClamp
	)
{
	UINT32 u32RegData = inp32(REG_AGCP1);
	*peAdcMaxClamp = (u32RegData & MAXGAIN)>>20;
	*peAdcMinClamp = (u32RegData & MINGAIN)>>16;				     				    						     						     				
}	

/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         						*/
/* FUNCTION                                                                                                					*/
/* 		DrvADC_SetAutoGainTiming()	                      		                                           		*/
/*                                                                                                         						*/
/* DESCRIPTION                                                                                             					*/
/*     	Set the parameter for AGC												*/
/*                                                                                                         						*/
/* INPUTS                                                                                                  					*/
/*      u32Period    	Detect max peak in the how many samples    					               	*/
/*      u32Attack 		      						    					           		*/
/*      u32Recovery        	A band in the uper side from u32OutputLevel+-eUpBand				*/
/*      u32Hold       A band in the buttom side from u32OutputLevel+-eUpBand					*/
/*                                                                                                         						*/
/* OUTPUTS                                                                                                 					*/
/*      none 														                        */
/*                                                                                                         						*/
/* RETURN                                                                                                  					*/
/*      none																	*/
/*                                                                                                         						*/
/*---------------------------------------------------------------------------------------------------------*/
void DrvADC_SetAutoGainTiming(
	UINT32 u32Period,
	UINT32 u32Attack,
	UINT32 u32Recovery,
	UINT32 u32Hold
	)
{  			
     UINT32 u32Reg;
     if((u32Period%16)!=0)
     {//Period must be multiple of 16.
     	u32Period = (u32Period/16+1)*16;
     	if(u32Period>(1024-16))
     		u32Period = (1024-16);
     }
     u32Reg = ((inp32(REG_AGC_CON) & ~(PERIOD | ATTACK | RECOVERY| HOLD)) | 
     				(( ((u32Period<<16) & PERIOD) |
     				((u32Attack<<8) & ATTACK) ) |     				
     				( ((u32Recovery <<4)& RECOVERY) | 
     				(u32Hold & HOLD) )));     				
    outp32(REG_AGC_CON, u32Reg);															     				     											     											     											     				
}	
void DrvADC_GetAutoGainTiming(
	PUINT32 pu32Period,
	PUINT32 pu32Attack,
	PUINT32 pu32Recovery,
	PUINT32 pu32Hold
	)
{  		

	UINT32 u32RegData = inp32(REG_AGC_CON);		
	*pu32Period = (u32RegData & PERIOD) >> 16;
    	*pu32Attack = (u32RegData & ATTACK) >> 8;
    	*pu32Recovery = (u32RegData & RECOVERY) >> 4;
    	*pu32Hold = u32RegData & HOLD; 								     				     											     											     											     				
}   

void DrvADC_OverFlowCtl(
	BOOL	bIsEnable,
	UINT16 	u16OverFlowThreshold
	)
{  		
	outp32(REG_OVFLCTL, (inp32(REG_OVFLCTL)& ~(OVFL_THLD | OVFL_EN)) | 
			((u16OverFlowThreshold<<16) & OVFL_THLD) | 
			(bIsEnable & OVFL_EN));				     				     											     											     											     				
}
void DrvADC_MISC(
	BOOL	bIsEdmaEnable
	)
{  		
	outp32(REG_AGCP1, (inp32(REG_AGCP1)& ~(EDMA_MODE)) |  
			((bIsEdmaEnable <<31) & EDMA_MODE) );				     				     											     											     											     				
}
