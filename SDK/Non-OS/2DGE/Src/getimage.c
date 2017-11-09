/******************************************************************************
 *
 *  Copyright (c) 2014 by Nuvoton Technology Corp. All rights reserved.       
 *  Nuvoton N3291x 2D Graphics Engine Low-Level Library
 *
 *  FILENAME
 *      GETIMAGE.C   
 *          
 *  VERSION
 *      1.0
 *                                                                                                                    
 *  DESCRIPTION
 *      This module is used to get a bitmap from destination surface 
 *      to source surface. .       
 * 
 *  HISTORY
 *      2014/12/01  Created.
 *
 *  REMARK
 *      None
 *                                                             
 ******************************************************************************/
#include "wbio.h"
#include "w55fa95_gfx.h"
#include "global.h"

#include "w55fa95_reg.h"
#include "wberrcode.h"


///////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION
//      gfxGetImage()
//
//  DESCRIPTION
//      This function is used to get a selected bitmap rectangle
//      from destination surface to a specified address of source surface. 
//
//  INPUTS
//      dest_rect:  destination rectangle
//      src_pnt:    source point (in X/Y addressing)
//      
//  OUTPUTS
//      None
//
//  RETURN
//      0:      success
//      others: error 
//
///////////////////////////////////////////////////////////////////////////////
INT gfxGetImage(GFX_RECT_T dest_rect, GFX_PNT_T src_pnt)
{
    UINT32 sx, sy, dx, dy, width, height;
    UINT32 cmd32, src_pitch, dest_pitch, pitch, dest_start, src_start, dimension;
    UINT32 data32, alpha;
    
    if (! _gfx_bInitialized) return ERR_GFX_ENV_NOT_INITIALIZED;

    if (! gfxIsValidRect(dest_rect)) return ERR_GFX_INVALID_RECT;
    
    gfxWaitEngineReady();

    sx = (UINT32)dest_rect.fC.nLeft;
    sy = (UINT32)dest_rect.fC.nTop;
    width = (UINT32)(dest_rect.fC.nRight - dest_rect.fC.nLeft +1);
    height = (UINT32)(dest_rect.fC.nBottom - dest_rect.fC.nTop +1);
    
    dx = (UINT32)src_pnt.nX;
    dy = (UINT32)src_pnt.nY;

    cmd32 = 0xcc430000;
    outpw(REG_2D_GECMD, cmd32);

    src_pitch = (UINT32)(_gfx_nSrcPitch / _gfx_nByte) << 16; // pitch in pixels
    dest_pitch = (UINT32)(_gfx_nDestPitch / _gfx_nByte) << 16; // pitch in pixels

    pitch = dest_pitch << 16 | src_pitch; // pitch in pixel
    outpw(REG_2D_GESDP, pitch);

    outpw(REG_2D_GEXYSOA, _gfx_uSrcStartAddr);
  
    src_start = sy << 16 | sx;
    outpw(REG_2D_GESSXYL, src_start);
  
    dest_start = dy << 16 | dx;
    outpw(REG_2D_GEDSXYL, dest_start);
  
    dimension = height << 16 | width;
    outpw(REG_2D_GEDIXYL, dimension);  
  
    if (_gfx_bClipEnabled)
    {
        cmd32 |= 0x00000200;
        outpw(REG_2D_GETCM, cmd32);
        outpw(REG_2D_GECBTL, _gfx_uClipTL);
        outpw(REG_2D_GECBBR, _gfx_uClipBR);
    }  
    
    if (_gfx_bColorKeyEnabled)
    {
        cmd32 |= 0x00008000; // color transparency 
        outpw(REG_2D_GETCM, cmd32);
        outpw(REG_2D_GETC, _gfx_uColorKey);
        outpw(REG_2D_GETCM, _gfx_uColorKeyMask);
    }
    
    if (_gfx_bAlphaEnabled)
    {
        cmd32 |= 0x00200000;
        outpw(REG_2D_GETCM, cmd32);
    
        data32 = inpw(REG_2D_GEMC) & 0x0000ffff;
        alpha = _gfx_uAlphaKs << 8 | (256 - _gfx_uAlphaKs);
        data32 |= (alpha << 16);
    
        outpw(REG_2D_GEMC, data32);
    }
  
    outpw(REG_2D_GETRIG, 1); 

    return 0;
}


