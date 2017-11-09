#include "wblib.h"
#include "W55FA95_VideoIn.h"
#include "W55FA95_GPIO.h"
#include "w55fa95_i2c.h"
#include "demo.h"

#include "DrvI2C.h"
// LG 303B : I2C slave address 0x30
#define CONFIG_W55FA95_HI702_BOARD_PCBA	/* Rainbow 1st PCB */
//#define CONFIG_W55FA95_HI702_BOARD_DV1	/* Rainbow 2nd PCB */
typedef struct tagLGRef
{
	UINT8	u8RegAddr;
	UINT8	u8RegData; 
}T_LG_I2C;

//#define LGE_CCIR656
static T_LG_I2C g_sLG303B_RegValue[]=
{	
	{0x03, 0x00},
	{0x01, 0xf1},	//PWRCTL
	{0x01, 0xf3},	//PWRCTL
	{0x01, 0xf1},	//PWRCTL
	
	{0x03, 0x20},	//page20
	{0x10, 0x0c},	//AE OFF
	
	{0x03, 0x22},	//page22
	{0x10, 0x6b},	//AWB OF
	
	//Control image size, Windowing and Sync
	{0x03, 0x00},	//page0
	{0x10, 0x00},	
	{0x11, 0xb0},//younjung.park temp{0x11, 0x90},	//VDOCTL2
	{0x12, 0x24},	//0x20},	//SYNCCTL	//pclk invert
	{0x20, 0x00},	//WINROWH
	{0x21, 0x06},	//WINROWL
	{0x22, 0x00},	//WINCOLH
	{0x23, 0x06},	//WINCOLL
	{0x24, 0x01},	//WINHGTH
	{0x25, 0xe0},	//WINHGTL
	{0x26, 0x02},	//WINWIDH
	{0x27, 0x80},	//WINWIL
	
	{0x40, 0x01}, //Hblank 516
	{0x41, 0x50}, 
	{0x42, 0x00}, //Vblank 20
	{0x43, 0x14}, 

	//Black level calibration
	{0x80, 0x3e},	//BLCCTL
	{0x81, 0x96},	//_hidden_
	{0x82, 0x90},	//_hidden_
	{0x83, 0x00},	//_hidden_
	{0x84, 0x20},	//_hidden_
	
	{0x03, 0x00}, //PAGE 0
	{0x90, 0x0c}, //BLC_TIME_TH_ON
	{0x91, 0x0c}, //BLC_TIME_TH_OFF 
	{0x92, 0xd8}, //BLC_AG_TH_ON
	{0x93, 0xd0}, //BLC_AG_TH_OFF
	
	{0x94, 0x88},	//BLCDGH
	{0x95, 0x80},	//BLCDGL
	{0x98, 0x20},	//_hidden_
	{0xA0, 0x41},	//DOFSB
	{0xA2, 0x41},	//DOFSGB
	{0xA4, 0x41},	//DOFSR
	{0xA6, 0x41},	//DOFSGR
	{0xA8, 0x00},	//_hidden_
	{0xAA, 0x00},	//_hidden_
	{0xAC, 0x00},	//_hidden_
	{0xAE, 0x00},	//_hidden_
	
	//Analog Power Control
	{0x03, 0x02},	//page 2
	{0x10, 0x00},
	{0x13, 0x00},
	{0x18, 0x1c},
	{0x19, 0x00},
	{0x1A, 0x00},
	{0x1B, 0x08},
	{0x1C, 0x00},
	{0x1D, 0x00},
	{0x20, 0x33},
	{0x21, 0xaa},
	{0x22, 0xa6},	//09.04.27(from SFT)_0x76
	{0x23, 0xb0},
	{0x31, 0x99},
	{0x32, 0x00},
	{0x33, 0x00},
	{0x34, 0x3C},
	{0x50, 0x21},
	{0x54, 0x30},
	{0x56, 0xfe},
	{0x62, 0x78},
	{0x63, 0x9e},
	{0x64, 0x78},
	{0x65, 0x9e},
	{0x72, 0x7a},
	{0x73, 0x9a},
	{0x74, 0x7a},
	{0x75, 0x9a},
	{0x82, 0x09},
	{0x84, 0x09},
	{0x86, 0x09},
	{0xA0, 0x03},
	{0xA8, 0x1d},
	{0xAA, 0x49},
	{0xB9, 0x8a},
	{0xBB, 0x8a},
	{0xBC, 0x04},
	{0xBD, 0x10},
	{0xBE, 0x04},
	{0xBF, 0x10},
	
	//Control image format and Image Effect
	{0x03, 0x10},	//page 10
	{0x10, 0x01},	//ISPCTL1	// 20110525 jinkwan.kim@lge.com camsensor YCbCr order
	{0x12, 0x30},	//ISPCTL3
	{0x41, 0x00},	//DYOFS
	{0x50, 0x60},	//AGBRT
	{0x60, 0x3d},	//SATCTL
	{0x61, 0x90},	//2011 0412//SATB
	{0x62, 0x79},	//2011 0412//SATR
	{0x63, 0x50},	//AGSAT
	{0x64, 0x80},	//_hidden_
	
#ifdef LGE_CCIR656 
	{0x10, 0x5},	// bit 2 ITU656 mode.
	//{0x14, 0x10},	// codeword sync = 0x8010
	
	{0x14, 0x15},	// codeword sync = 0x0000
	//{0x14, 0x05},	// codeword sync = 0x8010
	
	//{0x14, 0x04},	
	//{0x14, 0x00},
#endif	
	
	//LPF
	{0x03, 0x11},	//page 11
	{0x10, 0x19},	//ZLPFCTL1
	{0x11, 0x0e},	//ZLPFCTL2
	{0x21, 0x30},	//ZLPFAGTH
	{0x50, 0x03},	//_hidden_
	{0x60, 0x06},	//ZLPFTH
	{0x62, 0x43},	//ZLPFLLVL
	{0x63, 0x63},	//ZLFPDYTH
	{0x74, 0x0d},	//_hidden_
	{0x75, 0x0d},	//_hidden_
	
	//YCLPF
	{0x03, 0x12},	//page 12
	{0x40, 0x23},	//YCLPFCTL1
	{0x41, 0x37},	//YCLPFCTL2
	{0x50, 0x01},	//YCLPFTH
	{0x70, 0x1d},	//BLPFCTL
	{0x74, 0x0f},	//BLPFTH1
	{0x75, 0x0f},	//BLPFTH2
	{0x91, 0x34},	//_hidden_
	{0xB0, 0xc9},	//_hidden_
	{0xD0, 0xb1},	//_hidden
	
	//Edge Enhancement
	{0x03, 0x13},	//page 13
	{0x10, 0x35},	//EDGECTL1
	{0x11, 0x01},	//EDGECTL2
	{0x12, 0x00},	//_hidden_
	{0x13, 0x02},	//_hidden_
	{0x14, 0x01},	//EDGECTL3
	{0x20, 0x03},	//EDGENGAIN
	{0x21, 0x01},	//EDGEPGAIN
	{0x23, 0x2f},	//EDGEHCLPTH
	{0x24, 0x0f},	//EDGELCLIPTH
	{0x25, 0x40},	//_hidden_
	{0x28, 0x00},	//EDGETIMETH
	{0x29, 0x48},	//EDGEAGTH
	{0x30, 0xff},	//_hidden_
	{0x80, 0x0b},	//EDGE2DCTL1
	{0x81, 0x11},	//EDGE2DCTL2
	{0x83, 0x5d},	//EDGE2DCTL3
	{0x90, 0x05},	//EDGE2DNGAIN
	{0x91, 0x05},	//EDGE2DPGAIN
	{0x93, 0x2f},	//EDGE2DHCLIPTH
	{0x94, 0x0f},	//EDGE2DLCLIPTH
	{0x95, 0x80},	//_hidden_
	
	//Lens shading
	{0x03, 0x14},	//page 14
	{0x10, 0x05},	//LENSCTL
	{0x20, 0x80},	//center x-axis
	{0x21, 0x80},	//center y-axis
	{0x22, 0x40}, //R Lens Correction	
	{0x23, 0x2c},	//G Lens Correction
	{0x24, 0x32},	//B Lens Correction
	{0x25, 0x69},	//LAGOFF
	{0x26, 0x67},	//LAGON
	
	//Color Correction
	{0x03, 0x15},
	{0x10, 0x0f},	//CMCCTL
	//Rstep 16
	{0x14, 0x2c},	//CMCOFSGM 
	{0x16, 0x1c},	//CMCOFSGL
	{0x17, 0x2d},	//CMC SIGN
	//CMC
	{0x30, 0x5c},
	{0x31, 0x1f},
	{0x32, 0x03},
	{0x33, 0x0b},
	{0x34, 0x5a},
	{0x35, 0x0f},
	{0x36, 0x04},
	{0x37, 0x2d},
	{0x38, 0x69},
	//CMC OFS
	{0x40, 0x82},
	{0x41, 0x04},
	{0x42, 0x82},
	{0x43, 0x02},
	{0x44, 0x86},
	{0x45, 0x04},
	{0x46, 0x83},
	{0x47, 0x99},
	{0x48, 0x1c},
	
	//Gamma Correction
	{0x03, 0x16},	//page 16
	{0x30, 0x00},	//GMA0
	{0x31, 0x09},	//GMA1
	{0x32, 0x1c},	//GMA2
	{0x33, 0x31},	//GMA3
	{0x34, 0x54},	//GMA4
	{0x35, 0x71},	//GMA5
	{0x36, 0x8a},	//GMA6
	{0x37, 0x9f},	//GMA7
	{0x38, 0xb1},	//GMA8
	{0x39, 0xc0},	//GMA9
	{0x3a, 0xcd},	//GMA10
	{0x3b, 0xe1},	//GMA11
	{0x3c, 0xef},	//GMA12
	{0x3d, 0xf8},	//GMA13
	{0x3e, 0xff},	//GMA14
	
	//Auto Flicker Cancellation
	{0x03, 0x17},	//page 17
	{0xC4, 0x49},	//FLK200
	{0xC5, 0x3c},	//FLK100
	
	//scaling
	{0x03, 0x18},	//page 18
	{0x10, 0x00},	//scaling off
	
	//Auto Exposure
	{0x03, 0x20},	//page 20
	{0x10, 0x0c},	//AECTL1
	{0x11, 0x00},	//AECTL2
	{0x20, 0x01},	//AEFRAMECTL
	{0x28, 0x3f},	//AEFINECTL1
	{0x29, 0xaa},	//AEFINECTL2
	
	{0x2A, 0xf0}, //for Variable fps
	{0x2B, 0x34}, //for Variable fps
	{0x30, 0x78},	//_hidden_
	
	{0x60, 0xA8}, //AEWGT
	{0x70, 0x3c}, //YLVL //yvyl is luminance level to converge in AE operation
	{0x78, 0x23}, //YTH1
	{0x79, 0x1e}, //YTH2
	{0x7A, 0x24}, //_hidden_
	
	{0x83, 0x00}, //EXP Normal 30.00 fps 
	{0x84, 0xc3},						 
	{0x85, 0x50},						 
	{0x86, 0x00}, //EXPMin 6000.00 fps	 
	{0x87, 0xfa},						 
	
	//MIN 15FPS 60HZ
	{0x88, 0x02}, //EXP Max 10.00 fps 
	{0x89, 0x49}, 
	{0x8a, 0xf0}, 
		
	{0x8B, 0x3a}, //EXP100 
	{0x8C, 0x98}, 
	{0x8D, 0x30}, //EXP120 
	{0x8E, 0xd4}, 
	
	{0x8F, 0xc4},	//EXPDPCH
	{0x90, 0x68},	//EXPDPCL
	
	{0x91, 0x02}, //EXP Fix 10.00 fps
	{0x92, 0x40}, 
	{0x93, 0x2c}, 
	{0x98, 0x8c},	//EXPOUT1
	{0x99, 0x23},	//EXPOUT2
	
	{0x9c, 0x06}, //EXP Limit 771.79 fps 
	{0x9d, 0xd6}, 
	{0x9e, 0x00}, //EXP Unit 
	{0x9f, 0xfa}, 
	 
	
	{0xB0, 0x18},	//AG
	{0xB1, 0x14},	//AGMIN 
	{0xB2, 0xe0},	//AGMAX
	{0xB3, 0x14},	//AGLVL
	{0xB4, 0x14},	//AGTH1
	{0xB5, 0x38},	//AGTH2
	{0xB6, 0x26},	//AGBTH1
	{0xB7, 0x20},	//AGBTH2
	{0xB8, 0x1d},	//AGBTH3
	{0xB9, 0x1b},	//AGBTH4
	{0xBA, 0x1a},	//AGBTH5
	{0xBB, 0x19},	//AGBTH6
	{0xBC, 0x19},	//AGBTH7
	{0xBD, 0x18},	//AGBTH8
	{0xC0, 0x10},	//AGSKY
	{0xC3, 0x60},	//AGDPCON
	{0xC4, 0x58},	//AGDPCOFF
	{0xC8, 0x90},	//DGMAX
	{0xC9, 0x80},	//DGMIN
	
	//Auto white balance
	{0x03, 0x22},	//page 22
	{0x10, 0x6a},	//AWBCTL1
	{0x11, 0x2c},	//AWBCTL2, outdoor limt[1]
	{0x20, 0x01}, //AE Weight, Enable B[0]
	{0x21, 0x40},	//_hidden_
	
	{0x30, 0x80},	//ULVL
	{0x31, 0x80},	//VLVL
	{0x38, 0x12},	//UVTH1
	{0x39, 0x66},	//UVTH2
	{0x40, 0xf3},	//YRANGE
	{0x41, 0x55},	//CDIFF
	{0x42, 0x33},	//CSUM
	{0x43, 0xf5},	///_hidden_
	{0x44, 0xaa},	//_hidden_
	{0x45, 0x66},	//_hidden_
	{0x46, 0x0a},	//WHTPXLTH
	
	{0x60, 0x95}, //AE weight

	{0x80, 0x30},	//RGAIN
	{0x81, 0x20},	//GGAIN
	{0x82, 0x30},	//BGAIN
	
	{0x83, 0x50},	//RMAX
	{0x84, 0x0e},	//RMIN
	{0x85, 0x64},	//BMAX
	{0x86, 0x18},	//BMIN
	
	{0x87, 0x48},	//RmaxB
	{0x88, 0x35},	//Rmin B
	{0x89, 0x30},	//Bmax B
	{0x8a, 0x20},	//Bmin B
	
	{0x8B, 0x05},	//BOUNDARY STEP//Rbexplmt
	{0x8d, 0x17},	//Rdelta
	{0x8e, 0x61},	//Bdelta
				   
	{0x8F, 0x53},	//BGAINPARA1
	{0x90, 0x52},	//BGAINPARA2
	{0x91, 0x4e},	//BGAINPARA3
	{0x92, 0x41},	//BGAINPARA4
	{0x93, 0x30},	//BGAINPARA5
	{0x94, 0x23},	//BGAINPARA6
	{0x95, 0x17},	//BGAINPARA7
	{0x96, 0x0c},	//BGAINPARA8
	{0x97, 0x03},	//BGAINPARA9 
	{0x98, 0x02},	//BGAINPARA10
	{0x99, 0x02},	//BGAINPARA11
	{0x9A, 0x02},	//BGAINPARA12
	{0x9B, 0x0a},	//BGAINBND
	{0x10, 0xea},	//AWBCLT1
	
	{0x03, 0x20},	//page 20
	{0x10, 0x8c},	//60hz//AECTL1
	
	{0x03, 0x00},
	{0x01, 0xf0},	//sleep 0ff

    //{SEQUENCE_WAIT_MS, 0x64},
    //{SEQUENCE_END, 0x00}
 };



