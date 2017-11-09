/****************************************************************************
*                                                                           *
* Copyright (c) 2009 Nuvoton Tech. Corp. All rights reserved.               *
*                                                                           *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wblib.h"
#include "usbd.h"
#include "mass_storage_class.h"
#include "w55fa95_sic.h"
#include "nvtfat.h"
#include "nvtloader.h"
#include "w55fa95_gnand.h"

/* Please modified the Variable For Your Need */
#ifdef __MSC_NAND0_NAND2__
	UINT32 u32NAND_EXPORT = (MSC_NAND_CS0 | MSC_NAND_CS2);		/*Mounting NAND device  on chip-select-0 and chip-select-2 */
#else
	UINT32 u32NAND_EXPORT = MSC_NAND_CS0;						/*Mounting NAND device  on chip-select-0 */
#endif
#ifdef __TWO_SD__
	UINT32 u32SD_EXPORT = (MSC_SD_MP_PORT0 | MSC_SD_MP_PORT1);	/*Mounting SD device  on port0 and port 1 */
#else
	#ifdef __GRIMM__
	UINT32 u32SD_EXPORT = MSC_SD_MP_PORT2;					/*Mounting SD device  on port2  */	
	#else
	UINT32 u32SD_EXPORT = MSC_SD_MP_PORT0;					/*Mounting SD device  on port0  */
	#endif
#endif



//#define DETECT_USBD_PLUG
#define PLUG_DEVICE_TEST

#ifndef DETECT_USBD_PLUG
BOOL bPlugStauts = FALSE;
BOOL bHostPlugStauts = FALSE;
#endif

#ifdef PLUG_DEVICE_TEST
UINT32 u32UsbTimerChannel = 0;		
BOOL bTimeOut = FALSE;
void Timer0_Callback(void)
{
	bTimeOut = TRUE;
	sysClearTimerEvent(TIMER0, u32UsbTimerChannel);
}
#endif

BOOL PlugDetection(VOID)
{
#ifdef DETECT_USBD_PLUG
	return udcIsAttached();
#else
	
	#ifdef PLUG_DEVICE_TEST

	if(udcIsAttached())
	{
		
		if(bPlugStauts != udcIsAttached())
		{
			bPlugStauts = TRUE;
			bHostPlugStauts = FALSE;
			sysprintf("<Plug>");				
		}	
		
		if(bHostPlugStauts != udcIsAttachedToHost())
		{
			bHostPlugStauts = udcIsAttachedToHost();
			if(bHostPlugStauts)
			{
				bTimeOut = TRUE;
				sysClearTimerEvent(TIMER0, u32UsbTimerChannel);
				sysprintf("<Host>\n");				
			}
		}
		if(bTimeOut)
		{
			if(bHostPlugStauts)
				return TRUE;
			else
			{
				sysprintf("<Adaptor>\n");	
				return FALSE;
			}
		}
		return TRUE;
	}
	else
	{				
		return FALSE;
	}
	#else
		return TRUE;
	#endif	
#endif
}
extern INT32 g_ibr_boot_sd_port;
extern INT32 g_i32SD0TotalSector, g_i32SD1TotalSector, g_i32SD2TotalSector; 
void mass(NDISK_T *disk0, NDISK_T *disk1, NDISK_T *disk2)
{
#if defined(__ENABLE_SD_CARD_0__)||defined(__ENABLE_SD_CARD_1__)||defined(__ENABLE_SD_CARD_2__)
	if(g_ibr_boot_sd_port == 0)
		u32SD_EXPORT |= MSC_SD_MP_PORT0;
	else if(g_ibr_boot_sd_port == 1)
		u32SD_EXPORT |= MSC_SD_MP_PORT1;
	else if(g_ibr_boot_sd_port == 2)
		u32SD_EXPORT |= MSC_SD_MP_PORT2;
#endif			
	mscdInit();		
	#ifdef __SD__
		mscdSdEnable(u32SD_EXPORT);	
	#endif
	
	#ifdef __SD_ONLY__
		mscdFlashInitExtend(NULL,NULL,NULL, g_i32SD0TotalSector,g_i32SD1TotalSector,g_i32SD2TotalSector,0);
	#else	
		#ifdef __NAND__	
			mscdNandEnable(u32NAND_EXPORT);			
			//mscdFlashInitExtend((NDISK_T *)disk0,(NDISK_T *)disk1,(NDISK_T *)disk2,g_i32SD0TotalSector,g_i32SD1TotalSector,g_i32SD2TotalSector,0);
			mscdFlashInitExtend((NDISK_T *)disk0, NULL, (NDISK_T *)disk2,g_i32SD0TotalSector,g_i32SD1TotalSector,g_i32SD2TotalSector,0);
			
		#endif		
	#endif	
	udcInit();
#ifdef PLUG_DEVICE_TEST	
	sysStartTimer(TIMER0, 
					100, 
					PERIODIC_MODE);
					
   	u32UsbTimerChannel = sysSetTimerEvent(TIMER0, 
    									1,
    									(PVOID)Timer0_Callback);		
	
	bTimeOut = FALSE;
	bPlugStauts = FALSE;
	bHostPlugStauts = FALSE;
	
   #ifdef __KLE_DEMO__
   	#ifdef __ENABLE_SD_CARD_0__	
	mscdSdUserWriteProtectPin(NULL,		/* Useless if No write protect pin */
							FALSE,	/* No write protect pin */
							NULL,	/* Useless if No write protect pin */
							NULL);	/* Useless if No write protect pin */
	#endif						
   #endif 		
   
	mscdMassEvent(PlugDetection);
#else	
	mscdMassEvent(PlugDetection);
#endif	
	udcDeinit();
	udcClose();		
}
