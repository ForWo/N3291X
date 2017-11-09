/***************************************************************************
 *                                                                         									     *
 * Copyright (c) 2008 Nuvoton Technolog. All rights reserved.              					     *
 *                                                                         									     *
 ***************************************************************************/

#include <stdio.h>
#include "wblib.h"
#include "W55FA95_GPIO.h"
#include "W55FA95_VideoIn.h"
#include "demo.h"
#include "nvtfat.h"
#include "W55FA95_SIC.h"


__align(32) UINT8 u8PacketFrameBuffer[OPT_LCM_WIDTH*OPT_LCM_HEIGHT*2];		//Keep 640*480*2 RGB565 frame buffer
__align(32) UINT8 u8PacketFrameBuffer1[OPT_LCM_WIDTH*OPT_LCM_HEIGHT*2];		//Keep 640*480*2 RGB565 frame buffer
__align(32) UINT8 u8PacketFrameBuffer2[OPT_LCM_WIDTH*OPT_LCM_HEIGHT*2];		//Keep 640*480*2 RGB565 frame buffer
__align(32) UINT8 u8PlanarFrameBuffer[OPT_ENCODE_WIDTH*(OPT_ENCODE_HEIGHT)*2];		//Keep 640x480*2 PlanarYUV422 frame buffer


volatile UINT32 g_u32FrameCount = 0;
BOOL bIsFrameBuffer0=0,  bIsFrameBuffer1=0, bIsFrameBuffer2=0; /* 0 means buffer is clean */
UINT32 u32VideoInIdx = 0;


void CoWork_VideoIn_InterruptHandler(void)
{

}

void VideoIn_InterruptHandler(void)
{
#if 1
	switch(u32VideoInIdx)
	{//Current Frame
		case 0:		
			if(bIsFrameBuffer1==0)
			{/* Change frame buffer 1 if Frame Buffer 1 is clean, Otherwise, do nothing */	
					
				videoinIoctl(VIDEOIN_IOCTL_SET_BUF_ADDR,
								eVIDEOIN_PACKET,			
								0, 					
								(UINT32)((UINT32)u8PacketFrameBuffer1 + (OPT_STRIDE-OPT_PREVIEW_WIDTH)/2*2) );															
			 	bIsFrameBuffer0 = 1; u32VideoInIdx = 1; 
			}
				
				break; 	 				
		case 1:		
			if(bIsFrameBuffer2==0)
			{/* Change frame buffer 2 if Frame Buffer 2 is clean, Otherwise, do nothing */							
				videoinIoctl(VIDEOIN_IOCTL_SET_BUF_ADDR,
								eVIDEOIN_PACKET,			
								0, 					
								(UINT32)((UINT32)u8PacketFrameBuffer2 + (OPT_STRIDE-OPT_PREVIEW_WIDTH)/2*2) );											
 				bIsFrameBuffer1 = 1; u32VideoInIdx = 2;
			}	
				break; 	 				
		case 2:		
			if(bIsFrameBuffer0==0)
			{/* Change frame buffer 0 if Frame Buffer 0 is clean, Otherwise, do nothing */
	
				videoinIoctl(VIDEOIN_IOCTL_SET_BUF_ADDR,
								eVIDEOIN_PACKET,			
								0, 					
								(UINT32)((UINT32)u8PacketFrameBuffer + (OPT_STRIDE-OPT_PREVIEW_WIDTH)/2*2) );	
				bIsFrameBuffer2 = 1; u32VideoInIdx = 0; 
			}	
				break; 	 
	}
	videoinIoctl(VIDEOIN_IOCTL_SET_SHADOW,
				NULL,			//640/640
				NULL,
				NULL);		
#endif				
	g_u32FrameCount = g_u32FrameCount+1;
}
UINT32 VideoIn_GetCurrFrameCount(void)
{
	return g_u32FrameCount;
}
void VideoIn_ClearFrameCount(void)
{
	g_u32FrameCount = 0;
}
void Delay(UINT32 nCount)
{
	volatile UINT32 i;
	for(;nCount!=0;nCount--)
		for(i=0;i<5;i++);
}
UINT32 u3210msFlag=0;
void TimerBase(void)
{
	u3210msFlag = u3210msFlag+1;
}


