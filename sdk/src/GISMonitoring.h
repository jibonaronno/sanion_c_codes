
#define ACQ_WAVE_CYCLES			18		// 18cycles

#define ACQNAME1				"/ACQNAME1"
#define ACQNAME2				"/ACQNAME2"
#define ACQNAME3				"/ACQNAME3"
#define ACQNAME4				"/ACQNAME4"
#define ACQNAME5				"/ACQNAME5"
#define ACQNAME6				"/ACQNAME6"
#define ACQNAME7				"/ACQNAME7"
#define ACQNAME8				"/ACQNAME8"
#define ACQNAME9				"/ACQNAME9"


typedef struct {
	dsBasicStrForIEC61850	m_dsACQStatus[NUM_KIND_ACQ_STATUS];
	U32						m_u32ACQSetting[NUM_KIND_ACQ_SETTING];
	U32						m_u32CRC;
} dsACQBufferForIEC61850;
// int ACQcounting[MAX_GIS_ACQS];


// namebufferstruct filenamebuffs[MAX_GIS_ACQS];

typedef struct {
	U8	m_u8EventType;
	U32	m_u32EventTime;
	U32	m_u32EventTimeMsec;

	U16	m_u16TripCoil1Current[(ACQ_WAVE_CYCLES*SAMPLES_PER_CYCLE)];
	U16	m_u16TripCoil2Current[(ACQ_WAVE_CYCLES*SAMPLES_PER_CYCLE)];
	U16	m_u16CloseCoilCurrent[(ACQ_WAVE_CYCLES*SAMPLES_PER_CYCLE)];

	U16	m_u16PhaseACurrent[(ACQ_WAVE_CYCLES*SAMPLES_PER_CYCLE)];
	U16	m_u16PhaseBCurrent[(ACQ_WAVE_CYCLES*SAMPLES_PER_CYCLE)];
	U16	m_u16PhaseCCurrent[(ACQ_WAVE_CYCLES*SAMPLES_PER_CYCLE)];

	U8	m_u8Contacts[(ACQ_WAVE_CYCLES*SAMPLES_PER_CYCLE)];
} dsACQWaveData;

typedef struct {
	dsACQWaveData	m_dsData;
} dsACQWave;


U8 InitPreventionAndMonitoringForGIS(void);
void TerminatePreventionAndMonitoringForGIS(void);
void ExecuteGISMonitoring(void);

dsTotalDataToCU *mallocACQmemorypart();
U8 InitGISSharedVariables();

