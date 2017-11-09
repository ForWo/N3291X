//------------------------------------------------------------------------------
// File: VpuApi.c
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------

#include "wbio.h"
#include "vdodef.h"
#include "VpuApiFunc.h"

 
#include "BitAsmTable_CODADX8.h"

static PhysicalAddress rdPtrRegAddr[] = {
	BIT_RD_PTR_0,
	BIT_RD_PTR_1,
	BIT_RD_PTR_2,
	BIT_RD_PTR_3
};

static PhysicalAddress wrPtrRegAddr[] = {
	BIT_WR_PTR_0,
	BIT_WR_PTR_1,
	BIT_WR_PTR_2,
	BIT_WR_PTR_3
};

static PhysicalAddress disFlagRegAddr[] = {
	BIT_FRM_DIS_FLG_0,
	BIT_FRM_DIS_FLG_1,
	BIT_FRM_DIS_FLG_2,
	BIT_FRM_DIS_FLG_3
};

static int vpuDownLoaded ;

// If a frame is started, pendingInst is set to the proper instance.
static CodecInst * pendingInst;

CodecInst codecInstPool[MAX_NUM_INSTANCE] = {0, };
PhysicalAddress workBuffer;
PhysicalAddress codeBuffer;
PhysicalAddress paraBuffer;

int VPU_IsBusy()
{
	return VpuReadReg(BIT_BUSY_FLAG) != 0;
}

RetCode VPU_Init(PhysicalAddress workBuf)
{
	int i;
	Uint32 data;
	CodecInst * pCodecInst;

	codeBuffer = workBuf;
	workBuffer = codeBuffer + CODE_BUF_SIZE;
	paraBuffer = workBuffer + WORK_BUF_SIZE;// + PARA_BUF2_SIZE;

#ifndef EVB_BUILD	
	for (i = 0; i < sizeof(bit_code) / sizeof(bit_code[0]); i += 2) 
	{
	   data = (bit_code[i] << 16) | bit_code[i + 1];
	   VpuWriteMem(codeBuffer + i * 2, data);
	}
#else
	//  for fast  download fimware code to sdram by burst HPI Transfer
	{
	    for (i=0; i<sizeof(bit_code)/sizeof(bit_code[0]); i+=4) 
		{
			switch(1)
			{
				case 0:	// 64b LITTLE
					dataH = (bit_code[i+0] << 16) | bit_code[i+1];
					dataL = (bit_code[i+2] << 16) | bit_code[i+3];
					dataH = (dataH>>24) | ((dataH>>8)&0xFF00) | ((dataH<<8)&0xFF0000) | ((dataH<<24)&0xFF000000);
					dataL = (dataL>>24) | ((dataL>>8)&0xFF00) | ((dataL<<8)&0xFF0000) | ((dataL<<24)&0xFF000000);
					data64 = ((UI64) dataL) << 32;
					data64 |= (UI64) dataH;
					break;
				case 1:	// 64b BIG
					dataH = (bit_code[i+0] << 16) | bit_code[i+1];
					dataL = (bit_code[i+2] << 16) | bit_code[i+3];
					data64 = ((UI64) dataH) << 32;
					data64 |= (UI64) dataL;
					break;
				case 2:	// 32b LITTLE
					dataH = (bit_code[i+0] << 16) | bit_code[i+1];
					dataL = (bit_code[i+2] << 16) | bit_code[i+3];
					dataH = (dataH>>24) | ((dataH>>8)&0xFF00) | ((dataH<<8)&0xFF0000) | ((dataH<<24)&0xFF000000);
					dataL = (dataL>>24) | ((dataL>>8)&0xFF00) | ((dataL<<8)&0xFF0000) | ((dataL<<24)&0xFF000000);
					data64 = ((UI64) dataH) << 32;
					data64 |= (UI64) dataL;
					break;
				case 3:	// 32b BIG
					dataH = (bit_code[i+0] << 16) | bit_code[i+1];
					dataL = (bit_code[i+2] << 16) | bit_code[i+3];
					data64 = ((UI64) dataL) << 32;
					data64 |= (UI64) dataH;
					break;
			}
			WriteMem(codeBuffer+i*2, data64);
		}
	}
#endif

	VpuWriteReg(BIT_WORK_BUF_ADDR, workBuffer);
	VpuWriteReg(BIT_PARA_BUF_ADDR, paraBuffer);
	VpuWriteReg(BIT_CODE_BUF_ADDR, codeBuffer);

	pCodecInst = &codecInstPool[0];
	for( i = 0; i < MAX_NUM_INSTANCE; ++i, ++pCodecInst ) 
	{
		pCodecInst->instIndex = i;
		pCodecInst->inUse = 0;
	}

	if (VpuReadReg(BIT_CUR_PC) != 0)
		return RETCODE_SUCCESS;

	VpuWriteReg(BIT_CODE_RUN, 0);
	
	for( i = 0; i < 2048; ++i ) 
	{
	   data = bit_code[i];
	   VpuWriteReg(BIT_CODE_DOWN, (i << 16) | data);
	}

	data = STREAM_FULL_EMPTY_CHECK_DISABLE << 1;
	data |= STREAM_ENDIAN;
	data |= 1 << 2;					// default setting is flush mode
	VpuWriteReg(BIT_BIT_STREAM_CTRL, data);
	VpuWriteReg(BIT_FRAME_MEM_CTRL, CBCR_INTERLEAVE << 1 | IMAGE_ENDIAN);
	VpuWriteReg(BIT_INT_ENABLE, 8); // PIC_RUN irq enable

	if( vpuDownLoaded == 0 )
		vpuDownLoaded = 1;
	
	VpuWriteReg(BIT_BUSY_FLAG, 1);
	VpuWriteReg(BIT_CODE_RUN, 1);
 	while (VpuReadReg(BIT_BUSY_FLAG))
 			;

	return RETCODE_SUCCESS;
}

