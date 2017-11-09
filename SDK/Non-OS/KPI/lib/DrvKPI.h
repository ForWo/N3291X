/****************************************************************
 *                                                              *
 * Copyright (c) Nuvoton Technology Corp. All rights reserved. *
 *                                                              *
 ****************************************************************/

#ifndef _DRVKPI_H_
#define _DRVKPI_H_

/*---------------------------------------------------------------------------------------------------------*/
/* Includes of system headers                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
#include "wbtypes.h"
#include "w55fa95_reg.h"

#ifdef  __cplusplus 
extern "C"
{
#endif

#define KPI_CLOCK_SOURCE_BIT        5
#define KPI_CLOCK_DIVIDER0_BIT        11

#define MAX_ROW_NUM   16
#define MAX_COL_NUM   4
#define MIN_ROW_NUM   2
#define MIN_COL_NUM   1

#define 	ERR_KPI				(0xFFFF0000 | ((KPI_BA>>16) & 0xFF00) |((KPI_BA>>8) & 0xFF))

#define E_DRVKPI_FALSE_INPUT      	(ERR_KPI | 00)

typedef void (*PFN_DRVKPI_CALLBACK)(UINT32 userData);

typedef enum
{
	eDRVKPI_DISABLE =0,
	eDRVKPI_ENABLE
}E_DRVKPI_OPERATION;

/* Define De-bounce sampling cycle*/
typedef enum {
	eDRVKPI_DEBOUNCE_1CLK = 0,
	eDRVKPI_DEBOUNCE_2CLK,
	eDRVKPI_DEBOUNCE_4CLK,
	eDRVKPI_DEBOUNCE_8CLK,
	eDRVKPI_DEBOUNCE_16CLK,
	eDRVKPI_DEBOUNCE_32CLK,
	eDRVKPI_DEBOUNCE_64CLK,
	eDRVKPI_DEBOUNCE_128CLK,
	eDRVKPI_DEBOUNCE_256CLK,
	eDRVKPI_DEBOUNCE_512CLK,
	eDRVKPI_DEBOUNCE_1024CLK,
	eDRVKPI_DEBOUNCE_2048CLK,
	eDRVKPI_DEBOUNCE_4096CLK,
	eDRVKPI_DEBOUNCE_8192CLK		
} E_DRVKPI_DBCLKSEL;

/* Define De-bounce counter clock source select*/
typedef enum {
	eDRVKPI_DBCLK_XIN = 0,
	eDRVKPI_DBCLK_X32K	
} E_DRVKPI_DBCLKSRC;

typedef enum
{
	eDRVKPI_OUTPUT_FROM_GPIO=0,    
	eDRVKPI_OUTPUT_FROM_LVDAT  
}E_DRVKPI_LCM_ENABLE;

typedef enum
{
	eDRVKPI_PKINTEN = 2,
	eDRVKPI_RKINTEN = 4,
	eDRVKPI_INTEN = 8,	
	eDRVKPI_WAKEUP = 0x20
}E_DRVKPI_INT_ENABLE;

typedef enum
{
	eDRVKPI_PDWAKE_FLAG =1,
	eDRVKPI_RSTKEY_FLAG = 2,
	eDRVKPI_KEYINT_FLAG = 4,	
	eDRVKPI_KEYRELEASE_FLAG = 8,	          	    
	eDRVKPI_KEYPRESS_FLAG=0x10	  	
}E_DRVKPI_INT_FLAG;

typedef enum
{
	eDRVKPI_LOWER_KEY = 0,    
	eDRVKPI_UPPER_KEY 
}E_DRVKPI_KEYSEL;

typedef enum
{
	eDRVKPI_KEY0 = 0,    
	eDRVKPI_KEY1, 
	eDRVKPI_KEY2
}E_DRVKPI_THREE_KEY_RESET;

#define KPI_IOC_SET_MATRIX_NUMBER		0
#define KPI_IOC_SET_DEBOUNCE_TIME		1
#define KPI_IOC_SET_ROW_SCAN_TIME	2
#define KPI_IOC_SET_SUSPEND_CYCLE		3
#define KPI_IOC_SET_RESET_KEY			4
#define KPI_IOC_SET_RESET_KEY_OPERATION		5

VOID DrvKPI_Open(E_DRVKPI_DBCLKSRC eCLKSrc,  UINT8 u8CLKDiv);

VOID DrvKPI_Close(VOID);

ERRCODE DrvKPI_Ioctl(UINT32 u32cmd, UINT32 u32arg0, UINT32 u32arg1, UINT32 u32arg2);

ERRCODE DrvKPI_SetMatrixNumber(	
	UINT8 u8Row, UINT8 u8Col);

VOID DrvKPI_SetDeBounceTime(E_DRVKPI_DBCLKSEL eDBCLK, UINT8 u8Scale);

VOID DrvKPI_GetDeBounceTime(E_DRVKPI_DBCLKSEL* peDBCLK, PUINT8 pu8Scale);

VOID DrvKPI_SetRowScanTime(UINT32 u32HSDBRow, UINT32 u32HSDBNum);

VOID DrvKPI_SetSuspendCycle(UINT32 u32SUSCNum);

VOID DrvKPI_GetRowScanTime(UINT8* pu8HSDBRow, UINT8* pu8HSDBNum);

VOID DrvKPI_EnableInt(E_DRVKPI_INT_ENABLE eIntSource);

VOID DrvKPI_DisableInt(E_DRVKPI_INT_ENABLE eIntSource);

VOID DrvKPI_Enable(VOID);

VOID DrvKPI_SetThreeKeyReset(
	E_DRVKPI_THREE_KEY_RESET eKey,
	UINT8 u8Row, UINT8 u8Col
);

VOID DrvKPI_GetThreeKeyReset(
	E_DRVKPI_THREE_KEY_RESET eKey,
	UINT8* pu8Row, UINT8* pu8Col
);

VOID DrvKPI_SetThreeKeyResetEnable(E_DRVKPI_OPERATION eOP);

VOID DrvKPI_IRQHandler(void);

ERRCODE DrvKPI_InstallCallBack(	
	PFN_DRVKPI_CALLBACK pfncallback,
	PFN_DRVKPI_CALLBACK *pfnOldcallback
);

VOID DrvKPI_GetKeyState(
	UINT32* pu32LowerState, 
	UINT32* pu32UpperState
);

VOID DrvKPI_GetKeyPressEvent(
	UINT32* pu32LowerEvent, 
	UINT32* pu32UpperEvent
);

VOID DrvKPI_ClearKeyPressEvent(E_DRVKPI_KEYSEL eKeySel, UINT32 u32Event);

VOID DrvKPI_GetKeyReleaseEvent(
	UINT32* pu32LowerEvent, 
	UINT32* pu32UpperEvent
);

VOID DrvKPI_ClearKeyReleaseEvent(E_DRVKPI_KEYSEL eKeySel, UINT32 u32Event);

BOOL DrvKPI_PollInt(E_DRVKPI_INT_FLAG eIntFlag);

VOID DrvKPI_ClearInt(E_DRVKPI_INT_FLAG eIntFlag);

#ifdef __cplusplus
}
#endif

#endif	/*_DRVKPI_H_*/


