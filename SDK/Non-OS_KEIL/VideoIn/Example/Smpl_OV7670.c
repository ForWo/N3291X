#include "wblib.h"
#include "W55FA95_VideoIn.h"
#include "W55FA95_GPIO.h"
#include "demo.h"
#include "w55fa95_i2c.h"

typedef struct
{
	UINT32 u32Width;
	UINT32 u32Height;
	char* pszFileName;
}S_VIDEOIN_REAL;
typedef struct
{
	UINT32 u32Width;
	UINT32 u32Height;
	E_VIDEOIN_OUT_FORMAT eFormat;
	char* pszFileName;
}S_VIDEOIN_PACKET_FMT;

struct OV_RegValue
{
	UINT8	u8RegAddr;		//Register Address
	UINT8	u8Value;			//Register Data
};

struct OV_RegTable{
    struct OV_RegValue *sRegTable;
	UINT32 u32TableSize;
};


#define _REG_TABLE_SIZE(nTableName)	(sizeof(nTableName)/sizeof(struct OV_RegValue))

static struct OV_RegValue g_sOV7670_RegValue[]=
{//OV7670	

  	{0x12, 0x80},{0x11, 0x80},{0x3A, 0x04},{0x12, 0x00},{0x17, 0x13},{0x18, 0x01},{0x32, 0xB6}, 
	{0x2B, 0x10},{0x19, 0x02},{0x1A, 0x7A},{0x03, 0x0F},{0x0C, 0x00},{0x3E, 0x00},{0x70, 0x3A},
	{0x71, 0x35},{0x72, 0x11},{0x73, 0xF0},{0xA2, 0x3B},{0x1E, 0x07},{0x7a, 0x1e},{0x7b, 0x09},
	{0x7c, 0x14},{0x7d, 0x29},{0x7e, 0x50},{0x7f, 0x5F},{0x80, 0x6C},{0x81, 0x79},{0x82, 0x84},
	{0x83, 0x8D},{0x84, 0x96},{0x85, 0xA5},{0x86, 0xB0},{0x87, 0xC6},{0x88, 0xD8},{0x89, 0xE9},
	{0x55, 0x00},{0x13, 0xE0},{0x00, 0x00},{0x10, 0x00},{0x0D, 0x40},{0x42, 0x00},{0x14, 0x18},
	{0xA5, 0x02},{0xAB, 0x03},{0x24, 0x48},{0x25, 0x40},{0x26, 0x82},{0x9F, 0x78},{0xA0, 0x68},
	{0xA1, 0x03},{0xA6, 0xd2},{0xA7, 0xd2},{0xA8, 0xF0},{0xA9, 0x80},{0xAA, 0x14},{0x13, 0xE5},
	{0x0E, 0x61},{0x0F, 0x4B},{0x16, 0x02},{0x21, 0x02},{0x22, 0x91},{0x29, 0x07},{0x33, 0x0B},
	{0x35, 0x0B},{0x37, 0x1D},{0x38, 0x71},{0x39, 0x2A},{0x3C, 0x78},{0x4D, 0x40},{0x4E, 0x20},
	{0x69, 0x00},{0x6B, 0x0A},{0x74, 0x10},{0x8D, 0x4F},{0x8E, 0x00},{0x8F, 0x00},{0x90, 0x00},
	{0x91, 0x00},{0x96, 0x00},{0x9A, 0x80},{0xB0, 0x84},{0xB1, 0x0C},{0xB2, 0x0E},{0xB3, 0x7e},
	{0xB1, 0x00},{0xB1, 0x0c},{0xB8, 0x0A},{0x44, 0xfF},{0x43, 0x00},{0x45, 0x4a},{0x46, 0x6c},
	{0x47, 0x26},{0x48, 0x3a},{0x59, 0xd6},{0x5a, 0xff},{0x5c, 0x7c},{0x5d, 0x44},{0x5b, 0xb4},
	{0x5e, 0x10},{0x6c, 0x0a},{0x6d, 0x55},{0x6e, 0x11},{0x6f, 0x9e},{0x6A, 0x40},{0x01, 0x40},
	{0x02, 0x40},{0x13, 0xf7},{0x4f, 0x78},{0x50, 0x72},{0x51, 0x06},{0x52, 0x24},{0x53, 0x6c},
	{0x54, 0x90},{0x58, 0x1e},{0x62, 0x08},{0x63, 0x10},{0x64, 0x08},{0x65, 0x00},{0x66, 0x05},
	{0x41, 0x08},{0x3F, 0x00},{0x75, 0x44},{0x76, 0xe1},{0x4C, 0x00},{0x77, 0x01},{0x3D, 0xC2},
	{0x4B, 0x09},{0xC9, 0x60},{0x41, 0x18},{0x56, 0x40},{0x34, 0x11},{0x3b, 0x02},{0xa4, 0x89},
	{0x92, 0x00},{0x96, 0x00},{0x97, 0x30},{0x98, 0x20},{0x99, 0x20},{0x9A, 0x84},{0x9B, 0x29},
	{0x9C, 0x03},{0x9D, 0x99},{0x9E, 0x7F},{0x78, 0x00},{0x94, 0x08},{0x95, 0x0D},{0x79, 0x01},
	{0xc8, 0xf0},{0x79, 0x0f},{0xc8, 0x00},{0x79, 0x10},{0xc8, 0x7e},{0x79, 0x0a},{0xc8, 0x80},
	{0x79, 0x0b},{0xc8, 0x01},{0x79, 0x0c},{0xc8, 0x0f},{0x79, 0x0d},{0xc8, 0x20},{0x79, 0x09},
	{0xc8, 0x80},{0x79, 0x02},{0xc8, 0xc0},{0x79, 0x03},{0xc8, 0x40},{0x79, 0x05},{0xc8, 0x30},
	{0x79, 0x26},{0x3b, 0x82},{0x43, 0x02},{0x44, 0xf2}
};