void HI702_sensorSuspend(BOOL bIsSuspend)
{		
	/* Standby GPB4 = LOW */		
	outpw(REG_GPBFUN, inpw(REG_GPBFUN) &~ (0x3 << (4<<1)));
				
	outpw(REG_GPIOB_OMD , inpw(REG_GPIOB_OMD) & ~((1<<4) & ((1<<4) ^ (1<<4))));
	outpw(REG_GPIOB_OMD , inpw(REG_GPIOB_OMD) | ((1<<4) & (1<<4)));	
	if(bIsSuspend==TRUE)
	{// suspend GPB4 = LOW. 									
		outpw(REG_GPIOB_DOUT , inpw(REG_GPIOB_DOUT) & ~((1<<4) & ((1<<4) ^ (0<<4))));
		outpw(REG_GPIOB_DOUT , inpw(REG_GPIOB_DOUT) | ((1<<4) & (0<<4)));	
		sysprintf("Sensor suspend\n");
	}
	else
	{//Non suspend GPB4 = High. 
		outpw(REG_GPIOB_DOUT , inpw(REG_GPIOB_DOUT) & ~((1<<4) & ((1<<4) ^ (1<<4))));
		outpw(REG_GPIOB_DOUT , inpw(REG_GPIOB_DOUT) | ((1<<4) & (1<<4)));	
	}
}



