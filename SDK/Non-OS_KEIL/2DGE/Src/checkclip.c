/******************************************************************************
 *
 *  Copyright (c) 2014 by Nuvoton Technology Corp. All rights reserved.       
 *  Nuvoton N3291x 2D Graphics Engine Low-Level Library
 *
 *  FILENAME
 *      CHECKRECT.C   
 *          
 *  VERSION
 *      1.0
 *                                                                                                                    
 *  DESCRIPTION
 *      This is module is used to check if the output drawing rectangle
 *      is valid or not.       
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
//      gfxIsValidRect()
//
//  DESCRIPTION
//      This function is used to check the drawing rectangle is valid or not.
//
//  INPUTS
//      rect:   drawing rectangle
//
//  OUTPUTS
//
//  RETURN
//      TRUE:   valid
//      FALSE:  invalid
// 
///////////////////////////////////////////////////////////////////////////////
BOOL gfxIsValidRect(GFX_RECT_T rect)
{
    INT x1, x2, y1, y2;

    x1 = rect.fC.nLeft;
    y1 = rect.fC.nTop;
    x2 = rect.fC.nRight;
    y2 = rect.fC.nBottom;
    
    if ((x1 < 0) || (y1 < 0) || (x2 < 0) || (y2 < 0)) return FALSE;;
    
    if ((x1 > x2) || (y1 > y2)) return FALSE;
    
    if (x2 >= _gfx_nDestWidth) return FALSE;
    
    if (y2 >= _gfx_nDestHeight) return FALSE;     
    
    return TRUE;
}
