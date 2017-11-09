/***************************************************************************
 *                                                                         *
 * Copyright (c) 2009 Nuvoton Technology. All rights reserved.             *
 *                                                                         *
 ***************************************************************************/
 
/****************************************************************************
 * 
 * FILENAME
 *     vpe_io.c
 *
 * VERSION
 *     1.0
 *
 * DESCRIPTION
 *     The file for io control
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
#include <stdlib.h>
#include <stdio.h>
#include "w55fa95_reg.h"
#include "wblib.h"
#include <string.h>
#include "w55fa95_vpe.h"	//Export

UINT32 u32VpeYBufAddr, u32VpeUBufAddr, u32VpeVBufAddr, u32VpeDstBufAddr; 

UINT16 g_u16SrcWidth, g_u16SrcHeight; 		//Src or Dst dimension. 
UINT16 g_u16DstWidth, g_u16DstHeight; 	
UINT32 g_u16SrcLOffset, g_u16SrcROffset; 	//Src or Dst offset
UINT32 g_u16DstLOffset, g_u16DstROffset; 	
UINT32 u32SrcSize, u32DstSize;
UINT32 u32RotDir;
UINT32 u32SrcComPixelWidth, u32DstComPixelWidth;
PUINT32 pu32TLBEntry;
#define MAX_COUNT				(16)		//Assume max is 16MB
#define MAX_COMPONENT_ENTRY	(2)		//Each component has 2 entry

/*
typedef struct tagEntryInfo
{
	BOOL bIsInvalid;
	UINT32 u32Count;
}S_ENTRY_INFO;
S_ENTRY_INFO sEntryInfo[4][MAX_COUNT] = {0};
*/

void vpeGetReferenceVirtualAddress(
	PUINT32 pu32YBufAddr,
	PUINT32 pu32UBufAddr,
	PUINT32 pu32VBufAddr,
	PUINT32 pu32DstBufAddr)	
{
	*pu32YBufAddr = u32VpeYBufAddr;
	*pu32UBufAddr = u32VpeUBufAddr;
	*pu32VBufAddr = u32VpeVBufAddr;
	*pu32DstBufAddr = u32VpeDstBufAddr;	
}

/*====================================================
	return 
====================================================*/

void swap(UINT32 *x,UINT32 *y)
{
   int temp;
   temp = *x;
   *x = *y;
   *y = temp;
}
void bublesort(UINT32* list, int n)
{
   int i,j;
   for(i=0;i<(n-1);i++)
      for(j=0;j<(n-(i+1));j++)
             if(list[j] > list[j+1])
                    swap(&(list[j]),&(list[j+1]));
}
/*
void printlist(int list[],int n)
{
   int i;
   for(i=0;i<n;i++)
      printf("%d\t",list[i]);
}
*/
void srcComponent(UINT32 u32CompentIdx, PUINT32 pu32Num, PUINT32 pu32Den)
{
	UINT32 u32Fmt;
	u32Fmt = (inp32(REG_VPE_CMD) & BLOCK_SEQ) >> 20;	
	switch(u32Fmt)
	{
		case VPE_SRC_PLANAR_YONLY:	*pu32Num =1; *pu32Den=1;	break; 
		case VPE_SRC_PLANAR_YUV420:	
		case VPE_SRC_PLANAR_YUV411:	if(u32CompentIdx==0)	
									{
										*pu32Num =1; *pu32Den=1;	
									}	
									else if(u32CompentIdx==1)	
									{
										*pu32Num =1; *pu32Den=4;	
									}	
									else if(u32CompentIdx==2)	
									{
										*pu32Num =1; *pu32Den=4;		
									}	
									break;													
		case VPE_SRC_PLANAR_YUV422:										
		case VPE_SRC_PLANAR_YUV422T:	if(u32CompentIdx==0)	
									{
										*pu32Num =1; *pu32Den=1;	
									}	
									else if(u32CompentIdx==1)	
									{
										*pu32Num =1; *pu32Den=2;	
									}	
									else if(u32CompentIdx==2)	
									{
										*pu32Num =1; *pu32Den=2;		
									}	
									break;						
	
		case VPE_SRC_PLANAR_YUV444:	*pu32Num =1; *pu32Den=1;	break;		
		
		case VPE_SRC_PACKET_YUV422:
		case	VPE_SRC_PACKET_RGB555:
		case VPE_SRC_PACKET_RGB565:	*pu32Num =2; *pu32Den=1;	break;
			
		case VPE_SRC_PACKET_RGB888:	*pu32Num =4; *pu32Den=1;	break;
	}
}
void dstComponent(UINT32 u32CompentIdx, PUINT32 pu32Num, PUINT32 pu32Den)
{
	UINT32 u32Fmt;
	u32Fmt =  (inp32(REG_VPE_CMD) & DEST) >>24;
	switch(u32Fmt)
	{
		case VPE_DST_PACKET_YUV422:
		case	VPE_DST_PACKET_RGB555:
		case VPE_DST_PACKET_RGB565:	*pu32Num =2; *pu32Den=1;	break;
			
		case VPE_DST_PACKET_RGB888:	*pu32Num =4; *pu32Den=1;	break;
	}
}

