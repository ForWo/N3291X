#include "wblib.h"
#include "W55FA95_VideoIn.h"
#include "W55FA95_GPIO.h"
#include "demo.h"
#include "w55fa95_i2c.h"

#include "DrvI2C.h"	/* SW I2C */

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
static struct OV_RegValue g_sOV9665_INIT_RegValue[]=
{//OV9665
	//#include "OV9665_Init.dat"
	{0x12, 0x80},
};
static struct OV_RegValue g_sOV9665_VGA_RegValue[]=
{//OV9665
	//#include "OV9665_VGA.dat"
	{0x12, 0x80},
	#if 1
		{0xd5, 0xff}, {0xd6, 0x3f}, {0x3d, 0x3c}, {0x11, 0x80},	{0x2a, 0x00}, {0x2b, 0x00},	//PCLK = SCLK
	#else		
		{0xd5, 0xff}, {0xd6, 0x3f}, {0x3d, 0x3c}, {0x11, 0x81},	{0x2a, 0x00}, {0x2b, 0x00},	//PCLK = SCLK/2
	#endif	
		{0x3a, 0xd9}, {0x3b, 0x00},	{0x3c, 0x58}, {0x3e, 0x50},	{0x71, 0x00}, {0x15, 0x00},
		{0xD7, 0x10}, {0x6a, 0x24},	{0x85, 0xe7}, {0x63, 0x00}, {0x12, 0x40}, {0x4d, 0x09},
	#if 0		
		{0x17, 0x0c}, {0x18, 0x5c},	{0x19, 0x02}, {0x1a, 0x3f},	{0x03, 0x03}, {0x32, 0xb4},	//641*48?
	#else
		{0x17, 0x0b}, {0x18, 0x5c},	{0x19, 0x02}, {0x1a, 0x3f},	{0x03, 0x03}, {0x32, 0xb4}, //648x48?
	#endif		
		{0x2b, 0x00}, {0x5c, 0x80},	{0x36, 0xb4}, {0x65, 0x10}, {0x70, 0x02}, {0x71, 0x9f},
		{0x64, 0xa4}, {0x5c, 0x80},	{0x43, 0x00}, {0x5D, 0x55}, {0x5E, 0x57}, {0x5F, 0x21},
		{0x24, 0x3e}, {0x25, 0x38},	{0x26, 0x72}, {0x14, 0x68}, {0x0C, 0x38}, {0x4F, 0x4f},
		{0x50, 0x42}, {0x5A, 0x67}, {0x7d, 0x30}, {0x7e, 0x00}, {0x82, 0x03}, {0x7f, 0x00},
		{0x83, 0x07}, {0x80, 0x03}, {0x81, 0x04}, {0x96, 0xf0}, {0x97, 0x00}, {0x92, 0x33},
		{0x94, 0x5a}, {0x93, 0x3a},	{0x95, 0x48}, {0x91, 0xfc}, {0x90, 0xff}, {0x8e, 0x4e},
		{0x8f, 0x4e}, {0x8d, 0x13},	{0x8c, 0x0c}, {0x8b, 0x0c},	{0x86, 0x9e}, {0x87, 0x11},
		{0x88, 0x22}, {0x89, 0x05},	{0x8a, 0x03}, {0x9b, 0x0e},	{0x9c, 0x1c}, {0x9d, 0x34},
		{0x9e, 0x5a}, {0x9f, 0x68},	{0xa0, 0x76}, {0xa1, 0x82},	{0xa2, 0x8e}, {0xa3, 0x98},
		{0xa4, 0xa0}, {0xa5, 0xb0},	{0xa6, 0xbe}, {0xa7, 0xd2},	{0xa8, 0xe2}, {0xa9, 0xee},
		{0xaa, 0x18}, {0xAB, 0xe7},	{0xb0, 0x43}, {0xac, 0x04},	{0x84, 0x40}, {0xad, 0x82},
   #if 0		
		{0xd9, 0x11}, {0xda, 0x00},	{0xae, 0x10}, {0xab, 0xe7},	{0xb9, 0x50}, {0xba, 0x3c},		//641*48?	
		{0xbb, 0x50}, {0xbc, 0x3c},	{0xbd, 0x8},  {0xbe, 0x19},	{0xbf, 0x2},  {0xc0, 0x8},
   #else
		{0xd9, 0x11}, {0xda, 0x00},	{0xae, 0x10}, {0xab, 0xe7},	{0xb9, 0x51}, {0xba, 0x3c},		//648x48?
		{0xbb, 0x51}, {0xbc, 0x3c},	{0xbd, 0x8},  {0xbe, 0x19},	{0xbf, 0x2},  {0xc0, 0x8},	
   #endif		
		{0xc1, 0x2a}, {0xc2, 0x34},	{0xc3, 0x2d}, {0xc4, 0x2d},	{0xc5, 0x0},  {0xc6, 0x98},
		{0xc7, 0x18}, {0x69, 0x48},	{0x74, 0xc0}, {0x7c, 0x28},	{0x65, 0x11}, {0x66, 0x00},
		{0x41, 0xc0}, {0x5b, 0x24},	{0x60, 0x82}, {0x05, 0x07},	{0x03, 0x03}, {0xd2, 0x94},
		{0xc8, 0x06}, {0xcb, 0x40},	{0xcc, 0x40}, {0xcf, 0x00},	{0xd0, 0x20}, {0xd1, 0x00},
		{0xc7, 0x18}, {0x0d, 0x92},	{0x0d, 0x90}		
	
};
struct OV_RegValue g_sOV9665_SXGA_RegValue[]=
{//OV9665
	//#include "OV9665_SXGA.dat"
	{0x12, 0x80},
	{0xd5, 0xff}, {0xd6, 0x3f}, {0x3d, 0x3c}, {0x11, 0x81}, {0x2a, 0x00}, {0x2b, 0x00},
	{0x3a, 0xd9}, {0x3b, 0x00}, {0x3c, 0x58}, {0x3e, 0x50}, {0x71, 0x00}, {0x15, 0x00},
#if 1	
	{0xD7, 0x10}, {0x6a, 0x24}, {0x85, 0xe7}, {0x63, 0x01}, {0x17, 0x0c}, {0x18, 0x5c},
	{0x19, 0x01}, {0x1a, 0x82}, {0x03, 0x0f}, {0x2b, 0x00}, {0x32, 0x34}, {0x36, 0xb4},
#else
	{0xD7, 0x10}, {0x6a, 0x24}, {0x85, 0xe7}, {0x63, 0x01}, {0x17, 0x0D}, {0x18, 0x5D},
	{0x19, 0x01}, {0x1a, 0x90}, {0x03, 0x0f}, {0x2b, 0x00}, {0x32, 0x24}, {0x36, 0xb4},
#endif	
	{0x65, 0x10}, {0x70, 0x02}, {0x71, 0x9c}, {0x64, 0x24}, {0x43, 0x00}, {0x5D, 0x55},
	{0x5E, 0x57}, {0x5F, 0x21}, {0x24, 0x3e}, {0x25, 0x38}, {0x26, 0x72}, {0x14, 0x68},
	{0x0C, 0x38}, {0x4F, 0x9E}, {0x50, 0x84}, {0x5A, 0x67}, {0x7d, 0x30}, {0x7e, 0x00},
	{0x82, 0x03}, {0x7f, 0x00}, {0x83, 0x07}, {0x80, 0x03}, {0x81, 0x04}, {0x96, 0xf0},
	{0x97, 0x00}, {0x92, 0x33}, {0x94, 0x5a}, {0x93, 0x3a}, {0x95, 0x48}, {0x91, 0xfc},
	{0x90, 0xff}, {0x8e, 0x4e}, {0x8f, 0x4e}, {0x8d, 0x13}, {0x8c, 0x0c}, {0x8b, 0x0c},
	{0x86, 0x9e}, {0x87, 0x11}, {0x88, 0x22}, {0x89, 0x05}, {0x8a, 0x03}, {0x9b, 0x0e},
	{0x9c, 0x1c}, {0x9d, 0x34}, {0x9e, 0x5a}, {0x9f, 0x68}, {0xa0, 0x76}, {0xa1, 0x82},
	{0xa2, 0x8e}, {0xa3, 0x98}, {0xa4, 0xa0}, {0xa5, 0xb0}, {0xa6, 0xbe}, {0xa7, 0xd2},
	{0xa8, 0xe2}, {0xa9, 0xee}, {0xaa, 0x18}, {0xAB, 0xe7}, {0xb0, 0x43}, {0xac, 0x04},
	{0x84, 0x40}, {0xad, 0x84}, {0xd9, 0x24}, {0xda, 0x00}, {0xae, 0x10}, {0xab, 0xe7},
	{0xb9, 0xa0}, {0xba, 0x80}, {0xbb, 0xa0}, {0xbc, 0x80}, {0xbd, 0x08}, {0xbe, 0x19},
	{0xbf, 0x02}, {0xc0, 0x08}, {0xc1, 0x2a}, {0xc2, 0x34}, {0xc3, 0x2d}, {0xc4, 0x2d},
	{0xc5, 0x00}, {0xc6, 0x98}, {0xc7, 0x18}, {0x69, 0x48}, {0x74, 0xc0}, {0x7c, 0x28},
	{0x65, 0x11}, {0x66, 0x00}, {0x41, 0xc0}, {0x5b, 0x24}, {0x60, 0x82}, {0x05, 0x07},
	{0x03, 0x0f}, {0xd2, 0x94}, {0xc8, 0x06}, {0xcb, 0x40}, {0xcc, 0x40}, {0xcf, 0x00},
	{0xd0, 0x20}, {0xd1, 0x00}, {0xc7, 0x18}, {0x0d, 0x82}, {0x0d, 0x80},	
};

