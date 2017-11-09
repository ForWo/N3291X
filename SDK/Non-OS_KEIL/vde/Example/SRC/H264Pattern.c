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


void H264DecodOneFile(char *H264FileName)
{
   
	DecOpenParam		decOP		= { 0 };
	FileOpInfo			ReadFileInfo     = { 0 };
	DecConfigParam		decConfig = { 0 };		
	char BitStreamFileName[256];//,DecodedFileName[128];
	
    decOP.bitstreamFormat = STD_AVC;
    decOP.bitstreamBuffer = ADDR_BIT_STREAM; 
   	decOP.bitstreamBufferSize = STREAM_BUF_SIZE;    
    decOP.mp4DeblkEnable = 0;
    decOP.reorderEnable = 1;
    decConfig.prescanEnable= 0;  
     
    	
    strcpy(BitStreamFileName,H264FileName);
    Console_Printf("Testing %s file..\n",BitStreamFileName); 
    FileOP(&ReadFileInfo,READ_BITSTREAM,BitStreamFileName);

    DecoderAPI(&decOP,&ReadFileInfo,&decConfig,DECODE_TO_VPOST);
	     
}


