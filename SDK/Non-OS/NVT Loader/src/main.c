/****************************************************************************
*                                                                           *
* Copyright (c) 2009 Nuvoton Tech. Corp. All rights reserved.               *
*                                                                           *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wblib.h"
#include "w55fa95_sic.h"
#include "w55fa95_gnand.h"
#include "nvtfat.h"
#include "nvtloader.h"
#include "w55fa95_gpio.h"
#include "w55fa95_kpi.h"
//#include "AviLib.h"
#include "w55fa95_reg.h"
#include "w55fa95_vpost.h"
#include "w55fa95_adc.h"
#include "RTC.H"
#include "usbd.h"
#include "mass_storage_class.h"
#include "spu.h"
#include "AviLib.h"
#include "w55fa95_i2c.h"
#include "nvtloader.h"

#ifdef USB_HOST
#include "usb.h"
#endif

#ifdef __MASS_PARODUCT__
void	EMU_MassProduction(void);
#endif

extern void loadKernelCont(int fd, int offset);
extern void playAni(int fd, char* pcString);
extern void playAudio(int kfd, char* pcString);
//extern void mass(NDISK_T *disk);
#if 0
void mass(NDISK_T *disk, INT32 i32TotalSector);
#else
void mass(NDISK_T *disk0, NDISK_T *disk1, NDISK_T *disk2);
#endif
extern void lcmFill2Dark(unsigned char*);
extern void initVPostShowLogo(void);
unsigned char kbuf[CP_SIZE]; /* save first 16k of buffer. Copy to 0 after vector table is no longer needed */
extern void AudioChanelControl(void);
extern void backLightEnable(void);
extern VOID RTC_Check(VOID);

int g_ibr_boot_sd_port;     // indicate the SD port number which IBR boot from.

int kfd, mfd;
BOOL bIsIceMode = FALSE;
UINT32 u32UPll_clock = 0;
static NDRV_T _nandDiskDriver0 =
{
    nandInit0,
    nandpread0,
    nandpwrite0,
    nand_is_page_dirty0,
    nand_is_valid_block0,
    nand_ioctl,
    nand_block_erase0,
    nand_chip_erase0,
    0
};
static NDISK_T ptNDisk;

INT32 g_i32SD0TotalSector=0, g_i32SD1TotalSector=0, g_i32SD2TotalSector=0;

#ifdef __ENABLE_NAND_1__	/* Only definition, Not Using */
NDRV_T _nandDiskDriver1 =
{
    nandInit1,
    nandpread1,
    nandpwrite1,
    nand_is_page_dirty1,
    nand_is_valid_block1,
    nand_ioctl,
    nand_block_erase1,
    nand_chip_erase1,
    0
};
static NDISK_T ptNDisk1;
#else
NDISK_T ptNDisk1 = NULL;
#endif

#ifdef __ENABLE_NAND_2__
NDRV_T _nandDiskDriver2 =
{
    nandInit2,
    nandpread2,
    nandpwrite2,
    nand_is_page_dirty2,
    nand_is_valid_block2,
    nand_ioctl,
    nand_block_erase2,
    nand_chip_erase2,
    0
};
static NDISK_T ptNDisk2;
#else
NDISK_T ptNDisk2 = NULL;
#endif


#if 1
extern UINT16 u16Volume;
/* Volume config file locate in NAND disk */
void VolumeConfigFile(void)
{
	INT8 path[64];
	/* Check if volume config file exists */
	fsAsciiToUnicode(VOLUME_PATH, path, TRUE);
	kfd = fsOpenFile(path, 0, O_RDONLY);
	if(kfd > 0)
	{
		INT32		nStatus, nLen;
		UINT8  		u8VolCfg[2];
		nStatus = fsReadFile(kfd, u8VolCfg, 2, &nLen);
		if(nStatus>=0)
		{
			u16Volume = u8VolCfg[0]|(((UINT16)u8VolCfg[1])<<8);
			//u16Volume = u16Volume * 31/100;
			sysprintf("Volume = %d\n", u16Volume);
		}
		fsCloseFile(kfd);
	}
}
#endif

#if 1
extern UINT32 u32Saturation;
/* Saturation config file locate in NAND disk */
void SaturationConfigFile(void)
{
	INT8 path[64];
	/* Check if saturation config file exists */
	fsAsciiToUnicode(SATURATION_PATH, path, TRUE);
	kfd = fsOpenFile(path, 0, O_RDONLY);
	if(kfd > 0)
	{
		INT32		nStatus, nLen;
		UINT8  		u8VolCfg[2];
		nStatus = fsReadFile(kfd, u8VolCfg, 2, &nLen);
		if(nStatus>=0)
		{
			u32Saturation = u8VolCfg[0];
			sysprintf("Saturation = %d\n", u32Saturation);
		}
		fsCloseFile(kfd);
	}
	u32Saturation = u32Saturation*63/100;
}
#endif

