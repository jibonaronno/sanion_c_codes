/******************************************************************************
 **                     Copyright(C) 2016 SANION Co. Ltd.                    **
 **                  --------------------------------------                  **
 **                              WWW.SANION.COM                              **
 ******************************************************************************
    PROJECT     : LU MPU
 ******************************************************************************
    FILE NAME   : IOManager.c
    AUTHOR      : kej
    DATE        : 2021.08.04
    REVISION    : V1.00
    DESCRIPTION : IO Manager Page
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <sys/mman.h>
#include <ti/cmem.h>

#include "wdt_ds1832.h"
#include "Common.h"
#include "SharedMemMgr.h"
#include "ConsoleManager.h"
#include "VT100.h"
#include "IO_CanSocket.h"
#include "DBManager.h"
#include "main.h"
#include "global.h"
#include "UR_SDK.h"
#include "SystemSettingModule.h"
#include "iec61850_sdk.h"
#include "IOManager.h"
#include "ur_gpio.h"

dsCAN_GET_BOARD_INFO	g_dsGetBoardInfo;
dsCAN_BOARD_TX_INFO		g_dsCanBoardTxInfo;

volatile int IOM_ThreadExit = 1;

U32 g_u32TotIOBdCnt = 0;

// CAN Command 관련 변수 선언.
//U32 LiveCanID = 0, DIOReadCanID = 0, DOCtrlID = 0, DOPulseID = 0;
//U32 IOBoxLiveID = 0;
S8 s8UpdatCheckFlag = -1;
U32 u32MultiPacketSeq = 0;

// IO Board Count 관리를 위한 변수.
U32 UseDIPoint=0, UseDOPoint=0, UseRTDPoint=0, UseTDPoint=0, UseTCSPoint=0;
U32 DIOPUBdCnt = 0;

// IO Board의 Live 상태 감시를 위한 변수.
dsIOLiveCheck g_dsIOLiveCheck;

dsIORealValue g_dsIORealValse;

U32 g_PrevDOCtrlData[3] = {0,0,0};
int g_LiveCheckTmCnt = 0;

U8 g_u8DOControlVal = 0 ;

int compare(const void *a, const void *b)    // 오름차순 비교 함수 구현
{
    int num1 = *(int *)a;    // void 포인터를 int 포인터로 변환한 뒤 역참조하여 값을 가져옴
    int num2 = *(int *)b;    // void 포인터를 int 포인터로 변환한 뒤 역참조하여 값을 가져옴

    if (num1 < num2)    // a가 b보다 작을 때는
        return -1;      // -1 반환
    
    if (num1 > num2)    // a가 b보다 클 때는
        return 1;       // 1 반환
    
    return 0;    // a와 b가 같을 때는 0 반환
}

void IOM_GetFileData(const U8 *FileName, U8 *u8DataBuf, U32 u32DataSize)
{
	FILE *fp=0;
	U8 *u8TmpBuf;
	
	fp = fopen((char *)FileName, "rb");
	if(fp)
	{
		//U8 u8TmpBuf[u32DataSize] ;
		//memset(u8TmpBuf, 0x0, u32DataSize);
		u8TmpBuf = (U8 *)malloc(sizeof(U8)*u32DataSize);
		fread(u8TmpBuf, sizeof(U8), u32DataSize, fp);
		memcpy(u8DataBuf,u8TmpBuf,u32DataSize);
		free(u8TmpBuf);
		fclose(fp);
	}
	else
	{
		printf("[%s] Not Found File %s\n", __func__, FileName);
	}
	return;
}

U32 IOM_GetFileSize(const U8 *FileName)
{
	FILE *fp=0;
	U32 u32DataSize=0;

	fp = fopen((char *)FileName, "rb");
	if(fp)
	{
		//Get File Size
		fseek(fp, 0L, SEEK_END);
		u32DataSize = ftell(fp);
		fseek(fp, 0L, SEEK_SET);
		fclose(fp);
	}
	else
	{
		printf("[%s] Not Found File: %s\n", __func__, FileName);
	}

	//printf("[%s] Data Size: %d\n", __func__, u32DataSize);
	return u32DataSize;
}




S32 IOM_Init()
{
	U32 u32IEDBdCnt=0;
	
	struct can_frame canRxBuf;

	memset(&(g_dsIOLiveCheck),0x0,sizeof(g_dsIOLiveCheck));

	g_dsCanBoardTxInfo.s32DIOBoardCnt = 0;
	g_dsCanBoardTxInfo.s32DIStartIdx = -1;
	g_dsCanBoardTxInfo.s32DOStartIdx = -1;
	
	// CAN Socket init
	IO_CAN_SOCKET_INIT();

	// IED IO Board Init
	if(IO_CAN_TX(0, IO_BOARD_CHECK_CANID, NULL, 0))
	{
		struct timeval timeout;
		int i = 0, ret = -1;
		fd_set set;

		FD_ZERO(&set); /* clear the set */
	  	FD_SET(IO_CAN_GetDevice(0), &set); /* add our file descriptor to the set */

	  	timeout.tv_sec = 0;
		timeout.tv_usec = 500;

		//printf("\n");
		usleep(1000); // 1ms
		
		//printf("[%s] IO Board ID: ", __FUNCTION__);

		//sootoo23-160427: timeout check
		ret = select(IO_CAN_GetDevice(0) + 1, &set, NULL, NULL, &timeout);
		if(ret == -1)
		{
			printf("[%s]IO CAN Select Fail!\n", __func__);
			//return -1;
		}
		else if(ret == 0)
		{
			//printf("[%s]IO Bd [%d] CAN Time out!\n", __func__, i);
			//return -1;
		}
		else
		{
			int j=0;
			int s32NowCMD=0,s32NowCanID=0;
			int s32ChkIdx=0;
							
			IO_CAN_RX(&canRxBuf);

			s32NowCMD	= (canRxBuf.can_id & CAN_CMD_MASK) >> CAN_CMD_OFFSET;
			s32NowCanID	= (canRxBuf.can_id & CAN_ID_MASK);
			
			if(s32NowCMD == CMD_READ_BOARD_CHECK)
			{
				g_dsGetBoardInfo.s32BoardCANID	= s32NowCanID;
				g_dsCanBoardTxInfo.s32DIOBoardCnt++;
				g_u32TotIOBdCnt++;
			}

			g_pIOSharedMem->u8IOResponseErrFlag = FALSE;
		}

		//printf("\n[%s] IED Board Count: %d \n", __FUNCTION__, u32IEDBdCnt);
	}

	if(g_u32TotIOBdCnt > 0)
	{
		int i=0;
		char DIPortCnt = 0, DOPortCnt = 0;
		char RTDPortCnt = 0, TDPortCnt = 0;
		char TxBuf[2] = {0,}; //IO Start Pos Number Buf
		int s32Tx_CANID=0;

		g_dsIOLiveCheck.s32LiveID = g_dsGetBoardInfo.s32BoardCANID;
		g_dsIOLiveCheck.s32InitID = g_dsGetBoardInfo.s32BoardCANID;

		s32Tx_CANID = ((CMD_WRITE_DIO_ASSIGN << CAN_CMD_OFFSET) | g_dsGetBoardInfo.s32BoardCANID);
		TxBuf[0] = 0;
		TxBuf[1] = 0;
				
		IO_CAN_TX(0, s32Tx_CANID, TxBuf, sizeof(TxBuf));

		UseDIPoint  = 6;
		UseDOPoint  = 3;

		//IOM_IOTimeInfoTx();
		
		IO_CAN_TX(0, IO_VERSION_CANID, NULL, 0);
		
		IOM_ThreadExit = 0; //IOM Thread Start!
		
		return 0;
	}

	return -1;
}

