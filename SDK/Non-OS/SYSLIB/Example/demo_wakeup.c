/***************************************************************************
 *                                                                         									     *
 * Copyright (c) 2008 Nuvoton Technolog. All rights reserved.              					     *
 *                                                                         									     *
 ***************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include "wblib.h"
#include "demo.h"
#include "w55fa95_gpio.h"


#define BUF_SIZE		(16*1024)

__align(32) UINT8 u32Array[BUF_SIZE];

/* For SPU disable DAC off */
static void delay(UINT32 kk)
{
	volatile UINT32 ii, jj;
	
	for(ii=0; ii < kk; ii++)
	{
		for(jj=0; jj < 0x10; jj++);	
	}
}


UINT8
DrvSPU_ReadDACReg (
	UINT8 DACRegIndex
)
{
	UINT32 u32Reg = 0x30800000;		// clock divider = 0x30, ID = 0x80
	UINT8 u8Ret;

#if 0
	u32Reg |= DACRegIndex << 8;
	while(inp32(REG_SPU_DAC_CTRL) & V_I2C_BUSY);
	outp32(REG_SPU_DAC_CTRL, u32Reg);
	delay(10);
	while(inp32(REG_SPU_DAC_CTRL) & V_I2C_BUSY);
	outp32(REG_GPIOA_DOUT, inp32(REG_GPIOA_DOUT) & ~BIT4);			
	delay(50);
	outp32(REG_GPIOA_DOUT, inp32(REG_GPIOA_DOUT) | BIT4);					
	u8Ret = inp32(REG_SPU_DAC_CTRL) & 0xFF;
#else	
	u32Reg |= DACRegIndex << 8;
	while(inp32(REG_SPU_DAC_CTRL) & V_I2C_BUSY);
	outp32(REG_SPU_DAC_CTRL, u32Reg);
	delay(20);
	while(inp32(REG_SPU_DAC_CTRL) & V_I2C_BUSY);
	delay(200);
	u8Ret = inp32(REG_SPU_DAC_CTRL) & 0xFF;
#endif	
	
	return u8Ret;
}
VOID DrvSPU_WriteDACReg (
	UINT8 DACRegIndex, 
	UINT8 DACRegData
)
{
	UINT32 u32Reg = 0x30810000;		// clock divider = 0x30, ID = 0x80

#if 0
	u32Reg |= DACRegIndex << 8;
	u32Reg |= DACRegData;
	while(inp32(REG_SPU_DAC_CTRL) & V_I2C_BUSY);
	outp32(REG_SPU_DAC_CTRL, u32Reg);	
	outp32(REG_GPIOA_DOUT, inp32(REG_GPIOA_DOUT) | BIT4);		
	delay(10);
	while(inp32(REG_SPU_DAC_CTRL) & V_I2C_BUSY);	
	outp32(REG_GPIOA_DOUT, inp32(REG_GPIOA_DOUT) & ~BIT4);			
	delay(100);	
	outp32(REG_GPIOA_DOUT, inp32(REG_GPIOA_DOUT) | BIT4);				
#else
	u32Reg |= DACRegIndex << 8;
	u32Reg |= DACRegData;
	while(inp32(REG_SPU_DAC_CTRL) & V_I2C_BUSY);
	outp32(REG_SPU_DAC_CTRL, u32Reg);	
	delay(20);
	while(inp32(REG_SPU_DAC_CTRL) & V_I2C_BUSY);	
	delay(200);	
#endif	
}



/*--------------------------------------------------------------------------------------------------------*
 *                                                                                                        					     *
 * 																		     *
 *	Wake up source														     *
 * 	KPI_WE, ADC_WE, UHC_WE, UDC_WE, UART_WE, SDH_WE, RTC_WE, GPIO_WE		     *	
 *	2. Default priority	  													     *
 *---------------------------------------------------------------------------------------------------------*/
