/***************************************************************************
 *                                                                         *
 * Copyright (c) 2008 Nuvoton Technolog. All rights reserved.              *
 *                                                                         *
 ***************************************************************************/

/****************************************************************************
* FILENAME
*   wb_power.c
*
* VERSION
*   1.0
*
* DESCRIPTION
*   The power managemnet related function of Nuvoton ARM9 MCU
*
* DATA STRUCTURES
*   None
*
* FUNCTIONS
*   sysDisableAllPM_IRQ
*   sysEnablePM_IRQ
*   sysPMStart
*
* HISTORY
*   2008-07-24  Ver 1.0 Modified by Min-Nan Cheng
*
* REMARK
*   When enter PD or MIDLE mode, the sysPMStart function will disable cache
*   (in order to access SRAM) and then recovery it after wake up.
****************************************************************************/
#include <stdio.h>
#include <string.h>
#include "wblib.h"

extern BOOL	sysGetCacheState(VOID);
extern INT32 	sysGetCacheMode(VOID);
extern INT32 	_sysLockCode(UINT32, INT32);
extern INT32 	_sysUnLockCode(VOID);

static UINT32 volatile _sys_PM_IRQ = 0;
UINT8  _tmp_buf[PD_RAM_SIZE];
UINT32  _clock_reg[(0x38/4)];
/****************************************************************************
* FUNCTION
*   sysEnablePM_IRQ
*
* DESCRIPTION
*   This function is use to set _sys_PM_IRQ variable. The _sys_PM_IRQ variable
*   save the IRQ channel which use to wake up the system form IDLE, MIDLE, and
*   PD mode
*
* INPUTS
*   irq_no         The interrupt channel
*
* OUTPUTS
*   Successful              The interrupt channel saved successfully
*   WB_PM_INVALID_IRQ_NUM   The interrupt channel number out of range
*
****************************************************************************/
INT sysEnablePM_IRQ(INT irq_no) // set the PM needed IRQ flag
{
    if ((irq_no > 31) || (irq_no < 1))
    {
        return WB_PM_INVALID_IRQ_NUM;
    }

    _sys_PM_IRQ = _sys_PM_IRQ | (UINT32)(1<<irq_no);
    return Successful;
}

/****************************************************************************
* FUNCTION
*   sysDisableAllPM_IRQ
*
* DESCRIPTION
*   This function is use to reset the _sys_PM_IRQ variable.
*
* INPUTS
*   None
*
* OUTPUTS
*   None
*
****************************************************************************/
VOID sysDisableAllPM_IRQ(VOID)  // clear the PM IRQ flag
{
    _sys_PM_IRQ = 0;
}


/****************************************************************************
* FUNCTION
*   _sysEnterPowerSavingMode
*
* DESCRIPTION
*   This function is use to enter idle or power down mode
*
* INPUTS
*   None
*
* OUTPUTS
*   None
*
****************************************************************************/
VOID _sysEnterPowerSavingMode(INT mode, UINT32 irqno)
{
 	volatile UINT32 i;
		
	outpw(REG_AIC_MECR, irqno); /* interrupt source to return from power saving mode */            
	//outpw(REG_PMCON, mode);     /* write control register to enter power saving mode */             
	outpw(REG_SDCMD, inpw(REG_SDCMD) | 
				    	(AUTOEXSELFREF | REF_CMD));
	__asm 
	{
		mov 		r2, 0xb0000200
		ldmia 	r2, {r0, r1}
		bic		r0, r0, #0x01
		bic		r1, r1, #0x01
		stmia 	r2, {r0, r1}
	}			    	    
    for (i=0; i<0x100; i++) ;
}