void IOM_IOBdLiveIDAdd(int CanID)
{
	int i=0, j=0;
	int emptyIdx = -1;
	
	if(g_dsIOLiveCheck.s32LiveID == 0)
		emptyIdx = i;

	if(g_dsIOLiveCheck.s32LiveID == CanID)
		return;

	g_dsIOLiveCheck.s32LiveID = CanID;
}


void IOM_IOBdLiveCheck(int Interval, int CheckTm)
{	
	if(g_u32TotIOBdCnt <= 0)
		return;
	
	int i = 0, j = 0;
	int s32RetryCheckEndFlag = FALSE;
	U8 u8Tmbuf[8] = {0,};
	U8 u8LiveErrChkFlag = 0;
	
	if(g_LiveCheckTmCnt >= CheckTm)
	{
		g_LiveCheckTmCnt = 0;

		if(s8UpdatCheckFlag >= UPDAT_READY && s8UpdatCheckFlag < UPDAT_RESULT)
		{
			//만약, IO보드가 Update중이라면.. Sync data를 요청하지 않는다.
			return;
		}
		else
		{
			U8 TxBuf[8] = {0,};
			u8LiveErrChkFlag = FALSE;
			
			if(g_dsIOLiveCheck.s32LiveID != g_dsIOLiveCheck.s32InitID)
			{
				g_dsIOLiveCheck.u8IOLiveErrCnt++;
				u8LiveErrChkFlag = TRUE; 
			}

			if(u8LiveErrChkFlag == FALSE)
			{
				// 해제 이벤트 처리
				if(g_dsIOLiveCheck.u8IOTotalErrFlag == 0xFF)
				{
					g_pIOSharedMem->u8IOResponseErrFlag = FALSE;
					
					//ERM_EventDataWrite(COMMEVT_BOARD_RESPONSE_FAILED_RELEASED,g_dsIOLiveCheck.s32ErrID[i],0);
	
					g_dsIOLiveCheck.s32ErrID = 0;
					g_dsIOLiveCheck.u8IOLiveErrCnt = 0;
					g_dsIOLiveCheck.u8IOTotalErrFlag = 0X0;
					g_dsIOLiveCheck.u8ErrIDCnt = 0;
				}
			}
			else
			{
				// 발생 이벤ㅂ트 관련 처리
				if(g_dsIOLiveCheck.u8IOTotalErrFlag != 0xFF)
				{
					if((g_dsIOLiveCheck.u8IOTotalErrFlag != 0xFF) && (g_dsIOLiveCheck.u8IOLiveErrCnt >= 3))
					{
						g_pIOSharedMem->u8IOResponseErrFlag = TRUE;
						
						g_dsIOLiveCheck.s32ErrID = g_dsIOLiveCheck.s32InitID;
						g_dsIOLiveCheck.u8ErrIDCnt++;
						g_dsIOLiveCheck.u8IOTotalErrFlag = 0xFF;
						
						//ERM_EventDataWrite(COMMEVT_BOARD_RESPONSE_FAILED_OCCURRED,g_dsIOLiveCheck.s32InitID[i],0);
					}
				}

				// 복구 시도 처리
				g_dsIOLiveCheck.u8IOLiveWaitCnt++;
				if(g_dsIOLiveCheck.u8IOLiveWaitCnt >= 5)
				{
					g_dsIOLiveCheck.u8IOLiveWaitCnt = 0;
					if(g_dsIOLiveCheck.u8IOLiveRetryFlag == FALSE)
					{
						IO_CAN_TX(0, IO_BOARD_CHECK_CANID, (char *)u8Tmbuf, sizeof(u8Tmbuf));
		
						memset(&g_dsIOLiveCheck.s32RetryID, 0x0, sizeof(g_dsIOLiveCheck.s32RetryID));
						g_dsIOLiveCheck.u8IOLiveRetryIdx = 0;
						g_dsIOLiveCheck.u8IOLiveRetryFlag = TRUE;
						return;
					}
					else
					{
						if(g_dsIOLiveCheck.s32RetryID != g_dsIOLiveCheck.s32InitID)
						{
							s32RetryCheckEndFlag = TRUE;
						}
						
						if(s32RetryCheckEndFlag == FALSE)
						{
							char DIPortCnt = 0, DOPortCnt = 0;
							char TxBuf[2] = {0,}; //IO Start Pos Number Buf
							int s32Tx_CANID=0;
						
							s32Tx_CANID = ((CMD_WRITE_DIO_ASSIGN << CAN_CMD_OFFSET) | g_dsGetBoardInfo.s32BoardCANID);
							TxBuf[0] = 0;
							TxBuf[1] = 0;
							
							IO_CAN_TX(0, s32Tx_CANID, TxBuf, sizeof(TxBuf));

						}
						g_dsIOLiveCheck.u8IOLiveRetryFlag = FALSE;
					}
				}
			}			
			
IO_INFO_REQUEST:
			// 재검사를 위해 초기화 한다.
			memset(&g_dsIOLiveCheck.s32LiveID, 0x0, sizeof(g_dsIOLiveCheck.s32LiveID));
			U32 secTm   = (U32)(g_pAISharedMem->m_dsSysTime.m_u64TimeSec);
			U32 usecTm = (U32)(g_pAISharedMem->m_dsSysTime.m_u64TimeUsec);
			
			memcpy(u8Tmbuf, &secTm, sizeof(U32));
			memcpy(&(u8Tmbuf[4]), &usecTm, sizeof(U32));

			// IO Board DIO Read
			IO_CAN_TX(0, IO_DIO_INFOMAION_CANID, (char *)u8Tmbuf, sizeof(u8Tmbuf));
		
		}
	}
	else
	{
		g_LiveCheckTmCnt+=Interval; //ms cnt
	}
}


