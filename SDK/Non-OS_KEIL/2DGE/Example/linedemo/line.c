/******************************************************************************
 *
 *  Copyright (c) 2014 by Nuvoton Technology Corp. All rights reserved.       
 *       
 *  Nuvoton N3291x 2D Graphics Engine Low-Level Library Sample Program
 *
 *  This sample program shows the usage of N3291x linw drawing.  . 
 *                                                             
 ******************************************************************************/
#include "wblib.h"
#include "w55fa95_gfx.h"

#include "w55fa95_vpost.h"

/* options */

#define _GFX_BPP_           16 // can be 8, 16, 32

#define LCM_WIDTH           800
#define LCM_HEIGHT          480
#if (_GFX_BPP_==8)
#define LCM_FORMAT          GFX_BPP_332
#endif
#if (_GFX_BPP_==16)
#define LCM_FORMAT          GFX_BPP_565
#endif
#if (_GFX_BPP_==32)         
#define LCM_FORMAT          GFX_BPP_888
#endif

#define DEST_START_ADDR         0x200000
#define PAT_START_ADDR          0x400000
#define SRC_START_ADDR          0x600000
#define OFFSCREEN_START_ADDR    0x800000

#define SRC_PITCH           (LCM_WIDTH * (_GFX_BPP_/8))
#define DEST_PITCH          (LCM_WIDTH * (_GFX_BPP_/8))

#define COLOR_BLACK         0x00000000
#define COLOR_RED           0x00ff0000
#define COLOR_GREEN         0x0000ff00
#define COLOR_BLUE          0x000000ff
#define COLOR_WHITE         0x00ffffff

/*--- pen style ---*/

#define PS_SOLID   		    0   //1111111111111111 (1111111111111111)
#define PS_DASH	   		    1	//1100110011001100 (1111000011110000)
#define PS_DOT	   		    2	//1010101010101010 (1100110011001100)
#define PS_DASHDOT	   	    3	//1110010011100100 (1111110000110000)
#define PS_DASHDOTDOT       4	//1110101011101010 (1111110011001100)
#define PS_NULL	   		    5	//0000000000000000 (0000000000000000)


static UINT16 _usLinePatternData[6] =
{ 
  0xffff, // SOLID
  0xcccc, // DASH
  0xaaaa, // DOT
  0xe4e4, // DASHDOT
  0xeaea, // DASHDOTDOT
  0x0000  // NULL
};


void initPLL(void)
{
	UINT32 u32ExtFreq;	

	/* init timer */	
	u32ExtFreq = sysGetExternalClock();    	/* Hz unit */	
	sysSetTimerReferenceClock (TIMER0, u32ExtFreq);	/* Hz unit */
	sysStartTimer(TIMER0, 100, PERIODIC_MODE);
					
	sysSetLocalInterrupt(ENABLE_IRQ);
}


void initLCM(void)
{
    LCDFORMATEX lcdFormat;
    
    lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_RGB565;

	lcdFormat.nScreenWidth = LCM_WIDTH;
	lcdFormat.nScreenHeight = LCM_HEIGHT;    

	vpostLCMInit(&lcdFormat, (UINT32*)DEST_START_ADDR);    
}


static VOID delay(INT delay_ticks)
{
    volatile INT start_tick;
    
    start_tick = sysGetTicks(TIMER0);
    while ((sysGetTicks(TIMER0)-start_tick) < delay_ticks);
}


static VOID clear_screen()
{
    gfxClearScreen(gfxMakeColor(COLOR_BLACK));
}