static struct OV_RegTable g_OV_InitTable[] =
{//8 bit slave address, 8 bit data. 
	{0, 0},
	{0, 0},//{g_sOV6880_RegValue,	_REG_TABLE_SIZE(g_sOV6880_RegValue)},		
	{0, 0},//{g_sOV7648_RegValue,	_REG_TABLE_SIZE(g_sOV7648_RegValue)},
	{g_sOV7670_RegValue,	_REG_TABLE_SIZE(g_sOV7670_RegValue)},
	{0, 0},//{g_sOV2640_RegValue,	_REG_TABLE_SIZE(g_sOV2640_RegValue)},	
	{0, 0},//{g_sOV9660_VGA_RegValue,		_REG_TABLE_SIZE(g_sOV9660_VGA_RegValue)},
	{0, 0},//{g_sOV9660_SXGA_RegValue,		_REG_TABLE_SIZE(g_sOV9660_SXGA_RegValue)},
	{0, 0},//{g_sOV7725_VGA_RegValue,		_REG_TABLE_SIZE(g_sOV7725_VGA_RegValue)},
	{0, 0}
};

static UINT8 g_uOvDeviceID[]= 
{
	0x00,		// not a device ID
	0xc0,		// ov6680
	0x42,		// ov7648
	0x42,		// ov7670
	0x60,		// ov2640
	0x60,		// 0v9660 VGA
	0x60,		// 0v9660 SXGA
	0x42,		// 0v7725 VGA
	0x00			// not a device ID
};



#if 1
/*
	Sensor power down and reset may default control on sensor daughter board.
	Reset by RC.
	Sensor alway power on (Keep low)

*/
static void SnrReset(void)
{/* GPG11 reset:	H->L->H 	*/			
	
	gpio_configure(GPIO_PORTG, 11);					//GPIOG 10 as GPIO
	outp32(REG_GPGFUN, inp32(REG_GPGFUN) &~MF_GPG11);	
	outp32(REG_SHRPIN_TOUCH, inp32(REG_SHRPIN_TOUCH) &~MIC_BIAS_AEN);	
	gpio_setportval(GPIO_PORTG, 1<<11, 1<<11);	//GPIOG 11 set high default
	gpio_setportpull(GPIO_PORTG, 1<<11, 1<<11);	//GPIOG 11 pull-up 
	gpio_setportdir(GPIO_PORTG, 1<<11, 1<<11);	//GPIOG 11 output mode 
	sysDelay(10);			
	gpio_setportval(GPIO_PORTG, 1<<11, 0<<11);	//GPIOG 11 set low
	sysDelay(10);			
	gpio_setportval(GPIO_PORTG, 1<<11, 1<<11);	//GPIOG 11 set high
}

static void SnrPowerDown(BOOL bIsEnable)
{/* GPB4 power down, High for power down */
	gpio_configure(GPIO_PORTB, 4);					//GPIOG 4 as GPIO	
	outp32(REG_GPBFUN, inp32(REG_GPBFUN) &~MF_GPB4);	
	gpio_setportval(GPIO_PORTB, 1<<4, 1<<4);	//GPIOB 4 set high default
	gpio_setportpull(GPIO_PORTB, 1<<4, 1<<4);	//GPIOB 4 pull-up 
	gpio_setportdir(GPIO_PORTB, 1<<4, 1<<4);	//GPIOB 4 output mode 				
	if(bIsEnable)
		gpio_setportval(GPIO_PORTB, 1<<4, 1<<4);	//GPIOB 4 Set high	
	else					
		gpio_setportval(GPIO_PORTB, 1<<4, 0);	//GPIOB 4 Set k=low
}
#endif 