void HI702_sensorPoweron(BOOL bIsPowerOn)
{
#ifdef CONFIG_W55FA95_HI702_BOARD_DV1
	UINT32 u32RegData;
	/* GPD0 acts as CAM2V8(1: on, 0: off), 
	  
	   GPB4 acts as CHIP_ENABLE(1: Enable, 0: Suspend)
	   GPD12 acts as CAM1V8 (1: on, 0: off)*/
	/* CAM2V8 => output mode */
	outpw(REG_GPDFUN , inpw(REG_GPDFUN) & ~(0x3 << (0<<1)) );	/* GPD0 switch to GPIO */ 	
	u32RegData = inpw(REG_GPIOD_OMD);		
	outpw(REG_GPIOD_OMD , (u32RegData | (1<<0)));			/* GPD0 set to output mode */        		
	if(bIsPowerOn==TRUE)
	{//CAM2V8 on
		outpw(REG_GPIOD_DOUT , inpw(REG_GPIOD_DOUT)|(1) );
	}
	else
	{//CAM2V8 off 
		outpw(REG_GPIOD_DOUT , inpw(REG_GPIOD_DOUT)&~(1) );
	}
	/* CAM1V8 => output mode */
	outpw(REG_GPDFUN , inpw(REG_GPDFUN) & ~(0x3 << (12<<1)) );	/* GPD0 switch to GPIO */ 	
	u32RegData = inpw(REG_GPIOD_OMD);		
	outpw(REG_GPIOD_OMD , (u32RegData | (1<<12)));			/* GPD0 set to output mode */        		
	if(bIsPowerOn==TRUE)
	{//CAM2V8 on
		outpw(REG_GPIOD_DOUT , inpw(REG_GPIOD_DOUT)|(1<<12) );
	}
	else
	{//CAM2V8 off 
		outpw(REG_GPIOD_DOUT , inpw(REG_GPIOD_DOUT)&~(1<<12) );
	}				
#endif
#ifdef CONFIG_W55FA95_HI702_BOARD_PCBA
	#if 0
	/* Power on GPD0 = High */
	gpio_configure(GPIO_PORTD, 
					(0));	// pin number	
	gpio_setportdir(GPIO_PORTD, 
					(1<<0), 	// Mask 
					(1<<0));	// 1 output. 0 input.			
	gpio_setportval(GPIO_PORTD, 
					(1<<0), 	//Mask
					//(1<<0));	//High				
					(1<<0));	// power on

	#else			
	outpw(REG_GPDFUN , inpw(REG_GPDFUN) &~ (0x3 << (0<<1)));
															
	outpw(REG_GPIOD_OMD , inpw(REG_GPIOD_OMD) & ~(1 & (1 ^ 1)));
	outpw(REG_GPIOD_OMD , inpw(REG_GPIOD_OMD) | (1 & 1));						
	if(bIsPowerOn==TRUE)
	{	
		outpw(REG_GPIOD_DOUT , inpw(REG_GPIOD_DOUT) & ~(1 & (1 ^ 1)));
		outpw(REG_GPIOD_DOUT , inpw(REG_GPIOD_DOUT) | (1 & 1));
	}
	else
	{
		outpw(REG_GPIOD_DOUT , inpw(REG_GPIOD_DOUT) & ~(1 & (1 ^ 1)));
		outpw(REG_GPIOD_DOUT , inpw(REG_GPIOD_DOUT) | (1 & 1));
		sysprintf("Sensor power off\n");
	}
	#endif
#endif
}

