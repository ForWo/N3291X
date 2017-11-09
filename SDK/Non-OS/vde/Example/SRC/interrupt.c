/****************************************************************************
 *                                                                                    
 * Copyright (c) Nuvoton Technology Corp. All rights reserved.          
 *                                                                                    
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "w55fa95_reg.h"
#include "wblib.h"
#include "wbio.h"
#include "vdodef.h"
#include "vdoapi.h"
#include "VPUAPI.h"
#include "RegDefine.h"
#include "avctest.h"


void CallBackBufferEmpty(void)
{
    BITBufferEmptyOK = TRUE;
}

void CallBackPicRun(void)
{
    BITPicRunOK = TRUE;
}















