#include <stdio.h>
#include <string.h>

#include "wblib.h"
#include "wbio.h"
#include "w55fa95_reg.h"
#include "w55fa95_vpe.h"
#include "vdodef.h"
#include "vdoapi.h"
#include "avctest.h"
#include "w55fa95_vpe.h"

BOOL bIsVPECompleteInt = FALSE;
BOOL volatile bIsVPEMBCompleteInt = FALSE;
int volatile VPEIntNum;

void vpeCompleteCallback(void)
{
	VPEIntNum++;
	
	if (inp32(REG_VPE_INTS) & VP_INTS)
		bIsVPECompleteInt = TRUE;
	
	if (inp32(REG_VPE_INTS) & MB_INTS)
	{
		outp32(REG_VPE_CMD, inp32(REG_VPE_CMD) & ~(MB_EN | HOST_SEL));
		
		outp32(REG_VPE_INTS, MB_INTS);
		
		outp32(REG_VPE_RESET, 0x03);
		bIsVPEMBCompleteInt = TRUE;		
	}		

}

void vpeInit(void)
{
	PFN_VPE_CALLBACK OldVpeCallback;
	
	vpeOpen();	 
	vpeInstallCallback(VPE_INT_COMP,
						vpeCompleteCallback, 
						&OldVpeCallback);
						
	vpeInstallCallback(VPE_INT_MB_COMP,
						vpeCompleteCallback, 
						&OldVpeCallback);						
						
	VPEIntNum=0;					
	vpeEnableInt(VPE_INT_COMP);		
							
}


void vpeConfig(UINT Ysrc,UINT Usrc,INT Vsrc,UINT uDestAddr,int DLeftOffset, int SrcWidth,int SrcHeight,int DestWidth,int DestHeight,int stride)
{ 		

			vpeIoctl(VPE_IOCTL_HOST_OP,
						//VPE_HOST_FRAME,
						1,
						NULL,		
						NULL);

			vpeIoctl(VPE_IOCTL_SET_SRCBUF_ADDR,
						(UINT32)Ysrc,
						(UINT32)Usrc,
						(UINT32)Vsrc);
						
			vpeIoctl(VPE_IOCTL_SET_DSTBUF_ADDR,
						(UINT32)uDestAddr,
						NULL,
						NULL);	
						
			vpeIoctl(VPE_IOCTL_SET_FMT,
						VPE_SRC_PLANAR_YUV420,
						VPE_DST_PACKET_RGB565,
						0);
						
			vpeIoctl(VPE_IOCTL_SET_SRC_OFFSET,
						(UINT32)0,
						(UINT32)0,
						NULL);								
#ifndef _No_VPE_Upscale_	
			if (stride >= DestWidth)					
			{
				vpeIoctl(VPE_IOCTL_SET_DST_OFFSET,
						(UINT32) stride - DestWidth,
						(UINT32)0,
						NULL);	
			}
			else
			{
				vpeIoctl(VPE_IOCTL_SET_DST_OFFSET,
						(UINT32) 0,
						(UINT32)0,
						NULL);				
			}			
#else
			if (stride >= DestWidth)
			{
			vpeIoctl(VPE_IOCTL_SET_DST_OFFSET,
						(UINT32) DLeftOffset,							// Left Ofset
						(UINT32) stride - DLeftOffset -   DestWidth,		// Right Offset
						NULL);	
			}
			else
			{
				vpeIoctl(VPE_IOCTL_SET_DST_OFFSET,
						(UINT32) 0,
						(UINT32)0,
						NULL);				
			}			
#endif						
					
			vpeIoctl(VPE_IOCTL_SET_SRC_DIMENSION,						
						SrcWidth,
						SrcHeight,
						NULL);
			if (stride >= DestWidth)
			{			
				vpeIoctl(VPE_IOCTL_SET_DST_DIMENSION,	
							DestWidth,
							DestHeight,
							NULL);		
			}
			else
			{
				vpeIoctl(VPE_IOCTL_SET_DST_DIMENSION,	
							stride,
							DestHeight,
							NULL);			
			}						
										
			vpeIoctl(VPE_IOCTL_SET_COLOR_RANGE,
						FALSE,
						FALSE,
						NULL);			
	
}  

void vpeSetOTFCrop(int startX, int startY, int endX, int endY, int ImagWidth, int ImageHeight, int Rotate)
{
#ifdef _No_VPE_Upscale_

			vpeIoctl(VPE_IOCTL_SET_SRC_DIMENSION,						
						ImagWidth,
						ImageHeight,
						NULL);
			vpeIoctl(VPE_IOCTL_SET_DST_DIMENSION,	
						endX - startX,
						endY - startY,
						NULL);	

#else
			vpeIoctl(VPE_IOCTL_SET_SRC_DIMENSION,						
						ImagWidth,
						ImageHeight,
						NULL);
			vpeIoctl(VPE_IOCTL_SET_DST_DIMENSION,	
						LCM_WIDTH,
						LCM_HEIGHT,
						NULL);	
#endif

	//outp32(REG_VPE_CMD, inp32(REG_VPE_CMD) & ~0x400);	// Disable OTF Cropping Window	
	
	outp32(REG_VPE_FCOEF0, ((startY & 0x1FFF) << 16) | (startX & 0x1FFF));
	outp32(REG_VPE_FCOEF1, ((endY & 0x1FFF) << 16) | (endX & 0x1FFF));	

	outp32(REG_VPE_CMD, inp32(REG_VPE_CMD) | 0x400);	// Enable OTF Cropping Window	

	vpeIoctl(VPE_IOCTL_HOST_OP,
			//VPE_HOST_VDEC,
			3,
			Rotate,		
			NULL);	
	
}

void vpeSetDestOffset( int DestLeftOffset, int DestRightOffset)
{
#ifdef _No_VPE_Upscale_
			vpeIoctl(VPE_IOCTL_SET_DST_OFFSET,
						(UINT32) DestLeftOffset,
						(UINT32) DestRightOffset,
						NULL);
#else
			vpeIoctl(VPE_IOCTL_SET_DST_OFFSET,
						(UINT32) 0,
						(UINT32) 0,
						NULL);
#endif						
}


void vpeSetSrcDestOffsetRotate(int SrcLeftOffset, int SrcRightOffset, int DestLeftOffset, int DestRightOffset, int Rotate)
{
			
			vpeIoctl(VPE_IOCTL_SET_SRC_OFFSET,
						(UINT32) SrcLeftOffset,
						(UINT32) SrcRightOffset,
						NULL);
			

			vpeIoctl(VPE_IOCTL_SET_DST_OFFSET,
						(UINT32) DestLeftOffset,
						(UINT32) DestRightOffset,
						NULL);

			vpeIoctl(VPE_IOCTL_HOST_OP,
						//VPE_HOST_FRAME,
						1,
						Rotate,		
						NULL);


}