void HI702_sensorReset(BOOL bIsReset)
{//GPD13 acts as CAM_RESET,(1: reset inactive, 0: reset) 
#ifdef CONFIG_W55FA95_HI702_BOARD_DV1
	UINT32 u32RegData;	
	outpw(REG_GPDFUN , inpw(REG_GPDFUN) & ~(0x3 << (13<<1)) );	/* GPD13 switch to GPIO */ 			
	u32RegData = inpw(REG_GPIOD_OMD);							/* GPD13 set to output mode */
	outpw(REG_GPIOD_OMD , inpw(REG_GPIOD_OMD) | (1 << 13));	        /* 1 output. 0 input */
	if(bIsReset==TRUE)
	{//0: reset
		outpw(REG_GPIOD_DOUT , inpw(REG_GPIOD_DOUT)&~(1<<13) );
	}
	else
	{//1: non-reset
		outpw(REG_GPIOD_DOUT , inpw(REG_GPIOD_DOUT)|(1<<13) );
	}		
#endif
}

#if 1
#define RETRY	1
VOID LG303B_Init(UINT32 nIndex)
{
	UINT32 u32Idx;
	UINT32 u32TableSize;
	UINT8  u8DeviceID;
	UINT8 u8ID;
	INT32 rtval;
	INT j;	
	
	HI702_sensorPoweron(TRUE);
	HI702_sensorSuspend(FALSE);
	
	u32TableSize = sizeof(g_sLG303B_RegValue) / sizeof (T_LG_I2C);
	DBG_PRINTF("Sensor ID = %d\n", nIndex);
#if 0	
	if ( nIndex >= (sizeof(g_uOvDeviceID)/sizeof(UINT8)) )
		return;
#endif		
//#ifdef __FPGA__
//	videoIn_Open(12000, 12000);								/* For sensor clock output */
//	outp32(REG_CLKDIV0, inp32(REG_CLKDIV0) | 0x00E00000);		
//#else
	videoIn_Open(48000, 24000);								/* For sensor clock output */	
//#endif		 										
		
	//u32TableSize = g_OV_InitTable[nIndex].u32TableSize;
	//psRegValue = g_OV_InitTable[nIndex].sRegTable;
	//u8DeviceID = g_uOvDeviceID[nIndex];
	u8DeviceID = 0x60;
	sysprintf("Device Slave Addr = 0x%x\n", u8DeviceID);
	//if ( psRegValue == 0 )
	//	return;			
	/* Software I2C use GPIOB 13,14 */	
	i2cInit();	
	/* Byte Write/Random Read */
	rtval = i2cOpen();
	if(rtval < 0)
	{
		sysprintf("Open I2C error!\n");
		return;
	}
	i2cIoctl(I2C_IOC_SET_DEV_ADDRESS, (0x60>>1), 0);  
	i2cIoctl(I2C_IOC_SET_SPEED, 100, 0);	
	i2cIoctl(I2C_IOC_SET_SINGLE_MASTER, 1, 0); 
	for(u32Idx=0;u32Idx<u32TableSize; u32Idx++)
	{
		j = RETRY;
		i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, g_sLG303B_RegValue[u32Idx].u8RegAddr, 1);
		while(j-- > 0) 
		{
			if(i2cWrite(&(g_sLG303B_RegValue[u32Idx].u8RegData), 1) == 1)
				break;
		}						
		if(j < 0)
			sysprintf("WRITE ERROR [%d]!\n", u32Idx);				
	}	
	i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, 0x24, 1);	
	j = RETRY;
	while(j-- > 0) 
	{
		if(i2cRead_OV(&u8ID, 1) == 1)
			break;
	}
	if(j < 0)
		sysprintf("Read Sensor CR24 \n");
	sysprintf("Sensor CR24 = 0x%x\n", u8ID);

	i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, 0x25, 1);	
	j = RETRY;
	while(j-- > 0) 
	{
		if(i2cRead_OV(&u8ID, 1) == 1)
			break;
	}
	if(j < 0)
		DBG_PRINTF("Read Sensor CR25\n");
	sysprintf("Sensor CR25 = 0x%x\n", u8ID);

	i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, 0x26, 1);	
	j = RETRY;
	while(j-- > 0) 
	{
		if(i2cRead_OV(&u8ID, 1) == 1)
			break;
	}
	if(j < 0)
		DBG_PRINTF("Read Sensor CR26 \n");
	sysprintf("Sensor CR26 = 0x%x\n", u8ID);
	
	i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, 0x27, 1);	
	j = RETRY;
	while(j-- > 0) 
	{
		if(i2cRead_OV(&u8ID, 1) == 1)
			break;
	}
	if(j < 0)
		DBG_PRINTF("Read Sensor CR27 \n");
	sysprintf("Sensor CR27 = 0x%x\n", u8ID);
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

