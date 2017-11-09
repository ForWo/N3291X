/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright (c) Nuvoton Technology Corp. All rights reserved.                                             */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef __ADC_H__
#define __ADC_H__
#include "w55fa95_reg.h"
#include "wberrcode.h"
/*---------------------------------------------------------------------------------------------------------*/
/* Includes of system headers                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
#ifdef  __cplusplus
extern "C"
{
#endif
/*---------------------------------------------------------------------------------------------------------*/
/* Macro, type and constant definitions                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
/* version define with SysInfra */
#define	ADC_MAJOR_NUM 3
#define	ADC_MINOR_NUM 60
#define	ADC_BUILD_NUM 001
#define 	ADC_VERSION_NUM    	(ADC_MAJOR_NUM << 16)| (ADC_MINOR_NUM << 8) | (ADC_BUILD_NUM)

// ErrorCode    				ErrorDescription
#define E_DRVADC_INVALID_INT		(ADC_ERR_ID | 0x01)	
#define E_DRVADC_INVALID_CHANNEL	(ADC_ERR_ID | 0x02)	
#define ERR_ADC_INVALID_INT			E_DRVADC_INVALID_INT
#define ERR_ADC_INVALID_CHANNEL	E_DRVADC_INVALID_CHANNEL
#define ERR_ADC_ARGUMENT           	(ADC_ERR_ID | 0x03)
#define ERR_ADC_CLOCK              		(ADC_ERR_ID | 0x04)

#define ADC_NONBLOCK	0
#define ADC_BLOCK	1

/* error code */

typedef void (*PFN_ADC_CALLBACK)(VOID);

typedef enum{
	eADC_EXT = 0,
	eADC_X32K,
	eADC_APLL,
	eADC_UPLL
}E_ADC_CLK;	
/*
typedef enum{
	eADC_TOUCH, 
	eADC_RECORD,
	eADC_BOTH
} E_ADC_ENABLE;
  */                                   
typedef enum{
	eADC_NORMAL, 
	eADC_RECORD
} E_ADC_MODE;     
                                                                                                                                                                                                      
typedef enum{ 
	eADC_RECORD_MODE_0 = 0,                                             
	eADC_RECORD_MODE_1,
	eADC_RECORD_MODE_2,
	eADC_RECORD_MODE_3
}E_ADC_RECORD_MODE;
   
typedef enum{ 
	eADC_ADC_INT = 0,                                           
   	eADC_AUD_INT,	
	eADC_WT_INT   	                                                                                  
}E_ADC_INT;   


INT32 adc_open(UINT32 type, UINT32 hr, UINT32 vr); 
VOID adc_close(VOID);
UINT32 adc_normalread(UINT32 u32Channel, PUINT16 pu16Data);   
INT32 adc_read(UINT32 mode, PUINT16 x, PUINT16 y);



INT32 audio_open(E_SYS_SRC_CLK eSrcClock, UINT32 u32ConvClock);
VOID audio_close(VOID);   
VOID audio_startrecord(VOID);	
VOID audio_stoprecord(VOID);

VOID ADC_GetAutoGainTiming(
	PUINT32 pu32Period,
	PUINT32 pu32Attack,
	PUINT32 pu32Recovery,
	PUINT32 pu32Hold
	);   
VOID ADC_SetAutoGainTiming(
	UINT32 u32Period,
	UINT32 u32Attack,
	UINT32 u32Recovery,
	UINT32 u32Hold
	);	
/* Remove if combine with EDMA */	
#if 0
INT32  
adc_installCallback(
	E_ADC_INT eIntType,
	PFN_ADC_CALLBACK pfnCallback,
	PFN_ADC_CALLBACK* pfnOldCallback
	);	
	
INT32 
adc_disableInt(
	E_ADC_INT eIntType
	);	
	
INT32  
adc_enableInt(
	E_ADC_INT eIntType
	);	
#endif


typedef VOID (*PFN_DRVADC_CALLBACK)(VOID);			

#ifdef  __cplusplus
}
#endif

#endif

