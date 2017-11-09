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
#define OV7725_CCIR656
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

static struct OV_RegValue g_sGC0308_VGA_RegValue[]=
{//GC0308	
	#include "GC0308\GC0308_VGA.dat"
};

static struct OV_RegTable g_OV_InitTable[] =
{//8 bit slave address, 8 bit data. 
	{0, 0},
	{0, 0},//{g_sOV6880_RegValue,	_REG_TABLE_SIZE(g_sOV6880_RegValue)},		
	{0, 0},//{g_sOV7648_RegValue,	_REG_TABLE_SIZE(g_sOV7648_RegValue)},
	{0, 0},//{g_sOV7670_RegValue,	_REG_TABLE_SIZE(g_sOV7670_RegValue)},
	{0, 0},//{g_sOV2640_RegValue,	_REG_TABLE_SIZE(g_sOV2640_RegValue)},	
	{0, 0},//{g_sOV9660_VGA_RegValue,		_REG_TABLE_SIZE(g_sOV9660_VGA_RegValue)},
	{0, 0},//{g_sOV9660_SXGA_RegValue,		_REG_TABLE_SIZE(g_sOV9660_SXGA_RegValue)},
	{0, 0},//{g_sOV7725_VGA_RegValue,		_REG_TABLE_SIZE(g_sOV7725_VGA_RegValue)},
	{g_sGC0308_VGA_RegValue,		_REG_TABLE_SIZE(g_sGC0308_VGA_RegValue)},
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
	0x42,		// GC0308 VGA
	0x00			// not a device ID
};


#if 1
/*
	Sensor power down and reset may default control on sensor daughter board.
	Reset by RC.
	Sensor alway power on (Keep low)

*/
static void SnrReset(void)
{
	sysprintf("Sensor reset\n");	
	gpio_configure(GPIO_PORTG, 11);					//GPIOG 10 as GPIO
	outp32(REG_GPGFUN, inp32(REG_GPGFUN) &~MF_GPG11);	
	outp32(REG_SHRPIN_TOUCH, inp32(REG_SHRPIN_TOUCH) &~MIC_BIAS_AEN);	
	gpio_setportval(GPIO_PORTG, 1<<11, 1<<11);	//GPIOG 11 set high default
	gpio_setportpull(GPIO_PORTG, 1<<11, 1<<11);	//GPIOG 11 pull-up 
	gpio_setportdir(GPIO_PORTG, 1<<11, 1<<11);	//GPIOG 11 output mode 
	//sysDelay(1);			
	gpio_setportval(GPIO_PORTG, 1<<11, 1<<11);	//GPIOG 11 set high
	sysprintf("Reset Sensor done\n");
}

static void SnrPowerDown(BOOL bIsEnable)
{/* GPB4 power down, Low for power down */
	if(bIsEnable==TRUE)
		sysprintf("Sensor power down\n");
	else
		sysprintf("Sensor not power down\n");	
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
VOID GC0308_Init(UINT32 nIndex)
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
	
	i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, 0x00, 1);	
	j = RETRY;
	while(j-- > 0) 
	{
		if(i2cRead_OV(&u8ID, 1) == 1)
			break;
	}
	if(j <= 0)
		DBG_PRINTF("Read ID ERROR [%x]!\n", 0x00);
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);	
	
	
	 
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
	
	
  	//sharpness strength modified by pw 2008-03-05   
    	//regvalue = gpio_sccb_read{I2C_OV7725, 0xac},  
    	//{0xac, 0xdfRvalue},  
    	//{0x8f, 0x04},  	
    	
  
		
#if 0	
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

	i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, 0x00, 1);	
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
#endif	
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

UINT32 Smpl_GC0308_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2)
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
	videoIn_Init(TRUE, 0, 24000, eVIDEOIN_SNR_CCIR601);		
#endif
#ifdef __2ND_PORT__
	//videoIn_Init(TRUE, 0, 24000, eVIDEOIN_2ND_SNR_CCIR601);	
	videoIn_Init(TRUE, 0, 24000, eVIDEOIN_2ND_SNR_CCIR601_2);	
#endif	
	//SnrPowerDown(FALSE);
	//SnrReset();	
	GC0308_Init(8);			
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

 	videoinIoctl(VIDEOIN_IOCTL_SET_CCIR656,				
				TRUE,						//standard CCIR656 mode
				NULL,						
				NULL);	
 	videoinIoctl(VIDEOIN_IOCTL_SET_POLARITY,
				FALSE,
				TRUE,							//Polarity.	
				TRUE);											
				
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




