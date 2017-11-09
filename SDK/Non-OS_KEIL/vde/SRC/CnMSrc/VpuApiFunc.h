//------------------------------------------------------------------------------
// File: VpuApiFunc.h
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------


#ifndef VPUAPI_UTIL_H_INCLUDED
#define VPUAPI_UTIL_H_INCLUDED

#include "VpuApi.h"
#include "RegDefine.h"



#define MAX_NUM_INSTANCE 4

enum {
	MP4_DEC = 0,
	AVC_DEC = 1
};

enum {
	SEQ_INIT = 1,
	SEQ_END = 2,
	PIC_RUN = 3,
	SET_FRAME_BUF = 4,
	ENCODE_HEADER = 5,
	ENC_PARA_SET = 6,
	DEC_PARA_SET = 7,
	DEC_BUF_FLUSH = 8,
	RC_CHANGE_PARAMETER	= 9,	
	VPU_SLEEP = 10,
	VPU_WAKE = 11,
	FIRMWARE_GET = 0xf
};

typedef struct {
	DecOpenParam openParam;
	DecInitialInfo initialInfo;
	
	PhysicalAddress streamWrPtr;
	PhysicalAddress streamRdPtrRegAddr;
	PhysicalAddress streamWrPtrRegAddr;
	PhysicalAddress streamBufStartAddr;
	PhysicalAddress streamBufEndAddr;
	PhysicalAddress	frameDisplayFlagRegAddr;
	int streamBufSize;
	FrameBuffer * frameBufPool;
	int numFrameBuffers;
	FrameBuffer * recFrame;
	int stride;
	int rotationEnable;
	int mirrorEnable;
	MirrorDirection mirrorDirection;
	int rotationAngle;
	FrameBuffer rotatorOutput;
	int rotatorStride;
	int rotatorOutputValid;	
	int initialInfoObtained;
	FrameBuffer	deBlockingFilterOutput;	
	int deBlockingFilterOutputValid;
	int filePlayEnable;
	int picSrcSize;
	int	dynamicAllocEnable;
} DecInfo;


typedef struct CodecInst {
	int instIndex;
	int inUse;
	int codecMode;
	union {
		DecInfo decInfo;
	} CodecInfo;
} CodecInst;


#ifdef __cplusplus
extern "C" {
#endif	
	void	BitIssueCommand(int instIdx, int cdcMode, int cmd);

	RetCode GetCodecInstance(CodecInst ** ppInst);
	void	FreeCodecInstance(CodecInst * pCodecInst);
	RetCode CheckInstanceValidity(CodecInst * pci);

	RetCode CheckDecInstanceValidity(DecHandle handle);
	RetCode CheckDecOpenParam(DecOpenParam * pop);
	int		DecBitstreamBufEmpty(DecInfo * pDecInfo);
	void	SetParaSet(DecHandle handle, int paraSetType, DecParamSet * para);	
#ifdef __cplusplus
}
#endif

#endif // endif VPUAPI_UTIL_H_INCLUDED
