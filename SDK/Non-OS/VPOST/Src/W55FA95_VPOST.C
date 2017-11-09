/***************************************************************************
 *                                                                         *
 * Copyright (c) 2007 - 2009 Nuvoton Technology Corp. All rights reserved.*
 *                                                                         *
 ***************************************************************************/

/****************************************************************************
 * 
 * FILENAME
 *     W55FA95_VPOST.c
 *
 * VERSION
 *     0.1 
 *
 * DESCRIPTION
 *
 *
 *
 *
 * DATA STRUCTURES
 *     None
 *
 * FUNCTIONS
 *
 *
 *     
 * HISTORY
 *
 *
 * REMARK
 *     None
 *
 *
 **************************************************************************/
#include "wblib.h"
#include "w55FA95_vpost.h"
#include "w55FA95_reg.h"



#include <stdio.h>

VOID * g_VAFrameBuf = NULL;
VOID * g_VAOrigFrameBuf = NULL;

INT32 vpostLCMInit(PLCDFORMATEX plcdformatex, UINT32 *pFramebuf)
{

#ifdef HAVE_SHARP_LQ035Q1DH02
	return vpostLCMInit_SHARP_LQ035Q1DH02(plcdformatex, pFramebuf);
#endif

#ifdef HAVE_WINTEK_WMF3324
	return vpostLCMInit_WINTEK_WMF3324(plcdformatex, pFramebuf);
#endif

#ifdef HAVE_HANNSTAR_HSD043I9W1
	return vpostLCMInit_HANNSTAR_HSD043I9W1(plcdformatex, pFramebuf);
#endif

#ifdef HAVE_GOWORLD_GWMTF9406A
	return vpostLCMInit_GOWORLD_GWMTF9406A(plcdformatex, pFramebuf);
#endif

#ifdef HAVE_GOWORLD_GWMTF9360A
	return vpostLCMInit_GOWORLD_GWMTF9360A(plcdformatex, pFramebuf);
#endif

#ifdef HAVE_GOWORLD_GWMTF9615A
	return vpostLCMInit_GOWORLD_GWMTF9615A(plcdformatex, pFramebuf);
#endif
#ifdef HAVE_GOWORLD_GWMTF9360A_MODIFY
	return vpostLCMInit_GOWORLD_GWMTF9360A_modify(plcdformatex, pFramebuf);
#endif

#ifdef HAVE_GOWORLD_GW8973
	return vpostLCMInit_GOWORLD_GW8973(plcdformatex, pFramebuf);
#endif

#ifdef HAVE_HIMAX_HX7033
	return vpostLCMInit_HIMAX_HX7033(plcdformatex, pFramebuf);
#endif

#ifdef HAVE_HIMAX_HX7027
	return vpostLCMInit_HIMAX_HX7027(plcdformatex, pFramebuf);
#endif

#ifdef HAVE_VG680
	return vpostLCMInit_VG680(plcdformatex, pFramebuf);
#endif

#ifdef HAVE_TVOUT_720x480
	return vpostLCMInit_TVOUT_720x480(plcdformatex, pFramebuf);
#endif

#ifdef HAVE_TVOUT_640x480
	return vpostLCMInit_TVOUT_640x480(plcdformatex, pFramebuf);
#endif

#ifdef HAVE_TVOUT_320x240
	return vpostLCMInit_TVOUT_320x240(plcdformatex, pFramebuf);
#endif

#ifdef HAVE_AMPIRE_800x600
	return vpostLCMInit_AMPIRE_800x600(plcdformatex, pFramebuf);
#endif

#ifdef HAVE_AMPIRE_800x600_18B
	return vpostLCMInit_AMPIRE_800x600_18B(plcdformatex, pFramebuf);
#endif

#ifdef HAVE_AMPIRE_800x600_24B
	return vpostLCMInit_AMPIRE_800x600_24B(plcdformatex, pFramebuf);
#endif

#ifdef HAVE_HANNSTAR_HSD070IDW1
	return vpostLCMInit_HANNSTAR_HSD070IDW1(plcdformatex, pFramebuf);
#endif

#ifdef HAVE_AMPIRE_800x480
	return vpostLCMInit_AMPIRE_800x480(plcdformatex, pFramebuf);
#endif

#ifdef HAVE_AMPIRE_800x480_18B
	return vpostLCMInit_AMPIRE_800x480_18B(plcdformatex, pFramebuf);
#endif

#ifdef HAVE_KD070D10_800x480
	return vpostLCMInit_KD070D10_800x480(plcdformatex, pFramebuf);
#endif

#ifdef HAVE_E50V2A1_800x480
	return vpostLCMInit_E50V2A1_800x480(plcdformatex, pFramebuf);
#endif

#ifdef HAVE_HIMAX_HX8346
	return vpostLCMInit_HIMAX_HX8346(plcdformatex, pFramebuf);
#endif

// Add for GW9563 5-inch 24-bit
#ifdef HAVE_GOWORLD_GWTFM9563B
	return vpostLCMInit_GOWORLD_GWTFM9563B(plcdformatex, pFramebuf);
#endif

// Add for GW9563 5-inch 24-bit
#ifdef HAVE_SGMICRO_SGM3727
	return vpostLCMInit_SGMICRO_SGM3727(plcdformatex, pFramebuf);
#endif

#ifdef HAVE_SPLC780D
	return vpostLCMInit_SPLC780D(plcdformatex, pFramebuf);
#endif

#ifdef HAVE_ILITEK_ILI9322
	return vpostLCMInit_ILITEK_ILI9322(plcdformatex, pFramebuf);
#endif

#ifdef HAVE_ILITEK_ILI9486
	return vpostLCMInit_ILITEK_ILI9486(plcdformatex, pFramebuf);
#endif

#ifdef HAVE_ILITEK_ILI9488
	return vpostLCMInit_ILITEK_ILI9488(plcdformatex, pFramebuf);
#endif

#ifdef HAVE_TOPPLY_TD025THEA
	return vpostLCMInit_TOPPLY_TD025THEA(plcdformatex, pFramebuf);
#endif

#ifdef HAVE_ILITEK_ILI6150_24B
	return vpostLCMInit_ILITEK_ILI6150_24B(plcdformatex, pFramebuf);
#endif

#ifdef HAVE_INNOLUX_800x480_18B
	return vpostLCMInit_INNOLUX_800x480_18B(plcdformatex, pFramebuf);
#endif

#ifdef HAVE_ILITEK_ILI9488_MPU
	return vpostLCMInit_ILITEK_ILI9488_MPU(plcdformatex, pFramebuf);
#endif

#ifdef HAVE_ILITEK_ILI9341_MPU
	return vpostLCMInit_ILITEK_ILI9341_MPU(plcdformatex, pFramebuf);
#endif


}

