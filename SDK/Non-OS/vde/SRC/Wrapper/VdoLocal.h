/**********************************************************************************************************
 *                                                                          
 * Copyright (c) 2004 - 2007 Winbond Electronics Corp. All rights reserved.      
 *                                                                         
 * FILENAME
 *     vdoLocal.h
 *
 * VERSION
 *     1.0
 *
 * DESCRIPTION
 *
 *
 * HISTORY
 *     08/16/2007		 Ver 1.0 Created by PT11 KCHuang
 *
 * REMARK
 *     None
 *     
 **********************************************************************************************************/
 
 
//Interrupt Status bit  
#define     BIT_INITIALIZE_COOMPLETE_STATUS         0x00000001
#define     BIT_SEQ_INIT_STATUS                     0x00000002
#define     BIT_SEQ_END_STATUS                      0x00000004
#define     BIT_PIC_RUN_STATUS                      0x00000008
#define     BIT_SET_FRAME_BUF_STATUS                0x00000010
#define     BIT_ENC_HEADER_STATUS                   0x00000020
#define     BIT_ENC_PARA_SET_STATUS                 0x00000040
#define     BIT_DEC_PARA_SET_STATUS                 0x00000080
#define     BIT_DEC_BUF_FLUSH_STATUS                0x00000100
#define     BIT_BUFFER_EMPTY_STATUS                 0x00004000
#define     BIT_BUFFER_FULL_STATUS                  0x00008000

#define     BIT_INITIALIZE_COOMPLETE_ENABLE         0x00000001
#define     BIT_SEQ_INIT_ENABLE                     0x00000002
#define     BIT_SEQ_END_ENABLE                      0x00000004
#define     BIT_PIC_RUN_ENABLE                      0x00000008
#define     BIT_SET_FRAME_BUF_ENABLE                0x00000010
#define     BIT_ENC_HEADER_ENABLE                   0x00000020
#define     BIT_ENC_PARA_SET_ENABLE                 0x00000040
#define     BIT_DEC_PARA_SET_ENABLE                 0x00000080
#define     BIT_DEC_BUF_FLUSH_ENABLE                0x00000100
#define     BIT_BUFFER_EMPTY_ENABLE                 0x00004000
#define     BIT_BUFFER_FULL_ENABLE                  0x00008000



#define     MAX_MAINTAIN_INSTANCE_NUM               4
#define     MAX_INTERRUPT_SOURCE_NUM                16 
 
typedef void (*_gFunPtr)();   /* function pointer */
 
_gFunPtr _vdo_InterruptTable[MAX_INTERRUPT_SOURCE_NUM] = {0}; 


typedef struct{
    int instIndex;
    unsigned int  paBsBufStart;
    unsigned int  paBsBufEnd;    
    
} _CODEC_INSTANCE_INFO;
typedef struct{
    _CODEC_INSTANCE_INFO codecLibInfo[MAX_MAINTAIN_INSTANCE_NUM];
} _CODEC_LOCAL_INFO;

