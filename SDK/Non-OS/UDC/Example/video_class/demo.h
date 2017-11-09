#ifndef _VIDEOIN_DEMO
#define _VIDEOIN_DEMO


#define OPT_UART
#ifdef OPT_UART
#define DBG_PRINTF		sysprintf
#else
#define DBG_PRINTF		printf
#endif

#define	OPT_ENCODE_WIDTH		640			//35K pixel
#define	OPT_ENCODE_HEIGHT		480	
#define OPT_STRIDE				480
#define OPT_LCM_WIDTH			640
#define OPT_LCM_HEIGHT			480

#define OPT_CROP_WIDTH		640
#define OPT_CROP_HEIGHT		480
#define OPT_PREVIEW_WIDTH		366
#define OPT_PREVIEW_HEIGHT		272

void VideoIn_InterruptHandler(void);

UINT32 Smpl_NT99141_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1);

/* Buffer for Packet & Planar format */
extern UINT8 u8PlanarFrameBuffer0[];
extern UINT8 u8PlanarFrameBuffer1[];
extern UINT8 u8PlanarFrameBuffer2[];
extern UINT8 u8PacketFrameBuffer0[];
extern UINT8 u8PacketFrameBuffer1[];
extern UINT8 u8PacketFrameBuffer2[];

/* Current Width & Height */
extern UINT16 u16CurWidth, u16CurHeight;

void Delay(UINT32 nCount);

/* UVC Main */
VOID uvc_main(VOID);
/* UVC event */
VOID uvcdEvent(VOID);
/* Change VideoIN Setting for Frame size or Buffer Address */
VOID ChangeFrame(BOOL bChangeSize, UINT32 u32Address, UINT16 u16Width,UINT16 u16Height);
/* VideoIN Buffer Address Control when Frame End */
VOID VideoInFrameEnd_InterruptHandler(VOID);
/* Get Image Buffer for USB transfer */
INT GetImageBuffer(VOID);
/* JPEG Encode function */
UINT32 jpegEncode(UINT32 u32YAddress,UINT32 u32BitstreamAddress, UINT16 u16Width,UINT16 u16Height);
/* Process Unit Control */
UINT32 ProcessUnitControl(UINT32 u32ItemSelect,UINT32 u32Value);
/* Get Image Size and Address (Image data control for Foramt and Frame)*/
INT GetImage(PUINT32 pu32Addr, PUINT32 pu32transferSize);

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

#endif /* !_VIDEOIN_DEMO */