RetCode VPU_GetVersionInfo( Uint32 *versionInfo )
{
	Uint32 ver;
	
	if (VpuReadReg(BIT_CUR_PC) == 0)
		return RETCODE_NOT_INITIALIZED;

	if( pendingInst )
		return RETCODE_FRAME_NOT_COMPLETE;
	
	VpuWriteReg( RET_VER_NUM , 0 );

	BitIssueCommand( 0, 0, FIRMWARE_GET );
 	while (VpuReadReg(BIT_BUSY_FLAG))
 			;
		
	ver = VpuReadReg( RET_VER_NUM );
	
	if( ver == 0 )
		return RETCODE_FAILURE;

	*versionInfo = ver;

	return RETCODE_SUCCESS;
}


RetCode VPU_DecOpen(DecHandle * pHandle, DecOpenParam * pop)
{
	CodecInst * pCodecInst;
	DecInfo * pDecInfo;
	int instIdx;
	RetCode ret;

	if (VpuReadReg(BIT_CUR_PC) == 0){
		return RETCODE_NOT_INITIALIZED;
	}

	ret = CheckDecOpenParam(pop);
	if (ret != RETCODE_SUCCESS) {
		return ret;
	}

	ret = GetCodecInstance(&pCodecInst);
	if (ret == RETCODE_FAILURE) {
		*pHandle = 0;
		return RETCODE_FAILURE;
	}

	*pHandle = pCodecInst;
	instIdx = pCodecInst->instIndex;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;

	pDecInfo->openParam = *pop;

	if (pop->bitstreamFormat == STD_MPEG4) {
		pCodecInst->codecMode = MP4_DEC;
	}
	else if (pop->bitstreamFormat == STD_H263) {
		pCodecInst->codecMode = MP4_DEC;
	} 
	else if (pop->bitstreamFormat == STD_AVC) {
		pCodecInst->codecMode = AVC_DEC;
	} 


	pDecInfo->streamWrPtr = pop->bitstreamBuffer;
	pDecInfo->streamRdPtrRegAddr = rdPtrRegAddr[instIdx];
	pDecInfo->streamWrPtrRegAddr = wrPtrRegAddr[instIdx];
	pDecInfo->frameDisplayFlagRegAddr = disFlagRegAddr[instIdx];
	pDecInfo->streamBufStartAddr = pop->bitstreamBuffer;
	pDecInfo->streamBufSize = pop->bitstreamBufferSize;
	pDecInfo->streamBufEndAddr = pop->bitstreamBuffer + pop->bitstreamBufferSize;
	pDecInfo->frameBufPool = 0;

	pDecInfo->rotationEnable = 0;
	pDecInfo->mirrorEnable = 0;
	pDecInfo->mirrorDirection = MIRDIR_NONE;
	pDecInfo->rotationAngle = 0;
	pDecInfo->rotatorOutputValid = 0;
	pDecInfo->rotatorStride = 0;

	pDecInfo->filePlayEnable = pop->filePlayEnable;
	if( pop->filePlayEnable == 1 )
	{
//for 0622_NewAPI		pDecInfo->picSrcSize = (pop->picWidth << 10) | pop->picHeight;
		pDecInfo->picSrcSize = (pop->picWidth << 16) | pop->picHeight;		
		pDecInfo->dynamicAllocEnable = pop->dynamicAllocEnable;
	}

	pDecInfo->initialInfoObtained = 0;

	VpuWriteReg(pDecInfo->streamRdPtrRegAddr, pDecInfo->streamBufStartAddr);
	VpuWriteReg(pDecInfo->streamWrPtrRegAddr, pDecInfo->streamWrPtr);
	VpuWriteReg(pDecInfo->frameDisplayFlagRegAddr, 0);
	return RETCODE_SUCCESS;
}

