/******************************************************************************
 *
 *  Copyright (c) 2014 by Nuvoton Technology Corp. All rights reserved.       
 *       
 *  Nuvoton N3291x 2D Graphics Engine Low-Level Library Sample Program
 *
 *  This sample program shows the usage of N3291x transparency control. . 
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

#define IMAGE_WIDTH         320
#define IMAGE_HEIGHT        240

#define COLOR_KEY           0x64b9  // in RGB565
#define COLOR_MASK          0xffff  // in RGB565

/* definition of pan/tilt direction */

#define PX      0
#define PY      1
#define NX      2
#define NY      3

static UINT8 BackgroundImage[]=
{
    #include "../sea_800x480_RGB565.dat"
};

static UINT8 ColorKeyImage[]=
{
    #include "../keytest.dat"
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
    GFX_SURFACE_T src_surface, dest_surface;    
    GFX_RECT_T src_rect, ckey_rect;
    GFX_PNT_T dest_pnt, dest_pnt00;
    INT move;
    
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
    gfx_env_info.uSrcStartAddr          = (UINT32)ColorKeyImage;
    
    gfxOpenEnv(&gfx_env_info);
    
    clear_screen();    

    dest_pnt00.nX = dest_pnt.nX = 0;
    dest_pnt00.nY = dest_pnt.nY = 0;

    src_rect.fC.nLeft = 0;
    src_rect.fC.nTop = 0;
    src_rect.fC.nRight = LCM_WIDTH - 1;
    src_rect.fC.nBottom = LCM_HEIGHT - 1;

    ckey_rect.fC.nLeft = 0;
    ckey_rect.fC.nTop = 0;
    ckey_rect.fC.nRight = IMAGE_WIDTH - 1;
    ckey_rect.fC.nBottom = IMAGE_HEIGHT - 1;

    //
    // demo with source image smaller than LCD 
    //
    move = PX;
    
    while (1)
    {
        /*--- put background image to offscreen ---*/
        
        dest_surface.nWidth = LCM_WIDTH;
        dest_surface.nHeight = LCM_HEIGHT;
        dest_surface.nPitch = LCM_WIDTH * 2;
        dest_surface.uStartAddr = (UINT32)OFFSCREEN_START_ADDR;
        gfxSetDestSurface(&dest_surface);
        
        src_surface.nWidth = LCM_WIDTH;
        src_surface.nHeight = LCM_HEIGHT; 
        src_surface.nPitch = LCM_WIDTH * 2;
        src_surface.uStartAddr = (UINT32)BackgroundImage;
        gfxSetSrcSurface(&src_surface);

        src_rect.fC.nLeft = 0;
        src_rect.fC.nTop = 0;
        src_rect.fC.nRight = LCM_WIDTH - 1;
        src_rect.fC.nBottom = LCM_HEIGHT - 1;

        gfxPutImage(src_rect, dest_pnt00);

        
        /*--- overlay image with color key in offscreen ---*/
        
        dest_surface.nWidth = LCM_WIDTH;
        dest_surface.nHeight = LCM_HEIGHT;
        dest_surface.nPitch = LCM_WIDTH * 2;
        dest_surface.uStartAddr = (UINT32)OFFSCREEN_START_ADDR;
        gfxSetDestSurface(&dest_surface);
        
        src_surface.nWidth = IMAGE_WIDTH;
        src_surface.nHeight = IMAGE_HEIGHT; 
        src_surface.nPitch = IMAGE_WIDTH * 2;
        src_surface.uStartAddr = (UINT32)ColorKeyImage;
        gfxSetSrcSurface(&src_surface);

        gfxSetColorKey(TRUE, COLOR_KEY, COLOR_MASK);
        gfxPutImage(ckey_rect, dest_pnt);
        gfxSetColorKey(FALSE, COLOR_KEY, COLOR_MASK);
        
        /*--- flip memory from offscreen to onscreen ---*/
        
        dest_surface.nWidth = LCM_WIDTH;
        dest_surface.nHeight = LCM_HEIGHT;
        dest_surface.nPitch = LCM_WIDTH * 2;
        dest_surface.uStartAddr = (UINT32)DEST_START_ADDR;
        gfxSetDestSurface(&dest_surface);
        
        src_surface.nWidth = LCM_WIDTH;
        src_surface.nHeight = LCM_HEIGHT; 
        src_surface.nPitch = LCM_WIDTH * 2;
        src_surface.uStartAddr = (UINT32)OFFSCREEN_START_ADDR;
        gfxSetSrcSurface(&src_surface);

        src_rect.fC.nLeft = 0;
        src_rect.fC.nTop = 0;
        src_rect.fC.nRight = LCM_WIDTH - 1;
        src_rect.fC.nBottom = LCM_HEIGHT - 1;
        
        gfxPutImage(src_rect, dest_pnt00);
      
        switch (move)
        {
            case PX:
                dest_pnt.nX++;
                if ((dest_pnt.nX+IMAGE_WIDTH) >= LCM_WIDTH)
                {
                    dest_pnt.nX--;
                    dest_pnt.nY++;
                    move = PY;
                }
                break;
            case PY:
                dest_pnt.nY++;
                if ((dest_pnt.nY+IMAGE_HEIGHT) >= LCM_HEIGHT)
                {
                    dest_pnt.nY--;
                    dest_pnt.nX--;
                    move = NX;
                }
                break;
            case NX:
                dest_pnt.nX--;
                if (dest_pnt.nX < 0)
                {
                    dest_pnt.nX++;
                    dest_pnt.nY--;
                    move = NY;
                }
                break;
            case NY:
                dest_pnt.nY--;
                if (dest_pnt.nY < 0)
                {
                    dest_pnt.nY++;
                    dest_pnt.nX++;
                    move = PX;
                }
                break;
        }

    }
    
    gfxCloseEnv();
    
    while (1); 
           
    return 0; 
}

