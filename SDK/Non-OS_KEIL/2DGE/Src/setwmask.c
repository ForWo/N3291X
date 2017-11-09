/******************************************************************************
 *
 *  Copyright (c) 2014 by Nuvoton Technology Corp. All rights reserved.       
 *  Nuvoton N3291x 2D Graphics Engine Low-Level Library
 *
 *  FILENAME
 *      SETWMASK.C   
 *          
 *  VERSION
 *      1.0
 *                                                                                                                    
 *  DESCRIPTION
 *      This module is used to set write mask control of drawing functions.
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


///////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION
//      gfxSetWriteMask()
//
//  DESCRIPTION
//      This function is used set the write mask control of drawing functions.
//
//  INPUTS
//      color_mask:     output color mask
//
//  OUTPUTS
//      None
//
//  RETURN
//      0:      success
//      others: error
// 
///////////////////////////////////////////////////////////////////////////////
INT gfxSetWriteMask(GFX_MASK color_mask)
{
    gfxWaitEngineReady();

    _gfx_uWriteMask = (UINT32)color_mask;

    outpw(REG_2D_GEWPM, _gfx_uWriteMask);
        
    return 0;   
} 



