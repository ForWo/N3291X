/****************************************************************************
*                                                                           *
* Copyright (c) 2009 Nuvoton Tech. Corp. All rights reserved.               *
*                                                                           *
*****************************************************************************/

/****************************************************************************
* FILENAME
*   NVT_BatteryDect.c
*
* VERSION
*   1.0
*
* DESCRIPTION
*   Battery detection
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wblib.h"
#include "W55FA95_adc.h"

#ifdef __KLE_ECO__
/*-----------------------------------------------------------------------------
 * For SUPPORT_RTC_ON_OFF_ISSUE to fix RTC on/off fail issue.
 *---------------------------------------------------------------------------*/
    #define SUPPORT_RTC_ON_OFF_ISSUE
#endif

extern VOID RTC_Check(VOID);

#define LOW_BATTERY_LEVEL           (3.65)  // 2012/9/23, DV2-2 want 3.65v for low battery level without USB cable
#define LOW_BATTERY_LEVEL_WITH_USB  (3.65)  // 2012/9/23, DV2-2 want 3.65v for low battery level with USB cable


void ForceShutdown(void)
{
#ifdef SUPPORT_RTC_ON_OFF_ISSUE
    // use GPE0 to power off core power
    outpw(REG_GPIOE_DOUT, inpw(REG_GPIOE_DOUT) & (~0x0001));    // output 0 to GPE0 to power off core power (0810)
#endif

    outp32(REG_APBCLK, inp32(REG_APBCLK) | RTC_CKE);    // enable RTC clock to access RTC registers

    // Cowork with NVTLoader to judge that system boot up from RESET button or POWER button.
    //      RTC register TTR == 0 means system power off/on by POWER button.
    //      RTC register TTR != 0 means system power off/on by RESET button.
    //      NVTLoader will fill 0x7 to TTR when system boot up.
    //      Anyone who want to power off system MUST fill 0x0 before shutdown.
    //      So, if user power off/on system by RESET button, the TTR will keep value 0x7 and NVTLoader will check it.
    sysprintf("Set RTC_TTR to 0.\n");
    outp32(TTR, 0x00);
    RTC_Check();

    // shutdown by RTC
    outp32(PWRON, (inp32(PWRON) & ~RTC_PWRON) | SW_PCLR);
    RTC_Check();
    outp32(REG_AHBCLK, 0);
    while(1);	/* Forever loop */
}


/*
	There are 2 resistors form voltage divider in demo board. One is 100K Ohm, anothe is 200K Ohm
	The ratio is 1/3.
*/
void BatteryDetection(BOOL bIsExtraPower)
{
	float voltage, step, tmp;
	UINT16 u16Vol;
	step = 3.13/1024.;  // DV2-2 board changed reference votage from V3.3 to V3.13. Still 10bits ADC.


	if(adc_normalread(2, &u16Vol)==Successful)
	{
		UINT32 u32Dec, u32Fraction=3;
		tmp = step*u16Vol*3;				/* Ratio is 1/3 */
		voltage = tmp;
//        sysprintf("Battery value = %x\n", u16Vol);
		sysprintf("Battery Voltage = ");
		u32Dec = tmp;
		sysprintf("%d.",u32Dec);
		tmp = tmp - u32Dec;
		while((tmp!=0.) && (u32Fraction!=0))
		{
			tmp = tmp*10.;
			u32Dec = tmp;
			sysprintf("%d",u32Dec);
			tmp = tmp - u32Dec;
			u32Fraction = u32Fraction-1;
		}
		sysprintf("\n");

        // 2012/9/12 by CJChen1, change shutdown policy by customer's request.
        //      Always shutdown system if battery voltage is low. The voltage threshold is depend on with or without USB cable.
		if(bIsExtraPower==FALSE)
		{   // Without extra power form USB or power cable
			if(voltage < LOW_BATTERY_LEVEL)
			{
				sysprintf("Battery voltage too low...., please charge the battery.\n");
				sysprintf("Force shutdown !\n");
				ForceShutdown();
			}
		}
		else
		{   // With extra power form USB or power cable
			if(voltage < LOW_BATTERY_LEVEL_WITH_USB)
			{
				sysprintf("Battery voltage too low...., please wait the battery charging completed.\n");
				sysprintf("Force shutdown !\n");
				ForceShutdown();
			}
		}
	}
}