RetCode VPU_DecClose(DecHandle handle)
{
	CodecInst * pCodecInst;
	DecInfo * pDecInfo;
	RetCode ret;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	if (pendingInst) {
		return RETCODE_FRAME_NOT_COMPLETE;
	}

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;

	if (pDecInfo->initialInfoObtained) {
		VpuWriteReg( BIT_BUSY_FLAG, 0x1 );
		BitIssueCommand(pCodecInst->instIndex, pCodecInst->codecMode, SEQ_END);
		while (VpuReadReg(BIT_BUSY_FLAG))
			;
	}
	FreeCodecInstance(pCodecInst);
	return RETCODE_SUCCESS;
}

RetCode VPU_DecSetEscSeqInit( DecHandle handle, int escape )
{
	CodecInst * pCodecInst;
	DecInfo * pDecInfo;
	RetCode ret;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;
	
	VpuWriteReg(CMD_DEC_SEQ_INIT_ESCAPE, ( escape & 0x01 ) );	

	return RETCODE_SUCCESS;
}


RetCode VPU_DecGetInitialInfo(
		DecHandle handle,
		DecInitialInfo * info)
{
	CodecInst * pCodecInst;
	DecInfo * pDecInfo;
	Uint32 val, val2;
	RetCode ret;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	if (info == 0) {
		return RETCODE_INVALID_PARAM;
	}
	if (pendingInst) {
		return RETCODE_FRAME_NOT_COMPLETE;
	}

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;

	if (pDecInfo->initialInfoObtained) {
		return RETCODE_CALLED_BEFORE;
	}


	if (DecBitstreamBufEmpty(pDecInfo)) {
		return RETCODE_WRONG_CALL_SEQUENCE;
	}

	VpuWriteReg(CMD_DEC_SEQ_BB_START, pDecInfo->streamBufStartAddr);
	VpuWriteReg(CMD_DEC_SEQ_BB_SIZE, pDecInfo->streamBufSize / 1024); // size in KBytes

    if(pDecInfo->openParam.sorensonEnable == 1)
        VpuWriteReg(CMD_DEC_SEQ_MP4_CLASS, 256);
    else
        VpuWriteReg(CMD_DEC_SEQ_MP4_CLASS, 0);

	if( pDecInfo->filePlayEnable == 1 )
		VpuWriteReg(CMD_DEC_SEQ_START_BYTE, pDecInfo->openParam.streamStartByteOffset);

	val = ((pDecInfo->dynamicAllocEnable << 3) & 0x8) | ((pDecInfo->filePlayEnable << 2) & 0x4) | ((pDecInfo->openParam.reorderEnable << 1) & 0x2) | (pDecInfo->openParam.mp4DeblkEnable & 0x1);	
	VpuWriteReg(CMD_DEC_SEQ_OPTION, val);					

	VpuWriteReg( CMD_DEC_SEQ_SRC_SIZE, pDecInfo->picSrcSize );
	VpuWriteReg( BIT_BUSY_FLAG, 0x1 );
	BitIssueCommand(pCodecInst->instIndex, pCodecInst->codecMode, SEQ_INIT);
	while (VpuReadReg(BIT_BUSY_FLAG))
		;

	if (VpuReadReg(RET_DEC_SEQ_SUCCESS) == 0)
		return RETCODE_FAILURE;
	else if ((int)VpuReadReg(RET_DEC_SEQ_SUCCESS) == -1)
	{
		val = VpuReadReg(RET_DEC_SEQ_SRC_SIZE);		//0622_NewAPI
		info->picWidth = ( (val >> 16) & 0xffff );
		info->picHeight = ( val & 0xffff );	
		return RETCODE_NOTSUPPORTED;
	}		
	
	val = VpuReadReg(RET_DEC_SEQ_SRC_SIZE);
//0622_NewAPI	info->picWidth = ( (val >> 10) & 0x7ff );
//0630_NewAPI	info->picWidth = ( (val >> 10) & 0x3ff );	
//0630_NewAPI	info->picHeight = ( val & 0x3ff );
	info->picWidth = ( (val >> 16) & 0xffff );	
    info->picHeight = ( val & 0xffff );	
	

	val = VpuReadReg(RET_DEC_SEQ_SRC_F_RATE);
	info->frameRateInfo = val;

	if (pCodecInst->codecMode  == MP4_DEC) 
	{
		val = VpuReadReg(RET_DEC_SEQ_INFO);
		info->mp4_shortVideoHeader = (val >> 2) & 1;
		info->mp4_dataPartitionEnable = val & 1;
		info->mp4_reversibleVlcEnable =
			info->mp4_dataPartitionEnable ?
			((val >> 1) & 1) : 0;
		info->h263_annexJEnable = (val >> 3) & 1;
	}

	info->minFrameBufferCount = VpuReadReg(RET_DEC_SEQ_FRAME_NEED);
	info->frameBufDelay = VpuReadReg(RET_DEC_SEQ_FRAME_DELAY);				
	info->nextDecodedIdxNum = VpuReadReg(RET_DEC_SEQ_NEXT_FRAME_NUM);

	if (pCodecInst->codecMode == AVC_DEC) {
		val = VpuReadReg(RET_DEC_SEQ_CROP_LEFT_RIGHT);	
		val2 = VpuReadReg(RET_DEC_SEQ_CROP_TOP_BOTTOM);
		if( val == 0 && val2 == 0 ) {
			info->picCropRect.left = 0;
			info->picCropRect.right = 0;
			info->picCropRect.top = 0;
			info->picCropRect.bottom = 0;
		}
		else {
			//0630_NewAPI
			//info->picCropRect.left = ( (val>>10) & 0x3FF )*2;
			//info->picCropRect.right = info->picWidth - ( ( val & 0x3FF )*2 );
			//info->picCropRect.top = ( (val2>>10) & 0x3FF )*2;
			//info->picCropRect.bottom = info->picHeight - ( ( val2 & 0x3FF )*2 );

			info->picCropRect.left = ( (val>>16) & 0xFFFF )*2;
			info->picCropRect.right = info->picWidth - ( ( val & 0xFFFF )*2 );
			info->picCropRect.top = ( (val2>>16) & 0xFFFF )*2;
			info->picCropRect.bottom = info->picHeight - ( ( val2 & 0xFFFF )*2 );
			
		}
	}

	pDecInfo->initialInfo = *info;
	pDecInfo->initialInfoObtained = 1;

	


	return RETCODE_SUCCESS;
}