extern UINT32 u32EarphoneDetChannel;	/* For volume adjust */
extern void Timer0_200msCallback(void);

UINT32 u32TimerChannel = 0;			/* For back light */
void Timer0_300msCallback(void)
{
	LcmBacklightInit();
	LcmBacklightEnable();
	DBG_PRINTF("T300ms callback\n");
	sysClearTimerEvent(TIMER0, u32TimerChannel);
}


// Detect Earphone plug in?
#ifdef __LONG_POWER_KEY_CHECK__
UINT32 u32LongPowerKey= 0;
UINT32 u32LongPowerKeyFlag=0;
UINT32 u32LongPowerKeyChannel;			/* For Long power key power on */
UINT32 u32BackLightStep = 0;
void Timer0_LongPowerKey(void)
{
	u32LongPowerKey = u32LongPowerKey+1; 
	switch(u32BackLightStep)
	{
		case 0:					
		if(u32LongPowerKey==100)
		{//End of T1 ==> 1 sec.
			LcmBacklightInit();
			u32BackLightStep = 1;
			sysprintf("Back light T1 end\n");
		}	
		break;
		case 1:
		if(u32LongPowerKey>__LONG_POWER_KEY_TIME_)
		{//Long power key done
			u32LongPowerKeyFlag = 1;
			sysClearTimerEvent(TIMER0, u32LongPowerKeyChannel);	
		}
		break;	
	}
}
#endif

//#define __CC__

extern void ForceShutdown(void);
void init(void)
{
	WB_UART_T 	uart;
	UINT32 		u32ExtFreq;
	UINT32 u32Cke = inp32(REG_AHBCLK);
	
	/* RTC has been open in NAND/SD Loader	*/
	/* RTC_Ioctl(0,RTC_IOC_SET_POWER_KEY_DELAY, 7, 0); */

	SpeakerInit();
	EarphoneInit();

	AdaptorDetInit();
	AdaptorDetLow(); 	//AdaptorDetHigh(); Change ADAPTER_DET initial value in 6/20.

	/* Reset SIC engine to fix USB update kernel and mvoie file */
	outp32(REG_AHBCLK, u32Cke  | (SIC_CKE | NAND_CKE | SD_CKE | USBD_CKE));
	outp32(REG_AHBIPRST, inp32(REG_AHBIPRST )|(SICRST |  UDCRST));
	outp32(REG_AHBIPRST, 0);
	outp32(REG_APBCLK, inp32(REG_APBCLK) | RTC_CKE);
	outp32(REG_APBIPRST, TMR0RST | TMR1RST);
	outp32(REG_APBIPRST, 0);
	
#ifdef __LONG_POWER_KEY_CHECK__		/* Disable RTC hardware power off procedure, Linux will restart it */ 
	RTC_Check();
	outp32(PWRON, (inp32(PWRON) & ~0x04));		                        
 	RTC_Check();
 #endif
 	
	sysprintf("PWRON =  0x%x\n", inp32(PWRON));
	outp32(REG_AHBCLK,u32Cke);
	sysEnableCache(CACHE_WRITE_BACK);

	/* init timer */
	u32ExtFreq = sysGetExternalClock();    	/* Hz unit */
	sysSetTimerReferenceClock (TIMER0,
								u32ExtFreq);	/* Hz unit */
	sysStartTimer(TIMER0,
					100,
					PERIODIC_MODE);
					
#ifndef __CC__
   	kpi_init();
	kpi_open(0); // use nIRQ0 as external interrupt source
	bIsIceMode = kpi_read(KPI_NONBLOCK);
	if(bIsIceMode!=FALSE)
		bIsIceMode=TRUE;
#endif
	sysSetLocalInterrupt(ENABLE_IRQ);
						
#ifdef __LONG_POWER_KEY_CHECK__
    	if(inp32(TTR)==0)
	{//It means the system was powered off normally. So need to check long power key. 	
		u32LongPowerKeyChannel = sysSetTimerEvent(TIMER0,
    									1,
    									(PVOID)Timer0_LongPowerKey);
		while(u32LongPowerKeyFlag != 1)
		{
			UINT32 u32KpiReport = kpi_read(KPI_NONBLOCK) & LONG_POWER;
			sysprintf("u32KpiReport = 0x%x\n", u32KpiReport);
			if((u32KpiReport == LONG_POWER) && ( (inp32(PWRON) & PWR_KEY) != PWR_KEY) ) /* If the power key pressing, the PWR_KEY bit =0 */
			{		
				if((u32LongPowerKey%10)==0)
					sysprintf("Pressing power on key \n");
			}
			else
			{
///				LcmBacklightInit();
				ForceShutdown();
			}
		}
	}
	else
	{
		LcmBacklightInit();
	}
///	outp32(TTR, 0x7);	
#else
    	u32TimerChannel = sysSetTimerEvent(TIMER0,
    									30,
    									(PVOID)Timer0_300msCallback);
#endif
    	u32EarphoneDetChannel = sysSetTimerEvent(TIMER0,
    									20,
    									(PVOID)Timer0_200msCallback);

    	/* enable UART */
    	sysUartPort(1);
	uart.uiFreq = u32ExtFreq;					/* Hz unit */
	uart.uiBaudrate = 115200;
	uart.uiDataBits = WB_DATA_BITS_8;
	uart.uiStopBits = WB_STOP_BITS_1;
	uart.uiParity = WB_PARITY_NONE;
	uart.uiRxTriggerLevel = LEVEL_1_BYTE;
	sysInitializeUART(&uart);
	sysprintf("NVT Loader start\n");

	ChargerIndicatotInit();

	MuteEnable();		/* Mute fisrt for speaker depop */
	MuteInit();		/* Play AVI will disable mute, and enable mute before jump to kernel */
	
#ifdef __BAT_DECT__
	adc_open(NULL, 320, 240);
#endif
}


