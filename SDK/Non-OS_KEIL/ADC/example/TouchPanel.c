#include <stdio.h>
#include "wblib.h"
#include "W55FA95_adc.h"


void TouchPanel_Powerdown_Wakeup(void)
{
        UINT16 x, y;           
	adc_read(ADC_NONBLOCK, &x, &y);					
	sysPowerDown(WE_ADC);		
}	
