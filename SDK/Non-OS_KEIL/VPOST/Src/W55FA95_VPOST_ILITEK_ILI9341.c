/***************************************************************************
 *                                                                         *
 * Copyright (c) 2007 - 2009 Nuvoton Technology Corp. All rights reserved.*
 *                                                                         *
 ***************************************************************************/
 
/****************************************************************************
 * 
 * FILENAME
 *     FA95_VPOST_ILITEK_ILI9341_MPU.c
 *
 * VERSION
 *     0.1 
 *
 * DESCRIPTION
 *
 *
 *
 *
 * DATA STRUCTURES
 *     None
 *
 * FUNCTIONS
 *
 *
 *     
 * HISTORY
 *     2009.03.16		Created by Shu-Ming Fan
 *
 *
 * REMARK
 *     None
 *
 *
 **************************************************************************/
#include "stdio.h"
#include "stdlib.h"
#include "w55fa95_vpost.h"

extern void LCDDelay(unsigned int nCount);

#if defined(HAVE_ILITEK_ILI9341_MPU)

static UINT32 g_nScreenWidth;
static UINT32 g_nScreenHeight;

typedef enum 
{
	eEXT 	= 0,
	eX32K 	= 1,
	eAPLL  	= 2,
	eUPLL  	= 3
}E_CLK;

void VPOSTpin_GPIO_init(void)
{    
    // CS pin init
	outpw(REG_GPBFUN, inpw(REG_GPBFUN) & ~MF_GPB15);	
	outpw(REG_GPIOB_OMD, inpw(REG_GPIOB_OMD) | BIT15);	
	outpw(REG_GPIOB_DOUT, inpw(REG_GPIOB_DOUT) | BIT15);	
	
    // RS pin init
	outpw(REG_GPDFUN, inpw(REG_GPDFUN) & ~MF_GPD11);
	outpw(REG_GPIOD_OMD, inpw(REG_GPIOD_OMD) | BIT11);		
	outpw(REG_GPIOD_DOUT, inpw(REG_GPIOD_DOUT) | BIT11);		
	
    // RD pin init
	outpw(REG_GPDFUN, inpw(REG_GPDFUN) & ~MF_GPD10);
	outpw(REG_GPIOD_OMD, inpw(REG_GPIOD_OMD) | BIT10);			
	outpw(REG_GPIOD_DOUT, inpw(REG_GPIOD_DOUT) | BIT10);			

    // WR pin init
	outpw(REG_GPDFUN, inpw(REG_GPDFUN) & ~MF_GPD9);
	outpw(REG_GPIOD_OMD, inpw(REG_GPIOD_OMD) | BIT9);				
	outpw(REG_GPIOD_DOUT, inpw(REG_GPIOD_DOUT) | BIT9);			
	
    // LCD bus init
	outpw(REG_GPCFUN, inpw(REG_GPCFUN) & ~0xFFFF);
	outpw(REG_GPIOC_OMD, inpw(REG_GPIOC_OMD) | 0xFF);					
}


#define CsLow()		    outp32(REG_GPIOB_DOUT, inp32(REG_GPIOB_DOUT) & ~BIT15)
#define CsHigh()		outp32(REG_GPIOB_DOUT, inp32(REG_GPIOB_DOUT) |  BIT15)

#define RsLow()		    outp32(REG_GPIOD_DOUT, inp32(REG_GPIOD_DOUT) & ~BIT11)
#define RsHigh()		outp32(REG_GPIOD_DOUT, inp32(REG_GPIOD_DOUT) |  BIT11)

#define RdLow()		    outp32(REG_GPIOD_DOUT, inp32(REG_GPIOD_DOUT) & ~BIT10)
#define RdHigh()		outp32(REG_GPIOD_DOUT, inp32(REG_GPIOD_DOUT) |  BIT10)

#define WrLow()		    outp32(REG_GPIOD_DOUT, inp32(REG_GPIOD_DOUT) & ~BIT9)
#define WrHigh()		outp32(REG_GPIOD_DOUT, inp32(REG_GPIOD_DOUT) |  BIT9)

