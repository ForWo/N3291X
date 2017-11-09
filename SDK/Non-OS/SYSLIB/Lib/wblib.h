/***************************************************************************
 *                                                                         *
 * Copyright (c) 2009 Nuvoton Technology. All rights reserved.             *
 *                                                                         *
 ***************************************************************************/
 
/****************************************************************************
 * 
 * FILENAME
 *     WBLIB.h
 *
 * VERSION
 *     1.1
 *
 * DESCRIPTION
 *     The header file of NUC900 system library.
 *
 * DATA STRUCTURES
 *     None
 *
 * FUNCTIONS
 *     None
 *
 * HISTORY
 *     2008-06-26  Ver 1.0 draft by Min-Nan Cheng
 *     2009-02-26  Ver 1.1 Changed for NUC900 MCU
 *
 * REMARK
 *     None
 **************************************************************************/
#ifndef _WBLIB_H
#define _WBLIB_H

#ifdef __cplusplus
extern "C"{
#endif

//#include "NUC930_reg.h"
#include "W55FA95_reg.h"
#include "wberrcode.h"
#include "wbio.h"


/* Error code return value */
#define WB_INVALID_PARITY       	(SYSLIB_ERR_ID +1) 
#define WB_INVALID_DATA_BITS    	(SYSLIB_ERR_ID +2)
#define WB_INVALID_STOP_BITS    	(SYSLIB_ERR_ID +3)
#define WB_INVALID_BAUD         	(SYSLIB_ERR_ID +4)
#define WB_PM_PD_IRQ_Fail			(SYSLIB_ERR_ID +5)
#define WB_PM_Type_Fail			    (SYSLIB_ERR_ID +6)
#define WB_PM_INVALID_IRQ_NUM		(SYSLIB_ERR_ID +7)
#define WB_PM_CACHE_OFF				(SYSLIB_ERR_ID +8)

#define WB_MEM_INVALID_MEM_SIZE		(SYSLIB_ERR_ID +9)
#define WB_INVALID_TIME				(SYSLIB_ERR_ID +10)

//-- function return value
#define	   Successful  0
#define	   Fail        1

#define EXTERNAL_CRYSTAL_CLOCK  12000000

/* Define the vector numbers associated with each interrupt */
typedef enum int_source_e
{
	IRQ_WDT = 1, 
	IRQ_EXTINT0 = 2, 
	IRQ_EXTINT1 = 3, 
	IRQ_EXTINT2 = 4, 
    IRQ_EXTINT3 = 5, 
    IRQ_AES = 5, 
   	IRQ_SPU = 6, 
    IRQ_I2S = 7, 
    IRQ_VPOST = 8, 
    IRQ_VIN = 9, 
    IRQ_OVG = 10, 
    IRQ_GE = 11, 
    IRQ_VPE = 12, 
    IRQ_HUART = 13, 
    IRQ_TMR0 = 14, 
    IRQ_TMR1 = 15, 
    IRQ_UDC = 16, 
    IRQ_SIC = 17, 
    IRQ_UHC = 18, 
    IRQ_EDMA = 19, 
    IRQ_SPIMS0 = 20, 
    IRQ_SPIMS1 = 21, 
    IRQ_ADC = 22, 
    IRQ_RTC = 23, 
    IRQ_UART = 24, 
    IRQ_PWM = 25, 
    IRQ_JPG = 26,  
   	IRQ_VDE = 27, 
    IRQ_KPI = 28, 
    IRQ_TSC	= 29,
    IRQ_I2C = 30, 
    IRQ_PWR = 31	
} INT_SOURCE_E;


typedef enum
{
	WE_GPIO =0x01,
	WE_RTC =0x02,
	WE_SDH =0x04,
	WE_UART =0x08,
	WE_UDC =0x10,
	WE_UHC =0x20,
	WE_ADC =0x40,
	WE_KPI =0x80
}WAKEUP_SOURCE_E;

typedef struct datetime_t
{
	UINT32	year;
	UINT32	mon;
	UINT32	day;
	UINT32	hour;
	UINT32	min;
	UINT32	sec;
} DateTime_T;

/* Define constants for use timer in service parameters.  */
#define TIMER0            0
#define TIMER1            1

#define ONE_SHOT_MODE     0
#define PERIODIC_MODE     1
#define TOGGLE_MODE       2
#define UNINTERRUPT_MODE  3

#define ONE_HALF_SECS     0
#define FIVE_SECS         1
#define TEN_SECS          2
#define TWENTY_SECS       3

/* Define constants for use UART in service parameters.  */
#define WB_UART_0		0


#define WB_DATA_BITS_5    0x00
#define WB_DATA_BITS_6    0x01
#define WB_DATA_BITS_7    0x02
#define WB_DATA_BITS_8    0x03

#define WB_STOP_BITS_1    0x00
#define WB_STOP_BITS_2    0x04


#define WB_PARITY_NONE   0x00
#define WB_PARITY_ODD     0x08
#define WB_PARITY_EVEN    0x18

#define LEVEL_1_BYTE      0x0
#define LEVEL_4_BYTES     0x1
#define LEVEL_8_BYTES     0x2
#define LEVEL_14_BYTES    0x3
#define LEVEL_30_BYTES    0x4
#define LEVEL_46_BYTES    0x5
#define LEVEL_62_BYTES    0x6 

/* Define constants for use AIC in service parameters.  */
#define WB_SWI                     	0
#define WB_D_ABORT              1
#define WB_I_ABORT               2
#define WB_UNDEFINE             3

/* The parameters for sysSetInterruptPriorityLevel() and 
   sysInstallISR() use */
#define FIQ_LEVEL_0                0
#define IRQ_LEVEL_1                1
#define IRQ_LEVEL_2                2
#define IRQ_LEVEL_3                3
#define IRQ_LEVEL_4                4
#define IRQ_LEVEL_5                5
#define IRQ_LEVEL_6                6
#define IRQ_LEVEL_7                7

/* The parameters for sysSetGlobalInterrupt() use */
#define ENABLE_ALL_INTERRUPTS      0
#define DISABLE_ALL_INTERRUPTS     1

/* The parameters for sysSetInterruptType() use */
#define LOW_LEVEL_SENSITIVE        0x00
#define HIGH_LEVEL_SENSITIVE       0x01
#define NEGATIVE_EDGE_TRIGGER   0x02
#define POSITIVE_EDGE_TRIGGER    0x03

/* The parameters for sysSetLocalInterrupt() use */
#define ENABLE_IRQ                 0x7F
#define ENABLE_FIQ                 0xBF
#define ENABLE_FIQ_IRQ          0x3F
#define DISABLE_IRQ                0x80
#define DISABLE_FIQ                0x40
#define DISABLE_FIQ_IRQ         0xC0

/* Define Cache type  */
#define CACHE_WRITE_BACK		0
#define CACHE_WRITE_THROUGH	1
#define CACHE_DISABLE			-1

#define MMU_DIRECT_MAPPING	0
#define MMU_INVERSE_MAPPING	1


#define PD_RAM_BASE		0xFF000000
#define PD_RAM_START		0xFF001000
#define PD_RAM_SIZE		0x2000

/* Define Error Code */
#define E_ERR_CLK			(SYS_BA+0x01)

/* Define system clock come from */
typedef enum 
{
	eSYS_EXT 	= 0,
	eSYS_X32K 	= 1,
	eSYS_APLL  	= 2,
	eSYS_UPLL  	= 3
}E_SYS_SRC_CLK;

/* Define constants for use Cache in service parameters.  */
#define CACHE_4M		2
#define CACHE_8M		3
#define CACHE_16M		4 
#define CACHE_32M		5
#define I_CACHE		6
#define D_CACHE		7
#define I_D_CACHE		8


/* Define UART initialization data structure */
typedef struct UART_INIT_STRUCT
{
	UINT32		uart_no;
    	UINT32		uiFreq;
    	UINT32		uiBaudrate;
    	UINT8		uiDataBits;
    	UINT8		uiStopBits;
    	UINT8		uiParity;
    	UINT8		uiRxTriggerLevel;
}WB_UART_T;

/* Define the constant values of PM */
#define WB_PM_IDLE			1
#define WB_PM_PD			2
#define WB_PM_MIDLE	    	5

/* Define Wake up source */ 
#define	WAKEUP_GPIO 	0
#define	WAKEUP_RTC 		1
#define	WAKEUP_SDH  	2
#define	WAKEUP_UART  	3
#define	WAKEUP_UDC  	4
#define	WAKEUP_UHC  	5
#define	WAKEUP_ADC  	6
#define	WAKEUP_KPI  	7


/* Define system library Timer functions */
UINT32 sysGetTicks (INT32 nTimeNo);
INT32 sysResetTicks (INT32 nTimeNo);
INT32 sysUpdateTickCount(INT32 nTimeNo, 
					UINT32 uCount);
INT32 sysSetTimerReferenceClock (INT32 nTimeNo, 
							UINT32 uClockRate);
INT32 sysStartTimer (INT32 nTimeNo, 
					UINT32 uTicksPerSecond, 
					INT32 nOpMode);
INT32 sysStopTimer (INT32 nTimeNo);
INT32 sysSetTimerEvent(INT32 nTimeNo, 
			UINT32 uTimeTick, 
			PVOID pvFun);
VOID	sysClearTimerEvent(INT32 nTimeNo, 
				UINT32 uTimeEventNo);
void	sysSetLocalTime(DateTime_T ltime);
VOID	sysGetCurrentTime(DateTime_T *curTime);
VOID	sysDelay(UINT32 uTicks);


VOID	sysClearWatchDogTimerCount (void);
VOID	sysClearWatchDogTimerInterruptStatus(void);
VOID	sysDisableWatchDogTimer (void);
VOID	sysDisableWatchDogTimerReset(void);
VOID	sysEnableWatchDogTimer (void);
VOID	sysEnableWatchDogTimerReset(void);

PVOID sysInstallWatchDogTimerISR (INT32 nIntTypeLevel, 
				PVOID pvNewISR);
INT32 sysSetWatchDogTimerInterval (INT32 nWdtInterval);


/* Define system library UART functions */

#define UART_INT_RDA		1
#define UART_INT_RDTO		2
#define UART_INT_NONE		255

typedef void (*PFN_SYS_UART_CALLBACK)(
				UINT8* u8Buf, 	
				UINT32 u32Len);

void 	sysUartPort(UINT32 u32Port);
INT8	sysGetChar (VOID);
INT32	sysInitializeUART (WB_UART_T *uart);
VOID	sysPrintf (PINT8 pcStr,...);
VOID	sysprintf (PINT8 pcStr,...);
VOID	sysPutChar (UINT8 ucCh);
void 	sysUartEnableInt(INT32 eIntType);
void 	sysUartTransfer(char* pu8buf, UINT32 u32Len);

/* Define system library AIC functions */
ERRCODE sysDisableInterrupt (INT_SOURCE_E eIntNo);
ERRCODE sysEnableInterrupt (INT_SOURCE_E eIntNo);
BOOL sysGetIBitState(VOID);
UINT32 sysGetInterruptEnableStatus(VOID);
PVOID sysInstallExceptionHandler (INT32 nExceptType, 
								PVOID pvNewHandler);
PVOID sysInstallFiqHandler (PVOID pvNewISR);
PVOID sysInstallIrqHandler (PVOID pvNewISR);
PVOID sysInstallISR (INT32 nIntTypeLevel, 
				INT_SOURCE_E eIntNo, 
				PVOID pvNewISR);
ERRCODE sysSetGlobalInterrupt (INT32 nIntState);
ERRCODE sysSetInterruptPriorityLevel (INT_SOURCE_E eIntNo, 
							UINT32 uIntLevel);
ERRCODE sysSetInterruptType (INT_SOURCE_E eIntNo, 
						UINT32 uIntSourceType);
ERRCODE sysSetLocalInterrupt (INT32 nIntState);
ERRCODE sysSetAIC2SWMode(VOID);


/* Define system library Cache functions */
VOID	 sysDisableCache(VOID);
INT32 sysEnableCache(UINT32 uCacheOpMode);
VOID	 sysFlushCache(INT32 nCacheType);
BOOL sysGetCacheState(VOID);
INT32 sysGetSdramSizebyMB(VOID);
VOID	sysInvalidCache(VOID);
INT32 sysSetCachePages(UINT32 addr, 
					INT32 size, 
					INT32 cache_mode);


/* Define system clock functions */

/* Define system power management functions */
//VOID sysDisableAllPM_IRQ(VOID);
//INT sysEnablePM_IRQ(INT irq_no);
//INT sysPMStart(INT pd_type);
INT32 sysPowerDown(UINT32 u32WakeUpSrc);

//void sysSetClock(void);
//void sysExternalClock(void);
UINT32 sysGetExternalClock(void);
UINT32 sysSetPllClock(E_SYS_SRC_CLK eSrcClk, 
					UINT32 u32TargetHz);
					
void sysCheckPllConstraint(BOOL bIsCheck);

/* FA95 */
ERRCODE
sysSetSystemClock(E_SYS_SRC_CLK eSrcClk,		// Specified the system clock come from external clock, APLL or UPLL
				UINT32 u32PllHz,				// Specified the APLL/UPLL clock /* Unit HZ */
				UINT32 u32SysHz);				// Specified the system clock /* Unit HZ */
INT32 sysSetCPUClock(UINT32 u32CPUClock);/* Unit HZ */
INT32 sysSetAPBClock(UINT32 u32APBClock);/* Unit HZ */
INT32 sysSetSystemDivider(UINT32 u32Hclk, UINT32 u32SysDiv);
UINT32 sysGetPLLOutputHz(E_SYS_SRC_CLK eSysPll, UINT32 u32FinHz);
UINT32 sysGetSystemClock(void);	/* Unit HZ */
UINT32 sysGetCPUClock(void);	/* Unit HZ */
UINT32 sysGetHCLK1Clock(void);	/* Unit HZ */
UINT32 sysGetAPBClock(void);	/* Unit HZ */

#ifdef __cplusplus
}
#endif

#endif  /* _WBLIB_H */