static struct OV_RegTable g_OV_InitTable[] =
{//8 bit slave address, 8 bit data. 
	{g_sOV9665_INIT_RegValue,		_REG_TABLE_SIZE(g_sOV9665_INIT_RegValue)},
	{g_sOV9665_VGA_RegValue,		_REG_TABLE_SIZE(g_sOV9665_VGA_RegValue)},
	{g_sOV9665_SXGA_RegValue,		_REG_TABLE_SIZE(g_sOV9665_SXGA_RegValue)},
	{0, 0}
};

static UINT8 g_uOvDeviceID[]= 
{
	0x60,		// 0v9665 Init
	0x60,		// 0v9665 VGA
	0x60,		// 0v9665 SXGA
	0x00		// not a device ID
};


#if 1
/*
	Sensor power down and reset may default control on sensor daughter board.
	Reset by RC.
	Sensor alway power on (Keep low)

*/
static void SnrReset(void)
{/* GPG11 reset:	H->L->H 	*/			
	gpio_configure(GPIO_PORTG, 11);					//GPIOG 11 as GPIO
	outp32(REG_GPGFUN, inp32(REG_GPGFUN) &~MF_GPG11);	
	outp32(REG_SHRPIN_TOUCH, inp32(REG_SHRPIN_TOUCH) &~MIC_BIAS_AEN);	
	gpio_setportval(GPIO_PORTG, 1<<11, 1<<11);	//GPIOG 11 set high default
	gpio_setportpull(GPIO_PORTG, 1<<11, 1<<11);	//GPIOG 11 pull-up 
	gpio_setportdir(GPIO_PORTG, 1<<11, 1<<11);	//GPIOG 11 output mode 
	sysDelay(2);			
	sysprintf("Reset 1\n");
	gpio_setportval(GPIO_PORTG, 1<<11, 0<<11);	//GPIOG 11 set low
	sysprintf("Reset 2\n");
	sysDelay(2);				
	gpio_setportval(GPIO_PORTG, 1<<11, 1<<11);	//GPIOG 11 set high
	sysDelay(2);	
	sysprintf("Reset 3\n");
	gpio_setportval(GPIO_PORTG, 1<<11, 0<<11);	//GPIOG 11 set low
	sysDelay(2);	
	sysprintf("Reset 4\n");	
	gpio_setportval(GPIO_PORTG, 1<<11, 1<<11);	//GPIOG 11 set high
	sysDelay(2);		
	sysprintf("Reset 5\n");
}

