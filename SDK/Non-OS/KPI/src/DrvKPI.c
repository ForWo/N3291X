/****************************************************************
 *                                                              *
 * Copyright (c) Nuvoton Technology Corp. All rights reserved. *
 *                                                              *
 ****************************************************************/

#include <stdio.h>
#include <string.h>
#include "wblib.h"

#include "DrvKPI.h"

static PFN_DRVKPI_CALLBACK g_pfnKPIcallback = NULL;

VOID DrvKPI_Open(E_DRVKPI_DBCLKSRC eCLKSrc,  UINT8 u8CLKDiv)
{ 
	outp32(REG_APBCLK, inp32(REG_APBCLK)|KPI_CKE);	

	outp32(REG_CLKDIV0 , (inp32(REG_CLKDIV0) & ~ KPI_S) | (eCLKSrc<<KPI_CLOCK_SOURCE_BIT));	
	outp32(REG_CLKDIV0 , (inp32(REG_CLKDIV0) & ~ KPI_N0) | ((u8CLKDiv & 0x7F)<<KPI_CLOCK_DIVIDER0_BIT));	
}

VOID DrvKPI_Close(VOID)
{	
	outp32(REG_KPICONF, inp32(REG_KPICONF) & ~ENKP);
   	sysDisableInterrupt(IRQ_KPI);
	outp32(REG_APBCLK, inp32(REG_APBCLK) & ~KPI_CKE);
	
}

ERRCODE DrvKPI_Ioctl(UINT32 u32cmd, UINT32 u32arg0, UINT32 u32arg1, UINT32 u32arg2)
{
	switch(u32cmd)
	{
		case KPI_IOC_SET_MATRIX_NUMBER:
			DrvKPI_SetMatrixNumber(u32arg0, u32arg1);
			break;
			
		case KPI_IOC_SET_DEBOUNCE_TIME:
			DrvKPI_SetDeBounceTime(u32arg0, u32arg1);
			break;

		case KPI_IOC_SET_ROW_SCAN_TIME:
			DrvKPI_SetRowScanTime(u32arg0, u32arg1);
			break;
			
		case KPI_IOC_SET_SUSPEND_CYCLE:
			DrvKPI_SetSuspendCycle(u32arg0);
			break;

		case KPI_IOC_SET_RESET_KEY:
			DrvKPI_SetThreeKeyReset(u32arg0, u32arg1, u32arg2);
			break;

		case KPI_IOC_SET_RESET_KEY_OPERATION:
			DrvKPI_SetThreeKeyResetEnable(u32arg0);
			break;

		default:
			return(E_DRVKPI_FALSE_INPUT);
	}

	return Successful;
}

ERRCODE DrvKPI_SetMatrixNumber(	
	UINT8 u8Row, UINT8 u8Col)
{
	UINT32 i;

	if ((u8Row > MAX_ROW_NUM) ||(u8Col > MAX_COL_NUM))
		return E_DRVKPI_FALSE_INPUT;

	if ((u8Row < MIN_ROW_NUM) ||(u8Col < MIN_COL_NUM))
		return E_DRVKPI_FALSE_INPUT;	

	for(i=0;i<u8Col;i++)
		outp32(REG_GPAFUN, (inp32(REG_GPAFUN) & ~(0xC0<<i*2)) |(0x40<<i*2));		

	outp32(REG_KPICONF , (inp32(REG_KPICONF) & ~ (KROW |KCOL)) |((u8Row -1) & 0xF)<<28 |((u8Col -1) & 0x3)<<24);

	return Successful;
	
}

VOID DrvKPI_SetDeBounceTime(E_DRVKPI_DBCLKSEL eDBCLK, UINT8 u8Scale)
{
	outp32(REG_KPICONF , inp32(REG_KPICONF) | DB_EN);	
	outp32(REG_KPICONF , (inp32(REG_KPICONF) & ~ (DB_CLKSEL | PRESCALE)) | eDBCLK<<16 | u8Scale<<8);
	outp32(REG_KPIPRESCALDIV , (inp32(REG_KPIPRESCALDIV) & ~ PRESCALDIV) | 0x1F);
}