#define lcdData_out()   outp32(REG_GPIOC_OMD, inp32(REG_GPIOC_OMD) | 0xFF)					
#define lcdData_in()    outp32(REG_GPIOC_OMD, inp32(REG_GPIOC_OMD) & ~0xFF)					

//#define LcdData_Wr(x)	outp32(REG_GPIOC_DOUT, inp32(REG_GPIOC_DOUT) | 0xFF & x)
#define LcdData_Wr(x)	outp32(REG_GPIOC_DOUT, (inp32(REG_GPIOC_DOUT) & ~0xFF) | x)
#define LcdData_Rd()	inp32(REG_GPIOC_PIN) & 0xFF




static UINT32 GetPLLOutputKhz(
	E_CLK eSysPll,
	UINT32 u32FinKHz
	)
{
	UINT32 u32Freq, u32PllCntlReg;
	UINT32 NF, NR, NO;
	
	UINT8 au8Map[4] = {1, 2, 2, 4};
	if(eSysPll==eSYS_APLL)
		u32PllCntlReg = inp32(REG_APLLCON);
	else if(eSysPll==eSYS_UPLL)	
		u32PllCntlReg = inp32(REG_UPLLCON);		
	
	NF = (u32PllCntlReg&FB_DV)+2;
	NR = ((u32PllCntlReg & IN_DV)>>9)+2;
	NO = au8Map[((u32PllCntlReg&OUT_DV)>>14)];
//	sysprintf("PLL regster = 0x%x\n", u32PllCntlReg);	
//	sysprintf("NF = %d\n", NF);
//	sysprintf("NR = %d\n", NR);
//	sysprintf("NOr = %d\n", NO);
		
	u32Freq = u32FinKHz*NF/NR/NO;
//	sysprintf("PLL Freq = %d\n", u32Freq);
	return u32Freq;
}

static void BacklightControl(int OnOff)
{	
	// GPA[11] set OUTPUT mode  => control the backlight
	outpw(REG_GPIOA_OMD, (inpw(REG_GPIOA_OMD) & 0x0000FFFF)| 0x00000800);
	if(OnOff==TRUE) {
		// GPA[11] turn on the backlight
		outpw(REG_GPIOA_DOUT, (inpw(REG_GPIOA_DOUT) & 0x0000FFFF)| 0x00000800);
	} else {
		// GPA[11] diable backlight
		outpw(REG_GPIOA_DOUT, (inpw(REG_GPIOA_DOUT) & 0x0000FFFF) & 0xFFFFF7FF);
	}
}

static delay_loop(UINT32 u32delay)
{
	volatile UINT32 ii, jj;
	for (jj=0; jj<u32delay; jj++)
		for (ii=0; ii<200; ii++);
}

static VOID LCDRegIndexWrite
(
	UINT8  u16AddrIndex		// LCD register address
)
{
    CsHigh();
    LCDDelay(10);
    WrHigh();
    RdHigh();
    CsLow();
    LCDDelay(10);    
    RsLow();
    LCDDelay(10);  
    lcdData_out();
    LcdData_Wr(u16AddrIndex);
    WrLow();
    LCDDelay(100);      
    WrHigh();
    LCDDelay(10);      
    CsHigh();
    LCDDelay(10);          
    RsHigh();
    LCDDelay(10);              
} // DrvVPOST_LCMRegIndexWrite_8Bits

static VOID LCDRegIndexWriteLast
(
	UINT8  u16AddrIndex		// LCD register address
)
{
//    CsHigh();
    LCDDelay(10);
    WrHigh();
    RdHigh();
    CsLow();
    LCDDelay(10);    
    RsLow();
    LCDDelay(10);  
    lcdData_out();
    LcdData_Wr(u16AddrIndex);
    WrLow();
    LCDDelay(100);      
    WrHigh();
    LCDDelay(10);      
    CsHigh();
    LCDDelay(10);          
    RsHigh();
    LCDDelay(10);              
} // DrvVPOST_LCMRegIndexWrite_8Bits

static VOID LCDRegWrite
(
	UINT8  u16WriteData		// LCD register contnet
)
{
    CsHigh();
    LCDDelay(10);
    WrHigh();
    RdHigh();
    CsLow();
    RsHigh();
    LCDDelay(10);  
    lcdData_out();
    LcdData_Wr(u16WriteData);
    WrLow();
    LCDDelay(100);      
    WrHigh();
    LCDDelay(10);      
    CsHigh();
    LCDDelay(10);          
} // DrvVPOST_LCMRegIndexWrite_8Bits

