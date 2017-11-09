#include "wblib.h"
#include "W55FA95_VideoIn.h"
#include "W55FA95_GPIO.h"
#include "w55fa95_i2c.h"
#include "DrvI2C.h"
#include "demo.h"

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

struct NT_RegValue
{
	UINT16	u16RegAddr;		//Register Address
	UINT8	u8Value;			//Register Data
};

struct NT_RegTable{
	struct NT_RegValue *sRegTable;
	UINT32 u32TableSize;
};

extern UINT8 u8PlanarFrameBuffer[];


#define _REG_TABLE_SIZE(nTableName)	(sizeof(nTableName)/sizeof(struct NT_RegValue))

struct NT_RegValue g_sNT99050_RegValue[] = 
{
 	#include "NT99050\NT99050_30F.dat"
};

static struct NT_RegTable g_NT99050_InitTable[] =
{//8 bit slave address, 8 bit data. 
	{0, 0},
	{0, 0},//{g_sOV6880_RegValue,	_REG_TABLE_SIZE(g_sOV6880_RegValue)},		
	{0, 0},//{g_sOV7648_RegValue,	_REG_TABLE_SIZE(g_sOV7648_RegValue)},
	{0,0},
	{0, 0},//{g_sOV2640_RegValue,	_REG_TABLE_SIZE(g_sOV2640_RegValue)},	
	{0, 0},//{g_sOV9660_RegValue,	_REG_TABLE_SIZE(g_sOV9660_RegValue)},
	{g_sNT99050_RegValue,	_REG_TABLE_SIZE(g_sNT99050_RegValue)},
	{0, 0}
};

extern UINT8 u8DiffBuf[];
extern UINT8 u8OutLumBuf[];