static void SnrPowerDown(BOOL bIsEnable)
{
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
#if 1
#define RETRY	1
VOID OV9665_Init(UINT32 nIndex)
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
#if 1	//Sensor module default no power down and no reset. 
	SnrPowerDown(FALSE);
	SnrReset();		 
#endif 	 										
		
	u32TableSize = g_OV_InitTable[0].u32TableSize;
	psRegValue = g_OV_InitTable[0].sRegTable;
	u8DeviceID = g_uOvDeviceID[0];
	DBG_PRINTF("Device Slave Addr = 0x%x\n", u8DeviceID);
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
	
	/* I2C Read */
	i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, 0x1C, 1);	
	j = RETRY;
	while(j-- > 0) 
	{
		if(i2cRead_OV(&u8ID, 1) == 1)
			break;
	}
	if(j <= 0)
		DBG_PRINTF("Read ERROR [%x]!\n", 0x1C);
	DBG_PRINTF("Sensor IDH = 0x%x\n", u8ID);

	i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, 0x1D, 1);	
	j = RETRY;
	while(j-- > 0) 
	{
		if(i2cRead_OV(&u8ID, 1) == 1)
			break;
	}
	if(j < 0)
		DBG_PRINTF("Read ERROR [%x]!\n", 0x1D);		
	DBG_PRINTF("Sensor IDL = 0x%x\n", u8ID);
		
	
	
	/* I2C Write */
	for(u32Idx=0;u32Idx<u32TableSize; u32Idx++, psRegValue++)
	{//Init
		j = RETRY;
		i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, (psRegValue->u8RegAddr), 1);
		while(j-- > 0) 
		{
			if(i2cWrite(&(psRegValue->u8Value), 1) == 1)
				break;
		}						
		if(j < 0)
			sysprintf("WRITE ERROR [%d]!\n", u32Idx);
		if ((psRegValue->u8RegAddr==0x3E) && (psRegValue->u8Value==0xD0))
		{	
			sysDelay(2);		
			DBG_PRINTF("Delay A loop\n");
		}		
		if ((psRegValue->u8RegAddr==0x12) && (psRegValue->u8Value==0x80))
		{	
			sysDelay(2);		
			DBG_PRINTF("Delay A loop\n");
		}					
	}	
	
	u32TableSize = g_OV_InitTable[nIndex].u32TableSize;
	psRegValue = g_OV_InitTable[nIndex].sRegTable;
	u8DeviceID = g_uOvDeviceID[nIndex];
	
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


	
	
}
#else
extern ERRCODE
DrvI2C_Open(
	UINT32 u32SCKPortIndex,
	UINT32 u32SCKPinMask,
	UINT32 u32SDAPortIndex,
	UINT32 u32SDAPinMask,
	PFN_DRVI2C_TIMEDELY pfnDrvI2C_Delay	
);

