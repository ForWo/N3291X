/*-----------------------------------------------------------------------------------*/
/* Nuvoton Technology Corporation confidential                                       */
/*                                                                                   */
/* Copyright (c) 2008 by Nuvoton Technology Corporation                              */
/* All rights reserved                                                               */
/*                                                                                   */
/*-----------------------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>

#include "w55fa95_reg.h"
#include "wblib.h"
#include "w55fa95_sic.h"
#include "nvtfat.h"
#include "Font.h"
#include "writer.h"

extern void sicSMsetBCH(FMI_SM_INFO_T *pSM, int inIBR);
extern UINT32 fmiSM_GetIBRAreaSize(FMI_SM_INFO_T *pSM);
extern INT fmiSM_Reset(FMI_SM_INFO_T *pSM);
extern INT fmiSMCheckRB(FMI_SM_INFO_T *pSM);

INT8 nIsSysImage;
int volatile gMarkCount=0;
UINT8 *pInfo;
int volatile g_u32ExtraDataSize;
BOOL volatile gbSystemImage;
extern INT32 gNandLoaderSize;

#define NAND_EXTRA_512      16
#define NAND_EXTRA_2K       64
#define NAND_EXTRA_4K       128
#define NAND_EXTRA_8K       376

#define PRINTF  sysprintf

/* Set BCH function & Page Size */
VOID nvtSM_Initial(FMI_SM_INFO_T *pSM)  /* OK */
{
    outpw(REG_SMCSR, inpw(REG_SMCSR) & (~SMCR_PSIZE));

    if (pSM->nPageSize == NAND_PAGE_8KB)
    {
        outpw(REG_SMCSR, (inpw(REG_SMCSR) & ~(SMCR_BCH_TSEL|SMCR_ECC_EN)) | (BCH_T12 | SMCR_ECC_EN | SMCR_ECC_3B_PROTECT|PSIZE_8K));
        outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | PSIZE_8K);   // to meet FA95 spec, MUST reset PSIZE after BCH_TSEL
        g_u32ExtraDataSize = NAND_EXTRA_8K;
        outpw(REG_SMREAREA_CTL, g_u32ExtraDataSize);
        PRINTF("The Test NAND is 8KB page size\n");
    }
    else if (pSM->nPageSize == NAND_PAGE_4KB)
    {
        outpw(REG_SMCSR, (inpw(REG_SMCSR) & ~(SMCR_BCH_TSEL|SMCR_ECC_EN)) | (BCH_T8 | SMCR_ECC_EN | SMCR_ECC_3B_PROTECT | PSIZE_4K));
        outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | PSIZE_4K);   // to meet FA95 spec, MUST reset PSIZE after BCH_TSEL
        g_u32ExtraDataSize = NAND_EXTRA_4K;
        outpw(REG_SMREAREA_CTL, g_u32ExtraDataSize);
        PRINTF("The Test NAND is 4KB page size\n");
    }
    else if (pSM->nPageSize == NAND_PAGE_2KB)
    {
        outpw(REG_SMCSR, (inpw(REG_SMCSR) & ~(SMCR_BCH_TSEL|SMCR_ECC_EN)) | (BCH_T4 | SMCR_ECC_EN | SMCR_ECC_3B_PROTECT | PSIZE_2K));
        outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | PSIZE_2K);   // to meet FA95 spec, MUST reset PSIZE after BCH_TSEL
        g_u32ExtraDataSize = NAND_EXTRA_2K;
        outpw(REG_SMREAREA_CTL, g_u32ExtraDataSize);
        PRINTF("The Test NAND is 2KB page size\n");
    }
    else if (pSM->nPageSize == NAND_PAGE_512B)
    {
        outpw(REG_SMCSR, (inpw(REG_SMCSR) & ~(SMCR_BCH_TSEL|SMCR_ECC_EN)) | (BCH_T4 | SMCR_ECC_EN | SMCR_ECC_3B_PROTECT | PSIZE_512));
        outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | PSIZE_512);  // to meet FA95 spec, MUST reset PSIZE after BCH_TSEL
        g_u32ExtraDataSize = NAND_EXTRA_512;
        outpw(REG_SMREAREA_CTL, g_u32ExtraDataSize);
        PRINTF("The Test NAND is 512B page size\n");
    }
}

