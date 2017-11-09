#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wblib.h"
#include "nvtloader.h"
#include "w55fa95_gpio.h"
#include "w55fa95_kpi.h"
#include "w55fa95_reg.h"
/* 
	Mute Control : 
	LGE: LCM power : GPA5   = 1 (Enable LCM power)
			             		= 0 (Disable LCM power)
*/
void LcmPowerInit(void)
{
#ifdef __KLE_DEMO__
	gpio_configure(GPIO_PORTA, 
					//(1<<5));	// pin number	
						5);	// pin number	
	gpio_setportdir(GPIO_PORTA, 
					(1<<5), 	// Mask 
					(1<<5));	// 1 output. 0 input.				
#endif					
}
void LcmPowerEnable(void)
{
#ifdef __KLE_DEMO__
	gpio_setportval(GPIO_PORTA, 
					(1<<5), 	//Mask
					(1<<5));	//High
#endif								
}
void LcmPowerDisable(void)
{
#ifdef __KLE_DEMO__
	gpio_setportval(GPIO_PORTA, 
					(1<<5), 	//Mask
					(0<<5));	//Low
#endif					
}
/*
	Backlight Control : 
	LGE: LCM power : GPG5   = 1 (Enable LCM backlight)
			             		= 0 (Disable LCM backlight)
*/
void LcmBacklightInit(void)
{
#ifdef __KLE_DEMO__
	gpio_configure(GPIO_PORTG, 
					//(1<<5));	// pin number	
						5);	// pin number	
	gpio_setportdir(GPIO_PORTG, 
					(1<<5), 	// Mask 
					(1<<5));	// 1 output. 0 input.				
#endif		
#ifdef __CWC_DEMO__
	gpio_configure(GPIO_PORTD, 		/* GPD1 */
					//(1<<5));	// pin number	
						1);	// pin number	
	gpio_setportdir(GPIO_PORTD, 
					(1<<1), 	// Mask 
					(1<<1));	// 1 output. 0 input.	
#endif	
#if defined(__NVT_MP_WVGA_DEMO__) || defined(__NVT_MP_WVGA_IN_DEMO__) || defined(__SHBST_DEMO__)
	#ifdef __ICE__
	
	#else
	gpio_configure(GPIO_PORTD, 		/* GPD0 */
					//(1<<5));	// pin number	
						0);	// pin number	
	gpio_setportdir(GPIO_PORTD, 
					(1<<0), 	// Mask 
					(1<<0));	// 1 output. 0 input.	
	#endif		
#endif

#ifdef __PICO_PROJECTOR__
	gpio_configure(GPIO_PORTD, 		/* GPD0 */
						0);						
	gpio_setportval(GPIO_PORTD, 		/* GPD0 to high */
					(1<<0), 
					(1<<0));																
	gpio_setportdir(GPIO_PORTD, 		/* GPD0 output high */
					(1<<0), 		
					(1<<0));		// 1 output. 0 input.	
					
	gpio_configure(GPIO_PORTD, 		/* GPD1 */
						1);
	gpio_setportval(GPIO_PORTD, 		/* GPD1 to high */
					(1<<1), 
					(1<<1));								
	gpio_setportdir(GPIO_PORTD, 
					(1<<1), 		// Mask 
					(1<<1));		// 1 output. 0 input.		
					
	gpio_configure(GPIO_PORTD, 		/* GPD2 */
						2);
	gpio_setportval(GPIO_PORTD, 		/* GPD2 to low */
					(1<<2), 
					(0<<2));								
	gpio_setportdir(GPIO_PORTD, 
					(1<<2), 		// Mask 
					(1<<2));		// 1 output. 0 input.
	sysprintf( "### 0x%x, 0x%x, 0x%x, 0x%x ###\n", inp32(0xb8001030), inp32(0xb8001034), inp32(0xb8001038), inp32(0xb800103C) );
#endif
#ifdef __GRIMM__ 
	gpio_configure(GPIO_PORTD, 		/* GPD0 */
						0);						
	gpio_setportval(GPIO_PORTD, 		/* GPD0 to low */
					(1<<0), 
					(0<<0));																
	gpio_setportdir(GPIO_PORTD, 		/* GPD0 output high */
					(1<<0), 		
					(1<<0));		// 1 output. 0 input.	
					
	gpio_configure(GPIO_PORTD, 		/* GPD1 */
						1);
	gpio_setportval(GPIO_PORTD, 		/* GPD1 to high */
					(1<<1), 
					(1<<1));								
	gpio_setportdir(GPIO_PORTD, 
					(1<<1), 		// Mask 
					(1<<1));		// 1 output. 0 input.		
#endif
}
void LcmBacklightEnable(void)
{
#ifdef __KLE_DEMO__
	gpio_setportval(GPIO_PORTG, 
					(1<<5), 	//Mask
					(1<<5));	//High			
#endif					
#ifdef __CWC_DEMO__
	gpio_setportval(GPIO_PORTD, 
					(1<<1), 	//Mask
					(1<<1));	//High	
#endif		
#if defined(__NVT_MP_WVGA_DEMO__) || defined(__NVT_MP_WVGA_IN_DEMO__) || defined(__SHBST_DEMO__)
	#ifdef __ICE__
	
	#else
	gpio_setportval(GPIO_PORTD, 			/* GPD0 */ 
					(1<<0), 	//Mask	
					(1<<0));	//High				
	#endif 					
#endif
#ifdef __PICO_PROJECTOR__
	gpio_setportval(GPIO_PORTD, 		/* GPD1 to low */
					(1<<1), 
					(0<<1));	

	gpio_setportval(GPIO_PORTD, 		/* GPD0 to low */
					(1<<0), 
					(0<<0));		
#endif
}
void LcmBacklightDisable(void)
{
#ifdef __KLE_DEMO__
	gpio_setportval(GPIO_PORTG, 
					(1<<5), 	//Mask
					(0<<5));	//Low
#endif	
#ifdef __CWC_DEMO__
	gpio_setportval(GPIO_PORTD, 
					(1<<1), 	//Mask
					(0<<1));	//Low
#endif					
}


