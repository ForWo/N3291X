#include <stdio.h>
#include "stdlib.h"
#include <string.h>
#include "wbtypes.h"
#include "wblib.h"
#include "W55FA95_reg.h"

//define in UsbOHCI.h
#define OHCI_INTR_SO    (1 << 0)        /* scheduling overrun */
#define OHCI_INTR_WDH   (1 << 1)        /* writeback of done_head */
#define OHCI_INTR_SF    (1 << 2)        /* start frame */
#define OHCI_INTR_RD    (1 << 3)        /* resume detect */
#define OHCI_INTR_UE    (1 << 4)        /* unrecoverable error */
#define OHCI_INTR_FNO   (1 << 5)        /* frame number overflow */
#define OHCI_INTR_RHSC  (1 << 6)        /* root hub status change */
#define OHCI_INTR_OC    (1 << 30)       /* ownership change */
#define OHCI_INTR_MIE   0x80000000      /* master interrupt enable */

/* roothub.portstatus [i] bits */
#define RH_PS_CCS           0x00000001         	/* current connect status */
#define RH_PS_PES           0x00000002         	/* port enable status*/
#define RH_PS_PSS           0x00000004         	/* port suspend status */
#define RH_PS_POCI          0x00000008         	/* port over current indicator */
#define RH_PS_PRS           0x00000010         	/* port reset status */
#define RH_PS_PPS           0x00000100         	/* port power status */
#define RH_PS_LSDA          0x00000200         	/* low speed device attached */
#define RH_PS_CSC           0x00010000         	/* connect status change */
#define RH_PS_PESC          0x00020000         	/* port enable status change */
#define RH_PS_PSSC          0x00040000         	/* port suspend status change */
#define RH_PS_OCIC          0x00080000         	/* over current indicator change */
#define RH_PS_PRSC          0x00100000         	/* port reset status change */

/* roothub.status bits */
#define RH_HS_LPS           0x00000001         	/* local power status */
#define RH_HS_OCI           0x00000002         	/* over current indicator */
#define RH_HS_DRWE          0x00008000         	/* device remote wakeup enable */
#define RH_HS_LPSC          0x00010000         	/* local power status change */
#define RH_HS_OCIC          0x00020000         	/* over current indicator change */
#define RH_HS_CRWE          0x80000000         	/* clear remote wakeup enable */

/* roothub.b masks */
#define RH_B_DR         	0x0000FFFF			/* device removable flags */
#define RH_B_PPCM       	0xFFFF0000			/* port power control mask */

/* roothub.a masks */
#define RH_A_NDP        	(0xFF)				/* number of downstream ports */
#define RH_A_PSM        	(1 << 8)			/* power switching mode */
#define RH_A_NPS        	(1 << 9)			/* no power switching */
#define RH_A_DT         	(1 << 10)			/* device type (mbz) */
#define RH_A_OCPM       	(1 << 11)			/* over current protection mode */
#define RH_A_NOCP       	(1 << 12)			/* no over current protection */
#define RH_A_POTPGT     	(0xFF000000)		/* power on to power good time */

#undef min
#define min(a,b) (((a)<(b))?(a):(b))  

extern int  get_sof_int_flag(void);
extern void set_sof_int_flag(int);
extern int  get_write_done_head_flag(void);
extern void set_write_done_head_flag(int);
extern int  get_FNO_int_flag(void);
extern void set_FNO_int_flag(int);
extern int  get_schedule_overrun_int_flag(void);
extern void set_schedule_overrun_int_flag(int);
extern int  get_port_status_change_int_flag(void);
extern void set_port_status_change_int_flag(int);
extern int  get_resume_detect_int_flag(void);
extern void set_resume_detect_int_flag(int);
extern int  get_hcca_frame_number(void); 


extern int usbh_file_test(UINT32 loop);
extern int test_hid(void);
extern int usbh_test_w99683(int packet_size);
extern int usbh_test_w99683_overnight(int packet_size);
extern int usbh_integration_test(void);