void *IOM_IOBdUpdate(int argc, void *argv[])
{
	S32 ResWaitCnt = 0; //respone wait..
	U32 UpdatID = 0;
	U16 u16cmd = 0;
	
	if((argc == 0) || (argc > 1))
	{
		printf("Invalid Argument count\n");
		return 0;
	}

	//if(strncasecmp((char *)argv[0], "IO", sizeof("IO")) == 0)
	{
		U32 u32Total_TxPacket_Cnt = 0, u32TxSeq_Cnt = 0;
		U32 u32Cmd = CMD_WRITE_UPGARDE_DOWN_START, SeqErrCnt = 0, RetryTxCnt = 0;
		U8  u8FileName[64] = "/upgrade/UR-IO.bin";
		U32 u32FileSize = IOM_GetFileSize(u8FileName), u32DataOffset = 0;
		U8 au8FileSizeBuf[4] = {0,};
		U8  *u8UpdatData = 0, u8TxDataBuf[8] = {0,}, u8SeqNum = 0;

		//mem alloc
		u8UpdatData = (U8 *)malloc(u32FileSize);
		
		s8UpdatCheckFlag = -1;
        u32Total_TxPacket_Cnt = (u32FileSize / CAN_DATA_PACKET_SIZE);
        if(u32FileSize % CAN_DATA_PACKET_SIZE)
        	u32Total_TxPacket_Cnt++;

		//sootoo23 - dio board update -> multi packet..

		//1step. : update start signal tx.
		UpdatID = (CMD_WRITE_UPGRADE_START << CAN_CMD_OFFSET);
		UpdatID|= CAN_BCAST_ID;
		IO_CAN_TX(0, UpdatID, NULL, 0);
		
		printf("[%s] Update Signal tx!\n", __FUNCTION__);
		usleep(10000); //10ms

		//2step. : update start respone done check.
WAIT_DIOUPDAT_FLAG:
		if(ResWaitCnt > 100)
		{
			free(u8UpdatData);
			printf("[%s] Update Signal Recv Fail!\n", __FUNCTION__);
			return 0;
		}
			
		if(	s8UpdatCheckFlag < UPDAT_READY)
		{
			ResWaitCnt++;
			usleep(10000); //10ms
			goto WAIT_DIOUPDAT_FLAG;
		}
		else
			ResWaitCnt = 0;

		usleep(200000); //200ms

        //3step: dio update file load...
        IOM_GetFileData((U8 *)u8FileName,(U8 *)u8UpdatData,u32FileSize);
        printf("[%s] Update file load! (size:0x%x) \n", __FUNCTION__, u32FileSize);

        //4step: file size tx.
        UpdatID = (CMD_WRITE_UPGRADE_SIZE << CAN_CMD_OFFSET);
		UpdatID|= CAN_BCAST_ID;
		//u32FileSize = (u32FileSize >> 16) | ((u32FileSize & 0xFFFF) << 16);
	        memcpy(au8FileSizeBuf, &u32FileSize, sizeof(u32FileSize));
		
		IO_CAN_TX(0, UpdatID, (char *)au8FileSizeBuf, sizeof(u32FileSize));
		
		printf("[%s] File Size Tx! (size:0x%x) \n", __FUNCTION__, u32FileSize);

		//5step: file size respone check.
WAIT_DIOUPDAT_FILESIZE:
		if(ResWaitCnt > 100)
		{
			free(u8UpdatData);
			printf("[%s] file size Recv Fail!\n", __FUNCTION__);
			return 0;
		}
		
		if(	s8UpdatCheckFlag < UPDAT_DOWNLOAD_READY)
		{
			ResWaitCnt++;
			usleep(10000); //10ms
			goto WAIT_DIOUPDAT_FILESIZE;
		}
		else
			ResWaitCnt = 0;

		printf("[%s] Multi Packet Tx Start.. Total Seq: 0x%x\n\n", __FUNCTION__, u32Total_TxPacket_Cnt+1);
		
		//DIO Board Update ID Setting
		UpdatID = (u32Cmd << CAN_CMD_OFFSET);
		UpdatID|= CAN_BCAST_ID;
		
		//5step. : multi packet tx
		for(u32TxSeq_Cnt=0;  u32TxSeq_Cnt < u32Total_TxPacket_Cnt; u32TxSeq_Cnt++)
		{
			//Tx Data Copy
			u8TxDataBuf[DATA_SEQ] = u8SeqNum;
			memcpy((U8 *)&u8TxDataBuf[1], ((U8 *)u8UpdatData + u32DataOffset), CAN_DATA_PACKET_SIZE);
			u32DataOffset += CAN_DATA_PACKET_SIZE;
			
			if(u32TxSeq_Cnt == (u32Total_TxPacket_Cnt-1)) 
			{	
				//Last Packet
				UpdatID = (CMD_WRITE_UPGRADE_DOWN_FINISH << CAN_CMD_OFFSET);
				UpdatID|= CAN_BCAST_ID;
				
				if(u32FileSize % CAN_DATA_PACKET_SIZE)
					u32FileSize = (u32FileSize % (CAN_DATA_PACKET_SIZE)) +1; //+1은 Seq 사이즈..
			}

RETRY_TX:
			//Data Tx
			IO_CAN_TX(0, UpdatID, (char *)u8TxDataBuf, u32FileSize);
			
			printf("\r Packet Tx...[%04d/%04d] \t\t %f %%", u32TxSeq_Cnt+1, u32Total_TxPacket_Cnt, ((F32)(u32TxSeq_Cnt+1)/((F32)u32Total_TxPacket_Cnt))*100.0);
			usleep(10000); //10ms

WAIT_DIOUPDAT_SEQ:
			if(u32MultiPacketSeq != (U32)u8SeqNum)
			{
				//Packet Seq Respone Err.
				printf("\r Packet Seq Err (0x%x != 0x%x)", u32MultiPacketSeq, (U32)u8SeqNum);
				usleep(1000); // 1ms
				SeqErrCnt++;
				
				if(SeqErrCnt < 100)
					goto WAIT_DIOUPDAT_SEQ;
				else
				{
					RetryTxCnt++;
					if(RetryTxCnt < 10)
						goto RETRY_TX;
					else
					{
						//sootoo23-170731: Seq Err를 100번 기다리고 다시 Tx를 10번 보내기를 시도해도 Error라면 Update 중단.
						//Need: Event Record

						printf("\n[%s] No Respone... Update Fail.\n", __FUNCTION__);
						//DIO Board Update ID Setting
						UpdatID = (CMD_WRITE_UPGRADE_DOWN_STOP << CAN_CMD_OFFSET);
						UpdatID|= CAN_BCAST_ID;
						IO_CAN_TX(0, UpdatID, NULL, 0);
						
						s8UpdatCheckFlag = -1;
						//DBM_SaveLog(TYPE_EVENT_IO, "IO board update fail.", NULL);
						usleep(10000); //10ms
						break;
					}
				}
			}
			else
			{
				SeqErrCnt = 0;
				RetryTxCnt = 0;
				
				//Multi packet에서 255seq를 넘기면 1부터 시작한다.
				u8SeqNum = (u8SeqNum >= 255 ? 1 : u8SeqNum+1);
			}
		}

		free(u8UpdatData);
	}
	
	//DIO Board Update ID Setting
	UpdatID = (CMD_WRITE_UPGRADE_RESULT << CAN_CMD_OFFSET);
	UpdatID |= CAN_BCAST_ID;
	IO_CAN_TX(0, UpdatID, NULL, 0);
	
	printf("\n[%s] Update finish!\n", __FUNCTION__);
	return 0;
}

