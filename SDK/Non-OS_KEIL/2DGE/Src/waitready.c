/******************************************************************************
 *
 *  Copyright (c) 2014 by Nuvoton Technology Corp. All rights reserved.       
 *  Nuvoton N3291x 2D Graphics Engine Low-Level Library
 *
 *  FILENAME
 *      WAITREADY.C   
 *          
 *  VERSION
 *      1.0
 *                                                                                                                    
 *  DESCRIPTION
 *      This is module is used to wait for the previous engine command
 *      complete..      
 * 
 *  HISTORY
 *      2014/12/01  Created.
 *
 *  REMARK
 *      None
 *                                                             
 ******************************************************************************/
#include "wbio.h"
#include "w55fa95_reg.h"


///////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION
//      gfxWaitEngineReady()
//
//  DESCRIPTION
//      This function is used to wait for the previous command complete in 
//      order to process a new graphics engine command.
//
//  INPUTS
//      None
//
//  OUTPUTS
//      None
//
//  RETURN
//      None
// 
///////////////////////////////////////////////////////////////////////////////
VOID gfxWaitEngineReady(VOID)
{
    if ((inpw(REG_2D_GETRIG) & 0x01) != 0) // engine busy?
    {
        while ((inpw(REG_2D_GEINTS) & 0x01) == 0); // wait for command complete
    }         
           
    outpw(REG_2D_GEINTS, 1); // clear interrupt status
}