static UINT8 g_uOvDeviceID[]= 
{
	0x00,		// not a device ID
	0xc0,		// ov6680
	0x42,		// ov7648
	0x42,		// ov7670
	0x60,		// ov2640
	0x60,		// 0v9660
	0x42,		// NT99050 = 6
	0x00		// not a device ID
};


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
{
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


BOOL I2C_Write_8bitSlaveAddr_16bitReg_8bitData(UINT8 uAddr, UINT16 uRegAddr, UINT8 uData)
{
	// 3-Phase(ID address, regiseter address, data(8bits)) write transmission
	volatile u32Delay = 0x100;
	DrvI2C_SendStart();
	while(u32Delay--);		
	if ( (DrvI2C_WriteByte(uAddr,DrvI2C_Ack_Have,8)==FALSE) ||			// Write ID address to sensor
		 (DrvI2C_WriteByte((UINT8)(uRegAddr>>8),DrvI2C_Ack_Have,8)==FALSE) ||	// Write register address to sensor
		 (DrvI2C_WriteByte((UINT8)(uRegAddr&0xff),DrvI2C_Ack_Have,8)==FALSE) ||	// Write register address to sensor
		 (DrvI2C_WriteByte(uData,DrvI2C_Ack_Have,8)==FALSE) )		// Write data to sensor
	{
		sysprintf("wnoack Addr = 0x%x \n", uRegAddr);
		DrvI2C_SendStop();
		return FALSE;
	}
	DrvI2C_SendStop();

	if (uRegAddr==0x12 && (uData&0x80)!=0)
	{
		sysDelay(2);			
	}
	return TRUE;
}

UINT8 I2C_Read_8bitSlaveAddr_16bitReg_8bitData(UINT8 uAddr, UINT16 uRegAddr)
{
	UINT8 u8Data;
	
	// 2-Phase(ID address, register address) write transmission
	DrvI2C_SendStart();
	DrvI2C_WriteByte(uAddr,DrvI2C_Ack_Have,8);		// Write ID address to sensor
	DrvI2C_WriteByte((UINT8)(uRegAddr>>8),DrvI2C_Ack_Have,8);	// Write register address to sensor
	DrvI2C_WriteByte((UINT8)(uRegAddr&0xff),DrvI2C_Ack_Have,8);	// Write register address to sensor	
	DrvI2C_SendStop();

	// 2-Phase(ID-address, data(8bits)) read transmission
	DrvI2C_SendStart();
	DrvI2C_WriteByte(uAddr|0x01,DrvI2C_Ack_Have,8);		// Write ID address to sensor
	u8Data = DrvI2C_ReadByte(DrvI2C_Ack_Have,8);		// Read data from sensor
	DrvI2C_SendStop();
	
	return u8Data;

}


#define RETRY	1

#define CHIP_VERSION_H		0x3000  	/* Default 0x05 */
#define CHIP_VERSION_L		0x3001	/* Default 0x0 */
extern void Delay(UINT32 nCount);
VOID NT99050_Init(UINT32 nIndex)
{
	UINT32 u32Idx;
	UINT32 u32TableSize;
	UINT8  u8DeviceID;
	UINT8 u8IDH, u8IDL, u8ID;
	INT32 rtval, j;
	
	struct NT_RegValue *psRegValue;
	DBG_PRINTF("Sensor ID = %d\n", nIndex);
	if ( nIndex >= (sizeof(g_uOvDeviceID)/sizeof(UINT8)) )
		return;	
	
	//videoIn_Close();	
	//videoIn_Open(72000, 24000);								/* For sensor clock output */	
	
	SnrPowerDown(TRUE); 	
	sysDelay(5);
	videoIn_Open(48000, 24000);	
	sysDelay(1);

	/* nt99141 sensor need reset without sensor clock */
	SnrReset();
	SnrPowerDown(FALSE); 	 	
					
		
	sysDelay(1);
	u32TableSize = g_NT99050_InitTable[nIndex].u32TableSize;
	psRegValue = g_NT99050_InitTable[nIndex].sRegTable;
	u8DeviceID = g_uOvDeviceID[nIndex];
	DBG_PRINTF("Device Slave Addr = 0x%x\n", u8DeviceID);
	if ( psRegValue == 0 )
		return;	
#if 0
	outp32(REG_GPBFUN, inp32(REG_GPBFUN) & (~MF_GPB13));
	outp32(REG_GPBFUN, inp32(REG_GPBFUN) & (~MF_GPB14));
	DrvI2C_Open(eDRVGPIO_GPIOB, 					
				eDRVGPIO_PIN13, 
				eDRVGPIO_GPIOB,
				eDRVGPIO_PIN14, 
				(PFN_DRVI2C_TIMEDELY)Delay);
									
	for(u32Idx=0;u32Idx<u32TableSize; u32Idx++, psRegValue++)
	{
		//printf("Addr = 0x%x\n",  (psRegValue->u16RegAddr));
		I2C_Write_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID, (psRegValue->u16RegAddr), (psRegValue->u8Value));
			
		#if 0 
		if ((psRegValue->u8RegAddr)==0x12 && (psRegValue->u8Value)==0x80)
		{	
			Delay(1000);		
			DBG_PRINTF("Delay A loop\n");
		}
		#else
		if ((psRegValue->u16RegAddr)==0x3021 && (psRegValue->u8Value)==0x01)
		{	
			//Delay(1000);
			sysDelay(2);		
			DBG_PRINTF("Delay A loop\n");
		}	
		#endif				
	}
	
	u8ID = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID, 0x0A);
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);
	u8ID = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID, 0x0b);
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);
	u8ID = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID, 0x1C);
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);
	u8ID = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID, 0x1D);
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);

	u8ID = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID, 0xD7);
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);
	u8ID = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID, 0x6A);
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);	
	DrvI2C_Close();			
	
