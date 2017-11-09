//------------------------------------------------------------------------------
// File: VpuApiFunc.c
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------

#include "wbio.h"
#include "vdodef.h"
#include "VpuApiFunc.h"
#include "RegDefine.h"

 

extern CodecInst codecInstPool[MAX_NUM_INSTANCE];
extern PhysicalAddress workBuffer;
extern PhysicalAddress codeBuffer;
extern PhysicalAddress paraBuffer;

/*
 * GetCodecInstance() obtains a instance.
 * It stores a pointer to the allocated instance in *ppInst
 * and returns RETCODE_SUCCESS on success.
 * Failure results in 0(null pointer) in *ppInst and RETCODE_FAILURE.
 */

RetCode GetCodecInstance(CodecInst ** ppInst)
{
	int i;
	CodecInst * pCodecInst;

	pCodecInst = &codecInstPool[0];
	for (i = 0; i < MAX_NUM_INSTANCE; ++i, ++pCodecInst) {
		if (!pCodecInst->inUse)
			break;
	}

	if (i == MAX_NUM_INSTANCE) {
		*ppInst = 0;
		return RETCODE_FAILURE;
	}

	pCodecInst->inUse = 1;
	*ppInst = pCodecInst;
	return RETCODE_SUCCESS;
}

RetCode CheckInstanceValidity(CodecInst * pci)
{
	CodecInst * pCodecInst;
	int i;

	pCodecInst = &codecInstPool[0];
	for (i = 0; i < MAX_NUM_INSTANCE; ++i, ++pCodecInst) {
		if (pCodecInst == pci)
			return RETCODE_SUCCESS;
	}
	return RETCODE_INVALID_HANDLE;
}


RetCode CheckDecInstanceValidity(DecHandle handle)
{
	CodecInst * pCodecInst;
	RetCode ret;

	pCodecInst = handle;
	ret = CheckInstanceValidity(pCodecInst);
	if (ret != RETCODE_SUCCESS) {
		return RETCODE_INVALID_HANDLE;
	}
	if (!pCodecInst->inUse) {
		return RETCODE_INVALID_HANDLE;
	}
	if (pCodecInst->codecMode != MP4_DEC && 
		pCodecInst->codecMode != AVC_DEC) {
		return RETCODE_INVALID_HANDLE;
	}
	return RETCODE_SUCCESS;
}

void FreeCodecInstance(CodecInst * pCodecInst)
{
	pCodecInst->inUse = 0;
}

void BitIssueCommand(int instIdx, int cdcMode, int cmd)
{
	VpuWriteReg(BIT_RUN_INDEX, instIdx);
	VpuWriteReg(BIT_RUN_COD_STD, cdcMode);
	VpuWriteReg(BIT_RUN_COMMAND, cmd);
}




RetCode CheckDecOpenParam(DecOpenParam * pop)
{
	if (pop == 0) {
		return RETCODE_INVALID_PARAM;
	}
	if (pop->bitstreamBuffer % 4) { // not 4-bit aligned
		return RETCODE_INVALID_PARAM;
	}
	if (pop->bitstreamBufferSize % 1024 ||
			pop->bitstreamBufferSize < 1024 ||
			pop->bitstreamBufferSize > 16383 * 1024) {
		return RETCODE_INVALID_PARAM;
	}
	if (pop->bitstreamFormat != STD_MPEG4 &&
			pop->bitstreamFormat != STD_H263 &&
			pop->bitstreamFormat != STD_AVC ) {
		return RETCODE_INVALID_PARAM;
	}

	if( pop->mp4DeblkEnable == 1 && !( pop->bitstreamFormat == STD_MPEG4 || pop->bitstreamFormat == STD_H263 ) ) {
		return RETCODE_INVALID_PARAM;
	}

	return RETCODE_SUCCESS;
}


int DecBitstreamBufEmpty(DecInfo * pDecInfo)
{
	return ( VpuReadReg(pDecInfo->streamRdPtrRegAddr) == VpuReadReg(pDecInfo->streamWrPtrRegAddr) );
}


void SetParaSet(DecHandle handle, int paraSetType, DecParamSet * para)
{
	CodecInst * pCodecInst;
	DecInfo * pDecInfo;
	int i;
	Uint32 * src;

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;

	src = para->paraSet;
	for (i = 0; i < para->size; i += 4) {
		VpuWriteReg(paraBuffer + i, *src++);
	}
	VpuWriteReg(CMD_DEC_PARA_SET_TYPE, paraSetType); // 0: SPS, 1: PPS
	VpuWriteReg(CMD_DEC_PARA_SET_SIZE, para->size);
	BitIssueCommand(pCodecInst->instIndex, pCodecInst->codecMode, DEC_PARA_SET);
	while (VpuReadReg(BIT_BUSY_FLAG));
}