VOID DrvKPI_GetDeBounceTime(E_DRVKPI_DBCLKSEL* peDBCLK, PUINT8 pu8Scale)
{
	*peDBCLK = (inp32(REG_KPICONF) & DB_CLKSEL) >> 16;	
	*pu8Scale = (inp32(REG_KPICONF) & PRESCALE) >> 8;	
}

VOID DrvKPI_SetRowScanTime(UINT32 u32HSDBRow, UINT32 u32HSDBNum)
{		
	outp32(REG_KPILCM , (inp32(REG_KPILCM) & ~ (HSDBROW | HSDBNUM)) | (((u32HSDBRow & 0xF) << 8) | u32HSDBNum & 0xFF));	
	outp32(REG_KPILCM , inp32(REG_KPILCM) | LCMMODE);
}

VOID DrvKPI_SetSuspendCycle(UINT32 u32SUSCNum)
{			
	outp32(REG_KPISUS , (inp32(REG_KPISUS) & ~ (SUSCNUM)) | (u32SUSCNum & 0xFFFFF));	
}

VOID DrvKPI_GetRowScanTime(UINT8* pu8HSDBRow, UINT8* pu8HSDBNum)
{		
	*pu8HSDBRow = (inp32(REG_KPILCM) & HSDBROW) >> 8;	
	*pu8HSDBNum = (inp32(REG_KPILCM) & HSDBNUM);	
}

VOID DrvKPI_EnableInt(E_DRVKPI_INT_ENABLE eIntSource)
{		
	outp32(REG_KPICONF , (inp32(REG_KPICONF) & ~(PKINTEN | RKINTEN |INTEN |WAKEUP)) |eIntSource);		
}

VOID DrvKPI_DisableInt(E_DRVKPI_INT_ENABLE eIntSource)
{		
	outp32(REG_KPICONF , inp32(REG_KPICONF) & ~ eIntSource);		
}

VOID DrvKPI_Enable(VOID)
{		
	outp32(REG_KPICONF , inp32(REG_KPICONF)  | ODEN |INPU);
	outp32(REG_KPICONF , inp32(REG_KPICONF)  | ENKP );	
}

VOID DrvKPI_SetThreeKeyReset(
	E_DRVKPI_THREE_KEY_RESET eKey,
	UINT8 u8Row, UINT8 u8Col)
{	
	if(eKey == eDRVKPI_KEY0)
		outp32(REG_KPI3KCONF , (inp32(REG_KPI3KCONF) & ~ (K30R | K30C))  | ((u8Row & 0xF)<<2) | (u8Col & 0x3));	
	else if(eKey == eDRVKPI_KEY1)
		outp32(REG_KPI3KCONF , (inp32(REG_KPI3KCONF) & ~ (K31R | K31C))  | ((u8Row & 0xF)<<10) | ((u8Col & 0x3)<<8));
	else if(eKey == eDRVKPI_KEY2)
		outp32(REG_KPI3KCONF , (inp32(REG_KPI3KCONF) & ~ (K32R | K32C))  | ((u8Row & 0xF)<<18) | ((u8Col & 0x3)<<16));			
}

VOID DrvKPI_GetThreeKeyReset(
	E_DRVKPI_THREE_KEY_RESET eKey,
	UINT8* pu8Row, UINT8* pu8Col)	
{		
	if(eKey == eDRVKPI_KEY0)
	{
		* pu8Row = (inp32(REG_KPI3KCONF) & K30R) >> 2;
		* pu8Col = (inp32(REG_KPI3KCONF) & K30C);
	}
	else if(eKey == eDRVKPI_KEY1)
	{
		* pu8Row = (inp32(REG_KPI3KCONF) & K31R) >> 10;
		* pu8Col = (inp32(REG_KPI3KCONF) & K31C) >> 8;
	}
	else if(eKey == eDRVKPI_KEY2)
	{
		* pu8Row = (inp32(REG_KPI3KCONF) & K32R) >> 18;
		* pu8Col = (inp32(REG_KPI3KCONF) & K32C) >> 16;		
	}
}