typedef struct sd_info
{
	unsigned int startSector;
	unsigned int endSector;
	unsigned int fileLen;
	unsigned int executeAddr;
}NVT_SD_INFO_T;

UINT8 dummy_buffer[512];
unsigned char *buf;
unsigned int *pImageList;

/* read image information */
NVT_SD_INFO_T image;
UINT32 ParsingReservedArea(void)
{
	UINT32 u32ReservedSector=0;

	int count, i;

	buf = (UINT8 *)((UINT32)dummy_buffer | 0x80000000);
	pImageList=((unsigned int *)(((unsigned int)dummy_buffer)|0x80000000));

    if (g_ibr_boot_sd_port == 0)
    	sicSdRead0(33, 1, (UINT32)dummy_buffer);
    else if (g_ibr_boot_sd_port == 1)
	    sicSdRead1(33, 1, (UINT32)dummy_buffer);
    else if (g_ibr_boot_sd_port == 2)
	    sicSdRead2(33, 1, (UINT32)dummy_buffer);

	pImageList=((unsigned int *)(((unsigned int)dummy_buffer)|0x80000000));
	if( (*(pImageList+2)) !=0xFFFFFFFF)
	{
		sysprintf("Turbo writter reserved area %dKB\n",u32ReservedSector/2);
		u32ReservedSector = (*(pImageList+2));
	}
	if (((*(pImageList+0)) == 0x574255aa) && ((*(pImageList+3)) == 0x57425963))
	{
		count = *(pImageList+1);

		pImageList = pImageList+4;
		for (i=0; i<count; i++)
		{
			if (((*(pImageList) >> 16) & 0xffff) < 4)
			{
				image.startSector = *(pImageList + 1) & 0xffff;
				image.endSector = (*(pImageList + 1) & 0xffff0000) >> 16;
				if(image.endSector>u32ReservedSector)
					u32ReservedSector = image.endSector;
				image.executeAddr = *(pImageList + 2);
				image.fileLen = *(pImageList + 3);
			}
			/* pointer to next image */
			pImageList = pImageList+12;
		}
	}
	return (u32ReservedSector+1);
}
void NVT_FirmwareLoader(void)
{
	INT8 path[64];
	INT 	fd;
	void	(*_jump)(void);
	INT32 i32ErrorCode;

	fsAssignDriveNumber('X', DISK_TYPE_SD_MMC, 0, 1);
	fsAssignDriveNumber('Y', DISK_TYPE_SD_MMC, 0, 2);


	/*-----------------------------------------------------------------------*/
	/*  Init SD card                                                         				*/
	/*-----------------------------------------------------------------------*/
	sicIoctl(SIC_SET_CLOCK, u32UPll_clock/1000, 0, 0);
	sicOpen();

	// Alway use SD0 as firmware loader
	i32ErrorCode = sicSdOpen0();	/* Total sector or error code */
	sysprintf("Check Firmware, Total SD 0 sectors (%x)\n", i32ErrorCode);
	fsAsciiToUnicode(FIRMWARE_PATH_SD, path, TRUE);
	fd = fsOpenFile(path, 0, O_RDONLY);
	if(fd < 0)
	{//Close SD IP.
		sicSdClose0();
		sicClose();
		return;
	}
	sysprintf("Fireware found. Load firmware..\n");
//	ParsingReservedArea();	/* To get execute address and image length */
	loadKernelCont(fd, 0);

	fsCloseFile(fd);	//Close kernel file

    if (g_ibr_boot_sd_port == 0)
    	sicSdClose0();
    else if (g_ibr_boot_sd_port == 1)
    	sicSdClose1();
    else if (g_ibr_boot_sd_port == 2)
    	sicSdClose2();

	sicClose();

	sysSetGlobalInterrupt(DISABLE_ALL_INTERRUPTS);
	sysSetLocalInterrupt(DISABLE_FIQ_IRQ);
	// Invalid and disable cache
//	sysDisableCache();
//	sysInvalidCache();
	memcpy(0x0, kbuf, CP_SIZE);
	// JUMP to kernel
	outp32(TTR, 0x7);
	sysprintf("Jump to kernel\n");
	//lcmFill2Dark((char *)(FB_ADDR | 0x80000000));
	outp32(REG_AHBIPRST, JPGRST | SICRST |UDCRST | EDMARST);
	outp32(REG_AHBIPRST, 0);
	outp32(REG_APBIPRST, UART1RST | UART0RST | TMR1RST | TMR0RST );
	outp32(REG_APBIPRST, 0);

	sysFlushCache(I_D_CACHE);
	// Invalid and disable cache
	sysDisableCache();
	sysInvalidCache();

	_jump = (void(*)(void))(0x0); // Jump to 0x0 and execute kernel
	_jump();

	while(1);

}