// SM functions
INT nvtSM_ReadID(FMI_SM_INFO_T *pSM)    //Moidified
{
    UINT32 tempID[5];

    fmiSM_Reset(pSM);
    outpw(REG_SMCMD, 0x90);     // read ID command
    outpw(REG_SMADDR, 0x80000000);  // address 0x00

    tempID[0] = inpw(REG_SMDATA);
    tempID[1] = inpw(REG_SMDATA);
    tempID[2] = inpw(REG_SMDATA);
    tempID[3] = inpw(REG_SMDATA);
    tempID[4] = inpw(REG_SMDATA);

    if (tempID[0] == 0xC2)
        pSM->bIsCheckECC = FALSE;
    else
        pSM->bIsCheckECC = TRUE;

    pSM->bIsNandECC4 = FALSE;
    pSM->bIsNandECC8 = FALSE;
    pSM->bIsNandECC12 = FALSE;
    pSM->bIsNandECC15 = FALSE;
    pSM->bIsNandECC24 = FALSE;

// 2011/12/13, support Hynix H27UAG8T2A that with larger redundancy area size 224.
//      fmiSM_Initial() will set Redundancy Area size to 224 for 4K page NAND if pSM->bIsRA224 is TRUE.
//      Else, the default Redundancy Area size for other 4K page NAND is 216.
    pSM->bIsRA224 = 0;

    switch (tempID[1])
    {
        /* page size 512B */
        case 0x79:  // 128M
            pSM->uSectorPerFlash = 255744;
            pSM->uBlockPerFlash = 8191;
            pSM->uPagePerBlock = 32;
            pSM->uSectorPerBlock = 32;
            pSM->bIsMulticycle = TRUE;
            pSM->nPageSize = NAND_PAGE_512B;
            pSM->bIsNandECC4 = TRUE;
            break;

        case 0x76:  // 64M
            pSM->uSectorPerFlash = 127872;
            pSM->uBlockPerFlash = 4095;
            pSM->uPagePerBlock = 32;
            pSM->uSectorPerBlock = 32;
            pSM->bIsMulticycle = TRUE;
            pSM->nPageSize = NAND_PAGE_512B;
            pSM->bIsNandECC4 = TRUE;
            break;

        case 0x75:  // 32M
            pSM->uSectorPerFlash = 63936;
            pSM->uBlockPerFlash = 2047;
            pSM->uPagePerBlock = 32;
            pSM->uSectorPerBlock = 32;
            pSM->bIsMulticycle = FALSE;
            pSM->nPageSize = NAND_PAGE_512B;
            pSM->bIsNandECC4 = TRUE;
            break;

        case 0x73:  // 16M
            pSM->uSectorPerFlash = 31968;   // max. sector no. = 999 * 32
            pSM->uBlockPerFlash = 1023;
            pSM->uPagePerBlock = 32;
            pSM->uSectorPerBlock = 32;
            pSM->bIsMulticycle = FALSE;
            pSM->nPageSize = NAND_PAGE_512B;
            pSM->bIsNandECC4 = TRUE;
            break;

        /* page size 2KB */
        case 0xf1:  // 128M
        case 0xd1:
            pSM->uBlockPerFlash = 1023;
            pSM->uPagePerBlock = 64;
            pSM->uSectorPerBlock = 256;
            pSM->uSectorPerFlash = 255744;
            pSM->bIsMulticycle = FALSE;
            pSM->nPageSize = NAND_PAGE_2KB;
            //pSM->bIsNandECC8 = TRUE;
            pSM->bIsNandECC4 = TRUE;

            break;

        case 0xda:  // 256M
            if ((tempID[3] & 0x33) == 0x11)
            {
                pSM->uBlockPerFlash = 2047;
                pSM->uPagePerBlock = 64;
                pSM->uSectorPerBlock = 256;
            }
            else if ((tempID[3] & 0x33) == 0x21)
            {
                pSM->uBlockPerFlash = 1023;
                pSM->uPagePerBlock = 128;
                pSM->uSectorPerBlock = 512;
                pSM->bIsMLCNand = TRUE;
            }

            if ((tempID[3] & 0x0C) == 0x04)
                pSM->bIsNandECC4 = TRUE;
            else if ((tempID[3] & 0x0C) == 0x08)
                pSM->bIsNandECC4 = FALSE;

            pSM->uSectorPerFlash = 511488;
            pSM->bIsMulticycle = TRUE;
            pSM->nPageSize = NAND_PAGE_2KB;

            pSM->bIsNandECC4 = FALSE;
            pSM->bIsNandECC8 = TRUE;
            break;

        case 0xdc:  // 512M
            if ((tempID[3] & 0x33) == 0x11)
            {
                pSM->uBlockPerFlash = 4095;
                pSM->uPagePerBlock = 64;
                pSM->uSectorPerBlock = 256;
            }
            else if ((tempID[3] & 0x33) == 0x21)
            {
                pSM->uBlockPerFlash = 2047;
                pSM->uPagePerBlock = 128;
                pSM->uSectorPerBlock = 512;
                pSM->bIsMLCNand = TRUE;
            }

            if ((tempID[3] & 0x0C) == 0x04)
                pSM->bIsNandECC4 = TRUE;
            else if ((tempID[3] & 0x0C) == 0x08)
                pSM->bIsNandECC4 = FALSE;

            pSM->uSectorPerFlash = 1022976;
            pSM->bIsMulticycle = TRUE;
            pSM->nPageSize = NAND_PAGE_2KB;

            pSM->bIsNandECC4 = FALSE;
            pSM->bIsNandECC8 = TRUE;
            break;

        case 0xd3:  // 1024M
            if ((tempID[3] & 0x33) == 0x32)
            {
                pSM->uBlockPerFlash = 2047;
                pSM->uPagePerBlock = 128;
                pSM->uSectorPerBlock = 1024;    /* 128x8 */
                pSM->nPageSize = NAND_PAGE_4KB;
                pSM->bIsMLCNand = TRUE;
            }
            else if ((tempID[3] & 0x33) == 0x11)
            {
                pSM->uBlockPerFlash = 8191;
                pSM->uPagePerBlock = 64;
                pSM->uSectorPerBlock = 256;
                pSM->nPageSize = NAND_PAGE_2KB;
            }
            else if ((tempID[3] & 0x33) == 0x21)
            {
                pSM->uBlockPerFlash = 4095;
                pSM->uPagePerBlock = 128;
                pSM->uSectorPerBlock = 512;
                pSM->bIsMLCNand = TRUE;
                pSM->nPageSize = NAND_PAGE_2KB;
            }
            else if ((tempID[3] & 0x33) == 0x22)
            {
                pSM->uBlockPerFlash = 4095;
                pSM->uPagePerBlock = 64;
                pSM->uSectorPerBlock = 512; /* 64x8 */
                pSM->nPageSize = NAND_PAGE_4KB;
                pSM->bIsMLCNand = FALSE;
            }

            if ((tempID[3] & 0x0C) == 0x04)
                pSM->bIsNandECC4 = TRUE;
            else if ((tempID[3] & 0x0C) == 0x08)
                pSM->bIsNandECC4 = FALSE;

            pSM->uSectorPerFlash = 2045952;
            pSM->bIsMulticycle = TRUE;

            if ((pSM->nPageSize == NAND_PAGE_4KB) ||
                (pSM->bIsNandECC4 == FALSE) )
            {
                pSM->bIsNandECC4 = FALSE;
                pSM->bIsNandECC8 = TRUE;
                pSM->bIsNandECC12 = FALSE;
            }
            else
            {
                pSM->bIsNandECC4 = FALSE;
                pSM->bIsNandECC8 = TRUE;
            }
            break;

        case 0xd5:  // 2048M

            // 2011/7/28, To support Hynix H27UAG8T2B NAND flash
            if ((tempID[0]==0xAD)&&(tempID[2]==0x94)&&(tempID[3]==0x9A))
            {
                pSM->uBlockPerFlash  = 1023;        // block index with 0-base. = physical blocks - 1
                pSM->uPagePerBlock   = 256;
                pSM->nPageSize       = NAND_PAGE_8KB;
                pSM->uSectorPerBlock = pSM->nPageSize / 512 * pSM->uPagePerBlock;
                pSM->bIsMLCNand      = TRUE;
                pSM->bIsMulticycle   = TRUE;
                pSM->bIsNandECC24    = TRUE;

                // Reserve 0.1% sectors for bad block ? Seem no used in NAND and GNAND !!
                pSM->uSectorPerFlash = pSM->uSectorPerBlock * pSM->uBlockPerFlash * 999 / 1000;
                break;
            }

            // 2011/7/28, To support Toshiba TC58NVG4D2FTA00 NAND flash
            if ((tempID[0]==0x98)&&(tempID[2]==0x94)&&(tempID[3]==0x32))
            {
                pSM->uBlockPerFlash  = 2075;        // block index with 0-base. = physical blocks - 1
                pSM->uPagePerBlock   = 128;
                pSM->nPageSize       = NAND_PAGE_8KB;
                pSM->uSectorPerBlock = pSM->nPageSize / 512 * pSM->uPagePerBlock;
                pSM->bIsMLCNand      = TRUE;
                pSM->bIsMulticycle   = TRUE;
                pSM->bIsNandECC24    = TRUE;

                // Reserve 0.1% sectors for bad block ? Seem no used in NAND and GNAND !!
                pSM->uSectorPerFlash = pSM->uSectorPerBlock * pSM->uBlockPerFlash * 999 / 1000;
                break;
            }

            if ((tempID[0]==0xAD)&&(tempID[2] == 0x94)&&(tempID[3] == 0x25))
            {
                pSM->uBlockPerFlash = 4095;
                pSM->uPagePerBlock = 128;
                pSM->uSectorPerBlock = 1024;    /* 128x8 */
                pSM->nPageSize = NAND_PAGE_4KB;
                pSM->bIsMLCNand = TRUE;

                pSM->uSectorPerFlash = 4091904;
                pSM->bIsMulticycle = TRUE;

                pSM->bIsNandECC4 = FALSE;
                pSM->bIsNandECC8 = FALSE;
                pSM->bIsNandECC12 = TRUE;
                pSM->bIsRA224 = 1;
                break;
            }
            else
            {
            if ((tempID[3] & 0x33) == 0x32)
            {
                pSM->uBlockPerFlash = 4095;
                pSM->uPagePerBlock = 128;
                pSM->uSectorPerBlock = 1024;    /* 128x8 */
                pSM->nPageSize = NAND_PAGE_4KB;
                pSM->bIsMLCNand = TRUE;
            }
            else if ((tempID[3] & 0x33) == 0x11)
            {
                pSM->uBlockPerFlash = 16383;
                pSM->uPagePerBlock = 64;
                pSM->uSectorPerBlock = 256;
                pSM->nPageSize = NAND_PAGE_2KB;
            }
            else if ((tempID[3] & 0x33) == 0x21)
            {
                pSM->uBlockPerFlash = 8191;
                pSM->uPagePerBlock = 128;
                pSM->uSectorPerBlock = 512;
                pSM->nPageSize = NAND_PAGE_2KB;
                pSM->bIsMLCNand = TRUE;
            }

            if ((tempID[3] & 0x0C) == 0x04)
                pSM->bIsNandECC4 = TRUE;
            else if ((tempID[3] & 0x0C) == 0x08)
                pSM->bIsNandECC4 = FALSE;

            pSM->uSectorPerFlash = 4091904;
            pSM->bIsMulticycle = TRUE;

                if ((pSM->nPageSize == NAND_PAGE_4KB) ||
                    (pSM->bIsNandECC4 == FALSE) )
                {
                    pSM->bIsNandECC4 = FALSE;
                    pSM->bIsNandECC8 = TRUE;
                    pSM->bIsNandECC12 = FALSE;
                }
                else
                {
                    pSM->bIsNandECC4 = FALSE;
                    pSM->bIsNandECC8 = TRUE;
                }
            break;
            }

        default:
            // 2012/3/8, To support Micron MT29F16G08CBACA NAND flash
            if ((tempID[0]==0x2C)&&(tempID[1]==0x48)&&(tempID[2]==0x04)&&(tempID[3]==0x4A)&&(tempID[4]==0xA5))
            {
                pSM->uBlockPerFlash  = 2047;        // block index with 0-base. = physical blocks - 1
                pSM->uPagePerBlock   = 256;
                pSM->nPageSize       = NAND_PAGE_4KB;
                pSM->uSectorPerBlock = pSM->nPageSize / 512 * pSM->uPagePerBlock;
                pSM->bIsMLCNand      = TRUE;
                pSM->bIsMulticycle   = TRUE;
                pSM->bIsNandECC24    = TRUE;

                pSM->uSectorPerFlash = 4091904;
                break;
            }
            // 2012/3/27, To support Micron MT29F32G08CBACA NAND flash
            else if ((tempID[0]==0x2C)&&(tempID[1]==0x68)&&(tempID[2]==0x04)&&(tempID[3]==0x4A)&&(tempID[4]==0xA9))
            {
                pSM->uBlockPerFlash  = 4095;        // block index with 0-base. = physical blocks - 1
                pSM->uPagePerBlock   = 256;
                pSM->nPageSize       = NAND_PAGE_4KB;
                pSM->uSectorPerBlock = pSM->nPageSize / 512 * pSM->uPagePerBlock;
                pSM->bIsMLCNand      = TRUE;
                pSM->bIsMulticycle   = TRUE;
                pSM->bIsNandECC24    = TRUE;

                pSM->uSectorPerFlash = 8183808;
                break;
            }

            PRINTF("SM ID not support!![%x][%x][%x][%x]\n", tempID[0], tempID[1], tempID[2], tempID[3]);
            return FMI_SM_ID_ERR;
    }

    PRINTF("SM ID [%x][%x][%x][%x]\n", tempID[0], tempID[1], tempID[2], tempID[3]);

    PRINTF("nvtSM_ReadID: Found %s NAND, ID %02X-%02X-%02X-%02X, page size %d, BCH T%d\n",
        pSM->bIsMLCNand ? "MLC" : "SLC",
        tempID[0], tempID[1], tempID[2], tempID[3],
        pSM->nPageSize,
        pSM->bIsNandECC4*4 + pSM->bIsNandECC8*8 + pSM->bIsNandECC12*12 + pSM->bIsNandECC15*15 + pSM->bIsNandECC24*24
        );

    return 0;
}