RetCode VPU_DecRegisterFrameBuffer(
		DecHandle handle,
		FrameBuffer * bufArray,
		int num,
		int stride,
		DecBufInfo *pBufInfo)
{
	CodecInst * pCodecInst;
	DecInfo * pDecInfo;
	int i;
	RetCode ret;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	if (pendingInst) {
		return RETCODE_FRAME_NOT_COMPLETE;
	}

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;

	if (pDecInfo->frameBufPool) {
		return RETCODE_CALLED_BEFORE;
	}

	if (!pDecInfo->initialInfoObtained) {
		return RETCODE_WRONG_CALL_SEQUENCE;
	}

	if (bufArray == 0) {
		return RETCODE_INVALID_FRAME_BUFFER;
	}
	if (num < pDecInfo->initialInfo.minFrameBufferCount) {
		return RETCODE_INSUFFICIENT_FRAME_BUFFERS;
	}
	if (stride < pDecInfo->initialInfo.picWidth ||
			stride % 8 != 0
			) {
		return RETCODE_INVALID_STRIDE;
	}

	pDecInfo->frameBufPool = bufArray;
	pDecInfo->numFrameBuffers = num;
	pDecInfo->stride = stride;

	// Let the decoder know the addresses of the frame buffers.
	for (i = 0; i < num; ++i) {
		VpuWriteReg(paraBuffer + i * 3 * 4, bufArray[i].bufY);
		VpuWriteReg(paraBuffer + i * 3 * 4 + 4, bufArray[i].bufCb);
		VpuWriteReg(paraBuffer + i * 3 * 4 + 8, bufArray[i].bufCr);
	}
	// Tell the decoder how much frame buffers were allocated.
	VpuWriteReg(CMD_SET_FRAME_BUF_NUM, num);
	VpuWriteReg(CMD_SET_FRAME_BUF_STRIDE, stride);

	if( pCodecInst->codecMode == AVC_DEC) 
	{
		VpuWriteReg( CMD_SET_FRAME_SLICE_BB_START, pBufInfo->avcSliceBufInfo.sliceSaveBuffer);
		VpuWriteReg( CMD_SET_FRAME_SLICE_BB_SIZE, (pBufInfo->avcSliceBufInfo.sliceSaveBufferSize/1024) );
	}
	BitIssueCommand(pCodecInst->instIndex, pCodecInst->codecMode, SET_FRAME_BUF);
	while (VpuReadReg(BIT_BUSY_FLAG))
		;

	return RETCODE_SUCCESS;
}

