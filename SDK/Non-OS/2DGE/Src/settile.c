/******************************************************************************
 *
 *  Copyright (c) 2014 by Nuvoton Technology Corp. All rights reserved.       
 *  Nuvoton N3291x 2D Graphics Engine Low-Level Library
 *
 *  FILENAME
 *      SETROP.C   
 *          
 *  VERSION
 *      1.0
 *                                                                                                                    
 *  DESCRIPTION
 *      This module is used to set tile control of BitBlt.
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
//      gfxSetTile()
//
//  DESCRIPTION
//      This function is used set the tile control of BitBlt.
//
//  INPUTS
//      tile_x:     tile counter in X direction
//      tile_y:     tile counter in Y direction
//
//  OUTPUTS
//      None
//
//  RETURN
//      0:      success
//      others: error
// 
///////////////////////////////////////////////////////////////////////////////
INT gfxSetTile(INT tile_x, INT tile_y)
{
    _gfx_uTileX = (UINT32)tile_x;
    _gfx_uTileY = (UINT32)tile_y;
    
    return 0;   
} 