INT nvtSM_Write_512(FMI_SM_INFO_T *pSM, UINT32 uSector, UINT32 ucColAddr, UINT32 uSAddr)    //Modified
{
    int temp;

    // set the spare area configuration
    if (nIsSysImage != 0xFF)
    {
        // set Stop Marker on the first empty page
        if ((gMarkCount >> 8) * pSM->nPageSize >= gNandLoaderSize)
        {
            outpw(REG_SMRA_0, 0x00A55AFF);      // Stop Marker
            gMarkCount += 0x100;
        }
        else
        {
            temp = (((nIsSysImage + gMarkCount) & 0xFFFF) << 8) + 0x000000FF;
            outpw(REG_SMRA_0, temp);
            gMarkCount += 0x100;
            if(gMarkCount == 0x1D)  /* Next page is 0x1D (page 29) */
                gMarkCount = 0xA500;
        }
    }
    else
    {
        outpw(REG_SMRA_0, 0x0000FFFF);
        gMarkCount = 0;
    }
//  outpw(REG_SM_RA1, 0xFFFFFFFF);

    outpw(REG_FMICR, inpw(REG_FMICR)|FMI_SM_EN);

    fmiSM_Reset(pSM);

    /* set Page Size and BCH */
    sicSMsetBCH(pSM, gbSystemImage);
    g_u32ExtraDataSize = inpw(REG_SMREAREA_CTL) & SMRE_REA128_EXT;

    outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_REDUN_AUTO_WEN);            // enable Auto Write
    outpw(REG_SMCSR, inpw(REG_SMCSR) & ~SMCR_REDUN_REN);                // enable Read
    /* enable DMAC */
    while(inpw(REG_DMACCSR) & FMI_BUSY);                // wait DMAC FMI ready;
    outpw(REG_DMACCSR, inpw(REG_DMACCSR) | DMAC_EN);

    /* set DMA Transfer Starting Address */
    outpw(REG_DMACSAR, uSAddr);

    /* set SM columm & page address */
    outpw(REG_SMCMD, 0x80);                             // Program setup command
    outpw(REG_SMADDR, ucColAddr & 0xFF);                // CA0 - CA7
    outpw(REG_SMADDR, uSector & 0xFF);              // PA0 - PA7

    if (!pSM->bIsMulticycle)
        outpw(REG_SMADDR, ((uSector >> 8) & 0xFF) | EOA_SM);    // PA8 - PA15
    else
    {
        outpw(REG_SMADDR, (uSector >> 8) & 0xFF);   // PA8 - PA15
        outpw(REG_SMADDR, ((uSector >> 16) & 0xFF) | EOA_SM);   // PA16 - PA17
    }

    /* begin DMA write transfer */
    outpw(REG_SMISR, SMISR_DMA_IF);         // clear DMA flag
    outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_DWR_EN);

    while(!(inpw(REG_SMISR) & SMISR_DMA_IF));           // wait to finish DMAC transfer.
    outpw(REG_SMISR, SMISR_DMA_IF);                     // clear DMA flag

    outpw(REG_SMCMD, 0x10);                             // Progeam command

    if (!fmiSMCheckRB(pSM))
        return -1;

    outpw(REG_SMCMD, 0x70);             // read STATUS command

    if (inpw(REG_SMDATA) & 0x01)
    {
        PRINTF("DrvNAND_WriteOnePage_512: data error [%d]!!\n", uSector);

        return -1;  // Program Fail
    }
    return 0;
}


