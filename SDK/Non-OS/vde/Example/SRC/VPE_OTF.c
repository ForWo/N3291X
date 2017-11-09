/****************************************************************************
 *                                                                                    
 * Copyright (c) Nuvoton Electronics Corp. All rights reserved.             
 *                                                                                    
 ***************************************************************************/
 
/****************************************************************************
 * 
 * FILENAME
 *     H264Pattern.c
 *
 * VERSION
 *     1.0
 *
 * DESCRIPTION
 *     Test program for W55FA95 H/W Codec.
 *
 * DATA STRUCTURES
 *     None
 *
 * FUNCTIONS
 *     H264TestPattern
 *
 * HISTORY
 *     03/16/2011		 Ver 1.0 Created by PX40 K.C.Huang
 *
 * REMARK
 *     None
 **************************************************************************/
#include <stdio.h>
#include <string.h>

#include "w55fa95_reg.h"
#include "wblib.h"
#include "wbio.h"

//#include "VPUAPI.h"
#include "vdodef.h"
#include "vdoapi.h"

#include "avc.h"
#include "avctest.h"
#include "nvtfat.h" 

#include "w55fa95_vpe.h"


static void DecoderAPIx(DecOpenParam *decOP,FileOpInfo *DecbufInfo,DecConfigParam *decConfig,int vpost,
						int SrcLeftOff, int SrcRightOff, int DestLeftOff, int DestRightOff, int VPERotate)
{
    
	DecHandle		handle		= { 0 };
	DecInitialInfo	initialInfo = { 0 };
#ifndef SLICE_BUF
	DecBufInfo		decSliceBufInfo	= { 0 };
#endif		
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

	
	
		    
	ret = vdoDecOpen(&handle, decOP);	
	if( ret != RETCODE_SUCCESS )
	{
		Console_Printf("vdoDecOpen failed Error code is 0x%x \n", ret );
		goto ERR_DEC_INIT;
	}
	
	ret = WriteBsBufHelper( handle, DecbufInfo, STREAM_BUF_SIZE);	

	if( ret != RETCODE_SUCCESS )
	{
		Console_Printf("WriteBsBufHelper failed Error code is 0x%x \n", ret );
		goto ERR_DEC_OPEN;
	}
	

	ret = vdoDecGetInitialInfo(handle, &initialInfo);
	if( ret != RETCODE_SUCCESS )
	{
		//Console_Printf("vdoDecGetInitialInfo failed Error code is 0x%x \n", ret );
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
	
	
{
int framerate;
framerate = (fRateInfo & 0xffff)%(((fRateInfo >> 16) + 1)&0xffff);
	Console_Printf("Width: %u picHeight: %u frameRate: %d.%2d\n",
		initialInfo.picWidth, initialInfo.picHeight, (int)(fRateInfo & 0xffff)/(((fRateInfo >> 16) + 1)&0xffff),framerate);
}		
					
	

 
	if( decConfig->useRot )
		needFrameBufCount = initialInfo.minFrameBufferCount * 2;
	else
		needFrameBufCount = initialInfo.minFrameBufferCount  + ( ( decOP->mp4DeblkEnable == 1 )? 2 : 0 );        
		

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
	//ret = vdoDecRegisterFrameBuffer( handle, frameBuf, initialInfo.minFrameBufferCount, stride );
#ifndef SLICE_BUF
	decSliceBufInfo.avcSliceBufInfo.sliceSaveBuffer = ADDR_SLICE_SAVE_BUFFER;
	decSliceBufInfo.avcSliceBufInfo.sliceSaveBufferSize = SLICE_SAVE_SIZE;
	
	// Register frame buffers requested by the decoder.

	ret = VPU_DecRegisterFrameBuffer( handle, frameBuf, initialInfo.minFrameBufferCount, stride, &decSliceBufInfo );
#else
	ret = VPU_DecRegisterFrameBuffer( handle, frameBuf, initialInfo.minFrameBufferCount, stride);
#endif		
	if( ret != RETCODE_SUCCESS )
	{
		Console_Printf("vdoDecRegisterFrameBuffer failed Error code is 0x%x \n", ret );
		goto ERR_DEC_OPEN;
	}
	
#ifdef _OUTPUT_OTF_
		outp32(0xB100C800, inp32(0xB100C800) & ~0x01); 	
	    //vpeConfig(frameBuf[0].bufY,frameBuf[0].bufCb,frameBuf[0].bufCr,VPOSDISPLAYBUFADDR, 0,
	    //     framebufWidth, framebufHeight, framebufWidth,framebufHeight,LCM_WIDTH); 
	         
		vpeConfig(frameBuf[0].bufY,frameBuf[0].bufCb,frameBuf[0].bufCr,
	      	 VPOSDISPLAYBUFADDR+(20+SrcLeftOff)*LCM_WIDTH*2,0, framebufWidth-SrcLeftOff-SrcRightOff, framebufHeight, 
	      	 					framebufWidth-SrcLeftOff-SrcRightOff,framebufHeight,LCM_WIDTH); 	        
	   
	   //vpeSetSrcDestOffsetRotate(SrcLeftOff, SrcRightOff,framebufWidth,LCM_WIDTH-framebufHeight-framebufWidth,VPERotate);	
	   
	   //             (StartX,    StartY,      EndX,                       EndY,                         ImageWidth,   ImageHeigh);
	    vpeSetOTFCrop(SrcLeftOff, SrcRightOff, framebufWidth - SrcLeftOff, framebufHeight - SrcRightOff, framebufWidth,framebufHeight,VPERotate );	   
	    if ((VPERotate == VPE_OP_RIGHT) || (VPERotate == VPE_OP_LEFT))
	    {
		    vpeSetDestOffset(framebufWidth,LCM_WIDTH-framebufWidth-(framebufHeight - SrcRightOff -SrcRightOff));
	       	//vpeSetSrcDestOffsetRotate(SrcLeftOff, SrcRightOff,framebufWidth,LCM_WIDTH-framebufHeight-framebufWidth,VPERotate);			    
		}	       	
	    else	 
	    {
	       	if (VPERotate == VPE_OP_NORMAL)
	       	{
			    //vpeSetOTFCrop(SrcLeftOff, SrcRightOff, framebufWidth - SrcLeftOff, framebufHeight - SrcRightOff, framebufWidth,framebufHeight);
			    vpeSetDestOffset(framebufWidth,LCM_WIDTH-framebufWidth-(framebufWidth - SrcLeftOff-SrcLeftOff));	       	
		       	//vpeSetSrcDestOffsetRotate(SrcLeftOff, SrcRightOff,framebufWidth,LCM_WIDTH-framebufHeight-framebufWidth,VPERotate);	       	
			}		       	
	       	else
	       	{	// For UpsideDown, Flip, FLop...
			    vpeSetDestOffset(framebufWidth,LCM_WIDTH-framebufWidth-(framebufWidth - SrcLeftOff-SrcLeftOff));		       	
	       		//vpeSetSrcDestOffsetRotate(SrcLeftOff, SrcRightOff,framebufWidth,LCM_WIDTH-(framebufWidth-SrcLeftOff-SrcRightOff)-framebufWidth,
	       		//						  VPERotate);	       	
			}	       								  
	   }
#endif	         
      
	
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
        
		//clearTVBuf();        
        
#ifdef _OUTPUT_OTF_   
            vpeIoctl(VPE_IOCTL_TRIGGER, 0,0,0); 
#else                      
            while(vpeCheckTrigger());
#endif            
	    // Start decoding a frame.
		ret = vdoDecStartOneFrame(handle, &decParam );
		
    	if( ret != RETCODE_SUCCESS )
    	{
    		Console_Printf("vdoDecStartOneFrame failed Error code is 0x%x \n", ret );
    		goto ERR_DEC_OPEN;
    	}

		// Set flag by interrupt
		while( BITPicRunOK == FALSE ) 
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
        
		    vpeConfig(frameBuf[saveIdx].bufY,frameBuf[saveIdx].bufCb,frameBuf[saveIdx].bufCr,
#if 1		    
	        	 VPOSDISPLAYBUFADDR+((LCM_HEIGHT-srcHeight)/2 + SrcLeftOff)*2*LCM_WIDTH,0, 
#else	
	        	 VPOSDISPLAYBUFADDR+(20+SrcLeftOff)*LCM_WIDTH*2,0,         	 
#endif
	        	 srcWidth-SrcLeftOff-SrcRightOff, srcHeight, srcWidth-SrcLeftOff-SrcRightOff,srcHeight,vpostStride); 	        
	        	 
	        	 
	        if ((VPERotate == VPE_OP_RIGHT) || (VPERotate == VPE_OP_LEFT))
	        	vpeSetSrcDestOffsetRotate(SrcLeftOff, SrcRightOff,srcWidth,LCM_WIDTH-srcHeight-srcWidth,VPERotate);		        		
	        else	 
	        {
	        	if (VPERotate == VPE_OP_NORMAL)
	        		vpeSetSrcDestOffsetRotate(SrcLeftOff, SrcRightOff,0,LCM_WIDTH-(srcWidth-SrcLeftOff-SrcRightOff),VPERotate);		
	        	else
	        		vpeSetSrcDestOffsetRotate(SrcLeftOff, SrcRightOff,srcWidth,LCM_WIDTH-(srcWidth-SrcLeftOff-SrcRightOff)-srcWidth,
	        								  VPERotate);		        		        
	        }	
       
			// Trigger VPE
            vpeIoctl(VPE_IOCTL_TRIGGER, 0,0,0);        
#endif            
        } 

        
		frameIdx++;		
#if (SD_FILE_LIBRARY ==0)		
Console_Printf( " >>  Decoded Frame Number = %d \n", frameIdx );	
#endif


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
           
		if( frameIdx >= 100 )
		    break;	
   
   		}
	}
	