void init(void)
{
	WB_UART_T uart;
	UINT32 u32ExtFreq;
	UINT32 u32Channel;
	
	/* Cache on */ 
	sysInvalidCache();
	sysDisableCache();
	sysEnableCache(CACHE_WRITE_BACK);
	//outp32(0xb0000204, 0xFFFFFFFF);
	/* Init UART */
	#if 1
	u32ExtFreq = sysGetExternalClock();
	#else
	u32ExtFreq = 27000000;
	#endif
	sysUartPort(1);
	uart.uiFreq = u32ExtFreq;	//use Ext clock
	uart.uiBaudrate = 115200;
	uart.uiDataBits = WB_DATA_BITS_8;
	uart.uiStopBits = WB_STOP_BITS_1;
	uart.uiParity = WB_PARITY_NONE;
	uart.uiRxTriggerLevel = LEVEL_1_BYTE;
	uart.uart_no = WB_UART_0;
	sysInitializeUART(&uart);
	sysprintf("UART Init\n");
	/* Init Timer */
	u32ExtFreq = sysGetExternalClock();	
	sysSetTimerReferenceClock(TIMER0, u32ExtFreq); //External Crystal	
	sysStartTimer(TIMER0, 100, PERIODIC_MODE);		/* 100 ticks/per sec ==> 1tick/10ms */	
	u32Channel = sysSetTimerEvent(TIMER0, 1, (PVOID)TimerBase);	/* 1 ticks=10ms call back */	
	//sysSetLocalInterrupt(ENABLE_FIQ_IRQ);			
		
}

INT getFitPreviewDimension(UINT32 u32Lcmw,
						UINT32 u32Lcmh,
						UINT32 u32Patw,
						UINT32 u32Path,  
						UINT32* pu32Previewwidth, 
						UINT32* pu32Previewheight)				
{//Assume sensor aspect ratio is 4:3
	float lcmwidth, lcmheight;
	int prewidth, preheight;
	float aspect1=(float)u32Path/u32Patw;	
	float invaspect1=(float)u32Patw/u32Path;		
	float aspect2;
	

	lcmwidth = (float)u32Lcmw;
	lcmheight = (float)u32Lcmh;
	aspect2 = lcmheight/lcmwidth;
	if(aspect2>=aspect1)
	{
		
		prewidth = lcmwidth; 
	
		if( (((int)(lcmwidth*aspect1))%4!=0) && ((((int)lcmwidth*aspect1)/4+1)*4<=640) )
			preheight = (INT32)((lcmwidth*aspect1)/4+1)*4;
		else
			preheight = (INT32)((lcmwidth*aspect1)/4)*4;		
		
	}
	else
	{
		DBG_PRINTF("Domenonate is height, fixed height\n");
		preheight = lcmheight; 
		if( (((int)(lcmheight*invaspect1))%4!=0) && ((((int)lcmheight*invaspect1)/4+1)*4<=480) )
			prewidth = (INT32)((lcmheight*invaspect1)/4+1)*4;
		else
			prewidth = (INT32)((lcmheight*invaspect1)/4)*4;		
	}	
	

	*pu32Previewwidth = prewidth;
	*pu32Previewheight = preheight;

	sysprintf("target width, height= (%d, %d)\n", *pu32Previewwidth , *pu32Previewheight);	
	return Successful;
}