static VOID LCDRegWriteLast
(
	UINT8  u16WriteData		// LCD register contnet
)
{
//    CsHigh();
    LCDDelay(10);
    WrHigh();
    RdHigh();
    CsLow();
    RsHigh();
    LCDDelay(10);  
    lcdData_out();
    LcdData_Wr(u16WriteData);
    WrLow();
    LCDDelay(100);      
    WrHigh();
    LCDDelay(10);      
    CsHigh();
    LCDDelay(10);          
} // DrvVPOST_LCMRegIndexWrite_8Bits


static int LCDRegRead (void)
{
    int data;
    
    CsHigh();
    LCDDelay(10);
    WrHigh();
    RdHigh();
    CsLow();
    RsHigh();
    LCDDelay(10);  
    lcdData_in();
    RdLow();    
    LCDDelay(100);          
    data = LcdData_Rd();
    LCDDelay(10);      
    RdHigh();
    LCDDelay(10);      
    CsHigh();
    LCDDelay(10);          
} // DrvVPOST_LCMRegIndexWrite_8Bits

static VOID __LCDRegIndexWrite
(
	UINT8  u16AddrIndex		// LCD register address
)
{
	volatile UINT32 u32Flag, u32ModeBackup;

	u32ModeBackup = inp32(REG_LCM_MPUCMD);
	u32ModeBackup &= MPUCMD_MPU_SI_SEL;
	
	outp32(REG_LCM_MPUCMD, inp32(REG_LCM_MPUCMD) & ~(MPUCMD_MPU_ON|MPUCMD_MPU_CS|MPUCMD_MPU_RWn) );	// CS=0 	
	outp32(REG_LCM_MPUCMD, inp32(REG_LCM_MPUCMD) | MPUCMD_CMD_DISn );					// turn on Command Mode			

	outp32(REG_LCM_MPUCMD, inp32(REG_LCM_MPUCMD) & ~MPUCMD_WR_RS );					// RS=0 (write address)	
	outp32(REG_LCM_MPUCMD, inp32(REG_LCM_MPUCMD) & ~MPUCMD_MPU_RWn );					// Write Command/Data Selection			
	
	outp32(REG_LCM_MPUCMD, (inp32(REG_LCM_MPUCMD)&(~MPUCMD_MPU_SI_SEL)) | (0x05<<16));	// MPU 16-bit mode select
	
	outp32(REG_LCM_MPUCMD, (inp32(REG_LCM_MPUCMD) & ~MPUCMD_MPU_CMD) | u16AddrIndex);	// WRITE register data
	outp32(REG_LCM_MPUCMD, inp32(REG_LCM_MPUCMD) | MPUCMD_MPU_ON);						// trigger command output
	
	while(inp32(REG_LCM_MPUCMD) & MPUCMD_BUSY);								// wait command to be sent
	outp32(REG_LCM_MPUCMD, inp32(REG_LCM_MPUCMD) & ~MPUCMD_MPU_ON );					// reset command ON flag
	outp32(REG_LCM_MPUCMD, inp32(REG_LCM_MPUCMD) | MPUCMD_MPU_CS |MPUCMD_WR_RS);				// CS=1, RS=1	

	outp32(REG_LCM_MPUCMD, (inp32(REG_LCM_MPUCMD)&(~MPUCMD_MPU_SI_SEL)) | u32ModeBackup);// restore MPU mode 
	
	u32Flag = 1000;
	while( u32Flag--);	//delay for a while on purpose.
	
} // DrvVPOST_LCMRegIndexWrite_8Bits

