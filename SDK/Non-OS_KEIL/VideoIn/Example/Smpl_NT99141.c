#include "wblib.h"
#include "W55FA95_VideoIn.h"
#include "W55FA95_GPIO.h"
#include "w55fa95_i2c.h"
#include "DrvI2C.h"
#include "demo.h"

struct NT_RegValue
{
	UINT16	u16RegAddr;		//Register Address
	UINT8	u8Value;			//Register Data
};
#define _REG_TABLE_SIZE(nTableName)	sizeof(nTableName)/sizeof(struct NT_RegValue)

#define REG_VALUE_INIT	0
#define REG_VALUE_VGA		1	//640X480
#define REG_VALUE_SVGA	2	//800X600
#define REG_VALUE_HD720	3	//1280X720



#define DrvVideoIn_nt99140	1
#define CHIP_VERSION_H		0x3000
#define CHIP_VERSION_L		0x3001
#define NT99140_CHIP_ID	0x14

struct NT_RegTable{
	struct NT_RegValue *sRegTable;
	UINT16 uTableSize;
};

extern UINT8 u8PlanarFrameBuffer[];

struct NT_RegValue g_sNT99141_Init[] = 
{
	//[Inti]
	{0x3109, 0x04},
	{0x3040, 0x04},
	{0x3041, 0x02},
	{0x3042, 0xFF},
	{0x3043, 0x08},
	{0x3052, 0xE0},
	{0x305F, 0x33},
	{0x3100, 0x07},
	{0x3106, 0x03},

	{0x3108, 0x00},
	{0x3110, 0x22},
	{0x3111, 0x57},
	{0x3112, 0x22},
	{0x3113, 0x55},
	{0x3114, 0x05},
	{0x3135, 0x00},
	{0x32F0, 0x01},
	
	{0x306a,0x01}, //

	{0x3290, 0x01}, // Initial AWB Gain
	{0x3291, 0x80},
	{0x3296, 0x01},
	{0x3297, 0x73},

	{0x3250, 0x80}, // CA Ratio
	{0x3251, 0x03},
	{0x3252, 0xFF},
	{0x3253, 0x00},
	{0x3254, 0x03},
	{0x3255, 0xFF},
	{0x3256, 0x00},
	{0x3257, 0x50},

	{0x3270, 0x00}, // Gamma
	{0x3271, 0x0C},
	{0x3272, 0x18},
	{0x3273, 0x32},
	{0x3274, 0x44},
	{0x3275, 0x54},
	{0x3276, 0x70},
	{0x3277, 0x88},
	{0x3278, 0x9D},
	{0x3279, 0xB0},
	{0x327A, 0xCF},
	{0x327B, 0xE2},
	{0x327C, 0xEF},
	{0x327D, 0xF7},
	{0x327E, 0xFF},

	{0x3302, 0x00}, // Color Correction
	{0x3303, 0x40},
	{0x3304, 0x00},
	{0x3305, 0x96},
	{0x3306, 0x00},
	{0x3307, 0x29},
	{0x3308, 0x07},
	{0x3309, 0xBA},
	{0x330A, 0x06},
	{0x330B, 0xF5},
	{0x330C, 0x01},
	{0x330D, 0x51},
	{0x330E, 0x01},
	{0x330F, 0x30},
	{0x3310, 0x07},
	{0x3311, 0x16},
	{0x3312, 0x07},
	{0x3313, 0xBA},

	{0x3326, 0x02}, // EExt
	{0x32F6, 0x0F},
	{0x32F9, 0x42},
	{0x32FA, 0x24},
	{0x3325, 0x4A},
	{0x3330, 0x00},
	{0x3331, 0x0A},
	{0x3332, 0xFF},
	{0x3338, 0x30},
	{0x3339, 0x84},
	{0x333A, 0x48},
	{0x333F, 0x07},

