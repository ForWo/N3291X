/******************************************************************************
 *
 *  Copyright (c) 2014 by Nuvoton Technology Corp. All rights reserved.       
 *  Nuvoton N3291x 2D Graphics Engine Low-Level Library
 *
 *  FILENAME
 *      CLRSCR.C   
 *          
 *  VERSION
 *      1.0
 *                                                                                                                    
 *  DESCRIPTION
 *      This is module is used to clear graphics buffer.      
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
//      gfxClearScreen()
//
//  DESCRIPTION
//      This function is used to clear the destination graphics buffer
//      with a specified color. 
//
//  INPUTS
//      color:  color will be used to fill the graphics buffer
//
//  OUTPUTS
//      None
//
//  RETURN
//      0:      Success
//      others: Error  
// 
///////////////////////////////////////////////////////////////////////////////
INT gfxClearScreen(GFX_ENTRY color)
{
    UINT32 cmd32, dest_pitch, dest_dimension;
    
    if (! _gfx_bInitialized) return ERR_GFX_ENV_NOT_INITIALIZED;

    gfxWaitEngineReady();
    
    cmd32 = 0xcc430040;
    outpw(REG_2D_GECMD, cmd32);
    outpw(REG_2D_GEBC, (UINT32)color);
    
    dest_pitch = (UINT32)(_gfx_nDestPitch / _gfx_nByte) << 16; // pitch in pixels
    outpw(REG_2D_GESDP, dest_pitch);
    
    outpw(REG_2D_GEDSXYL, 0);   // starts from (0,0)
    
    dest_dimension = (UINT32)_gfx_nDestHeight << 16 | (UINT32)_gfx_nDestWidth;
    outpw(REG_2D_GEDIXYL, dest_dimension);
    
    outpw(REG_2D_GETRIG, 1); 
    
    return 0;
}
