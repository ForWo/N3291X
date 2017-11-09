/******************************************************************************
 *
 *  Copyright (c) 2014 by Nuvoton Technology Corp. All rights reserved.       
 *  Nuvoton N3291x 2D Graphics Engine Low-Level Library
 *
 *  FILENAME
 *      LINE.C   
 *          
 *  VERSION
 *      1.0
 *                                                                                                                    
 *  DESCRIPTION
 *      This is module is used to draw lines on to screen.      
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


///////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION
//      gfxDrawLine()
//
//  DESCRIPTION
//      This function is used to draw lines onto the destination 
//      surface.
//
//      The applicaiton must call gfxSetPen() to specify the line before
//      this function is called. 
//
//  INPUTS
//      p1:     start point in X/Y addressing
//      p2:     end point in X/Y addressing
//
//  OUTPUTS
//      None
//
//  RETURN
//      0:      success
//      others: error  
// 
///////////////////////////////////////////////////////////////////////////////
INT gfxDrawLine(GFX_PNT_T p1, GFX_PNT_T p2)
{
    INT abs_X, abs_Y, min, max;
    INT x1, y1, x2, y2;
    UINT32 step_constant, initial_error, direction_code;
    UINT32 cmd32, dest_pitch, dest_start;
    UINT32 temp32, line_control_code;
    
    if (! _gfx_bInitialized) return ERR_GFX_ENV_NOT_INITIALIZED;
    
    if (! _gfx_bPenInitialized) return ERR_GFX_PEN_NOT_INITIALIZED;

    gfxWaitEngineReady();
  
    x1 = p1.nX;
    y1 = p1.nY;
    x2 = p2.nX;
    y2 = p2.nY;
    
    abs_X = ABS(x2-x1);
    abs_Y = ABS(y2-y1);
    
    if (abs_X > abs_Y) // X major
    {	
        max = abs_X;
        min = abs_Y;
    
        step_constant = (((UINT32)(2*(min-max))) << 16) | (UINT32)(2*min);
        initial_error = (((UINT32)(2*min-max)) << 16) | (UINT32)(max);
        
        if (x2 > x1) // +X direction
        {
            if (y2 > y1) // +Y direction
                direction_code = XpYpXl;
            else // -Y direction   	
                direction_code = XpYmXl;
        }
        else // -X direction
        {
            if (y2 > y1) // +Y direction
                direction_code = XmYpXl;
            else // -Y direction   	
                direction_code = XmYmXl;    	
        }
    }		
    else // Y major
    {
        max = abs_Y;
        min = abs_X;
    
        step_constant = (((UINT32)(2*(min-max))) << 16) | (UINT32)(2*min);
        initial_error = (((UINT32)(2*min-max)) << 16) | (UINT32)(max);
  
        if (x2 > x1) // +X direction
        {
            if (y2 > y1) // +Y direction
                direction_code = XpYpYl;
            else // -Y direction   	
                direction_code = XpYmYl;
        }
        else // -X direction
        {
            if (y2 > y1) // +Y direction
                direction_code = XmYpYl;
            else // -Y direction   	
                direction_code = XmYmYl;    	
        }
    }
  
    outpw(REG_2D_GEBER, step_constant);
    outpw(REG_2D_GEBIR, initial_error);
  
    cmd32 = 0x009b0000 | direction_code; // styled line
  
    if (_gfx_nDrawMode == GFX_SRC_TRANSPARENT)
    {
        cmd32 |= 0x00004000; 
    }
    outpw(REG_2D_GECMD, cmd32);

    outpw(REG_2D_GEBC, _gfx_uPenForeColor); 
    outpw(REG_2D_GEFC, _gfx_uPenBackColor);

    dest_pitch = (UINT32)(_gfx_nDestPitch / _gfx_nByte) << 16; // pitch in pixels
    outpw(REG_2D_GESDP, dest_pitch);

    dest_start = y1 << 16 | x1;
    outpw(REG_2D_GEDSXYL, dest_start);
  
    if (_gfx_bClipEnabled)
    {
        cmd32 |= 0x00000200;
        outpw(REG_2D_GECMD, cmd32);
        outpw(REG_2D_GECBTL, _gfx_uClipTL);
        outpw(REG_2D_GECBBR, _gfx_uClipBR);
    }  
  
    line_control_code = (UINT32)_gfx_usPenStyle; 
    temp32 = inpw(REG_2D_GEMC) & 0x0000ffff;
    temp32 = (line_control_code << 16) | temp32;
  
    outpw(REG_2D_GEMC, temp32); 
    
    outpw(REG_2D_GETRIG, 1); 
    
    return 0;
}