extern void Delay( 
	UINT32 nCount 
);

BOOL 
I2C_Write_8bitSlaveAddr_8bitReg_8bitData(UINT8 uAddr, UINT8 uRegAddr, UINT8 uData)
{
	// 3-Phase(ID address, regiseter address, data(8bits)) write transmission
	volatile u32Delay = 0x100;
	DrvI2C_SendStart();
	while(u32Delay--);		
	if ( (DrvI2C_WriteByte(uAddr,DrvI2C_Ack_Have,8)==FALSE) ||			// Write ID address to sensor
		 (DrvI2C_WriteByte(uRegAddr,DrvI2C_Ack_Have,8)==FALSE) ||	// Write register address to sensor
		 (DrvI2C_WriteByte(uData,DrvI2C_Ack_Have,8)==FALSE) )		// Write data to sensor
	{
		DrvI2C_SendStop();
		return FALSE;
	}
	DrvI2C_SendStop();

	if (uRegAddr==0x12 && (uData&0x80)!=0)
	{
		Delay(1000);			
	}
	return TRUE;
}

UINT8 I2C_Read_8bitSlaveAddr_8bitReg_8bitData(UINT8 uAddr, UINT8 uRegAddr)
{
	UINT8 u8Data;
	
	// 2-Phase(ID address, register address) write transmission
	DrvI2C_SendStart();
	DrvI2C_WriteByte(uAddr,DrvI2C_Ack_Have,8);		// Write ID address to sensor
	DrvI2C_WriteByte(uRegAddr,DrvI2C_Ack_Have,8);	// Write register address to sensor
	DrvI2C_SendStop();

	// 2-Phase(ID-address, data(8bits)) read transmission
	DrvI2C_SendStart();
	DrvI2C_WriteByte(uAddr|0x01,DrvI2C_Ack_Have,8);		// Write ID address to sensor
	u8Data = DrvI2C_ReadByte(DrvI2C_Ack_Have,8);		// Read data from sensor
	DrvI2C_SendStop();
	
	return u8Data;
}