	{0x3360, 0x10}, // Auto Function
	{0x3361, 0x18},
	{0x3362, 0x1f},
	{0x3363, 0x37},
	{0x3364, 0x80},
	{0x3365, 0x80},
	{0x3366, 0x68},
	{0x3367, 0x60},
	{0x3368, 0x30},
	{0x3369, 0x28},
	{0x336A, 0x20},
	{0x336B, 0x10},
	{0x336C, 0x00},
	{0x336D, 0x20},
	{0x336E, 0x1C},
	{0x336F, 0x18},
	{0x3370, 0x10},
	{0x3371, 0x38},
	{0x3372, 0x3C},
	{0x3373, 0x3F},
	{0x3374, 0x3F},
	{0x338A, 0x34},
	{0x338B, 0x7F},
	{0x338C, 0x10},
	{0x338D, 0x23},
	{0x338E, 0x7F},
	{0x338F, 0x14},
	{0x3375, 0x0A}, 
	{0x3376, 0x0C}, 
	{0x3377, 0x10}, 
	{0x3378, 0x14}, 

	{0x3012, 0x02},
	{0x3013, 0xD0},
	{0x3060, 0x01},
};

struct NT_RegValue g_sNT99141_HD720[] = 
{
	//[YUYV_1280x720_30.00_30.04_Fps]  MCLK 24 M  PCLK 74M Fix 30.01_Fps
	{0x32BF, 0x60},
	{0x32C0, 0x5A},
	{0x32C1, 0x5A},
	{0x32C2, 0x5A},
	{0x32C3, 0x00},
	{0x32C4, 0x27},
	{0x32C5, 0x13},
	{0x32C6, 0x1F},
	{0x32C7, 0x00},
	{0x32C8, 0xDF},
	{0x32C9, 0x5A},
	{0x32CA, 0x6D},
	{0x32CB, 0x6D},
	{0x32CC, 0x79},
	{0x32CD, 0x79},
	{0x32DB, 0x7B},
	{0x32E0, 0x05},
	{0x32E1, 0x00},
	{0x32E2, 0x02},
	{0x32E3, 0xD0},
	{0x32E4, 0x00},
	{0x32E5, 0x00},
	{0x32E6, 0x00},
	{0x32E7, 0x00},
	{0x3200, 0x3E},
	{0x3201, 0x0F},
	{0x3028, 0x24},
	{0x3029, 0x20},
	{0x302A, 0x04},
	{0x3022, 0x24},
	{0x3023, 0x24},
	{0x3002, 0x00},
	{0x3003, 0x04},
	{0x3004, 0x00},
	{0x3005, 0x04},
	{0x3006, 0x05},
	{0x3007, 0x03},
	{0x3008, 0x02},
	{0x3009, 0xD3},
	{0x300A, 0x06},
	{0x300B, 0x7C},
	{0x300C, 0x02},
	{0x300D, 0xE6},
	{0x300E, 0x05},
	{0x300F, 0x00},
	{0x3010, 0x02},
	{0x3011, 0xD0},
	{0x32B8, 0x3F},
	{0x32B9, 0x31},
	{0x32BB, 0x87},
	{0x32BC, 0x38},
	{0x32BD, 0x3C},
	{0x32BE, 0x34},
	{0x3201, 0x7F},
	{0x3021, 0x06},
	{0x3060, 0x01},
};


struct NT_RegValue g_sNT99141_SVGA[] = 
{
	//[YUYV_800x600_30.00_30.04_Fps]  MCLK 24 M  PCLK 74M Fix 30.01_Fps
	{0x32BF, 0x60},
	{0x32C0, 0x5A},
	{0x32C1, 0x5A},
	{0x32C2, 0x5A},
	{0x32C3, 0x00},
	{0x32C4, 0x27},
	{0x32C5, 0x13},
	{0x32C6, 0x1F},
	{0x32C7, 0x00},
	{0x32C8, 0xDF},
	{0x32C9, 0x5A},
	{0x32CA, 0x6D},
	{0x32CB, 0x6D},
	{0x32CC, 0x79},
	{0x32CD, 0x79},
	{0x32DB, 0x7B},
	{0x32E0, 0x03},
	{0x32E1, 0x20},
	{0x32E2, 0x02},
	{0x32E3, 0x58},
	{0x32E4, 0x00},
	{0x32E5, 0x33},
	{0x32E6, 0x00},
	{0x32E7, 0x33},
	{0x3200, 0x3E},
	{0x3201, 0x0F},
	{0x3028, 0x24},
	{0x3029, 0x20},
	{0x302A, 0x04},
	{0x3022, 0x24},
	{0x3023, 0x24},
	{0x3002, 0x00},
	{0x3003, 0xA4},
	{0x3004, 0x00},
	{0x3005, 0x04},
	{0x3006, 0x04},
	{0x3007, 0x63},
	{0x3008, 0x02},
	{0x3009, 0xD3},
	{0x300A, 0x06},
	{0x300B, 0x7C},
	{0x300C, 0x02},
	{0x300D, 0xE6},
	{0x300E, 0x03},
	{0x300F, 0xC0},
	{0x3010, 0x02},
	{0x3011, 0xD0},
	{0x32B8, 0x3F},
	{0x32B9, 0x31},
	{0x32BB, 0x87},
	{0x32BC, 0x38},
	{0x32BD, 0x3C},
	{0x32BE, 0x34},
	{0x3201, 0x7F},
	{0x3021, 0x06},
	{0x3060, 0x01},
};

