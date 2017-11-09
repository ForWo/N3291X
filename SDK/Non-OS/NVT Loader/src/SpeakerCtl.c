#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wblib.h"
#include "nvtloader.h"
#include "w55fa95_gpio.h"
#include "w55fa95_kpi.h"
#include "w55fa95_reg.h"


/*	
	Speaker enable
	LGE: GPD4 =1  enable 
		GPD4 =0  disable
*/
void SpeakerInit(void)
{
#ifdef __KLE_DEMO__
	gpio_configure(GPIO_PORTD, 
					4);		// pin number	
	gpio_setportdir(GPIO_PORTD, 
					(1<<4), 	// Mask 
					(1<<4));	// 1 output. 0 input.
#endif					
}
void SpeakerEnable(void)
{
#ifdef __KLE_DEMO__
	gpio_setportval(GPIO_PORTD, 
					(1<<4), 	//Mask
					(1<<4));	//High
#endif					
}
void SpeakerDisable(void)
{
#ifdef __KLE_DEMO__
	gpio_setportval(GPIO_PORTD, 
					(1<<4), 	//Mask
					(0<<4));	//Low
#endif					
}
/*
	Earphone detect : 
	LGE: AUD_DET : GPG7 = 0. plug out
				  GPG7 = 1. plug in

*/
void EarphoneInit(void)
{
#ifdef __KLE_DEMO__
	gpio_configure(GPIO_PORTG, 
					7);	// pin number	
	gpio_setportdir(GPIO_PORTG, 
					(1<<7), 	// Mask 
					(0<<7));	// 1 output. 0 input.
#endif					
}
BOOL EarphoneDetect(void)
{
#ifdef __KLE_DEMO__
	unsigned short val;	
	gpio_readport(GPIO_PORTG,
					&val);
	if(val&(1<<7))
		return TRUE;	//Plun in
	else
		return FALSE;	//Plug out
#else	
	return FALSE;
#endif							
}
/*
	Mute Control : Control earphone and speaker to output sound
	LGE: MUTE : GPE0 = 1 (Default high)
			             = 0 (Enable Dac output)
			    	
	Usually after enable SPU output 1 sec, force MUTE to low to enable dac on.		    	
*/
void MuteInit(void)
{
#if 0
	gpio_configure(GPIO_PORTE, 
					0);	// pin number	
	gpio_setportdir(GPIO_PORTE, 
					(1<<0), 	// Mask 
					(1<<0));	// 1 output. 0 input.	
#endif								
}

void MuteEnable(void)
{
#if 0
	gpio_setportval(GPIO_PORTE, 
					(1<<0), 	//Mask
					(1<<0));	//High	
#endif							
}
void MuteDisable(void)
{
#if 0
	gpio_setportval(GPIO_PORTE, 
					(1<<0), 	//Mask
					(0<<0));	//Low
#endif					
}


