#if 0
#define LCM_WIDTH 		720
#define LCM_HEIGHT 		480
#else
#define LCM_WIDTH 		800
#define LCM_HEIGHT 		480
#endif

#define	ROTATE_DEGREE					90		// option: "0, 90, 180, 270" for Test C&M Rotate On-the-fly with VPE
#define	MIRROR_DIRECTION				0		// option : 0(no Mirror), 1(Horizontal Mirror), 2(Vertical Mirror), 3 (Horizon & Vertical Mirror)

#define CACHE_BIT31         0x80000000


#define	INPUT_PATTERN_FOLDER	"C:\\h264\\pattern\\"

#define ENABLECACHE 0x00000000

#define STREAM_END_SIZE			( 0 )

//---------------------Define for test------------------------------------------
#define	ADDR_BIT_STREAM			0x3000000	//0x0200000
#define STREAM_BUF_SIZE         512*1024	//512*1024//48*1024//8192//1024
#define	LARGE_STREAM_BUF_SIZE	0x040000
#define	SMALL_STREAM_BUF_SIZE	0x040000	//0x000400


#define	MULTI_STREAM_BUF_SIZE	1024*10      
#define	ADDR_BIT_STREAM_1		ADDR_BIT_STREAM
#define	ADDR_BIT_STREAM_2		(ADDR_BIT_STREAM_1+ MULTI_STREAM_BUF_SIZE)     
#define	ADDR_BIT_STREAM_3		(ADDR_BIT_STREAM_2+ MULTI_STREAM_BUF_SIZE)     
#define	ADDR_BIT_STREAM_4		(ADDR_BIT_STREAM_3+ MULTI_STREAM_BUF_SIZE)     
#define	STREAM_BUF_SIZE_1		MULTI_STREAM_BUF_SIZE
#define	STREAM_BUF_SIZE_2		MULTI_STREAM_BUF_SIZE
#define	STREAM_BUF_SIZE_3		MULTI_STREAM_BUF_SIZE
#define	STREAM_BUF_SIZE_4		MULTI_STREAM_BUF_SIZE
	
//#define	ADDR_BIT_WORK			0x100000
#define	ADDR_BIT_WORK			0x1C00000
#define ADDR_FRAME_BASE			0x400000
#define ADDR_SLICE_SAVE_BUFFER  0x1000000
#define SLICE_SAVE_SIZE			0xDE800


/*
    I/O routines
*/
#define VPint   			*(unsigned int volatile*)
#define VPshort 			*(unsigned short volatile*)
#define VPchar  			*(unsigned char volatile*)

#define TRUE 1
#define FALSE 0
#define TIMEOUT 500
#define SHORT_TIMEOUT 50

#define DECODE_TO_VPOST     1

#define READ_BITSTREAM	1
#define WRITE_BITSTREAM	0
  
#define VPOSDISPLAYBUFADDR      0x1800000                  

//--------------------------------------------------------------------------------------
//  Video Codec Predefined Buffer address
//--------------------------------------------------------------------------------------
#define STREAM_FILL_SIZE		( 512 * 4 )  //  4 * 1024 | 512 | 512+256( wrap around test )
#define MIXER_WIDTH				160            //720	
#define MAX_NUM_INSTANCE		4
#define MAX_FILE_PATH			256

// Maximum Image size 720P = 1280*720
#define     MaxImageWidth           1280
#define     MaxImageHeight          720

#define		DecWaitLength			0x02000	


#define     START_ADDR              0x300000
#define     YUVBuf_Size             0x97E00     // D1
#define		DecTestOutputAddr		(START_ADDR | ENABLECACHE)
#define		DecTestRefBufAddr		((DecTestOutputAddr + YUVBuf_Size) | ENABLECACHE)
#define		DecTestBitstreamAddr 	((DecTestRefBufAddr + YUVBuf_Size) | ENABLECACHE)
#define     TmpDisplayBufAddr       ((DecTestBitstreamAddr + DecWaitLength) & 0x000fffff) 
#define     DisplayBufAddr          (0x700000 | ENABLECACHE)


//VPU
#define     Uint16          UINT16
#define     Uint8           UINT8
#define     PRJ_CODA_DX_8	0xF306
#define     PRJ_BODA_DX_5B	0xE606
#define     Console_Printf  sysprintf//printf
#define		Console_getchar sysGetChar//getchar
#define MAX_FRAME				(36)     // AVC REF 16, REC 1 , FULL DUP 4	
#define NUM_FRAME_BUF			MAX_FRAME


typedef struct {
	char Name[256];                 // File name
    int OpenFP;
	int TotalFileSize;		        // Total file size
	int UsedOffset;		            // Offset for used data
	int BitstreamBufSize;	        // Current Free buffer size	
} FileOpInfo;

typedef struct {
	int  Index;
	int  AddrY;
	int  AddrCb;
	int  AddrCr;
	int  StrideY;
	int  StrideC;
	
	int  DispY;
	int  DispCb;
	int  DispCr;

	int  MvColBuf;
} FRAME_BUF;


typedef struct {
	int  bitFormat;
	int sorenson;
	int mode264;
	int rotAngle;
	int mirDir;
	int useRot;
	int outNum;
	int prescanEnable;
	int checkeos;
	int mp4DeblkEnable;
	int iframeSearchEnable;
	int chromaInterleave;
} DecConfigParam;


typedef struct {
	int codecMode;
	int numMulti;
	int  multiMode[MAX_NUM_INSTANCE];
    char multiFileName[MAX_NUM_INSTANCE][MAX_FILE_PATH];
	union {
		DecConfigParam decConfig;
	} ConfigParam;
} MultiConfigParam;

extern FRAME_BUF *GetFrameBuffer(int index);
//--------------------------------------------------------------------------------------
//  Emulation used Structure
//--------------------------------------------------------------------------------------

typedef struct FileOperation{
	char Name[256];                 // File name
	FILE *OpenFP;	                // File handle for current opened file
	int TotalFileSize;		        // Total file size
	int UsedOffset;		            // Offset for used data
	int BitstreamBufSize;	        // Current Free buffer size	
} FILEOPERATION;

//--------------------------------------------------------------------------------------
//  Global Variable 
//--------------------------------------------------------------------------------------
extern UINT32 volatile BITPicRunOK,BITBufferEmptyOK;
extern FileOpInfo ReadFile,WriteFile;

//--------------------------------------------------------------------------------------
//  Function Declaration
//--------------------------------------------------------------------------------------
int FileOP(FileOpInfo *OpenFile,int read,char *fileName); 
void H264TestPattern(void);
extern void CheckVersion(void);
void DecoderAPI(DecOpenParam *decOP,FileOpInfo *DecbufInfo,DecConfigParam *decconfig,int vpost);
void FrameBufferInit(int picX, int picY,int StartAddr);
void H264DecodOneFile(char *H264FileName);
extern void CallBackBufferEmpty(void);
extern void CallBackPicRun(void);
extern RetCode WriteBsBufHelper( DecHandle handle, FileOpInfo *pBufInfo, int defaultsize);
extern void vpeConfig(UINT Ysrc,UINT Usrc,INT Vsrc,UINT uDestAddr,int DLeftOffset, int SrcWidth,int SrcHeight,int DestWidth,int DestHeight,int stride);
extern void vpeInit(void);
extern void vpeSetSrcDestOffsetRotate(int SrcLeftOffset, int SrcRightOffset, int DestLeftOffset, int DestRightOffset, int Rotate);