struct NT_RegValue g_sNT99141_VGA[] = 
{
	//[YUYV_640x480 24M MCLK, 60MHz PCLK 30F/S]
	{0x32BF, 0x60},
	{0x32C0, 0x61},
	{0x32C1, 0x61},
	{0x32C2, 0x61},
	{0x32C3, 0x00},
	{0x32C4, 0x27},
	{0x32C5, 0x13},
	{0x32C6, 0x1F},
	{0x32C7, 0x00},
	{0x32C8, 0x76},
	{0x32C9, 0x61},
	{0x32CA, 0x74},
	{0x32CB, 0x74},
	{0x32CC, 0x80},
	{0x32CD, 0x80},
	{0x32DB, 0x6D},
	{0x32E0, 0x02},
	{0x32E1, 0x80},
	{0x32E2, 0x01},
	{0x32E3, 0xE0},
	{0x32E4, 0x00},
	{0x32E5, 0x00},
	{0x32E6, 0x00},
	{0x32E7, 0x00},
	{0x3200, 0x3E},
	{0x3201, 0x0F},
	{0x3028, 0x07},
	{0x3029, 0x00},
	{0x302A, 0x08},
	{0x3022, 0x24},
	{0x3023, 0x24},
	{0x3002, 0x01},
	{0x3003, 0x44},
	{0x3004, 0x00},
	{0x3005, 0x7C},
	{0x3006, 0x03},
	{0x3007, 0xC3},
	{0x3008, 0x02},
	{0x3009, 0x5B},
	{0x300A, 0x03},
	{0x300B, 0xFC},
	{0x300C, 0x01},
	{0x300D, 0xFF},
	{0x300E, 0x02},
	{0x300F, 0x80},
	{0x3010, 0x01},
	{0x3011, 0xE0},
	{0x32B8, 0x3F},
	{0x32B9, 0x31},
	{0x32BB, 0x87},
	{0x32BC, 0x38},
	{0x32BD, 0x3C},
	{0x32BE, 0x34},
	{0x3201, 0x7F},
	{0x3021, 0x06},
	{0x3060, 0x01},
};

struct NT_RegValue g_sNT99141_VGA_30FPS[] = 
{
	 //[YUYV_640x480_30.00_30.01_Fps]  MCLK 24 M  PCLK 60M   Fix 30.01_Fps
	{0x32BF, 0x60},
	{0x32C0, 0x5A},
	{0x32C1, 0x5A},
	{0x32C2, 0x5A},
	{0x32C3, 0x00},
	{0x32C4, 0x27},
	{0x32C5, 0x13},
	{0x32C6, 0x1F},
	{0x32C7, 0x00},
	{0x32C8, 0xE0},
	{0x32C9, 0x5A},
	{0x32CA, 0x6D},
	{0x32CB, 0x6D},
	{0x32CC, 0x79},
	{0x32CD, 0x79},
	{0x32DB, 0x7B},
	{0x32E0, 0x02},
	{0x32E1, 0x80},
	{0x32E2, 0x01},
	{0x32E3, 0xE0},
	{0x32E4, 0x00},
	{0x32E5, 0x80},
	{0x32E6, 0x00},
	{0x32E7, 0x80},
	{0x3200, 0x3E},
	{0x3201, 0x0F},
	{0x3028, 0x09},
	{0x3029, 0x00},
	{0x302A, 0x04},
	{0x3022, 0x24},
	{0x3023, 0x24},
	{0x3002, 0x00},
	{0x3003, 0xA4},
	{0x3004, 0x00},
	{0x3005, 0x04},
	{0x3006, 0x04},
	{0x3007, 0x63},
	{0x3008, 0x02},
	{0x3009, 0xD3},
	{0x300A, 0x05},
	{0x300B, 0x3C},
	{0x300C, 0x02},
	{0x300D, 0xEA},
	{0x300E, 0x03},
	{0x300F, 0xC0},
	{0x3010, 0x02},
	{0x3011, 0xD0},
	{0x32B8, 0x3F},
	{0x32B9, 0x31},
	{0x32BB, 0x87},
	{0x32BC, 0x38},
	{0x32BD, 0x3C},
	{0x32BE, 0x34},
	{0x3201, 0x7F},
	{0x3021, 0x06},
	{0x3060, 0x01},
};