void Demo_PowerDownWakeUp(void)
{
	/* 					*/
	PUINT8 pu8Buf, pu8Tmp;	
	UINT32 u32Idx, reg_AHBCLK;
	UINT32 reg_APBCLK;
	UINT32 reg_AUDIO_CON, reg_MISCR, reg_POR_LVRD;

	pu8Buf = u32Array;		
	DBG_PRINTF("Allocate memory address =0x%x\n", pu8Buf);
	pu8Tmp = pu8Buf;
	for(u32Idx=0; u32Idx<(BUF_SIZE);u32Idx=u32Idx+1)
		*pu8Tmp++= (UINT8)((u32Idx>>8) + u32Idx);
	gpio_setportpull(GPIO_PORTA, 0x01, 0x01);		/*Set GPIOA-0 to pull high 		*/	
	gpio_setportdir(GPIO_PORTA, 0x01, 0x00);		/*Correct	Set GPIOA-0 as input port		*/			
	//gpio_setportdir(GPIO_PORTA, 0x01, 0x01);		/*Wrong Set GPIOA-0 as input port		*/
	gpio_setsrcgrp(GPIO_PORTA, 0x01, 0x00);		/*Group GPIOA-0 to EXT_GPIO0	*/
	gpio_setintmode(GPIO_PORTA, 0x01, 0x01, 0x01);	/*Rising/Falling				 	*/
	outp32(REG_IRQTGSRC0, 0xFFFFFFFF);
	outp32(REG_IRQLHSEL, 0x11);
	/* Set gpio wake up source	*/
	DBG_PRINTF("Enter power down, GPIO Int status 0x%x\n", inp32(REG_IRQTGSRC0));

	DBG_PRINTF("Disable USB Transceiver\n");
	outp32(REG_AHBCLK, inp32(REG_AHBCLK) | USBH_CKE);					//USB Host transceiver disable. 
	outp32(0xb1009200, 0x08000000);

	DBG_PRINTF("Disable Audio ADC and Touch ADC and LVR\n");
	outp32(REG_APBCLK, inp32(REG_APBCLK) | (ADC_CKE | TOUCH_CKE));					
	outp32 (REG_ADC_CON, inp32(REG_ADC_CON) & ~(ADC_CON_ADC_EN | AUDADC_EN)); //ADC touch and ADC audio disable

#if 1	//20170818
	reg_AUDIO_CON = inp32(REG_AUDIO_CON);
	reg_MISCR = inp32(REG_MISCR);
	outp32 (REG_AUDIO_CON, inp32(REG_AUDIO_CON) & ~AUDIO_VOL_EN);
	outp32(REG_MISCR, inp32(REG_MISCR) & ~LVR_EN);	
	//outp32(REG_POR_LVRD, 0x70);			//Not to change LVR register, otherwise, next pwoer down then wakeup will reboot
#endif	
	//sysDelay(1);
	outp32(REG_APBCLK, inp32(REG_APBCLK) & ~(ADC_CKE|TOUCH_CKE));
	outp32(REG_AHBCLK, inp32(REG_AHBCLK) & ~USBH_CKE);					//USB Host transceiver disable. 


	
	DBG_PRINTF("Disable SPU and ADO\n");																												
	outp32(REG_AHBCLK, inp32(REG_AHBCLK) | (SPU_CKE | ADO_CKE));		//DAC VDD33 power down 															
	//outp32(REG_SPU_DAC_VOL, inp32(REG_SPU_DAC_VOL) | ANA_PD);		//DAC SPU HPVDD33		//DAC SPU VDD33
	DBG_PRINTF("DAC register Index 0x5 = 0x%x\n", DrvSPU_ReadDACReg(0x05));
	DBG_PRINTF("DAC register Index 0x7 = 0x%x\n", DrvSPU_ReadDACReg(0x07));
	DrvSPU_WriteDACReg(0x05, 0xFF);
	DrvSPU_WriteDACReg(0x07, 0x00);
	DBG_PRINTF("DAC register Index 0x5 = 0x%x\n", DrvSPU_ReadDACReg(0x05));
	DBG_PRINTF("DAC register Index 0x7 = 0x%x\n", DrvSPU_ReadDACReg(0x07));
	outp32(REG_AHBCLK, inp32(REG_AHBCLK) & ~(SPU_CKE | ADO_CKE));															
														

	DBG_PRINTF("Disable USB phy\n");														
	outp32(REG_AHBCLK, inp32(REG_AHBCLK) | USBD_CKE);				//USB phy disable
	outp32(PHY_CTL, inp32(PHY_CTL)&~Phy_suspend);
	outp32(REG_AHBCLK, inp32(REG_AHBCLK) & ~USBD_CKE);
	DBG_PRINTF("Disable TV DAC \n");
	outp32(REG_AHBCLK, inp32(REG_AHBCLK) | VPOST_CKE);				//TV DAC
	outp32(REG_LCM_TVCtl, inp32(REG_LCM_TVCtl) | TVCtl_Tvdac);
	outp32(REG_AHBCLK, inp32(REG_AHBCLK) & ~VPOST_CKE);

	reg_AHBCLK = inp32(REG_AHBCLK);
	reg_APBCLK = inp32(REG_APBCLK);


	/* change  SD card pin function */
	outpw(REG_GPAFUN, inpw(REG_GPAFUN) & ~MF_GPA1);	// GPA1 for SD-0 card detection
	outpw(REG_GPEFUN, inpw(REG_GPEFUN)& ~0x0000FFF0 );	// SD0_CLK/CMD/DAT0_3 pins selected
	outpw(REG_GPAFUN, inpw(REG_GPAFUN) & ~MF_GPA0);	
	outp32(REG_GPAFUN, (MF_GPA11 | MF_GPA10));
	outp32(REG_GPBFUN, 0x0);
	outp32(REG_GPCFUN, 0x0);
	

	//outp32(REG_GPDFUN, (MF_GPD4 | MF_GPD3| MF_GPD2 | MF_GPD1 | MF_GPD0)); //IF USE ICE, enable the line
	
	outp32(REG_GPEFUN, 0x0);
	outp32(REG_GPGFUN,  0);


	/* For IO power  */
	//outp32(REG_SHRPIN_TVDAC, 0x0);//MARK --> 205uA

	outp32(REG_SHRPIN_AUDIO, 0x0);
	outp32(REG_SHRPIN_TOUCH, 0x0);
    //outp32(REG_SHRPIN_R_FB, 0x0);  //X
	outp32(REG_SHRPIN_R_FB, R_DIV_ENB);
	
	//Bon suggest
    outp32(REG_GPIOG_PUEN, inp32(REG_GPIOG_PUEN)&~(BIT11|BIT12|BIT13|BIT14|BIT15));	

	outp32(REG_GPAFUN, (MF_GPA11 | MF_GPA10));
	DBG_PRINTF("GPIOA STATUS = 0x%x\n", inp32(REG_GPIOA_PIN));
	DBG_PRINTF("GPIOB STATUS = 0x%x\n", inp32(REG_GPIOB_PIN));
	DBG_PRINTF("GPIOC STATUS = 0x%x\n", inp32(REG_GPIOC_PIN));
	DBG_PRINTF("GPIOD STATUS = 0x%x\n", inp32(REG_GPIOD_PIN));
	DBG_PRINTF("GPIOE STATUS = 0x%x\n", inp32(REG_GPIOE_PIN));
	DBG_PRINTF("GPIOG STATUS = 0x%x\n", inp32(REG_GPIOG_PIN));
	DBG_PRINTF("GPIOH STATUS = 0x%x\n", inp32(REG_GPIOH_PIN));

	outp32(REG_GPIOA_OMD, 0x0);
	outp32(REG_GPIOB_OMD, 0x0);
	outp32(REG_GPIOC_OMD, 0x0);
	outp32(REG_GPIOD_OMD, 0x0);
	outp32(REG_GPIOE_OMD, 0x0);
	outp32(REG_GPIOG_OMD, 0x0);
	outp32(REG_GPIOH_OMD, 0x0);
	outp32(REG_GPIOA_PUEN, 0x3FF);
	outp32(REG_GPIOB_PUEN, 0xFFFF);
	outp32(REG_GPIOC_PUEN, 0xFFFF);
	outp32(REG_GPIOD_PUEN, 0xFFFF);
	outp32(REG_GPIOE_PUEN, 0x0FFF);	
	

	outp32(REG_GPIOG_PUEN, ~0xF85C);		//Pull up is inverse !	1.634~1.682mA

	//outp32(REG_GPIOH_PUEN, 0x0);		//Don't set GPIOH. R_FB will consume some power.
			
	outp32(REG_AHBCLK, 0x11F);
	outp32(REG_APBCLK, ~(KPI_CKE | TIC_CKE | WDCLK_CKE | TOUCH_CKE | TMR1_CKE | TMR0_CKE|\
						SPIMS1_CKE|SPIMS0_CKE|PWM_CKE|I2C_CKE|ADC_CKE));						
	
	outp32(REG_SDOPM, inp32(REG_SDOPM) & ~AUTOPDN);
	sysPowerDown(WE_GPIO);
	
	
	outp32(REG_AUDIO_CON, reg_AUDIO_CON);
	outp32(REG_MISCR, reg_MISCR);
	//outp32(REG_POR_LVRD, reg_POR_LVRD);	//Not to change LVR register, otherwise, next pwoer down then wakeup will reboot
	
	outp32(REG_SDOPM, inp32(REG_SDOPM) | AUTOPDN);
	outp32(REG_AHBCLK, reg_AHBCLK);
	outp32(REG_APBCLK, reg_APBCLK);	

#if 0	
	 /* For measure the wake up time after event trigger */	
	outp32(REG_GPIOA_OMD, inp32(REG_GPIOA_OMD) | 0x02);	//GPIOA-1 output. 
	outp32(REG_GPIOA_DOUT, inp32(REG_GPIOA_DOUT) & ~0x02);	//GPIOA-1 output LOW. 
#endif 
#if 1	
	 /* For measure the wake up time after event trigger */	
	outp32(REG_GPAFUN, inp32(REG_GPAFUN) & ~MF_GPA3);	 
	outp32(REG_GPIOA_OMD, inp32(REG_GPIOA_OMD) | 0x08);		//GPIOA-3 output
	outp32(REG_GPIOA_PUEN, inp32(REG_GPIOA_PUEN) & ~0x08); 
	outp32(REG_GPIOA_DOUT, inp32(REG_GPIOA_DOUT) & ~0x08);	//GPIOA-3 output LOW. 
#endif 
	
	
	sysprintf("Exit power down\n");		
	pu8Tmp = pu8Buf;
	for(u32Idx=0; u32Idx<(BUF_SIZE);u32Idx=u32Idx+1)
	{
		if( *pu8Tmp !=  (UINT8)((u32Idx>>8) + u32Idx))
		{
			sysprintf("!!!!!!!!!!!!!!!Data is noconsisient after power down\n");
			sysprintf("0x%x, 0x%x, 0x%x)\n",u32Idx, *pu8Tmp, (UINT8)((u32Idx>>8) + u32Idx) );
			free(pu8Buf);	
			return;
		}	
		pu8Tmp++;
	}
	sysprintf("Data is consisient\n");
	//free(pu8Buf);	
}






