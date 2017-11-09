
/***************************************************************************
 *                                                                         *
 * Copyright (c) 2008 Nuvoton Technolog. All rights reserved.              *
 *                                                                         *
 ***************************************************************************/
/****************************************************************************
 *
 * FILENAME : wb_config.c
 *
 * VERSION  : 1.1
 *
 * DESCRIPTION :
 *               PLL control functions of Nuvoton ARM9 MCU
 *
 * HISTORY
 *   2008-06-25  Ver 1.0 draft by Min-Nan Cheng
 * Modification
 *   2011-06-01  Ver 1.1 draft by Shih-Wen Chou
 *
 *		IBR set clocks default value
 * 			UPLL= 240MHz
 *			SYS = 120MHz
 *			CPU = 60MHz
 *			HCLK = 60MHz
 *
 *
 *
 *
 ****************************************************************************/
#include <string.h>
#include "wblib.h"
#define REAL_CHIP

static UINT32 g_u32SysClkSrc;
static UINT32 g_u32UpllHz = 240000000, g_u32ApllHz=240000000; //g_u32SysHz = 120000000, g_u32CpuHz = 60000000, g_u32HclkHz = 60000000;
static UINT32 g_i32REG_APLL, g_i32REG_UPLL;
static UINT32 g_u32ExtClk = 12000000;
static UINT32 g_u32SysInteg, g_u32SysFract, g_u32PllPreDiv;

extern INT32 sysGetCacheMode(void);

//extern UINT8  _tmp_buf[];
UINT8  _tmp_buf[PD_RAM_SIZE];

BOOL bIsFirstReadClkSkew = TRUE;
UINT32 u32ClkSkewInit = 0;

//#define DBG_PRINTF		sysprintf
#define DBG_PRINTF(...)
/*-----------------------------------------------------------------------------------------------------------
 *
 * Function : sysGetPLLOutputHz
 *
 * DESCRIPTION :
 *               According to the external clock and expected PLL output clock to get the content of PLL controll register
 *
 * Parameters
 *             	eSysPll : eSYS_APLL or eSYS_UPLL
 *			u32FinKHz: External clock. Unit: KHz
 * Return
 *			PLL clock.
 * HISTORY
 *               2010-07-15
 *
-----------------------------------------------------------------------------------------------------------*/
UINT32 sysGetPLLOutputHz(
	E_SYS_SRC_CLK eSysPll,
	UINT32 u32FinHz
	)
{
	UINT32 u32PllCntlReg, u32Fout;
	UINT32 NF, NR, NO;
	UINT32 u32NOArray[] = { 1, 2, 4, 8};


	if(eSysPll==eSYS_APLL)
		u32PllCntlReg = inp32(REG_APLLCON);
	else if(eSysPll==eSYS_UPLL)
		u32PllCntlReg = inp32(REG_UPLLCON);

	if(u32PllCntlReg&0x10000)			//PLL power down.
		return 0;

	NF = (u32PllCntlReg & 0x7F)<<1;
	NR = (u32PllCntlReg & 0x780)>>7;
	NO = u32NOArray[((u32PllCntlReg&0x1800)>>11)];

	DBG_PRINTF("NR, NF, NO = %d, %d, %d\n", NR, NF, NO);
	u32Fout = u32FinHz/NO/NR*NF;
	#ifdef REAL_CHIP
	return u32Fout;
	#else
	return u32FinHz;
	#endif

}
/*-----------------------------------------------------------------------------------------------------------
 *
 * Function : sysGetPLLControlRegister
 *
 * DESCRIPTION :
 *               According to the external clock and expected PLL output clock to get the content of PLL controll register
 *
 * Parameters
 *             	u32FinKHz : External clock.  Unit:KHz
 *			u32TargetKHz: PLL output clock. Unit:KHz
 * Return
 *               	0 : No any fit value for the specified PLL output clock
 *			PLL control register.
 * HISTORY
 *               2011-10-21
 *
-----------------------------------------------------------------------------------------------------------*/

BOOL bIsCheckConstraint = TRUE;
void sysCheckPllConstraint(BOOL bIsCheck)
{
	bIsCheckConstraint = bIsCheck;
}

