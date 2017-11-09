/******************************************************************************
 *
 *  Copyright (c) 2014 by Nuvoton Technology Corp. All rights reserved.       
 *  Nuvoton N3291x 2D Graphics Engine Low-Level Library
 *
 *  FILENAME
 *      FRAME.C   
 *          
 *  VERSION
 *      1.0
 *                                                                                                                    
 *  DESCRIPTION
 *      This is module is used draw a rectangle border.  
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
//      gfxDrawFrame()
//
//  DESCRIPTION
//      This function is used to draw a rectangle border. 
//      Only the solid border is supported in current implementation.
//
//  INPUTS
//      rect:   rectangle to be filled
//      color:  color will be used to fill the rectangle
//
//  OUTPUTS
//      None
//
//  RETURN
//      0:      success
//      others: error  
// 
///////////////////////////////////////////////////////////////////////////////
INT gfxDrawFrame(GFX_PNT_T p1, GFX_PNT_T p2, INT thick, GFX_ENTRY color)
{
    UINT32 cmd32, dest_start, dest_pitch, dest_dimension;
    UINT32 sx, sy, width, height;
    UINT32 x1, x2, y1, y2;
    
    if (! _gfx_bInitialized) return ERR_GFX_ENV_NOT_INITIALIZED;

    if (p1.nX > p2.nX) return ERR_GFX_INVALID_RECT;
    if (p1.nY > p2.nY) return ERR_GFX_INVALID_RECT;
    
    if (thick < 1) return ERR_GFX_INVALID_THICK;
    
    gfxWaitEngineReady();
        
    outpw(REG_2D_GEFC, (UINT32)color);

    dest_pitch = (UINT32)(_gfx_nDestPitch / _gfx_nByte) << 16; // pitch in pixels
    outpw(REG_2D_GESDP, dest_pitch);

    sx = (UINT32)p1.nX;
    sy = (UINT32)p1.nY;
    width = (UINT32)(p2.nX - p1.nX +1);
    height = (UINT32)(p2.nY - p1.nY +1);
    
    dest_start = sy << 16 | sx;
    outpw(REG_2D_GEDSXYL, dest_start);
  
    dest_dimension = height << 16 | width;
    outpw(REG_2D_GEDIXYL, dest_dimension);

    x1 = sx;
    x2 = x1 + width;    // not include last pixel
    y1 = sy;
    y2 = y1 + height;   // not include last pixel
    
    x1 += thick;
    x2 -= thick;
    y1 += thick;
    y2 -= thick;
    
    outpw(REG_2D_GECBTL, (y1 << 16) | x1);
    outpw(REG_2D_GECBBR, (y2 << 16) | x2);
  
    cmd32 = 0xcc430060;     // solid fill
    cmd32 |= 0x00000300;    // outside clip enable
    outpw(REG_2D_GECMD, cmd32);
    
    outpw(REG_2D_GETRIG, 1); 
    
    return 0;
}