typedef struct tagUSBH_REG
{
	UINT32		addr;
	UINT32		value;
	const char	*name;	
}reg_default;


static reg_default usbh_reg_default[] =
{
	{ REG_HC_REVISION, 		0x110,	"HC_REVISION" 		},
	{ REG_HC_CONTROL,		0x0,	"HC_CONTROL"		},
	{ REG_HC_CMD_STATUS,	0x0,	"HC_CMD_STATUS"		},
	{ REG_HC_INT_STATUS,	0x0,	"HC_INT_STATUS"		},
	{ REG_HC_INT_ENABLE,	0x0,	"HC_INT_ENABLE"		},
	{ REG_HC_INT_DISABLE,	0x0,	"HC_INT_DISABLE"	},
	{ REG_HC_HCCA,			0x0,	"HC_HCCA"			},
	{ REG_HC_CTRL_HEADED,	0x0,	"HC_CTRL_HEADED"	},
	{ REG_HC_CTRL_CURED,	0x0,	"HC_CTRL_CURED"		},
	{ REG_HC_BULK_HEADED,	0x0,	"HC_BULK_HEADED"	},
	{ REG_HC_DONE_HEAD,		0x0,	"HC_DONE_HEAD"		},
	{ REG_HC_FM_INTERVAL,	0x2EDF,	"HC_FM_INTERVAL"	},
//	{ REG_HC_FM_REMAINING,	0x0,	"HC_FM_REMAINING"	},
	{ REG_HC_FM_NUMBER,		0x0,	"HC_FM_NUMBER"		},
	{ REG_HC_PERIOD_START,	0x0,	"HC_PERIOD_START"	},
	{ REG_HC_LS_THRESHOLD,	0x628,	"HC_LS_THRESHOLD"	},
	{ REG_HC_RH_DESCRIPTORA,0x1000102,"HC_RH_DESCRIPTORA"},
	{ REG_HC_RH_DESCRIPTORB,0x0,	"HC_RH_DESCRIPTORB"	},
	{ REG_HC_RH_STATUS,		0x0,	"HC_RH_STATUS"		},
	{ REG_HC_RH_PORT_STATUS1,0x0,	"HC_RH_PORT_STATUS1"},
	{ REG_HC_RH_PORT_STATUS2,0x0,	"HC_RH_PORT_STATUS2"}
};

int  USBH_CheckRegsiters(void)
{
	int  idx;
	int  status = 0;
	
	sysprintf("<<< Check register default value >>> test ...\n");

	for (idx = 0; idx < sizeof(usbh_reg_default) / sizeof(reg_default); idx++)
	{
		if (inp32(usbh_reg_default[idx].addr) != usbh_reg_default[idx].value)
		{
			sysprintf("Register %s value %x mismatch, should be %x\n", 
					usbh_reg_default[idx].name, inp32(usbh_reg_default[idx].addr), usbh_reg_default[idx].value);
			status = -1;
		}
	}

	if (status == 0)
		sysprintf("Test passed.\n");
	else
		sysprintf("Test failed!!\n");
	
	return status;
}

int  USBH_CheckFrameCounting(void)
{
	UINT32  frame_number;
	int     i, k;
	int		status = 0;

	sysprintf("<<< Frame counting >>> test ...\n");
	
	InitUsbSystem(); 
	
	frame_number = inp32(REG_HC_FM_NUMBER);
	
	for (i = 0; i < 10; i++)
	{
		frame_number = inp32(REG_HC_FM_NUMBER);
		sysprintf("USB frame number: %d\n", frame_number);
		for (k = 0; k < 0x100000; k++)
		{
			if (frame_number != inp32(REG_HC_FM_NUMBER))
				break;
		}
		if (k >= 0x100000)
		{
			sysprintf("USB frame number is still %d. Not counting.\n", inp32(REG_HC_FM_NUMBER));
			status = -1;
			break;
		}
	}
	
	if (status == 0)
		sysprintf("Test passed.\n");
	else
		sysprintf("Test failed!!\n");
	
	DeInitUsbSystem();

	return status;
}