INT nvtSM_Write_2K(FMI_SM_INFO_T *pSM, UINT32 uSector, UINT32 ucColAddr, UINT32 uSAddr) //Modified
{
    int temp;

    if (nIsSysImage != 0xFF)
    {
        // set Stop Marker on the first empty page
        if ((gMarkCount >> 8) * pSM->nPageSize >= gNandLoaderSize)
        {
            outpw(REG_SMRA_0, 0x00A55AFF);      // Stop Marker
            gMarkCount += 0x100;
        }
        else
        {
            temp = (((nIsSysImage + gMarkCount) & 0xFFFF) << 8) + 0x000000FF;
            outpw(REG_SMRA_0, temp);
            gMarkCount += 0x100;
        }
    }
    else
    {
        outpw(REG_SMRA_0, 0x0000FFFF);
        gMarkCount = 0;
    }
    /*
    outpw(REG_SM_RA1, 0xFFFFFFFF);
    outpw(REG_SM_RA4, 0x0000FFFF);
    outpw(REG_SM_RA5, 0xFFFFFFFF);
    outpw(REG_SM_RA8, 0x0000FFFF);
    outpw(REG_SM_RA9, 0xFFFFFFFF);
    outpw(REG_SM_RA12, 0x0000FFFF);
    outpw(REG_SM_RA13, 0xFFFFFFFF);*/

    outpw(REG_FMICR, inpw(REG_FMICR)|FMI_SM_EN);

    fmiSM_Reset(pSM);

    /* set Page Size and BCH */
    sicSMsetBCH(pSM, gbSystemImage);
    g_u32ExtraDataSize = inpw(REG_SMREAREA_CTL) & SMRE_REA128_EXT;

    outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_REDUN_AUTO_WEN);            // enable Auto Write

    /* enable DMAC */
    while(inpw(REG_DMACCSR) & FMI_BUSY);    // wait DMAC FMI ready;
    outpw(REG_DMACCSR, inpw(REG_DMACCSR) | DMAC_EN);

    /* Set DMA Transfer Starting Address */
    outpw(REG_DMACSAR, uSAddr);

    outpw(REG_SMCMD, 0x80);                 // Program setup command

    outpw(REG_SMADDR, ucColAddr & 0xFF);
    outpw(REG_SMADDR, (ucColAddr >> 8) & 0xFF);
    outpw(REG_SMADDR, uSector & 0xFF);

    if (!pSM->bIsMulticycle)
        outpw(REG_SMADDR, ((uSector >> 8) & 0xFF) | EOA_SM);    // PA8 - PA15
    else
    {
        outpw(REG_SMADDR, (uSector >> 8) & 0xFF);                   // PA8 - PA15
        outpw(REG_SMADDR, ((uSector >> 16) & 0xFF) | EOA_SM);   // PA16 - PA17
    }

    /* begin DMA write transfer */
    outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_DWR_EN);

    while(!(inpw(REG_SMISR) & SMISR_DMA_IF));       // wait to finish DMAC transfer.
    outpw(REG_SMISR, SMISR_DMA_IF);                 // clear DMA flag

    outpw(REG_SMCMD, 0x10);                         // Progeam command

    if (!fmiSMCheckRB(pSM))
        return -1;

    outpw(REG_SMCMD, 0x70);                     // read STATUS command

    if (inpw(REG_SMDATA) & 0x01)
    {
        PRINTF("DrvNAND_WriteOnePage 2K: data error [%d]!!\n", uSector);
        return -1;  // Program Fail
    }
    return 0;               // Program Success
}


INT nvtSM_Write_4K(FMI_SM_INFO_T *pSM, UINT32 uSector, UINT32 ucColAddr, UINT32 uSAddr) //Modified
{
    int temp;

    if (nIsSysImage != 0xFF)
    {
        // set Stop Marker on the first empty page
        if ((gMarkCount >> 8) * pSM->nPageSize >= gNandLoaderSize)
        {
            outpw(REG_SMRA_0, 0x00A55AFF);      // Stop Marker
            gMarkCount += 0x100;
        }
        else
        {
            temp = (((nIsSysImage + gMarkCount) & 0xFFFF) << 8) + 0x000000FF;
            outpw(REG_SMRA_0, temp);
            gMarkCount += 0x100;
        }
    }
    else
    {
        outpw(REG_SMRA_0, 0x0000FFFF);
        gMarkCount = 0;
    }

    outpw(REG_FMICR, inpw(REG_FMICR)|FMI_SM_EN);

    fmiSM_Reset(pSM);

    /* set Page Size and BCH */
    sicSMsetBCH(pSM, gbSystemImage);
    g_u32ExtraDataSize = inpw(REG_SMREAREA_CTL) & SMRE_REA128_EXT;

    outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_REDUN_AUTO_WEN);            // enable Auto Write

    /* enable DMAC */
    while(inpw(REG_DMACCSR) & FMI_BUSY);    // wait DMAC FMI ready;
    outpw(REG_DMACCSR, inpw(REG_DMACCSR) | DMAC_EN);

    /* Set DMA Transfer Starting Address */
    outpw(REG_DMACSAR, uSAddr);

    outpw(REG_SMCMD, 0x80);                 // Program setup command

    outpw(REG_SMADDR, ucColAddr & 0xFF);
    outpw(REG_SMADDR, (ucColAddr >> 8) & 0xFF);
    outpw(REG_SMADDR, uSector & 0xFF);

    if (!pSM->bIsMulticycle)
    {
        outpw(REG_SMADDR, ((uSector >> 8) & 0xFF) | EOA_SM);    // PA8 - PA15
    }
    else
    {
        outpw(REG_SMADDR, (uSector >> 8) & 0xFF);                   // PA8 - PA15
        outpw(REG_SMADDR, ((uSector >> 16) & 0xFF) | EOA_SM);   // PA16 - PA17
    }

    /* begin DMA write transfer */
    outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_DWR_EN);

    while(!(inpw(REG_SMISR) & SMISR_DMA_IF));         // wait to finish DMAC transfer.
    outpw(REG_SMISR, SMISR_DMA_IF);                 // clear DMA flag

    outpw(REG_SMCMD, 0x10);     // auto program command

    if (!fmiSMCheckRB(pSM))
        return -1;

    outpw(REG_SMCMD, 0x70);     // status read command

    if (inpw(REG_SMDATA) & 0x1)
    {
        PRINTF("DrvNAND_WriteOnePage_4K: data error [%d]!!\n", uSector);
        return -1;  // Program Fail
    }
    return 0;
}


