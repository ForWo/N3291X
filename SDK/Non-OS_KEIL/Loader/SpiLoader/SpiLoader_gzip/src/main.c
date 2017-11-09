/****************************************************************************
*                                                                           *
* Copyright (c) 2009 Nuvoton Tech. Corp. All rights reserved.               *
*                                                                           *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wblib.h"
#include "spiloader.h"
#include "w55fa95_reg.h"
#include "w55fa95_vpost.h"
#ifdef __Security__
#include "Gneiss.h"
#endif
#include "tag.h"
#ifdef USB_HOST
#include "usb.h"
#endif
#include "spu.h"

#define LOAD_IMAGE			0
#define CHECK_HEADER_ONLY 	1
#define IMAGE_BUFFER		0xA00000

UINT8 image_buffer[4096];
unsigned char *imagebuf;
unsigned int *pImageList;

#ifdef __Security__
	#define DATE_CODE   "20160614 with Security"
#else
	#define DATE_CODE   "20140409"
#endif
int do_bootm (UINT32 u32SrcAddress,UINT32 u32DestAddress, UINT32 u32Mode);

extern void lcmFill2Dark(unsigned char*);
extern void initVPostShowLogo(void);
extern void AudioChanelControl(void);
extern void backLightEnable(void);
int kfd, mfd;
LCDFORMATEX lcdInfo;
#ifdef __Security__
#define KEY_INDEX 1
void RPMC_CreateRootKey(unsigned char *u8uid, unsigned int id_len, unsigned char *rootkey);
#endif
	
UINT32 u32TimerChannel = 0;
void Timer0_300msCallback(void)
{
#ifdef __BACKLIGHT__
	backLightEnable();
#endif	
	sysClearTimerEvent(TIMER0, u32TimerChannel);
}

#define __DDR2__
// Clock Skew for FA95 MCP version C
#define E_CLKSKEW   0x0088ff00

void initClock(void)
{
    UINT32 u32ExtFreq;
    UINT32 reg_tmp;

    u32ExtFreq = sysGetExternalClock();     // Hz unit
    if(u32ExtFreq==12000000)
    {
        outp32(REG_SDREF, 0x805A);
    }
    else
    {
        outp32(REG_SDREF, 0x80C0);
    }

#ifdef __UPLL_NOT_SET__
    // 2012/3/7 by CJChen1@nuvoton.com, don't change anything that include
    //      System clock, clock skew, REG_DQSODS and REG_SDTIME.
    //      System clock will follow IBR setting that should be UPLL/System/CPU 264MHz, HCLK1 132MHz, APB 33MHz.
    sysprintf("Spi Loader DONOT set anything and follow IBR setting !!\n");
#endif  // __UPLL_NOT_SET__

#ifdef __UPLL_264__
    // Follow all IBR setting but REG_DQSODS and Clock skew.
    outp32(REG_DQSODS, 0x1010);
    outp32(REG_CKDQSDS, E_CLKSKEW);
#endif  // __UPLL_264__

#ifdef __UPLL_288__
    outp32(REG_DQSODS, 0x1010);
    outp32(REG_CKDQSDS, E_CLKSKEW);
    #ifdef __DDR2__
        outp32(REG_SDTIME, 0x2A62F74A);     // REG_SDTIME for 288M/300MHz system clock
        outp32(REG_SDMR, 0x00000432);
        outp32(REG_MISC_SSEL, 0x00000155);  // set MISC_SSEL to Reduced Strength to improve EMI
    #endif

    // initial system clock
    sysSetSystemClock(eSYS_UPLL,
                    288000000,      // Specified the APLL/UPLL clock, unit Hz
                    288000000);     // Specified the system clock, unit Hz
    sysSetCPUClock (288000000);     // Unit Hz
    sysSetAPBClock ( 72000000);     // Unit Hz
#endif  // __UPLL_288__

#ifdef __UPLL_300__
    outp32(REG_DQSODS, 0x1010);
    outp32(REG_CKDQSDS, E_CLKSKEW);
    #ifdef __DDR2__
        outp32(REG_SDTIME, 0x2A62F74A);     // REG_SDTIME for 300MHz system clock
        outp32(REG_SDMR, 0x00000432);
        outp32(REG_MISC_SSEL, 0x00000155);  // set MISC_SSEL to Reduced Strength to improve EMI
    #endif

    // initial system clock
    sysSetSystemClock(eSYS_UPLL,
                    300000000,      // Specified the APLL/UPLL clock, unit Hz
                    300000000);     // Specified the system clock, unit Hz
    sysSetCPUClock (300000000);     // Unit Hz
    sysSetAPBClock ( 75000000);     // Unit Hz
#endif  // __UPLL_300__

    // always set APLL to 432MHz
    sysSetPllClock(eSYS_APLL, 432000000);

    // always set HCLK234 to 0
    reg_tmp = inp32(REG_CLKDIV4) | CHG_APB;     // MUST set CHG_APB to HIGH when configure CLKDIV4
    outp32(REG_CLKDIV4, reg_tmp & (~HCLK234_N));

    // for FA95c, set Watchdog Timer Prescale to Normal mode
    outp32(REG_WTCR, (inp32(REG_WTCR) | 0x0100));
}
#ifndef __No_LCM__
static UINT32 bIsInitVpost=FALSE;
void initVPostShowLogo(void)
{
	if(bIsInitVpost==FALSE)
	{
		bIsInitVpost = TRUE;
		//lcdInfo.ucVASrcFormat = DRVVPOST_FRAME_YCBYCR;	
		lcdInfo.ucVASrcFormat = DRVVPOST_FRAME_RGB565;	
		lcdInfo.nScreenWidth = PANEL_WIDTH;	
		lcdInfo.nScreenHeight = PANEL_HEIGHT;
		vpostLCMInit(&lcdInfo, (UINT32*)FB_ADDR);
		//backLightEnable();
	}
}
#endif
void init(void)
{
	WB_UART_T 	uart;
	UINT32 		u32ExtFreq;	    	    	
	UINT32 u32Cke = inp32(REG_AHBCLK);
	
	/* Reset SIC engine to fix USB update kernel and mvoie file */
	outp32(REG_AHBCLK, u32Cke  | (SIC_CKE | NAND_CKE | SD_CKE)); 
	outp32(REG_AHBIPRST, inp32(REG_AHBIPRST )|SICRST ); 
	outp32(REG_AHBIPRST, 0); 
	outp32(REG_AHBCLK,u32Cke);	
		
	outp32(REG_APBCLK, inp32(REG_APBCLK) | ADC_CKE);
	outp32(REG_AUDADC_CTL, inp32(REG_AUDADC_CTL) | BIT31);
	outp32(REG_APBCLK, inp32(REG_APBCLK) & ~ADC_CKE);	
	
	
	sysEnableCache(CACHE_WRITE_BACK);
	
	u32ExtFreq = sysGetExternalClock();    	/* KHz unit */	

    /* enable UART */
    sysUartPort(1);
	uart.uiFreq = u32ExtFreq;					/* Hz unit */	
	uart.uiBaudrate = 115200;
	uart.uiDataBits = WB_DATA_BITS_8;
	uart.uiStopBits = WB_STOP_BITS_1;
	uart.uiParity = WB_PARITY_NONE;
	uart.uiRxTriggerLevel = LEVEL_1_BYTE;
	sysInitializeUART(&uart);
	sysprintf("SPI Loader with gzip start (%s)\n",DATE_CODE);
	sysFlushCache(I_D_CACHE);
}


