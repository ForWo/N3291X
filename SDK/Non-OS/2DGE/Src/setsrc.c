/******************************************************************************
 *
 *  Copyright (c) 2014 by Nuvoton Technology Corp. All rights reserved.       
 *  Nuvoton N3291x 2D Graphics Engine Low-Level Library
 *
 *  FILENAME
 *      SETSRC.C   
 *          
 *  VERSION
 *      1.0
 *                                                                                                                    
 *  DESCRIPTION
 *      This module is used to change the source surface for BitBlt. 
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
//      gfxSetSrcSurface()
//
//  DESCRIPTION
//      This function is used change the source surface for BitBlt.
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
INT gfxSetSrcSurface(GFX_SURFACE_T *surface)
{
    if (! _gfx_bInitialized) return ERR_GFX_ENV_NOT_INITIALIZED;

    gfxWaitEngineReady();

    _gfx_nSrcWidth      = surface->nWidth;
    _gfx_nSrcHeight     = surface->nHeight;
    _gfx_nSrcPitch      = surface->nPitch;
    _gfx_uSrcStartAddr  = surface->uStartAddr;

    outpw(REG_2D_GEXYSOA, _gfx_uSrcStartAddr);
    
    return 0;   
} 