S32 IOM_Parser()
{
	int i = 0, DIOffset = 0, ReadByte = 0, BdNum = -1, BdCnt = 0, s32SvaeIdx=-1;
	int RTDOffset = 0, TDOffset = 0;
	S16 s16RtdVal[4] = {0,}, RtdIdx = 0;
	U16 u16cmd = 0;
	U16 BitMapBytePos = 0, BitMapDataPos=0;
	U16 BitMapBytePos_Map = 0, BitMapDataPos_Map=0;
	U16 BitMapBytePos_Phy = 0, BitMapDataPos_Phy=0;
	struct can_frame canRxBuf;
	dsEvtFrmDspIfc g_dsIOEvt;

	memset(&g_dsIOEvt.m_EvtTmsec,0,sizeof(dsEvtFrmDspIfc));
	//g_dsIOEvt.m_u32EvtNum = 0;

	//Rx Data
	for(BdCnt=0; BdCnt< g_u32TotIOBdCnt; BdCnt++)
	{
		ReadByte = IO_CAN_RX(&canRxBuf);	
		if(ReadByte > 0)
		{
			u16cmd = (canRxBuf.can_id & CAN_CMD_MASK) >> CAN_CMD_OFFSET;
			BdNum  = (canRxBuf.can_id & CAN_ID_MASK);

			//Data 1Byte = SEQ / Other 7Byte = Data
			switch(u16cmd)
			{
				case CMD_READ_BOARD_CHECK:
				{
					if(g_dsIOLiveCheck.u8IOLiveRetryFlag == 1)
					{
						g_dsIOLiveCheck.s32RetryID = BdNum;
						g_dsIOLiveCheck.u8IOLiveRetryIdx++;
					}
				}
				break;
				case CMD_READ_BOARD_SET_REQ:
				{
					char DIPortCnt = 0, DOPortCnt = 0;
					char TxBuf[2] = {0,}; //IO Start Pos Number Buf
					int s32Tx_CANID=0;
					
					s32Tx_CANID = ((CMD_WRITE_DIO_ASSIGN << CAN_CMD_OFFSET) | g_dsGetBoardInfo.s32BoardCANID);
					DIPortCnt = 0; 
					DOPortCnt = 0;
					TxBuf[0] = DIPortCnt;
					TxBuf[1] = DOPortCnt;
							
					IO_CAN_TX(0, s32Tx_CANID, TxBuf, sizeof(TxBuf));
					
					IOM_IOTimeInfoTx();
					break;
				}
				break;
				case CMD_READ_DIO_INFORMATION:
				{
					// Can통신 응답하는 Board는 Live ID로 저장한다.
					// Boards that respond to Can communication are saved as Live ID.
					IOM_IOBdLiveIDAdd(BdNum);

					U16 u16MaxDIOCount = 0;
					
					U32 RecvData = 0, masking = 0, PointNum = 0, PhyNum = 0;
					U8 Val = 0;
					
					//Data Copy
					memcpy(&RecvData, (U8*)canRxBuf.data, 2);

					/*
					*  IO 발생 이벤트 기록
					*/
					g_dsIOEvt.m_EvtTmsec = g_ds61850SharedMem.dsArmDestUTC.secs;
					g_dsIOEvt.m_EvtTmusec = g_ds61850SharedMem.dsArmDestUTC.usecs;
					g_dsIOEvt.m_u64EvtVal = 0;


					masking =1;
					for(i=0; i<DIPU_DI_MAX; i++)
					{
						masking = (U32)pow(2,i);
						Val = ((RecvData & masking) == 0 ? 0:1);
						PhyNum = i;
						if(g_pIOSharedMem->m_dsIOInfo.u8DIStsVal[i] != Val)
						{
							g_pIOSharedMem->m_dsIOInfo.u8DIStsVal[i] = Val;
							pthread_mutex_lock(&g_ds61850SharedMem.mutex);							
							g_ds61850SharedMem.dsPhyDIMem.wPhyDI[i] = g_pIOSharedMem->m_dsIOInfo.u8DIStsVal[i];
							pthread_mutex_unlock(&g_ds61850SharedMem.mutex);							
							//if(Val)
								//ERM_EventDataWrite(STS_IOEVT_DI_OCCURRED,PhyNum,0);
							//else
								//ERM_EventDataWrite(STS_IOEVT_DI_RELEASED,PhyNum,0);
						}									

					}
					
					masking =1;
					for(i=0; i<DOU_DO_MAX; i++)
					{
						masking = (U32)pow(2,(i+8));
						Val = ((RecvData & masking) == 0 ? 0:1);
						PointNum = i;
						if(g_pIOSharedMem->m_dsIOInfo.u8DOStsVal[i] != Val)
						{
							g_pIOSharedMem->m_dsIOInfo.u8DOStsVal[i] = Val;
						}
					}
					
					break;
				}
				break;
				case CMD_READ_DEBOUNCE_INFO:
				{
					U8 u8IndexNum1 = 0, u8IndexNum2 = 0, u8IndexNum3 = 0, u8IndexNum4 = 0;

					u8IndexNum1 = canRxBuf.data[0];
					u8IndexNum2 = canRxBuf.data[2];
					u8IndexNum3 = canRxBuf.data[4];
					u8IndexNum4 = canRxBuf.data[6];

					if(canRxBuf.data[1] != 0) 		memcpy((U32 *)&(g_dsIORealValse.m_u8DIDebounceTm[u8IndexNum1]), &(canRxBuf.data[1]), sizeof(U8));
					if(canRxBuf.data[3] != 0)		memcpy((U32 *)&(g_dsIORealValse.m_u8DIDebounceTm[u8IndexNum2]), &(canRxBuf.data[3]), sizeof(U8));
					if(canRxBuf.data[5] != 0)		memcpy((U32 *)&(g_dsIORealValse.m_u8DIDebounceTm[u8IndexNum3]), &(canRxBuf.data[5]), sizeof(U8));
					if(canRxBuf.data[7] != 0)		memcpy((U32 *)&(g_dsIORealValse.m_u8DIDebounceTm[u8IndexNum4]), &(canRxBuf.data[7]), sizeof(U8));
				}
				break;
				case CMD_WRITE_UPGRADE_START:
					s8UpdatCheckFlag = UPDAT_READY;
				break;
				case CMD_WRITE_UPGRADE_SIZE:
					s8UpdatCheckFlag = UPDAT_DOWNLOAD_READY;
				break;
				case CMD_WRITE_UPGARDE_DOWN_START:
					u32MultiPacketSeq = canRxBuf.data[0];
				break;
				case CMD_WRITE_UPGRADE_DOWN_STOP:
					s8UpdatCheckFlag = -1;
				break;
				case CMD_WRITE_UPGRADE_DOWN_FINISH:
				{
					s8UpdatCheckFlag = UPDAT_DOWNLOAD_FIN;
					u32MultiPacketSeq = canRxBuf.data[0];
				}
				break;
				case CMD_WRITE_UPGRADE_RESULT:
				{
					U32 u32EvtNum=0;
					U16 u16FWVer = 0, u16UpRet = 1;
					U8 updat_ret[16] = {0,}, LogBuf[128] = {0,};
					char DIPortCnt = 0, DOPortCnt = 0, DIPUBoardCnt = 0, DOUBoardCnt = 0;
					char TxBuf[2] = {0,}; //IO Start Pos Number Buf
					int s32Tx_CANID=0;
					
					char EvtValStr[64]={0,};

					s8UpdatCheckFlag = UPDAT_RESULT;
					u32MultiPacketSeq = 0;

					memcpy(&u16FWVer, &(canRxBuf.data[0]), sizeof(U16));
					memcpy(&u16UpRet, &(canRxBuf.data[2]), sizeof(U16));
					
					sprintf((char*)EvtValStr, "ID [0x%x]", BdNum);
						
					g_dsIOEvt.m_u64EvtVal = BdNum;
					g_dsIOEvt.m_u32EvtData1 = u16FWVer;
					sprintf((char*)g_dsIOEvt.u8EvtValContent, "ID [0x%x]", BdNum);
					ERM_EvtQueue_Insert(g_dsIOEvt);

					for(i=0; i<g_u32TotIOBdCnt; i++)
					{
						s32Tx_CANID = ((CMD_WRITE_DIO_ASSIGN << CAN_CMD_OFFSET) | g_dsGetBoardInfo.s32BoardCANID);
						TxBuf[1] = 0;
						DIPUBoardCnt++;
						
						IO_CAN_TX(0, s32Tx_CANID, TxBuf, sizeof(TxBuf));
					}
			
					IOM_IOTimeInfoTx();		
				}
				break;
			}
		}
	}
}