INT32 vpostLCMDeinit(void)
{
#ifdef HAVE_SHARP_LQ035Q1DH02
	return vpostLCMDeinit_SHARP_LQ035Q1DH02();
#endif

#ifdef HAVE_WINTEK_WMF3324
	return vpostLCMDeinit_WINTEK_WMF3324();
#endif

#ifdef HAVE_HANNSTAR_HSD043I9W1
	return vpostLCMDeinit_HANNSTAR_HSD043I9W1();
#endif

#ifdef HAVE_GOWORLD_GW8973
	return vpostLCMDeinit_GOWORLD_GW8973();
#endif

#ifdef HAVE_HIMAX_HX7033
	return vpostLCMDeinit_HIMAX_HX7033();
#endif

#ifdef HAVE_HIMAX_HX7027
	return vpostLCMDeinit_HIMAX_HX7027();
#endif

#ifdef HAVE_GOWORLD_GWMTF9406A
	return vpostLCMDeinit_GOWORLD_GWMTF9406A();
#endif

#ifdef HAVE_GOWORLD_GWMTF9360A
	return vpostLCMDeinit_GOWORLD_GWMTF9360A();
#endif

#ifdef HAVE_GOWORLD_GWMTF9615A
	return vpostLCMDeinit_GOWORLD_GWMTF9615A();
#endif
#ifdef HAVE_GOWORLD_GWMTF9360A_MODIFY
	return vpostLCMDeinit_GOWORLD_GWMTF9360A_modify();
#endif

#ifdef HAVE_VG680
	return vpostLCMDeinit_VG680();
#endif

#ifdef HAVE_TVOUT_720x480
	return vpostLCMDeinit_TVOUT_720x480();
#endif

#ifdef HAVE_TVOUT_640x480
	return vpostLCMDeinit_TVOUT_640x480();
#endif

#ifdef HAVE_TVOUT_320x240
	return vpostLCMDeinit_TVOUT_320x240();
#endif

#ifdef HAVE_AMPIRE_800x600
	return vpostLCMDeinit_AMPIRE_800x600();
#endif

#ifdef HAVE_AMPIRE_800x600_18B
	return vpostLCMDeinit_AMPIRE_800x600_18B();
#endif

#ifdef HAVE_AMPIRE_800x600_24B
	return vpostLCMDeinit_AMPIRE_800x600_24B();
#endif

#ifdef HAVE_AMPIRE_800x480
	return vpostLCMDeinit_AMPIRE_800x480();
#endif

#ifdef HAVE_AMPIRE_800x480_18B
	return vpostLCMDeinit_AMPIRE_800x480_18B();
#endif

#ifdef HAVE_KD070D10_800x480
	return vpostLCMDeinit_KD070D10_800x480();
#endif

#ifdef HAVE_E50V2A1_800x480
	return vpostLCMDeinit_E50V2A1_800x480();
#endif

#ifdef HAVE_HANNSTAR_HSD070IDW1
	return vpostLCMDeinit_HANNSTAR_HSD070IDW1();
#endif

#ifdef HAVE_HIMAX_HX8346
	return vpostLCMDeinit_HIMAX_HX8346();
#endif

// Add for GW9563 5-inch 24-bit
#ifdef HAVE_GOWORLD_GWTFM9563B
	return vpostLCMDeinit_GOWORLD_GWTFM9563B();
#endif

#ifdef HAVE_SGMICRO_SGM3727
	return vpostLCMDeinit_SGMICRO_SGM3727();
#endif

#ifdef HAVE_SPLC780D
	return vpostLCMDeinit_SPLC780D();
#endif

#ifdef HAVE_ILITEK_ILI9322
	return vpostLCMDeinit_ILITEK_ILI9322();
#endif

#ifdef HAVE_ILITEK_ILI9486
	return vpostLCMDeinit_ILITEK_ILI9486();
#endif

#ifdef HAVE_ILITEK_ILI9488
	return vpostLCMDeinit_ILITEK_ILI9488();
#endif

#ifdef HAVE_ILITEK_ILI9488_MPU
	return vpostLCMDeinit_ILITEK_ILI9488_MPU();
#endif

#ifdef HAVE_TOPPLY_TD025THEA
	return vpostLCMDeinit_TOPPLY_TD025THEA();
#endif

#ifdef HAVE_ILITEK_ILI6150_24B
	return vpostLCMDeinit_ILITEK_ILI6150_24B();
#endif

#ifdef HAVE_INNOLUX_800x480_18B
	return vpostLCMDeinit_INNOLUX_800x480_18B();
#endif

#ifdef HAVE_ILITEK_ILI9341_MPU
	return vpostLCMDeinit_ILITEK_ILI9341_MPU();
#endif

}

VOID* vpostGetFrameBuffer(void)
{
    return g_VAFrameBuf;
}

VOID vpostSetFrameBuffer(UINT32 pFramebuf)
{ 
	g_VAFrameBuf = (VOID *)pFramebuf;
	g_VAFrameBuf = (VOID*)((UINT32)g_VAFrameBuf | 0x80000000);
    outpw(REG_LCM_FSADDR, (UINT32)pFramebuf);
}


void LCDDelay(unsigned int nCount)
{
	unsigned volatile int i;
		
	for(;nCount!=0;nCount--)
//		for(i=0;i<100;i++);
		for(i=0;i<10;i++);
}