BOOL I2C_Write_8bitSlaveAddr_8bitReg_8bitData(UINT8 uAddr, UINT8 uRegAddr, UINT8 uData)
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

static VOID LG303B_Init(UINT32 nIndex)
{
	UINT32 u32Idx;
	UINT32 u32TableSize;
	UINT8  u8DeviceID;
	UINT8 u8ID;
	struct OV_RegValue *psRegValue;
	
#ifdef __FPGA__
	videoIn_Open(12000, 12000);								/* For sensor clock output */
	outp32(REG_CLKDIV0, inp32(REG_CLKDIV0) | 0x00E00000);		
#else
	videoIn_Open(48000, 24000);								/* For sensor clock output */	
#endif	
	u32TableSize = sizeof(g_sLG303B_RegValue) / sizeof (T_LG_I2C);

#ifdef __GPIO_PIN__		
	gpio_open(GPIO_PORTB, 13);				//GPIOB 13 as GPIO
	gpio_open(GPIO_PORTB, 14);				//GPIOB 14 as GPIO
#else	
	gpio_open(GPIO_PORTB);				//GPIOB as GPIO
	
#endif 	
	DrvI2C_Open(eDRVGPIO_GPIOB, 					
				eDRVGPIO_PIN13, 
				eDRVGPIO_GPIOB,
				eDRVGPIO_PIN14, 
				(PFN_DRVI2C_TIMEDELY)Delay);
									
	for(u32Idx=0;u32Idx<u32TableSize; u32Idx++)
	{
		I2C_Write_8bitSlaveAddr_8bitReg_8bitData(0x60, (g_sLG303B_RegValue[u32Idx].u8RegAddr), (g_sLG303B_RegValue[u32Idx].u8RegData));	
		//u8Data = I2C_Read_8bitSlaveAddr_8bitReg_8bitData( 0x4A, (g_sSA71113_RegValue[i].u8RegAddr));
		  //sysprintf("Addrr 0x%x = 0x%x\n", (g_sSA71113_RegValue[i].u8RegAddr), u8Data);				
	}
	u8ID = I2C_Read_8bitSlaveAddr_8bitReg_8bitData(0x60, 0x24);
	DBG_PRINTF("CR0x24 = 0x%x\n", u8ID);
	u8ID = I2C_Read_8bitSlaveAddr_8bitReg_8bitData(0x60, 0x25);
	DBG_PRINTF("CR0x25 = 0x%x\n", u8ID);
	u8ID = I2C_Read_8bitSlaveAddr_8bitReg_8bitData(0x60, 0x26);
	DBG_PRINTF("CR0x26 = 0x%x\n", u8ID);
	u8ID = I2C_Read_8bitSlaveAddr_8bitReg_8bitData(0x60, 0x27);
	DBG_PRINTF("CR0x27 = 0x%x\n", u8ID);

	DrvI2C_Close();	
}
#endif