UINT32 NVT_LoadKernelFromSD(void)
{
	INT8 path[64];
	//volatile INT32 i32ErrorCode;
	INT found_kernel = 0;
	INT found_avi = 0;
	PDISK_T		*pDiskList;
	UINT32 block_size, free_size, disk_size;
	UINT32 u32TotalSize;
	void	(*_jump)(void);
	UINT32 u32KpiReport;

	DBG_PRINTF("Loader will load conprog.bin in SD card.\n");
	fsAssignDriveNumber('X', DISK_TYPE_SD_MMC, 0, 1);
	fsAssignDriveNumber('Y', DISK_TYPE_SD_MMC, 0, 2);
	/*-----------------------------------------------------------------------*/
	/*  Init SD card                                                         */
	/*-----------------------------------------------------------------------*/
	sicIoctl(SIC_SET_CLOCK, u32UPll_clock/1000, 0, 0);
	sicOpen();

    if (g_ibr_boot_sd_port == 0)
    {
    	sysprintf("Load code from SD0\n");
    	g_i32SD0TotalSector = sicSdOpen0();	/* Total sector or error code */
    	if(g_i32SD0TotalSector < 0)
            sicSdClose0();
    	sysprintf("total SD0 sectors number (%x)\n", g_i32SD0TotalSector);
    }
    else if (g_ibr_boot_sd_port == 1)
    {
    	sysprintf("Load code from SD1\n");
    	g_i32SD1TotalSector = sicSdOpen1();	/* Total sector or error code */
    	if(g_i32SD1TotalSector < 0)
            sicSdClose1();
    	sysprintf("total SD1 sectors (%x)\n", g_i32SD1TotalSector);
    }
    else if (g_ibr_boot_sd_port == 2)
    {
    	sysprintf("Load code from SD2\n");
    	g_i32SD2TotalSector = sicSdOpen2();	/* Total sector or error code */
    	if(g_i32SD2TotalSector < 0)
            sicSdClose2();
    	sysprintf("total SD2 sectors (%x)\n", g_i32SD2TotalSector);
    }

	/* In here for USB VBus stable. Othwise, USB library can not judge VBus correct  */
	sysprintf("UDC open\n");
	udcOpen();

	//DBG_PRINTF("total SD sectors (%x)\n", sicSdOpen());

	/* Get SD disk information*/
	pDiskList = fsGetFullDiskInfomation();
	sysprintf("Total Disk Size = %dMB\n", pDiskList->uDiskSize/1024);
	/* Format NAND if necessery */
	if ((fsDiskFreeSpace('X', &block_size, &free_size, &disk_size) < 0) ||
	    (fsDiskFreeSpace('Y', &block_size, &free_size, &disk_size) < 0))
	{
		UINT32 u32Reserved;
		u32Reserved = ParsingReservedArea();
		sysprintf("unknow disk type, format device ...Reserved Area= %dKB \n", u32Reserved/2);
		fsSetReservedArea(u32Reserved);
	#if 1
		if (g_ibr_boot_sd_port == 0)
			u32TotalSize = (g_i32SD0TotalSector-u32Reserved)*512; 
		else if (g_ibr_boot_sd_port == 1)
			u32TotalSize = (g_i32SD1TotalSector-u32Reserved)*512; 
		else if (g_ibr_boot_sd_port == 2)
			u32TotalSize = (g_i32SD2TotalSector-u32Reserved)*512; 
	#endif	
		
    		if (fsTwoPartAndFormatAll((PDISK_T *)pDiskList->ptSelf, SD1_1_SIZE*1024, (u32TotalSize- SD1_1_SIZE*1024)) < 0)
    		{
			sysprintf("Format failed\n");
			goto sd_halt;
		}
    		fsSetVolumeLabel('X', "SD1-1\n", strlen("SD1-1"));
    		#ifdef __KLE_DEMO__
    		fsSetVolumeLabel('Y', "RAINBOW\n", strlen("RAINBOW"));
		#else
		fsSetVolumeLabel('Y', "SD1-2\n", strlen("SD1-2"));
		#endif
	}

	/* Read volume config file */
	VolumeConfigFile();

	/* Read saturation config file to same as linux kernel */
	//SaturationConfigFile();
	//LcmSaturationInc(u32Saturation);

	/* Detect USB */
	u32KpiReport = kpi_read(KPI_NONBLOCK) & MASS_STORAGE;
	sysprintf("KPI  Key Code = 0x%x\n", u32KpiReport);
	if(inp32(0xFF001804) == 0x6D617373){	//AutoWriter
		outp32(0xFF001804, 0);
		u32KpiReport = MASS_STORAGE;
	}
	if(u32KpiReport==(MASS_STORAGE))
	{//Demo board = "Up"+"down" Key
		sysprintf("Enter USB\n");
		if(udcIsAttached())
		{
			//for mass's issue. sicSdClose();
			sysprintf("Detect USB plug in\n");
			#if 0
			mass(&ptNDisk, i32ErrorCode);				/* ptNDisk is useless for SD mass-storage*/
			#else
			mass(NULL, NULL, NULL);					/* ptNDisk is useless for SD mass-storage*/
			#endif
			sysprintf("USB plug out\n");
		}
	}
	// Check if movie & Kernel exists.
#ifdef __PICO_PROJECTOR__
	LcmBacklightEnable();			
#endif	
	fsAsciiToUnicode(MOVIE_PATH_SD, path, TRUE);
	mfd = fsOpenFile(path, 0, O_RDONLY);
	if(mfd > 0)
	{
		found_avi = 1;
		fsCloseFile(mfd);
		sysprintf("animation file found\n");
	}

	fsAsciiToUnicode(KERNEL_PATH_SD, path, TRUE);
	kfd = fsOpenFile(path, 0, O_RDONLY);
	if(kfd > 0)
	{
		found_kernel = 1;
		sysprintf("kernel found\n");
	}
	
	/* Initial SPU in advance for linux set volume issue */
	//spuDacOn(1);			/* do in sdloader or nandloder */
	//sysDelay(100);			/* If need de-pop, please turn on 1s delay */ 
	spuOpen(eDRVSPU_FREQ_8000);
	spuSetDacSlaveMode();		
	if(found_avi)
	{
		char ucSring[64]= MOVIE_PATH_SD;
		playAni(kfd, ucSring);
	}
	else
	{
#ifndef __NON_PLAY_AVI__
		aviSetPlayVolume(u16Volume);
#endif		
		if(found_kernel)
			loadKernelCont(kfd, 0);
	}
	if(kfd > 0)
	{
		fsCloseFile(kfd);	//Close kernel file

        if (g_ibr_boot_sd_port == 0)
        	sicSdClose0();
        else if (g_ibr_boot_sd_port == 1)
        	sicSdClose1();
        else if (g_ibr_boot_sd_port == 2)
        	sicSdClose2();

		sicClose();

		sysSetGlobalInterrupt(DISABLE_ALL_INTERRUPTS);
		sysSetLocalInterrupt(DISABLE_FIQ_IRQ);


		memcpy(0x0, kbuf, CP_SIZE);

		// JUMP to kernel
		outp32(TTR, 0x7);
		sysprintf("Jump to kernel\n");
		//sysprintf( "### 0x%x, 0x%x, 0x%x, 0x%x ###\n", inp32(0xb8001030), inp32(0xb8001034), inp32(0xb8001038), inp32(0xb800103C) );
		//lcmFill2Dark((char *)(FB_ADDR | 0x80000000));
		outp32(REG_AHBIPRST, JPGRST | SICRST |UDCRST | EDMARST);
		outp32(REG_AHBIPRST, 0);
		outp32(REG_APBIPRST, UART1RST | UART0RST | TMR1RST | TMR0RST );
		outp32(REG_APBIPRST, 0);
		sysFlushCache(I_D_CACHE);
		// Invalid and disable cache
		sysDisableCache();
		sysInvalidCache();
#if 0	/* MC siad the MUTE function has been removed in DV2 */		
	#ifdef __DV1__
		MuteDisable();			/* Not Mute */
	#else
		MuteEnable();			/* Mute */
	#endif
#endif	
		outp32(REG_AHBCLK, inp32(REG_AHBCLK) & ~(SPU_CKE1|SPU_CKE|SD_CKE|NAND_CKE|USBD_CKE|I2S_CKE));
		outp32(REG_APBCLK, inp32(REG_APBCLK) & ~(KPI_CKE|WDCLK_CKE|TOUCH_CKE|TMR1_CKE|I2C_CKE|ADC_CKE));		
		_jump = (void(*)(void))(0x0); // Jump to 0x0 and execute kernel
		_jump();

		while(1);
		return(0); // avoid compilation warning
	}
	else
	{
		DBG_PRINTF("Cannot find conprog.bin in SD card.(err=0x%x)\n", kfd);
		DBG_PRINTF("Try load conprog.bin in NAND\n\n");
	}
	return Successful;
sd_halt:
	sysprintf("systen exit\n");
	while(1); // never return
}