int USBH_CheckFrameNumberOverflow(void)
{
	volatile UINT32  frame_number, last_frame_number = 0;
	int     k;
	int     status = 0;

	sysprintf("<<< Frame number overflow >>> test ...\n");
	
	InitUsbSystem(); 

	frame_number = inp32(REG_HC_FM_NUMBER);
	
	for (k = 0; k < 0x100000; k++)
	{
		if (frame_number != inp32(REG_HC_FM_NUMBER))
			break;
	}
	if (k >= 0x100000)
	{
		sysprintf("USB frame number is not counting.\n");
		status = -1;
	}
	
	set_FNO_int_flag(0);
	
	outp32(REG_HC_INT_ENABLE, OHCI_INTR_FNO);
	
	while (status == 0)
	{
		frame_number = inp32(REG_HC_FM_NUMBER);
		sysprintf("USB frame number: %d\n", frame_number);
		
		if (frame_number < last_frame_number)
			break;
		
		if (frame_number > 0x10000)
		{
			sysprintf("Fame number over spec.!!\n");
			status = -1;
			break;
		}
		last_frame_number = frame_number;
	}
	
	if (get_FNO_int_flag() == 0)
	{
		sysprintf("Frame overflow, but interrupt was not set!!\n");
		status = -1;
	}
	
	if (status == 0)
		sysprintf("Test passed.\n");
	else
		sysprintf("Test failed!!\n");
	
	DeInitUsbSystem();

	return status;
}




int USBH_SetGlobalPortPower(void)
{
	volatile UINT32  timeout = 0x100000;
	int   status = 0;
	
	sysprintf("<<< Set global port power >>> test ...\n");
	sysprintf("Please plug any one full/high speed device to run this test.\n");

	InitUsbSystem(); 

	// clear global port power
    outp32(REG_HC_RH_STATUS, 0x1);
    
    while (timeout-- > 0)
    {
    	if (!(inp32(REG_HC_RH_PORT_STATUS1) & 0x100))
    		break;
    }
    
    if (timeout <= 0)
    {
    	sysprintf("RH_PS_PPS was not clear!!\n");
    	sysprintf("Test failed!!\n");
    	status = -1; 
    }
	
	// set global port power
    outp32(REG_HC_RH_STATUS, 0x10000);
    
    while (timeout-- > 0)
    {
    	if (inp32(REG_HC_RH_PORT_STATUS1) & RH_PS_PPS)
    		break;
    }
    
    if (timeout <= 0)
    {
    	sysprintf("RH_PS_PPS was not set!!\n");
    	sysprintf("Test failed!!\n");
    	status = -1; 
    }

	timeout = 0x800000;
	while (1)
	{
   		if ((inp32(REG_HC_RH_STATUS) == 0) && 
			(inp32(REG_HC_RH_PORT_STATUS1) == 0x10101))
	 		break;
	 		
	 	if (timeout-- == 0)
		{
    		sysprintf("RH_PS_CSC or RH_PS_CCS were not set!!\n");
    		sysprintf("Test failed!!\n");
    		status = -1;
    	}
    }

    outp32(REG_HC_RH_PORT_STATUS1, 0x10000);

	if (inp32(REG_HC_RH_PORT_STATUS1) != 0x101)
	{
	 	sysprintf("RH_PS_CSC cannot be cleared!\n");
    	sysprintf("Test failed!!\n");
    	status = -1;
    }
    
	if (status == 0)
		sysprintf("Test passed.\n");
	else
		sysprintf("Test failed!!\n");
    
	DeInitUsbSystem();
	return status;
}