/*===================================================================
	LCD dimension = (OPT_LCD_WIDTH, OPT_LCD_HEIGHT)	
	Packet dimension = (OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT)	
	Stride should be LCM resolution  OPT_LCD_WIDTH.
	Packet frame start address = VPOST frame start address + (OPT_LCD_WIDTH-OPT_PREVIEW_WIDTH)/2*2 	
=====================================================================*/
UINT32 Smpl_KLE303B_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2)
{
	PFN_VIDEOIN_CALLBACK pfnOldCallback;
	PUINT8 pu8PacketBuf;

#if 1
	pu8PacketBuf = (PUINT8)((UINT32)pu8FrameBuffer0 | 0x80000000);
	sysprintf("memset 0x%x\n", (UINT32)pu8PacketBuf);
	memset(pu8PacketBuf, 0x0, OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT*2);
	pu8PacketBuf = (PUINT8)((UINT32)pu8FrameBuffer1 | 0x80000000);
	sysprintf("memset 0x%x\n", (UINT32)pu8PacketBuf);
	memset(pu8PacketBuf, 0x0, OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT*2);
	pu8PacketBuf = (PUINT8)((UINT32)pu8FrameBuffer2 | 0x80000000);
	sysprintf("memset 0x%x\n", (UINT32)pu8PacketBuf);
	memset(pu8PacketBuf, 0x0, OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT*2);
#endif

	gpio_configure(GPIO_PORTA, 
					//(1<<5));	// pin number	
						5);	// pin number	
	gpio_setportdir(GPIO_PORTA, 
					(1<<5), 	// Mask 
					(1<<5));	// 1 output. 0 input.	
	gpio_setportval(GPIO_PORTA, 
					(1<<5), 	//Mask
					(1<<5));	//High	
					
	gpio_configure(GPIO_PORTG, 
					//(1<<5));	// pin number	
						5);	// pin number	
	gpio_setportdir(GPIO_PORTG, 
					(1<<5), 	// Mask 
					(1<<5));	// 1 output. 0 input.		
					
	gpio_setportval(GPIO_PORTG, 
					(1<<5), 	//Mask
					(1<<5));	//High	
	
	sysprintf("Init VPOST\n");
	InitVPOST(pu8FrameBuffer0);	

	sysprintf("videoIn_Init\n");
#ifdef __1ST_PORT__	
	#ifdef LGE_CCIR656
	videoIn_Init(TRUE, 0, 24000, eVIDEOIN_SNR_CCIR656);	
	#else	
	videoIn_Init(TRUE, 0, 24000, eVIDEOIN_SNR_CCIR601);	
	#endif	
#endif
#ifdef __2ND_PORT__
	videoIn_Init(TRUE, 0, 24000, eVIDEOIN_2ND_SNR_CCIR601);	
#endif	

#ifdef CONFIG_W55FA95_HI702_BOARD_DV1	
	HI702_sensorReset(TRUE);
	sysDelay(30);				/* Dealy 300ms */
	HI702_sensorReset(FALSE);
#endif

	sysprintf("LG303B_Init\n");
	LG303B_Init(0);
		
	sysprintf("videoIn_Open\n");		
	videoIn_Open(48000, 24000);		
	
	sysprintf("videoIn_EnableInt\n");
	videoIn_EnableInt(eVIDEOIN_VINT);
	
	videoIn_InstallCallback(eVIDEOIN_VINT, 
						(PFN_VIDEOIN_CALLBACK)VideoIn_InterruptHandler,
						&pfnOldCallback	);	//Frame End interrupt
						
	videoIn_SetPacketFrameBufferControl(FALSE, FALSE);	
												
#ifdef LGE_CCIR656		
	videoinIoctl(VIDEOIN_IOCTL_SET_INPUT_TYPE,
				0,							//
				eVIDEOIN_TYPE_CCIR656,
				0);
	videoinIoctl(VIDEOIN_IOCTL_SET_CCIR656,				
				FALSE,						//Non-standard CCIR656 mode
				NULL,						
				NULL);	
	videoinIoctl(VIDEOIN_IOCTL_SET_POLARITY,
				FALSE,						//Inverse V (SOF=0xFF000020, EOF=0xFF000001)
				TRUE,						//Inverse H(SOL0xFF000010, EOF=0xFF000001)	
				TRUE);								
#else
	videoinIoctl(VIDEOIN_IOCTL_SET_CCIR656,				
				TRUE,						//standard CCIR656 mode
				NULL,						
				NULL);	
	videoinIoctl(VIDEOIN_IOCTL_SET_POLARITY,
				FALSE,
				FALSE,							//Polarity.	
				TRUE);					
#endif									
												
	videoinIoctl(VIDEOIN_IOCTL_ORDER_INFMT_OUTFMT,								
				eVIDEOIN_IN_VYUY, 			//Input Order 
				eVIDEOIN_IN_YUV422	,		//Intput format
				eVIDEOIN_OUT_YUV422);		//Output format for packet 														
		
	videoinIoctl(VIDEOIN_IOCTL_SET_CROPPING_START_POSITION,				
				0,							//Vertical start position
				0,							//Horizontal start position	
				0);							//Useless
				
	videoinIoctl(VIDEOIN_IOCTL_CROPPING_DIMENSION,				
				OPT_CROP_HEIGHT,			//UINT16 u16Height, 
				OPT_CROP_WIDTH,				//UINT16 u16Width;	
				0);							//Useless
				
	videoinIoctl(VIDEOIN_IOCTL_VSCALE_FACTOR,
				eVIDEOIN_PACKET,			
				OPT_PREVIEW_HEIGHT,
				OPT_CROP_HEIGHT);		
				
	videoinIoctl(VIDEOIN_IOCTL_HSCALE_FACTOR,
				eVIDEOIN_PACKET,			
				OPT_PREVIEW_WIDTH,
				OPT_CROP_WIDTH);		
				
				
				
	videoinIoctl(VIDEOIN_IOCTL_VSCALE_FACTOR,
				eVIDEOIN_PLANAR,			
				OPT_ENCODE_HEIGHT,
				OPT_CROP_HEIGHT);		
				
	videoinIoctl(VIDEOIN_IOCTL_HSCALE_FACTOR,
				eVIDEOIN_PLANAR,			
				OPT_ENCODE_WIDTH,
				OPT_CROP_WIDTH);
	
#ifdef __TV__
	videoinIoctl(VIDEOIN_IOCTL_SET_STRIDE,										
				OPT_STRIDE,				
				OPT_STRIDE,
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
	
	videoinIoctl(VIDEOIN_IOCTL_SET_PIPE_ENABLE,
				TRUE, 						// Engine enable ?
				eVIDEOIN_PACKET,			// which packet was enable. 											
				0 );							//Useless		
													
	videoinIoctl(VIDEOIN_IOCTL_SET_SHADOW,
				NULL,			//640/640
				NULL,
				NULL);		
	sysSetLocalInterrupt(ENABLE_IRQ);																	
	return Successful;			
}	