/****************************************************************************
*                                                                           *
* Copyright (c) 2009 Nuvoton Tech. Corp. All rights reserved.               *
*                                                                           *
*****************************************************************************/

/****************************************************************************
* FILENAME
*   main.c
*
* VERSION
*   1.0
*
* DESCRIPTION
*   SPU sample application using SPU library. 
*
* DATA STRUCTURES
*   None
*
* FUNCTIONS
*
* HISTORY
*
* REMARK
*   None
****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wblib.h"
#include "w55fa95_spu.h"
#include "spu.h"

#define OPT_8K_SAMPLE
//#define OPT_11K025_SAMPLE

#ifdef OPT_SPU_FROM_APLL
    __align (32) UINT8 g_AudioPattern[] = {
    
    #ifdef OPT_11K025_SAMPLE
    		#include "11.025k.dat"    
//    #else
//    		#include "pcm16_raw.dat"
    #endif    		

    #ifdef OPT_8K_SAMPLE
    		#include "8k.dat"    
    #endif    		
    
    };
#else
    __align (32) UINT8 g_AudioPattern[] = {
    		#include "pcm16_raw.dat"
    };
#endif
int main(void)
{
	UINT32 u32TestChannel;	
	UINT8  u8SrcFormat;
	UINT32 u8SampleRate;

		DrvSPU_Open();
		
		spuDacOn(1);
		sysDelay(100);		
		spuSetDacSlaveMode();
		
#ifdef OPT_SPU_FROM_APLL        // defined in "w55fa95_spu.h". The SPU driver must be recompiled at the same time


		// Right channel 
		u32TestChannel = 0;
		DrvSPU_ChannelOpen(u32TestChannel);
		DrvSPU_SetBaseAddress(u32TestChannel, (UINT32)g_AudioPattern);
		DrvSPU_SetThresholdAddress(u32TestChannel, (UINT32)g_AudioPattern + sizeof(g_AudioPattern));
		DrvSPU_SetEndAddress(u32TestChannel, (UINT32)g_AudioPattern + sizeof(g_AudioPattern));
		DrvSPU_SetChannelVolume(u32TestChannel, 100);	
		
    #ifdef OPT_11K025_SAMPLE		
        u8SampleRate = eDRVSPU_FREQ_11025;
//    #else
//        u8SampleRate = eDRVSPU_FREQ_44100;    
    #endif        

    #ifdef OPT_8K_SAMPLE		
        u8SampleRate = eDRVSPU_FREQ_8000;
    #endif        
    
		DrvSPU_SetPAN(u32TestChannel, 0x1F00);	// MSB 8-bit = right channel; LSB 8-bit = left channel			
		DrvSPU_SetDFA(u32TestChannel, 0x400);	
	//	DrvSPU_SetDFA(u32TestChannel, 0x1000);			
    
		u8SrcFormat = DRVSPU_STEREO_PCM16_RIGHT;		
		DrvSPU_SetSrcType(u32TestChannel, u8SrcFormat);

		// left channel 		
		u32TestChannel++;
		DrvSPU_ChannelOpen(u32TestChannel);
		DrvSPU_SetBaseAddress(u32TestChannel, (UINT32)g_AudioPattern);
		DrvSPU_SetThresholdAddress(u32TestChannel, (UINT32)g_AudioPattern + sizeof(g_AudioPattern));
		DrvSPU_SetEndAddress(u32TestChannel, (UINT32)g_AudioPattern + sizeof(g_AudioPattern));
		DrvSPU_SetChannelVolume(u32TestChannel, 100);	
		DrvSPU_SetSampleRate(u8SampleRate);				
		DrvSPU_SetPAN(u32TestChannel, 0x001F);	// MSB 8-bit = right channel; LSB 8-bit = left channel			
//		DrvSPU_SetDFA(u32TestChannel, 0x1000);	
		DrvSPU_SetDFA(u32TestChannel, 0x400);			
		
#else		
		// Right channel 
		u32TestChannel = 0;
		DrvSPU_ChannelOpen(u32TestChannel);
		DrvSPU_SetBaseAddress(u32TestChannel, (UINT32)g_AudioPattern);
		DrvSPU_SetThresholdAddress(u32TestChannel, (UINT32)g_AudioPattern + sizeof(g_AudioPattern));
		DrvSPU_SetEndAddress(u32TestChannel, (UINT32)g_AudioPattern + sizeof(g_AudioPattern));
		DrvSPU_SetChannelVolume(u32TestChannel, 100);	
		DrvSPU_SetSampleRate(eDRVSPU_FREQ_44100);		
		DrvSPU_SetPAN(u32TestChannel, 0x1F00);	// MSB 8-bit = right channel; LSB 8-bit = left channel			
		DrvSPU_SetDFA(u32TestChannel, 0x400);	
		u8SrcFormat = DRVSPU_STEREO_PCM16_RIGHT;		
		DrvSPU_SetSrcType(u32TestChannel, u8SrcFormat);

		// left channel 		
		u32TestChannel++;
		DrvSPU_ChannelOpen(u32TestChannel);
		DrvSPU_SetBaseAddress(u32TestChannel, (UINT32)g_AudioPattern);
		DrvSPU_SetThresholdAddress(u32TestChannel, (UINT32)g_AudioPattern + sizeof(g_AudioPattern));
		DrvSPU_SetEndAddress(u32TestChannel, (UINT32)g_AudioPattern + sizeof(g_AudioPattern));
		DrvSPU_SetChannelVolume(u32TestChannel, 100);	
		DrvSPU_SetSampleRate(eDRVSPU_FREQ_44100);			
		DrvSPU_SetPAN(u32TestChannel, 0x001F);	// MSB 8-bit = right channel; LSB 8-bit = left channel			
		DrvSPU_SetDFA(u32TestChannel, 0x400);	
#endif		

		u8SrcFormat = DRVSPU_STEREO_PCM16_LEFT;		
		DrvSPU_SetSrcType(u32TestChannel, u8SrcFormat);		

		DrvSPU_SetVolume(0x3232);
		DrvSPU_StartPlay();

    while(1);	
		
	return(0);
}

