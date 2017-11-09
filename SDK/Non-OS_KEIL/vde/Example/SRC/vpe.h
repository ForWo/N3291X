/*
#define VPE_BA						0x0D000
#define LCM_BA						0x05000

#define vpeYaddr            (VPE_BA+0x24)
#define vpeUaddr            (VPE_BA+0x28)
#define vpeVaddr            (VPE_BA+0x2c)
#define vpePaddr            (VPE_BA+0x34)

#define vpeDDA              (VPE_BA+0x30)
#define vpePitch            (VPE_BA+0x38)
#define vpeTrigger            (VPE_BA+0x00)
#define vpeStatus            (VPE_BA+0x10)
#define vpeCommand            (VPE_BA+0x20)

#define vpostBuf0Addr       (LCM_BA+0x24)
#define vpostBuf1Addr       (LCM_BA+0x28)


#define SOURCE_PACKET_ADDR					0x600000
#define VPE_USE_START_ADDR					0x100000
#define PLANAR_Y_ADDR						VPE_USE_START_ADDR
*/

typedef struct 
{	
	UINT32 dwAddr;
	UINT32 dwValue;
}T_REG_INFO;