INT nvtSM_Write_8K(FMI_SM_INFO_T *pSM, UINT32 uSector, UINT32 ucColAddr, UINT32 uSAddr) //Added
{
    int temp;

    if (nIsSysImage != 0xFF)
    {
        // set Stop Marker on the first empty page
        if ((gMarkCount >> 8) * pSM->nPageSize >= gNandLoaderSize)
        {
            outpw(REG_SMRA_0, 0x00A55AFF);      // Stop Marker
            gMarkCount += 0x100;
        }
        else
        {
            temp = (((nIsSysImage + gMarkCount) & 0xFFFF) << 8) + 0x000000FF;
            outpw(REG_SMRA_0, temp);
            gMarkCount += 0x100;
        }
    }
    else
    {
        outpw(REG_SMRA_0, 0x0000FFFF);
        gMarkCount = 0;
    }

    outpw(REG_FMICR, inpw(REG_FMICR)|FMI_SM_EN);

    fmiSM_Reset(pSM);

    /* set Page Size and BCH */
    sicSMsetBCH(pSM, gbSystemImage);
    g_u32ExtraDataSize = inpw(REG_SMREAREA_CTL) & SMRE_REA128_EXT;    

    outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_REDUN_AUTO_WEN);            // enable Auto Write

    /* enable DMAC */
    while(inpw(REG_DMACCSR) & FMI_BUSY);    // wait DMAC FMI ready;
    outpw(REG_DMACCSR, inpw(REG_DMACCSR) | DMAC_EN);

    /* Set DMA Transfer Starting Address */
    outpw(REG_DMACSAR, uSAddr);

    outpw(REG_SMCMD, 0x80);                 // Program setup command

    outpw(REG_SMADDR, ucColAddr & 0xFF);
    outpw(REG_SMADDR, (ucColAddr >> 8) & 0xFF);
    outpw(REG_SMADDR, uSector & 0xFF);

    if (!pSM->bIsMulticycle)
    {
        outpw(REG_SMADDR, ((uSector >> 8) & 0xFF) | EOA_SM);    // PA8 - PA15
    }
    else
    {
        outpw(REG_SMADDR, (uSector >> 8) & 0xFF);               // PA8 - PA15
        outpw(REG_SMADDR, ((uSector >> 16) & 0xFF) | EOA_SM);   // PA16 - PA17
    }

    /* begin DMA write transfer */
    outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_DWR_EN);

    while(!(inpw(REG_SMISR) & SMISR_DMA_IF));         // wait to finish DMAC transfer.
    outpw(REG_SMISR, SMISR_DMA_IF);                 // clear DMA flag

    outpw(REG_SMCMD, 0x10);     // auto program command

    if (!fmiSMCheckRB(pSM))
        return -1;

    outpw(REG_SMCMD, 0x70);     // status read command

    if (inpw(REG_SMDATA) & 0x1)
    {
        PRINTF("DrvNAND_WriteOnePage_8K: data error [%d]!!\n", uSector);
        return -1;  // Program Fail
    }
    return 0;
}


/*-----------------------------------------------------------------------------
 * Define some constants for BCH
 *---------------------------------------------------------------------------*/
// define the total padding bytes for 512/1024 data segment
#define BCH_PADDING_LEN_512     32
#define BCH_PADDING_LEN_1024    64
// define the BCH parity code lenght for 512 bytes data pattern
#define BCH_PARITY_LEN_T4  8
#define BCH_PARITY_LEN_T8  15
#define BCH_PARITY_LEN_T12 23
#define BCH_PARITY_LEN_T15 29
// define the BCH parity code lenght for 1024 bytes data pattern
#define BCH_PARITY_LEN_T24 45

static void fmiSM_CorrectData_BCH(UINT8 ucFieidIndex, UINT8 ucErrorCnt, UINT8* pDAddr)
{
    UINT32 uaData[24], uaAddr[24];
    UINT32 uaErrorData[4];
    UINT8  ii, jj;
    UINT32 uPageSize;
    UINT32 field_len, padding_len, parity_len;
    UINT32 total_field_num;
    UINT8  *smra_index;

    //--- assign some parameters for different BCH and page size
    switch (inpw(REG_SMCSR) & SMCR_BCH_TSEL)
    {
        case BCH_T24:
            field_len   = 1024;
            padding_len = BCH_PADDING_LEN_1024;
            parity_len  = BCH_PARITY_LEN_T24;
            break;
        case BCH_T15:
            field_len   = 512;
            padding_len = BCH_PADDING_LEN_512;
            parity_len  = BCH_PARITY_LEN_T15;
            break;
        case BCH_T12:
            field_len   = 512;
            padding_len = BCH_PADDING_LEN_512;
            parity_len  = BCH_PARITY_LEN_T12;
            break;
        case BCH_T8:
            field_len   = 512;
            padding_len = BCH_PADDING_LEN_512;
            parity_len  = BCH_PARITY_LEN_T8;
            break;
        case BCH_T4:
            field_len   = 512;
            padding_len = BCH_PADDING_LEN_512;
            parity_len  = BCH_PARITY_LEN_T4;
            break;
        default:
            PRINTF("ERROR: fmiSM_CorrectData_BCH(): invalid SMCR_BCH_TSEL = 0x%08X\n", (UINT32)(inpw(REG_SMCSR) & SMCR_BCH_TSEL));
            return;
    }

    uPageSize = inpw(REG_SMCSR) & SMCR_PSIZE;
    switch (uPageSize)
    {
        case PSIZE_8K:  total_field_num = 8192 / field_len; break;
        case PSIZE_4K:  total_field_num = 4096 / field_len; break;
        case PSIZE_2K:  total_field_num = 2048 / field_len; break;
        case PSIZE_512: total_field_num =  512 / field_len; break;
        default:
            PRINTF("ERROR: fmiSM_CorrectData_BCH(): invalid SMCR_PSIZE = 0x%08X\n", uPageSize);
            return;
    }

    //--- got valid BCH_ECC_DATAx and parse them to uaData[]
    // got the valid register number of BCH_ECC_DATAx since one register include 4 error bytes
    jj = ucErrorCnt/4;
    jj ++;
    if (jj > 6)
        jj = 6;     // there are 6 BCH_ECC_DATAx registers to support BCH T24

    for(ii=0; ii<jj; ii++)
    {
        uaErrorData[ii] = inpw(REG_BCH_ECC_DATA0 + ii*4);
    }

    for(ii=0; ii<jj; ii++)
    {
        uaData[ii*4+0] = uaErrorData[ii] & 0xff;
        uaData[ii*4+1] = (uaErrorData[ii]>>8) & 0xff;
        uaData[ii*4+2] = (uaErrorData[ii]>>16) & 0xff;
        uaData[ii*4+3] = (uaErrorData[ii]>>24) & 0xff;
    }

    //--- got valid REG_BCH_ECC_ADDRx and parse them to uaAddr[]
    // got the valid register number of REG_BCH_ECC_ADDRx since one register include 2 error addresses
    jj = ucErrorCnt/2;
    jj ++;
    if (jj > 12)
        jj = 12;    // there are 12 REG_BCH_ECC_ADDRx registers to support BCH T24

    for(ii=0; ii<jj; ii++)
    {
        uaAddr[ii*2+0] = inpw(REG_BCH_ECC_ADDR0 + ii*4) & 0x07ff;   // 11 bits for error address
        uaAddr[ii*2+1] = (inpw(REG_BCH_ECC_ADDR0 + ii*4)>>16) & 0x07ff;
    }

    //--- pointer to begin address of field that with data error
    pDAddr += (ucFieidIndex-1) * field_len;

    //--- correct each error bytes
    for(ii=0; ii<ucErrorCnt; ii++)
    {
        // for wrong data in field
        if (uaAddr[ii] < field_len)
        {
            //DBG_PRINTF("BCH error corrected for data: address 0x%08X, data [0x%02X] --> ",
            //    pDAddr+uaAddr[ii], *(pDAddr+uaAddr[ii]));

            *(pDAddr+uaAddr[ii]) ^= uaData[ii];

            //DBG_PRINTF("[0x%02X]\n", *(pDAddr+uaAddr[ii]));
        }
        // for wrong first-3-bytes in redundancy area
        else if (uaAddr[ii] < (field_len+3))
        {
            uaAddr[ii] -= field_len;
            uaAddr[ii] += (parity_len*(ucFieidIndex-1));    // field offset

            //DBG_PRINTF("BCH error corrected for 3 bytes: address 0x%08X, data [0x%02X] --> ",
            //    (UINT8 *)REG_SMRA_0+uaAddr[ii], *((UINT8 *)REG_SMRA_0+uaAddr[ii]));

            *((UINT8 *)REG_SMRA_0+uaAddr[ii]) ^= uaData[ii];

            //DBG_PRINTF("[0x%02X]\n", *((UINT8 *)REG_SMRA_0+uaAddr[ii]));
        }
        // for wrong parity code in redundancy area
        else
        {
            // BCH_ERR_ADDRx = [data in field] + [3 bytes] + [xx] + [parity code]
            //                                   |<--     padding bytes      -->|
            // The BCH_ERR_ADDRx for last parity code always = field size + padding size.
            // So, the first parity code = field size + padding size - parity code length.
            // For example, for BCH T12, the first parity code = 512 + 32 - 23 = 521.
            // That is, error byte address offset within field is
            uaAddr[ii] = uaAddr[ii] - (field_len + padding_len - parity_len);

            // smra_index point to the first parity code of first field in register SMRA0~n
            smra_index = (UINT8 *)
                         (REG_SMRA_0 + (inpw(REG_SMREAREA_CTL) & SMRE_REA128_EXT) - // bottom of all parity code -
                          (parity_len * total_field_num)                            // byte count of all parity code
                         );

            // final address = first parity code of first field +
            //                 offset of fields +
            //                 offset within field
            //DBG_PRINTF("BCH error corrected for parity: address 0x%08X, data [0x%02X] --> ",
            //    smra_index + (parity_len * (ucFieidIndex-1)) + uaAddr[ii],
            //    *((UINT8 *)smra_index + (parity_len * (ucFieidIndex-1)) + uaAddr[ii]));
            *((UINT8 *)smra_index + (parity_len * (ucFieidIndex-1)) + uaAddr[ii]) ^= uaData[ii];
            //DBG_PRINTF("[0x%02X]\n",
            //    *((UINT8 *)smra_index + (parity_len * (ucFieidIndex-1)) + uaAddr[ii]));
        }
    }   // end of for (ii<ucErrorCnt)
}


