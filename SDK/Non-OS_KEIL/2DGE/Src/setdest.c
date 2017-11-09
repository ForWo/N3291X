/******************************************************************************
 *
 *  Copyright (c) 2014 by Nuvoton Technology Corp. All rights reserved.       
 *  Nuvoton N3291x 2D Graphics Engine Low-Level Library
 *
 *  FILENAME
 *      SETDEST.C   
 *          
 *  VERSION
 *      1.0
 *                                                                                                                    
 *  DESCRIPTION
 *      This module is used to change the destination surface for BitBlt. 
 * 
 *  HISTORY
 *      2014/12/01  Created.
 *
 *  REMARK
 *      None
 *
 ******************************************************************************/
#include "w55fa95_gfx.h"
#include "global.h"

#include "wbio.h"
#include "w55fa95_reg.h"
#include "wberrcode.h"


///////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION
//      gfxSetDestSurface()
//
//  DESCRIPTION
//      This function is used change the destination surface for BitBlt.
//
//  INPUTS
//      surface:    pointer to a surface structure
//
//  OUTPUTS
//      None
//
//  RETURN
//      0:      success
//      others: error
// 
//  REMARK
//      The bpp can't be changed.
//
///////////////////////////////////////////////////////////////////////////////
INT gfxSetDestSurface(GFX_SURFACE_T *surface)
{
    if (! _gfx_bInitialized) return ERR_GFX_ENV_NOT_INITIALIZED;

    gfxWaitEngineReady();
    
    _gfx_nDestWidth     = surface->nWidth;
    _gfx_nDestHeight    = surface->nHeight;
    _gfx_nDestPitch     = surface->nPitch;
    _gfx_uDestStartAddr = surface->uStartAddr;

    outpw(REG_2D_GEXYDOA, _gfx_uDestStartAddr);
    
    return 0;   
} 



