#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#pragma import(__use_no_semihosting_swi)

#include "wbio.h"
#include "wblib.h"

#include "w55fa95_vpost.h"
#include "w55fa95_sic.h"
#include "nvtfat.h"
#include "AviLib.h"
#include "spu.h"


//#pragma import(__use_no_semihosting_swi)
//#define VPOST_FRAME_BUFSZ		(640*480*2)
#define VPOST_FRAME_BUFSZ		(800*480*2)

static __align(256) UINT8  _VpostFrameBufferPool[VPOST_FRAME_BUFSZ];
static UINT8   *_VpostFrameBuffer;


void  avi_play_control(AVI_INFO_T *aviInfo)
{
	static INT	last_time;
	int    frame_rate;
	
	if (aviInfo->uPlayCurTimePos != 0)
		frame_rate = ((aviInfo->uVidFramesPlayed - aviInfo->uVidFramesSkipped) * 100) / aviInfo->uPlayCurTimePos;
	
	if (aviInfo->uPlayCurTimePos - last_time > 100)
	{
		sysprintf("%02d:%02d / %02d:%02d  Vid fps: %d / %d\n", 
			aviInfo->uPlayCurTimePos / 6000, (aviInfo->uPlayCurTimePos / 100) % 60,
			aviInfo->uMovieLength / 6000, (aviInfo->uMovieLength / 100) % 60,
			frame_rate, aviInfo->uVideoFrameRate);
		last_time = aviInfo->uPlayCurTimePos;
	}
}


int main()
{
    WB_UART_T 	uart;
	LCDFORMATEX lcdformatex;
	CHAR		suFileName[128];
	INT			nStatus;
	UINT32 u32ExtFreq;
	UINT32 u32PllOutHz;	

	
	/* CACHE_ON	*/
	sysEnableCache(CACHE_WRITE_BACK);

	/*-----------------------------------------------------------------------*/
	/*  CPU/HCLK/APB:  300/150/75                                             */
	/*-----------------------------------------------------------------------*/
	sysSetSystemClock(eSYS_UPLL,
    									300000000,
    									300000000);

	/*-----------------------------------------------------------------------*/
	/*  Init UART, N,8,1, 115200                                             */
	/*-----------------------------------------------------------------------*/
	u32ExtFreq = sysGetExternalClock();
	
	uart.uiFreq = u32ExtFreq;					//use XIN clock
    uart.uiBaudrate = 115200;
    uart.uiDataBits = WB_DATA_BITS_8;
    uart.uiStopBits = WB_STOP_BITS_1;
    uart.uiParity = WB_PARITY_NONE;
	uart.uiRxTriggerLevel = LEVEL_1_BYTE;
    sysInitializeUART(&uart);
	sysprintf("UART initialized.\n");

	_VpostFrameBuffer = (UINT8 *)((UINT32)_VpostFrameBufferPool | 0x80000000);

	/*-----------------------------------------------------------------------*/
	/*  Init timer                                                           */
	/*-----------------------------------------------------------------------*/
	sysSetTimerReferenceClock (TIMER0, u32ExtFreq);
	sysStartTimer(TIMER0, 100, PERIODIC_MODE);

	/*-----------------------------------------------------------------------*/
	/*  Init FAT file system                                                 */
	/*-----------------------------------------------------------------------*/
	sysprintf("fsInitFileSystem.\n");
	fsInitFileSystem();

	/*-----------------------------------------------------------------------*/
	/*  Init SD card                                                         */
	/*-----------------------------------------------------------------------*/
	u32PllOutHz = sysGetPLLOutputHz(eSYS_UPLL, u32ExtFreq);
	sicIoctl(SIC_SET_CLOCK, u32PllOutHz/1000, 0, 0);    // clock from PLL
	sicOpen();
	sysprintf("total sectors (%x)\n", sicSdOpen0());
	
	spuOpen(eDRVSPU_FREQ_8000);
	spuDacOn(1);
	sysDelay(100);	
	spuSetDacSlaveMode();	

#if 0
	/*-----------------------------------------------------------------------*/
	/*                                                                       */
	/*  Direct RGB555 AVI playback 	               							 */
	/*                                                                       */
	/*-----------------------------------------------------------------------*/
	lcdformatex.ucVASrcFormat = DRVVPOST_FRAME_RGB555;
    vpostLCMInit(&lcdformatex, (UINT32 *)_VpostFrameBuffer);

	fsAsciiToUnicode("c:\\480x272.avi", suFileName, TRUE);	

   	if (aviPlayFile(suFileName, 0, 0, DIRECT_RGB555, avi_play_control) < 0)
		sysprintf("Playback failed, code = %x\n", nStatus);
	else
		sysprintf("Playback done.\n");

#endif
#if 1
	/*-----------------------------------------------------------------------*/
	/*                                                                       */
	/*  Direct RGB565 AVI playback 	                                         */
	/*                                                                       */
	/*-----------------------------------------------------------------------*/
	lcdformatex.ucVASrcFormat = DRVVPOST_FRAME_RGB565;
    vpostLCMInit(&lcdformatex, (UINT32 *)_VpostFrameBuffer);

	fsAsciiToUnicode("c:\\480x272.avi", suFileName, TRUE);	

   	if (aviPlayFile(suFileName, 0, 0, DIRECT_RGB565, avi_play_control) < 0)
		sysprintf("Playback failed, code = %x\n", nStatus);
	else
		sysprintf("Playback done.\n");
#endif
#if 0
	/*-----------------------------------------------------------------------*/
	/*                                                                       */
	/*  Direct YUV422 AVI playback 	                                         */
	/*                                                                       */
	/*-----------------------------------------------------------------------*/
	lcdformatex.ucVASrcFormat = DRVVPOST_FRAME_YCBYCR;
    vpostLCMInit(&lcdformatex, (UINT32 *)_VpostFrameBuffer);

	fsAsciiToUnicode("c:\\480x272.avi", suFileName, TRUE);	

   	if (aviPlayFile(suFileName, 0, 0, DIRECT_YUV422, avi_play_control) < 0)
		sysprintf("Playback failed, code = %x\n", nStatus);
	else
		sysprintf("Playback done.\n");
#endif	

	while(1);		
}