#if 1
BOOL LRUEntry[4];
#else
BOOL bIsSrcEntry0;
BOOL bIsSrcEntry1;
BOOL bIsSrcEntry2;
BOOL bIsDstEntry0;
#endif
void InitLRUTable(void)
{	
	UINT32 u32Idx;
#if 1
	for(u32Idx=0;u32Idx<4;u32Idx=u32Idx+1)
		LRUEntry[u32Idx] =  0;
#else	
	LRUEntry[0] =  1;	//Next will update 4 Y
	LRUEntry[1] =  1;	//Next will update 5 U
	LRUEntry[2] =  1;	//Next will update 6 V
	LRUEntry[3] =  0;	//Next will update 3 PAC	
#endif										
}
/*
	According to virtual address of page fault. 
	conversion to physical address 
	return to mmusectiontable index
*/
UINT32 Search(UINT32 u32PageFaultVirAddr)
{
	UINT32 u32MMUIdx;
	u32MMUIdx = u32PageFaultVirAddr /0x100000;
	return u32MMUIdx;
}
UINT32 vpeFindMatchAddr(
	UINT32 u32PageFaultVirAddr,
	PUINT32 pu32ComIdx			/* Indicate which component(ebtry) page fault. To update the new entry according to page fault address */
	)
{
	//UINT32 u32YBufAddr, u32UBufAddr,  u32VBufAddr, u32DstBufAddr;
	UINT32 u32BufAddr[4];
	UINT32 u32Offset[4];
	UINT32 u32Tmp[4];
	UINT32 u32Idx=0;
	UINT32 u32MMUIdx;
	
	vpeGetReferenceVirtualAddress(&(u32BufAddr[0]), &(u32BufAddr[1]), &(u32BufAddr[2]), &(u32BufAddr[3]));
	do
	{
		if(u32BufAddr[u32Idx]<=u32PageFaultVirAddr)		/* Report page fault address must be >= pattern start address */				
			u32Offset[u32Idx] = u32PageFaultVirAddr - u32BufAddr[u32Idx];				
		else
			u32Offset[u32Idx]  = 0x7FFFFFFF;
		u32Idx = u32Idx+1;
	}while(u32Idx<4);
	memcpy(u32Tmp, u32Offset, 16);				/* 4 component with word length */
	bublesort(u32Offset, 4);						/* Sorting, the min value will be index-0 */ 
	u32Idx=0;
	while(u32Offset[0]!=u32Tmp[u32Idx])			/* Compare */
		u32Idx=u32Idx+1;						/* u32Idx will be the component index. 0==> Y, 1==>U. 2==>V, 3==>Dst */
		
		
	/* 	LRU */	
	if(LRUEntry[u32Idx]==0)	
		LRUEntry[u32Idx] = 1;
	else
	{
		LRUEntry[u32Idx] = 0;
		u32Idx = u32Idx+4;
	}
	*pu32ComIdx = u32Idx;			
	sysprintf("Component Index = %d\n", u32Idx);	
	u32MMUIdx = Search(u32PageFaultVirAddr);
	sysprintf("MMU Table Inx = %d\n", u32MMUIdx);	
	return u32MMUIdx;							/* Report level 1 entry index */
}


