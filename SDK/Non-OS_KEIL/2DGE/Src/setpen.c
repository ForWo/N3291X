/******************************************************************************
 *
 *  Copyright (c) 2014 by Nuvoton Technology Corp. All rights reserved.       
 *  Nuvoton N3291x 2D Graphics Engine Low-Level Library
 *
 *  FILENAME
 *      SETPEN.C   
 *          
 *  VERSION
 *      1.0
 *                                                                                                                    
 *  DESCRIPTION
 *      This module is used to set the pen (line) style for drawing lines.
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

#include "w55fa95_reg.h"


///////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION
//      gfxSetPen()
//
//  DESCRIPTION
//      This function is used set the pen (line) style for drawing lines.
//
//  INPUTS
//      style:      pen style represented by a 16-bit data
//      fore_color: foreground color of the pen
//      back_color: background color of the pen
//
//  OUTPUTS
//      None
//
//  RETURN
//      0:      success
//      others: error
// 
///////////////////////////////////////////////////////////////////////////////
INT gfxSetPen(UINT16 style, GFX_ENTRY fore_color, GFX_ENTRY back_color)
{
    _gfx_usPenStyle = style;
    _gfx_uPenForeColor = fore_color;
    _gfx_uPenBackColor = back_color;
    
    _gfx_bPenInitialized = TRUE;
    
    return 0;   
} 



