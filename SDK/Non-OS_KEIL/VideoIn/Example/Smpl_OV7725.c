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

static struct OV_RegValue g_sOV7725_VGA_RegValue[]=
{//OV7725		

    {0x12, 0x80},  
#ifdef   OV7725_CCIR656  
    {0x12, 0x20}, 
    {0x15, 0x01}, /* Color Range from 0 ~240 */ 
#else
  {0x12, 0x00}, 	
  {0x15, 0x00}, /* full Range */ 	
#endif    
    {0x3d, 0x03}, 
    {0x17, 0x22},  
   // {0x17, 0x26},  
    {0x18, 0xa4},  /* Hsize = 10100100+ 00 */
    {0x19, 0x07}, 
    
     {0x1a, 0xf0},  /* VSIZE + 0x32[2] =0x1e0*/
    // {0x1a, 0xf2},     /* VSIZE + 0x32[2] = 0x1e4*/
    
     {0x32, 0x02},  
  
     {0x29, 0xa0},   /* Horizontal data output size + 0x29[1:0]*/   
   	
    
    {0x2c, 0xf0},   
//    {0x2a, 0x00},  /* default */
    {0x2a, 0x02},  /*  useless */
     
      //{0x33, 0x08},  /*  useless */
      //{0x34, 0x08},  /*  useless */
    
    {0x11, 0x01},//00/01/03/07 for 60/30/15/7.5fps         
    {0x42, 0x7f},   {0x4d, 0x09},  {0x63, 0xe0},  {0x64, 0xff},   {0x65, 0x20},  {0x66, 0x00},  {0x67, 0x48}, {0x13, 0xf0},      
    {0x0d, 0x41},  //0x51/0x61/0x71 for different AEC/AGC window   
    {0x0f, 0xc5},  {0x14, 0x11},//0x81   
    {0x22, 0x7f},//ff/7f/3f/1f for 60/30/15/7.5fps   
    {0x23, 0x03},//01/03/07/0f for 60/30/15/7.5fps   
    {0x24, 0x50},//0x80   
    {0x25, 0x30},//5a   
    {0x26, 0xa1},//c1   
    {0x2b, 0x00},//ff   
    {0x6b, 0xaa},  {0x13, 0xff},  {0x90, 0x05},  {0x91, 0x01},  {0x92, 0x03},  {0x93, 0x00}, {0x94, 0xb0},  {0x95, 0x9d},  
    {0x96, 0x13},  {0x97, 0x16},  {0x98, 0x7b},  {0x99, 0x91},  {0x9a, 0x1e},  {0x9b, 0x00},  {0x9c, 0x25},  {0x9e, 0x81},  
    {0xa6, 0x06},  
  
//modified saturation initialization value by pw 2008-03-04   
    {0xa7, 0x65},  {0xa8, 0x65},  {0x7e, 0x0c},  {0x7f, 0x16},  {0x80, 0x2a},  {0x81, 0x4e},  {0x82, 0x61},  {0x83, 0x6f},  
    {0x84, 0x7b},  {0x85, 0x86},  {0x86, 0x8e},  {0x87, 0x97},  {0x88, 0xa4},  {0x89, 0xaf},  {0x8a, 0xc5},  {0x8b, 0xd7},  
    {0x8c, 0xe8},   {0x8d, 0x20},  
    
#if 1   
    {0x34, 0x00},  {0x33, 0x40},//0x66/0x99   
    {0x22, 0x99},  {0x23, 0x03},  {0x4a, 0x10},  {0x49, 0x10},  {0x4b, 0x14},  {0x4c, 0x17},  {0x46, 0x05},  
    {0x0e, 0x65},  
#endif       
     {0x15, 1},  
    {0x69, 0x5d},  {0x0c, 0x00},  {0x33, 0x3f},//0x66/0x99   
    {0x0e, 0x65},  
    /* Remark by SW */
    /*		
    sharpness strength modified by pw 2008-03-05   
    regvalue = gpio_sccb_read{I2C_OV7725, 0xac},  
    {0xac, 0xdfRvalue},  
    */
    {0x8f, 0x04},  	
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
	{g_sOV7725_VGA_RegValue,		_REG_TABLE_SIZE(g_sOV7725_VGA_RegValue)},
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


#if 0
/*
	Sensor power down and reset may default control on sensor daughter board.
	Reset by RC.
	Sensor alway power on (Keep low)

*/
static void SnrReset(void)
{/* GPA11 reset:	H->L->H 	*/			
#ifdef __GPIO_PIN__
	gpio_open(GPIO_PORTA, 11);					//GPIOA 10 as GPIO
#else	
	gpio_open(GPIO_PORTA);						//GPIOA 10 as GPIO
#endif		
	gpio_setportval(GPIO_PORTA, 1<<11, 1<<11);	//GPIOA 11 set high default
	gpio_setportpull(GPIO_PORTA, 1<<11, 1<<11);	//GPIOA 11 pull-up 
	gpio_setportdir(GPIO_PORTA, 1<<11, 1<<11);	//GPIOA 11 output mode 
	Delay(1000);			
	gpio_setportval(GPIO_PORTA, 1<<11, 1<<11);	//GPIOA 11 set low
	Delay(1000);				
	gpio_setportval(GPIO_PORTA, 1<<11, 0);		//GPIOA 11 set high
}

static void SnrPowerDown(BOOL bIsEnable)
{/* GPA10 power down, Low for power down */
#ifdef __GPIO_PIN__
	gpio_open(GPIO_PORTA, 10);					//GPIOA 10 as GPIO
#else	
	gpio_open(GPIO_PORTA);						//GPIOA 10 as GPIO
#endif	
	gpio_setportval(GPIO_PORTA, 1<<10, 1<<10);	//GPIOA 10 set high default
	gpio_setportpull(GPIO_PORTA, 1<<10, 1<<10);	//GPIOA 10 pull-up 
	gpio_setportdir(GPIO_PORTA, 1<<10, 1<<10);	//GPIOA 10 output mode 				
	if(bIsEnable)
		gpio_setportval(GPIO_PORTA, 1<<10, 0);	//GPIOA 10 set high
	else			
		gpio_setportval(GPIO_PORTA, 1<<10, 1<<10);	//GPIOA 10 set low	
}
#endif 

#define RETRY	2
VOID OV7725_Init(UINT32 nIndex)
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
#if 0	//Sensor module default no power down and no reset. 
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
	
	
  	//sharpness strength modified by pw 2008-03-05   
    	//regvalue = gpio_sccb_read{I2C_OV7725, 0xac},  
    	//{0xac, 0xdfRvalue},  
    	//{0x8f, 0x04},  	
    	
  
		
	
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

UINT32 Smpl_OV7725_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2)
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
	OV7725_Init(7);			
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
				eVIDEOIN_IN_VYUY, 				//eVIDEOIN_IN_YUYV,//eVIDEOIN_IN_VYUY, 			//Input Order 
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




