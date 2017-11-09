#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wblib.h"
#include "W55FA95_adc.h"
#include "w55fa95_gpio.h"

void I2cInit(void)
{
	gpio_setportval(GPIO_PORTB, 		
					(1<<13), 		//Mask	
					(1<<13));	//High	
	gpio_configure(GPIO_PORTB, 		/* GPB13=SCK */ 
						13);								
	gpio_setportdir(GPIO_PORTB, 
					(1<<13), 		// Mask 
					(1<<13));	// 1 output. 0 input.	
					
	gpio_setportval(GPIO_PORTB, 		/* GPB14=SDA */ 
					(1<<14), 		//Mask	
					(1<<14));	//High					
	gpio_configure(GPIO_PORTB, 
						14);		// pin number	
	gpio_setportdir(GPIO_PORTB, 
					(1<<14), 		// Mask 
					(1<<14));	// 1 output. 0 input.														
}
void I2cSendStart(void)
{
	volatile UINT32 u32Delay=0x1000;
	gpio_setportval(GPIO_PORTB, 		/* SDA =LOW*/ 
					(1<<14), 		//Mask	
					(0<<14));	//High	
	while(u32Delay--);
	gpio_setportval(GPIO_PORTB, 		/* SCK =LOW*/ 
					(1<<13), 		//Mask
					(0<<13));	//High						
}
void I2cSendStop(void)
{
	volatile UINT32 u32Delay=0x1000;
	while(u32Delay--);
	gpio_setportval(GPIO_PORTB, 		/* SCK =HIGH*/ 
					(1<<13), 		//Mask
					(1<<13));	//High	
	u32Delay=0x1000;
	while(u32Delay--);
	gpio_setportval(GPIO_PORTB, 		/* SDA =High*/ 
					(1<<14), 		//Mask	
					(1<<14));	//High	
}
