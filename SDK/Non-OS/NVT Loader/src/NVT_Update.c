/****************************************************************************
*                                                                           *
* Copyright (c) 2009 Nuvoton Tech. Corp. All rights reserved.               *
*                                                                           *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wblib.h"
#include "nvtfat.h"
#include "nvtloader.h"
#include "w55fa95_sic.h"
/*===================================================================================
Update File Architecture
1. NAND Loader file name and version   	-- Burn to flash
2. Logo file name and version			-- Burn to flash	
3. LG Loader file name and version		-- Burn to flash
4. Linux Kernel file name and version		-- Copy to NAND 1-1
5. Flash Player file name and version		-- Copy to NAND 1-1
6. Flash UI folder name and version		-- Copy to NAND 1-1


Update_file_list example
@Rainbow project 
	NAND Loader 1.3
	Logo 1.0
	LG Loader 1.0
	Linux Kernel 1.4
	Flash Player 1.4
	UI 1.0
	
@Test environment	
===================================================================================*/
extern void loadKernelCont(int fd, int offset);
extern unsigned char kbuf[CP_SIZE];

#define UPDATE_PATH	"D:\\update"
char* UpdateFileName[] = {
	"Logo",
	"NVTLoader",
	"NAND1-1",
	"NAND1-2",
	};	
BOOL CheckUpdateFilesExist(void)
{		
	int			nStatus;
	CHAR		szLongName[MAX_FILE_NAME_LEN/2];
	FILE_FIND_T  	tFileInfo;
	UINT32 		u32idx=0;	
	CHAR 		suDirName[128];
	
	sprintf(szLongName, "D:\\update");
	fsAsciiToUnicode(szLongName, suDirName, TRUE);
	memset((UINT8 *)&tFileInfo, 0, sizeof(tFileInfo));
	nStatus = fsFindFirst(suDirName, NULL, &tFileInfo);
	if (nStatus < 0)
		return nStatus;	
	do 
	{
		fsUnicodeToAscii(tFileInfo.suLongName, szLongName, TRUE);					
		for(u32idx=0; u32idx<sizeof(UpdateFileName)/sizeof(char*); u32idx++)
		{
			sysprintf("Compare string = %s\n",UpdateFileName[u32idx]);
			sysprintf("Compare string len = %d\n",strlen(UpdateFileName[u32idx]));
			if(memcmp(szLongName, UpdateFileName[u32idx], strlen(UpdateFileName[u32idx]))==0)
				return TRUE;
		}	
	}while (!fsFindNext(&tFileInfo));
	return FALSE;			
}
/*
	Updater: It is uesed to burn code to the system area in NAND. And update some kernel files in NAND1-2. 	

*/
void NVT_Updater(void)
{
	INT 			fd;
	void			(*_jump)(void);
	CHAR		szLongName[MAX_FILE_NAME_LEN/2];
	CHAR 		suDirName[128];
		
	if(CheckUpdateFilesExist()==FALSE)
		return;
		
	sprintf(szLongName, "D:\\update\\NANDUpdater.bin");
	fsAsciiToUnicode(szLongName, suDirName, TRUE);
	
	fd = fsOpenFile(suDirName, 0, O_RDONLY);
	if(fd < 0) 
	{//NAND Updater does not exist. 
		return;
	}	
	sysprintf("NANDUpdater found. Load NANDUpdater\n");
	loadKernelCont(fd, 0);
	
	fsCloseFile(fd);
	sicClose();

	sysSetGlobalInterrupt(DISABLE_ALL_INTERRUPTS);
	sysSetLocalInterrupt(DISABLE_FIQ_IRQ);		
		
	memcpy(0x0, kbuf, CP_SIZE);		
	sysprintf("Jump to Updater\n");
	outp32(REG_AHBIPRST, JPGRST | SICRST |UDCRST | EDMARST);
	outp32(REG_AHBIPRST, 0);
	outp32(REG_APBIPRST, UART1RST | UART0RST | TMR1RST | TMR0RST );
	outp32(REG_APBIPRST, 0);
	sysFlushCache(I_D_CACHE);	 	  	
	// Invalid and disable cache
	sysDisableCache();
	sysInvalidCache();	
	_jump = (void(*)(void))(0x0); // Jump to 0x0 and execute kernel
	_jump();	
		
	while(1);	
}