static VOID __LCDRegWrite
(
	UINT8  u16WriteData		// LCD register data
)
{
	volatile UINT32 u32ModeBackup;

	u32ModeBackup = inp32(REG_LCM_MPUCMD);
	u32ModeBackup &= MPUCMD_MPU_SI_SEL;

	outp32(REG_LCM_MPUCMD, inp32(REG_LCM_MPUCMD) & ~(MPUCMD_MPU_ON|MPUCMD_MPU_CS|MPUCMD_MPU_RWn) );	// CS=0 	
	outp32(REG_LCM_MPUCMD, inp32(REG_LCM_MPUCMD) | MPUCMD_CMD_DISn );					// turn on Command Mode			

	outp32(REG_LCM_MPUCMD, inp32(REG_LCM_MPUCMD) | MPUCMD_WR_RS );						// RS=1	
	outp32(REG_LCM_MPUCMD, inp32(REG_LCM_MPUCMD) & ~MPUCMD_MPU_RWn );					// Write Command/Data Selection			

	outp32(REG_LCM_MPUCMD, (inp32(REG_LCM_MPUCMD)&(~MPUCMD_MPU_SI_SEL)) | (0x05<<16));	// MPU 16-bit mode select	
	
	outp32(REG_LCM_MPUCMD, (inp32(REG_LCM_MPUCMD) & ~MPUCMD_MPU_CMD) | u16WriteData);	// WRITE register data
	outp32(REG_LCM_MPUCMD, inp32(REG_LCM_MPUCMD) | MPUCMD_MPU_ON);						// trigger command output
	
	while(inp32(REG_LCM_MPUCMD) & MPUCMD_BUSY);								// wait command to be sent
	outp32(REG_LCM_MPUCMD, inp32(REG_LCM_MPUCMD) & ~MPUCMD_MPU_ON );					// reset command ON flag
	outp32(REG_LCM_MPUCMD, inp32(REG_LCM_MPUCMD) | MPUCMD_MPU_CS);						// CS=1
	
	outp32(REG_LCM_MPUCMD, (inp32(REG_LCM_MPUCMD)&(~MPUCMD_MPU_SI_SEL)) | u32ModeBackup);// restore MPU mode 	
   	
} // DrvVPOST_LCMRegWrite_8Bits



static INT Clock_Control(void)
{
}


