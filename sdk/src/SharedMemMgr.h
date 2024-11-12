/******************************************************************************
 **                     Copyright(C) 2016 SANION Co. Ltd.                    **
 **                  --------------------------------------                  **
 **                              WWW.SANION.COM                              **
 ******************************************************************************
    PROJECT     : UR-IED MPU
 ******************************************************************************
    FILE NAME   : SharedMemMgr.h
    AUTHOR      : sootoo23
    DATE        : 2015.06.09
    REVISION    : V1.00
    DESCRIPTION : DSP 공용 구조체 및 변수 관리 헤더
 ******************************************************************************
    HISTORY     :
        2015-06-09 초안 작성
 ******************************************************************************/

#include <ti/ipc/Std.h>
#include <ti/ipc/MessageQ.h>
#include <ti/ipc/Ipc.h>
#include <ti/ipc/transports/TransportRpmsg.h>
#include <ti/ipc/MultiProc.h>

#include "global.h"

#ifndef SHAREDMEMMGR_H_
#define SHAREDMEMMGR_H_

#define DEF_SHARED_MEM_INIT_VALUE 0

void InitSharedMemory(unsigned char *pCMemVAddr);
int SharedMemMgr_MemCheck(void);
void DspManager_DSPOff(void);
void DspManager_DSPOn(void);
int CCM_AllocShareMem(Void);

#endif /* SHAREDMEMMGR_H_ */