void IOM_IOTimeInfoTx(void)
{
	int i=0;
	char TxBuf[6] = {0,}; //IO Start Pos Number Buf
	int s32Tx_CANID=0;
	U32 u32IONo, u32BoardOffset=0;

	s32Tx_CANID = ((CMD_WRITE_DI_DEBOUNCE << CAN_CMD_OFFSET) | g_dsGetBoardInfo.s32BoardCANID);
	u32IONo = 0x1;
	for(i=0;i<DIPU_DI_MAX;i++)
	{
		memcpy(&TxBuf[0], &u32IONo, sizeof(U8)); //di port number
		//memcpy(&TxBuf[1], (U32*)g_pIOSharedMem->m_dsIOSetting.u8DIDebounceTime[i], sizeof(U8));
		IO_CAN_TX(0, s32Tx_CANID, (char *)TxBuf, sizeof(TxBuf));

		u32IONo = u32IONo << 1;
		usleep(100); 
	}
}

void IOM_DI_Unlock_Tx(void)
{
	U32 DIUnlockID=0;

	DIUnlockID = ((CMD_WRITE_DILOCK_RELEASE << CAN_CMD_OFFSET) | CAN_BCAST_ID);

	IO_CAN_TX(0, DIUnlockID, NULL, 0);
}