static void ILITEK_ILI9341_MPU_Init(void)  
{  
    int buf;        

        LCDRegIndexWrite(0xEF);
        LCDRegWrite(0x03);//00
        LCDRegWrite(0x80);//83
        LCDRegWrite(0x02);

        LCDRegIndexWrite(0xCF);  
        LCDRegWrite(0x00); 
        LCDRegWrite(0xC1); 
        LCDRegWrite(0x30); 

        LCDRegIndexWrite(0xED);  
        LCDRegWrite(0x64); 
        LCDRegWrite(0x03); 
        LCDRegWrite(0X12); 
        LCDRegWrite(0X81); 

        LCDRegIndexWrite(0xE8);  
        LCDRegWrite(0x85); 
        LCDRegWrite(0x10); 
        LCDRegWrite(0x78); 
         
        LCDRegIndexWrite(0xCB);  
        LCDRegWrite(0x39); 
        LCDRegWrite(0x2C); 
        LCDRegWrite(0x00); 
        LCDRegWrite(0x34); 
        LCDRegWrite(0x02); 
         
        LCDRegIndexWrite(0xF7);  
        LCDRegWrite(0x20); 
         
        LCDRegIndexWrite(0xEA);  
        LCDRegWrite(0x00); 
        LCDRegWrite(0x00); 
         
        LCDRegIndexWrite(0xC0);    //Power control 
        LCDRegWrite(0x21);   //VRH[5:0] 
         
        LCDRegIndexWrite(0xC1);    //Power control 
        LCDRegWrite(0x12);   //SAP[2:0];BT[3:0] 
         
        LCDRegIndexWrite(0xC5);    //VCM control 
        LCDRegWrite(0x32); 
        LCDRegWrite(0x3C); 
         
        LCDRegIndexWrite(0xC7);    //VCM control2 
        LCDRegWrite(0XB4); 
         
        LCDRegIndexWrite(0x36);    // Memory Access Control 
        LCDRegWrite(0x08); 
//        LCDRegWrite(0x28); 
         
        LCDRegIndexWrite(0x3A);   
        LCDRegWrite(0x55);        // 16-bit color
//        LCDRegWrite(0x66);        // 18-bit color

        LCDRegIndexWrite(0xB1);   
        LCDRegWrite(0x00);   
        LCDRegWrite(0x17); 
         
        LCDRegIndexWrite(0xB6);    // Display Function Control 
        LCDRegWrite(0x0A); 
        LCDRegWrite(0xA2); 

        LCDRegIndexWrite(0xF6);    
        LCDRegWrite(0x01); 
        LCDRegWrite(0x30); 
         
        LCDRegIndexWrite(0xF2);    // 3Gamma Function Disable 
        LCDRegWrite(0x00); 
         
        LCDRegIndexWrite(0x26);    //Gamma curve selected 
        LCDRegWrite(0x01); 
         
        LCDRegIndexWrite(0xE0);    //Set Gamma 
        LCDRegWrite(0x0F); 
        LCDRegWrite(0x20); 
        LCDRegWrite(0x1E); 
        LCDRegWrite(0x07); 
        LCDRegWrite(0x0A); 
        LCDRegWrite(0x03); 
        LCDRegWrite(0x52); 
        LCDRegWrite(0X63); 
        LCDRegWrite(0x44); 
        LCDRegWrite(0x08); 
        LCDRegWrite(0x17); 
        LCDRegWrite(0x09); 
        LCDRegWrite(0x19); 
        LCDRegWrite(0x13); 
        LCDRegWrite(0x00); 
         
        LCDRegIndexWrite(0XE1);    //Set Gamma 
        LCDRegWrite(0x00); 
        LCDRegWrite(0x16); 
        LCDRegWrite(0x19); 
        LCDRegWrite(0x02); 
        LCDRegWrite(0x0F); 
        LCDRegWrite(0x03); 
        LCDRegWrite(0x2F); 
        LCDRegWrite(0x13); 
        LCDRegWrite(0x40); 
        LCDRegWrite(0x01); 
        LCDRegWrite(0x08); 
        LCDRegWrite(0x07); 
        LCDRegWrite(0x2E); 
        LCDRegWrite(0x3C); 
        LCDRegWrite(0x0F); 
         
        LCDRegIndexWrite(0x11);    //Exit Sleep 
        delay_loop(120); 
        LCDRegIndexWrite(0x29);    //Display on 

        LCDRegIndexWrite(0X2A);    //Set Gamma 
        LCDRegWrite(0x00); 
        LCDRegWrite(0x00); 
        LCDRegWrite(0x00); 
        LCDRegWrite(0xEF); 

        LCDRegIndexWrite(0X2B);    //Set Gamma 
        LCDRegWrite(0x00); 
        LCDRegWrite(0x00); 
        LCDRegWrite(0x01); 
        LCDRegWrite(0x3F); 

        delay_loop(50);
        LCDRegIndexWrite(0x2c);       //Display on 
        
        
}


