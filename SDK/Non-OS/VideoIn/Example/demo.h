#ifndef _VIDEOIN_DEMO
#define _VIDEOIN_DEMO

#define OPT_UART
#ifdef OPT_UART
#define DBG_PRINTF		sysprintf
#else
#define DBG_PRINTF		sysprintf
#endif

//Choice sensor type. It deteminate the cropping size. 
//#define OV9660_VGA	
//#define OV9660_SXGA
//#define OV7725_VGA
//#define KLE_303B
//#define NT99141_VGA
//#define NT99141_HD
//#define OV7670_VGA
#define NT99050_VGA
//#define GC0308_VGA
//#define OV9665_VGA	
//#define OV9665_SXGA

#if defined(OV9660_VGA)||defined(OV7725_VGA)||defined(SA71113)||defined(WT8861)||defined(KLE_303B)\
	|| defined(NT99141_VGA) || defined(OV7670_VGA) || defined(NT99050_VGA)|| defined(GC0308_VGA) || defined(OV9665_VGA)
#define OPT_CROP_WIDTH		640
#define OPT_CROP_HEIGHT		480
#define ENCODE_VGA_DIMENSION
#endif
#if defined(OV2640_SVGA)
#define OPT_CROP_WIDTH		800
#define OPT_CROP_HEIGHT		600	
#define ENCODE_SVGA_DIMENSION
#endif
#if defined(OV2640_UXGA)
#define OPT_CROP_WIDTH		1600
#define OPT_CROP_HEIGHT		1200	
#define ENCODE_UXGA_DIMENSION
#endif
#if defined(OV9660_SXGA) ||  defined(OV9665_SXGA)
#define ENCODE_SXGA_DIMENSION
#define OPT_CROP_WIDTH		1280
#define OPT_CROP_HEIGHT		960
#endif
#if defined(NT99141_HD)
#define ENCODE_HD_DIMENSION
#define OPT_CROP_WIDTH		1280
#define OPT_CROP_HEIGHT		720
#endif

#if defined(OV3642_QXGA)
#define ENCODE_QXGA_DIMENSION
#define OPT_CROP_WIDTH		2048
#define OPT_CROP_HEIGHT		1536
#endif 

#if defined(VPG_QXGA)
#define ENCODE_QXGA_DIMENSION
#define OPT_CROP_WIDTH		2048
#define OPT_CROP_HEIGHT		1536
#endif 

/* Choice one */
#ifdef ENCODE_QVGA_DIMENSION	
	#define	OPT_ENCODE_WIDTH		320			//35K pixel
	#define	OPT_ENCODE_HEIGHT		240	
#endif
#ifdef ENCODE_VGA_DIMENSION
	#define	OPT_ENCODE_WIDTH		640			//35K pixel
	#define	OPT_ENCODE_HEIGHT		480	
#endif
#ifdef ENCODE_SVGA_DIMENSION
	#define	OPT_ENCODE_WIDTH		800			//
	#define	OPT_ENCODE_HEIGHT		600	
#endif
#ifdef ENCODE_SXGA_DIMENSION
	#define	OPT_ENCODE_WIDTH		1280		//1.3M pixel 
	#define	OPT_ENCODE_HEIGHT		960	
#endif
#ifdef ENCODE_HD_DIMENSION
	#define	OPT_ENCODE_WIDTH		1280		//1280x720 pixel 
	#define	OPT_ENCODE_HEIGHT		720	
#endif 
#ifdef ENCODE_UXGA_DIMENSION
	#define	OPT_ENCODE_WIDTH		1600		//2M pixel
	#define	OPT_ENCODE_HEIGHT		1200
#endif
#ifdef ENCODE_QXGA_DIMENSION
	#define	OPT_ENCODE_WIDTH		2048		//3M pixel
	#define	OPT_ENCODE_HEIGHT		1536		
#endif

/* Choice one */
#ifdef __TV__
#define OPT_STRIDE				640
#define OPT_LCM_WIDTH			640
#define OPT_LCM_HEIGHT		480
#define OPT_PREVIEW_WIDTH		640
#define OPT_PREVIEW_HEIGHT		480
#elif defined(__LCM_320x240__)
#define OPT_STRIDE				320
#define OPT_LCM_WIDTH			320
#define OPT_LCM_HEIGHT		240
#define OPT_PREVIEW_WIDTH		320
#define OPT_PREVIEW_HEIGHT		240
#elif defined(__LCM_480x272__)
#define OPT_STRIDE				480
#define OPT_LCM_WIDTH			480
#define OPT_LCM_HEIGHT		272
#define OPT_PREVIEW_WIDTH		366
#define OPT_PREVIEW_HEIGHT		272
#elif defined(__LCM_800x480__)
#define OPT_STRIDE				800
#define OPT_LCM_WIDTH			800
#define OPT_LCM_HEIGHT		480
#define OPT_PREVIEW_WIDTH		640
#define OPT_PREVIEW_HEIGHT		480
#endif