RetCode VPU_DecGetBitstreamBuffer( DecHandle handle,
		PhysicalAddress * prdPrt,
		PhysicalAddress * pwrPtr,
		Uint32 * size)
{
	CodecInst * pCodecInst;
	DecInfo * pDecInfo;
	PhysicalAddress rdPtr;
	PhysicalAddress wrPtr;
	Uint32 room;
	RetCode ret;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;


	if ( prdPrt == 0 || pwrPtr == 0 || size == 0) {
		return RETCODE_INVALID_PARAM;
	}

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;


	rdPtr = VpuReadReg(pDecInfo->streamRdPtrRegAddr);
	wrPtr = pDecInfo->streamWrPtr;
	
	if (wrPtr < rdPtr) {
		room = rdPtr - wrPtr - 1;
	}
	else {
		room = ( pDecInfo->streamBufEndAddr - wrPtr ) + ( rdPtr - pDecInfo->streamBufStartAddr ) - 1;	
	}

	*prdPrt = rdPtr;
	*pwrPtr = wrPtr;
	*size = room;

	return RETCODE_SUCCESS;
}


RetCode VPU_DecUpdateBitstreamBuffer(
		DecHandle handle,
		Uint32 size)
{
	CodecInst * pCodecInst;
	DecInfo * pDecInfo;
	PhysicalAddress wrPtr;
	PhysicalAddress rdPtr;
	RetCode ret;
	int		val = 0;
	int		room = 0;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;
	wrPtr = pDecInfo->streamWrPtr;
	
	if (size == 0) 
	{
		val = VpuReadReg( BIT_BIT_STREAM_PARAM );
		val |= 1 << ( pCodecInst->instIndex + 2);
		VpuWriteReg(BIT_BIT_STREAM_PARAM, val);
		return RETCODE_SUCCESS;
	}
	
	rdPtr = VpuReadReg(pDecInfo->streamRdPtrRegAddr);
	if (wrPtr < rdPtr) {
		if (rdPtr <= wrPtr + size)
			return RETCODE_INVALID_PARAM;
	}
	wrPtr += size;

	if( pDecInfo->filePlayEnable != 1 )  {
		if (wrPtr > pDecInfo->streamBufEndAddr) {
			room = wrPtr - pDecInfo->streamBufEndAddr;
			wrPtr = pDecInfo->streamBufStartAddr;
			wrPtr += room;
		}
		if (wrPtr == pDecInfo->streamBufEndAddr) {
			wrPtr = pDecInfo->streamBufStartAddr;
		}
	}

	pDecInfo->streamWrPtr = wrPtr;
	VpuWriteReg(pDecInfo->streamWrPtrRegAddr, wrPtr);

	return RETCODE_SUCCESS;
}

//---- VPU_SLEEP/WAKE
RetCode VPU_SleepWake(DecHandle handle, int iSleepWake)
{
	CodecInst	*pCodecInst ;
	static unsigned int regBk[64];
	int i=0;

	int	RunIndex, RunCodStd ;
	
	pCodecInst	= handle ;
	RunIndex	= pCodecInst->instIndex ;
	RunCodStd	= pCodecInst->codecMode ;

	if(iSleepWake==1) {
		for ( i = 0 ; i < 64 ; i++)
			regBk[i] = VpuReadReg(BIT_BASE + 0x100 + (i * 4));
		BitIssueCommand(RunIndex, RunCodStd, VPU_SLEEP);
	}
	else {
		if (VpuReadReg(BIT_CUR_PC) != 0) {
			BitIssueCommand(RunIndex, RunCodStd, VPU_WAKE);
			while( VPU_IsBusy() );
			return RETCODE_SUCCESS;
		}
		for ( i = 0 ; i < 64 ; i++)
			VpuWriteReg(BIT_BASE + 0x100 + (i * 4), regBk[i]);
		VpuWriteReg(BIT_CODE_RUN,	   0);
		//---- LOAD BOOT CODE
		{	Uint32 data;
			for( i = 0; i <2048; i++ ) {
			   data = bit_code[i];
			   VpuWriteReg(BIT_CODE_DOWN, (i << 16) | data);
			}
		}
		VpuWriteReg(BIT_BUSY_FLAG, 1);
		VpuWriteReg(BIT_CODE_RUN, 1);
 		while (VpuReadReg(BIT_BUSY_FLAG))
 			//Sleep( 1 );
		BitIssueCommand(RunIndex, RunCodStd, VPU_WAKE);
	}

	while( VPU_IsBusy() ) {
		//Sleep( 1 );
	}
	return RETCODE_SUCCESS;
}