#else		
	/* Hardware I2C use GPIOB 13,14 */	
	i2cInit();	
	/* Byte Write/Random Read */
	rtval = i2cOpen();
	if(rtval < 0)
	{
		DBG_PRINTF("Open I2C error!\n");
		return;
	}	
	i2cIoctl(I2C_IOC_SET_DEV_ADDRESS, (u8DeviceID>>1), 0);  
	i2cIoctl(I2C_IOC_SET_SPEED, 200, 0);	
	i2cIoctl(I2C_IOC_SET_SINGLE_MASTER, 1, 0); 
	for(u32Idx=0;u32Idx<u32TableSize; u32Idx++, psRegValue++)
	{
		j = RETRY;
		i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, (psRegValue->u16RegAddr), 2); /* 2 means 16 bit */
		while(j-- > 0) 
		{
			//Delay(100);
			if(i2cWrite(&(psRegValue->u8Value), 1) == 1)
			{
				break;
			}
			else
				sysprintf("WRITE ERROR 1 [%d]!\n", u32Idx);	
		}						
		if(j < 0)
			sysprintf("WRITE ERROR 2 [%d]!\n", u32Idx);			
	}	
	sysDelay(1);
	
	for(u32Idx=0;u32Idx<u32TableSize; u32Idx++, psRegValue++)
	{
		j = RETRY;
		i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, (psRegValue->u16RegAddr), 2); /* 2 means 16 bit */
		while(j-- > 0) 
		{
			//Delay(100);
			if(i2cWrite(&(psRegValue->u8Value), 1) == 1)
				break;
		}						
		if(j < 0)
			sysprintf("WRITE ERROR [%d]!\n", u32Idx);			
	}		
	
	/* Read IDH */
	i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, CHIP_VERSION_H, 2);	/* 2 bytes */
	j = RETRY;
	while(j-- > 0) {
		if(i2cRead(&u8IDH, 1) == 1)
			break;
	}
	
	/* Read IDL */
	i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, CHIP_VERSION_L, 2);	/* 2 bytes */	
	j = RETRY;
	while(j-- > 0) {
		if(i2cRead(&u8IDL, 1) == 1)
			break;
	}
	sysprintf("Detectd sensor IDH=%0x, IDL =%02x\n",u8IDH, u8IDL);
#endif	
}


