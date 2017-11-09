/***************************************************************************
 *                                                                         *
 * Copyright (c) 2007 - 2009 Nuvoton Technology Corp. All rights reserved.*
 *                                                                         *
 ***************************************************************************/
 
/****************************************************************************
 * 
 * FILENAME
 *     W55FA95_VPOST_SPLC780D.c
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
//#include "NUC930_VPOST_Regs.h"
//#include "w55fa93_vpost.h"
#include "w55fa95_vpost.h"
#include "w55fa95_i2c.h"

extern void LCDDelay(unsigned int nCount);

#if defined(HAVE_SPLC780D)

static void delay_5mS(UINT32 u32TimeCnt)
{
	volatile UINT32 ii, jj;
	
	u32TimeCnt *= 375*5;		// for 300MHz CPU clock speed
	for (ii=0; ii<u32TimeCnt; ii++)
		for (jj=0; jj<100; jj++)
			jj++;
}

static void delay_10uS(UINT32 u32TimeCnt)
{
	volatile UINT32 ii, jj;
	
	u32TimeCnt *= 4000;		// for 300MHz CPU clock speed
	for (ii=0; ii<u32TimeCnt; ii++)
		jj++;
}

#define LCD_RS_HI()		outpw(REG_GPIOD_DOUT, inpw(REG_GPIOD_DOUT) |  0x800)	
#define LCD_RS_LO()		outpw(REG_GPIOD_DOUT, inpw(REG_GPIOD_DOUT) & ~0x800)	
#define LCD_ENA_HI()	outpw(REG_GPIOD_DOUT, inpw(REG_GPIOD_DOUT) |  0x400)	
#define LCD_ENA_LO()	outpw(REG_GPIOD_DOUT, inpw(REG_GPIOD_DOUT) & ~0x400)	
#define LCD_RD()		outpw(REG_GPIOD_DOUT, inpw(REG_GPIOD_DOUT) |  0x200)	
#define LCD_WR()		outpw(REG_GPIOD_DOUT, inpw(REG_GPIOD_DOUT) & ~0x200)	

void SPLC780D_LCD_4bit_WR(char LCDdata)
{
	LCD_WR();	
	LCD_ENA_LO();	
	delay_10uS(1);	
	LCD_ENA_HI();		
	LCDdata &= 0x0f;
	LCDdata <<= 4;
	outpw(REG_GPIOC_DOUT, (inpw(REG_GPIOC_DOUT)& ~0xF0) | LCDdata);
	delay_10uS(1);
	LCD_ENA_LO();
	delay_10uS(1);		
}

void SPLC780D_DDRAM_Addr(char index)
{
	char buf = index;
	
	LCD_RS_LO();
	index |= 0x80;	
	SPLC780D_LCD_4bit_WR((index >> 4));	
	SPLC780D_LCD_4bit_WR(buf & 0x0F);	
}

void SPLC780D_DDRAM_data(char data)
{
	LCD_RS_HI();
	SPLC780D_LCD_4bit_WR((data >> 4));	
	SPLC780D_LCD_4bit_WR(data & 0x0F);	
}

void SPLC780D_Display_Line(char lineNo, char startAddr, char lenth, char* ptr)
{
	int ii;
	
	startAddr &= 0x07;		// maximum starting address 0 ~ 7
	switch(lineNo)
	{
		case 0: 
		default:
			SPLC780D_DDRAM_Addr(startAddr);			// line-0
			for(ii=0; ii<lenth; ii++)
				SPLC780D_DDRAM_data(*(ptr+ii));
			break;
		
		case 1: 
			SPLC780D_DDRAM_Addr(startAddr+0x40);	// line-1
			for(ii=0; ii<lenth; ii++)
				SPLC780D_DDRAM_data(*(ptr+ii));
			break;
	}
}

void SPLC780D_Init(void)
{
	LCD_ENA_LO();
	LCD_WR();	
	LCD_RS_LO();
	
	delay_5mS(50);
	SPLC780D_LCD_4bit_WR(0x03);	// function set

	delay_5mS(10);
	SPLC780D_LCD_4bit_WR(0x03);	// function set
	
	delay_5mS(5);
	SPLC780D_LCD_4bit_WR(0x03);	// function set	
	delay_5mS(1);	
	
	// set 4-bit mode
	SPLC780D_LCD_4bit_WR(0x02);	// 4-bit mode	
		
	// set 4-bit mode and 2-line 5x8 display
	SPLC780D_LCD_4bit_WR(0x02);	// 4-bit mode
	SPLC780D_LCD_4bit_WR(0x08);	// 2-line, 5x8 character
	
	// clear display, AC = 0
	SPLC780D_LCD_4bit_WR(0x00);	
	SPLC780D_LCD_4bit_WR(0x01);	

	delay_5mS(1);	
	
	// display shift (increment) after write operation
	SPLC780D_LCD_4bit_WR(0x00);	
	SPLC780D_LCD_4bit_WR(0x06);	// incremental (bit-1), shift (bit-0)

	// display off
	SPLC780D_LCD_4bit_WR(0x00);	
	SPLC780D_LCD_4bit_WR(0x08);	// display off (bit-2), cursor off (bit-1), blink off (bit-0)

	// display on
	SPLC780D_LCD_4bit_WR(0x00);	
	SPLC780D_LCD_4bit_WR(0x0F);	// display on (bit-2), cursor on (bit-1), blink on (bit-0)
}

INT vpostLCMInit_SPLC780D(PLCDFORMATEX plcdformatex, UINT32 *pFramebuf)
{

	// Enable VPOST function pins
	// DB[7:4]
	outpw(REG_GPCFUN, inpw(REG_GPCFUN) & ~0xFF00);			// GPC[7:4] to GPIO mode
	outpw(REG_GPIOC_OMD, inpw(REG_GPIOC_OMD) | 0xF0);		// GPC[7:4] in output mode
	outpw(REG_GPIOC_PUEN, inpw(REG_GPIOC_PUEN) | 0xF0);		// GPC[7:4] pull-up resistor enabled
	
	// RS, R/W_, ENA (RS:GPD_11, ENA:GPD_10, R/W:GPD_9)
	outpw(REG_GPDFUN, inpw(REG_GPDFUN) & ~0xFC0000);		// GPD[11:9] to GPIO mode
	outpw(REG_GPIOD_OMD, inpw(REG_GPIOD_OMD) | 0xE00);		// GPD[11:9] in output mode
	outpw(REG_GPIOD_PUEN, inpw(REG_GPIOD_PUEN) | 0xE00);	// GPD[11:9] pull-up resistor enabled

	outpw(REG_GPIOD_DOUT, inpw(REG_GPIOD_DOUT) & ~0x400);	// ENA = low --> disabled
	
	// DIR pin (for level-shift control)
	outpw(REG_GPBFUN, inpw(REG_GPBFUN) & ~0xC0000000);		// GPB[15] to GPIO mode
	outpw(REG_GPIOB_OMD, inpw(REG_GPIOB_OMD) | 0x8000);		// GPB[15] in output mode
	outpw(REG_GPIOB_DOUT, inpw(REG_GPIOB_DOUT) | 0x8000);	// GPB[15] to High
	
	delay_5mS(50);
	SPLC780D_Init();
	
	// display "Hello !!" "Nuvoton "
	SPLC780D_Display_Line(0, 0, 8, "Hello !!");
	SPLC780D_Display_Line(1, 0, 8, "Nuvoton ");		

//	while(1);		
	return 0;
}

INT32 vpostLCMDeinit_SPLC780D(VOID)
{
	return 0;
}
#endif    //HAVE_SPLC780D