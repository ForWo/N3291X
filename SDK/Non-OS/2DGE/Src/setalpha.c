/******************************************************************************
 *
 *  Copyright (c) 2014 by Nuvoton Technology Corp. All rights reserved.       
 *  Nuvoton N3291x 2D Graphics Engine Low-Level Library
 *
 *  FILENAME
 *      SETALPHA.C   
 *          
 *  VERSION
 *      1.0
 *                                                                                                                    
 *  DESCRIPTION
 *      This module is used to set the alpha control of BitBlt.
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

#include "wberrcode.h"


///////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION
//      gfxSetAlpha()
//
//  DESCRIPTION
//      This function is used set the alpha control of BitBlt. 
//      The alpha only allows in SRCCOPY in current implementation.
//
//  INPUTS
//      enable:     enable/disable alpha control
//      alpha:      alpha [1..255]
//                  0:      keep destination result (nothing to do)
//                  256:    keep source result (same as disable)
//
//  OUTPUTS
//      None
//
//  RETURN
//      0:      success
//      others: error
// 
///////////////////////////////////////////////////////////////////////////////
INT gfxSetAlpha(BOOL enable, INT alpha)
{
    if (! enable)
    {
        _gfx_bAlphaEnabled = FALSE;
        
        return 0;
    }
    
    if ((alpha < 0) || (alpha > 256)) return ERR_GFX_INVALID_ALPHA;
    
    if (alpha == 256) _gfx_bAlphaEnabled = FALSE;
    
    _gfx_uAlphaKs = (UINT32)alpha;
    
    _gfx_bAlphaEnabled = TRUE;
    
    return 0;   
} 



