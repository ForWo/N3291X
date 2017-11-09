/**********************************************************************************************************
 *                                                                          
 * Copyright (c) 2004 - 2007 Winbond Electronics Corp. All rights reserved.      
 *                                                                         
 * FILENAME
 *     vdoAPI.c
 *
 * VERSION
 *     1.0
 *
 * DESCRIPTION
 *
 *
 * HISTORY
 *     04/18/2011		 Ver 1.0 Created by PX40 KCHuang
 *
 * REMARK
 *     None
 *     
 **********************************************************************************************************/


#include "w55fa95_reg.h"
#include "wblib.h"
#include "wbio.h"

#include "vdodef.h"
#include "vpuapi.h"
#include "vdolocal.h"
#include "vdoapi.h"
#include "regDefine.h"
#include "vpuapifunc.h"

_CODEC_LOCAL_INFO   _codecLibInfor_;
static __align(32) BYTE	CnMWorkBuf[CODE_BUF_SIZE + WORK_BUF_SIZE + PARA_BUF_SIZE];

UINT32 g_startaddr,g_endaddr;


/*****************************************************************************\
**
** FUNCTION Group: Global & ISR
**
\*****************************************************************************/
static void vdoInitCompleteISR(void)
{
    if (_vdo_InterruptTable[INT_INIT_COMPLETE_NUM])
        (*_vdo_InterruptTable[INT_INIT_COMPLETE_NUM])();
}    

static void vdoSEQINITIsr(void)
{
    if (_vdo_InterruptTable[INT_SEQ_INIT_NUM])
        (*_vdo_InterruptTable[INT_SEQ_INIT_NUM])();
}  

static void vdoSEQENDIsr(void)
{
    if (_vdo_InterruptTable[INT_SEQ_END_NUM])
        (*_vdo_InterruptTable[INT_SEQ_END_NUM])();
}  

static void vdoPICRUNIsr(void)
{
    if (_vdo_InterruptTable[INT_PIC_RUN_NUM])
        (*_vdo_InterruptTable[INT_PIC_RUN_NUM])();
}  

static void vdoSetFrameBufIsr(void)
{
    if (_vdo_InterruptTable[INT_SET_FRAME_BUF_NUM])
        (*_vdo_InterruptTable[INT_SET_FRAME_BUF_NUM])();
}  

static void vdoEncHeaderIsr(void)
{
    if (_vdo_InterruptTable[INT_ENC_HEADER_NUM])
        (*_vdo_InterruptTable[INT_ENC_HEADER_NUM])();
}  

static void vdoEncParaSetIsr(void)
{
    if (_vdo_InterruptTable[INT_ENC_PARA_SET_NUM])
        (*_vdo_InterruptTable[INT_ENC_PARA_SET_NUM])();
}  

static void vdoDecParaSetIsr(void)
{
    if (_vdo_InterruptTable[INT_DEC_PARA_SET_NUM])
        (*_vdo_InterruptTable[INT_DEC_PARA_SET_NUM])();
}  

static void vdoDecBufFlushIsr(void)
{
    if (_vdo_InterruptTable[INT_DEC_BUF_FLUSH_NUM])
        (*_vdo_InterruptTable[INT_DEC_BUF_FLUSH_NUM])();
}  

static void vdoBufferEmptyIsr(void)
{
    if (_vdo_InterruptTable[INT_BUF_EMPTY_NUM])
        (*_vdo_InterruptTable[INT_BUF_EMPTY_NUM])();
}  

static void vdoBufferFullIsr(void)
{
    if (_vdo_InterruptTable[INT_BUF_FUL_NUM])
        (*_vdo_InterruptTable[INT_BUF_FUL_NUM])();
}  



void vdoISrHandler(void)
{
    UINT32 volatile interruptStatus,InterruptEnableBit;
    
    interruptStatus = inpw(BIT_INT_REASON);
    InterruptEnableBit = inpw(BIT_INT_ENABLE);
    
    // Make sure the status bit only be set when enable bit is active
    //if ((interruptStatus & InterruptEnableBit) ==0)
    //    InterruptEnableError = 1;
    //else    
    //    InterruptEnableError = 0;
        
    if(interruptStatus & BIT_INITIALIZE_COOMPLETE_STATUS)
    {
	    vdoInitCompleteISR();
    }
    else if(interruptStatus & BIT_SEQ_INIT_STATUS)
    {
        vdoSEQINITIsr();
    }
    else if(interruptStatus & BIT_SEQ_END_STATUS)
    {
        vdoSEQENDIsr();
    }
    else if(interruptStatus & BIT_PIC_RUN_STATUS)
    {
        vdoPICRUNIsr();
    }
    else if(interruptStatus & BIT_SET_FRAME_BUF_STATUS)
    {
        vdoSetFrameBufIsr();
    }
    else if(interruptStatus & BIT_ENC_HEADER_STATUS)
    {
        vdoEncHeaderIsr();
    }
    else if(interruptStatus & BIT_ENC_PARA_SET_STATUS)
    {
        vdoEncParaSetIsr();
    }
    else if(interruptStatus & BIT_DEC_PARA_SET_STATUS)
    {
        vdoDecParaSetIsr();
    }
    else if(interruptStatus & BIT_DEC_BUF_FLUSH_STATUS)
    {
        vdoDecBufFlushIsr();
    }
    else if(interruptStatus & BIT_BUFFER_EMPTY_STATUS)
    {
        vdoBufferEmptyIsr();
    }
    else if(interruptStatus & BIT_BUFFER_FULL_STATUS)
    {
        vdoBufferFullIsr();
    }

    outpw(BIT_INT_REASON,0x0);
    outpw(BIT_INT_CLEAR,0x01);

}


RetCode vdoResetCodec(void)	
{
    return RETCODE_SUCCESS;
}