/*===================================================================
	LCD dimension = (OPT_LCD_WIDTH, OPT_LCD_HEIGHT)	
	Packet dimension = (OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT)	
	Stride should be LCM resolution  OPT_LCD_WIDTH.
	Packet frame start address = VPOST frame start address + (OPT_LCD_WIDTH-OPT_PREVIEW_WIDTH)/2*2 	
=====================================================================*/
UINT32 Smpl_NT99050(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2)
{
	PFN_VIDEOIN_CALLBACK pfnOldCallback;
	PUINT8 pu8PacketBuf;
	
	pu8PacketBuf = (PUINT8)((UINT32)pu8FrameBuffer0 | 0x80000000);
	memset(pu8PacketBuf, 0x0, OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT*2);
	pu8PacketBuf = (PUINT8)((UINT32)pu8FrameBuffer1 | 0x80000000);
	memset(pu8PacketBuf, 0x0, OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT*2);
	pu8PacketBuf = (PUINT8)((UINT32)pu8FrameBuffer2 | 0x80000000);
	memset(pu8PacketBuf, 0x0, OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT*2);
	outp32(REG_AHBCLK, 0xFFFFFFFF);

	InitVPOST(pu8FrameBuffer0);	
	#ifdef __3RD_PORT__
		// GPIOD2 pull high
		gpio_setportval(GPIO_PORTD, 0x04, 0x04);    //GPIOD2 high to enable Amplifier 
		gpio_setportpull(GPIO_PORTD, 0x04, 0x04);	//GPIOD2 pull high
		gpio_setportdir(GPIO_PORTD, 0x04, 0x04);	//GPIOD2 output mode
	#endif 

#ifdef __1ST_PORT__	
	videoIn_Init(TRUE, 0, 24000, eVIDEOIN_SNR_CCIR601);	
#endif
#ifdef __2ND_PORT__
	//videoIn_Init(TRUE, 0, 12000, eVIDEOIN_3RD_SNR_CCIR601);
	videoIn_Init(TRUE, 0, 24000, eVIDEOIN_2ND_SNR_CCIR601);	
#endif	
#ifdef __3RD_PORT__
	videoIn_Init(TRUE, 0, 24000, eVIDEOIN_3RD_SNR_CCIR601);	
#endif	
	NT99050_Init(6);			
	videoIn_Open(48000, 24000);		
	videoIn_EnableInt(eVIDEOIN_VINT);
	
	videoIn_InstallCallback(eVIDEOIN_VINT, 
						(PFN_VIDEOIN_CALLBACK)VideoIn_InterruptHandler,
						&pfnOldCallback	);	//Frame End interrupt
						
	videoIn_SetPacketFrameBufferControl(FALSE, FALSE);	
	
	videoinIoctl(VIDEOIN_IOCTL_SET_POLARITY,
				FALSE,							//NT99050	
				FALSE,							//Polarity.	
				TRUE);							
												
	videoinIoctl(VIDEOIN_IOCTL_ORDER_INFMT_OUTFMT,								
				eVIDEOIN_IN_UYVY, 			//NT99050
				eVIDEOIN_IN_YUV422	,	//Intput format
				eVIDEOIN_OUT_YUV422);		//Output format for packet 														
				//eVIDEOIN_OUT_RGB565);
	videoinIoctl(VIDEOIN_IOCTL_SET_CROPPING_START_POSITION,				
				0,							//NT99050
				0,							//Horizontal start position	
				0);							//Useless
	videoinIoctl(VIDEOIN_IOCTL_CROPPING_DIMENSION,				
				OPT_CROP_HEIGHT,							//UINT16 u16Height, 
				OPT_CROP_WIDTH,							//UINT16 u16Width;	
				0);							//Useless
	
	/* Set Packet dimension */			
	videoinIoctl(VIDEOIN_IOCTL_VSCALE_FACTOR,
				eVIDEOIN_PACKET,			
				OPT_PREVIEW_HEIGHT,
				OPT_CROP_HEIGHT);		
	videoinIoctl(VIDEOIN_IOCTL_HSCALE_FACTOR,
				eVIDEOIN_PACKET,			
				OPT_PREVIEW_WIDTH,
				OPT_CROP_WIDTH);		
				
	/* Set Planar dimension  */			
	videoinIoctl(VIDEOIN_IOCTL_VSCALE_FACTOR,
				eVIDEOIN_PLANAR,			//640/640
				OPT_ENCODE_HEIGHT,
				OPT_CROP_HEIGHT);		
	videoinIoctl(VIDEOIN_IOCTL_HSCALE_FACTOR,
				eVIDEOIN_PLANAR,			//640/640
				OPT_ENCODE_WIDTH,
				OPT_CROP_WIDTH);
	/* Set Pipes stride */					
	videoinIoctl(VIDEOIN_IOCTL_SET_STRIDE,										
				OPT_STRIDE,				
				OPT_ENCODE_WIDTH,
				0);
				
	/* Set Packet Buffer Address */			
	videoinIoctl(VIDEOIN_IOCTL_SET_BUF_ADDR,
				eVIDEOIN_PACKET,			
				0, 							//Packet buffer addrress 0	
				(UINT32)((UINT32)pu8FrameBuffer0 + (OPT_STRIDE-OPT_PREVIEW_WIDTH)/2*2) );
	
	/* Set Planar Buffer Address */						
	videoinIoctl(VIDEOIN_IOCTL_SET_BUF_ADDR,
				eVIDEOIN_PLANAR,				
				0, 							//0 = Planar Y buffer address
				(UINT32)(&u8PlanarFrameBuffer) );							
	videoinIoctl(VIDEOIN_IOCTL_SET_BUF_ADDR,
				eVIDEOIN_PLANAR,			
				1, 							//1 = Planar U buffer address
				(UINT32)(&u8PlanarFrameBuffer) +  OPT_ENCODE_WIDTH*OPT_ENCODE_HEIGHT);							
	videoinIoctl(VIDEOIN_IOCTL_SET_BUF_ADDR,
				eVIDEOIN_PLANAR,			
				2, 							//2 = Planar V buffer address
				(UINT32)(&u8PlanarFrameBuffer) +  OPT_ENCODE_WIDTH*OPT_ENCODE_HEIGHT+OPT_ENCODE_WIDTH*OPT_ENCODE_HEIGHT/2 );																					
					
	
	videoinIoctl(VIDEOIN_IOCTL_SET_PIPE_ENABLE,
				TRUE, 						// Engine enable ?
				eVIDEOIN_BOTH_PIPE_ENABLE,		// which Pipes was enable. 											
				0 );							//Useless		
							
					
							
	sysSetLocalInterrupt(ENABLE_IRQ);						
														
	return Successful;			
}	