void vpeHostOperation(
	E_VPE_HOST eHost,
	E_VPE_CMD_OP eOper	
	)
{
	outp32(REG_VPE_CMD, (inp32(REG_VPE_CMD)&~(HOST_SEL|OPCMD))|
						(((eHost<<4)&HOST_SEL) |
						((eOper<<16)&OPCMD))  );							 
}
/*==========================================================================
	MMU disable, u32YPacAddr, u32UAddr, u32VAddr is physical address.
	--------------------------------------------------------------------------
	MMU enable, u32YPacAddr, u32UAddr, u32VAddr are virtual address.
==========================================================================*/
void vpeSetSrcBufAddr(		
	UINT32 u32YPacAddr,
	UINT32 u32UAddr,
	UINT32 u32VAddr)
{		
	outp32(REG_VPE_PLYA_PK, u32YPacAddr);
	outp32(REG_VPE_PLUA, u32UAddr);
	outp32(REG_VPE_PLVA, u32VAddr);	
	u32VpeYBufAddr = u32YPacAddr; 
	u32VpeUBufAddr = u32UAddr; 
	u32VpeVBufAddr = u32VAddr;
}
void vpeSetDstBufAddr(UINT32 u32PacAddr)
{	
	outp32(REG_VPE_DEST_PK, u32PacAddr);
	u32VpeDstBufAddr = u32PacAddr;
}
ERRCODE vpeSetFmtOperation(
	E_VPE_SRC_FMT eSrcFmt,
	E_VPE_DST_FMT eDstFmt,
	E_VPE_CMD_OP eOper)
{	
	
	UINT32 u32BlockSeq=0;
	if(eDstFmt>VPE_DST_PACKET_RGB888)
			return ERR_VPE_DST_FMT; 
	if(eSrcFmt>VPE_SRC_PACKET_RGB888)
			return ERR_VPE_SRC_FMT;	
	if(eOper>VPE_DDA_SCALE)
			return ERR_VPE_OP;	
			
	switch(eSrcFmt)		
	{
		
		case VPE_SRC_PLANAR_YUV420: 
		//case VPE_SRC_PLANAR_YUV400: 				// Y Only		
									u32BlockSeq=1; break;
		case VPE_SRC_PLANAR_YUV411:	u32BlockSeq=2; break;								
		case VPE_SRC_PLANAR_YUV422: 	u32BlockSeq=3; break;	
		case VPE_SRC_PLANAR_YUV422T: 	u32BlockSeq=5; break;	
		case VPE_SRC_PLANAR_YUV444: 	u32BlockSeq=9; break;			
		case VPE_SRC_PACKET_YUV422:	
		case VPE_SRC_PACKET_RGB555:
		case VPE_SRC_PACKET_RGB565:
		case VPE_SRC_PACKET_RGB888:	u32BlockSeq=0; break;			
	}	
#if 0	
	if(eSrcFmt==VPE_SRC_PLANAR_YUV400)
		u32BlockSeq=0;
			
	outp32(REG_VPE_CMD, (inp32(REG_VPE_CMD) & ~BLOCK_SEQ) |
				((u32BlockSeq<<20)&BLOCK_SEQ));
	if(eSrcFmt!=VPE_SRC_PLANAR_YUV422T)
	{//Planar YUV422 Treanpose doesn't map to source format. 							
		outp32(REG_VPE_CMD, (inp32(REG_VPE_CMD) & ~SORC) |
					((eSrcFmt<< 28) & SORC));		
	}	
#else
	outp32(REG_VPE_CMD, (inp32(REG_VPE_CMD) & ~BLOCK_SEQ) |
				((eSrcFmt<< 20) & BLOCK_SEQ));					
#endif				
				
	outp32(REG_VPE_CMD, (inp32(REG_VPE_CMD) & ~DEST) |
					((eDstFmt<< 24) & DEST));			

	return Successful;		
}
void vpeSetSrcOffset(
	UINT16 u16LeftOff, 
	UINT16 u16RightOff)
{//Pixel unit
	outp32(REG_VPE_SLORO, (inp32(REG_VPE_SLORO) & ~(SRCLLO|SRCRLO)) |
					(((u16LeftOff<< 16) & SRCLLO) | (u16RightOff &SRCRLO)) );
}
void vpeSetDstOffset(
	UINT16 u16LeftOff, 
	UINT16 u16RightOff)
{//Pixel unit
	outp32(REG_VPE_DLORO, (inp32(REG_VPE_DLORO) & ~(DSTLLO|DSTRLO)) |
					(((u16LeftOff<< 16) & DSTLLO) | (u16RightOff &DSTRLO)) );
}