struct NT_RegValue g_sNT99141_50Hz[] = 
{
	//[50Hz]  
	{0x32BF, 0x60}, 
	{0x32C0, 0x5A}, 
	{0x32C1, 0x5A}, 
	{0x32C2, 0x5A}, 
	{0x32C3, 0x00}, 
	{0x32C4, 0x27}, 
	{0x32C5, 0x13}, 
	{0x32C6, 0x1F}, 
	{0x32C7, 0x00}, 
	{0x32C8, 0xDF}, 
	{0x32C9, 0x5A}, 
	{0x32CA, 0x6D}, 
	{0x32CB, 0x6D}, 
	{0x32CC, 0x79}, 
	{0x32CD, 0x79}, 
	{0x32DB, 0x7B}, 
};

struct NT_RegValue g_sNT99141_60Hz[] = 
{
	//[60Hz]       
	{0x32BF, 0x60}, 
	{0x32C0, 0x60}, 
	{0x32C1, 0x5F}, 
	{0x32C2, 0x5F}, 
	{0x32C3, 0x00}, 
	{0x32C4, 0x27}, 
	{0x32C5, 0x13}, 
	{0x32C6, 0x1F}, 
	{0x32C7, 0x00}, 
	{0x32C8, 0xBA}, 
	{0x32C9, 0x5F}, 
	{0x32CA, 0x72}, 
	{0x32CB, 0x72}, 
	{0x32CC, 0x7E}, 
	{0x32CD, 0x7F}, 
	{0x32DB, 0x77}, 
};


struct NT_RegTable g_NT99141_InitTable[] =
{
	{g_sNT99141_Init,_REG_TABLE_SIZE(g_sNT99141_Init)},
	{g_sNT99141_VGA_30FPS, _REG_TABLE_SIZE(g_sNT99141_VGA_30FPS)},	
	{g_sNT99141_SVGA,_REG_TABLE_SIZE(g_sNT99141_SVGA)},
	{g_sNT99141_HD720,_REG_TABLE_SIZE(g_sNT99141_HD720)},		
	
	{0,0}
};

#if 0
extern UINT8 u8DiffBuf[];
extern UINT8 u8OutLumBuf[];
#endif
static UINT8 g_uOvDeviceID[]= 
{
	0x00,		// not a device ID
	0xc0,		// ov6680
	0x42,		// ov7648
	0x42,		// ov7670
	0x60,		// ov2640
	0x60,		// 0v9660
	0x42,		// NT99050 = 6
	0x54,		// NT99140 = 7
	0x54,		// NT99141 = 8
	0x00		// not a device ID
};


static void SnrReset(void)
{
	gpio_configure(GPIO_PORTG, 11);					//GPIOG 10 as GPIO
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



static BOOL I2C_Write_8bitSlaveAddr_16bitReg_8bitData(UINT8 uAddr, UINT16 uRegAddr, UINT8 uData)
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

	return TRUE;
}

