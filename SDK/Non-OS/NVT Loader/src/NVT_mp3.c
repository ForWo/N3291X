#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#pragma import(__use_no_semihosting_swi)

#include "wbio.h"
#include "wblib.h"

#include "w55fa95_sic.h"
#include "nvtfat.h"
#include "spu.h"
#include "nvtloader.h"
#include "nvtloader.h"
#include "w55fa95_kpi.h"
#include "w55fa95_gpio.h"
#include "SPU.h"
#include "w55fa95_spu.h"
//#include "MediaFileLib.h"
#include "AviLib.h"

static int _complete = 0;
static int _offset = 0;
static int _fd;


extern unsigned char kbuf[CP_SIZE];

static BOOL bEscapeKeyPress=FALSE;
extern UINT16 u16Volume;


MV_CFG_T	_tMvCfg;
void mp3loadKernel(MV_CFG_T *ptMvCfg)
{
	int bytes, result;
	if(bEscapeKeyPress==TRUE)	
		return;
	//sysprintf("\n");
	if(!_complete) {
		if(_offset < CP_SIZE)
		{//1th, keep the original vector table in  SDRAM. 			
			result = fsReadFile(_fd, (kbuf + _offset), (CP_SIZE - _offset), &bytes);
			sysprintf("Load Kernel to temp buffer\n");	
		}	
		else
		{//2nd, 3rd, .... Copy the kernel content to address 16K, 32K, 	
			result = fsReadFile(_fd, (UINT8 *)_offset, CP_SIZE, &bytes);
			//sysprintf("Load Kernel to original place\n");
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
			mflPlayControl(&_tMvCfg, PLAY_CTRL_STOP, 0);	
		}		
	}	
	return;
}


void mp3loadKernelCont(int fd, int offset)
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

//#pragma import(__use_no_semihosting_swi)



void  mp3_play_callback(MV_CFG_T *ptMvCfg)
{
	MV_INFO_T 	*ptMvInfo;
	static INT	last_time = 0;

	mflGetMovieInfo(ptMvCfg, &ptMvInfo);

	if ((sysGetTicks(TIMER0) - last_time > 100) &&
		ptMvInfo->uAuTotalFrames)
	{
		sysprintf("T=%d, Progress = %02d:%02d / %02d:%02d\n", 
					sysGetTicks(TIMER0) / 100,
					ptMvInfo->uPlayCurTimePos / 6000, (ptMvInfo->uPlayCurTimePos / 100) % 60,
					ptMvInfo->uMovieLength / 6000, (ptMvInfo->uMovieLength / 100) % 60);
		last_time = sysGetTicks(TIMER0);
	}
}


void playAudio(int kfd, char* pcString)
{
	CHAR		suFileName[128];	
	INT			nStatus;
	
	_fd = kfd; // let callback function know the file descriptor
	/*-----------------------------------------------------------------------*/
	/*                                                                       				*/
	/*  MP3 File playback          	                                         		*/
	/*                                                                       				*/
	/*-----------------------------------------------------------------------*/
	
	memset((UINT8 *)&_tMvCfg, 0, sizeof(_tMvCfg));
	fsAsciiToUnicode("c:\\audio.mp3", suFileName, TRUE);
	_tMvCfg.eInMediaType			= MFL_MEDIA_MP3;
	_tMvCfg.eInStrmType			= MFL_STREAM_FILE;
	_tMvCfg.szIMFAscii				= NULL;
	_tMvCfg.suInMetaFile			= NULL;
	_tMvCfg.szITFAscii				= NULL;
	_tMvCfg.nAudioPlayVolume		= u16Volume;
	_tMvCfg.uStartPlaytimePos		= 0;
	_tMvCfg.nAuABRScanFrameCnt		= 50;
	_tMvCfg.ap_time				= mp3loadKernel;
	_tMvCfg.suInMediaFile			= suFileName;
	sysprintf("MP3 volume = %d\n", u16Volume);
	if ( (nStatus = mflMediaPlayer(&_tMvCfg)) < 0 )
		sysprintf("Playback failed, code = %x\n", nStatus);
	else
		sysprintf("Playback done.\n");
}		