void vpeDstDimension(
	UINT16 u16Width, 
	UINT16 u16Height)
{
	UINT32 u32Height = u16Height;
	UINT32 u32Width = u16Width;
	g_u16DstWidth = u16Width;
	g_u16DstWidth = u16Height;
	outp32(REG_VPE_VYDSF, (inp32(REG_VPE_VYDSF) & ~VSF_N) |				//Vertical --> Height
					((u32Height<<16) & VSF_N)  );
																		//Horizontal --> Width		
	outp32(REG_VPE_HXDSF, (inp32(REG_VPE_HXDSF) & ~HSF_N) |		
					((u32Width<< 16) & HSF_N) );
							 				
}
void vpeSrcDimension(
	UINT16 u16Width,
	UINT16 u16Height)
{
	g_u16SrcWidth = u16Width;
	g_u16SrcWidth = u16Height;
	outp32(REG_VPE_VYDSF, (inp32(REG_VPE_VYDSF) & ~VSF_M) |		//Vertical --> Height
				 	(u16Height&VSF_M) );				 	
	outp32(REG_VPE_HXDSF, (inp32(REG_VPE_HXDSF) & ~HSF_M) |		//Horizontal --> Width
					  (u16Width &HSF_M) );			 	
					  
					
}
/*
	VPE will auto converse to full range automaticly.
	If want to set the output pattern as CCIR601. bIsConverseCCIR601 will be set to 1
*/
void vpeSetColorRange(
	BOOL bIsPatCCIR601,
	BOOL bIsConverseCCIR601)
{
	outp32(REG_VPE_CMD, (inp32(REG_VPE_CMD) & ~(CCIR601|LEVEL)) |
					(((bIsPatCCIR601<< 31) & CCIR601) | ((bIsConverseCCIR601<<27)&LEVEL)) );
}

void vpeSetScaleFilter(
	BOOL bIsSoftwareMode,
	BOOL bIsBypass3x3Filter,		//Block base 3x3 ==> Line Base 3x3 
	BOOL bisBilinearFilter)
{
#if 0
	outp32(REG_VPE_CMD, (inp32(REG_VPE_CMD) & ~(TAP|BYPASS|BILINEAR)) |
					 	(((bIsSoftwareMode<<10) &TAP) | ((bIsBypass3x3Filter<<9) &BYPASS) |
					 	 ((BisBilinearFilter<<7)&BILINEAR))
						);							
#else
	outp32(REG_VPE_CMD, (inp32(REG_VPE_CMD) & ~(TAP|BYPASS|BILINEAR)) |
					 	(((bIsSoftwareMode<<10) &TAP) | ((bIsBypass3x3Filter<<9) &BYPASS) |			//Block base 3x3 filter is need enable if 2 step step 3x3 filter  
					 	 ((bisBilinearFilter<<7)&BILINEAR))
						);
						
	if(bIsBypass3x3Filter==FALSE)
	{//Line base 3x3 enable 
		outp32(REG_VPE_CMD, (inp32(REG_VPE_CMD) | BIT19));	//frame base 3X3 filter enable
	}
	else
	{//Line base 3x3 disable 
		outp32(REG_VPE_CMD, (inp32(REG_VPE_CMD) & ~BIT19));	//frame base 3X3 filter disable		
	}	
#endif	
}