#define MIN_FBDV_M		4
#define MAX_FBDV_M		256
#define MIN_INDV_N		2
#define MAX_INDV_N		16
INT32 _sysGetPLLControlRegister(UINT32 u32FinKHz, UINT32 u32TargetHz)
{
	UINT32 u32ClkOut;
	UINT32 u32NO;
	UINT32 u32IdxM, u32IdxN;
	INT32 i32IdxNO;
	UINT32 u32NOArray[] = { 1, 2, 4, 8};
#if 0
	for(u32IdxM=MIN_FBDV_M;u32IdxM<MAX_FBDV_M;u32IdxM=u32IdxM+1)
	{//u32IdxM=NR >=4. Fedback divider.
		for(u32IdxN=MIN_INDV_N;u32IdxN<MAX_INDV_N;u32IdxN=u32IdxN+1)
		{//u32IdxN=N >=2. (NR = u32IdxN). Input divider
			for(i32IdxNO=0;i32IdxNO<4;i32IdxNO=i32IdxNO+1)
			{
#else
	/* To get little jiter on PLL output, i32IdxNO has better =0x03 or 0x02 */
	for(i32IdxNO=3;i32IdxNO>0;i32IdxNO=i32IdxNO-1)
	{
		for(u32IdxM=MIN_FBDV_M;u32IdxM<MAX_FBDV_M;u32IdxM=u32IdxM+1)
		{//u32IdxM=NR >=4. Fedback divider.
			for(u32IdxN=MIN_INDV_N;u32IdxN<MAX_INDV_N;u32IdxN=u32IdxN+1)
			{//u32IdxN=N >=2. (NR = u32IdxN). Input divider
#endif
				if(bIsCheckConstraint==TRUE)
				{
					if((u32FinKHz/u32IdxN)>50000)					/* 1MHz < FIN/NR < 50MHz */
						continue;
					if((u32FinKHz/u32IdxN)<1000)
						continue;
				}
				u32NO = u32NOArray[i32IdxNO];				/* FOUT = (FIN * NF/NR)/NO */
				u32ClkOut = u32FinKHz*u32IdxM/u32IdxN/u32NO;	/* Where NF = u32IdxM, 	NR = u32IdxN, NO=u32NOArray[i32IdxNO]. */
				if((u32ClkOut*1000)==u32TargetHz)
				{
					if(bIsCheckConstraint==TRUE)
					{
						if((u32ClkOut*u32NO)<500000)   			/* 500MHz <= FIN/NO < 1500MHz */
							continue;
						if((u32ClkOut*u32NO)>1500000)
							continue;
					}
					DBG_PRINTF("\n****************\n");
					DBG_PRINTF("M = 0x%x\n",u32IdxM);
					DBG_PRINTF("N = 0x%x\n",u32IdxN);
					DBG_PRINTF("NO = 0x%x\n",i32IdxNO);
					return ((u32IdxM>>1) | (u32IdxN<<7) | (i32IdxNO<<11));
				}
			}
		}
	}
	return -1;
}

/*-----------------------------------------------------------------------------------------------------------
* Function: sysSetPLLControlRegister
*
* Parameters:
*              u32PllValue - [in], PLL setting value
*
* Returns:
*      None
*
* Description:
*              To set the PLL control register.
*
-----------------------------------------------------------------------------------------------------------*/
void
sysSetPLLControlRegister(
	E_SYS_SRC_CLK eSysPll,
	UINT32 u32PllValue
	)
{
	if(eSysPll==eSYS_APLL)
		outp32(REG_APLLCON, u32PllValue);
	else if(eSysPll==eSYS_APLL)
		outp32(REG_UPLLCON, u32PllValue);
}


/*-----------------------------------------------------------------------------------------------------------
* Function: sysSetSystemDivider
*
* Parameters:
* 		u32Integ - [in], System clock divider integrate part. 		0<=u32Integ<=7
*              u32Fract - [in], System clock divider u32Fraction part. 	u32Fract can only be 0
* Returns:
*      Successful or Error Code
*
* Description:
*              To set the system clock divider directly.
*
-----------------------------------------------------------------------------------------------------------*/
void _sysClockDiv(register UINT32  u32Hclk, register UINT32 u32Integ)
{
	register UINT32 u32Clk0Div;
	volatile register UINT32 i=3000;
	register UINT32 j, z,x ,y;

	u32Clk0Div =  inp32(REG_CLKDIV0) & ~(SYSTEM_INTEG|SYSTEM_FRACTM|SYSTEM_FRACTL);
	u32Clk0Div = ((u32Clk0Div | (u32Integ<<29)) | ((0>>5) <<27)) | ((0&0x1F)<<6);

	outp32(REG_AIC_MDCR, 0xFFFFFFFE);
	if(bIsFirstReadClkSkew==TRUE)
	{/* Record the default value from IBR or NAND Loader */
		bIsFirstReadClkSkew = FALSE;
		u32ClkSkewInit = inp32(REG_CKDQSDS);
	}

	if(u32Hclk<=100000000)
	{//Low Freq
		outp32(REG_CKDQSDS, 0x008DDD00);
		outp32(REG_CLKDIV0, (inp32(REG_CLKDIV0) & ~(SYSTEM_INTEG|SYSTEM_FRACTM|SYSTEM_FRACTL)) | u32Clk0Div );
		outp32(REG_SDEMR, inp32(REG_SDEMR)  | DLLEN); //Disable DLL of SDRAM device

		outp32(REG_SDOPM,0x00078476);
	}
	else
	{//High Freq
		outp32(REG_CKDQSDS, u32ClkSkewInit);
		outp32(REG_CLKDIV0, (inp32(REG_CLKDIV0) & ~(SYSTEM_INTEG|SYSTEM_FRACTM|SYSTEM_FRACTL)) | u32Clk0Div );
		outp32(REG_SDOPM, inp32(REG_SDOPM) & ~(LOWFREQ | SEL_SCLKI));
		while(i--);
		outp32(REG_SDEMR, inp32(REG_SDEMR)  & ~DLLEN);  //Enable DLL of SDRAM device
		outp32(REG_SDOPM, inp32(REG_SDOPM) | AUTO_DQSPHASE);

		i = 0x3000;
		while(i--);
		for(j=0; j<2; j=j+1)
		{
			for(i=0; i<2; i=i+1)
			{

				if((j%2)==1)
				{
					outp32(REG_CKDQSDS, (inp32(REG_CKDQSDS) &~0x00FFFF00)|0x008EEE00);
				}
				else
				{
					if(i)
						outp32(REG_CKDQSDS, (inp32(REG_CKDQSDS) &~0x00FFFF00)|0x008AAA00);
					else
						outp32(REG_CKDQSDS, (inp32(REG_CKDQSDS) &~0x00FFFF00)|0x0088FF00);
				}
				outp32(REG_SDOPM, 0x04130476); //DQS_PHASE_RST
				outp32(REG_SDOPM, 0x04030476);
				z = inp32(0x0);			//Read DRAM
				if(i==0)
					x = inp32(REG_SDOPM) >>28;
				else
					y = inp32(REG_SDOPM) >>28;
				if(i==1)
				{
					if(x==y)
					{
					#if 0
						if(j==0)
							outp32(REG_CKDQSDS, 0x0088FF00);
					#endif
						break;
					}
				}
			}
			if(x==y)
				break;
		}
		outp32(REG_CKDQSDS, u32ClkSkewInit);
	}
}

INT32 sysSetSystemDivider(UINT32 u32Hclk, UINT32 u32Integ)
{
	UINT32   vram_base, aic_status = 0;
	UINT32 	u32RegLVR;
	VOID    (*wb_func)(UINT32, UINT32);

	/* Error Check */
	if( u32Integ>7 )
		return E_ERR_CLK;

	/* Disable LVR */
	u32RegLVR = inp32(REG_POR_LVRD);
	outp32(REG_POR_LVRD, 0x41);

	aic_status = inpw(REG_AIC_IMR);					//Disable interrupt
	outpw(REG_AIC_MDCR, 0xFFFFFFFF);

	vram_base = PD_RAM_BASE;
	/* ignore by NandLoader */
	/*
	memcpy((char *)((UINT32)_tmp_buf | 0x80000000),
			(char *)((UINT32)vram_base | 0x80000000),
			PD_RAM_SIZE);					//Backup RAM content
	*/
	memcpy((VOID *)((UINT32)vram_base | 0x80000000),
			(VOID *) (((UINT32)_sysClockDiv-(PD_RAM_START-PD_RAM_BASE)) | 0x80000000), 
			PD_RAM_SIZE);	

	vram_base = PD_RAM_START;
	wb_func = (void(*)(UINT32, UINT32)) vram_base;

	DBG_PRINTF("Jump to SRAM Hclk = %d\n", u32Hclk);
	wb_func(u32Hclk, u32Integ);

	//Restore VRAM
	/* ignore by NandLoader */
	/*
	vram_base = PD_RAM_BASE;
	memcpy((VOID *)((UINT32)vram_base | 0x80000000),
			(VOID *)((UINT32)_tmp_buf | 0x80000000),
			PD_RAM_SIZE);
	*/
	outpw(REG_AIC_MDCR, 0xFFFFFFFF);    	// Disable all interrupt
	outpw(REG_AIC_MECR, aic_status);    	// Restore AIC setting

	/* Restore LVR */
	outp32(REG_POR_LVRD, u32RegLVR);
	return Successful;
}




/*-----------------------------------------------------------------------------------------------------------
* Function: sysSetSystemClock
*
* Parameters:
*              u32PllValue - [in], PLL setting value
*
* Returns:
*      None
*
* Description:
*              To set the PLL control register.
*
*Note:
*		Switch systetm clock to external clock first.
*
*		refresh rate = REPEAT/Fmclk
*		1. Disable interrupt
*		2. Enter Self-refresh
*		3. Switch to external clock
*		4. Adjustment the sys divider.
*
*
*
*
*
*
-----------------------------------------------------------------------------------------------------------*/
void _sysClockSwitch(register E_SYS_SRC_CLK eSrcClk,
						register UINT32 u32Hclk,
						register UINT32 u32PllReg,
						register UINT32 u32SysDiv)
{
	UINT32 u32IntTmp, i, j, x, y, z;
	/* disable interrupt (I will recovery it after clock changed) */
	u32IntTmp = inp32(REG_AIC_IMR);
	outp32(REG_AIC_MDCR, 0xFFFFFFFE);

	if(bIsFirstReadClkSkew==TRUE)
	{/* Record the default value from IBR or NAND Loader */
		bIsFirstReadClkSkew = FALSE;
		u32ClkSkewInit = inp32(REG_CKDQSDS);
	}

	/* DRAM enter self refresh mode */
	//outp32(REG_SDCMD, inp32(REG_SDCMD) | (SELF_REF| REF_CMD));
	outp32(REG_SDCMD, (inp32(REG_SDCMD) & ~0x20) | 0x10);

	/* Switch to external clock and divider to 0*/
	outp32(REG_CLKDIV0, (inp32(REG_CLKDIV0) & ~(SYSTEM_INTEG | SYSTEM_FRACTM | SYSTEM_FRACTL | SYSTEM_S | SYSTEM_N0)) );
	if(eSrcClk==eSYS_EXT)
	{
		/* Fill system clock divider */
		outp32(REG_CLKDIV0, (inp32(REG_CLKDIV0) & ~(SYSTEM_INTEG|SYSTEM_FRACTM|SYSTEM_FRACTL|SYSTEM_S|SYSTEM_N0)) |
							u32SysDiv);
	}
	else if((eSrcClk==eSYS_APLL)|| (eSrcClk==eSYS_UPLL))
	{
		outp32(REG_CLKDIV0,  (inp32(REG_CLKDIV0) | 0x01));	//PLL/3 Safe consider
		if(1) //eSrcClk==eSYS_UPLL)
			outp32(REG_UPLLCON, u32PllReg);
		else
			outp32(REG_APLLCON, u32PllReg);
		__asm
		{
			mov 	r2, #10000
			mov		r1, #0
			mov		r0, #1
		loop1:	add 		r1, r1, r0
			cmp 	r1, r2
			bne		loop1
		}

		/* Fill system clock divider */
		outp32(REG_CLKDIV0, (inp32(REG_CLKDIV0) & ~(SYSTEM_INTEG|SYSTEM_FRACTM|SYSTEM_FRACTL|SYSTEM_S|SYSTEM_N0)) |
							u32SysDiv);

	}

	__asm
	{
		;mov 	r2, #20000
		mov 	r2, #20000
		mov		r1, #0
		mov		r0, #1
	loop2:	add 		r1, r1, r0
		cmp 	r1, r2
		bne		loop2
	}

	 /* DRAM escape self refresh mode */
	//outp32(REG_SDCMD, inp32(REG_SDCMD) & ~REF_CMD);
	outp32(0xB0003004,  0x20);

	__asm
	{
		mov 	r2, #60000
		mov		r1, #0
		mov		r0, #1
	loop3a:	add 		r1, r1, r0
		cmp 	r1, r2
		bne		loop3a
	}
#if 1
	if(u32Hclk>100000000)
	{//>100MHz
		for(j=0; j<2; j=j+1)
		{
			for(i=0; i<2; i=i+1)
			{

				if((j%2)==1)
				{
					outp32(REG_CKDQSDS, (inp32(REG_CKDQSDS) &~0x00FFFF00)|0x008EEE00);
				}
				else
				{
					if(i)
						outp32(REG_CKDQSDS, (inp32(REG_CKDQSDS) &~0x00FFFF00)|0x008AAA00);
					else
						outp32(REG_CKDQSDS, (inp32(REG_CKDQSDS) &~0x00FFFF00)|u32ClkSkewInit);
				}
				outp32(REG_SDOPM, 0x04130476); //DQS_PHASE_RST
				outp32(REG_SDOPM, 0x04030476);
				z = inp32(0x0);			//Read DRAM
				if(i==0)
					x = inp32(REG_SDOPM) >>28;
				else
					y = inp32(REG_SDOPM) >>28;
				if(i==1)
				{
					if(x==y)
					{
					#if 0
						if(j==0)
							outp32(REG_CKDQSDS, 0x0088FF00);
					#endif
						break;
					}
				}
			}
			if(x==y)
				break;
		}
		outp32(REG_CKDQSDS, u32ClkSkewInit);
	}
	else
	{//<=100000000
		outp32(REG_CKDQSDS, 0x008DDD00);
		outp32(REG_SDEMR, inp32(REG_SDEMR)  | DLLEN); //Disable DLL of SDRAM device
		outp32(REG_SDOPM,0x00078476);
	}
#endif

	outp32(REG_AIC_MECR, u32IntTmp);

}
void _sysClockSwitchStart(E_SYS_SRC_CLK eSrcClk,
						UINT32 u32Hclk,
						UINT32 u32RegPll,
						UINT32 u32SysDiv)
{
	UINT32   vram_base, aic_status = 0;
	BOOL bIsCacheState = FALSE;
	UINT32 u32CacheMode;
	
	VOID    (*wb_func)(E_SYS_SRC_CLK,
					UINT32,
					UINT32,
					UINT32);

	aic_status = inpw(REG_AIC_IMR);					//Disable interrupt
	outpw(REG_AIC_MDCR, 0xFFFFFFFF);

	vram_base = PD_RAM_BASE;
	memcpy((char *)((UINT32)_tmp_buf | 0x80000000),
			(char *)((UINT32)vram_base | 0x80000000),
			PD_RAM_SIZE);					//Backup RAM content
					
	memcpy((VOID *)((UINT32)vram_base | 0x80000000),
			(VOID *)( ((UINT32)_sysClockSwitch -(PD_RAM_START-PD_RAM_BASE)) | 0x80000000), 
			PD_RAM_SIZE);					//
	
	if(sysGetCacheState()==TRUE){
		DBG_PRINTF("Cache enable\n");	
		bIsCacheState = TRUE;		
		u32CacheMode = sysGetCacheMode();
		sysDisableCache(); 	
		sysFlushCache(I_D_CACHE);	
	}else{
		DBG_PRINTF("Cache disable\n");	
	}					
	//sysFlushCache(I_CACHE);
	
	vram_base = PD_RAM_START;	
	wb_func = (void(*)(E_SYS_SRC_CLK,
					UINT32,	//u32Hclk
					UINT32,
					UINT32)) vram_base;

	DBG_PRINTF("SYS_DIV = %x\n", u32SysDiv);
//	DBG_PRINTF("CPU_DIV = %x\n", u32CpuDiv);
//	DBG_PRINTF("APB_DIV = %x\n", u32ApbDiv);
	DBG_PRINTF("Jump to SRAM\n");
	wb_func(eSrcClk,
			u32Hclk,
			u32RegPll,
			u32SysDiv);

	if(bIsCacheState==TRUE)
		sysEnableCache(u32CacheMode);	
	DBG_PRINTF("Calibration Value = 0x%x\n", inp32(REG_SDOPM));
	//Restore VRAM
	vram_base = PD_RAM_BASE;	
	memcpy((VOID *)((UINT32)vram_base | 0x80000000),
			(VOID *)((UINT32)_tmp_buf | 0x80000000),
			PD_RAM_SIZE);

	outpw(REG_AIC_MDCR, 0xFFFFFFFF);    	// Disable all interrupt
	outpw(REG_AIC_MECR, aic_status);    	// Restore AIC setting
}

/*-----------------------------------------------------------------------------------------------------------
	Get the system clock divider.
	static UINT32 g_u32SysInteg, g_u32SysFract, g_u32PllPreDiv;
	DDR2/DDR system divider is only support integer. For example,
	Divider = 1. Integ part =0. and Fractional part =0.
	Divider = 2. Integ part =1. and Fractional part =0.
	Divider = 3. Integ part =2. and Fractional part =0.
	...
	Divider = 8. Integ part =7. and Fractional part =0.

	return
-----------------------------------------------------------------------------------------------------------*/
UINT32 _sysGetSystemClockDivider(E_SYS_SRC_CLK eSrcClk,
							UINT32 u32PllClk,
							UINT32 u32SysClk)
{
	UINT32 u32PllReg, i, j;
	UINT32 sysDiv;

	if(u32PllClk==u32SysClk)
	{//If u32PllClk==u32SysClk, Divider =0;
		g_u32SysInteg = 0;
		g_u32SysFract = 0;
		g_u32PllPreDiv = 0;
		return u32SysClk;
	}

	g_u32ExtClk = sysGetExternalClock();
	u32PllReg = _sysGetPLLControlRegister((g_u32ExtClk/1000), u32PllClk);
	if(eSrcClk == eSYS_EXT)
	{
		sysDiv = g_u32ExtClk/u32SysClk;
		g_u32PllPreDiv = 0;
		g_u32SysInteg = (UINT32)sysDiv-1;
		g_u32SysFract = 0;
	}
	else
	{/* Designer said if systerm divider = 2. It will be 1+100*0.01, and real chip not support  */
		sysDiv = u32PllClk/u32SysClk;
		for(i=1; i<=7; i=i+1)		//APLL/UPLL pre-divider
		{
			UINT32 u32Done=FALSE;
			for(j=1; j<8; j=j+1) 	//System divider
			{
				g_u32PllPreDiv = i-1;						/* PLL divider */
				u32SysClk = u32SysClk/(g_u32PllPreDiv+1);
				DBG_PRINTF("u32SysClk = %d\n",u32SysClk);
				sysDiv = (u32PllClk/i)/u32SysClk;			/* System divider */
				if(sysDiv<=8)
				{
					if(sysDiv==1)
					{//Both 0 mean sys divider = 1
						g_u32SysInteg = 0;
						g_u32SysFract = 0;
					}
					else
					{
						g_u32SysInteg = sysDiv-1;
						g_u32SysFract = 0;
					}
					DBG_PRINTF("Done\n");
					u32Done = TRUE;
					break;
				}
			}
			if(u32Done==TRUE)
					break;
		}
	}

	//sysDiv = (g_u32PllPreDiv+1)*(g_u32SysInteg + ((float)g_u32SysFract)/100);
	sysDiv = (g_u32PllPreDiv+1)*(g_u32SysInteg + (g_u32SysFract)/100);
	DBG_PRINTF("System div integ= %d, Fraction = %d, Normalized system = %d\n", g_u32SysInteg, g_u32SysFract, sysDiv);
	DBG_PRINTF("PLL = %d ,  System clock = %d", u32PllClk, u32PllClk/(sysDiv+1));
	return (UINT32)(u32PllClk/(sysDiv+1));
}
/*-----------------------------------------------------------------------------------------------------------
* Function: sysSetSystemClock
*
* Parameters:
*              u32PllHz 		- [in], Specified PLL Clock
*              u32SysHz 	- [in], Specified SYS Clock
* Returns:
*      Error Code
*
* Description:
*              To set the PLL clock and system clock
*
-----------------------------------------------------------------------------------------------------------*/
ERRCODE
sysSetSystemClock(E_SYS_SRC_CLK eSrcClk,		// Specified the system clock come from external clock, APLL or UPLL
				UINT32 u32PllHz,			// Specified the APLL/UPLL clock
				UINT32 u32SysHz			// Specified the system clock
	)
{
	UINT32 u32RegPll;
	UINT32 g_u32SysDiv;
	UINT32 u32RegSysDiv;
	UINT32 u32RegLVR;

	g_u32ExtClk = sysGetExternalClock();

	/* Disable LVR */
	u32RegLVR = inp32(REG_POR_LVRD);
	outp32(REG_POR_LVRD, 0x41);

	/* Error Check */
	if((u32PllHz%u32SysHz)!=0)
		return E_ERR_CLK;		//System divider for integrate and fractional part is not workable for DDR2/DDR  */
	if((u32PllHz/u32SysHz)>8)
		return E_ERR_CLK;

	switch(eSrcClk)
	{
		case eSYS_EXT:
		    g_u32SysClkSrc =  eSYS_EXT;
			DBG_PRINTF("Switch to external \n");
			g_u32SysDiv = u32PllHz/u32SysHz;
			_sysGetSystemClockDivider(eSrcClk, u32PllHz, u32SysHz);	/* g_u32PllPreDiv = 0 after here*/
			break;
		case eSYS_APLL:
			g_u32SysClkSrc = eSYS_APLL;
			g_u32ApllHz = u32PllHz;
			g_i32REG_APLL = _sysGetPLLControlRegister((g_u32ExtClk/1000), g_u32ApllHz);
			if(g_i32REG_APLL==-1)
				return E_ERR_CLK;
			_sysGetSystemClockDivider(eSrcClk, u32PllHz, u32SysHz);
//			DBG_PRINTF("APLL register = 0x%x\n", g_u32REG_APLL);
			break;
		case eSYS_UPLL:
			g_u32SysClkSrc = eSYS_UPLL;
			g_u32UpllHz = u32PllHz;
			g_i32REG_UPLL = _sysGetPLLControlRegister((g_u32ExtClk/1000), g_u32UpllHz);
			//printf("UPLL register = %d\n", g_i32REG_UPLL);
			if(g_i32REG_UPLL==-1)
				return E_ERR_CLK;
			_sysGetSystemClockDivider(eSrcClk, u32PllHz, u32SysHz);
//			DBG_PRINTF("UPLL register = 0x%x\n", g_u32REG_UPLL);
			break;
		default:
			return E_ERR_CLK;
	}
	if(eSrcClk==eSYS_UPLL)
	{
		u32RegPll = g_i32REG_UPLL;
		DBG_PRINTF("UPLL  = %d\n", u32RegPll);
	}
	else if(eSrcClk==eSYS_APLL)
	{
		u32RegPll = g_i32REG_APLL;
		DBG_PRINTF("APLL = %d\n", u32RegPll);
	}

	u32RegSysDiv = (g_u32SysInteg<<29) | (((g_u32SysFract&0x60)>>5)<<27) | ((g_u32SysFract&0x1F)<<6) | (eSrcClk<<3) | (g_u32PllPreDiv);
	DBG_PRINTF("system Divider = 0x%x\n", u32RegSysDiv);
	_sysClockSwitchStart(eSrcClk,
						u32SysHz/2,
						u32RegPll,
						u32RegSysDiv);
	/* Restore LVR */
	outp32(REG_POR_LVRD, u32RegLVR);

	return Successful;
}
/*
	The function will set the CPU clock below the specified CPU clock
	And return the real clock of CPU.
	!!! Assume change CPU clock is workable in SDRAM!!!
*/
INT32 sysSetCPUClock(UINT32 u32CPUClock)
{
	UINT32 CPUClock, u32CPUDiv;
	UINT32 u32SysClock = sysGetSystemClock();
	if(u32CPUClock> u32SysClock)
		return E_ERR_CLK;

	/* u32CPUDiv must be multiple of 2 */
	u32CPUDiv = u32SysClock/u32CPUClock;
	if(u32CPUDiv==1)
		u32CPUDiv = 0;
	else if(u32CPUDiv%2==0)
		u32CPUDiv=u32CPUDiv-1;	/* u32CPUDiv = 2, 4 ,6, .... Fill to register */
								/* Otherwise CPU speed is slower than specified speed */
	if(u32CPUDiv>16)
		return E_ERR_CLK;

	outp32(REG_CLKDIV4, (inp32(REG_CLKDIV4) &~CPU_N) | u32CPUDiv);
	CPUClock = u32SysClock/(u32CPUDiv+1);
	return CPUClock;
}
/*
	 HCLK1 clcok is always equal to CPUCLK or CPUCLK/2 depends on CPU_N
	 INT32 sysSetHCLK1Clock(UINT32 u32HCLK1Clock)
	{

	}
*/

/*
	Set APB clcok
*/
INT32 sysSetAPBClock(UINT32 u32APBClock)
{
	UINT32 u32APBDiv;
	UINT32 u32HCLK1Clock;
	u32HCLK1Clock = sysGetHCLK1Clock();
	if(u32APBClock> u32HCLK1Clock)
		return E_ERR_CLK;
	u32APBDiv = (u32HCLK1Clock/u32APBClock)-1;
	if(u32APBDiv>7)
		return E_ERR_CLK;

	outp32(REG_CLKDIV4, (inp32(REG_CLKDIV4) & ~APB_N)|
						((u32APBDiv<<8) | CHG_APB)); /* CHG_APB: Enable change APB clock */
													/* CHG_APB will auto clear */
	return (u32HCLK1Clock/(u32APBDiv+1));
}



BOOL bIsAPLLInitialize = FALSE;

UINT32 sysGetExternalClock(void)
{
	if((inp32(REG_CHIPCFG) & 0xC) == 0x8)	//Different with FA93
		g_u32ExtClk = 27000000;
	else
		g_u32ExtClk = 12000000;
	return g_u32ExtClk;
}
/*
	Get system clcok
*/
UINT32 sysGetSystemClock(void)
{
	UINT32 u32Fin;
	UINT32 u32SysSrc, u32PllPreDiv;
	UINT32 u32Integ, u32Fract;
	UINT32 sysDiv;
	u32Fin = sysGetExternalClock();
	u32SysSrc = (inp32(REG_CLKDIV0) & SYSTEM_S)>>3;
	u32PllPreDiv = (inp32(REG_CLKDIV0) & SYSTEM_N0)+1;
	switch(u32SysSrc)
	{
		case 0:
			u32SysSrc = u32Fin;	 									break;
		case 2:
			u32SysSrc = sysGetPLLOutputHz(eSYS_APLL, u32Fin )/u32PllPreDiv;	break;
		case 3:
			u32SysSrc = sysGetPLLOutputHz(eSYS_UPLL, u32Fin)/u32PllPreDiv;	break;
	}

	u32Integ = (inp32(REG_CLKDIV0) & SYSTEM_INTEG)>>29;
	u32Fract = ((inp32(REG_CLKDIV0) & SYSTEM_FRACTM)>>27)<<5;
	u32Fract = u32Fract | ((inp32(REG_CLKDIV0) & SYSTEM_FRACTL)>>6);
	if(u32Fract==0)
	{//0 means 1, 1 means 2. 2 means 3
		sysDiv = 	u32Integ+1;
		if(sysDiv==0)
			sysDiv = 1;
	}
	if(u32Fract==100)
	{//0 means 1, 1 means 1. 2 means 2.
		sysDiv = 	u32Integ;
		if(sysDiv==0)
			sysDiv = 1;
	}
	return (u32SysSrc/sysDiv);
}
/*
	Get CPU clcok
*/
UINT32 sysGetCPUClock()
{
	UINT32 u32SysClock = sysGetSystemClock();
	UINT32 CPUClock;
	CPUClock = u32SysClock/((inp32(REG_CLKDIV4) & CPU_N)+1);
#ifdef REAL_CHIP
	return (UINT32)CPUClock;
#else
	return(sysGetExternalClock());
#endif
}
/*
	Get HCLK1 clcok
*/
UINT32 sysGetHCLK1Clock()
{
	UINT32 u32CPUClock;
	UINT32 u32CPUDiv;
	u32CPUClock = sysGetCPUClock();
	u32CPUDiv = inp32(REG_CLKDIV4) & CPU_N;
#ifdef REAL_CHIP
	if(u32CPUDiv == 0)
		return u32CPUClock/2;
	else
		return u32CPUClock;
#else
	return(sysGetExternalClock());
#endif
}
/*
	Get APB clcok
*/
UINT32 sysGetAPBClock()
{
	UINT32 u32APBDiv;
	u32APBDiv = ((inp32(REG_CLKDIV4) & APB_N)>>8) +1;
#ifdef REAL_CHIP
	return (sysGetHCLK1Clock()/u32APBDiv);
#else
	return(sysGetExternalClock());
#endif

}


/*-----------------------------------------------------------------------------------------------------------
*	The Function is used to set the other PLL which is not the system clock source.
*	If system clock source come from eSYS_UPLL. The eSrcClk only can be eSYS_APLL
*	And if specified PLL not meet some costraint, the funtion will search the near frequency and not over the specified frequency
*
* 	Paramter:
*		eSrcClk: eSYS_UPLL or eSYS_APLL
*		u32TargetKHz: The specified frequency. Unit:Khz.
*
*	Return:
*		The specified PLL output frequency really.
-----------------------------------------------------------------------------------------------------------*/
UINT32 sysSetPllClock(E_SYS_SRC_CLK eSrcClk, UINT32 u32TargetHz)
{
	UINT32 u32PllReg, u32PllOutFreqHz, u32FinHz;


	u32FinHz = sysGetExternalClock();


	//Specified clock is system clock,  return working frequency directly.
	if( (inp32(REG_CLKDIV0) & SYSTEM_S)== 0x18 )
	{//System from UPLL
		if(eSrcClk==eSYS_UPLL)
		{
			u32PllOutFreqHz = sysGetPLLOutputHz(eSrcClk, u32FinHz);
			return u32PllOutFreqHz;
		}
	}
	if( (inp32(REG_CLKDIV0) & SYSTEM_S)== 0x10 )
	{//System from APLL
		if(eSrcClk==eSYS_APLL)
		{
			u32PllOutFreqHz = sysGetPLLOutputHz(eSrcClk, u32FinHz);
			return u32PllOutFreqHz;
		}
	}
	//Specified clock is not system clock,
	u32PllReg = _sysGetPLLControlRegister((u32FinHz/1000), u32TargetHz);
	if(eSrcClk == eSYS_APLL)
		outp32(REG_APLLCON, u32PllReg);
	else if(eSrcClk == eSYS_UPLL)
		outp32(REG_UPLLCON, u32PllReg);
	if((eSrcClk == eSYS_APLL) || (eSrcClk == eSYS_UPLL))
	{
		u32PllOutFreqHz = sysGetPLLOutputHz(eSrcClk, u32FinHz);
		if(eSrcClk == eSYS_APLL)
			g_u32ApllHz = u32PllOutFreqHz;
		else
			g_u32UpllHz = u32PllOutFreqHz;
		return u32PllOutFreqHz;
	}
	else
		return 0;

}

