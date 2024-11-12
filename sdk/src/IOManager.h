/******************************************************************************
 **                     Copyright(C) 2016 SANION Co. Ltd.                    **
 **                  --------------------------------------                  **
 **                              WWW.SANION.COM                              **
 ******************************************************************************
    PROJECT     : LU MPU
 ******************************************************************************
    FILE NAME   : IOManager.h
    AUTHOR      : kej
    DATE        : 2021.08.04
    REVISION    : V1.00
    DESCRIPTION : IO Manager Page
 ******************************************************************************/

#ifndef IOMANAGER_H_
#define IOMANAGER_H_

#include "global.h"
#include "UR_SDK.h"

#define CAN_CMD_OFFSET			0x5//0x6
#define CAN_CMD_MASK			0x7E0
#define CAN_ID_MASK				0x1F
#define CAN_BCAST_ID			0xF
//#define CAN_PACKETTYPE_OFFSET 0x5
#define CAN_DATA_PACKET_SIZE	0x7

#define DIPU_DI_MAX				6
#define DOU_DO_MAX				3

#define	IO_VERSION_CANID		((CMD_READ_VERSION			<< CAN_CMD_OFFSET) | CAN_BCAST_ID)
#define IO_BOARD_CHECK_CANID	((CMD_READ_BOARD_CHECK		<< CAN_CMD_OFFSET) | CAN_BCAST_ID)
#define IO_DIO_INFOMAION_CANID	((CMD_READ_DIO_INFORMATION	<< CAN_CMD_OFFSET) | CAN_BCAST_ID)
#define IO_DO_CONTROL_CANID 	((CMD_WRITE_DO_CONTROL		<< CAN_CMD_OFFSET) | CAN_BCAST_ID)

typedef enum
{
	CMD_READ_VERSION = 0x0,
	CMD_READ_BOARD_CHECK,
	CMD_READ_BOARD_SET_REQ,
	CMD_READ_DIO_INFORMATION,
	CMD_WRITE_DO_CONTROL,
	CMD_WRITE_DIO_ASSIGN,
	CMD_WRITE_DILOCK_RELEASE,
	CMD_WRITE_DI_DEBOUNCE,
	CMD_WRITE_DEVICE_STOP,
	CMD_READ_DEBOUNCE_INFO,
	CMD_WRITE_UPGRADE_START = 0x10,
	CMD_WRITE_UPGRADE_SIZE,
	CMD_WRITE_UPGARDE_DOWN_START,
	CMD_WRITE_UPGRADE_DOWN_FINISH,
	CMD_WRITE_UPGRADE_DOWN_STOP,
	CMD_WRITE_UPGRADE_RESULT,
	CMD_READ_POWER_OFF_EVENT,
	CMD_READ_CAN_COMM_FAIL_EVENT,
	CMD_READ_EVENT  = 0x1F,
	CMD_LAST = CMD_READ_EVENT
}UR_CAN_CMD_TYPE;

typedef struct {
	int s32BoardCANID;
	U8 CheckFalg;
}dsCAN_GET_BOARD_INFO;

typedef struct {
	int s32DIStartIdx;
	int s32DOStartIdx;
	int s32DIOBoardCnt;
}dsCAN_BOARD_TX_INFO;

extern dsCAN_GET_BOARD_INFO g_dsGetBoardInfo;
extern dsCAN_BOARD_TX_INFO	g_dsCanBoardTxInfo;

typedef enum
{
	DATA_SEQ = 0,
	DATA_DATA1,
	DATA_DATA2,
	DATA_DATA3,
	DATA_DATA4,
	DATA_DATA5,
	DATA_DATA6,
	DATA_DATA7,
}UR_CAN_DATA_PACKET;


typedef enum
{
	BD_MDCU = 1,
	BD_DI_1 = 0x11,
	BD_DI_2,
	BD_DI_3,
	BD_DI_4,
	BD_DI_5,
	BD_DIOU_1,
	BD_DIOU_2,
	BD_DIOU_3,
	BD_DIOU_4,
	BD_DIOU_5,
	BD_DIOU_6,
	BD_RTDTD_1,
	BD_RTDTD_2,
	BD_RTD,
	BD_IED_DOPU,
	BD_IOBOX_DOPU
}UR_CAN_IOBOARD;

typedef enum
{
	UPDAT_READY = 0,
	UPDAT_DOWNLOAD_READY,
	UPDAT_DOWNLOAD_FIN,
	UPDAT_RESULT
}UR_IO_UPDAT_TYPE;

#pragma pack (push, 1)
typedef struct {
	U8 m_u8DIInfo; 		//max 6
	U8 m_u8DOInfo;      //max 3
}dsCanDIOInfo;

#pragma pack (pop)

typedef struct{
	S32 s32LiveID;
	S32	s32InitID;
	S32	s32RetryID;

	S32	s32ErrID;
	U8	u8ErrIDCnt;
	U8	u8IOLiveErrCnt;

	U8	u8IOTotalErrFlag;
	U8	u8IOLiveWaitCnt;
	U8	u8IOLiveRetryFlag;
	U8	u8IOLiveRetryIdx;
	
}dsIOLiveCheck;

typedef struct{
	U8  m_u8DIDebounceTm[MAX_DI_CHANNEL];
}dsIORealValue;

extern dsIOLiveCheck g_dsIOLiveCheck;

/******************************************************************************************************
 IO Manager Func
******************************************************************************************************/

void* IOM_IOBdUpdate(int argc, void *argv[]);

U32 IOM_GetFileSize(const U8 *FileName);
void IOM_GetFileData(const U8 *FileName, U8 *u8DataBuf, U32 u32DataSize);

void* IOM_ProcessMain(void *params);
void* Watchdog();

S32 IOM_Init();

void IOM_IOTimeInfoTx(void);
void IOM_DI_Unlock_Tx(void);

void *IOM_DispDboTm(int argc, void *argv[]);
void *IOM_CLISetDebounceTm(int argc, void* argv[]);
void *IOM_DOControl(int argc, void* argv[]);

void IOM_IOBdLiveCheck(int Interval, int CheckTm);


//extern dsCtrlInfo* g_dsIOCtrlInfo;
extern U32 UseRTDPoint;
extern U32 UseTDPoint;
extern U32 UseDIPoint;
extern U32 UseDOPoint;
extern U32 g_u32TotIOBdCnt;

#endif /* IOMANAGER_H_ */


