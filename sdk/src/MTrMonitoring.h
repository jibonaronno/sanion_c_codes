
#define DGANAME1					"/DGANAME1"
#define DGANAME2					"/DGANAME2"
#define DGANAME3					"/DGANAME3"
#define DGANAME4					"/DGANAME4"
#define DGANAME5					"/DGANAME5"
#define DGANAME6					"/DGANAME6"

#define OLTCNAME1					"/OLTCNAME1"
#define OLTCNAME2					"/OLTCNAME2"
#define OLTCNAME3					"/OLTCNAME3"

#define BSHNAME1					"/BSHNAME1"
#define BSHNAME2					"/BSHNAME2"


#define	MODBUS_TCP_FC_READ_REGISTERS		0x04

#define	MINITRANS_ADDR_OIL_SOURCE_A_MEAS	4100
#define	MINITRANS_LENGTH_OIL_SOURCE_A_MEAS	68


#define DI_OLTC_MOTOR_OP			0				// OLTC 구동 모터의 동작 접점은 DI #1에 입력

#define OLTC_WAVE_LENGTH_IN_SEC		15


typedef struct {
	bool			m_bMotorInOperation;
	bool			m_bMotorOpEventExists;

	U16				m_u16AccumulatedOpCnt;

	dsPhasor		m_dsMotorI[NUM_BASIC_PHASE];
	float			m_fOutsideTemp;

	struct timeval	m_dsMotorStartTime;
	struct timeval	m_dsMotorStopTime;
	U32				m_u32MotorOperationTime;
} dsOLTCMonInfo;

typedef struct {
	U32	m_u32EventTime;
	U8	m_u8AlarmLevel;

	U32	m_u32OpDuration;
	U32	m_u32OpCount;

	U32	m_u32MaxCurrent;
	U32	m_u32AvgCurrent;

	U32	m_u32SamplesPerCycle;
	U8	m_u8SampleDataUnitLength;
	U32	m_u32NumOfSampleData;

	U16	m_u16PhaseACurrent[(OLTC_WAVE_LENGTH_IN_SEC*POWER_FREQ*SAMPLES_PER_CYCLE)];
	U16	m_u16PhaseBCurrent[(OLTC_WAVE_LENGTH_IN_SEC*POWER_FREQ*SAMPLES_PER_CYCLE)];
	U16	m_u16PhaseCCurrent[(OLTC_WAVE_LENGTH_IN_SEC*POWER_FREQ*SAMPLES_PER_CYCLE)];
} dsOLTCWaveData;


typedef struct {
	U32	m_u32StartIndex;
	U32	m_u32StopIndex;
	U32	m_u32NumberOfTotalSamples;
	U32	m_u32OLTCOpDurationCalculatedByIndex;

	dsOLTCWaveData	m_dsData;
} dsOLTCWave;


typedef struct {
	bool		m_bCalResExists;

	dsPhasor	m_dsBusV[NUM_BASIC_PHASE];
	dsPhasor	m_dsBshLeakI[NUM_BASIC_PHASE];
	dsPhasor	m_dsBshLeak3I0;

	float		m_fCapVal[NUM_BASIC_PHASE];
	float		m_fPwrFactor[NUM_BASIC_PHASE];
	float		m_fDisFactor[NUM_BASIC_PHASE];
	float		m_fDisplI[NUM_BASIC_PHASE];
} dsBshMonCalRes;


// 2021. 12. 13. 필요 없는 코드이지만 만약을 위해 남겨 둠
#if (0)
typedef struct {
	int		m_ifd_DGAData[MAX_MTR_TANKS];		// DGA 데이터용 shared memory descriptor
	int		m_ifd_OLTCData[MAX_MTR_OLTCS];		// OLTC 데이터용 shared memory descriptor
	int		m_ifd_BSHData[MAX_MTR_BUSHINGS];	// Bushing 데이터용 shared memory descriptor
} dsShMemFD;
#endif


typedef struct {
	float		m_fTemperature;
	float		m_fLevel;
	float		m_fPressure;

	float		m_fH2Oppm;
	float		m_fH2ppm;
	float		m_fN2ppm;
	float		m_fCOppm;
	float		m_fCO2ppm;
	float		m_fCH4ppm;
	float		m_fC2H2ppm;
	float		m_fC2H4ppm;
	float		m_fC2H6ppm;
	float		m_fO2ppm;
	float		m_fC3H8ppm;
} dsDgaMonCalRes;
// int OLTCcounting[MAX_MTR_OLTCS];

// namebufferstruct MTRfilenamebuffs[MAX_MTR_OLTCS];

// Modbus-TCP 통신용 프레임 구조
typedef struct {
	// Transaction ID
	U16	m_u16TransactionID;
	// Protocol ID
	U16	m_u16ProtocolID;
	// Length
	U16	m_u16Length;
	// Unit ID
	U8	m_u8UnitID;

	// Function code
	U8	m_u8FunctionCode;
	// Start address
	U16	m_u16StartAddr;
	// Read length
	U16	m_u16ReadLength;
} dsModbusTCPReadCommand;


typedef struct {
	dsBasicStrForIEC61850	m_dsDgaStatus[NUM_KIND_DGA_STATUS];
	U32						m_u32DgaSetting[NUM_KIND_DGA_SETTING];
	U32						m_u32CRC;
} dsDgaBufferForIEC61850;


typedef struct {
	dsBasicStrForIEC61850	m_dsOLTCStatus[NUM_KIND_OLTC_STATUS];
	U32						m_u32OLTCSetting[NUM_KIND_OLTC_SETTING];
	U32						m_u32CRC;
} dsOLTCBufferForIEC61850;


typedef struct {
	dsBasicStrForIEC61850	m_dsBSHStatus[NUM_KIND_BSH_STATUS];
	U32						m_u32BSHSetting[NUM_KIND_BSH_SETTING];
	U32						m_u32CRC;
} dsBSHBufferForIEC61850;


U8 InitPreventionAndMonitoringForMTr(void);
void TerminatePreventionAndMonitoringForMTr(void);
void ExecuteMTrMonitoring(void);

dsTotalDataToCU *mallocdgamemorypart();
U8 InitPDASharedVariables();
