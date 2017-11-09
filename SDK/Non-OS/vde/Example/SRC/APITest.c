/****************************************************************************
 *                                                                                    
 * Copyright (c) Nuvoton Technology Corp. All rights reserved.           
 *                                                                                    
 ***************************************************************************/ 

#include <stdio.h>
#include <string.h>
#include "w55fa95_reg.h"
#include "wblib.h"
#include "wbio.h"
#include "vdodef.h"
#include "vdoapi.h"
#include "avctest.h"
 
#include "nvtfat.h" 
#include "w55fa95_sic.h"
#include "w55fa95_vpe.h"

 
int g_newsize=-1,g_size=0;
extern UINT32 g_startaddr,g_endaddr;
 
void CheckVersion(void)
{
	
	RetCode ret = RETCODE_SUCCESS;
		
	char productstr[18]={0};
	char versionstr[18]={0};
	UINT32 versioninfo;
	UINT16 pn;
	UINT16 version;
	UINT8  ipprjnum;
	
	ret = vdoGetVersionInfo( &versioninfo );	
	if( ret != RETCODE_SUCCESS )
	{
	    Console_Printf("vdoGetVersion failed !\n");
	    return;
	}    
	    
	
	pn = (Uint16)(versioninfo>>16);
	version = (Uint16)versioninfo;
	ipprjnum = (Uint8)(pn);
	

	if (pn != PRJ_BODA_DX_5B)
	{	
	    Console_Printf("Video Codec ID is wrong\n");
	    //while(1);
	}
	else
		strcpy( productstr, "BodaDX5B");
			
	sprintf( versionstr, "%04d.%04d.%08d", (version>>(12))&0x0f, (version>>(8))&0x0f, (version)&0xff );
	Console_Printf("VDO Firmware Version => \n product : %s | version : %s\n\n", productstr, versionstr );
	
}

// read bitstream to bistream buffer from file for decoding
int  WriteSdramBurst(FileOpInfo* pBufInfo, int addr, int byteSize, int startaddr,int endaddr)
{

	char codecFileNameAbsolute[256];
	int nByteCnt;
	char suDirName[256];		
	int writesize;
	
    if (pBufInfo->UsedOffset > pBufInfo->TotalFileSize)
        return FALSE;
        
	strcpy(codecFileNameAbsolute, pBufInfo->Name);	

    fsAsciiToUnicode(codecFileNameAbsolute,suDirName,1);                         
   	if((pBufInfo->OpenFP = fsOpenFile(suDirName,codecFileNameAbsolute,O_RDONLY)) < 0) 	
	{
		Console_Printf("    Can't open %s to read.\n",pBufInfo->Name);
		return FALSE;
	}   	
		  
	fsFileSeek(pBufInfo->OpenFP, pBufInfo->UsedOffset, SEEK_SET);
	
	if ((addr + byteSize) > endaddr)
	{
		writesize = endaddr - addr;
		fsReadFile(pBufInfo->OpenFP,(UINT8 *)(addr | CACHE_BIT31), writesize, &nByteCnt);
		pBufInfo->UsedOffset +=   nByteCnt;						
		byteSize -= writesize;
		fsReadFile(pBufInfo->OpenFP,(UINT8 *)(startaddr | CACHE_BIT31), byteSize, &nByteCnt);			
		pBufInfo->UsedOffset +=   nByteCnt;				
	}			
	else
	{
		fsReadFile(pBufInfo->OpenFP,(UINT8 *)(addr | CACHE_BIT31), byteSize, &nByteCnt);			
		pBufInfo->UsedOffset +=   nByteCnt;				
	}
			
	fsCloseFile(pBufInfo->OpenFP);
	pBufInfo->OpenFP=0;


    return 1;	    
}    