int	vdoIsBusy(void)
{
    return VPU_IsBusy();
}

//RetCode vdoInit(PhysicalAddress workBuf,int *workSize)
RetCode vdoInit(void)
{
	RetCode			ret = RETCODE_SUCCESS;	
	    
    
#if 1    
	ret = VPU_Init((PhysicalAddress)&CnMWorkBuf);
#else
	ret = VPU_Init((PhysicalAddress)0x300000);
#endif	
	
	sysprintf("CodeBuf size = %dK, WorkBuf Size = %dK, ParaBuf size = %dK\n",CODE_BUF_SIZE/1024,WORK_BUF_SIZE/1024,PARA_BUF_SIZE/1024 );
	
    return ret;
}

RetCode vdoGetVersionInfo( Uint32 *versionInfo )
{
    return VPU_GetVersionInfo(versionInfo);
}

RetCode vdoInstallISR(void)
{
	sysInstallISR(IRQ_LEVEL_1, IRQ_VDE, (PVOID)vdoISrHandler);	    
    return RETCODE_SUCCESS;	
}

RetCode vdoEnableInterruptSrc(Uint32 Src)
{
    outpw(BIT_INT_ENABLE,inpw(BIT_INT_ENABLE) | (1<<Src));
    return RETCODE_SUCCESS;	
}

RetCode vdoDisableInterruptSrc(Uint32 Src)
{
    outpw(BIT_INT_ENABLE,inpw(BIT_INT_ENABLE) & ~(1<<Src));
    return RETCODE_SUCCESS;	
}



PVOID vdoInstallCallBackFunction(Uint32 uSource, PVOID pvNewISR)
{
   	PVOID  _mOldVect;
   	
   	outpw(BIT_INT_ENABLE,inpw(BIT_INT_ENABLE) | (1<<uSource));
   	_mOldVect = (PVOID)_vdo_InterruptTable[uSource];	
	_vdo_InterruptTable[uSource] =  (_gFunPtr)pvNewISR;
	return _mOldVect;    
}


/*****************************************************************************\
**
** FUNCTION Group: Decoder
**
\*****************************************************************************/
	// function for decode
RetCode vdoDecOpen(DecHandle * pHandle, DecOpenParam * pop)
{
    RetCode			ret = RETCODE_SUCCESS;
    CodecInst *pCodecInst;    
        
    ret = VPU_DecOpen(pHandle,pop);
    
    if (ret == RETCODE_SUCCESS)
    {
        pCodecInst = *pHandle;
    	_codecLibInfor_.codecLibInfo[pCodecInst->instIndex].paBsBufStart = pop->bitstreamBuffer; 
    	_codecLibInfor_.codecLibInfo[pCodecInst->instIndex].paBsBufEnd = pop->bitstreamBuffer + pop->bitstreamBufferSize; 
	}
	
	return ret;    
}

RetCode vdoDecClose(DecHandle handle)
{
    return VPU_DecClose(handle);
}


RetCode vdoDecGetInitialInfo(DecHandle handle,DecInitialInfo * info)
{
    RetCode			ret = RETCODE_SUCCESS;
    
    VPU_DecSetEscSeqInit( handle, 1 );

    ret = VPU_DecGetInitialInfo(handle,info);
    if (ret != RETCODE_SUCCESS)
        return ret;    
    
    VPU_DecSetEscSeqInit( handle, 0 );
    
    return ret;
}

RetCode vdoDecRegisterFrameBuffer(DecHandle handle,FrameBuffer * bufArray,int num,int stride, DecBufInfo *pBufInfo)
{
    return VPU_DecRegisterFrameBuffer(handle,bufArray,num,stride, pBufInfo);
}


RetCode vdoDecGetBitstreamBuffer(DecHandle handle,	PhysicalAddress * pwrPtr,Uint32 * size )
{
	PhysicalAddress prdPrt,startaddr,endaddr;
    RetCode			ret = RETCODE_SUCCESS;	
    CodecInst *pCodecInst;
    

    ret = VPU_DecGetBitstreamBuffer(handle,&prdPrt,pwrPtr,size);
    
    if (ret == RETCODE_SUCCESS)
    {
        *size = ( ( *size >> 9 ) << 9 );    //  Let it be 512 multiple
        
        pCodecInst = handle;
    	g_startaddr = _codecLibInfor_.codecLibInfo[pCodecInst->instIndex].paBsBufStart; 
    	g_endaddr   = _codecLibInfor_.codecLibInfo[pCodecInst->instIndex].paBsBufEnd;
    	/*
    	if (((int)*pwrPtr + (int)*size ) > (int)endaddr)
    	{   // Get the free buffer size before end of buffer addr
    	    *size = endaddr - *pwrPtr;
    	}
    	*/
    }	
    
    return ret;
}

RetCode vdoDecUpdateBitstreamBuffer(DecHandle handle,Uint32 size)
{
    return VPU_DecUpdateBitstreamBuffer(handle,size);
}

RetCode vdoSleepWake(DecHandle handle, int iSleepWake )
{
    return VPU_SleepWake(handle,iSleepWake);
}

RetCode vdoDecStartOneFrame(DecHandle handle,DecParam *param )
{
    return VPU_DecStartOneFrame(handle,param);
}

RetCode vdoDecGetOutputInfo(DecHandle handle,DecOutputInfo * info)
{
    return VPU_DecGetOutputInfo(handle,info);
}

RetCode vdoDecClrDispFlag(	DecHandle handle, int index)
{
	return VPU_DecClrDispFlag(handle, index);
}

RetCode vdoDecGiveCommand(DecHandle handle,CodecCommand cmd,void * parameter)
{
    return VPU_DecGiveCommand(handle,cmd,parameter);
}
