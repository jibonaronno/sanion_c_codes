
/******************************************************************************
 **                     Copyright(C) 2016 SANION Co. Ltd.                    **
 **                  --------------------------------------                  **
 **                              WWW.SANION.COM                              **
 ******************************************************************************
    PROJECT     : UR-IED
 ******************************************************************************
    FILE NAME   : global.h
    AUTHOR      : sootoo23
    DATE        : 2016.11.10
    REVISION    : V1.00
    DESCRIPTION : 공용 구조�?�?변??관�??�더
 ******************************************************************************
    HISTORY     :
        2016-11-10 초안 ?�성
        2017-01-09 MPU-DSP 구조�?Define
 ******************************************************************************/

#ifndef GLOBAL_H_
#define GLOBAL_H_

#include "Common.h"
#include "UR_SDK.h"

/******************************************************************************
								Common Define
******************************************************************************/

// #define M_PI 3.141592

#define CPP_BLOCK_ENABLED

#define CON_PASSCODE 2000

typedef enum {
    DBG_MODE_USER,
	DBG_MODE_OPERATOR,
    DBG_MODE_SUPERVISOR,
    DBG_MODE_DEVELOPER
}DBG_MODE_TYPE;
	
typedef struct {
	U64  m_u64TimeSec;       // timeval 구조체의 tv_sec�?
	U64  m_u64TimeUsec;      // timeval 구조체의 tv_usec�?
	U32  m_u32Flag;          // Update Flag
}dsSystemTime;

/******************************************************************************
								IO Define
******************************************************************************/
#define MAX_DI_COUNT	6
#define MAX_DO_COUNT	3

typedef struct {
	U8		u8DIStsVal[MAX_DI_COUNT];
	U8		u8DILockSts;
	U8		u8DOStsVal[MAX_DO_COUNT];
	U8		u8DOFBError;
	U32		u32HFlag;
}dsIOInfo;

typedef struct {
	U32		u8DIDebounceTime[MAX_DI_COUNT];
	U32		u32HFlag;
}dsIOSetting;


typedef struct {
	dsIOInfo		m_dsIOInfo;
	dsIOSetting		m_dsIOSetting;

	U8				u8IOResponseErrFlag;
}dsIOData;

/******************************************************************************
								AI Define
******************************************************************************/
#define DATA_QUEUE_SIZE		(60*128*30)
#define MAX_OCT_COUNT		9
#define MAX_BCT_COUNT		6
#define MAX_PT_COUNT		6
#define MAX_TD_COUNT		8

typedef struct {
	F32		m_f32Mag[MAX_OCT_COUNT];
	F32		m_f32Ang[MAX_OCT_COUNT];
	F32		m_f32Real[MAX_OCT_COUNT];
	F32		m_f32Imag[MAX_OCT_COUNT];
}dsOCTInfo;

typedef struct {
	F32		m_f32Mag[MAX_BCT_COUNT];
	F32		m_f32Ang[MAX_BCT_COUNT];
	F32		m_f32Real[MAX_BCT_COUNT];
	F32		m_f32Imag[MAX_BCT_COUNT];
}dsBCTInfo;

typedef struct {
	F32		m_f32Mag[MAX_PT_COUNT];
	F32		m_f32Ang[MAX_PT_COUNT];
	F32		m_f32Real[MAX_PT_COUNT];
	F32		m_f32Imag[MAX_PT_COUNT];
}dsPTInfo;

typedef struct {
	F32		m_f32Mag[MAX_TD_COUNT];
}dsTDInfo;

typedef struct {
	U32		m_u32TDScaleUse[MAX_TD_COUNT];
	S32		m_s32TDScaleMin[MAX_TD_COUNT];
	S32		m_s32TDScaleMax[MAX_TD_COUNT];
	U32		m_u32TDScaleUnit[MAX_TD_COUNT];
	U32 	m_u32HFlag;
}dsTDScale;

/*******************************************************************************
* ADC AD7770 Calibration Factor Structure
******************************************************************************/
// Command definition of ulCalibrationCMD
//   rsvd [31:5],CTPT[4], ACT[3:2], MODE [1:0]
#define DEFS_CALIBCMD_MODE_IDLE    0b00		// ?��?
#define DEFS_CALIBCMD_MODE_PHASE 0b01		// ?�택??CH, ADC???�?? Phase offset factor�??�택??action??취함.
#define DEFS_CALIBCMD_MODE_GAIN   0b10		// ?�택??CH, ADC???�?? gain factor�??�택??action??취함.
#define DEFS_CALIBCMD_MODE_DCOFFSET 0b11	// ?�택??CH, ADC???�?? DC offset factor�??�택??action??취함.
#define DEFS_CALIBCMD_MODE_MASK 0b11		// Command mode mask

#define DEFS_CALIBCMD_ACT_NONE	0b0000    // 
#define DEFS_CALIBCMD_ACT_AUTO	0b0100    // ?�격?�로 ?�차�?보정.
#define DEFS_CALIBCMD_ACT_USFIX	0b1000    // user??값으�??�팅.
#define DEFS_CALIBCMD_ACT_RESET	0b1100    // 초기??