// implement ring buffer 
// Write bitstream to bistream buffer for decoding
int FillSdramBurst( FileOpInfo * pBufInfo, Uint32 targetAddr, Uint32 size, int *writesize, int startaddr, int endaddr )
{
    UINT32 countbyte=0;
    
    if (pBufInfo->UsedOffset >= pBufInfo->TotalFileSize)
    {
       // All bitstream is read to bitstream buffer, It should be not here 
       *writesize =0;
		return 1;        
    }
    
    if ((pBufInfo-> TotalFileSize - pBufInfo-> UsedOffset) < size)
    {
        countbyte = pBufInfo-> TotalFileSize - pBufInfo-> UsedOffset;
    }
    else
    {
        countbyte = size;
    }
    
    *writesize = countbyte;

	WriteSdramBurst(pBufInfo, targetAddr, countbyte,startaddr,endaddr);
    
	return 1;
}


// Write bitstream to stream buffer for decoding
RetCode WriteBsBufHelper( DecHandle handle, FileOpInfo *pBufInfo, int defaultsize)
{
    
	RetCode ret = RETCODE_SUCCESS;
	int size = 0;
	PhysicalAddress paWrPtr;
	int writesize;
	
	ret = vdoDecGetBitstreamBuffer(handle, &paWrPtr, (Uint32 *)&size);
	if( ret != RETCODE_SUCCESS )
	{
		Console_Printf( "vdoDecGetBitstreamBuffer failed Error code is 0x%x \n", ret );
		goto FILL_BS_ERROR;				
	}
	
	if (size <=0)
	    return RETCODE_SUCCESS;
	    
	// Only load first K bytes to parse the header information
	// This is used to avoid the register value of 0x114 for vdoDecUpdateBitstreamBuffer(handle,0) cleared by VPU_DecSetEscSeqInit(..) or other function
	if (defaultsize != 0)
	{
	    if (defaultsize < size)
	        size = defaultsize;
    }	 

	if( !FillSdramBurst( pBufInfo, paWrPtr,  size,  &writesize,g_startaddr,g_endaddr) )
	{
		Console_Printf(  "FillSdramBurst failed Error code is 0x%x \n", ret );
		goto FILL_BS_ERROR;
	}
	
	ret = vdoDecUpdateBitstreamBuffer(handle, writesize);
	if( ret != RETCODE_SUCCESS )
	{
		Console_Printf(  "vdoDecUpdateBitstreamBuffer failed Error code is 0x%x \n", ret );
		goto FILL_BS_ERROR;					
	}		
		
	// This is for the last portion of bitstream 
	if (writesize < size)
	{
		ret = vdoDecUpdateBitstreamBuffer(handle, STREAM_END_SIZE);
		if( ret != RETCODE_SUCCESS )
		{
			Console_Printf(  "vdoDecUpdateBitstreamBuffer failed Error code is 0x%x \n", ret );
			goto FILL_BS_ERROR;					
		}			    
	}

	
FILL_BS_ERROR:
	
	return ret;
}

#define FREE_TH_SIZE 128*1024 
int checkFreeBuf(DecHandle handle,FileOpInfo *DecbufInfo)
{
	PhysicalAddress paWrPtr;

	vdoDecGetBitstreamBuffer(handle, &paWrPtr, (Uint32 *)&g_size);		

	g_newsize = g_size;
	
	if ((g_size >= FREE_TH_SIZE) || (g_size==512))	  
	{
		WriteBsBufHelper( handle, DecbufInfo, g_size);	 	
		//if (g_newsize != g_size)		
		//	Console_Printf("WriteBsBufHelper size=%d\n",g_size);
	}
	return 0;
}