#define RETRY	2
VOID OV7670_Init(UINT32 nIndex)
{
	UINT32 u32Idx;
	UINT32 u32TableSize;
	UINT8  u8DeviceID;
	UINT8 u8ID;
	INT32 rtval;
	INT j;	
	
	struct OV_RegValue *psRegValue;
	DBG_PRINTF("Sensor ID = %d\n", nIndex);
	
	if ( nIndex >= (sizeof(g_uOvDeviceID)/sizeof(UINT8)) )
		return;
	videoIn_Open(48000, 24000);								/* For sensor clock output */	
#if 1	
	SnrPowerDown(FALSE);
	SnrReset();		 
#endif 	 										
		
	u32TableSize = g_OV_InitTable[nIndex].u32TableSize;
	psRegValue = g_OV_InitTable[nIndex].sRegTable;
	u8DeviceID = g_uOvDeviceID[nIndex];
	sysprintf("Device Slave Addr = 0x%x\n", u8DeviceID);
	if ( psRegValue == 0 )
		return;	
	/* Software I2C use GPIOB 13,14 */	
	i2cInit();	
	/* Byte Write/Random Read */
	rtval = i2cOpen();
	if(rtval < 0)
	{
		DBG_PRINTF("Open I2C error!\n");
		return;
	}	
	i2cIoctl(I2C_IOC_SET_DEV_ADDRESS, (u8DeviceID>>1), 0);  
	i2cIoctl(I2C_IOC_SET_SPEED, 100, 0);	
	i2cIoctl(I2C_IOC_SET_SINGLE_MASTER, 1, 0); 
	for(u32Idx=0;u32Idx<u32TableSize; u32Idx++, psRegValue++)
	{
		j = RETRY;
		i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, (psRegValue->u8RegAddr), 1);
		while(j-- > 0) 
		{
			if(i2cWrite(&(psRegValue->u8Value), 1) == 1)
				break;
		}						
		if(j < 0)
			sysprintf("WRITE ERROR [%d]!\n", u32Idx);
		if ((psRegValue->u8RegAddr)==0x12 && (psRegValue->u8Value)==0x80)
		{	
			sysDelay(2);		
			DBG_PRINTF("Delay A loop\n");
		}					
	}	
	
	i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, 0x0A, 1);	
	j = RETRY;
	while(j-- > 0) 
	{
		if(i2cRead_OV(&u8ID, 1) == 1)
			break;
	}
	if(j <= 0)
		DBG_PRINTF("Read ERROR [%x]!\n", 0x0A);
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);

	i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, 0x0B, 1);	
	j = RETRY;
	while(j-- > 0) 
	{
		if(i2cRead_OV(&u8ID, 1) == 1)
			break;
	}
	if(j <= 0)
		DBG_PRINTF("Read ERROR [%x]!\n", 0x0B);
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);

	i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, 0x1C, 1);	
	j = RETRY;
	while(j-- > 0) 
	{
		if(i2cRead_OV(&u8ID, 1) == 1)
			break;
	}
	if(j < 0)
		DBG_PRINTF("Read ERROR [%x]!\n", 0x1C);		
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);
	
	i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, 0x1D, 1);	
	j = RETRY;
	while(j-- > 0) 
	{
		if(i2cRead_OV(&u8ID, 1) == 1)
			break;
	}
	if(j < 0)
		DBG_PRINTF("Read ERROR [%x]!\n", 0x1D);
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);

	i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, 0xD7, 1);	
	j = RETRY;
	while(j-- > 0) 
	{
		if(i2cRead_OV(&u8ID, 1) == 1)
			break;
	}
	if(j < 0)
		DBG_PRINTF("Read ERROR [%x]!\n", 0xD7);
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);

	i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, 0x6A, 1);	
	j = RETRY;
	while(j-- > 0) 
	{
		if(i2cRead_OV(&u8ID, 1) == 1)
			break;
	}
	if(j < 0)
		DBG_PRINTF("Read ERROR [%x]!\n", 0x6A);
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);
}
static void Clean_Framebuffer(PUINT8 pu8FrameBuf, UINT32 u32BufLen)
{
	UINT32 u32Idx;
	PUINT8 pu8TmpBuf;
	
	pu8TmpBuf = pu8FrameBuf;
	for(u32Idx=0; u32Idx<u32BufLen; u32Idx=u32Idx+1)
	{
		if(u32Idx%2)
			*pu8TmpBuf++ = 0x80;	
		else
			*pu8TmpBuf++ = 0x10;
	}
}
/*===================================================================
	LCD dimension = (OPT_LCD_WIDTH, OPT_LCD_HEIGHT)	
	Packet dimension = (OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT)	
	Stride should be LCM resolution  OPT_LCD_WIDTH.
	Packet frame start address = VPOST frame start address + (OPT_LCD_WIDTH-OPT_PREVIEW_WIDTH)/2*2 	
=====================================================================*/

