/******************************************************************************
 *
 *  Copyright (c) 2014 by Nuvoton Technology Corp. All rights reserved.       
 *  Nuvoton N3291x 2D Graphics Engine Low-Level Library
 *
 *  FILENAME
 *      SETDMODE.C   
 *          
 *  VERSION
 *      1.0
 *                                                                                                                    
 *  DESCRIPTION
 *      This module is used to set the drawing mode.
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


///////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION
//      gfxSetDrawMode()
//
//  DESCRIPTION
//      This function is used set the drawing mode. 
//
//  INPUTS
//      draw_mode:      drawing mode
//
//  OUTPUTS
//      None
//
//  RETURN
//      0:      success
//      others: error
// 
///////////////////////////////////////////////////////////////////////////////
INT gfxSetDrawMode(GFX_DRAW_MODE_E draw_mode)
{
    _gfx_nDrawMode = draw_mode;
    
    return 0;   
} 



