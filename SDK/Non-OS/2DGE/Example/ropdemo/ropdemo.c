/******************************************************************************
 *
 *  Copyright (c) 2014 by Nuvoton Technology Corp. All rights reserved.       
 *       
 *  Nuvoton N3291x 2D Graphics Engine Low-Level Library Sample Program
 *
 *  This sample program shows the usage of N3291x ROP functions. 
 *                                                             
 ******************************************************************************/
#include "wblib.h"
#include "w55fa95_gfx.h"

#include "w55fa95_vpost.h"

/* options */

#define _GFX_BPP_           16 // can be 8, 16, 32
//#define _MONO_PATTERN_

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
#define PAT_START_ADDR          0x280000
#define SRC_START_ADDR          0x300000
#define OFFSCREEN_START_ADDR    0x380000

#define SRC_PITCH           (LCM_WIDTH * (_GFX_BPP_/8))
#define DEST_PITCH          (LCM_WIDTH * (_GFX_BPP_/8))

#define COLOR_BLACK         0x00000000
#define COLOR_RED           0x00ff0000
#define COLOR_GREEN         0x0000ff00
#define COLOR_BLUE          0x000000ff
#define COLOR_WHITE         0x00ffffff

#define SRC_OUT_X1          0
#define SRC_OUT_X2          (SRC_OUT_X1 + (LCM_WIDTH/2) - 1)
#define SRC_OUT_Y1          0
#define SRC_OUT_Y2          (SRC_OUT_Y1 + (LCM_HEIGHT/2) - 1)
#define SRC_OUT_COLOR       0xFFFF00
#define SRC_IN_X1           (LCM_WIDTH/8)
#define SRC_IN_X2           (SRC_IN_X1 + (LCM_WIDTH/4) - 1)
#define SRC_IN_Y1           (LCM_HEIGHT/8)
#define SRC_IN_Y2           (SRC_IN_Y1 + (LCM_HEIGHT/4) - 1)
#define SRC_IN_COLOR        0xFF00FF

#define DEST_OUT_X1         (SRC_OUT_X1 + (LCM_WIDTH/2))
#define DEST_OUT_X2         (SRC_OUT_X2 + (LCM_WIDTH/2))
#define DEST_OUT_Y1         (SRC_OUT_Y1 + (LCM_HEIGHT/2))
#define DEST_OUT_Y2         (SRC_OUT_Y2 + (LCM_HEIGHT/2))
#define DEST_OUT_COLOR      0x0000FF
#define DEST_IN_X1          (SRC_IN_X1 + (LCM_WIDTH/2))
#define DEST_IN_X2          (SRC_IN_X2 + (LCM_WIDTH/2))
#define DEST_IN_Y1          (SRC_IN_Y1 + (LCM_HEIGHT/2))
#define DEST_IN_Y2          (SRC_IN_Y2 + (LCM_HEIGHT/2))
#define DEST_IN_COLOR       0x00FF00

#define FORE_COLOR          SRC_IN_COLOR
#define BACK_COLOR          DEST_IN_COLOR

static UINT32 MonoPatternData[1][2] =
{
    {0x81422418, 0x18244281}  // HS_DIAGCROSS
};

static UINT8 SourceColorPatternData[8*8*4] = // 8*8*4
{
    #include "../pat8x8-1.dat"
};

UINT8 ColorPatternData[8*8*2];


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


static VOID init_color_pattern()
{
  UINT8 *ptr_pat;
  UINT16 *ptr16, r16, g16, b16;
  INT idx;
  
  ptr_pat = (UINT8 *)SourceColorPatternData;
  
  ptr16 = (UINT16 *)ColorPatternData;
  
  for (idx=0; idx<64; idx++)
  {
    b16 = (UINT16)(*ptr_pat++) & 0x000f8; // 5 bits
    g16 = (UINT16)(*ptr_pat++) & 0x000fc; // 6 bits
    r16 = (UINT16)(*ptr_pat++) & 0x000f8; // 5 bits
    ptr_pat++; 
    *ptr16++ = (r16<<8) | (g16<<3) | (b16>>3);
  }
}


INT main()
{
    GFX_INFO_T gfx_env_info;
    GFX_RECT_T rect;
    GFX_PNT_T dest_pnt;    
    UINT8 rop;
    
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

    clear_screen();

#ifdef _MONO_PATTERN_
    gfxSetPattern(GFX_MONO_PATTERN, gfxMakeColor(FORE_COLOR), gfxMakeColor(BACK_COLOR), (PVOID)&MonoPatternData[0][0]);
#else 
    init_color_pattern();
    gfxSetPattern(GFX_COLOR_PATTERN, 0, 0, (PVOID)ColorPatternData);
#endif  

    //
    // 0x0F : Pn
    // 0x55 : Dn
    // 0xF0 : P
    //
    rop = 0x00;

    /*--- put source image */
        
    rect.fC.nLeft = SRC_OUT_X1;
    rect.fC.nRight = SRC_OUT_X2;
    rect.fC.nTop = SRC_OUT_Y1;
    rect.fC.nBottom = SRC_OUT_Y2;
                
    gfxFillSolidRect(rect, gfxMakeColor(SRC_OUT_COLOR));

    rect.fC.nLeft = SRC_IN_X1;
    rect.fC.nRight = SRC_IN_X2;
    rect.fC.nTop = SRC_IN_Y1;
    rect.fC.nBottom = SRC_IN_Y2;
                
    gfxFillSolidRect(rect, gfxMakeColor(SRC_IN_COLOR));
    
    while (1)
    {
        /*--- put destination image */

        rect.fC.nLeft = DEST_OUT_X1;
        rect.fC.nRight = DEST_OUT_X2;
        rect.fC.nTop = DEST_OUT_Y1;
        rect.fC.nBottom = DEST_OUT_Y2;
                
        gfxFillSolidRect(rect, gfxMakeColor(DEST_OUT_COLOR));

        rect.fC.nLeft = DEST_IN_X1;
        rect.fC.nRight = DEST_IN_X2;
        rect.fC.nTop = DEST_IN_Y1;
        rect.fC.nBottom = DEST_IN_Y2;
                
        gfxFillSolidRect(rect, gfxMakeColor(DEST_IN_COLOR));

        gfxSetROP(rop++);
   
        rect.fC.nLeft = SRC_OUT_X1;
        rect.fC.nRight = SRC_OUT_X2;
        rect.fC.nTop = SRC_OUT_Y1;
        rect.fC.nBottom = SRC_OUT_Y2;
        
        dest_pnt.nX = DEST_OUT_X1;
        dest_pnt.nY = DEST_OUT_Y1;
        
        gfxScreenToScreenBlt(rect, dest_pnt);
        
        delay(50);
    }
    
    gfxCloseEnv();
    
    while (1); 
           
    return 0; 
}