void DecoderAPI(DecOpenParam *decOP,FileOpInfo *DecbufInfo,DecConfigParam *decConfig,int vpost)
{
    
	DecHandle		handle		= { 0 };
	DecInitialInfo	initialInfo = { 0 };
	DecBufInfo		decSliceBufInfo	= { 0 };
	DecOutputInfo	outputInfo	= { 0 };
	DecParam		decParam	= { 0 };
	FrameBuffer		frameBuf[NUM_FRAME_BUF] = { 0 };
	FRAME_BUF *		pFrame[NUM_FRAME_BUF]	= { 0 };	
	RetCode			ret = RETCODE_SUCCESS;	
	Uint32			fRateInfo = 0,  framebufWidth = 0, framebufHeight = 0;
	int				frameIdx = 0, totalNumofErrMbs = 0, stride = 0, decodefinish = 0;
	int				dispIdx = 0,prevDispIdx=-1;
	int             needFrameBufCount=0,i,rotIdx,deblkIdx,rotStride;
	MirrorDirection mirrorDirection;	
	int count = 0;
	int waitingEn = 0;
	int framerate;
	
		    
	ret = vdoDecOpen(&handle, decOP);	
	if( ret != RETCODE_SUCCESS )
	{
		Console_Printf("vdoDecOpen failed Error code is 0x%x \n", ret );
		goto ERR_DEC_INIT;
	}
	
	// Only load first N bytes to parser the header information. 
	// The only limtation is not to larger than the total file size.
#if 0	
	ret = WriteBsBufHelper( handle, DecbufInfo, 1024);
#else	
	ret = WriteBsBufHelper( handle, DecbufInfo, STREAM_BUF_SIZE);	
#endif	
	if( ret != RETCODE_SUCCESS )
	{
		Console_Printf("WriteBsBufHelper failed Error code is 0x%x \n", ret );
		goto ERR_DEC_OPEN;
	}
	

	ret = vdoDecGetInitialInfo(handle, &initialInfo);
	if( ret != RETCODE_SUCCESS )
	{
		if(ret == RETCODE_NOTSUPPORTED)
		{
			Console_Printf( "VPU_DecGetInitialInfo failed Error code is 0x%x (Not supported)\n", ret );
			Console_Printf( "VPU SIZE X = %d Y = %d\n", initialInfo.picWidth,initialInfo.picHeight );
		}
		else
			Console_Printf( "VPU_DecGetInitialInfo failed Error code is 0x%x \n", ret );
	
		goto ERR_DEC_OPEN;
	}
	
	fRateInfo = initialInfo.frameRateInfo;

	framebufWidth = ( ( initialInfo.picWidth + 15 ) & ~15 );
	framebufHeight = ( ( initialInfo.picHeight + 15 ) & ~15 );
	
	framerate = (fRateInfo & 0xffff)%(((fRateInfo >> 16) + 1)&0xffff);
	Console_Printf("Width: %u picHeight: %u frameRate: %d.%2d",
		initialInfo.picWidth, initialInfo.picHeight, (int)(fRateInfo & 0xffff)/(((fRateInfo >> 16) + 1)&0xffff),framerate);
		
					

    // Here only uses 1 frame for rotation and 1 frame for MP4 deblocking filter. It is just to simplify the sample
    // Using 1 or 2 frame for these operation is dependent on you. You must take care of buffer index if you use 2 buffer for each operation.
       
	if( decConfig->useRot )
		needFrameBufCount = initialInfo.minFrameBufferCount * 2;
	else
		needFrameBufCount = initialInfo.minFrameBufferCount  + ( ( decOP->mp4DeblkEnable == 1 )? 2 : 0 );        
		
	Console_Printf(", FrameBufCnt=%d\n",needFrameBufCount);	

    // Buffer allocation is dependent on application.    
	FrameBufferInit( initialInfo.picWidth, initialInfo.picHeight,ADDR_FRAME_BASE );
	
	for( i = 0; i < needFrameBufCount; ++i ) 
	{
		pFrame[i] = GetFrameBuffer(i);
		frameBuf[i].bufY = pFrame[i]->AddrY;
		frameBuf[i].bufCb = pFrame[i]->AddrCb;
		frameBuf[i].bufCr = pFrame[i]->AddrCr;
	}	
	
	stride = framebufWidth ;
	
	// Register frame buffers requested by the decoder.
	decSliceBufInfo.avcSliceBufInfo.sliceSaveBuffer = ADDR_SLICE_SAVE_BUFFER;
	decSliceBufInfo.avcSliceBufInfo.sliceSaveBufferSize = SLICE_SAVE_SIZE;
	ret = VPU_DecRegisterFrameBuffer( handle, frameBuf, initialInfo.minFrameBufferCount, stride, &decSliceBufInfo );

	if( ret != RETCODE_SUCCESS )
	{
		Console_Printf("vdoDecRegisterFrameBuffer failed Error code is 0x%x \n", ret );
		goto ERR_DEC_OPEN;
	}
	
	decParam.prescanEnable = decConfig->prescanEnable;
	
	//-----------------------------------------------------------------------------------------------------------------------------------------------------
	//  Frame buffer num    minFrameBufferCount-1    minFrameBufferCount                                            Action
	//------------------------------------------------------------------------------------------------------------------------------------------------------	
	//      0 ......................... N-1                 N (decoded frame)         x                       -> No roate & No MP4deblocking
	//                                                      N (Rotate frame)          N+1(MP4Delock frame)    -> Rotate + MP4Deblocking
	//                                                      N (MP4Delock frame)       x                       -> MP4Deblocking only
	//------------------------------------------------------------------------------------------------------------------------------------------------------	
	if( decConfig->useRot )
	{
		rotIdx = initialInfo.minFrameBufferCount;
		if( decOP->mp4DeblkEnable == 1 )
			deblkIdx = rotIdx + 2;			
			
		dispIdx = rotIdx;			
	}
	else
	{
		if( decOP->mp4DeblkEnable == 1 )
		{
			deblkIdx = initialInfo.minFrameBufferCount;
			dispIdx = deblkIdx;				
    	}
    	else
			dispIdx = 0;    	
	}
	
		
	if (decConfig->useRot)
	{
		vdoDecGiveCommand(handle, SET_ROTATION_ANGLE, &decConfig->rotAngle);
		mirrorDirection = decConfig->mirDir;
		vdoDecGiveCommand(handle, SET_MIRROR_DIRECTION, &mirrorDirection);

		rotStride = ( decConfig->rotAngle == 90 || decConfig->rotAngle == 270 ) ? framebufHeight : framebufWidth;
		vdoDecGiveCommand(handle, SET_ROTATOR_STRIDE, &rotStride);	    
    }	    
    
          
    
	while( 1 )
	{
	    
		if( decOP->mp4DeblkEnable == 1 ) 
		{
			ret = vdoDecGiveCommand(handle, DEC_SET_DEBLOCK_OUTPUT, &frameBuf[deblkIdx] );
			if( ret != RETCODE_SUCCESS )	
			{
				Console_Printf("vdoDecGiveCommand failed Error code is 0x%x \n", ret );
				goto ERR_DEC_OPEN;
			}
		}
		
		if( decConfig->useRot && (outputInfo.indexFrameDisplay >= 0) )
		{
			if (waitingEn)
			{
				vdoDecGiveCommand( handle, SET_ROTATOR_OUTPUT, &frameBuf[rotIdx] );
				rotIdx = outputInfo.indexFrameDisplay + initialInfo.minFrameBufferCount;
			}
			else
			{
				vdoDecGiveCommand( handle, SET_ROTATOR_OUTPUT, &frameBuf[rotIdx] );
			
			}
			
			if( frameIdx == 0 ) 
			{
				vdoDecGiveCommand(handle, ENABLE_ROTATION, 0);
				vdoDecGiveCommand(handle, ENABLE_MIRRORING, 0);
			}			
			
		}		

        BITPicRunOK = FALSE;
        BITBufferEmptyOK=FALSE;
        
	    // Start decoding a frame.
		ret = vdoDecStartOneFrame(handle, &decParam );
		
    	if( ret != RETCODE_SUCCESS )
    	{
    		Console_Printf("vdoDecStartOneFrame failed Error code is 0x%x \n", ret );
    		goto ERR_DEC_OPEN;
    	}
#if 0
		// Check Busy by API
		while( vdoIsBusy() ) 
		{
		    while ((BITBufferEmptyOK==FALSE) && (BITPicRunOK == FALSE));
			    
		    if (BITPicRunOK)
		    {   // Decode complete. Don't write bitstream now.
		        break;
            }			        

            
            //BITBufferEmptyOK=FALSE; 
		    ret = WriteBsBufHelper( handle, DecbufInfo, 0);				    		
		    
   			if( ret != RETCODE_SUCCESS )
   			{
   				Console_Printf("WriteBsBufHelper failed Error code is 0x%x \n", ret );
   				goto ERR_DEC_OPEN;
   			}
		}
#else
		// Set flag by interrupt
		while( BITPicRunOK == FALSE ) 
		{
		    while ((BITBufferEmptyOK==FALSE) && (BITPicRunOK == FALSE))
			{
				checkFreeBuf(handle,DecbufInfo);		    
			};
 
			    
		    if (BITPicRunOK)
		    {   // Decode complete. Don't write bitstream now.
		        break;
            }			        

		    ret = WriteBsBufHelper( handle, DecbufInfo, 0);	            
			Console_Printf("BITBufferEmptyOK, frameIndx=%d\n",frameIdx);		    
		    
   			if( ret != RETCODE_SUCCESS )
   			{
   				Console_Printf("WriteBsBufHelper failed Error code is 0x%x \n", ret );
   				goto ERR_DEC_OPEN;
   			}
		}
#endif
        BITPicRunOK = FALSE;
        
		ret = vdoDecGetOutputInfo(handle, &outputInfo);
		if( ret == RETCODE_FAILURE )
		{	// Careful -> If return value is RETCODE_FAILURE, trigger vdoDecStartOneFrame(..) again.
			Console_Printf("vdoDecGetOutputInfo decode fail framdIdx %d \n", frameIdx);
			
			if( outputInfo.numOfErrMBs ) 
			{
			    Console_Printf("Warnning : Should be no ErrorMB here\n");
			}
			
			continue;
		}		
		else if( ret != RETCODE_SUCCESS )
		{
			Console_Printf("vdoDecGetOutputInfo failed Error code is 0x%x \n", ret );
			goto ERR_DEC_OPEN;
		}
		
		
		if( outputInfo.decodingSuccess == 0 )
		{
			Console_Printf("VPU_DecGetOutputInfo decode fail framdIdx %d \n", frameIdx);
		} else if (outputInfo.decodingSuccess == -1)
		{
			Console_Printf("VPU_DecGetOutputInfo decode fail (Not Supported) framdIdx %d \n", frameIdx);
		}

		//Console_Printf("frameidx %d | indexFrameDisplay %d | indexFrameDecoded %d | RdPtr %x | WrPtr %x | picType %d\n", 
		//			frameIdx, outputInfo.indexFrameDisplay, outputInfo.indexFrameDecoded, 
		//			VpuReadReg( BIT_RD_PTR_0 ) , VpuReadReg( BIT_WR_PTR_0 ), outputInfo.picType );		
	
		if( outputInfo.indexFrameDisplay == -1 )
			decodefinish = 1;
		else if ( outputInfo.indexFrameDisplay == -2 ) 
			decodefinish = 1;
		else if( ( outputInfo.indexFrameDisplay > needFrameBufCount ) && ( outputInfo.prescanresult != 0 ) )
			decodefinish = 1;		
		
		if( decodefinish )
			break;					
       
		
		if( outputInfo.prescanresult == 0 && decParam.prescanEnable == 1)	// if prescan is NG		
		{
			{
				UINT32 size = 0;
				PhysicalAddress paWrPtr;
				
				ret = vdoDecGetBitstreamBuffer(handle, &paWrPtr, &size);
				
				if( ret != RETCODE_SUCCESS )
				{
					Console_Printf("vdoDecGetBitstreamBuffer failed Error code is 0x%x \n", ret );
					goto ERR_DEC_OPEN;				
				}
				if( size < STREAM_FILL_SIZE )
				{
					Console_Printf("Warnning!! Bitstream buffer size is smaller than size of one picture stream.\n" );
				}

    			ret = WriteBsBufHelper( handle, DecbufInfo, 0);				
				if( ret != RETCODE_SUCCESS )
				{
					Console_Printf("WriteBsBufHelper failed Error code is 0x%x \n", ret );
					goto ERR_DEC_OPEN;
				}
				continue;
			}
		}		
		
		if( decConfig->useRot)
		{
			if((outputInfo.indexFrameDisplay == -3 || outputInfo.indexFrameDisplay == -2))
			{
				waitingEn = 1;
				rotIdx = ( rotIdx + 1 ) % initialInfo.minFrameBufferCount + initialInfo.minFrameBufferCount;
				vdoDecGiveCommand( handle, SET_ROTATOR_OUTPUT, &frameBuf[rotIdx] );
				continue;
			}
		}
		else if( outputInfo.indexFrameDisplay == -3 || outputInfo.indexFrameDisplay == -2 ) 
			continue;
		
			
        {
			//int interLeave = 0;            
            int saveIdx = 0;
            
			if( decConfig->useRot )
			{
				if(waitingEn)
				{				
					if (count == 0)
					{
						rotIdx = ( rotIdx + 1 ) % initialInfo.minFrameBufferCount + initialInfo.minFrameBufferCount;
						count++;
					}
				}
				else
				{
					rotIdx = ( rotIdx + 1 ) % initialInfo.minFrameBufferCount + initialInfo.minFrameBufferCount;
				}
				if( decOP->mp4DeblkEnable == 1 )
					deblkIdx = ( rotIdx + 2 );									
				
				//dispIdx = rotIdx;		
				saveIdx = outputInfo.indexFrameDisplay + initialInfo.minFrameBufferCount;
		
			}
			else
			{
				if( decOP->mp4DeblkEnable == 1 )
				{
					saveIdx = dispIdx;
					deblkIdx = ( deblkIdx + 1 ) % 2 + ( initialInfo.minFrameBufferCount );					
					dispIdx = deblkIdx;
				}
				else
					saveIdx = outputInfo.indexFrameDisplay;
			}
			            
      
   		// Display to VPOST here
        {
            int srcWidth,srcHeight,vpostWidth,vpostHeight,vpostStride;

            vpostWidth = LCM_WIDTH;
            vpostHeight = LCM_HEIGHT;             

            // For VPE rotate
            if (decConfig->rotAngle == 90 || decConfig->rotAngle == 270)
            {
	            srcHeight = (UINT32)initialInfo.picWidth;
	            srcWidth = (UINT32)initialInfo.picHeight;            
            }
            else
            {
	            srcWidth = (UINT32)initialInfo.picWidth;
	            srcHeight = (UINT32)initialInfo.picHeight;
            }

            vpostStride = LCM_WIDTH;
               
#ifndef _OUTPUT_OTF_             
            while(vpeCheckTrigger());
        
#ifdef _No_VPE_Upscale_     
		if (LCM_HEIGHT >= srcHeight)
		{
	    	if (LCM_WIDTH >= srcWidth)
	    	{	// LCM_Height >= srcHeight, LCM_Width >= srcWidth
		    	vpeConfig(frameBuf[saveIdx].bufY,frameBuf[saveIdx].bufCb,frameBuf[saveIdx].bufCr,
	        	 VPOSDISPLAYBUFADDR+(LCM_HEIGHT-srcHeight)/2*2*LCM_WIDTH,(LCM_WIDTH - srcWidth)/2, 
	        	 srcWidth, srcHeight, srcWidth,srcHeight,LCM_WIDTH); 	
	        }
	        else
	        {	// LCM_Height >= srcHeight, LCM_Width < FrameHeight
		    	vpeConfig(frameBuf[saveIdx].bufY,frameBuf[saveIdx].bufCb,frameBuf[saveIdx].bufCr,
	        	 VPOSDISPLAYBUFADDR+(LCM_HEIGHT-framebufWidth)/2*2*LCM_WIDTH,0, 
	        	 srcWidth, srcHeight, LCM_WIDTH,srcHeight,LCM_WIDTH); 	        
	        }	 
		}	        	 
	    else
	    {
	    	if (LCM_WIDTH >= framebufHeight)
	    	{	// LCM_Height < srcHeight, LCM_Width >= FrameHeight
			    vpeConfig(frameBuf[saveIdx].bufY,frameBuf[saveIdx].bufCb,frameBuf[saveIdx].bufCr,VPOSDISPLAYBUFADDR,
		        	 (LCM_WIDTH - framebufHeight)/2, 
		        	 srcWidth, srcHeight, srcWidth,LCM_HEIGHT,LCM_WIDTH); 	    
			}
			else
			{	// LCM_Height < srcHeight, LCM_Width < FrameHeight
			    vpeConfig(frameBuf[saveIdx].bufY,frameBuf[saveIdx].bufCb,frameBuf[saveIdx].bufCr,VPOSDISPLAYBUFADDR, 0,
		        	 srcWidth, srcHeight, LCM_WIDTH,LCM_HEIGHT,LCM_WIDTH); 	    
			
			}	        	 
	    }   
   	      
#else	    
		    vpeConfig(frameBuf[saveIdx].bufY,frameBuf[saveIdx].bufCb,frameBuf[saveIdx].bufCr,
	        	 VPOSDISPLAYBUFADDR,0, srcWidth, srcHeight, vpostWidth,vpostHeight,vpostStride); 	        
#endif            
	       if( decConfig->useRot)
	       {
	       		UINT32 srcrightoffset;
	       		// if not multiple of 16
	       		if (initialInfo.picHeight & 0xF)
	       		{
		       		srcrightoffset = (srcWidth + 15)/16 * 16 - srcWidth;
	       			outp32(REG_VPE_SLORO, inp32(REG_VPE_SLORO) & ~0x1FFF | srcrightoffset);
	       		}
	       }
			// Trigger VPE
            vpeIoctl(VPE_IOCTL_TRIGGER, 0,0,0);        
#endif            
        } 

        }    

        	        
		frameIdx++;		

		if (prevDispIdx >= 0)
		{
			vdoDecClrDispFlag(handle, prevDispIdx);			
		}
		prevDispIdx = outputInfo.indexFrameDisplay;	
		
		if( outputInfo.numOfErrMBs ) 
		{
			totalNumofErrMbs += outputInfo.numOfErrMBs;			
			Console_Printf(" -------> Num of Error Mbs : %d, in Frame : %d \n", outputInfo.numOfErrMBs, frameIdx);
		}
          
	}
	
ERR_DEC_OPEN:
    Console_Printf( " >> Total Decoded Frame Number = %d \n", frameIdx );
            

        if (DecbufInfo->OpenFP)
        {
            fsCloseFile(DecbufInfo->OpenFP);
            DecbufInfo->OpenFP=0;
        }


     if ((inp32(REG_VPE_CMD) & 0x30) == 0)
		 while(vpeCheckTrigger());        	// Wait complete if not on-the-fly mode
		 
	// Now that we are done with decoding, close the open instance.
	ret = vdoDecClose(handle);
	if( ret == RETCODE_FRAME_NOT_COMPLETE )
	{
		vdoDecGetOutputInfo(handle, &outputInfo);
		vdoDecClose(handle);
	}	
				
ERR_DEC_INIT:	
    return;    
}