#define DEFS_CALIBCMD_SEL_OCT	0b000000    //
#define DEFS_CALIBCMD_SEL_BCT	0b010000    //
#define DEFS_CALIBCMD_SEL_PT	0b100000    //
#define DEFS_CALIBCMD_SEL_MASK	0b110000    // Command select mask

typedef struct{
	F32 f32PhaseFactor[3][MAX_OCT_COUNT]; // 0: OCT, 1: BCT, 2: PT
	F32 f32OffsetFactor[3][MAX_OCT_COUNT];
	F32 f32GainFactor[3][MAX_OCT_COUNT];
	F32 fTargetValue;
	U32 u32ChannelSelect;
	U32 u32CalibrationCMD;
	U32 u32HFlag;
} dsMeasCalib;

typedef struct {
	float	fGain[MAX_TD_COUNT];
	float	fOffset[MAX_TD_COUNT];
	float 	fTemp;
	U32 	u32ChannelSelect;
	U32 	u32Mode;
	U32		u32HFlag;
}ds_4_20_TD_CALIB;

typedef struct {
	dsOCTInfo			m_dsOCTDataInfo;
	dsBCTInfo			m_dsBCTDataInfo;
	dsPTInfo			m_dsPTDataInfo;
	dsTDInfo			m_dsTDDataInfo;
	
	dsTDScale			m_dsTDScaleInfo;
	
	dsMeasCalib			m_dsMeasCalibration;
	ds_4_20_TD_CALIB	m_dsTDCalibration;

	float				m_fInternalDCVoltage[8];
}dsAIData;

typedef struct {
	F32 f32OCT1mag[DATA_QUEUE_SIZE];
	F32 f32OCT2mag[DATA_QUEUE_SIZE];
	F32 f32OCT3mag[DATA_QUEUE_SIZE];
	F32 f32OCT4mag[DATA_QUEUE_SIZE];
	F32 f32OCT5mag[DATA_QUEUE_SIZE];
	F32 f32OCT6mag[DATA_QUEUE_SIZE];
	F32 f32OCT7mag[DATA_QUEUE_SIZE];
	F32 f32OCT8mag[DATA_QUEUE_SIZE];
	F32 f32OCT9mag[DATA_QUEUE_SIZE];
	U32 u32Index;
	U32 u32HFlag;
}dsAIDataQueue;

typedef struct _dsPhasorDataInfo
{
    F32 _pt[MAX_PT_COUNT];
    F32 _oct[MAX_OCT_COUNT];
    F32 _bct[MAX_BCT_COUNT];
}dsPhasorDataInfo;

typedef struct {
	U32					m_u32LiveTickCnt[2];
	dsSystemTime		m_dsSysTime;                // System Time	
	
	dsAIData		m_dsMeasValueData;
	dsAIDataQueue	m_dsMeasQueueData;

	dsPhasorDataInfo m_dsPhasorDataInfo;

	U32 m_u32dsSize[2];								// DSP SharedMemory Size Check (0:MPU, 1:DSP1)
}dsAISharedMemory;

/******************************************************************************
								LU App Define
******************************************************************************/
typedef struct {
	U32 DummyData;
} dsLUAppSharedMemory;

/******************************************************************************
								61850 Define
******************************************************************************/
typedef enum
{
	STB_CTRL_TYPE_NONE,
   	STB_CTRL_TYPE_SPC,
   	STB_CTRL_TYPE_DPC,
}DEFS_STB_CTRL_TYPE_ENUM;
	
typedef enum
{
	STB_DIDO_TYPE_NONE,
   	STB_DIDO_TYPE_DI,
   	STB_DIDO_TYPE_DO,
}DEFS_STB_DIDO_TYPE_ENUM;

extern S32	gs32SntpTimeQuality;
extern U8 gu8is61850;
extern U8 gu8changedetected;
extern int* gaps32phydi61850[MAX_DI_CHANNEL];

/*******************************************************************************
								Global variable
******************************************************************************/
#define DDR_AI_MEM_SIZE				0x1400000 // 20MB
#define DDR_DIO_MEM_SIZE			0x1400000 // 20MB
#define DDR_LUAPP_MEM_SIZE			0x1400000 // 20MB

#define DDR_DIO_DATA_OFFSET			DDR_AI_MEM_SIZE 
#define DDR_LU_APP_DATA_OFFSET		(DDR_DIO_DATA_OFFSET + DDR_DIO_MEM_SIZE) 
#define DDR_STBP_DATA_OFFSET		(DDR_LU_APP_DATA_OFFSET + DDR_LUAPP_MEM_SIZE) 

extern U8 g_u8WDTCtrlFlag;
extern U8 g_u8WDTSignalFlag;

// Memory pointers
extern U8 *g_pCMemVAddr;
extern volatile dsIOData				*g_pIOSharedMem;
extern volatile dsAISharedMemory		*g_pAISharedMem; 
extern volatile dsLUAppSharedMemory 	*g_pLUAppSharedMem;
extern volatile ds61850SharedMemory		*g_pSTBSharedMem;

#endif /* GLOBAL_H_ */