typedef struct sd_info
{
	unsigned int startSector;
	unsigned int endSector;
	unsigned int fileLen;
	unsigned int executeAddr;
}NVT_SD_INFO_T;



//The following codes are added to support Linux tag list [2007/03/21]
//currently, only compressed romfs's size and physical address are supported!
int  TAG_create(unsigned int addr, unsigned int size)
{
	static struct tag *tlist;
	
	tlist = (struct tag *) 0x100; //will destroy BIB_ShowInfo()
	
	//tag-list start
//	sysprintf("tlist->hdr.tag = ATAG_CORE;\n");
	tlist->hdr.tag = ATAG_CORE;
    tlist->hdr.size = tag_size (tag_core);
    tlist = tag_next (tlist);

	//tag-list node
    tlist->hdr.tag = ATAG_INITRD2;
    tlist->hdr.size = tag_size (tag_initrd);
    tlist->u.initrd.start = addr;  //romfs starting address 
    tlist->u.initrd.size = size;   //romfs size 
    tlist = tag_next (tlist);

	//tag-list node
 //   tlist->hdr.tag = ATAG_MACADDR;
 //   tlist->hdr.size = tag_size (tag_macaddr);
 ///   memcpy(&tlist->u.macaddr.mac[0], &_HostMAC[0], 6);
    /*    
    uprintf("===>%02x %02x %02x %02x %02x %02x\n", tlist->u.macaddr.mac[0],
                                                   tlist->u.macaddr.mac[1],
                                                   tlist->u.macaddr.mac[2],
                                                   tlist->u.macaddr.mac[3],
                                                   tlist->u.macaddr.mac[4],
                                                   tlist->u.macaddr.mac[5],
                                                   tlist->u.macaddr.mac[6]);
    */                                                   
    tlist = tag_next (tlist);

 	//tag-list end
    tlist->hdr.tag = ATAG_NONE;
    tlist->hdr.size = 0;
    
    return 0;
}

