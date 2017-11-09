/****************************************************************************
*                                                                           *
* Copyright (c) 2009 Nuvoton Tech. Corp. All rights reserved.               *
*                                                                           *
*****************************************************************************/

/****************************************************************************
* FILENAME
*   nuc930_kpi.h
*
* VERSION
*   1.0
*
* DESCRIPTION
*   KPI library header file
*
* DATA STRUCTURES
*   None
*
* FUNCTIONS
*
* HISTORY
*
* REMARK
*   None
****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wblib.h"
#include "w55fa95_reg.h"

#define KPI_NONBLOCK	0
#define KPI_BLOCK		1

#define KPI_CHECK_COL0	0x01
#define KPI_CHECK_COL1	0x02
#define KPI_CHECK_COL2	0x04
#define KPI_CHECK_COL3	0x08

extern void kpi_init(void);
extern int kpi_open(unsigned int src);
extern void kpi_close(void);
extern int kpi_read(unsigned char mode);