UINT32 NVT_LoadKernelFromNAND(void)
{
	INT found_kernel = 0;
	INT found_avi = 0;
	UINT32 block_size, free_size, disk_size;
	UINT32 u32TotalSize, u32RefClock;
	void	(*_jump)(void);
	INT8 path[64];
	UINT32 u32KpiReport;

	/* In here for USB VBus stable. Otherwise, USB library can not judge VBus correct  */

	udcOpen();

   	/* For detect VBUS stable */
   	sicOpen();

	u32RefClock = sysGetPLLOutputHz(eSYS_UPLL, sysGetExternalClock());
	sicIoctl(SIC_SET_CLOCK, u32RefClock/1000, 0, 0);

	/* Initialize GNAND */
	if(GNAND_InitNAND(&_nandDiskDriver0, &ptNDisk, TRUE) < 0)
	{
		sysprintf("GNAND_InitNAND error\n");
		goto halt;
	}

	if(GNAND_MountNandDisk(&ptNDisk) < 0)
	{
		sysprintf("GNAND_MountNandDisk error\n");
		goto halt;
	}

	/* Get NAND disk information*/
	u32TotalSize = (UINT32)((UINT64)ptNDisk.nZone* ptNDisk.nLBPerZone*ptNDisk.nPagePerBlock*ptNDisk.nPageSize/1024);
	sysprintf("Total Disk Size %u KB\n", u32TotalSize);
#ifndef __ENABLE_NAND_2__	 /* 1 NAND device, two partitions */
	/* Format NAND if necessery */
	if ((fsDiskFreeSpace('C', &block_size, &free_size, &disk_size) < 0) ||
	    (fsDiskFreeSpace('D', &block_size, &free_size, &disk_size) < 0))
	    	{
	    		sysprintf("unknow disk type, format device .....\n");
		    	if (fsTwoPartAndFormatAll((PDISK_T *)ptNDisk.pDisk, NAND1_1_SIZE*1024, (u32TotalSize- NAND1_1_SIZE*1024)) < 0) {
					sysprintf("Format failed\n");
				goto halt;
	    	}
	    	fsSetVolumeLabel('C', "NAND1-1\n", strlen("NAND1-1"));
	    	#ifdef __KLE_DEMO__
	    	fsSetVolumeLabel('D', "RAINBOW\n", strlen("RAINBOW"));
	    	#else
		fsSetVolumeLabel('D', "NAND1-2\n", strlen("NAND1-2"));
		#endif
	}
#else /* 2 NAND devices, one partition for each device */
	/* Format NAND if necessery */
	{
		PDISK_T       *pDiskList, *ptPDiskPtr;
		ptPDiskPtr = pDiskList = fsGetFullDiskInfomation();
		if (fsDiskFreeSpace('C', &block_size, &free_size, &disk_size) < 0){
			sysprintf("unknow disk type for NAND Disk 0, format device .....\n");
			fsFormatFlashMemoryCard(ptPDiskPtr);
			sysprintf("Format NAND1-1 done\n");
			fsSetVolumeLabel('C', "NAND1-1\n", strlen("NAND1-1"));
		}
		fsReleaseDiskInformation(pDiskList);
	 	if(GNAND_InitNAND(&_nandDiskDriver2, &ptNDisk2, TRUE) < 0) {
			sysprintf("GNAND_InitNAND error\n");
			goto halt;
		}
		if(GNAND_MountNandDisk(&ptNDisk2) < 0) {
			sysprintf("GNAND_MountNandDisk error\n");
			goto halt;
		}
		u32TotalSize = (UINT32)((UINT64)ptNDisk2.nZone* ptNDisk2.nLBPerZone*ptNDisk2.nPagePerBlock*ptNDisk2.nPageSize/1024);
		sysprintf("The Second Disk Total Size %d KB\n", u32TotalSize);
#if 0
		if(u32TotalSize>(2048*1024)){ /* The 2nd NAND flash is 4GB. */
			if (fsDiskFreeSpace('D', &block_size, &free_size, &disk_size) < 0){
				fsSetReservedArea(0x3F);	/* Set start sector as default */
				ptPDiskPtr = pDiskList = fsGetFullDiskInfomation();
				ptPDiskPtr = ptPDiskPtr->ptPDiskAllLink;
				fsFormatFlashMemoryCard(ptPDiskPtr);
				fsSetVolumeLabel('D', "NAND2-1\n", strlen("NAND2-1"));
				sysprintf("Format NAND2 done\n");
				fsReleaseDiskInformation(pDiskList);
			}
		}
		else{ /*  The 2nd NAND flash is 2GB --> Partition into 512MB and 1.5GB */
#endif
			{
				INT32 i32Status1, i32Status2;
				UINT32 disk1_size, disk2_size;
				i32Status1 = fsDiskFreeSpace('D', &block_size, &free_size, &disk1_size);
				sysprintf("D: Block/Free/Disk = %d - %d- %d KB\n", block_size, free_size, disk1_size);
				i32Status2 = fsDiskFreeSpace('E', &block_size, &free_size, &disk2_size);
				sysprintf("E: Block/Free/Disk = %d - %d- %d KB\n", block_size, free_size, disk2_size);

				if ( ((i32Status1< 0) || (((disk1_size<500000) || (disk1_size>600000)) &&   ((disk2_size<500000) || (disk2_size>600000))) ) || /* NAND2-1 is about 524385 KB */
			    		(i32Status2< 0) ) {
					sysprintf("unknow disk type or size is not as expect, format device .....\n");
					fsSetReservedArea(0x3F);	/* Set start sector as default */
				    	if (fsTwoPartAndFormatAll((PDISK_T *)ptNDisk2.pDisk, NAND2_1_SIZE*1024, (u32TotalSize- NAND2_1_SIZE*1024)) < 0) {
							sysprintf("Format failed\n");
						goto halt;
					}
					#ifdef __KLE_DEMO__
	    				fsSetVolumeLabel('D', "RAINBOW1\n", strlen("RAINBOW1"));
	    				fsSetVolumeLabel('E', "RAINBOW2\n", strlen("RAINBOW2"));
	    				#else
					fsSetVolumeLabel('D', "NAND2-1\n", strlen("NAND2-1"));
					fsSetVolumeLabel('E', "NAND2-2\n", strlen("NAND2-2"));
					#endif
					sysprintf("Format NAND2 done\n");
				}
			}
#if 0
		}
#endif
	}
#endif

	/* Read saturation config file to same as linux kernel */
	//SaturationConfigFile();
	//LcmSaturationInc(u32Saturation);

#ifdef __BAT_DECT__
	if(ChargerDetect()==TRUE)
	{// No any charger or charge done. need to do battery detect 
		sysprintf("No charger or charge done\n");	
		if(udcIsAttached())
		{
			sysprintf("Adaptor or USB plug in\n");			
			BatteryDetection(TRUE);		/* USB plug-in or Power adaptor connect */
		}	
		else
			BatteryDetection(FALSE);
	}		
#endif

	/* Detect USB */
	u32KpiReport = kpi_read(KPI_NONBLOCK);
	sysprintf("KPI  Key Code = 0x%x\n", u32KpiReport);
	if(inp32(0xFF001804) == 0x6D617373){	//AutoWriter
		outp32(0xFF001804, 0);
		u32KpiReport = MASS_STORAGE;
	}
	if(u32KpiReport==(MASS_STORAGE))
	{//Demo board = "Up"+"down" Key
		sysprintf("Enter USB Detection\n");
		if(udcIsAttached())
		{
				sysprintf("USB plug in\n");
#if defined(__ENABLE_NAND_0__) || defined(__ENABLE_NAND_2__)
				mass(&ptNDisk, NULL, &ptNDisk2);
#endif
#if defined(__ENABLE_NAND_0__)
				mass(&ptNDisk, NULL, NULL);	/* Total sector in ptNDisk. parameter 2 is useless for NAND*/
#endif
				sysprintf("USB plug out\n");
		}
	}

	NVT_Updater();

	/* Read volume config file to same as linux kernel */
	VolumeConfigFile();
	
#ifdef __PICO_PROJECTOR__
	LcmBacklightEnable();			
#endif	

#ifdef __PLAYBACK_MP3__
	/* Check if mp3 & Kernel exists */
	sysprintf("%s\n",MP3_PATH);
	fsAsciiToUnicode(MP3_PATH, path, TRUE);
	kfd = fsOpenFile(path, 0, O_RDONLY);
	if(kfd > 0)
	{
		found_avi = 1;
		fsCloseFile(kfd);
		sysprintf("mp3 file found\n");
	}
#else 
	/* Check if movie & Kernel exists */
	sysprintf("%s\n",MOVIE_PATH);
	fsAsciiToUnicode(MOVIE_PATH, path, TRUE);
	kfd = fsOpenFile(path, 0, O_RDONLY);
	if(kfd > 0)
	{
		found_avi = 1;
		fsCloseFile(kfd);
		sysprintf("animation file found\n");
	}
#endif
	fsAsciiToUnicode(KERNEL_PATH, path, TRUE);
	kfd = fsOpenFile(path, 0, O_RDONLY);
	if(kfd > 0)
	{
		found_kernel = 1;
		sysprintf("kernel found\n");
	}

	#if defined(__IXL_WINTEK__) || defined(__GWMT9360A__) || defined(__GWMT9615A__)
	AudioChanelControl();
	#endif
	
	/* Initial SPU in advance for linux set volume issue */
	//spuDacOn(1);			/* do in sdloader or nandloder */
	//sysDelay(100);			/* If need de-pop, please turn on 1s delay */ 	
	spuOpen(eDRVSPU_FREQ_8000);
	spuSetDacSlaveMode();		
	if(found_avi)
	{
#ifdef __PLAYBACK_MP3__	
		char ucSring[64]= MP3_PATH;
#else	
		char ucSring[64]= MOVIE_PATH;
#endif		
		
		
#ifdef __PLAYBACK_MP3__
		playAudio(kfd, ucSring);		
#else
		playAni(kfd, ucSring);
#endif 
	}
	else
	{
#ifndef __NON_PLAY_AVI__
		aviSetPlayVolume(u16Volume);
#endif		
		if(found_kernel)
			loadKernelCont(kfd, 0);
	}
	if(found_kernel)
	{
		GNAND_UnMountNandDisk(&ptNDisk);
       	fmiSMClose(0);

        if (g_ibr_boot_sd_port == 0)
        	sicSdClose0();
        else if (g_ibr_boot_sd_port == 1)
        	sicSdClose1();
        else if (g_ibr_boot_sd_port == 2)
        	sicSdClose2();

		sicClose();
		/* Disable interrupt */
		sysSetGlobalInterrupt(DISABLE_ALL_INTERRUPTS);
		sysSetLocalInterrupt(DISABLE_FIQ_IRQ);

		/* Reset IPs */
		sysprintf("Jump to kernel\n");
		//outp32(REG_AHBIPRST, JPGRST | SICRST |UDCRST );
		outp32(REG_AHBIPRST, JPGRST | SICRST |UDCRST | EDMARST);
		outp32(REG_AHBIPRST, 0);
		outp32(REG_APBIPRST, UART1RST | UART0RST | TMR1RST | TMR0RST );
		outp32(REG_APBIPRST, 0);

		memcpy(0x0, kbuf, CP_SIZE);
    		sysFlushCache(I_D_CACHE);
    		/* Invalid and disable cache */
		sysDisableCache();
		sysInvalidCache();
		
		outp32(REG_AHBCLK, inp32(REG_AHBCLK) & ~(SPU_CKE1|SPU_CKE|SD_CKE|NAND_CKE|USBD_CKE|I2S_CKE));
		outp32(REG_APBCLK, inp32(REG_APBCLK) & ~(KPI_CKE|WDCLK_CKE|TOUCH_CKE|TMR1_CKE|I2C_CKE|ADC_CKE));
		_jump = (void(*)(void))(0x0); /* Jump to 0x0 and execute kernel */
		_jump();
	} 
	else 
	{
		sysprintf("Cannot find conprog.bin");
	}
halt:	
	sysprintf("systen exit\n");
	while(1); // never return	

}


