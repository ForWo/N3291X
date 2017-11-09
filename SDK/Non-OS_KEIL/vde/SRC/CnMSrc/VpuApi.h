//------------------------------------------------------------------------------
// File: VpuApi.h
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------
#ifndef VPUAPI_H_INCLUDED
#define VPUAPI_H_INCLUDED
//------------------------------------------------------------------------------
// system dependent definition
//------------------------------------------------------------------------------
  
#define VpuWriteReg( ADDR, DATA )	outpw(ADDR,DATA) 
#define VpuReadReg( ADDR )			inpw(ADDR)	   
#define VpuWriteMem( ADDR, DATA )	outpw(ADDR,DATA) 
#define VpuReadMem( ADDR )			inpw(ADDR)		


#if 0
// Setting for V2.5.3 or before
#define CODE_BUF_SIZE (96 * 1024)
#define FMO_SLICE_SAVE_BUF_SIZE	( 64 )
#define WORK_BUF_SIZE	(288 * 1024) + ( FMO_SLICE_SAVE_BUF_SIZE * 1024 * 8 )
#define PARA_BUF2_SIZE	(1728)
#define PARA_BUF_SIZE	(10 * 1024)
#else
// Setting for V2.6.0
#define CODE_BUF_SIZE (32 * 1024)
//#define FMO_SLICE_SAVE_BUF_SIZE	( 64 )
//#define WORK_BUF_SIZE	(512 * 1024)	// V2.6.0
#define WORK_BUF_SIZE	(396 * 1024)	// V2.6.1
//#define WORK_BUF_SIZE	(1024 * 1024)	//debgging
#define PARA_BUF2_SIZE	(0)
#define PARA_BUF_SIZE	(1 * 1024)
#endif


#define IMAGE_ENDIAN			0		// 0 (64 bit little endian), 1 (64 bit big endian), 2 (32 bit swaped little endian), 3 (32 bit swaped big endian)
#define STREAM_ENDIAN			0       // 0 (64 bit little endian), 1 (64 bit big endian), 2 (32 bit swaped little endian), 3 (32 bit swaped big endian)
#define	CBCR_INTERLEAVE			0		// 0 (chroma seperate mode), 1 (chroma interleave mode)

typedef unsigned int    UINT;
typedef unsigned char   BYTE;
#define DEFAULT_SEARCHRAM_ADDR  0x280000
#define STREAM_FULL_EMPTY_CHECK_DISABLE 0


#ifdef __cplusplus
extern "C" {
#endif

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