void *IOM_DispDboTm(int argc, void *argv[])
{
	int i=0;
	int s32Tx_CANID=0;
	U16 u16DIInfoType = 0;

	if(argc == 0)
	{
		printf("\nDIINFO [Type] ");
		printf("\nType 0: IO 보드에 정보 요청, Type 1: DI Info Display\n");
		return 0;
	}
	else 
	{
		u16DIInfoType = strtoul((const char *)argv[0], NULL, 10);
	}

	if(u16DIInfoType == 0)
	{
			s32Tx_CANID = ((CMD_READ_DEBOUNCE_INFO << CAN_CMD_OFFSET) | g_dsGetBoardInfo.s32BoardCANID);
				
			IO_CAN_TX(0, s32Tx_CANID, NULL, 0);
	}
	else if(u16DIInfoType == 1)
	{
    	printf("\n=========+=========+================");  
    	printf("\n  Name   |  Index  |  Debounce[ms]  "); 
    	printf("\n=========+=========+================");
		for(i = 0; i<UseDIPoint;i++)	
		{
    			printf("\n DI %03d  |  [%3d]  |  [%3d]     ", i+1, i, g_dsIORealValse.m_u8DIDebounceTm[i]);
		}
		printf("\n=========+=========+================\n");            
	}
}

void *IOM_CLISetDebounceTm(int argc, void* argv[])
{
	unsigned int u32IONo = 0, DBOID = 0;
	U8 au8TxBuf[2] = {0,};
	U8 dboTm=0;
	
	if(argc == 0)
	{
		printf("\nDBO [DI Pin] [Time(ms)]\n");
		return 0;
	}
	
	u32IONo = convert_Str_to_DEC((char *)argv[0]);
	dboTm = (U8)convert_Str_to_DEC((char *)argv[1]);
	
	if((u32IONo < 1) || (u32IONo > MAX_DI_CHANNEL))
	{
		printf("[ERR] Invalid DI Pin (1~%d)\n",MAX_DI_CHANNEL);
		return 0;
	}
	else if((dboTm > 255) || (dboTm < 1))
	{
		printf("[ERR] Invalid DBO Time (1~255)\n");
		return 0;
	}
	
	//DI Debounce tm ID
	DBOID = (CMD_WRITE_DI_DEBOUNCE<< CAN_CMD_OFFSET);

	//BdNum = (((u32IONo-1)/DIPU_DI_MAX)+1);
	DBOID |= g_dsGetBoardInfo.s32BoardCANID;
	u32IONo = 1 << (u32IONo-1);

	au8TxBuf[0] = u32IONo;
	au8TxBuf[1] = dboTm;

	g_pIOSharedMem->m_dsIOSetting.u8DIDebounceTime[u32IONo] = dboTm;

	IO_CAN_TX(0, DBOID, (char *)au8TxBuf, sizeof(au8TxBuf));
	
	printf("[%s] txbuf 1:0x%x 2:0x%x\n", __func__, au8TxBuf[0], au8TxBuf[1]);

	
	return 0;
}

