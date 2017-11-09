#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wblib.h"
#include "nvtloader.h"
#include "w55fa95_gpio.h"
#include "w55fa95_kpi.h"
#include "w55fa95_reg.h"

//#define __DV1__
void AdaptorDetInit(void)
{
#ifdef __KLE_DEMO__
	/*
	Adaptor Det GPG3 
	Set high to prevent from big current as power on
	*/
	gpio_configure(GPIO_PORTG, 
						3);	// pin number	
	gpio_setportdir(GPIO_PORTG, 
					(1<<3), 	// Mask 
					(1<<3));	// 1 output. 0 input.				
#endif					
}
void AdaptorDetHigh(void)
{
#ifdef __KLE_DEMO__
	gpio_setportval(GPIO_PORTG, 
					(1<<3), 	//Mask
					(1<<3));	//High			
#endif	
}
void AdaptorDetLow(void)
{
#ifdef __KLE_DEMO__
	gpio_setportval(GPIO_PORTG, 
					(1<<3), 	//Mask
					(0<<3));	//High			
#endif	
}

void ChargerIndicatotInit(void)
{
#ifdef __KLE_DEMO__
	#ifdef __DV1__
	gpio_configure(GPIO_PORTH, 
						5);	// pin number	
	gpio_setportdir(GPIO_PORTH, 
					(1<<5), 	// Mask 
					(1<<5));	// 1 output. 0 input.	
	gpio_setportval(GPIO_PORTH, 						/* MC: Has better clear pin to low. */
					(1<<5), 	//Mask
					(0<<5));	//Low						
	#else
	/*
	RainBow:	
		Charge indicator 
		GPH5 = 0 means battery under charging  
		GPH5 = 1 means battery be charged full or
		In NVT Loader only set the GPH5 as input mode. It is enough. ==>7/16 change to need detect the status, 
		
	*/	
	gpio_configure(GPIO_PORTH, 
						5);	// pin number	
	gpio_setportdir(GPIO_PORTH, 
					(1<<5), 	// Mask 
					(0<<5));	// 1 output. 0 input.				
	#endif				
#endif				
} 
/*

	Only in DV2 
	
	= 0: In charging 

	= 1: Charge done or Noncharge state ==>need to do batttery detect
*/
BOOL  ChargerDetect(void)
{
#ifdef __KLE_DEMO__
	unsigned short val;	
	gpio_readport(GPIO_PORTH,
					&val);
	if(val&(1<<5))
		return TRUE;	//Charge done or non-charger
	else
		return FALSE;	//In charging
#else	
	return FALSE;
#endif				
}