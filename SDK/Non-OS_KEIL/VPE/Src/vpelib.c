/***************************************************************************
 *                                                                         *
 * Copyright (c) 2009 Nuvoton Technology. All rights reserved.             *
 *                                                                         *
 ***************************************************************************/
 
/****************************************************************************
 * 
 * FILENAME
 *     vpelib.c
 *
 * VERSION
 *     1.0
 *
 * DESCRIPTION
 *     The header file of w55fa95 vpe library.
 *
 * DATA STRUCTURES
 *     None
 *
 * FUNCTIONS
 *     None
 *
 *
 * REMARK
 *     None
 **************************************************************************/

#include "w55fa95_reg.h"
#include "wblib.h"
#include "w55fa95_vpe.h"

BOOL bIsVPEPageFault = FALSE;
BOOL bIsVPEPageMiss = FALSE;
extern PUINT32 pu32TLBEntry;
//#define DBG_PRINTF(...)
#define DBG_PRINTF	sysprintf

extern UINT32 vpeFindMatchAddr(UINT32 u32PageFaultVirAddr, PUINT32 pu32ComIdx);
extern void vpeSetTtbEntry(UINT32 u32Entry, UINT32 u32Level1Entry);

void vpePageFaultCallback(void)
{
	UINT32 u32PageFaultVirtAddr; 
	UINT32 u32ComIdx;
	UINT32 u32PageFaultIdx; 
	bIsVPEPageFault = TRUE;
	
	u32PageFaultVirtAddr = inp32(REG_VMMU_PFTVA);		
	u32PageFaultIdx = vpeFindMatchAddr(u32PageFaultVirtAddr, &u32ComIdx);		
	vpeSetTtbEntry(u32ComIdx, *(pu32TLBEntry+u32PageFaultIdx));			
	DBG_PRINTF("***********************************************\n"); 
	DBG_PRINTF("Update Table index 		= %d\n", u32ComIdx); 
	DBG_PRINTF("Page Fault Virtual Address  	= 0x%x\n", u32PageFaultVirtAddr);														
	DBG_PRINTF("MMU[%d]				= 0x%x\n", u32PageFaultIdx, *(pu32TLBEntry+u32PageFaultIdx));
	outp32(REG_VMMU_CMD, inp32(REG_VMMU_CMD) | RESUME);	//resume		
}	
void vpePageMissCallback(void)
{
	UINT32 u32PageMissVirtuAddr; 
//	UINT32 u32PagMissPhyAddr;
	UINT32 u32ComIdx;
	UINT32 u32PageMissIdx; 
	bIsVPEPageMiss = TRUE;
	
	u32PageMissVirtuAddr = inp32(REG_VMMU_PFTVA); //In linux must converstion to physical address
	u32PageMissIdx = vpeFindMatchAddr(u32PageMissVirtuAddr, &u32ComIdx);		
	vpeSetTtbEntry(u32ComIdx, *(pu32TLBEntry+u32PageMissIdx));	
	DBG_PRINTF("---------------------Page Miss-----------------------------------------------\n"); 
	DBG_PRINTF("Update Table index 		= %d\n", u32ComIdx); 
	DBG_PRINTF("Page Fault Virtual Address  	= 0x%x\n", u32PageMissVirtuAddr);
	DBG_PRINTF("MMU[%d]				= 0x%x\n", u32PageMissIdx, *(pu32TLBEntry+u32PageMissIdx));
	outp32(REG_VMMU_CMD, inp32(REG_VMMU_CMD) | RESUME);	//resume	
}	
/* 
	
*/
PFN_VPE_CALLBACK (vpeIntHandlerTable)[6]={0};
void vpeInIntHandler(void)
{
	UINT32 u32VpeInt;
	u32VpeInt = inp32(REG_VPE_INTS)&0x3F;
	/*
	if((u32VpeInt&LL_INTS)==LL_INTS)
	{
		sysprintf("Page Misss interrupt\n");
		while(1);
	}
	*/
	if( (u32VpeInt&VP_INTS) == VP_INTS)
	{//VPE complete
		if(vpeIntHandlerTable[0]!=0)	
			vpeIntHandlerTable[0]();			/* Clear Interrupt */
		outp32(REG_VPE_INTS, (u32VpeInt & ~(TA_INTS | DE_INTS | MB_INTS | LL_INTS | PFT_INTS)));			
		return;
	}	
	if( (u32VpeInt&PFT_INTS) == PFT_INTS)
	{//Page Fault
		outp32(REG_VPE_INTS, (u32VpeInt & ~(TA_INTS | DE_INTS | MB_INTS | LL_INTS | VP_INTS)));	/* Clear Interrupt */	
		#if 0
		if(vpeIntHandlerTable[1]!=0)	
			vpeIntHandlerTable[1]();					
		#else
		vpePageFaultCallback();
		#endif	
		return;	
	}	
	if( (u32VpeInt&LL_INTS) == LL_INTS)
	{//Page Missing
		outp32(REG_VPE_INTS, (u32VpeInt & ~(TA_INTS | DE_INTS | MB_INTS | PFT_INTS |VP_INTS)));	/* Clear Interrupt */
		#if 0
		if(vpeIntHandlerTable[2]!=0)	
			vpeIntHandlerTable[2]();			
		#else
		vpePageMissCallback();
		#endif	
		return;
	}	
	if( (u32VpeInt&MB_INTS) == MB_INTS)
	{//MB complete, Invalid due to JPEG OTF removed 
		if(vpeIntHandlerTable[3]!=0)	
			vpeIntHandlerTable[3]();			
		outp32(REG_VPE_INTS, (u32VpeInt & ~(TA_INTS | DE_INTS | LL_INTS | PFT_INTS |VP_INTS)));	/* Clear Interrupt */	
		outp32(REG_VPE_TG, inp32(REG_VPE_TG)&~0x01);
	}
	if( (u32VpeInt&DE_INTS) == DE_INTS)
	{//Decode error,  Invalid due to JPEG OTF removed 
		if(vpeIntHandlerTable[4]!=0)	
			vpeIntHandlerTable[4]();			/* Clear Interrupt */
		outp32(REG_VPE_INTS, (u32VpeInt & ~(TA_INTS | MB_INTS| LL_INTS | PFT_INTS |VP_INTS)));		
	}
	if( (u32VpeInt&TA_INTS) == TA_INTS)
	{//DMA abort
		if(vpeIntHandlerTable[5]!=0)	
			vpeIntHandlerTable[5]();			/* Clear Interrupt */
		outp32(REG_VPE_INTS, (u32VpeInt & ~(DE_INTS | MB_INTS| LL_INTS | PFT_INTS |VP_INTS)));		
	}
}
/*-----------------------------------------------------------------------------------------------------------
*	The Function open VPE driver. 
*	And if specified PLL not meet some costraint, the funtion will search the near frequency and 
*	not over the specified frequency
*	
*	1. Enable clock 
*	2. Set correct clock
*	3. Set multiple pin function
*	3. Reset IP
*	 
*	Return: 
*		Error code or Successful                                                                                                        
-----------------------------------------------------------------------------------------------------------*/

