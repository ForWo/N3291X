/*-----------------------------------------------------------------------------------*/
/* Nuvoton Technology Corporation confidential                                       */
/*                                                                                   */
/* Copyright (c) 2012 by Nuvoton Technology Corporation                              */
/* All rights reserved                                                               */
/*                                                                                   */
/*-----------------------------------------------------------------------------------*/
#ifdef __KLE_DEMO__

#include <stdlib.h>
#include <string.h>

#include "wbio.h"
#include "wblib.h"

#include "w55fa95_reg.h"
#include "w55fa95_sic.h"
//#include "fmi.h"
#include "w55fa95_gnand.h"
#include "GNAND_global.h"

/*-----------------------------------------------------------------------------
 * Define message display level
 *---------------------------------------------------------------------------*/
// show large messages for debug
#define DBG_PRINTF  sysprintf
//#define DBG_PRINTF(...)

/*----------------------------------------------------------------------------*/
#define GNAND_MAGIC_WORD    "SkipBlk"

#define GNAND_PUT16_L(bptr,n,val)   bptr[n] = val & 0xFF;               \
                                    bptr[n+1] = (val >> 8) & 0xFF;
#define GNAND_PUT32_L(bptr,n,val)   bptr[n] = val & 0xFF;               \
                                    bptr[n+1] = (val >> 8) & 0xFF;      \
                                    bptr[n+2] = (val >> 16) & 0xFF;     \
                                    bptr[n+3] = (val >> 24) & 0xFF;

__align (32) UINT8 g_ram0[8192];

/*-----------------------------------------------------------------------------
 * If NAND pre-program by FlashPAK III with Skip Block Method algorithm,
 * nand_post_process() will skip bad block and rebuild GNAND,
 * and then, system can boot up normally.
 *---------------------------------------------------------------------------*/