static UINT8 I2C_Read_8bitSlaveAddr_16bitReg_8bitData(UINT8 uAddr, UINT16 uRegAddr)
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
extern void Delay(UINT32 nCount);
#define RETRY	2
VOID NT99141_Init(UINT32 nIndex, UINT32 u32Resolution)
{
	UINT32 u32Idx;
	UINT32 u32TableSize;
	UINT8  u8DeviceID;
	UINT8 u8IDH, u8IDL;
	INT32 rtval, j;
	
	struct NT_RegValue *psRegValue;
	DBG_PRINTF("Sensor ID = %d\n", nIndex);
	if ( nIndex >= (sizeof(g_uOvDeviceID)/sizeof(UINT8)) )
		return;	
	videoIn_Open(72000, 6000);								/* For sensor clock output */	
	sysDelay(2);
	

	/* nt99141 sensor need reset without sensor clock */
	SnrPowerDown(FALSE); 	 	//NOT OK
	SnrReset();				//NOT OK
		
	sysDelay(2);
	u32TableSize = g_NT99141_InitTable[0].uTableSize;
	psRegValue = g_NT99141_InitTable[0].sRegTable;
	u8DeviceID = g_uOvDeviceID[nIndex];
	DBG_PRINTF("Device Slave Addr = 0x%x\n", u8DeviceID);
	if ( psRegValue == 0 )
		return;	
#if 1
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
	sysDelay(10);
	if(u32Resolution == REG_VALUE_HD720){
		u32TableSize = g_NT99141_InitTable[3].uTableSize;
		psRegValue = g_NT99141_InitTable[3].sRegTable;
	}else if(u32Resolution == REG_VALUE_SVGA){
		u32TableSize = g_NT99141_InitTable[2].uTableSize;
		psRegValue = g_NT99141_InitTable[2].sRegTable;
	}else if(u32Resolution == REG_VALUE_VGA){
		u32TableSize = g_NT99141_InitTable[1].uTableSize;
		psRegValue = g_NT99141_InitTable[1].sRegTable;
	}
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
#else
	#if 1
	outp32(REG_GPBFUN, inp32(REG_GPBFUN) & (~MF_GPB13));
	outp32(REG_GPBFUN, inp32(REG_GPBFUN) & (~MF_GPB14));
	DrvI2C_Open(eDRVGPIO_GPIOB, 					
				eDRVGPIO_PIN13, 
				eDRVGPIO_GPIOB,
				eDRVGPIO_PIN14, 
				(PFN_DRVI2C_TIMEDELY)Delay);
	#else
	outp32(REG_GPDFUN, inp32(REG_GPDFUN) & (~MF_GPD9));
	outp32(REG_GPDFUN, inp32(REG_GPDFUN) & (~MF_GPD10));
	DrvI2C_Open(eDRVGPIO_GPIOD, 					
				eDRVGPIO_PIN9, 
				eDRVGPIO_GPIOD,
				eDRVGPIO_PIN10, 
				(PFN_DRVI2C_TIMEDELY)Delay);
	#endif				
									
	for(u32Idx=0;u32Idx<u32TableSize; u32Idx++, psRegValue++)
	{		
		I2C_Write_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID, (psRegValue->u16RegAddr), (psRegValue->u8Value));						
	}
	id0 = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID,CHIP_VERSION_H);
	id1 = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID,CHIP_VERSION_L);
	//static UINT8 I2C_Read_8bitSlaveAddr_16bitReg_8bitData(UINT8 uAddr, UINT16 uRegAddr)
	sysprintf("Detectd sensor id0=%0x id1=%02x\n",id0, id1);
	
	u32TableSize = g_NT99141_InitTable[u32Resolution].uTableSize;
	psRegValue = g_NT99141_InitTable[u32Resolution].sRegTable;
	u8DeviceID = g_uOvDeviceID[nIndex];
	for(u32Idx=0;u32Idx<u32TableSize; u32Idx++, psRegValue++)
	{		
		I2C_Write_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID, (psRegValue->u16RegAddr), (psRegValue->u8Value));						
	}

	DrvI2C_Close();	
#endif	
}

extern UINT8 u8PlanarFrameBuffer[];
/*===================================================================
	LCD dimension = (OPT_LCD_WIDTH, OPT_LCD_HEIGHT)	
	Packet dimension = (OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT)	
	Stride should be LCM resolution  OPT_LCD_WIDTH.
	Packet frame start address = VPOST frame start address + (OPT_LCD_WIDTH-OPT_PREVIEW_WIDTH)/2*2 	
=====================================================================*/
UINT32 Smpl_NT99141_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2)
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
	outp32(0xb100201C, 0x03200103);
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

	NT99141_Init(8, REG_VALUE_VGA);			
	videoIn_Open(72000, 24000);		
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
				eVIDEOIN_PLANAR,	
				OPT_ENCODE_HEIGHT,
				OPT_CROP_HEIGHT);														
	videoinIoctl(VIDEOIN_IOCTL_HSCALE_FACTOR,
				eVIDEOIN_PLANAR,	
				640,
				640);		
					
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
		
			
#if 0	
	DrvVideoIn_SetMotionDet(TRUE, //BOOL bEnable,
								TRUE, //TRUE=8x8. FALSE = 16x16 BOOL bBlockSize,	
								FALSE);//FALSE for 1 bit Threshold+7 bits diff// BOOL bSaveMode
								
	DrvVideoIn_SetMotionDetEx(0,			//UINT32 u32DetFreq,
								20,			//UINT32 u32Threshold,
								(UINT32)u8DiffBuf,	//UINT32 u32OutBuffer,	
								(UINT32)u8OutLumBuf);	//UINT32 u32YBuffer			
