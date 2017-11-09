/******************************************************************************
 *
 *  Copyright (c) 2014 by Nuvoton Technology Corp. All rights reserved.       
 *  Nuvoton N3291x 2D Graphics Engine Low-Level Library
 *
 *  FILENAME
 *      CHECKROP.C   
 *          
 *  VERSION
 *      1.0
 *                                                                                                                    
 *  DESCRIPTION
 *      This is module is used to check the ROP.
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
//      gfxIsPInROP()
//      
//  DESCRIPTION
//      This function is used to check if the input ROP has P or not.
//      Rule for checking ROP: b0=b4, b1=b5, b2=b6, b3=b7 => No P in a ROP         
//  INPUTS
//      rop:    rop code
//
//  OUTPUTS
//      None
//
//  RETURN
//      TRUE:   ROP contains P
//      FALSE:  ROP not contains P
// 
///////////////////////////////////////////////////////////////////////////////
BOOL gfxIsPInROP(UINT8 rop)
{
    if ((rop & 0xf0)==(rop << 4)) return FALSE;
     
    return TRUE;
}