RetCode VPU_DecStartOneFrame(DecHandle handle, DecParam *param)
{
	CodecInst * pCodecInst;
	DecInfo * pDecInfo;
	Uint32 rotMir;
	Uint32 val;
	RetCode ret;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	if (pendingInst) {
		return RETCODE_FRAME_NOT_COMPLETE;
	}

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;

	if (pDecInfo->frameBufPool == 0) { // This means frame buffers have not been registered.
		return RETCODE_WRONG_CALL_SEQUENCE;
	}
	rotMir = 0;
	if (pDecInfo->rotationEnable) {
		rotMir |= 0x10; // Enable rotator
		switch (pDecInfo->rotationAngle) {
			case 0:
				rotMir |= 0x0;
				break;

			case 90:
				rotMir |= 0x1;
				break;

			case 180:
				rotMir |= 0x2;
				break;

			case 270:
				rotMir |= 0x3;
				break;
		}
	}
	if (pDecInfo->mirrorEnable) {
		rotMir |= 0x10; // Enable rotator
		switch (pDecInfo->mirrorDirection) {
			case MIRDIR_NONE :
				rotMir |= 0x0;
				break;

			case MIRDIR_VER :
				rotMir |= 0x4;
				break;

			case MIRDIR_HOR :
				rotMir |= 0x8;
				break;

			case MIRDIR_HOR_VER :
				rotMir |= 0xc;
				break;

		}

	}

	if (rotMir & 0x10) // rotator enabled
	{ 
		VpuWriteReg(CMD_DEC_PIC_ROT_ADDR_Y, pDecInfo->rotatorOutput.bufY);
		VpuWriteReg(CMD_DEC_PIC_ROT_ADDR_CB, pDecInfo->rotatorOutput.bufCb);
		VpuWriteReg(CMD_DEC_PIC_ROT_ADDR_CR, pDecInfo->rotatorOutput.bufCr);
		VpuWriteReg(CMD_DEC_PIC_ROT_STRIDE, pDecInfo->rotatorStride);
	}

	VpuWriteReg(CMD_DEC_PIC_ROT_MODE, rotMir);

	if ( pCodecInst->codecMode == MP4_DEC && pDecInfo->openParam.mp4DeblkEnable == 1 ) {
		if (pDecInfo->deBlockingFilterOutputValid) {
			VpuWriteReg(CMD_DEC_PIC_DBK_ADDR_Y, pDecInfo->deBlockingFilterOutput.bufY);
			VpuWriteReg(CMD_DEC_PIC_DBK_ADDR_CB, pDecInfo->deBlockingFilterOutput.bufCb);
			VpuWriteReg(CMD_DEC_PIC_DBK_ADDR_CR, pDecInfo->deBlockingFilterOutput.bufCr);
		}
		else
			return RETCODE_DEBLOCKING_OUTPUT_NOT_SET;
	}

	if( param->iframeSearchEnable == 1 ) // if iframeSearch is Enable, other bit is ignore;
		val = ( param->iframeSearchEnable << 2 ) & 0x4;
	else
		val = ( param->skipframeMode << 3 ) | ( param->iframeSearchEnable << 2 ) | ( param->prescanMode << 1 ) | param->prescanEnable;
	VpuWriteReg( CMD_DEC_PIC_OPTION, val );
	VpuWriteReg( CMD_DEC_PIC_SKIP_NUM, param->skipframeNum );

	if( pCodecInst->codecMode == AVC_DEC ) {
		if( pDecInfo->openParam.reorderEnable == 1 ) {	// 0622_NewAPI
			VpuWriteReg( CMD_DEC_DISPLAY_REORDER, param->dispReorderBuf << 1 | (~2 & VpuReadReg( CMD_DEC_DISPLAY_REORDER )) );	
		}
	}
		
	
	if( pDecInfo->filePlayEnable == 1 )
	{
		VpuWriteReg( CMD_DEC_PIC_CHUNK_SIZE, param->chunkSize );
		if( pDecInfo->dynamicAllocEnable == 1 ) {
			VpuWriteReg( CMD_DEC_PIC_BB_START, param->picStreamBufferAddr );					
		}
		VpuWriteReg(CMD_DEC_PIC_START_BYTE, param->picStartByteOffset);
	}

	VpuWriteReg( BIT_BUSY_FLAG, 0x1 );
	BitIssueCommand(pCodecInst->instIndex, pCodecInst->codecMode, PIC_RUN);

	pendingInst = pCodecInst;

	return RETCODE_SUCCESS;
}