/* Read Extra Data */
int nvtSM_ReadOnePage_ExtraData(FMI_SM_INFO_T *pSM, UINT32 uPageIndex)  //Added
{
    UINT32 ii;
    UINT8 *uDAddr;

    outpw(REG_FMICR, inpw(REG_FMICR)|FMI_SM_EN);

    fmiSM_Reset(pSM);

    /* set Page Size = 8192 bytes */
    if(pSM->nPageSize == 512)
    {
        outpw(REG_SMCSR, inpw(REG_SMCSR) & (~SMCR_PSIZE));      //uExtraNo->16
        /* set READ command */
        outpw(REG_SMCMD, 0x50);             // READ command 1 (0x50), read from extra area address (area C)
        outpw(REG_SMADDR, 0x00);
    }
    else
    {
        switch(pSM->nPageSize)
        {
            case 2048:
                outpw(REG_SMCSR, (inpw(REG_SMCSR) & ~SMCR_PSIZE)|PSIZE_2K); //uExtraNo->64
                /* set READ command */
                outpw(REG_SMCMD, 0x00);     // READ 1st cycle command
                outpw(REG_SMADDR, 0x00);
                outpw(REG_SMADDR, 0x08);
                break;
            case 4096:
                outpw(REG_SMCSR, (inpw(REG_SMCSR) & ~SMCR_PSIZE)|PSIZE_4K);
                /* set READ command */
                outpw(REG_SMCMD, 0x00);     // READ 1st cycle command
                outpw(REG_SMADDR, 0x00);
                outpw(REG_SMADDR, 0x10);
                break;
            case 8192:
                outpw(REG_SMCSR, (inpw(REG_SMCSR) & ~SMCR_PSIZE)|PSIZE_8K);
                /* set READ command */
                outpw(REG_SMCMD, 0x00);     // READ 1st cycle command
                outpw(REG_SMADDR, 0x00);
                outpw(REG_SMADDR, 0x20);
                break;
        }
    }

    outpw(REG_SMADDR, uPageIndex & 0xFF);

    if (!pSM->bIsMulticycle)
        outpw(REG_SMADDR, ((uPageIndex >> 8) & 0xFF) | EOA_SM); // PA8 - PA15
    else
    {
        outpw(REG_SMADDR, (uPageIndex >> 8) & 0xFF);                    // PA8 - PA15
        outpw(REG_SMADDR, ((uPageIndex >> 16) & 0xFF) | EOA_SM);    // PA16 - PA17
    }

    if(pSM->nPageSize != 512)
        outpw(REG_SMCMD, 0x30);         // READ 2nd cycle command

    if (!fmiSMCheckRB(pSM))
        return -1;

    uDAddr = (UINT8*) REG_SMRA_0;
    for (ii=0; ii<g_u32ExtraDataSize; ii++)
    {
        *(UINT8*) uDAddr = inpb(REG_SMDATA);
        uDAddr++;
    }
    return 0;
}