ERR_DEC_OPEN:
    Console_Printf( " >> Total Decoded Frame Number = %d \n", frameIdx );
            

#if SD_FILE_LIBRARY	

        if (DecbufInfo->OpenFP)
        {
            fsCloseFile(DecbufInfo->OpenFP);
            DecbufInfo->OpenFP=0;
        }

        if (vpost == DECODE_TO_FILE)
        {
            if (WriteFile.OpenFP)  
            {
  	            fsCloseFile(WriteFile.OpenFP);
   	            WriteFile.OpenFP=0;                
            }
        }              
#endif	            

#ifndef _OUTPUT_OTF_ 
	while(vpeCheckTrigger()); 
#endif
        
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

void VPEDecodOneFile(char *FileName,int SrcLeftOff, int SrcRightOff, int DestLeftOff, int DestRightOff, int Rotate, int bSorenson)
{
   
	DecOpenParam		decOP		= { 0 };
	FileOpInfo			ReadFileInfo     = { 0 };
	DecConfigParam		decConfig = { 0 };		

	char BitStreamFileName[256];//,DecodedFileName[128];
	
    decOP.bitstreamFormat = STD_MPEG4;
   	decOP.sorensonEnable = bSorenson;     
    decOP.bitstreamBuffer = ADDR_BIT_STREAM; 
   	decOP.bitstreamBufferSize = STREAM_BUF_SIZE;    
    
    decOP.mp4DeblkEnable = 0;
     
       
    decOP.reorderEnable = 1;

    decConfig.prescanEnable= 0;  
    
    	
    strcpy(BitStreamFileName,FileName);
    Console_Printf("Testing %s file..\n",BitStreamFileName); 
    FileOP(&ReadFileInfo,READ_BITSTREAM,BitStreamFileName,0);
    
    DecoderAPIx(&decOP,&ReadFileInfo,&decConfig,DECODE_TO_VPOST, SrcLeftOff,SrcRightOff, DestLeftOff,DestRightOff, Rotate);
    
    clearTVBuf();
	     
}


