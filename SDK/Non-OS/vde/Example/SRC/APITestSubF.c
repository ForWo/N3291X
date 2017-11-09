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

/* Interrupt statuses */
UINT32 volatile BITPicRunOK= FALSE,BITBufferEmptyOK= FALSE;
FileOpInfo ReadFile,WriteFile;
static FRAME_BUF FrameBuf[MAX_FRAME];

FRAME_BUF *GetFrameBuffer(int index)
{
    return &FrameBuf[index];
}

void FrameBufferInit(int picX, int picY,int StartAddr)
{
    int  Ysize;
    int  addrY;
    int  i;
    int  Cbsize;

    Ysize = ((picX+15)& ~15) * ((picY+15) & ~15);
    Cbsize = Ysize/4;       // YUV420
    
    addrY = StartAddr;    
	
    for (i=0; i<MAX_FRAME; i++) {
        FrameBuf[i].Index  = i;

        FrameBuf[i].AddrY = addrY;
        FrameBuf[i].AddrCb = FrameBuf[i].AddrY  + Ysize;
        FrameBuf[i].AddrCr = FrameBuf[i].AddrCb + Cbsize;
        addrY = FrameBuf[i].AddrCr + Cbsize;
        
        FrameBuf[i].StrideY = picX;
        FrameBuf[i].StrideC = picX/2;

    }
    
}