void *IOM_DOControl(int argc, void* argv[])
{
	U8 au8TxBuf[2] = {0,};

	if(argc < 2)
	{
		printf("DOCTRL [CH (1~3 or ALL)] [CONTROL(0: OFF, 1: ON)] \n");
		printf("EX) DOCTRL 1 1 \n");
		printf("    DOCTRL ALL 0 \n");
		return 0;
	}
	else
	{
		U8 DOPin = g_u8DOControlVal;
		U8 DOCtrlVal = atoi((char *)argv[1]);

		if(strncasecmp(((char*)argv[0]), "ALL", sizeof("ALL")) == 0)
		{
			if(DOCtrlVal)
				DOPin = 0x07;
			else
				DOPin = 0x00;
		}
		else if(atoi((char*)argv[0]) > MAX_DO_COUNT)
		{
			printf("[ERR] Invalid Channel !\n");
			return 0;
		}
		else
		{
			U16 u16SelCH = atoi((char*)argv[0]);

			if(DOCtrlVal)
				DOPin |= (U8)pow(2, u16SelCH-1);
			else
			{
				U8 u8val = ~((U8)pow(2,u16SelCH-1));
				DOPin &= u8val;
			}
		}
		
		memcpy(au8TxBuf, &DOPin, sizeof(au8TxBuf));

		g_u8DOControlVal = DOPin;
	}

	IO_CAN_TX(0, IO_DO_CONTROL_CANID, (char *)au8TxBuf, sizeof(au8TxBuf));
	
	return 0;
}