RetCode VPU_DecGetOutputInfo(
		DecHandle handle,
		DecOutputInfo * info)
{
	CodecInst * pCodecInst;
	DecInfo	  *	pDecInfo;
	RetCode		ret;
	Uint32		val = 0;
	int			i = 0;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	if (info == 0) {
		return RETCODE_INVALID_PARAM;
	}

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;

	if (pendingInst == 0) {
		return RETCODE_WRONG_CALL_SEQUENCE;
	}

	if (pCodecInst != pendingInst) {

		return RETCODE_INVALID_HANDLE;
	}

	info->indexFrameDisplay = VpuReadReg(RET_DEC_PIC_FRAME_IDX);
	info->indexFrameDecoded = VpuReadReg(RET_DEC_PIC_CUR_IDX);
	info->picType = VpuReadReg(RET_DEC_PIC_TYPE);
	info->numOfErrMBs = VpuReadReg(RET_DEC_PIC_ERR_MB);
	info->prescanresult = VpuReadReg( RET_DEC_PIC_OPTION );
	info->decodingSuccess = VpuReadReg(RET_DEC_PIC_SUCCESS)&1;
	info->useDirectModeForVc1 = (VpuReadReg(RET_DEC_PIC_SUCCESS)>>16)&1;

	val = VpuReadReg(RET_DEC_PIC_NEXT_IDX);
	
	for (i = 0 ; i < 3 ; i++) {
		if (i < pDecInfo->initialInfo.nextDecodedIdxNum) {
			info->indexFrameNextDecoded[i] = ((val >> (i * 5)) & 0x1f);
		} else {
			info->indexFrameNextDecoded[i] = -1;
		}
	}
	pendingInst = 0;

	return RETCODE_SUCCESS;
}

RetCode VPU_DecBitBufferFlush(DecHandle handle)
{
	CodecInst * pCodecInst;
	DecInfo * pDecInfo;
	RetCode ret;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	if (pendingInst) {
		return RETCODE_FRAME_NOT_COMPLETE;
	}

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;

	if (pDecInfo->frameBufPool == 0) { // This means frame buffers have not been registered.
		return RETCODE_WRONG_CALL_SEQUENCE;
	}

	VpuWriteReg( BIT_BUSY_FLAG, 0x1 );
	BitIssueCommand(pCodecInst->instIndex, pCodecInst->codecMode, DEC_BUF_FLUSH);
	while (VpuReadReg(BIT_BUSY_FLAG))
		;

	pDecInfo->streamWrPtr = pDecInfo->streamBufStartAddr;
	VpuWriteReg(pDecInfo->streamWrPtrRegAddr, pDecInfo->streamBufStartAddr);	

	return RETCODE_SUCCESS;
}

RetCode VPU_DecClrDispFlag(DecHandle handle, int index)
{
	CodecInst * pCodecInst;
	DecInfo * pDecInfo;
	RetCode ret;
	int	val;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;

	if (pDecInfo->frameBufPool == 0) { // This means frame buffers have not been registered.
		return RETCODE_WRONG_CALL_SEQUENCE;
	}

	if ((index < 0) || (index > (pDecInfo->numFrameBuffers - 1)))
		return	RETCODE_INVALID_PARAM;

	val = 1 << index;
	val = ~val;
	val = (val & VpuReadReg(pDecInfo->frameDisplayFlagRegAddr));
	VpuWriteReg(pDecInfo->frameDisplayFlagRegAddr, val );

	return RETCODE_SUCCESS;
}