#endif	
	videoinIoctl(VIDEOIN_IOCTL_SET_PIPE_ENABLE,
				TRUE, 						// Engine enable ?
				eVIDEOIN_BOTH_PIPE_ENABLE,		// which pipe was enable. 											
				0 );							//Useless		
							
					
							
	sysSetLocalInterrupt(ENABLE_IRQ);						
	sysDelay(300);													
	return Successful;			
}	

/*===================================================================
	LCD dimension = (OPT_LCD_WIDTH, OPT_LCD_HEIGHT)	
	Packet dimension = (OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT)	
	Stride should be LCM resolution  OPT_LCD_WIDTH.
	Packet frame start address = VPOST frame start address + (OPT_LCD_WIDTH-OPT_PREVIEW_WIDTH)/2*2 	
=====================================================================*/
UINT32 Smpl_NT99141_SVGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2)
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
	NT99141_Init(8, REG_VALUE_SVGA);			
	
	videoIn_Open(72000, 24000);
		
	videoIn_EnableInt(eVIDEOIN_VINT);
	
	videoIn_InstallCallback(eVIDEOIN_VINT, 
						(PFN_VIDEOIN_CALLBACK)VideoIn_InterruptHandler,
						&pfnOldCallback	);	//Frame End interrupt
						
	videoIn_SetPacketFrameBufferControl(FALSE, FALSE);	
#ifdef __3RD_PORT__
	videoinIoctl(VIDEOIN_IOCTL_SET_POLARITY,
				FALSE,							
				FALSE,							//Polarity.	
				FALSE);						
#else	
	videoinIoctl(VIDEOIN_IOCTL_SET_POLARITY,
				FALSE,							
				FALSE,							//Polarity.	
				TRUE);							
#endif 												
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
				600,				//UINT16 u16Height, 
				800,				//UINT16 u16Width;	
				0);							//Useless
				
	/* Set Packet dimension */	 							 
	videoinIoctl(VIDEOIN_IOCTL_VSCALE_FACTOR,
				eVIDEOIN_PACKET,			//272/480
				OPT_PREVIEW_HEIGHT,
				600);		
																		
	videoinIoctl(VIDEOIN_IOCTL_HSCALE_FACTOR,
				eVIDEOIN_PACKET,			//364/640
				OPT_PREVIEW_WIDTH,
				800);						
						
	/* Set Planar dimension  */							 							 
	videoinIoctl(VIDEOIN_IOCTL_VSCALE_FACTOR,
				eVIDEOIN_PLANAR,			//640/640
				OPT_ENCODE_HEIGHT,
				600);																
	videoinIoctl(VIDEOIN_IOCTL_HSCALE_FACTOR,
				eVIDEOIN_PLANAR,			//640/640
				OPT_ENCODE_WIDTH,
				800);

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
	
#if 0	
	DrvVideoIn_SetMotionDet(TRUE, //BOOL bEnable,
								TRUE, //TRUE=8x8. FALSE = 16x16 BOOL bBlockSize,	
								FALSE);//FALSE for 1 bit Threshold+7 bits diff// BOOL bSaveMode
								
	DrvVideoIn_SetMotionDetEx(0,			//UINT32 u32DetFreq,
								20,			//UINT32 u32Threshold,
								(UINT32)u8DiffBuf,	//UINT32 u32OutBuffer,	
								(UINT32)u8OutLumBuf);	//UINT32 u32YBuffer			