/****************************************************************************
* FUNCTION
*   sysPMStart
*
* DESCRIPTION
*   This function is use to enter power saving mode.
*
* INPUTS
*   pd_type     WB_PM_IDLE/WB_PM_MIDLE/WB_PM_PD
*               The Power saving mode
*
* OUTPUTS
*   Successful          Enter power saving mode and wake up successfully
*   WB_PM_PD_IRQ_Fail   Enter PD mode without enable one of USB DEVICE, KPI, 
*                       RTC or nIRQ
*                       which supports to wake up the system from PD mode
*   WB_PM_Type_Fail     pd_type error
*   WB_PM_CACHE_OFF     cache is disabled that can not lock necessary 
*                       instructions into cache
*
****************************************************************************/
INT sysPMStart(INT pd_type)
{
#if 0	
	UINT32 u32PllKHz;	
	UINT32 u32SysKHz;
	UINT32 u32CpuKHz;
	UINT32 u32HclkKHz;
	UINT32 u32ApbKHz;
#else
	UINT32 u32Idx;
#endif	

	VOID    (*wb_func)(INT32, UINT32);
	UINT32   vram_base, aic_status = 0;

	if(pd_type != WB_PM_IDLE && pd_type != WB_PM_MIDLE && pd_type != WB_PM_PD)
	{
		return WB_PM_Type_Fail;
	}

	if(pd_type == WB_PM_PD)
	{
		// Enter PD mode, but IRQ setting error
		// one of nIRQ0~7, KPI, RTC or USBD_INT must enable when enter PD mode
		if (!(_sys_PM_IRQ & (1<<IRQ_EXTINT0 | 1<<IRQ_EXTINT1 | 1<<IRQ_EXTINT2 | 1<<IRQ_EXTINT3 | 1<<IRQ_ADC)))
		{
			return WB_PM_PD_IRQ_Fail;
		}
	}

	aic_status = inpw(REG_AIC_IMR);
	outpw(REG_AIC_MDCR, 0xFFFFFFFF);

	if ((pd_type == WB_PM_MIDLE) || (pd_type == WB_PM_PD))
	{
		//use VRAM Block 1 to store power down function		
		vram_base = PD_RAM_BASE;		
		memcpy((VOID *)_tmp_buf, (VOID *)vram_base, PD_RAM_SIZE);
		memcpy((VOID *)vram_base,(VOID *)_sysEnterPowerSavingMode, PD_RAM_SIZE);		
	}

#if 0
	sysGetSystemClock(&eSrcClk,
				 	&u32PllKHz,	
					&u32SysKHz,
					&u32CpuKHz,
					&u32HclkKHz,
					&u32ApbKHz);	
#else
	for(u32Idx = 0; u32Idx<(sizeof(_clock_reg)/sizeof(_clock_reg[0])); u32Idx=u32Idx+1)
		_clock_reg[u32Idx]  = inp32(CLK_BA+u32Idx*4);
#endif
	// Entering Power saving mode
	wb_func = (void(*)(INT32, UINT32)) vram_base;
	wb_func(pd_type, _sys_PM_IRQ);

#if 0
	// Restore CPU clock source
	sysSetSystemClock(eSrcClk,
					u32PllKHz,
					u32SysKHz,
					u32CpuKHz,
					u32HclkKHz,
					u32ApbKHz);
#else
	for(u32Idx = 0; u32Idx<(sizeof(_clock_reg)/sizeof(_clock_reg[0])); u32Idx=u32Idx+1)
		 outp32((CLK_BA+u32Idx*4), _clock_reg[u32Idx]) ;
#endif	

	//Restore VRAM
	memcpy((VOID *)vram_base, (VOID *)_tmp_buf, PD_RAM_SIZE);

	outpw(REG_AIC_MDCR, 0xFFFFFFFF);    	// Disable all interrupt
	outpw(REG_AIC_MECR, aic_status);    	// Restore AIC setting    

	return Successful;

}


static void Sample_PowerDown(void)
{
	/* Enter self refresh */	
	__asm      /* In 264MHz' condition, need to delay a period for forcing DDR2 enter self-refresh (Bon's experiment) */
	{/* Dealy */
		mov 	r2, #20
		mov		r1, #0
		mov		r0, #1
	loop_p:		add r1, r1, r0
		cmp 	r1, r2
		bne		loop_p
	}	
	
	outp32(REG_SDCMD, (inp32(REG_SDCMD) & ~0x20) | 0x10);					
	__asm
	{/* Dealy */
		mov 	r2, #100
		mov		r1, #0
		mov		r0, #1
	loopa:		add r1, r1, r0
		cmp 	r1, r2
		bne		loopa
	}	
	//outp32(REG_GPIOD_DOUT, inp32(REG_GPIOD_DOUT) & ~0x3);  
	/* Change the system clock souce to 12M crystal*/
	outp32(REG_CLKDIV0, (inp32(REG_CLKDIV0) & (~0x18)) );				
	__asm
	{/* Delay */
		mov 		r2, #100
		mov		r1, #0
		mov		r0, #1
	loop0:	add 		r1, r1, r0
		cmp 		r1, r2
		bne		loop0
	}		
	//outp32(REG_GPIOD_DOUT, inp32(REG_GPIOD_DOUT) | 0x3);  
	
	outp32(REG_UPLLCON, inp32(REG_UPLLCON) | 0x4000);		/* Power down UPLL and APLL */
	outp32(REG_APLLCON, inp32(REG_APLLCON) | 0x4000);			
	__asm
	{
			mov 		r2, #300
			mov		r1, #0
			mov		r0, #1
	loop1:	add 		r1, r1, r0
			cmp 		r1, r2
			bne		loop1
	}		
	/* Fill and enable the pre-scale for power down wake up */
	outp32(REG_PWRCON, (inp32(REG_PWRCON)  & ~0xFFFF00) | 0xFFFF02);	
	
	__asm
	{
		mov 		r2, #10
		mov		r1, #0
		mov		r0, #1
	loop2:	add 		r1, r1, r0
		cmp 		r1, r2
		bne		loop2
	}	
	//outp32(REG_GPIOD_DOUT, (inp32(REG_GPIOD_DOUT) & ~0x03) | 0x1);
	/*  Enter power down. (Stop the external clock */
	__asm 
    {/* Power down */
    		mov 	r2, 0xb0000200
    		ldmia 	r2, {r0}
    		bic		r0, r0, #0x01
    		stmia 	r2, {r0}
    }   
	__asm
	{/* Wake up*/ 
			mov 	r2, #300
			mov		r1, #0
			mov		r0, #1
	loop3:	add 	r1, r1, r0
			cmp 	r1, r2
			bne		loop3
	}
	//outp32(REG_GPIOD_DOUT, (inp32(REG_GPIOD_DOUT) & ~0x03) | 0x2);
	
	 /* Force UPLL and APLL in normal mode */ 
	outp32(REG_UPLLCON, inp32(REG_UPLLCON) & (~0x4000));		
	outp32(REG_APLLCON, inp32(REG_APLLCON) & (~0x4000));	

	__asm
	{/* Delay a moment for PLL stable */
			mov 	r2, #5000
			mov		r1, #0
			mov		r0, #1
	loop4:	add 	r1, r1, r0
			cmp 	r1, r2
			bne		loop4
	}		
	/* Change system clock to PLL and delay a moment.  Let DDR/SDRAM lock the frequency */
	outp32(REG_CLKDIV0, inp32(REG_CLKDIV0) | 0x18);					
	__asm
	{
			mov 	r2, #500
			mov		r1, #0
			mov		r0, #1
	loop5:	add 	r1, r1, r0
			cmp 		r1, r2
			bne		loop5
	}
	/*Force DDR/SDRAM escape self-refresh */
	outp32(0xB0003004,  0x20);				
	__asm
	{/*  Delay a moment until the escape self-refresh command reached to DDR/SDRAM */
			mov 		r2, #100
			mov		r1, #0
			mov		r0, #1
	loop6:	add 		r1, r1, r0
			cmp 		r1, r2
			bne		loop6
	}			
		
}