VOID DrvKPI_SetThreeKeyResetEnable(E_DRVKPI_OPERATION eOP)
{
	outp32(REG_KPIRSTC , 0x89);	
	
	if(eOP == eDRVKPI_DISABLE)
		outp32(REG_KPI3KCONF , inp32(REG_KPI3KCONF) & ~EN3KYRST);
	else
		outp32(REG_KPI3KCONF , inp32(REG_KPI3KCONF) | EN3KYRST);	
	
}

VOID DrvKPI_IRQHandler(void)
{
    volatile UINT32 u32IntStatus;	

    u32IntStatus = inp32(REG_KPISTATUS);		

	if ((u32IntStatus & RST_3KEY) == RST_3KEY)
	{
		outp32(REG_KPISTATUS , inp32(REG_KPISTATUS) & RST_3KEY);
	}

	if ((u32IntStatus & PDWAKE) == PDWAKE)
	{
		outp32(REG_KPISTATUS , inp32(REG_KPISTATUS) & PDWAKE);
		//sysprintf("wake up INT \n");	
		
		outp32(REG_KPIKPE0 , 0xFFFFFFFF);
		outp32(REG_KPIKPE1 , 0xFFFFFFFF);
		outp32(REG_KPIKRE0 , 0xFFFFFFFF);
		outp32(REG_KPIKRE1 , 0xFFFFFFFF);
		return;					
	}				
	
    if(g_pfnKPIcallback != NULL)
    {
        g_pfnKPIcallback(u32IntStatus);
    }

}


ERRCODE DrvKPI_InstallCallBack(	
	PFN_DRVKPI_CALLBACK pfncallback,
	PFN_DRVKPI_CALLBACK *pfnOldcallback
)
{    

	*pfnOldcallback = g_pfnKPIcallback;
	g_pfnKPIcallback = pfncallback;        	

	sysInstallISR(IRQ_LEVEL_7, IRQ_KPI, (PVOID)DrvKPI_IRQHandler);
	sysSetInterruptType(IRQ_KPI, HIGH_LEVEL_SENSITIVE);
	sysSetLocalInterrupt(ENABLE_IRQ);
	sysEnableInterrupt(IRQ_KPI);
   
	return Successful;
}

VOID DrvKPI_GetKeyState(
	UINT32* pu32LowerState, 
	UINT32* pu32UpperState
)
{
	* pu32LowerState = inp32(REG_KPIKEST0);
	* pu32UpperState = inp32(REG_KPIKEST1);
}

VOID DrvKPI_GetKeyPressEvent(
	UINT32* pu32LowerEvent, 
	UINT32* pu32UpperEvent
)
{
	* pu32LowerEvent = inp32(REG_KPIKPE0);
	* pu32UpperEvent = inp32(REG_KPIKPE1);
}

VOID DrvKPI_ClearKeyPressEvent(E_DRVKPI_KEYSEL eKeySel, UINT32 u32Event)
{
	if(eKeySel == eDRVKPI_LOWER_KEY)
		outp32(REG_KPIKPE0 , u32Event);
	else
		outp32(REG_KPIKPE1 , u32Event);
}

VOID DrvKPI_GetKeyReleaseEvent(
	UINT32* pu32LowerEvent, 
	UINT32* pu32UpperEvent
)
{
	* pu32LowerEvent = inp32(REG_KPIKRE0);
	* pu32UpperEvent = inp32(REG_KPIKRE1);
}

VOID DrvKPI_ClearKeyReleaseEvent(E_DRVKPI_KEYSEL eKeySel, UINT32 u32Event)
{
	if(eKeySel == eDRVKPI_LOWER_KEY)
		outp32(REG_KPIKRE0 , u32Event);
	else
		outp32(REG_KPIKRE1 , u32Event);
}

BOOL DrvKPI_PollInt(E_DRVKPI_INT_FLAG eIntFlag)
{    
    return inp32(REG_KPISTATUS) & eIntFlag;
}

VOID DrvKPI_ClearInt(E_DRVKPI_INT_FLAG eIntFlag)
{   
    outp32(REG_KPISTATUS, eIntFlag);
}


                