#endif	
	videoinIoctl(VIDEOIN_IOCTL_SET_PIPE_ENABLE,
				TRUE, 						// Engine enable ?
				eVIDEOIN_BOTH_PIPE_ENABLE,		// which pipe was enable. 											
				0 );							//Useless		
							
					
							
	sysSetLocalInterrupt(ENABLE_IRQ);						
														
	return Successful;			
}
/*===================================================================
	LCD dimension = (OPT_LCD_WIDTH, OPT_LCD_HEIGHT)	
	Packet dimension = (OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT)	
	Stride should be LCM resolution  OPT_LCD_WIDTH.
	Packet frame start address = VPOST frame start address + (OPT_LCD_WIDTH-OPT_PREVIEW_WIDTH)/2*2 	
=====================================================================*/
UINT32 Smpl_NT99141_HD(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2)
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
	//videoIn_Init(TRUE, 0, 24000, eVIDEOIN_2ND_SNR_CCIR601);	
	videoIn_Init(TRUE, 0, 24000, eVIDEOIN_2ND_SNR_CCIR601_2);	
#endif		

	NT99141_Init(8, REG_VALUE_HD720);			
	videoIn_Open(72000, 24000);		
	videoIn_EnableInt(eVIDEOIN_VINT);
	
	videoIn_InstallCallback(eVIDEOIN_VINT, 
						(PFN_VIDEOIN_CALLBACK)VideoIn_InterruptHandler,
						&pfnOldCallback	);	//Frame End interrupt
						
	videoIn_SetPacketFrameBufferControl(FALSE, FALSE);	
#ifdef __3RD_PORT__
	videoinIoctl(VIDEOIN_IOCTL_SET_POLARITY,
				FALSE,							
				FALSE,							//Polarity.	
				FALSE);						
#else		
	videoinIoctl(VIDEOIN_IOCTL_SET_POLARITY,
				FALSE,							//NT99050	
				FALSE,							//Polarity.	
				TRUE);							
#endif												
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
				720,							//UINT16 u16Height, 
				1280,						//UINT16 u16Width;	
				0);							//Useless
		 							 
	videoinIoctl(VIDEOIN_IOCTL_VSCALE_FACTOR,
				eVIDEOIN_PACKET,			
				OPT_PREVIEW_HEIGHT,
				720);		
																		
	videoinIoctl(VIDEOIN_IOCTL_HSCALE_FACTOR,
				eVIDEOIN_PACKET,			
				OPT_PREVIEW_WIDTH,
				1280);		
					 							 
	videoinIoctl(VIDEOIN_IOCTL_VSCALE_FACTOR,
				eVIDEOIN_PLANAR,			
				OPT_ENCODE_HEIGHT,
				720);	
																		
	videoinIoctl(VIDEOIN_IOCTL_HSCALE_FACTOR,
				eVIDEOIN_PLANAR,			
				OPT_ENCODE_WIDTH,
				1280);
				
	/* Set Pipes stride */					
	videoinIoctl(VIDEOIN_IOCTL_SET_STRIDE,										
				OPT_STRIDE,				
				OPT_ENCODE_WIDTH,
				0);
				
	/* Set Packet Buffer Address */	
	videoinIoctl(VIDEOIN_IOCTL_SET_BUF_ADDR,
				eVIDEOIN_PACKET,			
				0, 							//Packet buffer addrress 0	
				(UINT32)((UINT32)u8PlanarFrameBuffer + (OPT_STRIDE-OPT_PREVIEW_WIDTH)/2*2) );					
		
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
	
#if 0	
	DrvVideoIn_SetMotionDet(TRUE, //BOOL bEnable,
								TRUE, //TRUE=8x8. FALSE = 16x16 BOOL bBlockSize,	
								FALSE);//FALSE for 1 bit Threshold+7 bits diff// BOOL bSaveMode
								
	DrvVideoIn_SetMotionDetEx(0,			//UINT32 u32DetFreq,
								20,			//UINT32 u32Threshold,
								(UINT32)u8DiffBuf,	//UINT32 u32OutBuffer,	
								(UINT32)u8OutLumBuf);	//UINT32 u32YBuffer			
#endif	
	videoinIoctl(VIDEOIN_IOCTL_SET_PIPE_ENABLE,
				TRUE, 						// Engine enable ?
				eVIDEOIN_BOTH_PIPE_ENABLE,				// which pipe was enable. 											
				0 );							//Useless		
							
					
							
	sysSetLocalInterrupt(ENABLE_IRQ);						
														
	return Successful;			
}	