static void Entry_PowerDown(UINT32 u32WakeUpSrc)
{
	UINT32 j;
	UINT32 u32IntEnable;
	void (*wb_fun)(void);
	UINT32 u32RamBase = PD_RAM_BASE;
	UINT32 u32RamSize = PD_RAM_SIZE;	
	UINT32 u32CacheMode;
	BOOL bIsEnableIRQ = FALSE;
	BOOL bIsCacheState = FALSE;
	
	if( sysGetIBitState()==TRUE )
	{
		bIsEnableIRQ = TRUE;
		sysSetLocalInterrupt(DISABLE_IRQ);	
	}		
	memcpy((char*)((UINT32)_tmp_buf| 0x80000000), (char*)(u32RamBase | 0x80000000), u32RamSize);
	//memcpy((char*)(u32RamBase | 0x80000000), (char*)((UINT32)Sample_PowerDown | 0x80000000), u32RamSize);
	memcpy((VOID *)((UINT32)u32RamBase | 0x80000000),
			(VOID *) (((UINT32)Sample_PowerDown-(PD_RAM_START-PD_RAM_BASE)) | 0x80000000), 
			PD_RAM_SIZE);
	
	if(memcmp((char*)(u32RamBase | 0x80000000), (char*) (((UINT32)Sample_PowerDown-(PD_RAM_START-PD_RAM_BASE)) | 0x80000000), u32RamSize)!=0)
	{
		sysprintf("Memcpy copy wrong\n");
	}
	if(sysGetCacheState()==TRUE){
		sysprintf("Cache enable\n");	
		bIsCacheState = TRUE;		
		u32CacheMode = sysGetCacheMode();
		sysDisableCache(); 	
		sysFlushCache(I_D_CACHE);	
	}else{
		sysprintf("Cache disable\n");	
	}
	//sysFlushCache(I_CACHE);		
	
	u32RamBase = PD_RAM_START;
	wb_fun = (void(*)(void)) u32RamBase;
	sysprintf("Jump to SRAM (Suspend)\n");
	u32IntEnable = inp32(REG_AIC_IMR);	
	outp32(REG_AIC_MDCR, 0xFFFFFFFE);
	outp32(REG_AIC_MECR, 0x00000000);	
	j = 0x800;
	while(j--);
	outp32(0xb0000030, ((u32WakeUpSrc<<24)|(u32WakeUpSrc<<16)));	//Enable and Clear interrupt
	wb_fun();
	outp32(0xb0000030, ((u32WakeUpSrc<<24)|(u32WakeUpSrc<<16)));	//Enable and Clear interrupt
	if(bIsCacheState==TRUE)
		sysEnableCache(u32CacheMode);	
	
	u32RamBase = PD_RAM_BASE;
	memcpy((VOID *)u32RamBase, (VOID *)_tmp_buf, PD_RAM_SIZE);
	sysprintf("Exit to SRAM (Suspend)\n");
	outp32(REG_AIC_MECR, u32IntEnable);								/*  Restore the interrupt channels */		
	if(bIsEnableIRQ==TRUE)
		sysSetLocalInterrupt(ENABLE_IRQ);	
}
ERRCODE sysPowerDown(UINT32 u32WakeUpSrc)
{
	Entry_PowerDown(u32WakeUpSrc);
	return Successful;
}
