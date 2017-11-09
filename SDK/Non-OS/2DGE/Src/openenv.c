/******************************************************************************
 *
 *  Copyright (c) 2014 by Nuvoton Technology Corp. All rights reserved.       
 *  Nuvoton N3291x 2D Graphics Engine Low-Level Library
 *
 *  FILENAME
 *      OPENENV.C   
 *          
 *  VERSION
 *      1.0
 *                                                                                                                    
 *  DESCRIPTION
 *      This module is used to open the drawing environment.      
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


#define GE_MISC_BPP_8   0x00000000
#define GE_MISC_BPP_16  0x00000010
#define GE_MISC_BPP_32  0x00000020


///////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION
//      gfxCheckColorFormat()
//
//  DESCRIPTION
//      This function is used to check if the color format is valid
//      or not.
//
//  INPUTS
//      color_format:   color format of the drawing environment
//      
//  OUTPUTS
//      None
//
//  RETURN
//      0:      the color format is invalid
//      others: the BPP of the drawing environment
//
///////////////////////////////////////////////////////////////////////////////
static INT gfxCheckColorFormat(GFX_COLOR_FORMAT_E color_format)
{
    if (color_format == GFX_BPP_332) 
        return 8;
    
    if ((color_format == GFX_BPP_444H) || (color_format == GFX_BPP_444L))
        return 16;
    
    if (color_format == GFX_BPP_565) return 16;
    
    if ((color_format == GFX_BPP_666) || (color_format == GFX_BPP_888))
        return 32;
    
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION
//      gfxResetGE()
//
//  DESCRIPTION
//      This function is used to reset graphics engine hardware.
//
//  INPUTS
//      None
//      
//  OUTPUTS
//      None
//
//  RETURN
//      None
//
///////////////////////////////////////////////////////////////////////////////
static VOID gfxResetGE()
{
    outpw(REG_2D_GEMC, 0x00000080); // reset 2D engine 
    
    // check if the FIFO is empty and engine is ready 

}


///////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION
//      gfxInitGE()
//
//  DESCRIPTION
//      This function is used to initialize the graphics engine hardware
//      according to the specified drawing environment parameters.
//
//  INPUTS
//      None
//      
//  OUTPUTS
//      None
//
//  RETURN
//      None
//
///////////////////////////////////////////////////////////////////////////////
static VOID gfxInitGE()
{
    gfxResetGE();

#if 0    
    outpw(REG_2D_GECMD, 0);   // disable interrupt
#else
    outpw(REG_2D_GECMD, 0x00020000);    // enable interrupt 
#endif     
    outpw(REG_2D_GEINTS, 0);      // clear interrupt
    
    outpw(REG_2D_GEPLS, _gfx_uColorPatternStartAddr);
    outpw(REG_2D_GEXYDOA, _gfx_uDestStartAddr);
    outpw(REG_2D_GEXYSOA, _gfx_uSrcStartAddr);

    outpw(REG_2D_GEWPM, 0x00ffffff);
    
    switch (_gfx_nBpp)
    {
        case 8:
            outpw(REG_2D_GEMC, GE_MISC_BPP_8);
            break;
        case 16:
            outpw(REG_2D_GEMC, GE_MISC_BPP_16);
            break;
        case 32:
            outpw(REG_2D_GEMC, GE_MISC_BPP_32);
            break;          
    }

#if 0    
    gfxClearScreen(0);  // clear destination graphics buffer to black
#endif
}


///////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION
//      gfxOpenEnv()
//
//  DESCRIPTION
//      This function is used to open a drawing environment. The drawing 
//      environment is specified a GFX_INFO_T structure. This function 
//      surface with a specified address and raster operation.
//
//  INPUTS
//      in_param:   pointer to a drawing environment structure 
//
//  OUTPUTS
//      None
//
//  RETURN
//      0:      success
//      othesr: error
// 
///////////////////////////////////////////////////////////////////////////////
INT gfxOpenEnv(GFX_INFO_T *in_param)
{
    if (_gfx_bInitialized) // check if the drawing environment is open
        return ERR_GFX_ENV_OPENED;
    
    _gfx_nBpp = gfxCheckColorFormat(in_param->nColorFormat);

#if (DBG_LEVEL > 0)    
    gfx_debug("_gfx_nBpp = %d\n", _gfx_nBpp);
#endif
    
    if (_gfx_nBpp == 0) // color format valid?
        return ERR_GFX_INVALID_COLOR_FORMAT;
    
    _gfx_nColorFormat = in_param->nColorFormat;
    
    _gfx_nByte = _gfx_nBpp >> 3;   
    
    _gfx_nDestWidth     = in_param->nDestWidth;
    _gfx_nDestHeight    = in_param->nDestHeight;
    _gfx_nSrcWidth      = in_param->nSrcWidth;
    _gfx_nSrcHeight     = in_param->nSrcHeight;

#if (DBG_LEVEL > 0)    
    gfx_debug("_gfx_nDestWidth = %d\n", _gfx_nDestWidth);
    gfx_debug("_gfx_nDestHeight = %d\n", _gfx_nDestHeight);
    gfx_debug("_gfx_nSrcWidth = %d\n", _gfx_nSrcWidth);
    gfx_debug("_gfx_nSrcHeight = %d\n", _gfx_nSrcHeight);
#endif
    
    _gfx_nDestPitch     = in_param->nDestPitch;
    _gfx_nSrcPitch      = in_param->nSrcPitch;
    
    _gfx_nScreenSize    = _gfx_nDestHeight * _gfx_nDestPitch;
    
    _gfx_uDestStartAddr         = in_param->uDestStartAddr;
    _gfx_uColorPatternStartAddr = in_param->uColorPatternStartAddr;
    _gfx_uSrcStartAddr          = in_param->uSrcStartAddr;
    
    gfxInitGE();
    
    _gfx_bInitialized = TRUE;
    
    return 0; 
}

