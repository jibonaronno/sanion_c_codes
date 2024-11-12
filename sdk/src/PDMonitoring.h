
#define PDNAME1				"/PDNAME1"
#define PDNAME2				"/PDNAME2"
#define PDNAME3				"/PDNAME3"
#define PDNAME4				"/PDNAME4"
#define PDNAME5				"/PDNAME5"
#define PDNAME6				"/PDNAME6"
#define PDNAME7				"/PDNAME7"
#define PDNAME8				"/PDNAME8"
#define PDNAME9				"/PDNAME9"
#define PDNAME10			"/PDNAME10"
#define PDNAME11			"/PDNAME11"
#define PDNAME12			"/PDNAME12"
#define PDNAME13			"/PDNAME13"
#define PDNAME14			"/PDNAME14"
#define PDNAME15			"/PDNAME15"
#define PDNAME16			"/PDNAME16"
#define PDNAME17			"/PDNAME17"
#define PDNAME18			"/PDNAME18"
#define PDNAME19			"/PDNAME19"
#define PDNAME20			"/PDNAME20"
#define PDNAME21			"/PDNAME21"
#define PDNAME22			"/PDNAME22"
#define PDNAME23			"/PDNAME23"
#define PDNAME24			"/PDNAME24"


typedef struct {
	dsBasicStrForIEC61850	m_dsPDStatus[NUM_KIND_PD_STATUS];
	U32						m_u32PDSetting[NUM_KIND_PD_SETTING];
	U32						m_u32CRC;
} dsPDBufferForIEC61850;

U8 InitPDMonitoring(void);
void TerminatePDMonitoring(void);
void ExecutePDMonitoring(void);

dsTotalDataToCU *mallocPDmemorypart();
U8 InitPDSharedVariables();

