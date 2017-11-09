#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "wblib.h"
#include "W55FA95_VideoIn.h"
#include "W55FA95_GPIO.h"
//#include "DrvI2C.h"
#include "demo.h"
#include "nvtfat.h"
#include "W55FA95_SIC.h"
/*=====================================================================
	Digitial zoom is only supported if cropping dimension bigger than display dimension.
	Now max sensor cropping dimension is 1280x960. Display dimension is 640x480. 
	The first step is downscale 1280x960 into 640x480 through videoin downscale.
	The senond step is downscale ((1280-8), (960-6)) into 640x480.
	.....
	The last step is downscale (640x480) into 640x480	
=====================================================================*/
void PacketZooming(void)
{//Packet YUV422/RGB565 width must multiple of 2. Y only must be multiple of 4. Planar format's width must multiple of 8
	UINT32 u32CropW, u32CropH;
	UINT32 u32PreW, u32PreH;
	UINT32 u32CropStartX=4, u32CropStartY = 0;
	UINT32 u32FramePass;
	
	u32CropW = OPT_CROP_WIDTH;
	u32CropH = OPT_CROP_HEIGHT;

	getFitPreviewDimension(OPT_LCM_WIDTH,
						OPT_LCM_HEIGHT,
						OPT_CROP_WIDTH,
						OPT_CROP_HEIGHT,
						&u32PreW,
						&u32PreH);
	
	if(OPT_CROP_WIDTH<= u32PreW)
	{
		sysprintf("Sensor input width should bigger than preview width at least 8 pixels\n");
		return ;
	}	
	if(OPT_CROP_HEIGHT<= u32PreH)
	{
		sysprintf("Sensor input height should bigger than preview height at least 6 pixels\n");
		return ;
	}
	

	while(1)
	{
		while( (u32CropW>=u32PreW)&& u32CropH >= u32PreH)
		{		
			videoinIoctl(VIDEOIN_IOCTL_CROPPING_DIMENSION,				
						u32CropH,			//UINT16 u16Height, 
						u32CropW,				//UINT16 u16Width;	
						0);							//Useless
	
			videoinIoctl(VIDEOIN_IOCTL_VSCALE_FACTOR,
						eVIDEOIN_PACKET,			
						OPT_PREVIEW_HEIGHT,
						u32CropH);		
//			u32GCD = GCD(OPT_PREVIEW_WIDTH, u32CropW);																
			videoinIoctl(VIDEOIN_IOCTL_HSCALE_FACTOR,
						eVIDEOIN_PACKET,			
						OPT_PREVIEW_WIDTH,
						u32CropW);	

			videoinIoctl(VIDEOIN_IOCTL_SET_CROPPING_START_POSITION,				
				u32CropStartY,							//Vertical start position
				u32CropStartX,							//Horizontal start position	
				0);							//Useless						
						
			videoinIoctl(VIDEOIN_IOCTL_SET_SHADOW,
					NULL,			//640/640
					NULL,
					NULL);	
			sysSetLocalInterrupt(DISABLE_IRQ);		
			u32FramePass = VideoIn_GetCurrFrameCount();												
			sysSetLocalInterrupt(ENABLE_IRQ);
			while(1)
			{
				if( (VideoIn_GetCurrFrameCount()-u32FramePass)>=1)
					break;
			}						
			u32CropW = u32CropW -8;		//Assume ratio is 4:3
			u32CropH = u32CropH -6;	
			u32CropStartX = u32CropStartX +4;
			u32CropStartY = u32CropStartY +3;									
		
		}
	
		while( (u32CropW<=OPT_CROP_WIDTH)&& u32CropH <= OPT_CROP_HEIGHT)
		{		
			videoinIoctl(VIDEOIN_IOCTL_CROPPING_DIMENSION,				
						u32CropH,			//UINT16 u16Height, 
						u32CropW,				//UINT16 u16Width;	
						0);							//Useless
						
	
			videoinIoctl(VIDEOIN_IOCTL_VSCALE_FACTOR,
						eVIDEOIN_PACKET,			
						OPT_PREVIEW_HEIGHT,
						u32CropH);		
			videoinIoctl(VIDEOIN_IOCTL_HSCALE_FACTOR,
						eVIDEOIN_PACKET,			
						OPT_PREVIEW_WIDTH,
						u32CropW);
										
			videoinIoctl(VIDEOIN_IOCTL_SET_CROPPING_START_POSITION,				
				u32CropStartY,							//Vertical start position
				u32CropStartX,							//Horizontal start position	
				0);								
			videoinIoctl(VIDEOIN_IOCTL_SET_SHADOW,
					NULL,		
					NULL,
					NULL);	
			sysSetLocalInterrupt(DISABLE_IRQ);		
			u32FramePass = VideoIn_GetCurrFrameCount();												
			sysSetLocalInterrupt(ENABLE_IRQ);
			while(1)
			{
				if( (VideoIn_GetCurrFrameCount()-u32FramePass)>=1)
					break;
			}						
			u32CropW = u32CropW +8;		//Assume ratio is 4:3
			u32CropH = u32CropH +6;	
			u32CropStartX = u32CropStartX -4;
			u32CropStartY = u32CropStartY -3;									
		}	
	}
}