void vpeSetScale3x3Coe(
	UINT32 u32Coe1to4,
	UINT32 u32Coe5to8,
	UINT16 u16CoePreCur)
{

	while( (inp32(REG_VPE_TG)&VPE_GO==VPE_GO) );		
	if((u32Coe1to4&0xFF)==0)
	{//Central pixel=0 ==> hardware mode
		outp32(REG_VPE_CMD, inp32(REG_VPE_CMD)| TAP); 	//Hardware mode
	}
	else
	{
		outp32(REG_VPE_CMD, (inp32(REG_VPE_CMD) & ~TAP)); //Software mode
		outp32(REG_VPE_FCOEF0, u32Coe1to4);
		outp32(REG_VPE_FCOEF1, u32Coe5to8);
		outp32(REG_VPE_TG, ((inp32(REG_VPE_TG)& ~(TAPC_JUMP|TAPC_CEN)) |
								(u16CoePreCur<<16)&(TAPC_JUMP|TAPC_CEN)) );
	}							
}
void vpeSetMatchMacroBlock(
	UINT16 u16YMcu,
	UINT16 u16XMcu)
{	
	UINT32 u32YMcu;
	u32YMcu = u16YMcu;
	outp32(REG_VPE_MCU, ((u32YMcu << 16) | u16XMcu));
}
static UINT32 u32VpeTrigger =0;
void vpeTrigger(void)
{
	//UINT32 i,j;
	while( (inp32(REG_VPE_TG)&VPE_GO==VPE_GO) );	
	outp32(REG_VPE_RESET,  0x03);
	outp32(REG_VPE_RESET,  0x00);
	if((inp32(REG_VMMU_CR)&MMU_EN)==MMU_EN)
	{
		InitLRUTable();				
	}		
	outp32(REG_VPE_TG, inp32(REG_VPE_TG)|VPE_GO);
	u32VpeTrigger = u32VpeTrigger+1;
}
BOOL vpeCheckTrigger(void)
{	
	if( (inp32(REG_VPE_TG)&VPE_GO) == VPE_GO )
		return TRUE;
	else
		return FALSE;	
}	

void vpeEnableVmmu(BOOL bIsEnable)
{
	//outp32(REG_VMMU_CR, bIsEnable&0x03);
	if(bIsEnable==TRUE)
		//outp32(REG_VMMU_CR, 0x03);		//mmu enable, micro tlb enable
		//outp32(REG_VMMU_CR, 0x13);		//SC test mode and mmu enable, micro tlb enable
		outp32(REG_VMMU_CR, 0x1);			//mmu enable
	else
		outp32(REG_VMMU_CR, 0x00);	
}

void vpeSetTtbAddress(UINT32 u32PhysicalTtbAddr)
{
	outp32(REG_VMMU_TTB, u32PhysicalTtbAddr);
	pu32TLBEntry = (PUINT32)u32PhysicalTtbAddr;
}
UINT32 vpeGetTtbAddress(void)
{
	return (inp32(REG_VMMU_TTB));
}
void vpeSetTtbEntry(UINT32 u32Entry, UINT32 u32Level1Entry)
{//Level 1 entry. 
	if(u32Entry>7)
		return;
	/*	
	
	*/	
	outp32(REG_VMMU_L1PT0+4*u32Entry, u32Level1Entry);		
}
UINT32 vpeGetTtbEntry(UINT32 u32Entry)
{//Level 1 entry. 
	if(u32Entry>3)
		return 0;
	return (inp32(REG_VMMU_L1PT0+4*u32Entry));
}
PUINT32 pu32TTB;

