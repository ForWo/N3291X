#include <stdio.h>
#include <string.h>
#include "wblib.h"
#include "W55FA95_SIC.h"
#include "W55FA95_adc.h"
#include "W55FA95_edma.h"
#include "nvtfat.h"
#include "DrvEDMA.h"

#define DBG_PRINTF			sysprintf
#define AUDIO_REC_SEC		10


__align(32) INT16 g_pi16SampleBuf[16000*AUDIO_REC_SEC];		/* Keep max 16K sample rate */

char WaveHeader[]= {'R', 'I', 'F', 'F', 0x00, 0x00, 0x00, 0x00,	   //ForthCC code+(RAW-data-size+0x24)	
					'W', 'A', 'V', 'E', 'f', 'm', 't', ' ',			
					0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,//Chunk-size, audio format, and NUMChannel
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//Sample-Rate and Byte-Count-Per-Sec 
					0x02, 0x00, 0x10, 0x00,						   //Align and Bits-per-sample.
					'd', 'a', 't', 'a', 0x00, 0x00, 0x00, 0x00};   //"data"and RAW-data-size		
						
INT32 AudioWriteFile(char* szAsciiName, 
					PUINT16 pu16BufAddr, 
					UINT32 u32Length,
					UINT32 u32SampleRate)
{
	INT hFile, i32Errcode, u32WriteLen;
	char suFileName[256];
	UINT32 *pu32ptr; 
	
	strcat(szAsciiName, ".wav");
	fsAsciiToUnicode(szAsciiName, suFileName, TRUE);
	hFile = fsOpenFile(suFileName, NULL, O_CREATE);
	if (hFile < 0)
	{
		DBG_PRINTF("Fail to creating file \n");					
		return hFile;
	}	
	else
		DBG_PRINTF("Succeed to creating file \n");	
	pu32ptr =  (UINT32*)(WaveHeader+4);
	*pu32ptr = u32Length+0x24;		
	pu32ptr = (UINT32*)(WaveHeader+0x28); //(UINT32*)WaveHeader[0x28];
	*pu32ptr = u32Length;
	pu32ptr = (UINT32*)(WaveHeader+0x18);//(UINT32*)WaveHeader[0x10];
	*pu32ptr = u32SampleRate;
	pu32ptr = (UINT32*)(WaveHeader+0x1C);//(UINT32*)WaveHeader[0x14];
	*pu32ptr = u32SampleRate*2;				//16bits 
	i32Errcode = fsWriteFile(hFile, 
						(UINT8 *)WaveHeader, 
						0x2C, 
						&u32WriteLen);	
	if (i32Errcode < 0)
		return i32Errcode;											
	i32Errcode = fsWriteFile(hFile, 
							(UINT8 *)pu16BufAddr, 
							u32Length, 
							&u32WriteLen);
	if (i32Errcode < 0)
		return i32Errcode;	
		
	fsCloseFile(hFile);
	return Successful;	
}

INT32 AudioWriteFileHead(char* szAsciiName, 
					UINT32 u32Length,
					UINT32 u32SampleRate)
{
	INT hFile, i32Errcode, u32WriteLen;
	char suFileName[256];
	UINT32 *pu32ptr; 
	
	strcat(szAsciiName, ".wav");
	fsAsciiToUnicode(szAsciiName, suFileName, TRUE);
	hFile = fsOpenFile(suFileName, NULL, O_CREATE);
	if (hFile < 0)
	{
		DBG_PRINTF("Fail to creating file \n");					
		return hFile;
	}	
	else
		DBG_PRINTF("Succeed to creating file \n");	
#if 0		
	pu32ptr =  (UINT32*)(WaveHeader+4);
	*pu32ptr = u32Length+0x24;		
	pu32ptr = (UINT32*)(WaveHeader+0x28); //(UINT32*)WaveHeader[0x28];
	*pu32ptr = u32Length;
	pu32ptr = (UINT32*)(WaveHeader+0x18);//(UINT32*)WaveHeader[0x10];
	*pu32ptr = u32SampleRate;
	pu32ptr = (UINT32*)(WaveHeader+0x1C);//(UINT32*)WaveHeader[0x14];
	*pu32ptr = u32SampleRate*2;				//16bits 
#else
	pu32ptr =  (UINT32*) ((UINT32)(WaveHeader+4) | 0x80000000);
	*pu32ptr = u32Length+0x24;		
	pu32ptr = (UINT32*)((UINT32)(WaveHeader+0x28) | 0x80000000) ; //(UINT32*)WaveHeader[0x28];
	*pu32ptr = u32Length;
	pu32ptr = (UINT32*)((UINT32)(WaveHeader+0x18) | 0x80000000);//(UINT32*)WaveHeader[0x10];
	*pu32ptr = u32SampleRate;
	pu32ptr = (UINT32*)((UINT32)(WaveHeader+0x1C) | 0x80000000);//(UINT32*)WaveHeader[0x14];
	*pu32ptr = u32SampleRate*2;				//16bits 
#endif	
	i32Errcode = fsWriteFile(hFile, 
							(UINT8 *)((UINT32)WaveHeader | 0x80000000), 
							0x2C, 
							&u32WriteLen);	
	return hFile;						
}							
INT32 AudioWriteFileData(INT hFile, UINT16* pu16BufAddr, UINT32 u32Length)
{
	INT i32Errcode, u32WriteLen;
	i32Errcode = fsWriteFile(hFile, 
						(UINT8 *)pu16BufAddr, 
						u32Length, 
						&u32WriteLen);
	if(i32Errcode<0)
		return i32Errcode;
	else
		return Successful;						
}
INT32 AudioWriteFileClose(INT hFile)
{
	fsCloseFile(hFile);
	return Successful;	
}
/*
static void pfnRecordCallback(void)
{
	g_i8PcmReady = TRUE;
}
*/
volatile BOOL bIsBufferDone=0;
void edmaCallback(UINT32 u32WrapStatus)
{
	UINT32 u32Period, u32Attack, u32Recovery, u32Hold;
	if(u32WrapStatus==256)
	{
		bIsBufferDone = 1;
		//sysprintf("I %d\n\n", bIsBufferDone);
	}	
	else if(u32WrapStatus==1024)
	{
		
		bIsBufferDone = 2;		
		//sysprintf("I %d\n\n", bIsBufferDone);
	}
	/* AGC response speed */
	ADC_GetAutoGainTiming(&u32Period, &u32Attack, &u32Recovery, &u32Hold);
	if(u32Period<128)
	{		
		u32Period = u32Period+16;
		ADC_SetAutoGainTiming(u32Period, u32Attack, u32Recovery, u32Hold);		
	}
}