void* Watchdog()
{
	U16 u16WatchDogCnt=0;
	
	printf("[%s] Task Start.\n", __func__);

	while(1)
	{
		//if(initfinish==1){
			u16WatchDogCnt++;
			if(u16WatchDogCnt > 25)
			{
				WDT_InputSignal();
				u16WatchDogCnt = 0;
			}
		//}
		usleep(50000);
	}

}



void* IOM_ProcessMain(void *params)
{
	printf("[%s] Task Start.\n", __func__);
	struct timespec t, t_now; 
	U32 interval = 100*1000; //100us.
	double timenow = 0.0;
	U16 u16tick = 0;
	struct timeval tv;
	
		/*
	* sootoo23 - RT Schedule Setting. -> FIFO
	* 높을수록 높은 priority 를 의미
	*/
	struct sched_param param; 
	param.sched_priority = 2; 	/* important, running above kernel RT or below? */ 

	if(sched_setscheduler(0, SCHED_RR, &param) == -1) 
		{ perror("sched_setscheduler failed"); exit(-1); }
	if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) 
		{ perror("mlockall failed"); }

    printf(" ==========================================IOM_ProcessMain PID: %lu\n", pthread_self());

	if(IOM_Init())
		printf("\n[ERR] IOM Init Fail!! \n");

	//Cur Time Get
	//clock_gettime(CLOCK_MONOTONIC,&t_now);
	t.tv_sec = 0;
	t.tv_nsec = interval;
	IOM_ThreadExit = 0;
	
	while(!IOM_ThreadExit)
	{
		/*
		*	sootoo23 - I/O Packet Parser
		*	WARNING - CAN Dev NonBlocking Mode
		*/
		IOM_IOBdLiveCheck(10, 10);

		IOM_Parser();
		
		/*
		*	IO 처리를 Blocking모드로 해버리기 때문에..
		*	다른 일반 Thread들은 끼어들 틈이 없다..
		*	숨쉴 틈을 주기위해 100us정도 쉬어 준다.
		*/
		nanosleep(&t, NULL);
	}

	
	pthread_exit(NULL);
	printf("[%s] Task Close.\n", __func__);
	return 0;
}

