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

#define KPI_ROW_NUM	16
#define KPI_COL_NUM		4

// latest key pressed recorded by ISR, might _not_ be the most updated status
static UINT32 _key = 0;

static unsigned char _opened = 0;

/*
//key(m,n) m=row, n=column
const int key_map[KEY_CNT] =
{
  key(0,0), key(1,0), key(2,0), ..., key(13,0), key(14,0), key(15,0),
  key(0,1), key(1,1), key(2,1), ..., key(13,1), key(14,1), key(15,1),
  key(0,2), key(1,2), key(2,2), ..., key(13,2), key(14,2), key(15,2),
  key(0,3), key(1,3), key(2,3), ..., key(13,3), key(14,3), key(15,3), 
};
*/
static const unsigned short Key_Code_List[] =
{
    0,
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
    33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
    49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64,
    0    
};


static UINT32 check_column(UINT32 value, UINT32 bit) 
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

static UINT32 check_bit(UINT32 value)
{
    int i = 0;

    for (i = 0; i < KPI_ROW_NUM; i++)
    {        
        if(value & (1 << i)) 
        {
            return (i + 1);
        }
    }
    return 0;
}

VOID readkey(UINT32 multi_key)
{
	UINT32 u32LowerState, u32UpperState;
	UINT32 new_key = 0, key;
	UINT32 u32Colbit, i;

	DrvKPI_GetKeyState(&u32LowerState, &u32UpperState);

	if((u32LowerState == 0) && (u32UpperState == 0))
	{
		_key =0;
		return;
	}

	for (i = 0; i < KPI_COL_NUM; i++)
	{
		if (u32LowerState != 0)
		{
			u32Colbit = check_column(u32LowerState, (KPI_CHECK_COL0<<i));
			new_key = new_key |u32Colbit;
		}
		
		if(u32UpperState != 0)
		{
			u32Colbit = check_column(u32UpperState, (KPI_CHECK_COL0<<i));
			new_key = new_key |(u32Colbit<<8);
		}

		if (multi_key)
		{
			if(new_key != 0)
			{
				_key = i*0x10000 + new_key;
				break;
			}		
		}
		else
		{
			if(new_key != 0)
			{
				key = i*KPI_ROW_NUM + check_bit(new_key);
				_key = Key_Code_List[key];
				break;
			}			
		}			
	}	
	
}

void kpi_isr(void)
{
	volatile UINT32 u32IntStatus;	
	UINT32 u32LowerEvent, u32UpperEvent;	

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

	if((u32IntStatus & PKEY_INT) == PKEY_INT)
	{				
		DrvKPI_GetKeyPressEvent(&u32LowerEvent, &u32UpperEvent);				
		if (u32LowerEvent != 0)
			DrvKPI_ClearKeyPressEvent(eDRVKPI_LOWER_KEY, u32LowerEvent);
		if (u32UpperEvent != 0)
			DrvKPI_ClearKeyPressEvent(eDRVKPI_UPPER_KEY, u32UpperEvent);		
	}

	if((u32IntStatus & RKEY_INT) == RKEY_INT)
	{				
		DrvKPI_GetKeyReleaseEvent(&u32LowerEvent, &u32UpperEvent);				
		if (u32LowerEvent != 0)
			DrvKPI_ClearKeyReleaseEvent(eDRVKPI_LOWER_KEY, u32LowerEvent);	
		if (u32UpperEvent != 0)
			DrvKPI_ClearKeyReleaseEvent(eDRVKPI_UPPER_KEY, u32UpperEvent);		
	}

	readkey(0);

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
	if(_opened == 0)
		kpi_init();
	if(_opened != 1)
		return(-1);
	
	_opened = 2;

	outp32(REG_APBCLK, inp32(REG_APBCLK)|KPI_CKE);

	DrvKPI_EnableInt(eDRVKPI_INTEN |eDRVKPI_PKINTEN |eDRVKPI_RKINTEN);	
		
	sysInstallISR(IRQ_LEVEL_7, IRQ_KPI, (PVOID)kpi_isr);
	sysSetInterruptType(IRQ_KPI, HIGH_LEVEL_SENSITIVE);
	sysSetLocalInterrupt(ENABLE_IRQ);
	sysEnableInterrupt(IRQ_KPI);

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


