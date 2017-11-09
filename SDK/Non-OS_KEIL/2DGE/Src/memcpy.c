/******************************************************************************
 *
 *  Copyright (c) 2014 by Nuvoton Technology Corp. All rights reserved.       
 *  Nuvoton N3291x 2D Graphics Engine Low-Level Library
 *
 *  FILENAME
 *      MEMCPY.C   
 *          
 *  VERSION
 *      1.0
 *                                                                                                                    
 *  DESCRIPTION
 *      This is module is used to copy memory using graphics engine.
 * 
 *  HISTORY
 *      2014/12/01  Created.
 *
 *  REMARK
 *      Because linear addressing mode not good for use, the firmware 
 *      uses the X/Y addressing mode to do the memory copy.
 *                                                             
 ******************************************************************************/
#include <stdio.h>
#include <string.h>

#include "w55fa95_gfx.h"
#include "global.h"

#include "wbio.h"
#include "w55fa95_reg.h"


///////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION
//      gfxMemcpy()
//
//  DESCRIPTION
//      This function is used to copy memory using graphics engine. 
//      This function can be used by application as a memory to memory DMA.
//
//  INPUTS
//      dest:   pointer to destination memory
//      src:    pointer to source memory
//      count:  length to be copied in bytes
//
//  OUTPUTS
//      None
//
//  RETURN
//      0:      success
//      others: error  
// 
///////////////////////////////////////////////////////////////////////////////
INT gfxMemcpy(PVOID dest, PVOID src, INT count)
{
    UINT32 temp_misc, temp_mask;
    UINT32 new_dest, new_src;
    INT M, N;
    
    gfxWaitEngineReady();

    // backup BPP information
    temp_misc = inpw(REG_2D_GEMC);
    
    // must use 8-BPP  
    outpw(REG_2D_GEMC, temp_misc & 0xffffffcf); // filter always uses 8-BPP

    // backup write mask
    temp_mask = inpw(REG_2D_GEWPM);
    
    outpw(REG_2D_GEWPM, 0x0000ff);
  
#if 1 // hardware linear addressing mode has problem in GAD001   
  
    /*
    ** This function uses linear addressing mode. The REG_GE_DEST_START00_ADDR
    ** and REG_GE_SRC_START00_ADDR are not used. The starting address is 
    ** always mapped to 0 of physical memory.
    */
    #if 0 // PP
    
    outpw(REG_2D_GECMD, 0xcc420000); // linear addressing
    
    outpw(REG_2D_GEDSXYL, (UINT32)dest);
    
    outpw(REG_2D_GESSXYL, (UINT32)src);
    
    outpw(REG_2D_GEDIXYL, (UINT32)count);
       
    outpw(REG_2D_GETRIG, 1); 
    
    while ((inpw(REG_2D_GEINTS) & 0x01) == 0); // wait for command complete

    outpw(REG_2D_GEINTS, 1); // clear interrupt status   
    
    #else // NN
    
    outpw(REG_2D_GECMD, 0xcc420004); // linear addressing
    
    outpw(REG_2D_GEDSXYL, (UINT32)dest+count-1);
    
    outpw(REG_2D_GESSXYL, (UINT32)src+count-1);
    
    outpw(REG_2D_GEDIXYL, (UINT32)count);
       
    outpw(REG_2D_GETRIG, 1); 
    
    while ((inpw(REG_2D_GEINTS) & 0x01) == 0); // wait for command complete

    outpw(REG_2D_GEINTS, 1); // clear interrupt status      
    
    #endif    
    
#else // use X/Y addressing mode

    outpw(REG_GE_CONTROL,0xcc430000);

    outpw(REG_GE_DEST_START_ADDR, 0); 
    outpw(REG_GE_SRC_START_ADDR, 0);
    outpw(REG_GE_PITCH, 0x04000400); // 1024 bytes
    
    new_src = (UINT32)src;
    new_dest = (UINT32)dest;
    
    while (count > 0x100000) // 1024x1024
    {
        outpw(REG_GE_SRC_START00_ADDR, new_src);
        outpw(REG_GE_DEST_START00_ADDR, new_dest);
        outpw(REG_GE_DIMENSION, 0x04000400); // 1024x1024
        
        outpw(REG_GE_TRIGGER, 1); 

        count -= 0x100000;
        new_src += 0x100000;
        new_dest += 0x100000;
            
        while ((inpw(REG_GE_INTS) & 0x01) == 0); // wait for command complete

        outpw(REG_GE_INTS, 1); // clear interrupt status       
    }
    
    //
    // count = 1024 * M + N
    //
    M = count / 1024;
    N = count % 1024;
    
    outpw(REG_GE_SRC_START00_ADDR, new_src);
    outpw(REG_GE_DEST_START00_ADDR, new_dest);
    outpw(REG_GE_DIMENSION, ((UINT32)M << 16) | 0x00000400); // 1024xM
        
    outpw(REG_GE_TRIGGER, 1); 
    
    while ((inpw(REG_GE_INTS) & 0x01) == 0); // wait for command complete

    count -= (M * 1024);
    new_src += (M * 1024);
    new_dest += (M * 1024);

    outpw(REG_GE_INTS, 1); // clear interrupt status       
    
    if (N != 0) // avoid harware BitBLT bug for some conditions
        memcpy((PVOID)new_dest, (PVOID)new_src, N);
        
#endif

    outpw(REG_2D_GEMC, temp_misc); // restore BPP
    outpw(REG_2D_GEWPM, temp_mask); // restore mask

    return 0;
}


