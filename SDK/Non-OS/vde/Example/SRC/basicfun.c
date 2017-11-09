/****************************************************************************
 *                                                                                    
 * Copyright (c) Nuvoton Technology Corp. All rights reserved.           
 *                                                                                    
 ***************************************************************************/
#include <stdio.h>
#include <string.h>
#include "w55fa95_reg.h"
#include "wblib.h"
#include "wbtypes.h"
#include "wbio.h"
#include "vdodef.h"
#include "vdoapi.h"
#include "avctest.h"


#include "nvtfat.h" 
#include "w55fa95_sic.h"

             

void H264TestPattern(void)
{
	INT         nStatus;
	CHAR		szLongName[MAX_FILE_NAME_LEN];	
	CHAR		suDirName[256],fullPathName[256];	
	FILE_FIND_T tFileInfo;
    int file_length;	

	memset((CHAR *)&tFileInfo, 0, sizeof(FILE_FIND_T));


    strcpy(fullPathName,INPUT_PATTERN_FOLDER); 
	fsAsciiToUnicode(fullPathName, suDirName, 1);
	nStatus = fsFindFirst(suDirName,NULL,&tFileInfo);


	if (nStatus < 0)
	{
		Console_Printf("No file found in %s folder\n", fullPathName);
		return;
	}		
		
	do 
	{
		if (tFileInfo.ucAttrib & FA_ARCHIVE)  
        {		
    		fsUnicodeToAscii(tFileInfo.suLongName, szLongName, 1);
    		strcpy(fullPathName,INPUT_PATTERN_FOLDER);
	
            strcat(fullPathName,szLongName);
            
            file_length = strlen(fullPathName);    
            
            if ((strcmp(&fullPathName[file_length-3],"264") ==0) || (strcmp(&fullPathName[file_length-3],"jsv") ==0))    		
            	H264DecodOneFile(fullPathName);
		}			
	} while (!fsFindNext(&tFileInfo));	
	
	fsFindClose(&tFileInfo);
    
}



/* 
    Get the file size of opened file
    (Before calling this function, file is opened)
*/
int getFileLength(int fileFP)
{
    return fsGetFileSize(fileFP);
   
}


/* 
    Open file to get the file size and reset offset to 0
    (file will be open then close)
*/
int FileOP(FileOpInfo *OpenFile,int read,char *fileName)
{
	
	char suDirName[256];	
	int codecFP;	
  

	char codecFileNameAbsolute[256];	
	
	    
    if (read)
    {
        strcpy(OpenFile->Name,"");
        
        strcpy(codecFileNameAbsolute, fileName);        

        fsAsciiToUnicode(codecFileNameAbsolute,suDirName,1);                
    	if((codecFP = fsOpenFile(suDirName,codecFileNameAbsolute,O_RDONLY)) < 0)       
      	
    	{
    		Console_Printf("    Can't open %s to read.\n",fileName);
    		return FALSE;
    	}
	        

        OpenFile->OpenFP = codecFP;
        fsFileSeek(codecFP,0,SEEK_SET);
        
        strcat(OpenFile->Name,fileName);
        OpenFile->TotalFileSize = getFileLength(codecFP);
        OpenFile->UsedOffset =0;
        
    }
    else
    {
        strcpy(OpenFile->Name,""); 
        strcpy(codecFileNameAbsolute, fileName);       

        fsAsciiToUnicode(codecFileNameAbsolute,suDirName,1);                         
    	if((codecFP = fsOpenFile(suDirName,codecFileNameAbsolute,O_CREATE)) < 0)        //O_CREATE,O_WRONLY 
    	
    	{
    		Console_Printf("    Can't open %s to write.\n",fileName);
    		Console_Printf("    Please create the folder by manual\n");
    		return FALSE;
    	}
    	  
        OpenFile->OpenFP = codecFP;
        
        strcat(OpenFile->Name,fileName);
        OpenFile->TotalFileSize = getFileLength(codecFP);
        OpenFile->UsedOffset =0;        
    }
    

	fsCloseFile(codecFP);		// Close file when all bitstream is read
   
    
   	return TRUE;
}

 

