/******************************************************************************
 *
 *  Copyright (c) 2014 by Nuvoton Technology Corp. All rights reserved.       
 *       
 *  Nuvoton N3291x 2D Graphics Engine Low-Level Library Sample Program
 *
 *  This sample program shows the usage of N3291x alpha blending functions. 
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
#define PAT_START_ADDR          0x300000
#define SRC_START_ADDR          0x400000
#define OFFSCREEN_START_ADDR    0x500000

#define SRC_PITCH           (LCM_WIDTH * (_GFX_BPP_/8))
#define DEST_PITCH          (LCM_WIDTH * (_GFX_BPP_/8))

#define COLOR_BLACK         0x00000000
#define COLOR_RED           0x00ff0000
#define COLOR_GREEN         0x0000ff00
#define COLOR_BLUE          0x000000ff
#define COLOR_WHITE         0x00ffffff

#define IMAGE_WIDTH         800
#define IMAGE_HEIGHT        480

#define TOTAL_MONO_PATTERN  6
#define TOTAL_COLOR_PATTERN 2

static UINT32 MonoPatternData[6][2] =
{{0x00000000, 0xff000000}, // HS_HORIZONTAL
 {0x08080808, 0x08080808}, // HS_VERTICAL
 {0x80402010, 0x08040201}, // HS_FDIAGONAL
 {0x01020408, 0x10204080}, // HS_BDIAGONAL
 {0x08080808, 0xff080808}, // HS_CROSS
 {0x81422418, 0x18244281}  // HS_DIAGCROSS
};

static UINT8 SourceColorPatternData[2][8*8*4] = // 8*8*4
{
    {
        #include "../pat8x8-0.dat"
    },
    {
        #include "../pat8x8-1.dat"
    }
};

UINT8 ColorPatternData[8*8*4];


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


static VOID init_color_pattern(INT opt)
{
    UINT8 *ptr_pat;
    UINT8 *ptr8, r8, g8, b8;
    UINT16 *ptr16, r16, g16, b16;
    UINT32 *ptr32, r32, g32, b32;
    INT idx;
  
    ptr_pat = (UINT8 *)SourceColorPatternData[opt];
  
    if (_GFX_BPP_==8)
    {
        ptr8 = (UINT8 *)ColorPatternData;
        for (idx=0; idx<64; idx++)
        {
            b8 = (UINT8)(*ptr_pat++) & 0xc0; // 2 bits
            g8 = (UINT8)(*ptr_pat++) & 0xe0; // 3 bits
            r8 = (UINT8)(*ptr_pat++) & 0xe0; // 3 bits
            ptr_pat++;
            *ptr8++ = r8 | (g8>>3) | (b8>>6);
        }    
    }
    else if (_GFX_BPP_==16)
    {
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
    else if (_GFX_BPP_==32)
    {
        ptr32 = (UINT32 *)ColorPatternData;
        for (idx=0; idx<64; idx++)
        {   
            b32 = (UINT32)(*ptr_pat++);
            g32 = (UINT32)(*ptr_pat++);
            r32 = (UINT32)(*ptr_pat++);
            ptr_pat++;
            *ptr32++ = (r32<<16) | (g32<<8) | b32;    
        } 
    }             
}


INT main()
{
    GFX_INFO_T gfx_env_info;
    GFX_RECT_T src_rect, dest_rect;
    GFX_PNT_T dest_pnt;    
    INT x, y, alpha;
    UINT8 *ptr8;
    
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

    /* fill source pattern for blending */

#if (_GFX_BPP_==16)    
    ptr8 = (UINT8 *)SRC_START_ADDR;
    for (y=0; y<LCM_HEIGHT; y++)
    {
        for (x=0; x<LCM_WIDTH; x++)
        {
            *ptr8++ = 0x00;
            *ptr8++ = 0x00;
        }
    }
#endif    
 
 
#if (_GFX_BPP_==32)    
    ptr8 = (UINT8 *)SRC_START_ADDR;
    for (y=0; y<LCM_HEIGHT; y++)
    {
        for (x=0; x<LCM_WIDTH; x++)
        {
            *ptr8++ = 0x00;
            *ptr8++ = 0x00;
            *ptr8++ = 0x00;
            *ptr8++ = 0x00;
        }
    }
#endif    

    dest_rect.fC.nLeft = 0;
    dest_rect.fC.nTop = 0;
    dest_rect.fC.nRight = LCM_WIDTH - 1;
    dest_rect.fC.nBottom = LCM_HEIGHT - 1;
        
    src_rect.fC.nLeft = 0;
    src_rect.fC.nTop = 0;
    src_rect.fC.nRight = IMAGE_WIDTH - 1;
    src_rect.fC.nBottom = IMAGE_HEIGHT - 1;

    dest_pnt.nX = 0;
    dest_pnt.nY = 0;
    
    for (alpha=0; alpha<256; alpha++)
    {
        clear_screen();
        init_color_pattern(0);       
        gfxSetPattern(GFX_COLOR_PATTERN, 0, 0, (PVOID)ColorPatternData);
        gfxFillRect(dest_rect);        

        gfxSetAlpha(TRUE, alpha);
        gfxPutImage(src_rect, dest_pnt); 
        
        delay(50);   
    }
    
    gfxCloseEnv();
    
    while (1); 
           
    return 0; 
}

