
#define MAJOR_VERSION_NUM   1
#define MINOR_VERSION_NUM   1

//--- define mode of program data
#define FAT_MODE_SKIP           -1
#define FAT_MODE_IMAGE_NO_MBR   0
#define FAT_MODE_FILE           1
#define FAT_MODE_IMAGE_WITH_MBR 2

#define NVT_SM_INFO_T FMI_SM_INFO_T

/* F/W update information */
typedef struct fw_update_info_t
{
    UINT16  imageNo;
    UINT16  imageFlag;
    UINT16  startBlock;
    UINT16  endBlock;
    UINT32  executeAddr;
    UINT32  fileLen;
    CHAR    imageName[32];
} FW_UPDATE_INFO_T;

typedef struct IBR_boot_struct_t
{
    UINT32  BootCodeMarker;
    UINT32  ExeAddr;
    UINT32  ImageSize;
    UINT32  Reserved;
} IBR_BOOT_STRUCT_T;

typedef struct INI_Info {
    char NandLoader[32];
    char Logo[32];
    char NVTLoader[32];
    int  SystemReservedMegaByte;
    int  NAND1_1_SIZE;
    int  NAND1_1_FAT;
    int  NAND1_2_FAT;
    int  NANDCARD_FAT;
} INI_INFO_T;

/* extern parameters */
extern UINT32 infoBuf;
extern FMI_SM_INFO_T *pNvtSM0;
extern UINT8 *pInfo;
extern INT8 nIsSysImage;

INT nvtSMInit(void);
INT setNandPartition(INT sysArea);
INT nvtSMchip_erase(UINT32 startBlcok, UINT32 endBlock);
INT nvtSMpread(INT PBA, INT page, UINT8 *buff);
INT nvtSMpwrite(INT PBA, INT page, UINT8 *buff);
INT nvtSMblock_erase(INT PBA);
INT CheckBadBlockMark(UINT32 block);

int ProcessINI(char *fileName);