#ifdef __KLE_DEMO__
    extern int nand_post_process(void);
#endif

int main(void)
{
    // IBR and SDLoader keep the booting SD port number on register SDCR.
    // NVTLoader should load image from same SD port.
    outpw(REG_AHBCLK, inpw(REG_AHBCLK) | SD_CKE);   // SDLoader disable SIC/SD clock before jump to NVTLoader.
    outpw(REG_AHBCLK, inpw(REG_AHBCLK) | SIC_CKE);  // Now, enable clock to read SD register.
    g_ibr_boot_sd_port = (inpw(REG_SDCR) & SDCR_SDPORT) >> 29;
    // sysprintf("g_ibr_boot_sd_port = %d\n", g_ibr_boot_sd_port);
    outpw(REG_AHBCLK, inpw(REG_AHBCLK) & ~SD_CKE);
    outpw(REG_AHBCLK, inpw(REG_AHBCLK) & ~SIC_CKE);

	sysDisableCache(); 	
	sysFlushCache(I_D_CACHE);	
	init();	
	initVPostShowLogo();	
	
#ifdef __KLE_DEMO__	
	I2cInit();
	I2cSendStart();
	I2cSendStop();
#endif	

#ifdef __BAT_DECT__	
	if(ChargerDetect()==TRUE)
	{// No any charger or charge done. need to do battery detect 
		sysprintf("No charger or charge done\n");	
		BatteryDetection(FALSE);		
	}
	else
		sysprintf("In charging\n");		
#endif
				
	u32UPll_clock = sysGetPLLOutputHz(eSYS_UPLL, sysGetExternalClock());
#if defined(__NAND_POST_PROCESS__) && !defined(__ENABLE_NAND_2__) &&  !defined(__ENABLE_SD_CARD_0__)
	//--- To support NAND post-process for NAND pre-programming in KLE project
	nand_post_process();  
#endif

	fsInitFileSystem();

	NVT_FirmwareLoader();

#ifdef __MASS_PARODUCT__
	EMU_MassProduction();
#endif

#if defined(__ENABLE_SD_CARD_0__)||defined(__ENABLE_SD_CARD_1__)||defined(__ENABLE_SD_CARD_2__)
	NVT_LoadKernelFromSD();
#endif
	sysprintf("Load code from NAND\n");
	fsInitFileSystem();
	NVT_LoadKernelFromNAND();

	return(0); // avoid compilation warning
}