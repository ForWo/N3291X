/****************************************************************************
*                                                                           *
* Copyright (c) 2009 Nuvoton Tech. Corp. All rights reserved.               *
*                                                                           *
*****************************************************************************/

/****************************************************************************
* FILENAME
*   libkpi.c
*
* VERSION
*   1.0
*
* DESCRIPTION
*   KPI library source file
*
* DATA STRUCTURES
*   None
*
* FUNCTIONS
*
* HISTORY
*
* REMARK
*   None
****************************************************************************/
#include "w55fa95_gpio.h"
#include "w55fa95_kpi.h"

// latest key pressed recorded by ISR, might _not_ be the most updated status
static unsigned int _key;
// interrupt number for kpi
static unsigned char _int;
static unsigned char _opened = 0;
static BOOL bGetKey = TRUE;


static unsigned int readkey(void)
{
	unsigned int read0, read1;
	gpio_readport(GPIO_PORTA, (unsigned short *)&read0);
	read0 = (read0 & 0x78) >> 3;
	gpio_readport(GPIO_PORTB, (unsigned short *)&read1);
	read1 = (read1 & 0xC00) >> 6;
	read0 |= read1;	
	if(read0 == 0x3F)
		return(0);

	return (~read0 & 0x3F);	
}

static void kpi_isr(void)
{	
	_key = readkey();
	//(*(unsigned int volatile *)REG_AIC_SCCR) = (1 << (_int + 2));
	gpio_cleartriggersrc(GPIO_PORTA);
	gpio_cleartriggersrc(GPIO_PORTB);
	
	return;

}

void kpi_init(void)
{

	_opened = 1;	

	// PORTA[3,4,5,6]
	gpio_setportdir(GPIO_PORTA, ((1 << 3) | (1 << 4) | (1 << 5) | (1 << 6)), 0);
	gpio_setportpull(GPIO_PORTA, ((1 << 3) | (1 << 4) | (1 << 5) | (1 << 6)), ((1 << 3) | (1 << 4) | (1 << 5) | (1 << 6)));	
	gpio_setintmode(GPIO_PORTA, ((1 << 3) | (1 << 4) | (1 << 5) | (1 << 6)), 
					((1 << 3) | (1 << 4) | (1 << 5) | (1 << 6)), ((1 << 3) | (1 << 4) | (1 << 5) | (1 << 6)));

	//PORTB[10,11]	
	gpio_setportdir(GPIO_PORTB, ((1 << 10) | (1 << 11)), 0);
	gpio_setportpull(GPIO_PORTB, ((1 << 10) | (1 << 11)), 0);	
	gpio_setintmode(GPIO_PORTB, ((1 << 10) | (1 << 11)), 
					((1 << 10) | (1 << 11)), ((1 << 10) | (1 << 11)));

    return;    
}

int kpi_open(unsigned int src)
{
	if(_opened == 0)
		kpi_init();
	if(_opened != 1)
		return(-1);
	
	_opened = 2;
	if(src > 3)
		return(-1);
		
	_int = src;	

	gpio_setsrcgrp(GPIO_PORTA, ((1 << 3) | (1 << 4) | (1 << 5) | (1 << 6)), src);
	gpio_setsrcgrp(GPIO_PORTB, ((1 << 10) | (1 << 11)), src);

	gpio_setdebounce(128, 1 << src);
	gpio_setlatchtrigger(1 << src);

	sysInstallISR(IRQ_LEVEL_7, src + 2, (PVOID)kpi_isr);	
		
	sysSetInterruptType(src + 2, HIGH_LEVEL_SENSITIVE);
	sysSetLocalInterrupt(ENABLE_IRQ);	

    return(0);    
}

void kpi_close(void)
{
	if(_opened != 2)
		return;
	_opened = 1;
	sysDisableInterrupt(_int + 2);  
	return;
}


int kpi_read(unsigned char mode)
{
	// add this var in case key released right before return.
	int volatile k = 0;
	
	if(_opened != 2)
		return(-1);
	
	if(mode != KPI_NONBLOCK && mode != KPI_BLOCK) {
		//sysDisableInterrupt(_int + 2);  
		return(-1);
	}
	sysEnableInterrupt(_int + 2);

	if(bGetKey == TRUE)
	{
		bGetKey = FALSE;
		_key = readkey();
	}
	
	if(_key == 0) {
		// not pressed, non blocking, return immediately
		if(mode == KPI_NONBLOCK) {
			sysDisableInterrupt(_int + 2);  
			return(0);
		}
		// not pressed, non blocking, wait for key pressed
#pragma O0
// ARMCC is tooooo smart to compile this line correctly, so ether set O0 or use pulling....
		while((k = _key) == 0);
	} else {
		// read latest key(s) and return.
		sysDisableInterrupt(_int + 2);
		do {		
			k = readkey();		
		} while(k == 0 && mode != KPI_NONBLOCK);
	}
	return(k);
}