/* Read One Page */
int fmiSM_Read(FMI_SM_INFO_T *pSM, UINT32 uPageIndex,UINT32 uDAddr) //Added
{
    volatile UINT32 uStatus, uErrorCnt;
    volatile UINT32 uF1_status, uF2_status;
    volatile UINT8 ii, jj,EccLoop;

    outpw(REG_FMICR, inpw(REG_FMICR)|FMI_SM_EN);

    fmiSM_Reset(pSM);

    /* set Page Size and BCH */
    sicSMsetBCH(pSM, gbSystemImage);
    g_u32ExtraDataSize = inpw(REG_SMREAREA_CTL) & SMRE_REA128_EXT;    

    outpw(REG_SMCSR, inpw(REG_SMCSR) & ~SMCR_REDUN_REN);

    /* enable DMAC */
    while(inpw(REG_DMACCSR) & FMI_BUSY);    // wait DMAC FMI ready;
    outpw(REG_DMACCSR, inpw(REG_DMACCSR) | DMAC_EN);

    /* Set DMA Transfer Starting Address */
    outpw(REG_DMACSAR, uDAddr);

    outpw(REG_SMCMD, 0x00);     // READ 1st cycle command

    outpw(REG_SMADDR, 0);

    if(pSM->nPageSize != 512)
        outpw(REG_SMADDR, 0);

    outpw(REG_SMADDR, uPageIndex & 0xFF);

    if (!pSM->bIsMulticycle)
    {
        outpw(REG_SMADDR, ((uPageIndex >> 8) & 0xFF) | EOA_SM); // PA8 - PA15
    }
    else
    {
        outpw(REG_SMADDR, (uPageIndex >> 8) & 0xFF);                    // PA8 - PA15
        outpw(REG_SMADDR, ((uPageIndex >> 16) & 0xFF) | EOA_SM);    // PA16 - PA17
    }

    if(pSM->nPageSize != 512)
        outpw(REG_SMCMD, 0x30);     // READ 2nd cycle command

    if (!fmiSMCheckRB(pSM))
        return -1;

    uF1_status = 0;
    uF2_status = 0;
    uStatus = 0;
    /* begin DMA read transfer */
    outpw(REG_SMISR, SMISR_DMA_IF); // clear DMA flag
    outpw(REG_SMISR, inpw(REG_SMISR)| SMISR_DMA_IF| SMISR_ECC_FIELD_IF);    // clear DMA flag
    outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_DRD_EN);

    //--- uLoop is the number of SM_ECC_STx should be check.
    //      One SM_ECC_STx include ECC status for 4 fields.
    //      Field size is 1024 bytes for BCH_T24 and 512 bytes for other BCH.
    //switch (pSM->nPageSize)
    switch (inpw(REG_SMCSR) & SMCR_PSIZE)
    {
        case PSIZE_2K:
            EccLoop = 1;
            break;
        case PSIZE_4K:
            if (inpw(REG_SMCSR) & SMCR_BCH_TSEL == BCH_T24)
                EccLoop = 1;
            else
                EccLoop = 2;
            break;
        case PSIZE_8K:
            if (inpw(REG_SMCSR) & SMCR_BCH_TSEL == BCH_T24)
                EccLoop = 2;
            else
                EccLoop = 4;
            break;
        default:
            return -1;     // don't work for 512 bytes page
    }

    while(1)
    {
        if (inpw(REG_SMISR) & SMISR_ECC_FIELD_IF)
        {
            switch(pSM->nPageSize)
            {
                case 512:
                {
                    uF1_status = inpw(REG_SM_ECC_ST0);
                    uF1_status &= 0x3f;

                    if ((uF1_status & 0x03)==0x01)  // correctable error in 1st field
                    {
                        uErrorCnt = uF1_status >> 2;
                        fmiSM_CorrectData_BCH(1, uErrorCnt, (UINT8*)uDAddr);
                    }
                    else if (((uF1_status & 0x03)==0x02)
                          ||((uF1_status & 0x03)==0x03)) // uncorrectable error or ECC error in 1st field
                    {
                      uStatus = 1;
                      PRINTF("Page %d - Uncorrectable Error 0x%X\n",uPageIndex, inpw(REG_SM_ECC_ST0));
                    }
                    break;
                }
                case 2048:
                {
                    uF1_status = inpw(REG_SM_ECC_ST0);
                    for (ii=1; ii<5; ii++)
                    {
                        if ((uF1_status & 0x03)==0x01)  // correctable error in 1st field
                        {
                            uErrorCnt = uF1_status >> 2;
                            fmiSM_CorrectData_BCH(ii, uErrorCnt, (UINT8*)uDAddr);
                            break;
                        }
                        else if (((uF1_status & 0x03)==0x02)
                              ||((uF1_status & 0x03)==0x03)) // uncorrectable error or ECC error in 1st field
                        {
                            uStatus = 1;
                            PRINTF("Page %d - Uncorrectable Error 0x%X\n",uPageIndex,inpw(REG_SM_ECC_ST0));
                            break;
                        }
                        uF1_status >>= 8;
                    }
                    break;
                }
                case 4096:
                case 8192:
                {
                    for (jj=0; jj<EccLoop; jj++)
                    {
                        uF1_status = inpw(REG_SM_ECC_ST0+jj*4);
                        for (ii=1; ii<5; ii++)
                        {
                            if ((uF1_status & 0x03)==0x01)  // correctable error in 1st field
                            {
                                uErrorCnt = uF1_status >> 2;
                                fmiSM_CorrectData_BCH(ii+jj*4, uErrorCnt, (UINT8*)uDAddr);
                                //PRINTF("Warning: Field %d have %d BCH error. Corrected!!\n", jj*4+ii, uErrorCnt);
                                break;
                            }
                            else if (((uF1_status & 0x03)==0x02)
                                  ||((uF1_status & 0x03)==0x03)) // uncorrectable error or ECC error in 1st field
                            {
                                uStatus = 1;
                                PRINTF("Page %d - Uncorrectable Error 0x%X\n",uPageIndex, inpw(REG_SM_ECC_ST0));
                                break;
                            }
                            uF1_status >>= 8;
                        }
                    }
                }
                break;
            }
            outpw(REG_SMISR, SMISR_ECC_FIELD_IF);       // clear ECC_FLD_Error
        }

        if (inpw(REG_SMISR) & SMISR_DMA_IF)      // wait to finish DMAC transfer.
        {
            if ( !(inpw(REG_SMISR) & SMISR_ECC_FIELD_IF) )
                break;
        }
    }

    outpw(REG_SMISR, SMISR_DMA_IF);                 // clear DMA flag
    outpw(REG_SMISR, SMISR_ECC_FIELD_IF);       // clear ECC flag

    if (uStatus)
        return -1;

    return 0;
}


/* function pointer */
BOOL volatile bIsNandInit = FALSE;
FMI_SM_INFO_T *pNvtSM0, *pNvtSMInfo;

INT nvtSMInit(void)
{
    if (!bIsNandInit)
    {
        outpw(REG_DMACCSR, inpw(REG_DMACCSR) | DMAC_EN);
        // Reset DMAC
        outpw(REG_DMACCSR, inpw(REG_DMACCSR) | DMAC_SWRST);
        outpw(REG_DMACCSR, inpw(REG_DMACCSR) & ~DMAC_SWRST);
        // SM Enable
        outpw(REG_FMICR, FMI_SM_EN);

        /* set CLE/ALE setup and hold time width */
        outpw(REG_SMTCR, 0x10204);   // CLE/ALE=0x01, R/W_h=0x02, R/W_l=0x04
        outpw(REG_SMCSR, (inpw(REG_SMCSR) & ~SMCR_PSIZE) | PSIZE_512);
        outpw(REG_SMCSR, inpw(REG_SMCSR) |  SMCR_ECC_3B_PROTECT);
        outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_ECC_CHK);

        /* init SM interface */
        outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_REDUN_AUTO_WEN);

#if (TARGET_Chip_Select == 0)   // target CS0
        outpw(REG_GPDFUN, inpw(REG_GPDFUN) | 0x0003CC00);       // enable NAND NWR/NRD/RB0 pins
        outpw(REG_GPEFUN, inpw(REG_GPEFUN) | 0x00F30000);       // enable NAND ALE/CLE/CS0 pins
        outpw(REG_SMCSR, inpw(REG_SMCSR) & ~ SMCR_CS0);         // CS0 pin low, enable CS0
        outpw(REG_SMCSR, inpw(REG_SMCSR) |   SMCR_CS1);         // CS1 pin high, disable CS1