void LcmSaturationDelay(void)
{
#ifdef __KLE_DEMO__
	UINT32 u32Delay;	
	for (u32Delay=0;u32Delay<=10000;u32Delay++);
#endif	
}

void LcmSaturationInc(UINT32 u32Value)
{
#ifdef __KLE_DEMO__
	UINT32 u32Counter;	
	gpio_setportval(GPIO_PORTA, 0x40, 0x40);
	gpio_setportval(GPIO_PORTE, 0x2, 0x2);
	LcmSaturationDelay();	
			
	gpio_setportval(GPIO_PORTE, 0x2, 0x0);		//CS=L
	LcmSaturationDelay();
	
	for (u32Counter=1;u32Counter<=u32Value;u32Counter++)
	{		
		gpio_setportval(GPIO_PORTA, 0x40, 0x0);	//U/D=L	
		LcmSaturationDelay();
		
		gpio_setportval(GPIO_PORTA, 0x40, 0x40);	//U/D=H	
		LcmSaturationDelay();
	}	
	gpio_setportval(GPIO_PORTE, 0x2, 0x2);		//CS=H
#endif
}

void LcmSaturationDec(UINT32 u32Value)
{
#ifdef __KLE_DEMO__
	UINT32 u32Counter;
	gpio_setportval(GPIO_PORTA, 0x40, 0x0);
	gpio_setportval(GPIO_PORTE, 0x2, 0x2);
	LcmSaturationDelay();
			
	gpio_setportval(GPIO_PORTE, 0x2, 0x0);		//CS=L
	LcmSaturationDelay();
	
	for (u32Counter=1;u32Counter<=u32Value;u32Counter++)
	{
		gpio_setportval(GPIO_PORTA, 0x40, 0x0);	//U/D=L	
		LcmSaturationDelay();
		
		gpio_setportval(GPIO_PORTA, 0x40, 0x40);	//U/D=H	
		LcmSaturationDelay();
	}

	gpio_setportval(GPIO_PORTA, 0x40, 0x0);	//U/D=L	
	LcmSaturationDelay();
	gpio_setportval(GPIO_PORTE, 0x2, 0x2);		//CS=H
#endif	
}

void LcmSaturationInit(void)
{
#ifdef __KLE_DEMO__
	gpio_configure(GPIO_PORTA, 6);	//U/D
	gpio_configure(GPIO_PORTE, 1);	//CS

	gpio_setportval(GPIO_PORTA, 0x40, 0x40);
	gpio_setportval(GPIO_PORTE, 0x2, 0x2);

	gpio_setportdir(GPIO_PORTA, 0x40, 0x40);
	gpio_setportdir(GPIO_PORTE, 0x2, 0x2);

	LcmSaturationDec(64);
#endif	
}