ERRCODE vpeIoctl(UINT32 u32Cmd, UINT32 u32Arg0, UINT32 u32Arg1, UINT32 u32Arg2) 
{
	ERRCODE ErrCode=Successful; 

	switch(u32Cmd)
	{
		case VPE_IOCTL_SET_SRCBUF_ADDR:		
				vpeSetSrcBufAddr(u32Arg0, u32Arg1, u32Arg2);
				
				vpeSetTtbEntry(0, pu32TTB[u32Arg0/0x100000]);	
				vpeSetTtbEntry(4,  pu32TTB[u32Arg0/0x100000+1]);	
				vpeSetTtbEntry(1,  pu32TTB[u32Arg1/0x100000]);	
				vpeSetTtbEntry(5,  pu32TTB[u32Arg1/0x100000+1]);	
				vpeSetTtbEntry(2,  pu32TTB[u32Arg2/0x100000]);	
				vpeSetTtbEntry(6,  pu32TTB[u32Arg2/0x100000+1]);
			break;	
		case VPE_IOCTL_SET_DSTBUF_ADDR:
				vpeSetDstBufAddr(u32Arg0);
				
				vpeSetTtbEntry(3,  pu32TTB[u32Arg0/0x100000]);	
				vpeSetTtbEntry(7,  pu32TTB[u32Arg0/0x100000+1]);	
			break;	
		case VPE_IOCTL_SET_SRC_OFFSET:
				vpeSetSrcOffset(u32Arg0, u32Arg1);
			break;
		case VPE_IOCTL_SET_DST_OFFSET:
				vpeSetDstOffset(u32Arg0, u32Arg1);
			break;	
		case VPE_IOCTL_SET_SRC_DIMENSION:
				vpeSrcDimension(u32Arg0, u32Arg1);
			break;
		case VPE_IOCTL_SET_DST_DIMENSION:
				vpeDstDimension(u32Arg0, u32Arg1);	
			break;
		case VPE_IOCTL_SET_COLOR_RANGE:			
				vpeSetColorRange(u32Arg0, u32Arg1);
			break;
		case VPE_IOCTL_SET_FILTER:
				if(u32Arg0==VPE_SCALE_DDA)			
					vpeSetScaleFilter(FALSE,			//BOOL bIsSoftware
										TRUE,			//BOOL bIsBypass3x3Filter,
										FALSE);			//BOOL BisBilinearFilter)
				else if(u32Arg0==VPE_SCALE_3X3)
					vpeSetScaleFilter(FALSE,			//BOOL bIsSoftware,
										FALSE,			//3x3 enable 
										FALSE);			//BOOL BisBilinearFilter)							
				else if(u32Arg0==VPE_SCALE_BILINEAR)
					vpeSetScaleFilter(FALSE,			//BOOL bIsSoftware,
										TRUE,			//3x3 disable 
										TRUE);			//BOOL BisBilinearFilter)	
				else if(u32Arg0==VPE_SCALE_3X3_BILINEAR)   //Not support now
					vpeSetScaleFilter(FALSE,			//BOOL bIsSoftware,
										FALSE,			//3x3 enable
										TRUE);			//Bilinear enable											
			break;
		case VPE_IOCTL_SET_3X3_COEF:
				vpeSetScale3x3Coe(u32Arg0, u32Arg1, u32Arg2);			
			break;
		case VPE_IOCTL_SET_FMT:
				vpeSetFmtOperation(u32Arg0, u32Arg1, u32Arg2);	
			break;
		case VPE_IOCTL_SET_MACRO_BLOCK:
				vpeSetMatchMacroBlock(u32Arg0, u32Arg1);	
			break;
		case VPE_IOCTL_HOST_OP:		
				vpeHostOperation(u32Arg0, u32Arg1);				
			break;	
		case VPE_IOCTL_TRIGGER:	
				vpeTrigger();		
			break;	
		case VPE_IOCTL_CHECK_TRIGGER:				
				ErrCode = vpeCheckTrigger();	//0= Complete, 1=Not Complete, <0 ==> IOCTL Error
			break;	
		case VPE_IOCTL_SET_MMU_ENTRY:
				{
					pu32TTB = (PUINT32)u32Arg1;
					
					vpeEnableVmmu(u32Arg0);	
					vpeSetTtbAddress(u32Arg1);	
				/*												
					vpeSetTtbEntry(0, pu32TTB[u32VpeYBufAddr/0x100000]);	
					vpeSetTtbEntry(4,  pu32TTB[u32VpeYBufAddr/0x100000+1]);	
					vpeSetTtbEntry(1,  pu32TTB[u32VpeUBufAddr/0x100000]);	
					vpeSetTtbEntry(5,  pu32TTB[u32VpeUBufAddr/0x100000+1]);	
					vpeSetTtbEntry(2,  pu32TTB[u32VpeVBufAddr/0x100000]);	
					vpeSetTtbEntry(6,  pu32TTB[u32VpeVBufAddr/0x100000+1]);	
					vpeSetTtbEntry(3,  pu32TTB[u32VpeDstBufAddr/0x100000]);	
					vpeSetTtbEntry(7,  pu32TTB[u32VpeDstBufAddr/0x100000+1]);	
				*/	
				}
			break;							
		case VPE_IOCTL_SET_TLB_ENTRY:
				vpeSetTtbEntry(u32Arg0, u32Arg1);	
			break;											
		default:
			return ERR_VPE_IOCTL;
	}
	
	
	return ErrCode;
}