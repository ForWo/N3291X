/****************************************************************************
*                                                                           *
* Copyright (c) 2009 Nuvoton Tech. Corp. All rights reserved.               *
*                                                                           *
*****************************************************************************/
#include "w55fa95_gnand.h"

#define KERNEL_PATH_SD 	"x:\\conprog.bin"
#define MOVIE_PATH_SD 		"x:\\movie.avi"
#define MP3_PATH_SD 		"x:\\audio.mp3"

#define FIRMWARE_PATH_SD 	"x:\\SDBoot.bin"

#define KERNEL_PATH 		"c:\\conprog.bin"
#define MOVIE_PATH 		"c:\\movie.avi"
#define MP3_PATH 			"c:\\audio.mp3"

#if defined(__ENABLE_SD_CARD_0__)|| defined(__ENABLE_SD_CARD_1__)||defined(__ENABLE_SD_CARD_2__)
#define VOLUME_PATH		"x:\\volume.cfg"
#define SATURATION_PATH	"x:\\saturation.cfg"
#else
#define VOLUME_PATH		"c:\\volume.cfg"
#define SATURATION_PATH	"c:\\saturation.cfg"
#endif						

#define CP_SIZE 16 * 1024


/* PLL clock option */
//#define __CLK_CTL__			/* Clock change in NVT-Loader */ 
#ifdef __CLK_CTL__
	/* Clock Skew */ 
	#define E_CLKSKEW	0x00888800
		
	/* DDR timing option */
	#define __DDR_6__
	//#define __DDR_75__
	
	/* PLL setting */
	#define __UPLL_240__
	//#define __UPLL_192__
	//#define __UPLL_288__
#endif

/* Start for option for VPOST frame buffer */
#if defined(__TV__)
	#ifdef __TV_QVGA__ 
	#define PANEL_WIDTH		320
	#define PANEL_HEIGHT		240
	#else
	#define PANEL_WIDTH		640
	#define PANEL_HEIGHT		480
	#endif
#elif defined( __LCM_800x600__) 
	#define PANEL_WIDTH		800
	#define PANEL_HEIGHT		600
#elif defined( __LCM_480x272__)
	#define PANEL_WIDTH		480
	#define PANEL_HEIGHT		272
#elif defined( __LCM_320x480__)
	#define PANEL_WIDTH		320
	#define PANEL_HEIGHT		480	
#elif defined( __LCM_800x480__)
	#define PANEL_WIDTH		800
	#define PANEL_HEIGHT		480
#elif defined( __LCM_VGA__)
	#define PANEL_WIDTH		640
	#define PANEL_HEIGHT		480	
#elif defined( __LCM_QVGA__)
	#define PANEL_WIDTH		320
	#define PANEL_HEIGHT		240
#elif defined( __LCM_128x64__)
	#define PANEL_WIDTH		128
	#define PANEL_HEIGHT		64	
#else 	
	#define PANEL_WIDTH		480
	#define PANEL_HEIGHT		272
#endif

/* Defined For Key Matrix And Low Battery Option */
#if defined(__NVT_DEMO__)
	#define HOME_KEY			(16)
	#define UP_KEY				(1)
	#define DOWN_KEY			(2)
	#define MASS_STORAGE		(UP_KEY+DOWN_KEY)
	#define LOW_BATTERY_LEVEL	(3.5)
	/* NAND1-1 Size */
	#define NAND1_1_SIZE	 32	/* MB unit */
	#define SD1_1_SIZE	 	128  /* MB unit */ 
	#define NAND2_1_SIZE	1024 /* MB unit */

#elif defined(__NVT_MP_DEMO__)
  #ifdef __PICO_PROJECTOR__
	#define HOME_KEY			(16)
	#define __LONG_POWER_KEY_CHECK__
	#define __LONG_POWER_KEY_TIME_		(300)
	#define VOL_UP			(4)
	#define VOL_DOWN			(8)
	#define LONG_POWER		(VOL_UP+VOL_DOWN)	
  #else
  	#define HOME_KEY			(32)
  #endif	
	#define UP_KEY				(1)
	#define DOWN_KEY			(2)
	#define MASS_STORAGE		(UP_KEY+DOWN_KEY)
	#define LOW_BATTERY_LEVEL	(3.5)
	/* NAND1-1 Size */
	#define NAND1_1_SIZE	 32	/* MB unit */
	#define SD1_1_SIZE	 	128  /* MB unit */ 
	#define NAND2_1_SIZE	1024 /* MB unit */
#elif defined(__NVT_MP_WVGA_DEMO__)
	#define HOME_KEY			(32)
	#define UP_KEY				(1)
	#define DOWN_KEY			(2)
	#define LEFT_KEY			(4)
	#define RIGHT_KEY			(8)	
	#define ENTER_KEY			(16)			
	#define VOL_UP			(64)
	#define VOL_DOWN			(128)
	#define MASS_STORAGE		(VOL_UP+VOL_DOWN)		
	#define LOW_BATTERY_LEVEL	(3.6)
	/* NAND1-1 Size */
	#define NAND1_1_SIZE	 32	/* MB unit */
	#define SD1_1_SIZE	 	128  /* MB unit */ 
	#define NAND2_1_SIZE	1024 /* MB unit */	
#elif defined(__NVT_MP_WVGA_IN_DEMO__)
	#define HOME_KEY			(32)
	#define UP_KEY				(1)
	#define DOWN_KEY			(2)
	#define MASS_STORAGE		(UP_KEY+DOWN_KEY)
	#define LOW_BATTERY_LEVEL	(3.6)
	/* NAND1-1 Size */
	#define NAND1_1_SIZE	 32	/* MB unit */
	#define SD1_1_SIZE	 	128  /* MB unit */ 
	#define NAND2_1_SIZE	1024 /* MB unit */	
