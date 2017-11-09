/*************************************************************************
 * Nuvoton Electronics Corporation confidential
 *
 * Copyright (c) 2008 by Nuvoton Electronics Corporation
 * All rights reserved
 *
 * FILENAME
 *     usbbasic.h
 *
 * VERSION
 *     1.0
 *
 * DESCRIPTION
 *     W99683 USBh basic emulation header file
 *
 * HISTORY
 *     2008.06.24       Created
 *
 * REMARK
 *     None
 **************************************************************************/
 
int  USBH_CheckRegsiters(void);
int  USBH_CheckFrameCounting(void);
int  USBH_CheckFrameNumberOverflow(void);
int  USBH_SetPortReset(void);
int  USBH_PortSuspend(void);
int  USBH_ConnectDisconnect(void);
int  USBH_HccaFrameNumber(void);
int  USBH_InterruptSOF(void);
int  USBH_InterruptRootHubStatusChange(void);
int  USBH_InterruptResumeDetected(void);