UINT32 Smpl_OV7670_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2)
{
	PFN_VIDEOIN_CALLBACK pfnOldCallback;
	PUINT8 pu8PacketBuf;

	UINT32 u32PreW, u32PreH;
	pu8PacketBuf = (PUINT8)((UINT32)pu8FrameBuffer0 | 0x80000000);
	Clean_Framebuffer(pu8PacketBuf, OPT_LCM_WIDTH*OPT_LCM_HEIGHT*2);
	pu8PacketBuf = (PUINT8)((UINT32)pu8FrameBuffer1 | 0x80000000);	
	Clean_Framebuffer(pu8PacketBuf, OPT_LCM_WIDTH*OPT_LCM_HEIGHT*2);
	pu8PacketBuf = (PUINT8)((UINT32)pu8FrameBuffer2 | 0x80000000);
	Clean_Framebuffer(pu8PacketBuf, OPT_LCM_WIDTH*OPT_LCM_HEIGHT*2);
	
	 InitVPOST(pu8FrameBuffer0);
	
#ifdef __1ST_PORT__	
#ifdef OV7725_CCIR656	
	videoIn_Init(TRUE, 0, 24000, eVIDEOIN_SNR_CCIR656);
#else	
	videoIn_Init(TRUE, 0, 24000, eVIDEOIN_SNR_CCIR601);	
#endif	
#endif
#ifdef __2ND_PORT__
	//videoIn_Init(TRUE, 0, 24000, eVIDEOIN_2ND_SNR_CCIR601);	
	videoIn_Init(TRUE, 0, 24000, eVIDEOIN_2ND_SNR_CCIR601_2);	
#endif	
	//SnrPowerDown(FALSE);
	//SnrReset();	
	OV7670_Init(3);			
	videoIn_Open(72000, 24000);		
	videoIn_EnableInt(eVIDEOIN_VINT);
		
	getFitPreviewDimension(OPT_LCM_WIDTH,
						OPT_LCM_HEIGHT,
						OPT_CROP_WIDTH,
						OPT_CROP_HEIGHT,
						&u32PreW,
						&u32PreH);
						
	sysprintf("Preview (Width , Height)=(%d, %d)\n", u32PreW, u32PreH);
	videoIn_InstallCallback(eVIDEOIN_VINT, 
						(PFN_VIDEOIN_CALLBACK)VideoIn_InterruptHandler,
						&pfnOldCallback	);	//Frame End interrupt						
	videoIn_SetPacketFrameBufferControl(FALSE, FALSE);	
	
	
												
	videoinIoctl(VIDEOIN_IOCTL_ORDER_INFMT_OUTFMT,								
				eVIDEOIN_IN_UYVY, 				//eVIDEOIN_IN_YUYV,//eVIDEOIN_IN_VYUY, 			//Input Order 
				eVIDEOIN_IN_YUV422,			//Intput format
				eVIDEOIN_OUT_YUV422);			//Output format for packet 														
		
	videoinIoctl(VIDEOIN_IOCTL_SET_CROPPING_START_POSITION,				
				0,							//Vertical start position 	Y
				0,							//Horizontal start position	X
				0);							//Useless
#ifdef OV7725_CCIR656			
	videoinIoctl(VIDEOIN_IOCTL_SET_INPUT_TYPE,
				0,							//
				eVIDEOIN_TYPE_CCIR656,
				0);
	videoinIoctl(VIDEOIN_IOCTL_SET_CCIR656,				
				FALSE,						//Non-standard CCIR656 mode
				NULL,						
				NULL);					
	videoinIoctl(VIDEOIN_IOCTL_SET_POLARITY,
				FALSE,							// Inverse the SOF EOF
				FALSE,							// Inverse the SOL EOL
				TRUE);	
			
 #else
 	videoinIoctl(VIDEOIN_IOCTL_SET_CCIR656,				
				TRUE,						//standard CCIR656 mode
				NULL,						
				NULL);	
 	videoinIoctl(VIDEOIN_IOCTL_SET_POLARITY,
				TRUE,
				FALSE,							//Polarity.	
				TRUE);											
#endif				
	videoinIoctl(VIDEOIN_IOCTL_CROPPING_DIMENSION,				
				OPT_CROP_HEIGHT,							//UINT16 u16Height, 
				OPT_CROP_WIDTH,							//UINT16 u16Width;	
				0);							//Useless
//	u32GCD = GCD(OPT_PREVIEW_HEIGHT, OPT_CROP_HEIGHT);						 							 
	videoinIoctl(VIDEOIN_IOCTL_VSCALE_FACTOR,
				eVIDEOIN_PACKET,			//272/480
				u32PreH/1,
				OPT_CROP_HEIGHT);		
//	u32GCD = GCD(OPT_PREVIEW_WIDTH, OPT_CROP_WIDTH);																
	videoinIoctl(VIDEOIN_IOCTL_HSCALE_FACTOR,
				eVIDEOIN_PACKET,			//364/640
				u32PreW,
				OPT_CROP_WIDTH);		
				
//	u32GCD = GCD(OPT_ENCODE_HEIGHT, OPT_CROP_HEIGHT);							 							 
	videoinIoctl(VIDEOIN_IOCTL_VSCALE_FACTOR,
				eVIDEOIN_PLANAR,			//480/480
				OPT_ENCODE_HEIGHT,
				OPT_CROP_HEIGHT);	
				
//	u32GCD = GCD(OPT_ENCODE_WIDTH, OPT_CROP_WIDTH);																			
	videoinIoctl(VIDEOIN_IOCTL_HSCALE_FACTOR,
				eVIDEOIN_PLANAR,			//640/640
				OPT_ENCODE_WIDTH,
				OPT_CROP_WIDTH);
	
#ifdef __TV__
	videoinIoctl(VIDEOIN_IOCTL_SET_STRIDE,										
				OPT_STRIDE,				
				OPT_ENCODE_WIDTH,
				0);
	videoinIoctl(VIDEOIN_IOCTL_SET_BUF_ADDR,
				eVIDEOIN_PACKET,			
				0, 							//Packet buffer addrress 0	
				(UINT32)((UINT32)pu8FrameBuffer0 ) );		
				//(UINT32)((UINT32)pu8FrameBuffer + (OPT_STRIDE-OPT_PREVIEW_WIDTH)/2*2) );	
				
				
								
#else				
	videoinIoctl(VIDEOIN_IOCTL_SET_STRIDE,										
				OPT_STRIDE,				// Packet Stride
				OPT_ENCODE_WIDTH,		// 
				0);
	videoinIoctl(VIDEOIN_IOCTL_SET_BUF_ADDR,
				eVIDEOIN_PACKET,			
				0, 							//Packet buffer addrress 0	
				(UINT32)((UINT32)pu8FrameBuffer0 + (OPT_STRIDE-OPT_PREVIEW_WIDTH)/2*2) );					
#endif			
	videoinIoctl(VIDEOIN_IOCTL_SET_BUF_ADDR,
				eVIDEOIN_PLANAR,			
				0, 							//Planar buffer Y addrress
				(UINT32)u8PlanarFrameBuffer);
	
	//ng
	
	videoinIoctl(VIDEOIN_IOCTL_SET_BUF_ADDR,
				eVIDEOIN_PLANAR,			
				1, 							//Planar buffer U addrress
				(UINT32)u8PlanarFrameBuffer+OPT_ENCODE_WIDTH*OPT_ENCODE_HEIGHT);
				
	videoinIoctl(VIDEOIN_IOCTL_SET_PLANAR_FORMAT,
				FALSE,		//FALSE = Planar YUV422. 
				NULL,
				NULL);		
				
	//ok		
	videoinIoctl(VIDEOIN_IOCTL_SET_BUF_ADDR,
				eVIDEOIN_PLANAR,			
				2, 							//Planar buffer V addrress
				(UINT32)u8PlanarFrameBuffer+OPT_ENCODE_WIDTH*OPT_ENCODE_HEIGHT+OPT_ENCODE_WIDTH*OPT_ENCODE_HEIGHT/2);			
	
	//ok	
				
	videoinIoctl(VIDEOIN_IOCTL_SET_PIPE_ENABLE,
				TRUE, 						// Engine enable?
				eVIDEOIN_BOTH_PIPE_ENABLE,			// which packet was enable. 											
				0 );							//Useless		
				
	videoinIoctl(VIDEOIN_IOCTL_SET_SHADOW,
				NULL,			//640/640
				NULL,
				NULL);	
				
						
	sysSetLocalInterrupt(ENABLE_IRQ);						
															
	return Successful;			
}	




