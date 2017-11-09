/******************************************************************************
 *
 *  Copyright (c) 2014 by Nuvoton Technology Corp. All rights reserved.       
 *  Nuvoton N3291x 2D Graphics Engine Low-Level Library
 *
 *  FILENAME
 *      EMU.C   
 *          
 *  VERSION
 *      1.0
 *                                                                                                                    
 *  DESCRIPTION
 *      This module is created for W55FA95 data flow emulation.
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

#include "wblib.h"
#include "w55fa95_gfx.h"

#include "wbio.h"
#include "w55fa95_reg.h"


#define EMU_WIDTH       320
#define EMU_HEIGHT      240
#define EMU_PITCH       (EMU_WIDTH*4)
#define EMU_DIM         (EMU_WIDTH*EMU_HEIGHT)
#define EMU_BUF_SIZE    (EMU_PITCH*EMU_HEIGHT)

static BOOL             _emu_gfx_bInitialized = FALSE;
static GFX_INFO_T       _emu_gfx_env_info;
static GFX_RECT_T       _emu_gfx_src_rect;
static GFX_PNT_T        _emu_gfx_dest_pnt;
static UINT32           _emu_gfx_pat32 = 0x00ff55aa;
static UINT32           _emu_gfx_cache_ctl;


///////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION
//      gfxEmuInit()
//
//  DESCRIPTION
//      This function is used to do the initialization. 
//
//  INPUTS
//      uBaseAddr:  base address allocated to 2D for data flow emulation
//      uCacheCtl:  for cache ON mode memory access
//
//  OUTPUTS
//      None
//
//  RETURN
//      buffer address
//      others: error  
// 
///////////////////////////////////////////////////////////////////////////////
UINT32 gfxEmuInit(UINT32 uBaseAddr, UINT32 uCacheCtl)
{
    INT i;
    UINT32 *ptr32;
    UINT32 aligned_base_addr; 
    
    aligned_base_addr = ((uBaseAddr+31) >> 5) << 5; 

    _emu_gfx_env_info.nDestWidth             = EMU_WIDTH;
    _emu_gfx_env_info.nDestHeight            = EMU_HEIGHT;
    _emu_gfx_env_info.nDestPitch             = EMU_PITCH;
    _emu_gfx_env_info.nSrcWidth              = EMU_WIDTH;
    _emu_gfx_env_info.nSrcHeight             = EMU_HEIGHT;
    _emu_gfx_env_info.nSrcPitch              = EMU_PITCH;
    _emu_gfx_env_info.nColorFormat           = GFX_BPP_888;
    _emu_gfx_env_info.uDestStartAddr         = aligned_base_addr;
    _emu_gfx_env_info.uColorPatternStartAddr = 0;
    _emu_gfx_env_info.uSrcStartAddr          = aligned_base_addr;    

    if (gfxOpenEnv(&_emu_gfx_env_info) != Successful) return 0;
    
    /*--- fill the source pattern to the buffer */

    _emu_gfx_cache_ctl = uCacheCtl;
        
    ptr32 = (UINT32 *)(_emu_gfx_env_info.uDestStartAddr | _emu_gfx_cache_ctl);
    for (i=0; i<EMU_DIM; i++) *ptr32++ = _emu_gfx_pat32;
        
    gfxSetROP(DSTINVERT); // use Dn at the test key

    _emu_gfx_src_rect.fC.nLeft = 0;
    _emu_gfx_src_rect.fC.nTop = 0;
    _emu_gfx_src_rect.fC.nRight = EMU_WIDTH - 1;
    _emu_gfx_src_rect.fC.nBottom = EMU_HEIGHT - 1;
    
    _emu_gfx_dest_pnt.nX = 0;
    _emu_gfx_dest_pnt.nY = 0;
        
    _emu_gfx_bInitialized = TRUE;
        
    return (EMU_BUF_SIZE + 32);
}


INT gfxEmuReset()
{
    INT i;
    UINT32 *ptr32;

    _emu_gfx_pat32 = 0x00ff55aa;

    ptr32 = (UINT32 *)(_emu_gfx_env_info.uDestStartAddr | _emu_gfx_cache_ctl);
    for (i=0; i<EMU_DIM; i++) *ptr32++ = _emu_gfx_pat32;

    return Successful;
}


INT gfxEmuTrigger()
{
    if (! _emu_gfx_bInitialized) return Fail;
    
    gfxScreenToScreenBlt(_emu_gfx_src_rect, _emu_gfx_dest_pnt);
     
    _emu_gfx_pat32 = (~_emu_gfx_pat32) & 0x00ffffff; // expected result (only check LSB 24 bits)
       
    return Successful;
}


BOOL gfxEmuBusy()
{
    if ((inpw(REG_2D_GETRIG) & 0x01) != 0) return TRUE;

    return FALSE;
}


INT gfxEmuCompare()
{
    UINT32 *ptr32;
    INT cnt;
    
    cnt = 0;
    ptr32 = (UINT32 *)(_emu_gfx_env_info.uDestStartAddr | _emu_gfx_cache_ctl);
    while (cnt < EMU_WIDTH*EMU_HEIGHT)
    {
        if (*ptr32 != _emu_gfx_pat32) 
        {
            printf("[2D ERR at 0x%x]: 0x%x => 0x%x\n", (UINT32)ptr32, _emu_gfx_pat32, *ptr32);
            
            return Fail;
        }   
        ptr32++; 
        cnt++;
    }
    
    return Successful;
}