#else                           // target CS1
        outpw(REG_GPDFUN, inpw(REG_GPDFUN) | 0x0003F000);       // enable NAND NWR/NRD/RB1 pins 
        outpw(REG_GPEFUN, inpw(REG_GPEFUN) | 0x00FC0000);       // enable NAND ALE/CLE/CS1 pins     
        outpw(REG_SMCSR, inpw(REG_SMCSR) & ~ SMCR_CS1);         // CS1 pin low, enable CS1
        outpw(REG_SMCSR, inpw(REG_SMCSR) |   SMCR_CS0);         // CS0 pin high, disable CS0
#endif

        pNvtSMInfo = (FMI_SM_INFO_T *)malloc(sizeof(NVT_SM_INFO_T));
        memset((char *)pNvtSMInfo, 0, sizeof(NVT_SM_INFO_T));
#if (TARGET_Chip_Select == 0)   // target CS0
        pSM0= pNvtSMInfo;
        pNvtSM0 = pSM0;
#else
        pSM1= pNvtSMInfo;
        pNvtSM0 = pSM1;
#endif

        if (nvtSM_ReadID(pNvtSM0) < 0)
            return -1;
        nvtSM_Initial(pNvtSM0);

        pNvtSM0->uIBRBlock = fmiSM_GetIBRAreaSize(pNvtSM0);

        bIsNandInit = TRUE;
    }
    return 0;
}


INT nvtSMpread(INT PBA, INT page, UINT8 *buff)  //Modified
{
    FMI_SM_INFO_T *pSM;
    int pageNo;

    pSM = pNvtSM0;

    // enable SM
    outpw(REG_FMICR, FMI_SM_EN);
    if(PBA < pNvtSM0->uIBRBlock)
        gbSystemImage = TRUE;
    else
        gbSystemImage = FALSE;

    pageNo = PBA * pSM->uPagePerBlock + page;
    nvtSM_ReadOnePage_ExtraData(pSM, pageNo);
    return (fmiSM_Read(pSM, pageNo, (UINT32)buff));
}


INT nvtSMpwrite(INT PBA, INT page, UINT8 *buff) // Modified
{
    FMI_SM_INFO_T *pSM;
    int pageNo;

    pSM = pNvtSM0;

    // enable SM
    outpw(REG_FMICR, 0x08);

    if(PBA >= pNvtSM0->uIBRBlock)
        nIsSysImage = 0xFF;
    else
    {
        if (page == 0)
            gMarkCount = 0;
        else if (page > 32/* 10 */)
        {
            gMarkCount = 0;
            nIsSysImage = 0xFF;
        }
    }

    if(PBA < pNvtSM0->uIBRBlock)
        gbSystemImage = TRUE;
    else
        gbSystemImage = FALSE;

    pageNo = PBA * pSM->uPagePerBlock + page;

    if (pSM->nPageSize == NAND_PAGE_2KB)        /* 2KB */
        return (nvtSM_Write_2K(pSM, pageNo, 0, (UINT32)buff));
    else if (pSM->nPageSize == NAND_PAGE_4KB)   /* 4KB */
        return (nvtSM_Write_4K(pSM, pageNo, 0, (UINT32)buff));
    else if (pSM->nPageSize == NAND_PAGE_8KB)   /* 8KB */
        return (nvtSM_Write_8K(pSM, pageNo, 0, (UINT32)buff));
    else    /* 512B */
        return (nvtSM_Write_512(pSM, pageNo, 0, (UINT32)buff));
}


INT nvtSMchip_erase(UINT32 startBlcok, UINT32 endBlock) //Modified
{
    int i, status=0;
    FMI_SM_INFO_T *pSM;
    pInfo = (UINT8 *)((UINT32)infoBuf | 0x80000000);

    pSM = pNvtSM0;

    // enable SM
    outpw(REG_FMICR, FMI_SM_EN);

    // erase all chip
    for (i=startBlcok; i<=endBlock; i++)
    {
        status = nand_block_erase0(i);
        if (status < 0)
            PRINTF("SM block erase fail for block <%d>!! Return code = 0x%x\n", i, status);
    }
    return 0;
}


INT fmiMarkBadBlock(UINT32 block)
{
    UINT32 ucColAddr, uSector;

    uSector = block * pNvtSM0->uPagePerBlock;
    if (pNvtSM0->nPageSize != NAND_PAGE_512B)
    {
        // send command
        if (pNvtSM0->nPageSize == NAND_PAGE_2KB)
            ucColAddr = 2048;
        else if (pNvtSM0->nPageSize == NAND_PAGE_4KB)
            ucColAddr = 4096;
        else if (pNvtSM0->nPageSize == NAND_PAGE_8KB)
            ucColAddr = 8192;
        else
            return -1;

        outpw(REG_SMCMD, 0x80);     // serial data input command
        outpw(REG_SMADDR, ucColAddr);   // CA0 - CA7
        outpw(REG_SMADDR, (ucColAddr >> 8) & 0x1f);
        outpw(REG_SMADDR, uSector & 0xff);  // PA0 - PA7
        if (!pNvtSM0->bIsMulticycle)
            outpw(REG_SMADDR, ((uSector >> 8) & 0xff)|0x80000000);      // PA8 - PA15
        else
        {
            outpw(REG_SMADDR, (uSector >> 8) & 0xff);       // PA8 - PA15
            outpw(REG_SMADDR, ((uSector >> 16) & 0xff)|0x80000000);     // PA16 - PA17
        }
    }
    else
    {
        ucColAddr = 0;
        outpw(REG_SMCMD, 0x50);     // read RA command
        outpw(REG_SMCMD, 0x80);     // serial data input command
        outpw(REG_SMADDR, ucColAddr);   // CA0 - CA7
        outpw(REG_SMADDR, uSector & 0xff);  // PA0 - PA7
        if (!pNvtSM0->bIsMulticycle)
            outpw(REG_SMADDR, ((uSector >> 8) & 0xff)|0x80000000);      // PA8 - PA15
        else
        {
            outpw(REG_SMADDR, (uSector >> 8) & 0xff);       // PA8 - PA15
            outpw(REG_SMADDR, ((uSector >> 16) & 0xff)|0x80000000);     // PA16 - PA17
        }
    }

    /* mark data */
    outpw(REG_SMDATA, 0x00);
    outpw(REG_SMDATA, 0x00);
    outpw(REG_SMDATA, 0x00);
    outpw(REG_SMDATA, 0x00);

    outpw(REG_SMCMD, 0x10);     // auto program command

    if (!fmiSMCheckRB(pNvtSM0))
        return Fail;

    if (fmiSM_Reset(pNvtSM0) < 0)
        return -1;

    return Successful;
}


/* Check the 1th byte of spare */
INT CheckBadBlockMark(UINT32 block)
{
    pInfo = (UINT8 *)((UINT32)infoBuf | 0x80000000);

    if (nvtSMpread(block, 0, pInfo) < 0)
        return -1;

    if (inpw(REG_SMRA_0) == 0x0)
        return Fail;
    else
        return Successful;
}