INT main()
{
    GFX_INFO_T gfx_env_info;
    GFX_PNT_T p1, p2;   
    
    /* initialize timer and LCM */
    
	initPLL();
      
    initLCM();

    /* open the drawing environment */
        
    gfx_env_info.nDestWidth             = LCM_WIDTH;
    gfx_env_info.nDestHeight            = LCM_HEIGHT;
    gfx_env_info.nDestPitch             = DEST_PITCH;
    gfx_env_info.nSrcWidth              = LCM_WIDTH;
    gfx_env_info.nSrcHeight             = LCM_HEIGHT;
    gfx_env_info.nSrcPitch              = SRC_PITCH;
    gfx_env_info.nColorFormat           = LCM_FORMAT;
    gfx_env_info.uDestStartAddr         = DEST_START_ADDR;
    gfx_env_info.uColorPatternStartAddr = PAT_START_ADDR;
    gfx_env_info.uSrcStartAddr          = SRC_START_ADDR;
    
    gfxOpenEnv(&gfx_env_info);

    while (1)
    {
        gfxClearScreen(gfxMakeColor(0x000000));

        p1.nX = 0;
        p1.nY = 0;
        p2.nX = LCM_WIDTH - 1;
        p2.nY = LCM_HEIGHT - 1;
        gfxDrawFrame(p1, p2, 1, gfxMakeColor(COLOR_BLUE));  

        gfxClearScreen(gfxMakeColor(0x000000));
                
        p1.nX = LCM_WIDTH/4;
        p1.nY = LCM_HEIGHT/4;
        p2.nX = p1.nX + (LCM_WIDTH/2);
        p2.nY = p1.nY + (LCM_HEIGHT/2);
        gfxDrawFrame(p1, p2, 5, gfxMakeColor(COLOR_RED)); 
        p1.nX = 0;
        p1.nY = 0;
        p2.nX = LCM_WIDTH - 1;
        p2.nY = 0;
        gfxDrawSolidLine(p1, p2, gfxMakeColor(COLOR_BLUE));

        p1.nX = LCM_WIDTH - 1;
        p1.nY = 0;
        p2.nX = LCM_WIDTH - 1;
        p2.nY = LCM_HEIGHT - 1;
        gfxDrawSolidLine(p1, p2, gfxMakeColor(COLOR_BLUE));

        p1.nX = LCM_WIDTH - 1;
        p1.nY = LCM_HEIGHT - 1;
        p2.nX = 0;
        p2.nY = LCM_HEIGHT - 1;
        gfxDrawSolidLine(p1, p2, gfxMakeColor(COLOR_BLUE));

        p1.nX = 0;
        p1.nY = LCM_HEIGHT - 1;
        p2.nX = 0;
        p2.nY = 0;
        gfxDrawSolidLine(p1, p2, gfxMakeColor(COLOR_BLUE));

        p1.nX = 0;
        p1.nY = 0;
        p2.nX = LCM_WIDTH - 1;
        p2.nY = LCM_HEIGHT - 1;
        gfxDrawSolidLine(p1, p2, gfxMakeColor(COLOR_BLUE));

        p1.nX = 0;
        p1.nY = LCM_HEIGHT - 1;
        p2.nX = LCM_WIDTH - 1;
        p2.nY = 0;
        gfxDrawSolidLine(p1, p2, gfxMakeColor(COLOR_BLUE));
 
        delay(100);
       
        /*--- styled line drawing function ---*/
        gfxClearScreen(gfxMakeColor(0x000000));

        
        gfxSetPen(_usLinePatternData[PS_DASH], gfxMakeColor(COLOR_BLUE), gfxMakeColor(COLOR_RED));  
        
        p1.nX = 0;
        p1.nY = 0;
        p2.nX = LCM_WIDTH - 1;
        p2.nY = 0;
        gfxDrawLine(p1, p2);
        
        p1.nX = LCM_WIDTH - 1;
        p1.nY = 0;
        p2.nX = LCM_WIDTH - 1;
        p2.nY = LCM_HEIGHT - 1;
        gfxDrawLine(p1, p2);

        p1.nX = LCM_WIDTH - 1;
        p1.nY = LCM_HEIGHT - 1;
        p2.nX = 0;
        p2.nY = LCM_HEIGHT - 1;
        gfxDrawLine(p1, p2);

        p1.nX = 0;
        p1.nY = LCM_HEIGHT - 1;
        p2.nX = 0;
        p2.nY = 0;
        gfxDrawLine(p1, p2);

        p1.nX = 0;
        p1.nY = 0;
        p2.nX = LCM_WIDTH - 1;
        p2.nY = LCM_HEIGHT - 1;
        gfxDrawLine(p1, p2);

        p1.nX = 0;
        p1.nY = LCM_HEIGHT - 1;
        p2.nX = LCM_WIDTH - 1;
        p2.nY = 0;
        gfxDrawLine(p1, p2);
       
        delay(100);
    }
    
    gfxCloseEnv();
    
    while (1); 
           
    return 0; 
}