int  USBH_ClearGlobalPortPower(void)
{
	volatile INT  timeout = 0x100000;
	int   status = 0;

	sysprintf("<<< Clear global port power >>> test ...\n");

	InitUsbSystem();

	// clear global port power
    outp32(REG_HC_RH_STATUS, 0x1);
    
    while (timeout-- > 0)
    {
    	if (!(inp32(REG_HC_RH_PORT_STATUS1) & 0x100))
    		break;
    }
    
    if (timeout <= 0)
    {
    	sysprintf("RH_PS_PPS was not clear!!\n");
    	sysprintf("Test failed!!\n");
    	status = -1; 
    }
    		
   	if ((inp32(REG_HC_RH_STATUS) != 0) || 
	 	(inp32(REG_HC_RH_PORT_STATUS1) != 0x0))
	{
		sysprintf("HC_RH_STATUS or HC_RH_PORT_STATUS1 were not 0x0\n");
    	status = -1;
    }

	if (status == 0)
		sysprintf("Test passed.\n");
	else
		sysprintf("Test failed!!\n");
    
	DeInitUsbSystem();

	return status;
}



int  USBH_SetPortReset(void)
{
	volatile INT  i;
	int    	status = 0;

	sysprintf("<<< Set port reset >>> test ...\n");
	sysprintf("Please plug any one full/high speed device to run this test.\n");

	InitUsbSystem();
	
    outp32(REG_HC_RH_PORT_STATUS1, 0x10);

	for (i = 0; i < 0x800000; i++)
    {
		if ((inp32(REG_HC_RH_STATUS) == 0) && 
	 		(inp32(REG_HC_RH_PORT_STATUS1) == 0x100103))
    		break;;
    }
    if (i >= 0x800000)
    {
		sysprintf("RH_PS_PRSC not set or port enable not success!!\n"); 	
	 	status = -1;
	}

    outp32(REG_HC_RH_PORT_STATUS1, 0x100000);
	
	if ((inp32(REG_HC_RH_STATUS) != 0) || 
	 	(inp32(REG_HC_RH_PORT_STATUS1) != 0x103))
	{	 	
		sysprintf("RH_PS_PRSC cannot be be cleared!!\n"); 	
	 	status = -1;
	}	 	

	if (status == 0)
		sysprintf("Test passed.\n");
	else
		sysprintf("Test failed!!\n");
    
	DeInitUsbSystem();

	return status;
}


int  USBH_PortSuspend(void)
{
	volatile INT  i;
	int    	status = 0;

	sysprintf("<<< Set port suspend >>> test ...\n");
	sysprintf("Please plug any one full/high speed device to run this test.\n");

	InitUsbSystem();

    outp32(REG_HC_RH_PORT_STATUS1, 0x4);

	for (i = 0; i < 0x100000; i++)
	{
		if (inp32(REG_HC_RH_PORT_STATUS1) & 0x4)
	 		break;
	}

    if (i >= 0x100000)
    {
		sysprintf("RH_PS_PSS not set!!\n"); 	
	 	status = -1;
	}

    outp32(REG_HC_RH_PORT_STATUS1, 0x8);

	for (i = 0; i < 0x100000; i++)
	{
		if (!(inp32(REG_HC_RH_PORT_STATUS1) & 0x4))
	 	{
	 		outp32(REG_HC_RH_PORT_STATUS1, 0x40000);
	 		break;
	 	}
	}

    if (i >= 0x100000)
    {
		sysprintf("RH_PS_PSS not cleared!!\n"); 	
	 	status = -1;
	}

	if (status == 0)
		sysprintf("Test passed.\n");
	else
		sysprintf("Test failed!!\n");
    
	DeInitUsbSystem();
	
	return status;
}


int  USBH_ConnectDisconnect(void)
{
	sysprintf("<<< Connect/Disconnect >>> test ...\n");
	sysprintf("Please build USBCoreH library with USB_DEBUG enabled to do this test.\n");

	InitUsbSystem();

	while (1)
		Hub_CheckIrqEvent();
	DeInitUsbSystem();	
}


