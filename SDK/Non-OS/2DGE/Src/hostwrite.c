/******************************************************************************
 *
 *  Copyright (c) 2014 by Nuvoton Technology Corp. All rights reserved.       
 *  Nuvoton N3291x 2D Graphics Engine Low-Level Library
 *
 *  FILENAME
 *      HOSTWRITE.C   
 *          
 *  VERSION
 *      1.0
 *                                                                                                                    
 *  DESCRIPTION
 *      This module is used to handle the Host Write BLT operation.       
 * 
 *  HISTORY
 *      2014/12/01  Created.
 *
 *  REMARK
 *      None
 *                                                             
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wbio.h"
#include "wblib.h"
#include "w55fa95_gfx.h"
#include "global.h"

#include "w55fa95_reg.h"
#include "wberrcode.h"


///////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION
//      gfxHostWriteBlt()
//
//  DESCRIPTION
//      This function is used to do the Host Write BLT.
//
//  INPUTS
//      dest_rect:  dest rectangle to be filled with the source image
//      src_buf:    pointer to source image buffer
//      
//  OUTPUTS
//      None
//
//  RETURN
//      0:      success
//      others: error 
//
///////////////////////////////////////////////////////////////////////////////
INT gfxHostWriteBlt(GFX_RECT_T dest_rect, PVOID src_buf)
{
    INT i, j; 
    UINT32 destx, desty, width, height;
    UINT32 cmd32, dest_pitch, pitch, dest_start, dimension;
    UINT32 data32, alpha;
    PUINT32 ptr32; 
    
   
    if (! _gfx_bInitialized) return ERR_GFX_ENV_NOT_INITIALIZED;
    
    if (! gfxIsValidRect(dest_rect)) return ERR_GFX_INVALID_RECT;

    gfxWaitEngineReady();
    
    destx = (UINT32)dest_rect.fC.nLeft; 
    desty = (UINT32)dest_rect.fC.nTop;
    width = (UINT32)(dest_rect.fC.nRight - dest_rect.fC.nLeft + 1);
    height = (UINT32)(dest_rect.fC.nBottom - dest_rect.fC.nTop + 1);
    
    cmd32 = 0xcc430020; // only 0xCC is allowed 
    
    outpw(REG_2D_GECMD, cmd32); 
  
    dest_pitch = (UINT32)(_gfx_nDestPitch / _gfx_nByte); // pitch in pixels
    pitch = dest_pitch << 16 | dest_pitch; 
    outpw(REG_2D_GESDP, pitch);

    dest_start = desty << 16 | destx;
    outpw(REG_2D_GEDSXYL, dest_start);
  
    dimension = height << 16 | width;
    outpw(REG_2D_GEDIXYL, dimension);  
  
    //
    // force to use the same starting address
    //
    outpw(REG_2D_GEXYSOA, _gfx_uDestStartAddr);
  
    if (_gfx_bClipEnabled) // can be supported? 
    {
        cmd32 |= 0x00000200;
        outpw(REG_2D_GECMD, cmd32);
        outpw(REG_2D_GECBTL, _gfx_uClipTL);
        outpw(REG_2D_GECBBR, _gfx_uClipBR);
    }  

    /* 
    ** Hardware can support destination transparency as well.
    ** Current implementation only supports source transparency.
    */
    if (_gfx_nDrawMode == GFX_SRC_TRANSPARENT) // can be supported? 
    {
        if (! _gfx_bColorKeyEnabled)
            return ERR_GFX_COLOR_KEY_NOT_INITIALIZED;
            
        cmd32 |= 0x00008000; // source color transparency 
        outpw(REG_2D_GECMD, cmd32);
        outpw(REG_2D_GETC, _gfx_uColorKey);
        outpw(REG_2D_GETCM, _gfx_uColorKeyMask);
    }  
  
    if (_gfx_bAlphaEnabled)
    {
        cmd32 |= 0x00200000;
        outpw(REG_2D_GECMD, cmd32);
    
        data32 = inpw(REG_2D_GEMC) & 0x0000ffff;
        alpha = (_gfx_uAlphaKs << 8) | (256 - _gfx_uAlphaKs);
        data32 |= (alpha << 16);
    
        outpw(REG_2D_GEMC, data32);
    }
  
    outpw(REG_2D_GETRIG, 1); 
    
    ptr32 = (PUINT32)src_buf; 
    
    for (i=0; i<height; i++)
    {
        for (j=0; j<((width*_gfx_nByte)>>2); j++)
            outpw(REG_2D_GEHBDW0, *ptr32++); 
    }

    return 0;
}