static VOID OV9665_Init(UINT32 nIndex)
{
	UINT32 u32Idx;
	UINT32 u32TableSize;
	UINT8  u8DeviceID;
	UINT8 u8ID;
	struct OV_RegValue *psRegValue;
	DBG_PRINTF("Sensor ID = %d\n", nIndex);
	if ( nIndex >= (sizeof(g_uOvDeviceID)/sizeof(UINT8)) )
		return;
	videoIn_Open(48000, 24000);								/* For sensor clock output */	

#if 1	//Sensor module default no power down and no reset. 
	SnrPowerDown(FALSE);
	SnrReset();		 
#endif 	 										
		
	u32TableSize = g_OV_InitTable[nIndex].u32TableSize;
	psRegValue = g_OV_InitTable[nIndex].sRegTable;
	u8DeviceID = g_uOvDeviceID[nIndex];
	DBG_PRINTF("Device Slave Addr = 0x%x\n", u8DeviceID);
	if ( psRegValue == 0 )
		return;	

	gpio_open(GPIO_PORTB);				//GPIOB as GPIO

	DrvI2C_Open(eDRVGPIO_GPIOB, 					
				eDRVGPIO_PIN13, 
				eDRVGPIO_GPIOB,
				eDRVGPIO_PIN14, 
				(PFN_DRVI2C_TIMEDELY)Delay);
	
	u8ID = I2C_Read_8bitSlaveAddr_8bitReg_8bitData(u8DeviceID, 0x1C);
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);
	u8ID = I2C_Read_8bitSlaveAddr_8bitReg_8bitData(u8DeviceID, 0x1D);
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);
									
	for(u32Idx=0;u32Idx<u32TableSize; u32Idx++, psRegValue++)
	{
		I2C_Write_8bitSlaveAddr_8bitReg_8bitData(u8DeviceID, (psRegValue->u8RegAddr), (psRegValue->u8Value));	
		if ((psRegValue->u8RegAddr)==0x12 && (psRegValue->u8Value)==0x80)
		{	
			Delay(1000);		
			DBG_PRINTF("Delay A loop\n");
		}				
	}
	
	
	
	DrvI2C_Close();	
}
#endif
/*===================================================================
	LCD dimension = (OPT_LCD_WIDTH, OPT_LCD_HEIGHT)	
	Packet dimension = (OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT)	
	Stride should be LCM resolution  OPT_LCD_WIDTH.
	Packet frame start address = VPOST frame start address + (OPT_LCD_WIDTH-OPT_PREVIEW_WIDTH)/2*2 	
=====================================================================*/
UINT32 Smpl_OV9665_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2)
{
	PFN_VIDEOIN_CALLBACK pfnOldCallback;
	PUINT8 pu8PacketBuf;

	UINT32 u32PreW, u32PreH;
	pu8PacketBuf = (PUINT8)((UINT32)pu8FrameBuffer0 | 0x80000000);
	memset(pu8PacketBuf, 0x0, OPT_LCM_WIDTH*OPT_LCM_HEIGHT*2);
	pu8PacketBuf = (PUINT8)((UINT32)pu8FrameBuffer1 | 0x80000000);
	memset(pu8PacketBuf, 0x0, OPT_LCM_WIDTH*OPT_LCM_HEIGHT*2);
	pu8PacketBuf = (PUINT8)((UINT32)pu8FrameBuffer2 | 0x80000000);
	memset(pu8PacketBuf, 0x0, OPT_LCM_WIDTH*OPT_LCM_HEIGHT*2);
	
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
	OV9665_Init(1);			
	videoIn_Open(48000, 24000);		
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
	
	videoinIoctl(VIDEOIN_IOCTL_SET_POLARITY,
				TRUE,
				FALSE,							//Polarity.	
				TRUE);							
												
	videoinIoctl(VIDEOIN_IOCTL_ORDER_INFMT_OUTFMT,								
				eVIDEOIN_IN_UYVY, 			//Input Order 
				eVIDEOIN_IN_YUV422	,		//Intput format
				eVIDEOIN_OUT_YUV422);		//Output format for packet 														
		
	videoinIoctl(VIDEOIN_IOCTL_SET_CROPPING_START_POSITION,				
				0,							//Vertical start position 	Y
				2,							//Horizontal start position	X
				0);							//Useless
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
				OPT_STRIDE,				
				OPT_ENCODE_WIDTH,
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
	
	videoinIoctl(VIDEOIN_IOCTL_SET_BUF_ADDR,
				eVIDEOIN_PLANAR,			
				1, 							//Planar buffer U addrress
				(UINT32)u8PlanarFrameBuffer+OPT_ENCODE_WIDTH*OPT_ENCODE_HEIGHT);
				
	videoinIoctl(VIDEOIN_IOCTL_SET_PLANAR_FORMAT,
				FALSE,		//FALSE = Planar YUV422. 
				NULL,
				NULL);			
	videoinIoctl(VIDEOIN_IOCTL_SET_BUF_ADDR,
				eVIDEOIN_PLANAR,			
				2, 							//Planar buffer V addrress
				(UINT32)u8PlanarFrameBuffer+OPT_ENCODE_WIDTH*OPT_ENCODE_HEIGHT+OPT_ENCODE_WIDTH*OPT_ENCODE_HEIGHT/2);			
				
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




/*===================================================================
	LCD dimension = (OPT_LCD_WIDTH, OPT_LCD_HEIGHT)	
	Packet dimension = (OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT)	
	Stride should be LCM resolution  OPT_LCD_WIDTH.
	Packet frame start address = VPOST frame start address + (OPT_LCD_WIDTH-OPT_PREVIEW_WIDTH)/2*2 	
=====================================================================*/
UINT32 Smpl_OV9665_SXGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2)
{
	PFN_VIDEOIN_CALLBACK pfnOldCallback;
	PUINT8 pu8PacketBuf;

	UINT32 u32PlanarFormat;
	UINT32 u32PreW, u32PreH;
	
	sysprintf("Please choice planar format [0]=Planar YUV422.  [!0]=Planar YUV420\n");	
	u32PlanarFormat = sysGetChar();
	if(u32PlanarFormat==0)
		u32PlanarFormat = 0;	//0 ==> planar YUV422
	else
		u32PlanarFormat = 1;	//!0 ==> planar YUV420
	pu8PacketBuf = (PUINT8)((UINT32)pu8FrameBuffer0 | 0x80000000);
	memset(pu8PacketBuf, 0x0, OPT_LCM_WIDTH*OPT_LCM_HEIGHT*2);
	pu8PacketBuf = (PUINT8)((UINT32)pu8FrameBuffer1 | 0x80000000);
	memset(pu8PacketBuf, 0x0, OPT_LCM_WIDTH*OPT_LCM_HEIGHT*2);
	pu8PacketBuf = (PUINT8)((UINT32)pu8FrameBuffer2 | 0x80000000);
	memset(pu8PacketBuf, 0x0, OPT_LCM_WIDTH*OPT_LCM_HEIGHT*2);
	
	InitVPOST(pu8FrameBuffer0);
#ifdef __1ST_PORT__	
	videoIn_Init(TRUE, 0, 24000, eVIDEOIN_SNR_CCIR601);	
#endif
#ifdef __2ND_PORT__
	videoIn_Init(TRUE, 0, 24000, eVIDEOIN_2ND_SNR_CCIR601);	
#endif	
	//SnrPowerDown(FALSE);
	//SnrReset();	
	OV9665_Init(2);			//SXGA
	videoIn_Open(96000, 24000);		
	videoIn_EnableInt(eVIDEOIN_VINT);
		
	getFitPreviewDimension(OPT_LCM_WIDTH,
						OPT_LCM_HEIGHT,
						OPT_CROP_WIDTH,
						OPT_CROP_HEIGHT,
						&u32PreW,
						&u32PreH);
						
	videoIn_InstallCallback(eVIDEOIN_VINT, 
						(PFN_VIDEOIN_CALLBACK)VideoIn_InterruptHandler,
						&pfnOldCallback	);	//Frame End interrupt
						
	videoIn_SetPacketFrameBufferControl(FALSE, FALSE);	
	
	videoinIoctl(VIDEOIN_IOCTL_SET_POLARITY,
				TRUE,
				FALSE,							//Polarity.	
				TRUE);							
												
	videoinIoctl(VIDEOIN_IOCTL_ORDER_INFMT_OUTFMT,								
				eVIDEOIN_IN_UYVY, 			//Input Order 
				eVIDEOIN_IN_YUV422	,		//Intput format
				eVIDEOIN_OUT_YUV422);		//Output format for packet 														
		
	videoinIoctl(VIDEOIN_IOCTL_SET_CROPPING_START_POSITION,				
				0,							//Vertical start position
				4,							//Horizontal start position	
				0);							//Useless
	videoinIoctl(VIDEOIN_IOCTL_CROPPING_DIMENSION,				
				OPT_CROP_HEIGHT,							//UINT16 u16Height, 
				OPT_CROP_WIDTH,							//UINT16 u16Width;	
				0);							//Useless
//	u32GCD = GCD(OPT_PREVIEW_HEIGHT, OPT_CROP_HEIGHT);						 							 
	videoinIoctl(VIDEOIN_IOCTL_VSCALE_FACTOR,
				eVIDEOIN_PACKET,			//272/480
				OPT_PREVIEW_HEIGHT,
				OPT_CROP_HEIGHT);		
//	u32GCD = GCD(OPT_PREVIEW_WIDTH, OPT_CROP_WIDTH);																
	videoinIoctl(VIDEOIN_IOCTL_HSCALE_FACTOR,
				eVIDEOIN_PACKET,			//364/640
				OPT_PREVIEW_WIDTH,
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
	
#if  0
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
				OPT_STRIDE,				
				OPT_ENCODE_WIDTH,
				0);
	videoinIoctl(VIDEOIN_IOCTL_SET_BUF_ADDR,
				eVIDEOIN_PACKET,			
				0, 							//Packet buffer addrress 0	
				(UINT32)((UINT32)pu8FrameBuffer0 + (OPT_STRIDE-u32PreW)/2*2) );					
#endif			
	videoinIoctl(VIDEOIN_IOCTL_SET_BUF_ADDR,
				eVIDEOIN_PLANAR,			
				0, 							//Planar buffer Y addrress
				(UINT32)u8PlanarFrameBuffer);
	
	videoinIoctl(VIDEOIN_IOCTL_SET_BUF_ADDR,
				eVIDEOIN_PLANAR,			
				1, 							//Planar buffer U addrress
				(UINT32)u8PlanarFrameBuffer+OPT_ENCODE_WIDTH*OPT_ENCODE_HEIGHT);
	if(u32PlanarFormat==0)
	{	
		videoinIoctl(VIDEOIN_IOCTL_SET_PLANAR_FORMAT,
				FALSE,		//FALSE = Planar YUV422. 
				NULL,
				NULL);			
		videoinIoctl(VIDEOIN_IOCTL_SET_BUF_ADDR,
				eVIDEOIN_PLANAR,			
				2, 							//Planar buffer V addrress
				(UINT32)u8PlanarFrameBuffer+OPT_ENCODE_WIDTH*OPT_ENCODE_HEIGHT+OPT_ENCODE_WIDTH*OPT_ENCODE_HEIGHT/2);			
	}
	else
	{
		videoinIoctl(VIDEOIN_IOCTL_SET_PLANAR_FORMAT,
				TRUE,		//TRUE = Planar YUV420. 
				NULL,
				NULL);			
		videoinIoctl(VIDEOIN_IOCTL_SET_BUF_ADDR,
				eVIDEOIN_PLANAR,			
				2, 							//Planar buffer V addrress
				(UINT32)u8PlanarFrameBuffer+OPT_ENCODE_WIDTH*OPT_ENCODE_HEIGHT+OPT_ENCODE_WIDTH*OPT_ENCODE_HEIGHT/4);	
	}
				
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