int main()
{
	UINT32 u32Item;
	
	init();			 	
	DBG_PRINTF("================================================================\n");
	DBG_PRINTF("Please use TV as output device														\n");    	
	DBG_PRINTF("================================================================\n");

	do
	{    	
		DBG_PRINTF("================================================================\n");
		DBG_PRINTF("				VideoIn library demo code												\n");
		DBG_PRINTF(" [1] OV9660 VGA demo 															\n");
		DBG_PRINTF(" [2] OV9660 SXGA demo 															\n");
		DBG_PRINTF(" [3] OV7725 demo 																\n");
		DBG_PRINTF(" [4] LGE303B demo 	(Burn to NAND flash due to pin conflict with ICE)							\n");
		DBG_PRINTF(" [5] YS-1M55 demo board. NT99141 VGA demo 											\n");
		DBG_PRINTF(" [6] YS-1M55 demo board. NT99141 HD demo 											\n");
		DBG_PRINTF(" [7] YS-1M55 demo board. OV7670 VGA demo 											\n");
		DBG_PRINTF(" [8] YS-1M55 demo board. NT99050 VGA demo 											\n");
		DBG_PRINTF(" [9] YS-1M55 demo board. GC0308 VGA demo 											\n");
		DBG_PRINTF("================================================================\n");
#ifdef OPT_UART
		u32Item = sysGetChar();
		//u32Item = 'E';
#else
		scanf("%c", &u32Item);	
#endif

		switch(u32Item)
		{
			case '1':	
						sysprintf("Enable conditional compile  [OV9660_VGA]\n");					
					#ifdef __1ST_PORT__ 
						sysprintf("Plug in sensor OV9660 to GPB\n");	
						videoIn_Port(0); 
					#endif
					#ifdef __2ND_PORT__
						sysprintf("Plug in sensor OV9660 to GPC\n");	
						videoIn_Port(1); 
					#endif						
						Smpl_OV9660_VGA(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);		
						break; 	
												
			case '2': 		
						sysprintf("Enable conditional compile  [OV9660_SXGA]\n");
						sysprintf("Plug in sensor OV9660\n");
					#ifdef __1ST_PORT__ 
						sysprintf("Plug in sensor OV9660 to GPB\n");	
						videoIn_Port(0); 
					#endif
					#ifdef __2ND_PORT__
						sysprintf("Plug in sensor OV9660 to GPC\n");	
						videoIn_Port(1); 
					#endif	
						Smpl_OV9660_SXGA(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);		
						PacketZooming();
						break; 
			case '3':
						sysprintf("Enable conditional compile  [OV7225_VGA]\n");
						sysprintf("Plug in sensor OV7225\n");
					#ifdef __1ST_PORT__ 
						sysprintf("Plug in sensor OV7225 to GPB\n");	
						videoIn_Port(0); 
					#endif
					#ifdef __2ND_PORT__
						sysprintf("Plug in sensor OV7225 to GPC\n");	
						videoIn_Port(1); 
					#endif						
						Smpl_OV7725_VGA(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);						
						break;
			case '4':
					
						LcmPowerInit();
						LcmPowerEnable();
						LcmBacklightInit();	
						LcmBacklightEnable();	
																						
						sysprintf("Enable conditional compile  [KLE303B]\n");
						sysprintf("Plug in sensor KLE303B\n");						
					#ifdef __1ST_PORT__ 
						sysprintf("Plug in sensor KLE303B to GPB\n");	
						videoIn_Port(0); 
					#endif
					#ifdef __2ND_PORT__
						sysprintf("Plug in sensor KLE303B to GPC\n");	
						videoIn_Port(1); 
					#endif	
						Smpl_KLE303B_VGA(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);						
						break;			
			case '5':
						sysprintf("Enable conditional compile  [NT99141_VGA]\n");
						sysprintf("Plug in sensor NT99141\n");
						videoIn_Close();
					#ifdef __1ST_PORT__ 
						sysprintf("Plug in sensor NT99141 to GPB\n");	
						videoIn_Port(0); 
					#endif
					#ifdef __2ND_PORT__
						sysprintf("Plug in sensor NT99141 to GPC\n");	
						videoIn_Port(1); 
					#endif													
						Smpl_NT99141_VGA(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);						
						break;	
			case '6':
						sysprintf("Enable conditional compile  [NT99141_HD]\n");
						sysprintf("Plug in sensor NT99141\n");
					#ifdef __1ST_PORT__ 
						sysprintf("Plug in sensor NT99141 to GPB\n");	
						videoIn_Port(0); 
					#endif
					#ifdef __2ND_PORT__
						sysprintf("Plug in sensor NT99141 to GPC\n");	
						videoIn_Port(1); 
					#endif						
						Smpl_NT99141_HD(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);						
						break;				
			case '7':
						sysprintf("Enable conditional compile  [OV7670]\n");
						sysprintf("Plug in sensor OV7670\n");
					#ifdef __1ST_PORT__ 
						sysprintf("Plug in sensor OV7670 to GPB\n");	
						videoIn_Port(0); 
					#endif
					#ifdef __2ND_PORT__
						sysprintf("Plug in sensor OV7670 to GPC\n");	
						videoIn_Port(1); 
					#endif						
						Smpl_OV7670_VGA(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);						
						break;	
			case '8':
						sysprintf("Enable conditional compile  [NT99050]\n");
						sysprintf("Plug in sensor NT99050\n");
					#ifdef __1ST_PORT__ 
						sysprintf("Plug in sensor NT99050 to GPB\n");	
						videoIn_Port(0); 
					#endif
					#ifdef __2ND_PORT__
						sysprintf("Plug in sensor NT99050 to GPC\n");	
						videoIn_Port(1); 
					#endif						
						Smpl_NT99050(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);	
						break;	
			case '9':
						sysprintf("Enable conditional compile  [GC0308]\n");
						sysprintf("Plug in sensor GC0308\n");
					#ifdef __1ST_PORT__ 
						sysprintf("Plug in sensor GC0308 to GPB\n");	
						videoIn_Port(0); 
					#endif
					#ifdef __2ND_PORT__
						sysprintf("Plug in sensor GC0308 to GPC\n");	
						videoIn_Port(1); 
					#endif						
						Smpl_GC0308_VGA(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);	
						break;	
			case 'a':	
						sysprintf("Enable conditional compile  [OV9660_VGA]\n");					
					#ifdef __1ST_PORT__ 
						sysprintf("Plug in sensor OV9660 to GPB\n");	
						videoIn_Port(0); 
					#endif
					#ifdef __2ND_PORT__
						sysprintf("Plug in sensor OV9660 to GPC\n");	
						videoIn_Port(1); 
					#endif						
						Smpl_OV9665_VGA(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);		
						break; 	
												
			case 'b': 		
						sysprintf("Enable conditional compile  [OV9660_SXGA]\n");
						sysprintf("Plug in sensor OV9660\n");
					#ifdef __1ST_PORT__ 
						sysprintf("Plug in sensor OV9660 to GPB\n");	
						videoIn_Port(0); 
					#endif
					#ifdef __2ND_PORT__
						sysprintf("Plug in sensor OV9660 to GPC\n");	
						videoIn_Port(1); 
					#endif	
						Smpl_OV9665_SXGA(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);		
						break; 															
		}		
	}while((u32Item!= 'q') || (u32Item!= 'Q'));	
	
    	return 0;
} /* end main */
