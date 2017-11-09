/****************************************************************************
*                                                                           *
* Copyright (c) 2009 Nuvoton Tech. Corp. All rights reserved.               *
*                                                                           *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wblib.h"
#include "w55fa95_vpost.h"
#include "AviLib.h"
#include "nvtfat.h"
#include "nvtloader.h"
#include "w55fa95_kpi.h"
#include "w55fa95_gpio.h"
#include "SPU.h"
#include "w55fa95_spu.h"

static int _complete = 0;
static int _offset = 0;
static int _fd;


extern unsigned char kbuf[CP_SIZE];
extern BOOL bIsIceMode;
void initVPost(unsigned char*);


LCDFORMATEX lcdInfo;

static BOOL bEscapeKeyPress=FALSE;
static BOOL bSetVolume=FALSE;
UINT32 u16Volume = 60;		// 60*63/100=; Linux driver default = 60 
UINT32 u32Saturation = 60;		// 60*63/100=; Linux driver default = 60 

// Detect Earphone plug in?
UINT32 u32EarphoneDetChannel = 0;
UINT32 u32Flag20ms=0;

//#define __OPT_SPEAKER_DECADE_1DB__

#ifdef __OPT_SPEAKER_DECADE_1DB__		
volatile UINT8 gb_IsFirstChkEarphone = TRUE;
volatile UINT8 gb_IsEarphoneState = TRUE;
extern void spuSwitchVolume(UINT8);
#endif

void Timer0_200msCallback(void)
{
	if(EarphoneDetect()==FALSE)
	{
		DBG_PRINTF("Speaker High\n");
		SpeakerEnable();
		
#ifdef __OPT_SPEAKER_DECADE_1DB__		
		if (gb_IsFirstChkEarphone == TRUE)
		{
			gb_IsFirstChkEarphone = FALSE;
			gb_IsEarphoneState = FALSE;
			spuSwitchVolume(gb_IsEarphoneState);
		}
		else
		{
			if (gb_IsEarphoneState == TRUE)
			{
				gb_IsEarphoneState = FALSE;
				spuSwitchVolume(gb_IsEarphoneState);			
			}
		}
#endif					
	}	
	else
	{
		DBG_PRINTF("Speaker Low\n");
		SpeakerDisable();	

#ifdef __OPT_SPEAKER_DECADE_1DB__		
		if (gb_IsFirstChkEarphone == TRUE)
		{
			gb_IsFirstChkEarphone = FALSE;
			gb_IsEarphoneState = TRUE;
		}
		else
		{
			if (gb_IsEarphoneState == FALSE)
			{
				gb_IsEarphoneState = TRUE;
				spuSwitchVolume(gb_IsEarphoneState);			
			}
		}
#endif					
	}	
}

void loadKernel(AVI_INFO_T *aviInfo)
{
	int bytes, result;
	
	if(bSetVolume==FALSE)
	{
		aviSetPlayVolume(u16Volume);
		sysprintf("Volume = %d\n", u16Volume);	
		bSetVolume = TRUE;				
	}
	
	if(bEscapeKeyPress==TRUE)	
		return;
	//sysprintf("\n");
	if(!_complete) {
		if(_offset < CP_SIZE)
		{//1th, keep the original vector table in  SDRAM. 			
			result = fsReadFile(_fd, (kbuf + _offset), (CP_SIZE - _offset), &bytes);
		}	
		else
		{//2nd, 3rd, .... Copy the kernel content to address 16K, 32K, 	
			result = fsReadFile(_fd, (UINT8 *)_offset, CP_SIZE, &bytes);
		}	
		if(result == FS_OK)
			_offset += bytes;
		else
			_complete = 1;
	}
	if(bEscapeKeyPress==FALSE)
	{
		result = kpi_read(KPI_NONBLOCK);		
		if(result == HOME_KEY)
		{//Stop AVI playback
			bEscapeKeyPress = TRUE;
			sysprintf("Key pressed %d\n", result);
			aviStopPlayFile();				
			aviSetPlayVolume(u16Volume);		
			sysprintf("Volume = %d\n", u16Volume);
		}		
	}	
	return;
}

void loadKernelCont(int fd, int offset)
{	
	int bytes, result;
	
	while(1) {
		if(offset < CP_SIZE)		
			result = fsReadFile(fd, (kbuf + offset), (CP_SIZE - offset), &bytes);
		else
			result = fsReadFile(fd, (UINT8 *)offset, CP_SIZE, &bytes);	
		if(result == FS_OK)
			offset += bytes;
		else
			return;	
	}
}
void lcmFill2Dark(unsigned char* fb)
{
	lcdInfo.ucVASrcFormat = DRVVPOST_FRAME_YCBYCR;
	if(lcdInfo.ucVASrcFormat == DRVVPOST_FRAME_YCBYCR)
	{	
		UINT32 i;
		UINT32* ptBufAddr=(UINT32*)((UINT32)fb | 0x80000000);
		for(i=0;i<(PANEL_WIDTH * PANEL_HEIGHT * PANEL_BPP);i=i+4)
		{
			outpw(ptBufAddr, 0x80108010);
			ptBufAddr = ptBufAddr+1;		//2 pixels 
		}
	}
	else if(lcdInfo.ucVASrcFormat == DRVVPOST_FRAME_RGB565)
	{
		memset((char*)fb, 0, PANEL_WIDTH * PANEL_HEIGHT * PANEL_BPP);
	} 
}


static UINT32 bIsInitVpost=FALSE;
void initVPostShowLogo(void)
{

	LcmPowerInit();
	LcmPowerEnable();
	if(bIsInitVpost==FALSE)
	{
		bIsInitVpost = TRUE;		
		lcdInfo.ucVASrcFormat = DRVVPOST_FRAME_RGB565;				
		lcdInfo.nScreenWidth = PANEL_WIDTH;	
		lcdInfo.nScreenHeight = PANEL_HEIGHT;
		vpostLCMInit(&lcdInfo, (UINT32*)FB_ADDR);
//#if defined (__LCM_800x480__) ||  defined (__LCM_800x600__) 
	outp32(REG_SDOPM, (inp32(REG_SDOPM) & ~(PCHMODE|OPMODE)) | OPMODE); 
	outp32(REG_LCM_COLORSET, inp32(REG_LCM_COLORSET) | BIT31);
//#endif
		//backLightEnable();
	}		
	//LcmSaturationInit();
}


void playAni(int kfd, char* pcString)
{
	char aniPath[64];	
	if(EarphoneDetect()==FALSE)	/* Fixed the defaut speaker out while earphone attached */
		SpeakerEnable();
	else
		SpeakerDisable();		
			
	lcdInfo.ucVASrcFormat = DRVVPOST_FRAME_YCBYCR;				
	fsAsciiToUnicode(pcString, aniPath, TRUE);
	
	MuteDisable();
	_fd = kfd; // let callback function know the file descriptor
	bEscapeKeyPress = FALSE;
	if (aviPlayFile(aniPath, 0, 0, DIRECT_RGB565, (kfd > 0) ? loadKernel : NULL) < 0)
		DBG_PRINTF("Playback failed\n");
	else
		DBG_PRINTF("Playback done.\n");

	// If movie is too short for callback function to load kernel to SDRAM, keep working...
	if(kfd > 0 && _complete == 0) {
		loadKernelCont(kfd, _offset);
	}
	
	if(kfd > 0) {
		fsCloseFile(_fd);
	}
	
	return;

}