int USBH_HccaFrameNumber(void)
{
	int  test_loop, status = 0;
	int  frame_number;
	volatile int  i; 
	
	sysprintf("<<< HCCA frame number >>> test ...\n");

	InitUsbSystem();

	for (test_loop = 0; test_loop < 20; test_loop++)
	{
		frame_number = get_hcca_frame_number();
		sysprintf("HCCA frame number: %d\n", frame_number);
		
		for (i = 0; i < 0x1000000; i++)
		{	
			if (frame_number != get_hcca_frame_number())
				break;
		}
		
		if (i >= 0x1000000)
		{
			status = -1;
			break;
		}
	}

	if (status == 0)
		sysprintf("Test passed.\n");
	else
		sysprintf("Test failed!!\n");
    
	DeInitUsbSystem();

	return status;
}

int USBH_InterruptSOF(void)
{
	int  status = 0;
	volatile int  i; 
	
	sysprintf("<<< SOF interrupt >>> test ...\n");
	sysprintf("Please plug any one full/high speed device to run this test.\n");

	set_sof_int_flag(0);

	InitUsbSystem();
	
	outp32(REG_HC_INT_ENABLE, OHCI_INTR_SF);
		
	for (i = 0; i < 0x80000; i++)
	{	
		if (get_sof_int_flag() != 0)
			break;
	}
		
	if (i >= 0x80000)
		status = -1;

	if (status == 0)
		sysprintf("Test passed.\n");
	else
		sysprintf("Test failed!!\n");
    
	DeInitUsbSystem();

	return status;
}

int USBH_InterruptRootHubStatusChange(void)
{
	int  test_loop, status = 0;
	volatile int  i; 
	
	sysprintf("<<< Root hub port status change interrupt >>> test ...\n");

	InitUsbSystem();
	
	sysprintf("Pease keep plug/unplug device to do test...\n");

	for (test_loop = 0; test_loop < 6; test_loop++)
	{
		set_port_status_change_int_flag(0);
		outp32(REG_HC_INT_ENABLE, OHCI_INTR_RHSC);
		
		for (i = 0; i < 0x800000; i++)
		{	
			if (get_port_status_change_int_flag() != 0)
				break;
		}
		
		if (i >= 0x800000)
		{
			status = -1;
			break;
		}
	}

	if (status == 0)
		sysprintf("Test passed.\n");
	else
		sysprintf("Test failed!!\n");
    
	DeInitUsbSystem();

	return status;
}

int USBH_InterruptResumeDetected(void)
{
	int  status = 0;
	volatile int  i; 
	
	sysprintf("<<< Resume detect interrupt >>> test ...\n");

	InitUsbSystem();

	set_resume_detect_int_flag(0);
	
	outp32(REG_HC_INT_ENABLE, OHCI_INTR_RD);

	// SetRemoteWakeupEnable 
	outp32(REG_HC_RH_STATUS, inp32(REG_HC_RH_STATUS) | 0x8000);

	// Let OHCI enter suspend state
    	outp32(REG_HC_CONTROL, (inp32(REG_HC_CONTROL) & 0xffffff3f) | 0xc0);

	sysprintf("Now, please generate a remote-wakeup event by plug/unplug device!!\n");
	
	for (i = 0; i < 0x1000000; i++)
	{
		if (get_resume_detect_int_flag())
			break;
	}

	if (i >= 0x1000000)
	{
		sysprintf("Resume detect interrupt not set!!\n");
		status = -1;
	}

	if (status == 0)
		sysprintf("Test passed.\n");
	else
		sysprintf("Test failed!!\n");
    
	DeInitUsbSystem();
	return status;
}
/*
int USBH_InterruptScheduleOverrun(void)
{
	outp32(REG_HC_INT_ENABLE, OHCI_INTR_SO);
	return usbh_test_w99683(1022);
}
*/