void FindFile(char *subDir,int codec, int bSorenson)
{
	INT         nStatus;
	CHAR		szLongName[MAX_FILE_NAME_LEN];	
	CHAR		suDirName[256],fullPathName[256];	
	FILE_FIND_T tFileInfo;
	int  subdirlength;
	int i;

    subdirlength = strlen(subDir);
    
	memset((CHAR *)&tFileInfo, 0, sizeof(FILE_FIND_T));


    strcpy(fullPathName,INPUT_PATTERN_FOLDER); 
    strcat(fullPathName,subDir); 
	fsAsciiToUnicode(fullPathName, suDirName, 1);
	nStatus = fsFindFirst(suDirName,NULL,&tFileInfo);
	
	if (nStatus < 0)
	{
		Console_Printf("No file found in %s folder\n", fullPathName);
		return;
	}		
	
	do 
	{
		if (tFileInfo.ucAttrib & FA_ARCHIVE)  
        {		
    		fsUnicodeToAscii(tFileInfo.suLongName, szLongName, 1);
            strcpy(fullPathName,subDir);    		
            strcat(fullPathName,szLongName);     		
			break;
		}			
	} while (!fsFindNext(&tFileInfo));		
	
    
#if 0
	//debug    
	for(i=8;i<60;i++)    
		VPEDecodOneFile(fullPathName,i,i,0,0,VPE_OP_NORMAL,bSorenson); 
//		VPEDecodOneFile(fullPathName,i,i,0,0,VPE_OP_RIGHT);		
#else		

	for(i=0;i<60;i++)    
		VPEDecodOneFile(fullPathName,i,i,0,0,VPE_OP_NORMAL,bSorenson); 
	
	for(i=0;i<60;i++)
		VPEDecodOneFile(fullPathName,i,i,0,0,VPE_OP_FLIP,bSorenson);
		
	for(i=0;i<60;i++)
		VPEDecodOneFile(fullPathName,i,i,0,0,VPE_OP_FLOP,bSorenson);		
			
	for(i=0;i<60;i++)
		VPEDecodOneFile(fullPathName,i,i,0,0,VPE_OP_RIGHT,bSorenson); 
		
	for(i=0;i<60;i++)
		VPEDecodOneFile(fullPathName,i,i,0,0,VPE_OP_UPSIDEDOWN,bSorenson); 				
		
	for(i=0;i<60;i++)
		VPEDecodOneFile(fullPathName,i,i,0,0,VPE_OP_LEFT,bSorenson); 		
		
#endif		
	
	fsFindClose(&tFileInfo);
    
}

void VPE_TestFile(void)
{
    FindFile("Crop\\",STD_MPEG4, 0);	// MP4 bitstream
//    FindFile("Crop\\",STD_MPEG4, 1);  // Sorenson bitstream  
}    

void VPE_OTF_Test(void)
{
    VPE_TestFile();
}