INT vpostLCMInit_ILITEK_ILI9341_MPU(PLCDFORMATEX plcdformatex, UINT32 *pFramebuf)
{
	volatile S_DRVVPOST_MPULCM_CTRL sMPU;
//	volatile S_DRVVPOST_MPULCM_TIMING sTiming = {5,5,10,6};
	volatile S_DRVVPOST_MPULCM_TIMING sTiming = {2,2,3,2};
//	volatile S_DRVVPOST_MPULCM_WINDOW sWindow = {240,320};
	volatile S_DRVVPOST_MPULCM_WINDOW sWindow = {320, 240};

	UINT32 nBytesPixel, u32PLLclk, u32ClockDivider;

	// VPOST clock control
	outpw(REG_AHBCLK, inpw(REG_AHBCLK) | VPOST_CKE | HCLK4_CKE);
	outpw(REG_AHBIPRST, inpw(REG_AHBIPRST) | VPOSTRST);
	delay_loop(2);			
	outpw(REG_AHBIPRST, inpw(REG_AHBIPRST) & ~VPOSTRST);	
	delay_loop(2);				
	
	u32PLLclk = GetPLLOutputKhz(eUPLL, 12000);			// CLK_IN = 12 MHz
	u32ClockDivider = u32PLLclk / 40000;
	
    u32ClockDivider--;
	outpw(REG_CLKDIV1, inpw(REG_CLKDIV1) & ~VPOST_N0);						
	outpw(REG_CLKDIV1, (inpw(REG_CLKDIV1) & ~VPOST_N1) | ((u32ClockDivider & 0xFF) << 8));						
	outpw(REG_CLKDIV1, inpw(REG_CLKDIV1) & ~VPOST_S);
	outpw(REG_CLKDIV1, inpw(REG_CLKDIV1) | (3<<3));		// VPOST clock from UPLL		

	outp32(REG_GPGFUN, inp32(REG_GPGFUN)&~MF_GPG9);		    // GPG_9 in GPIO
	outp32(REG_GPGFUN, inp32(REG_GPGFUN)&~MF_GPG10);		// GPG_10 in GPIO	
	outp32(REG_GPIOG_OMD, inp32(REG_GPIOG_OMD)|BIT9|BIT10);	// GPG_9/10 to output
		
	outp32(REG_GPDFUN, inp32(REG_GPDFUN)&~MF_GPD0);		    // GPD_0 in GPIO	
	outp32(REG_GPIOD_OMD, inp32(REG_GPIOD_OMD)|BIT0);	    // GPD_0 to output	

	// reset LCM by GPIOD_12	
	outp32(REG_GPIOG_DOUT, inp32(REG_GPIOG_DOUT) & ~BIT9);	// GPG_9 = LOW
    delay_loop(60);
	outp32(REG_GPIOG_DOUT, inp32(REG_GPIOG_DOUT) | BIT9);	// GPG_9 = HIGH
    delay_loop(1000);

    // enable backlight
	outp32(REG_GPIOG_DOUT, inp32(REG_GPIOG_DOUT) | BIT10);	// GPG_10 = HIGH
	outp32(REG_GPIOD_DOUT, inp32(REG_GPIOD_DOUT) | BIT0);	// GPD_0 = HIGH	

    // initialize LCM
    VPOSTpin_GPIO_init();
	ILITEK_ILI9341_MPU_Init();
	
	// Enable VPOST function pins
//	vpostSetDataBusPin(eDRVVPOST_DATA_16BITS);	
	vpostSetDataBusPin(eDRVVPOST_DATA_8BITS);	
	
	// RS pin to High
	outp32(REG_LCM_MPUCMD, inp32(REG_LCM_MPUCMD) | MPUCMD_MPU_CS |MPUCMD_WR_RS);				// CS=1, RS=1		
		  
	// LCD image source select
	vpostSetLCM_ImageSource(eDRVVPOST_FRAME_BUFFER);
	
	// configure LCD interface
	vpostSetLCM_TypeSelect(eDRVVPOST_MPU);	

	// configure LCD timing sync or async with TV timing	
	vpostsetLCM_TimingType(eDRVVPOST_ASYNC_TV);

    // set MPU timing 
    vpostSetMPULCM_TimingConfig(&sTiming);

    // set MPU LCM window 
	vpostSetMPULCM_ImageWindow(&sWindow);
    
#if 0
	vpostSetFrameBuffer_BaseAddress(pFramebuf);
#else    
    // set frambuffer base address
    if(pFramebuf != NULL) {
		vpostAllocVABufferFromAP(pFramebuf);
	} else {
    	if( vpostAllocVABuffer(plcdformatex, nBytesPixel)==FALSE)
    		return ERR_NULL_BUF;
    }
#endif    
	
	// set frame buffer data format
	vpostSetFrameBuffer_DataType(plcdformatex->ucVASrcFormat);
	
	vpostSetYUVEndianSelect(eDRVVPOST_YUV_LITTLE_ENDIAN);

	// set MPU bus mode
//	vpostSetMPULCM_BusModeSelect(eDRVVPOST_MPU_8_8_8);
	vpostSetMPULCM_BusModeSelect(eDRVVPOST_MPU_8_8);
	
	// enable MPU LCD controller
	vpostVAStartTrigger_MPUContinue();
	
//	BacklightControl(TRUE);			
	return 0;
}

INT32 vpostLCMDeinit_ILITEK_ILI9341_MPU(VOID)
{
	vpostVAStopTrigger();
	vpostFreeVABuffer();
	outpw(REG_AHBCLK, inpw(REG_AHBCLK) & ~VPOST_CKE);	
	return 0;
}
#endif    //HAVE_ILITEK_ILI9341_MPU