void VideoIn_InterruptHandler(void);


extern UINT8 u8PlanarFrameBuffer[];
extern INT getFitPreviewDimension(UINT32 u32Lcmw,
						UINT32 u32Lcmh,
						UINT32 u32Patw,
						UINT32 u32Path,  
						UINT32* pu32Previewwidth, 
						UINT32* pu32Previewheight);


UINT32 Smpl_OV9660_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_OV9660_SXGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_OV7725_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_KLE303B_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_NT99141_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);						
UINT32 Smpl_NT99141_HD(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);						
UINT32 Smpl_OV7670_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);	
UINT32 Smpl_GC0308_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);	
void PacketZooming(void);


//void Delay(UINT32 nCount);

//Smpl_VPOST.C
void InitVPOST(UINT8* pu8FrameBuffer);

//demo.C
void VideoIn_InterruptHandler(void);
void CoWork_VideoIn_InterruptHandler(void);
UINT32 VideoIn_GetCurrFrameCount(void);
void VideoIn_ClearFrameCount(void);

//FrameSyn.c
VOID pfnFSC_Ch0_ReadSwitchCallback(VOID);
VOID pfnFSC_Ch0_WriteSwitchCallback(VOID);
VOID pfnFSC_Ch0_ReadErrorCallback(VOID);
VOID pfnFSC_Ch0_WriteErrorCallback(VOID);

//Smpl_I2C.C
BOOL 
I2C_Write_8bitSlaveAddr_8bitReg_8bitData(
	UINT8 uAddr, 
	UINT8 uRegAddr, 
	UINT8 uData	
);
UINT8 
I2C_Read_8bitSlaveAddr_8bitReg_8bitData(
	UINT8 uAddr, 
	UINT8 uRegAddr
);

void LcmPowerInit(void);
void LcmPowerEnable(void);
void LcmBacklightInit(void);	
void LcmBacklightEnable(void);

#define __60HZ__
//#define __50HZ__

	#if 0//From Wiki
	1 Video graphics array 
		1.1 QQVGA (160กั120) 
		1.2 HQVGA (240กั160) 
		1.3 QVGA (320กั240) 
		1.4 WQVGA (432กั240) 
		1.5 HVGA (480กั320) 
		1.6 VGA (640กั480) 
		1.7 WVGA (800กั480) 
		1.8 FWVGA (854กั480) 
		1.9 SVGA (800กั600) 
		1.10 WSVGA (1024กั576/600) 

	2 Extended graphics array 
		2.1 XGA (1024กั768) 
		2.2 WXGA (1280กั768) 
		2.3 XGA+ (1152กั864) 
		2.4 WXGA+ (1440กั900) 
		2.5 SXGA (1280กั1024) 
		2.6 SXGA+ (1400กั1050) 
		2.7 WSXGA+ (1680กั1050) 
		2.8 UXGA (1600กั1200) 
		2.9 WUXGA (1920กั1200) 
	3 Quad-extended graphics array 
		3.1 QWXGA (2048กั1152) 
		3.2 QXGA (2048กั1536) 
		3.3 WQXGA (2560กั1600) 
		3.4 QSXGA (2560กั2048) 
		3.5 WQSXGA (3200กั2048) 
		3.6 QUXGA (3200กั2400) 
		3.7 WQUXGA (3840กั2400) 
	4 Hyper-extended graphics array 
		4.1 HXGA (4096กั3072) 
		4.2 WHXGA (5120กั3200) 
		4.3 HSXGA (5120กั4096) 
		4.4 WHSXGA (6400กั4096) 
		4.5 HUXGA (6400กั4800) 
		4.6 WHUXGA (7680กั4800) 
	5 Multiples of 720 and 1080 
		5.1 nHD (640กั360) 
		5.2 qHD (960กั540) 
		5.3 WQHD (2560กั1440) 
		5.4 QFHD (3840กั2160) 
	#endif

#endif /* !_VIDEOIN_DEMO */