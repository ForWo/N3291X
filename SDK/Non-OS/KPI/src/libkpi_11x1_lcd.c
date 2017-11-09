/****************************************************************************
*                                                                           *
* Copyright (c) 2009 Nuvoton Tech. Corp. All rights reserved.               *
*                                                                           *
*****************************************************************************/

/****************************************************************************
* FILENAME
*   libkpi.c
*
* VERSION
*   1.0
*
* DESCRIPTION
*   KPI library source file
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
#include "w55fa95_kpi.h"
#include "DrvKPI.h"

#define KPI_ROW_NUM	11
#define KPI_COL_NUM		1

// latest key pressed recorded by ISR, might _not_ be the most updated status
static UINT32 _key = 0;

static unsigned char _opened = 0;

UINT32 check_bit(UINT32 value, UINT32 bit) 
{
	int i, keybit = 0;

	for (i = 0; i < 8; i++)
	{		
		if (value & (bit << (i<<2)))
		{
		    keybit |= (1<<i);
		}
	}   
	return keybit;
}

UINT32 check_lbit(UINT32 value, UINT32 bit) 
{
	int i, keybit = 0;

	for (i = 0; i < 4; i++)
	{		
		if (value & (bit << (i<<2)))
		{
		    keybit |= (1<<i);
		}
	}   

	if(value & BIT28)
		keybit |= 0x10;
	
	return keybit;
}

UINT32 check_ubit(UINT32 value, UINT32 bit) 
{
	int i, keybit = 0;

	for (i = 0; i < 3; i++)
	{		
		if (value & (bit << (i<<2)))
		{
		    keybit |= (1<<(i+5));
		}
	}   
	return keybit;
}

VOID readkey(UINT32 u32KPIStatus)
{
	UINT32 u32LowerEvent, u32UpperEvent;
	UINT32 u32Colbit;		
	
	if((u32KPIStatus & PKEY_INT) == PKEY_INT)
	{				
		DrvKPI_GetKeyPressEvent(&u32LowerEvent, &u32UpperEvent);
		
		if (u32LowerEvent != 0)
		{
			u32Colbit = check_lbit(u32LowerEvent, KPI_CHECK_COL0);
			_key = _key |u32Colbit;			
			
			DrvKPI_ClearKeyPressEvent(eDRVKPI_LOWER_KEY, u32LowerEvent);			
		}	

		if (u32UpperEvent != 0)
		{
			u32Colbit = check_ubit(u32UpperEvent, KPI_CHECK_COL0);
			_key = _key |u32Colbit;			
			
			DrvKPI_ClearKeyPressEvent(eDRVKPI_UPPER_KEY, u32UpperEvent);			
		}
	}

	if((u32KPIStatus & RKEY_INT) == RKEY_INT)
	{				
		DrvKPI_GetKeyReleaseEvent(&u32LowerEvent, &u32UpperEvent);

		if (u32LowerEvent != 0)
		{
			u32Colbit = check_lbit(u32LowerEvent, KPI_CHECK_COL0);
			_key = _key & ~u32Colbit;
			
			DrvKPI_ClearKeyReleaseEvent(eDRVKPI_LOWER_KEY, u32LowerEvent);
		}

		if (u32UpperEvent != 0)
		{
			u32Colbit = check_ubit(u32UpperEvent, KPI_CHECK_COL0);
			_key = _key & ~u32Colbit;
			
			DrvKPI_ClearKeyReleaseEvent(eDRVKPI_UPPER_KEY, u32UpperEvent);
		}
	}	
	
}

void kpi_init(void)
{
	UINT32 u32ExtFreq;
	
	_opened = 1;
	
	outpw(REG_MISC_DS_GPC, 0xFFFF);

	u32ExtFreq = sysGetExternalClock();
	if (u32ExtFreq == 12000)
		DrvKPI_Open(eDRVKPI_DBCLK_XIN, 11);
	else
		DrvKPI_Open(eDRVKPI_DBCLK_XIN, 26);

	DrvKPI_Ioctl(KPI_IOC_SET_MATRIX_NUMBER, KPI_ROW_NUM, KPI_COL_NUM, NULL);	
	DrvKPI_Ioctl(KPI_IOC_SET_DEBOUNCE_TIME, eDRVKPI_DEBOUNCE_1024CLK, 0x40, NULL);
	DrvKPI_Ioctl(KPI_IOC_SET_SUSPEND_CYCLE, 0xFFFFF, NULL, NULL);
	DrvKPI_Ioctl(KPI_IOC_SET_ROW_SCAN_TIME, 1, 2, NULL);	   
	
	return;
}

int kpi_open(unsigned int src)
{
	PFN_DRVKPI_CALLBACK pfnOldcallback;

	if(_opened == 0)
		kpi_init();
	if(_opened != 1)
		return(-1);
	
	_opened = 2;

	outp32(REG_APBCLK, inp32(REG_APBCLK)|KPI_CKE);

	DrvKPI_EnableInt(eDRVKPI_INTEN |eDRVKPI_PKINTEN |eDRVKPI_RKINTEN);	

	/* Install the call back function */		
	DrvKPI_InstallCallBack(readkey, &pfnOldcallback);	

	/* Keypad Scan Enable */
	DrvKPI_Enable();
	
	return(0);    
}

void kpi_close(void)
{	
	if(_opened != 2)
		return;
	_opened = 1;

	DrvKPI_Close();
	
	return;
}

int kpi_read(unsigned char mode)
{
	// add this var in case key released right before return.
	int volatile k = 0;
	
	if(_opened != 2)
		return(-1);
	
	if(mode != KPI_NONBLOCK && mode != KPI_BLOCK) {		  
		return(-1);
	}
		
	if(_key == 0) {
		// not pressed, non blocking, return immediately
		if(mode == KPI_NONBLOCK) {			
			return(0);
		}
		// not pressed, blocking, wait for key pressed
#pragma O0
// ARMCC is tooooo smart to compile this line correctly, so ether set O0 or use pulling....
		while((k = _key) == 0);
	} else {
		// read latest key(s) and return.
		
		do {						
			k = _key;
		} while(k == 0 && mode != KPI_NONBLOCK);
	}
	
	return(k);
}