/*=================================================================
	Convert 10 bit ADC data to 16 bit sign audio data
	
	ADC RAW 10 bit from 0 ~ 1023. 
	Generally, the audio from miscrophone is 20 ~ 30 mv. It is very small. 	
	1. Force the DC level to middle of 3.3 v. ==>Add 512. (The analog part add the DC bise) 
=================================================================*/
static INT32 nIdx=1;
#define E_AUD_BUF 16000
#define CLIENT_ADC_NAME 
int edma_channel=0;
int initEDMA(void)
{
	int i;
	EDMA_Init();	
#if 1
	i = PDMA_FindandRequest(CLIENT_ADC_NAME); //w55fa95_edma_request
#else
	for (i = 4; i >=1; i--)
	{
		if (!EDMA_Request(i, CLIENT_ADC_NAME))
			break;
	}
#endif

//	if(i == -ENODEV)
//		return -ENODEV;

	edma_channel = i;
	EDMA_SetAPB(edma_channel,			//int channel, 
						eDRVEDMA_ADC,			//E_DRVEDMA_APB_DEVICE eDevice, 
						eDRVEDMA_READ_APB,		//E_DRVEDMA_APB_RW eRWAPB, 
						eDRVEDMA_WIDTH_32BITS);	//E_DRVEDMA_TRANSFER_WIDTH eTransferWidth	

	EDMA_SetupHandlers(edma_channel, 		//int channel
						eDRVEDMA_WAR, 			//int interrupt,	
						edmaCallback, 				//void (*irq_handler) (void *),
						NULL);					//void *data

	EDMA_SetWrapINTType(edma_channel , 
								eDRVEDMA_WRAPAROUND_EMPTY | 
								eDRVEDMA_WRAPAROUND_HALF);	//int channel, WR int type

	EDMA_SetDirection(edma_channel , eDRVEDMA_DIRECTION_FIXED, eDRVEDMA_DIRECTION_WRAPAROUND);


	EDMA_SetupSingle(edma_channel,		// int channel, 
								0xB800E020,		// unsigned int src_addr,  (ADC data port physical address) 
								(UINT32)g_pi16SampleBuf, //phaddrrecord,		// unsigned int dest_addr,
								E_AUD_BUF);	// unsigned int dma_length /* Lenth equal 2 half buffer */

	return Successful;
}
void releaseEDMA(void)
{
	if(edma_channel!=0)
	{
		EDMA_Free(edma_channel);
		edma_channel = 0;
	}		
}

INT32 AudioRecord(UINT32 u32SampleRate)
{	
	CHAR    	szFileName[128];	
        INT32 hFile;
        UINT32 u32Length=0;

	audio_open(eSYS_APLL, u32SampleRate);

	initEDMA();
	outp32(REG_AUDADC_CTL, inp32(REG_AUDADC_CTL) | MIC_BIAS_EN);													     	    
	audio_startrecord(); 	
	sprintf(szFileName, "C:\\Audio_%04d", nIdx);	
	
	
	//outp32(REG_AUDADC_CTL, inp32(REG_AUDADC_CTL)|MIC_BIAS_EN);
	
	hFile = AudioWriteFileHead(szFileName,							
							u32SampleRate*2*AUDIO_REC_SEC,		
							u32SampleRate);
	DrvEDMA_CHEnablelTransfer(eDRVEDMA_CHANNEL_1);
	bIsBufferDone = 0;
	do
	{
		if(bIsBufferDone==1)
		{		
			AudioWriteFileData(hFile,
							(UINT16*)(((UINT32)g_pi16SampleBuf+E_AUD_BUF/2) |0x80000000),
							E_AUD_BUF/2);							
			u32Length = u32Length+E_AUD_BUF/2;		
			bIsBufferDone = 0;		
		}
		else if(bIsBufferDone==2)
		{		
			
			AudioWriteFileData(hFile,
							(PUINT16)((UINT32)(&g_pi16SampleBuf) | 0x80000000),
							E_AUD_BUF/2);
							
			u32Length = u32Length+E_AUD_BUF/2;	
			bIsBufferDone = 0;
		}		
	}while(u32Length<(u32SampleRate*2*AUDIO_REC_SEC));
//	}while(1);
	AudioWriteFileClose(hFile);
	audio_stoprecord();   
	releaseEDMA();
	audio_close();
	sysprintf("Close File\n");
	nIdx = nIdx+1;			    
    	return Successful;
}