static INT vpeOpenCount = 0;
ERRCODE vpeOpen(void)
{
	if(vpeOpenCount>0)
	{
		return ERR_VPE_OPEN;
	}
	outp32(REG_AHBCLK, inp32(REG_AHBCLK) |GVE_CKE | HCLK3_CKE | VPE_CKE | GE_CKE );
	outp32(REG_AHBIPRST, inp32(REG_AHBIPRST) | VPE_RST);
	outp32(REG_AHBIPRST, inp32(REG_AHBIPRST) & ~VPE_RST);	
	outp32(REG_VPE_CMD, inp32(REG_VPE_CMD) | BUSRT | BIT13 | BIT12);	//Burst write- Dual buffer //Lost Block 	
	vpeOpenCount = vpeOpenCount+1;	
	sysInstallISR(IRQ_LEVEL_1, 
						IRQ_VPE, 
						(PVOID)vpeInIntHandler);						
	sysEnableInterrupt(IRQ_VPE);		
	return Successful;  
}
/*-----------------------------------------------------------------------------------------------------------
*	The Function close VPE driver. 
*	And if specified PLL not meet some costraint, the funtion will search the near frequency and 
*	not over the specified frequency
*	
*	 
*	Return: 
*		Error code or Successful                                                                                                        
-----------------------------------------------------------------------------------------------------------*/
ERRCODE vpeClose(void)
{
	if(vpeOpenCount==1)
		vpeOpenCount = vpeOpenCount-1;			
	else	
		return ERR_VPE_CLOSE;		
	return Successful;  
}

/*-----------------------------------------------------------------------------------------------------------
*	The Function install call back function
*   
*	
*	 
*	Return: 
*		Error code or Successful                                                                                                        
-----------------------------------------------------------------------------------------------------------*/
ERRCODE 
vpeInstallCallback(
	E_VPE_INT_TYPE eIntType, 
	PFN_VPE_CALLBACK pfnCallback,
	PFN_VPE_CALLBACK* pfnOldCallback
	)
{
	if(eIntType == VPE_INT_COMP)
	{//VPE complete
		*pfnOldCallback = vpeIntHandlerTable[0];
		vpeIntHandlerTable[0] = (PFN_VPE_CALLBACK)(pfnCallback);
	}	
	else if(eIntType == VPE_INT_PAGE_FAULT)
	{//Page fault
		*pfnOldCallback = vpeIntHandlerTable[1];
		vpeIntHandlerTable[1] = (PFN_VPE_CALLBACK)(pfnCallback);
	}
	else if(eIntType == VPE_INT_PAGE_MISS)
	{
		*pfnOldCallback = vpeIntHandlerTable[2];
		vpeIntHandlerTable[2] = (PFN_VPE_CALLBACK)(pfnCallback);
	}	
	else if(eIntType == VPE_INT_MB_COMP)
	{
		*pfnOldCallback = vpeIntHandlerTable[3];
		vpeIntHandlerTable[3] = (PFN_VPE_CALLBACK)(pfnCallback);
	}	
	else if(eIntType == VPE_INT_MB_ERR)
	{
		*pfnOldCallback = vpeIntHandlerTable[4];
		vpeIntHandlerTable[4] = (PFN_VPE_CALLBACK)(pfnCallback);
	}	
	else if(eIntType == VPE_INT_DMA_ERR)
	{
		*pfnOldCallback = vpeIntHandlerTable[5];
		vpeIntHandlerTable[5] = (PFN_VPE_CALLBACK)(pfnCallback);
	}	
	else
		return E_VPE_INVALID_INT;			
	return Successful;	
}

ERRCODE 
vpeEnableInt(
	E_VPE_INT_TYPE eIntType
	)
{
	UINT32 u32IntEnable = inp32(REG_VPE_CMD);
	
	if(eIntType>VPE_INT_MB_COMP)
			return E_VPE_INVALID_INT; 
	
	outp32(REG_VPE_CMD, u32IntEnable |(1<<eIntType));	
	return Successful;
}	
ERRCODE 
vpeDisableInt(
	E_VPE_INT_TYPE eIntType
	)
{
	UINT32 u32IntEnable = inp32(REG_VPE_CMD);
	
	if(eIntType>VPE_INT_MB_COMP)
			return E_VPE_INVALID_INT; 
	
	outp32(REG_VPE_CMD, u32IntEnable& ~(1<<eIntType));	
	return Successful;
}