#elif defined( __KLE_DEMO__)
	#define HOME_KEY			(1)
	#define VOL_UP			(2)
	#define VOL_DOWN			(4)
	#define CAMERA			(8)
	#define MASS_STORAGE		(VOL_UP+VOL_DOWN)
	#define __BAT_DECT__	
	#define __OPT_SPEAKER_DECADE_1DB__
	#define LOW_BATTERY_LEVEL	(3.5)
	/* NAND1-1 Size */
	#define NAND1_1_SIZE	512/* MB unit */
	#define SD1_1_SIZE	 	1024 /* MB unit */ 
	#define NAND2_1_SIZE	512 /* MB unit */	
	#define __NAND_POST_PROCESS__ 
#elif defined( __CWC_DEMO__)
	#define HOME_KEY			(1)  /* S2 */
	#define VOL_UP			(2)  /* S3 */
	#define VOL_DOWN			(4)  /* S4 */
	#define CAMERA			(8)  /* S5 */
	#define MASS_STORAGE		(VOL_UP+VOL_DOWN)	/* S3+S4 */
	#define LOW_BATTERY_LEVEL	(3.6)
	/* NAND1-1 Size */
	#define NAND1_1_SIZE	 32	/* MB unit */
	#define SD1_1_SIZE	 	128  /* MB unit */ 
	#define NAND2_1_SIZE	1024 /* MB unit */	
#elif defined( __LCM_800x600__)
	#ifdef __GRIMM__
		#define HOME_KEY			(4)  /* Test only */
		#define VOL_UP			(2)  /* Test only */
		#define VOL_DOWN			(1)  /* Test only */
		#define __LONG_POWER_KEY_CHECK__
		#define __LONG_POWER_KEY_TIME_		(300)	
		#define LONG_POWER		(VOL_UP+VOL_DOWN)	
	#else
		#define HOME_KEY			(1)  /* Test only */
		#define VOL_UP			(2)  /* Test only */
		#define VOL_DOWN			(4)  /* Test only */
	#endif 	
	#define CAMERA			(8)  /* Test only */
	#define MASS_STORAGE		(VOL_UP+VOL_DOWN) /* Test only */
	#define LOW_BATTERY_LEVEL	(3.6)
	/* NAND1-1 Size */
	#define NAND1_1_SIZE	 32	/* MB unit */
	#define SD1_1_SIZE	 	128  /* MB unit */ 
	#define NAND2_1_SIZE	1024 /* MB unit */	
#elif defined( __SHBST_DEMO__)	
	#define VOL_UP			(1) 
	#define VOL_DOWN			(2)
	#define SET1				(4) 
	 #define HOME_KEY			(SET1) 
	#define T1				(8) 
	#define MASS_STORAGE		(VOL_UP+VOL_DOWN) /* Test only */
	#define LOW_BATTERY_LEVEL	(3.6)
	/* NAND1-1 Size */
	#define NAND1_1_SIZE	 32	/* MB unit */
	#define SD1_1_SIZE	 	128  /* MB unit */ 
	#define NAND2_1_SIZE	1024 /* MB unit */		
#endif

/* Defined For Mass Storage Option */
#ifdef __ENABLE_SD_CARD_0__
#define __SD_ONLY__
#define __SD__
#endif

#if defined(__ENABLE_NAND_0__) || defined(__ENABLE_NAND_1__) || defined(__ENABLE_NAND_2__)
#define __NAND__
#endif


/* End for option for VPOST frame buffer */

#define PANEL_BPP		2
#define FB_ADDR		0x500000



#ifdef __DEBUG__
#define DBG_PRINTF		sysprintf
#else
#define DBG_PRINTF(...)		
#endif
 
 
 /* Turn on the optional. Back light enable */
 /* Turn off the optional, ICE can connect to */
 /* Default Demo Board  GPD1 keep pull high */
 /* 								*/ 
//#define __BACKLIGHT__

void SpeakerInit(void);
void SpeakerEnable(void);
void SpeakerDisable(void);
void EarphoneInit(void);
BOOL EarphoneDetect(void);
void MuteInit(void);
void MuteEnable(void);
void MuteDisable(void);
void AdaptorDetInit(void);
void AdaptorDetHigh(void);
void AdaptorDetLow(void);

void LcmPowerInit(void);
void LcmPowerEnable(void);
void LcmBacklightInit(void);
void LcmBacklightEnable(void);
void LcmBacklightDisable(void);

void ChargerIndicatotInit(void);
BOOL  ChargerDetect(void);
void LcmSaturationInc(UINT32 u32Value);
void LcmSaturationInit(void);
void SaturationConfigFile(void);

void BatteryDetection(BOOL bIsExtraPower);

void NVT_Updater(void);

void I2cInit(void);
void I2cSendStart(void);
void I2cSendStop(void);

#if 0
void mass(NDISK_T *disk, INT32 i32TotalSector);
void mass_NandDevice2(NDISK_T *disk, INT32 i32TotalSector, NDISK_T *disk2, INT32 i32TotalSector2);
#else
void mass(NDISK_T *disk0, NDISK_T *disk1, NDISK_T *disk2);
void mass_NandDevice2(NDISK_T *disk, INT32 i32TotalSector, NDISK_T *disk2, INT32 i32TotalSector2);
#endif