volatile int tag_flag = 0, tagaddr,tagsize;

int main(void)
{
	unsigned int startBlock;
	unsigned int endBlock;
	unsigned int fileLen;
	unsigned int executeAddr;
	
#ifdef __Security__
	UINT8 	u8UID[8];
	unsigned char ROOTKey[32];	// Rootkey array	
	unsigned char HMACKey[32];	// HMACkey array
	unsigned char HMACMessage[4]; // HMAC message data, use for update HMAC key
	unsigned char Input_tag[12];	// Input tag data for request conte
	unsigned char RPMCStatus;
#endif
	int count, i;	
	void	(*fw_func)(void);
#ifndef __No_RTC__
	outp32(PWRON, 0x60005);    	/* Press Power Key during 6 sec to Power off (0x'6'0005) */  
#endif	

	initClock();
	init();			
#ifndef __No_LCM__	
	initVPostShowLogo();	
#endif

#ifdef __DAC_ON__
	spuOpen(eDRVSPU_FREQ_8000);
	spuDacOn(2);
	spuSetDacSlaveMode();		
    spuSetVolume(60, 60);
#endif
	
	imagebuf = (UINT8 *)((UINT32)image_buffer | 0x80000000);
	pImageList=((unsigned int *)(((unsigned int)image_buffer)|0x80000000));

	/* Initial DMAC and NAND interface */
	SPI_OpenSPI();
#ifdef __Security__
	if ((RPMC_ReadUID(u8UID)) == -1)
	{
		sysprintf("read id error !!\n");
		return -1;
	}

	sysprintf("SPI flash uid [0x%02X%02X%02X%02X%02X%02X%02X%02X]\n",u8UID[0], u8UID[1],u8UID[2], u8UID[3],u8UID[4], u8UID[5],u8UID[6], u8UID[7]);
  
	/* first stage, initial rootkey */
	RPMC_CreateRootKey((unsigned char *)u8UID,8, ROOTKey);	// caculate ROOTKey with UID & ROOTKeyTag by SHA256

	/* Second stage, update HMACKey after ever power on. without update HMACkey, Gneiss would not function*/
	HMACMessage[0] = rand()%0x100;        // Get random data for HMAC message, it can also be serial number, RTC information and so on.
	HMACMessage[1] = rand()%0x100;
	HMACMessage[2] = rand()%0x100;
	HMACMessage[3] = rand()%0x100;

	/* Update HMAC key and get new HMACKey. 
	HMACKey is generated by SW using Rootkey and HMACMessage.
	RPMC would also generate the same HMACKey by HW */
	RPMCStatus = RPMC_UpHMACkey(KEY_INDEX, ROOTKey, HMACMessage, HMACKey); 	
	if(RPMCStatus == 0x80)
	{
		// update HMACkey success
		sysprintf("RPMC_UpHMACkey Success - 0x%02X!!\n",RPMCStatus );
	}
	else
	{
		// write HMACkey fail, check datasheet for the error bit
		sysprintf("RPMC_UpHMACkey Fail - 0x%02X!!\n",RPMCStatus );
	}

	/* Third stage, increase RPMC counter */  
	/* input tag is send in to RPMC, it could be time stamp, serial number and so on*/
	for(i= 0; i<12;i++)
		Input_tag[i] = u8UID[i%8];
	
	RPMCStatus = RPMC_IncCounter(KEY_INDEX, HMACKey, Input_tag);	
	if(RPMCStatus == 0x80){
		// increase counter success
		sysprintf("RPMC_IncCounter Success - 0x%02X!!\n",RPMCStatus );
	}
	else{
		// increase counter fail, check datasheet for the error bit
		sysprintf("RPMC_IncCounter Fail - 0x%02X!!\n",RPMCStatus );
		while(1);
	}
			
	if(RPMC_Challenge(KEY_INDEX, HMACKey, Input_tag)!=0)
	{
		sysprintf("RPMC_Challenge Fail!!\n" );
		/* return signature miss-match */
		while(1);
	}	
	else	
		sysprintf("RPMC_Challenge Pass!!\n" );
#endif		
	memset(imagebuf, 0, 1024);
	sysprintf("Load Image ");
	/* read image information */
	SPIReadFast(0, 63*1024, 1024, (UINT32*)imagebuf);  /* offset, len, address */

	if (((*(pImageList+0)) == 0xAA554257) && ((*(pImageList+3)) == 0x63594257))
	{
		count = *(pImageList+1);

		pImageList=((unsigned int*)(((unsigned int)image_buffer)|0x80000000));
		startBlock = endBlock = fileLen = executeAddr = 0;
		
		/* load logo first */
		pImageList = pImageList+4;
		for (i=0; i<count; i++)
		{
			if (((*(pImageList) >> 16) & 0xffff) == 4)	// logo
			{
				startBlock = *(pImageList + 1) & 0xffff;
				endBlock = (*(pImageList + 1) & 0xffff0000) >> 16;
				executeAddr = *(pImageList + 2);
				fileLen = *(pImageList + 3);
				SPIReadFast(0, startBlock * 0x10000, fileLen, (UINT32*)executeAddr);						
				break;
			}
			/* pointer to next image */
			pImageList = pImageList+12;
		}

		pImageList=((unsigned int*)(((unsigned int)image_buffer)|0x80000000));
		startBlock = endBlock = fileLen = executeAddr = 0;

		/* load romfs file */
		pImageList = pImageList+4;
		for (i=0; i<count; i++)
		{
			if (((*(pImageList) >> 16) & 0xffff) == 2)	// RomFS
			{
				startBlock = *(pImageList + 1) & 0xffff;
				endBlock = (*(pImageList + 1) & 0xffff0000) >> 16;
				executeAddr = *(pImageList + 2);
				fileLen = *(pImageList + 3);
				SPIReadFast(0, startBlock * 0x10000, fileLen, (UINT32*)executeAddr);
				tag_flag = 1;
				tagaddr = executeAddr;
				tagsize = fileLen;
				
				break;
			}
			/* pointer to next image */
			pImageList = pImageList+12;
		}

		pImageList=((unsigned int*)(((unsigned int)image_buffer)|0x80000000));
		startBlock = endBlock = fileLen = executeAddr = 0;

		/* load execution file */
		pImageList = pImageList+4;
		for (i=0; i<count; i++)
		{
			if (((*(pImageList) >> 16) & 0xffff) == 1)	// execute
			{
				UINT32 u32Result;
				startBlock = *(pImageList + 1) & 0xffff;
				endBlock = (*(pImageList + 1) & 0xffff0000) >> 16;
				executeAddr = *(pImageList + 2);
				fileLen = *(pImageList + 3);
				
				sysSetGlobalInterrupt(DISABLE_ALL_INTERRUPTS);
				sysSetLocalInterrupt(DISABLE_FIQ_IRQ);	
				
				SPIReadFast(0, startBlock * 0x10000, 64, (UINT32*)IMAGE_BUFFER);
				
				u32Result = do_bootm(IMAGE_BUFFER, 0, CHECK_HEADER_ONLY);		
						
				if(u32Result)		/* Not compressed */
					SPIReadFast(0, startBlock * 0x10000, fileLen, (UINT32*)executeAddr);
				else				/* compressed */
				{
					SPIReadFast(0, startBlock * 0x10000, fileLen, (UINT32*)IMAGE_BUFFER);
					do_bootm(IMAGE_BUFFER, executeAddr, LOAD_IMAGE);	
				}
				
				// Invalid and disable cache
				sysDisableCache();
				sysInvalidCache();
				
			//	memcpy(0x0, kbuf, CP_SIZE);				

				if(tag_flag)
				{
					sysprintf("Create Tag - Address 0x%08X, Size 0x%08X\n",tagaddr,tagsize );
					TAG_create(tagaddr,tagsize);				
				}								
				
				// JUMP to kernel
				sysprintf("Jump to kernel\n");

				
				//lcmFill2Dark((char *)(FB_ADDR | 0x80000000));	
				outp32(REG_AHBIPRST, JPGRST | SICRST |UDCRST | EDMARST);
				outp32(REG_AHBIPRST, 0);
				outp32(REG_APBIPRST, UART1RST | UART0RST | TMR1RST | TMR0RST );
				outp32(REG_APBIPRST, 0);
				sysFlushCache(I_D_CACHE);	   	
				
					
				fw_func = (void(*)(void))(executeAddr);
				fw_func();				
				break;
			}
			/* pointer to next image */
			pImageList = pImageList+12;
		}
	}	
	
	return(0); // avoid compilation warning
}