int nand_post_process()
{
    UINT8   *ptr_g_ram0;
    NDISK_T *ptMassNDisk;
    NDISK_T MassNDisk;
    int     i, j, status=0 ;
    int     blocks_in_gnand, gpba, lba;
    int     p2ln_block, op_block;
    int     nWritePages;
    P2LM_T  *p2lm = NULL;
    UINT8   *buff;

    sysprintf("Do NAND post-process if needed.\n");

    sicOpen();
    ptMassNDisk = (NDISK_T*)&MassNDisk;
    if (nandInit0((NDISK_T *)ptMassNDisk))
    {
        sysprintf("ERROR: NAND 0 initial fail !!\n");
        status = -1;
        goto _exit_;
    }
    ptr_g_ram0 = (UINT8 *)((UINT32)g_ram0 | 0x80000000);    // non-cache

    DBG_PRINTF("pSM0->uLibStartBlock = %d\n", pSM0->uLibStartBlock);

    for (i = 0; i < 8; i++)
    {
        nandpread0(i, 0, ptr_g_ram0);
        if (strcmp((char *)ptr_g_ram0, GNAND_MAGIC_WORD) == 0)
        {
            sysprintf("Found %s at block %d, begin to do NAND post-process...\n", GNAND_MAGIC_WORD, i);

            //--- initial P2LN mapping table
            blocks_in_gnand = ptMassNDisk->nZone * ptMassNDisk->nBlockPerZone;
            p2lm = (P2LM_T *)malloc(4 * blocks_in_gnand);
            if (p2lm == NULL)
            {
                status = -1;
                goto _exit_;
            }
            memset(p2lm, 0xFF, 4 * blocks_in_gnand);    // initial to FREE_BLOCK (0xFFFF)

            //--- allocate P2LN block
            p2ln_block = -1;
            op_block = -1;
            for (gpba = 0; gpba < 8; gpba++)
            {
                if (nand_is_valid_block0(gpba))
                {
                    if (gpba == i)
                        p2lm[gpba].lba = FREE_BLOCK;    // GNAND_MAGIC_WORD is here now. We have to erase it when everything done.
                    else if (p2ln_block == -1)
                    {
                        p2lm[gpba].lba = P2LN_BLOCK;
                        p2ln_block = gpba;
                        DBG_PRINTF("Allocate P2LN block to block %d.\n", gpba);
                    }
                    else
                        p2lm[gpba].lba = FREE_BLOCK;
                }
                else
                {
                    p2lm[gpba].lba = BAD_BLOCK;
                    DBG_PRINTF("Found bad block %d and skip it.\n", gpba);
                }
                p2lm[gpba].age = 0;
            }

            if (p2ln_block == -1)
            {
                sysprintf("ERROR: No free block for P2LN table when NAND post-process, NAND crash !!\n");
                status = -1;
                goto _exit_;
            }

            //--- allocate OP block
            for (gpba = 8; gpba < blocks_in_gnand; gpba++)
            {
                if (nand_is_valid_block0(gpba))
                {
                    if (op_block == -1)
                    {
                        // the first valid block is reserved for OP table when initial mapping table
                        p2lm[gpba].lba = OP_BLOCK;
                        p2lm[gpba].age = 0;
                        op_block = gpba;
                        DBG_PRINTF("Allocate OP block to block %d.\n", gpba);
                        break;
                    }
                }
                else
                {
                    p2lm[gpba].lba = BAD_BLOCK;
                    p2lm[gpba].age = 0;
                    DBG_PRINTF("Found bad block %d and skip it.\n", gpba);
                }
            }

            //--- initial P2LN information block
            memset(ptr_g_ram0, 0xff, ptMassNDisk->nPageSize);
            strcpy((char *)ptr_g_ram0, P2LN_INFO_MAGIC);
            strcpy((char *)(ptr_g_ram0+8), P2LN_INFO_VERSION);
            strcpy((char *)(ptr_g_ram0+16), P2LN_INFO_DATE);
            GNAND_PUT16_L(ptr_g_ram0, 32, op_block);        // OP block address
            GNAND_PUT16_L(ptr_g_ram0, 34, op_block);        // Old OP block address
            GNAND_PUT16_L(ptr_g_ram0, 36, p2ln_block);      // old P2LN block address
            if (ptMassNDisk->need2P2LN)
                GNAND_PUT16_L(ptr_g_ram0, 38, p2ln_block);  // old P2LN1 block address
            GNAND_PUT32_L(ptr_g_ram0, 40, ptMassNDisk->nBlockPerZone);

            status = nandpwrite0(p2ln_block, 0, ptr_g_ram0);
            if (status < 0)
                goto _exit_;

            //--- create block mapping table
            lba = 0;
            for (gpba = op_block + 1; gpba < blocks_in_gnand; gpba++)
            {
                if (nand_is_valid_block0(gpba))
                {
                    if (nand_is_page_dirty0(gpba, 0))
                    {
                        p2lm[gpba].lba = lba;
                        lba++;
                    }
                    else
                        p2lm[gpba].lba = FREE_BLOCK;
                }
                else
                {
                    p2lm[gpba].lba = BAD_BLOCK;
                    DBG_PRINTF("Found bad block %d and skip it.\n", gpba);
                }
                p2lm[gpba].age = 0;
            }
        
            //--- write p2lm to P2LN block page 1 ~
            nWritePages = (4 * blocks_in_gnand) / ptMassNDisk->nPageSize;
            if (((4 * blocks_in_gnand) % ptMassNDisk->nPageSize) != 0)
                nWritePages++;
        
            buff = (UINT8 *)p2lm;
            for (j = 1; j <= nWritePages; j++)
            {
                memset(ptr_g_ram0, 0xff, ptMassNDisk->nPageSize);
                memcpy(ptr_g_ram0, buff, ptMassNDisk->nPageSize);
                status = nandpwrite0(p2ln_block, j, ptr_g_ram0);
                if (status < 0)
                    goto _exit_;
                buff += ptMassNDisk->nPageSize;
            }
            DBG_PRINTF("Allocate %d pages as block mapping table.\n", nWritePages);
            
            //--- erase block with GNAND_MAGIC_WORD. Post-process done.
            nand_block_erase0(i);
            sysprintf("NAND post-process done!!\n");
            
        }   // end of found GNAND_MAGIC_WORD
    }

    //--- close SIC/NAND engine
_exit_:
    
    if (p2lm != NULL)
        free (p2lm);
    fmiSMClose(0);
    sicClose();
    return status;
}
#endif  // end of __KLE_DEMO__