RetCode VPU_DecGiveCommand(
		DecHandle handle,
		CodecCommand cmd,
		void * param)
{
	CodecInst * pCodecInst;
	DecInfo * pDecInfo;
	RetCode ret;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	if (pendingInst) {
		return RETCODE_FRAME_NOT_COMPLETE;
	}

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;
	switch (cmd) 
	{
		case ENABLE_ROTATION :
			{
				if (!pDecInfo->rotatorOutputValid) {
					return RETCODE_ROTATOR_OUTPUT_NOT_SET;
				}
				if (pDecInfo->rotatorStride == 0) {
					return RETCODE_ROTATOR_STRIDE_NOT_SET;
				}
				pDecInfo->rotationEnable = 1;
				break;
			}

		case DISABLE_ROTATION :
			{
				pDecInfo->rotationEnable = 0;
				break;
			}

		case ENABLE_MIRRORING :
			{
				if (!pDecInfo->rotatorOutputValid) {
					return RETCODE_ROTATOR_OUTPUT_NOT_SET;
				}
				if (pDecInfo->rotatorStride == 0) {
					return RETCODE_ROTATOR_STRIDE_NOT_SET;
				}
				pDecInfo->mirrorEnable = 1;
				break;
			}
		case DISABLE_MIRRORING :
			{
				pDecInfo->mirrorEnable = 0;
				break;
			}
		case SET_MIRROR_DIRECTION :
			{

				MirrorDirection mirDir;

				if (param == 0) {
					return RETCODE_INVALID_PARAM;
				}
				mirDir = *(MirrorDirection *)param;
				if (!(MIRDIR_NONE <= mirDir && mirDir <= MIRDIR_HOR_VER)) {
					return RETCODE_INVALID_PARAM;
				}
				pDecInfo->mirrorDirection = mirDir;

				break;
			}
		case SET_ROTATION_ANGLE :
			{
				int angle;

				if (param == 0) {
					return RETCODE_INVALID_PARAM;
				}
				angle = *(int *)param;
				if (angle != 0 && angle != 90 &&
						angle != 180 && angle != 270) {
					return RETCODE_INVALID_PARAM;
				}
				if (pDecInfo->rotatorStride != 0) {				
					if (angle == 90 || angle ==270) {
						if (pDecInfo->initialInfo.picHeight > pDecInfo->rotatorStride) {
							return RETCODE_INVALID_PARAM;
						}
					} else {
						if (pDecInfo->initialInfo.picWidth > pDecInfo->rotatorStride) {
							return RETCODE_INVALID_PARAM;
						}
					}
				}
							
				pDecInfo->rotationAngle = angle;
				break;
			}

		case SET_ROTATOR_OUTPUT :
			{
				FrameBuffer * frame;

				if (param == 0) {
					return RETCODE_INVALID_PARAM;
				}
				frame = (FrameBuffer *)param;
				pDecInfo->rotatorOutput = *frame;
				pDecInfo->rotatorOutputValid = 1;
				break;
			}

		case SET_ROTATOR_STRIDE :
			{
				int stride;

				if (param == 0) {
					return RETCODE_INVALID_PARAM;
				}
				stride = *(int *)param;
				if (stride % 8 != 0 || stride == 0) {
					return RETCODE_INVALID_STRIDE;
				}
				if (pDecInfo->rotationAngle == 90 || pDecInfo->rotationAngle == 270) {
					if (pDecInfo->initialInfo.picHeight > stride) {
						return RETCODE_INVALID_STRIDE;
					}
				} else {
					if (pDecInfo->initialInfo.picWidth > stride) {
						return RETCODE_INVALID_STRIDE;
					}					
				}

				pDecInfo->rotatorStride = stride;
				break;
			}
		case DEC_SET_SPS_RBSP:
			{
				if (pCodecInst->codecMode != AVC_DEC) {
					return RETCODE_INVALID_COMMAND;
				}
				if (param == 0) {
					return RETCODE_INVALID_PARAM;
				}
				SetParaSet(handle, 0, param);
				break;
			}

		case DEC_SET_PPS_RBSP:
			{
				if (pCodecInst->codecMode != AVC_DEC) {
					return RETCODE_INVALID_COMMAND;
				}
				if (param == 0) {
					return RETCODE_INVALID_PARAM;
				}
				SetParaSet(handle, 1, param);
				break;
			}
		case DEC_SET_DEBLOCK_OUTPUT:
			{
				FrameBuffer * frame;

				if (param == 0) {
					return RETCODE_INVALID_PARAM;
				}
				frame = (FrameBuffer *)param;
				pDecInfo->deBlockingFilterOutput = *frame;
				pDecInfo->deBlockingFilterOutputValid = 1;
				break;
			}
		default:
			return RETCODE_INVALID_COMMAND;
	}
	return RETCODE_SUCCESS;
}
