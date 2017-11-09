//------------------------------------------------------------------------------
// File: vdoApi.h
//
// Copyright (c) Nuvoton Electronics Corp.  All rights reserved.
//------------------------------------------------------------------------------
#ifndef VDOAPI_H_INCLUDED
#define VDOAPI_H_INCLUDED




#ifdef __cplusplus
extern "C" {
#endif
    // Globacl API 
	RetCode vdoResetCodec(void);	
	int		vdoIsBusy(void);
//	RetCode vdoInit(PhysicalAddress workBuf,int *workSize);
	RetCode vdoInit(void);	
	RetCode vdoGetVersionInfo( Uint32 *versionInfo );
	RetCode vdoInstallISR(void);	
	PVOID   vdoInstallCallBackFunction(Uint32 interruptSrc, PVOID callbackfunction);		
    RetCode vdoEnableInterruptSrc(Uint32 Src);
    RetCode vdoDisableInterruptSrc(Uint32 Src);  

	// function for decode
	RetCode vdoDecOpen(DecHandle *, DecOpenParam *);
	RetCode vdoDecClose(DecHandle);
	RetCode vdoDecGetInitialInfo(DecHandle handle,DecInitialInfo * info);
	RetCode vdoDecRegisterFrameBuffer(DecHandle handle, FrameBuffer * bufArray,	int num, int stride, DecBufInfo *pBufInfo);
	RetCode vdoDecGetBitstreamBuffer(DecHandle handle,PhysicalAddress * pwrPtr,Uint32 * size );
	RetCode vdoDecUpdateBitstreamBuffer(DecHandle handle,Uint32 size);
	RetCode vdoSleepWake(DecHandle handle, int iSleepWake);
	RetCode vdoDecStartOneFrame(DecHandle handle,DecParam *param ); 
	RetCode vdoDecGetOutputInfo(DecHandle handle,DecOutputInfo * info);
	RetCode vdoDecClrDispFlag(	DecHandle handle, int index);
	RetCode vdoDecGiveCommand(DecHandle handle,CodecCommand cmd,void * parameter);	
	
	
		int		VPU_IsBusy(void);
	RetCode VPU_Init(PhysicalAddress workBuf);
	RetCode VPU_GetVersionInfo( Uint32 *versionInfo );

	// function for decode
	RetCode VPU_DecOpen(DecHandle *, DecOpenParam *);
	RetCode VPU_DecClose(DecHandle);
	RetCode VPU_DecSetEscSeqInit( 
		DecHandle handle, 
		int escape );
	RetCode VPU_DecGetInitialInfo(
		DecHandle handle,
		DecInitialInfo * info);

	RetCode VPU_DecRegisterFrameBuffer(
		DecHandle handle,
		FrameBuffer * bufArray,
		int num,
		int stride,
		DecBufInfo *pBufInfo);

	RetCode VPU_DecGetBitstreamBuffer(
		DecHandle handle,
		PhysicalAddress * prdPrt,
		PhysicalAddress * pwrPtr,
		Uint32 * size );
	RetCode VPU_DecUpdateBitstreamBuffer(
		DecHandle handle,
		Uint32 size);
	RetCode VPU_SleepWake(
		DecHandle handle, 
		int iSleepWake);

	RetCode VPU_DecStartOneFrame( 
		DecHandle handle, 
		DecParam *param ); 
	RetCode VPU_DecGetOutputInfo(
		DecHandle handle,
		DecOutputInfo * info);
	RetCode VPU_DecBitBufferFlush(
		DecHandle handle);
	RetCode VPU_DecClrDispFlag(
		DecHandle handle, int index);
	RetCode VPU_DecGiveCommand(
		DecHandle handle,
		CodecCommand cmd,
		void * parameter);	
	
#ifdef __cplusplus
}
#endif





#endif