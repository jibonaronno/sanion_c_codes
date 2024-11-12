/******************************************************************************
 **                     Copyright(C) 2017 SANION Co. Ltd.                    **
 **                  --------------------------------------                  **
 **                              WWW.SANION.COM                              **
 ******************************************************************************
    PROJECT     : UR-IED
 ******************************************************************************
    FILE NAME   : SharedMemMgr.c
    AUTHOR      : sootoo23
    DATE        : 2017.01.10
    REVISION    : V1.00
    DESCRIPTION : DSP 공용 구조체 및 변수 관리
 ******************************************************************************
    HISTORY     :
        2017-01-10 초안 작성
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ti/cmem.h>
#include <unistd.h>

#include "Common.h"
#include "ur_gpio.h"
#include "SharedMemMgr.h"

CMEM_AllocParams  alloc_params;
S32 Shared_PhyAddr = 0;

void InitSharedMemory(U8 *pCMemVAddr)
{	
	g_pAISharedMem		= (dsAISharedMemory*)pCMemVAddr;
	g_pIOSharedMem		= (dsIOData*)(pCMemVAddr + DDR_DIO_DATA_OFFSET);
	g_pLUAppSharedMem	= (dsLUAppSharedMemory*)(pCMemVAddr + DDR_LU_APP_DATA_OFFSET);
	g_pSTBSharedMem		= (ds61850SharedMemory*)(pCMemVAddr + DDR_STBP_DATA_OFFSET);

	// Clear
	memset((dsAISharedMemory*)g_pAISharedMem, 0x0, sizeof(dsAISharedMemory));
	memset((dsIOData*)g_pIOSharedMem, 0x0, sizeof(dsIOData));
	memset((dsLUAppSharedMemory*)g_pLUAppSharedMem, 0x0, sizeof(dsLUAppSharedMemory));
	memset((ds61850SharedMemory*)g_pSTBSharedMem, 0x0, sizeof(ds61850SharedMemory));

	/*printf("CmemAddr          [%x]\n",pCMemVAddr);
	printf("g_pAISharedMem    [%x]\n",g_pAISharedMem);
	printf("g_pIOSharedMem   [%x]\n",g_pIOSharedMem);
	printf("g_pLUAppSharedMem [%x]\n",g_pLUAppSharedMem);
	printf("g_pSTBSharedMem   [%x]\n",g_pSTBSharedMem);*/
}

int SharedMemMgr_MemCheck(void)
{	
	//Shared memory check.
	if(g_pAISharedMem == NULL)
	{
		printf("[%s] ERR: Shared Memory is NULL!");
		return -1;
	}
	else
	{
		system("/apps/dspUnbind.sh");
		DspManager_DSPOn();
		usleep(3000*1000);	//wait DSP boot
	}
	
	/*
	*	sootoo23-190129: ARM<>DSP IF에서 IPC를 제거하면서.
	*	초기 App 부팅 시 타이밍을 모르게 되어 Shared Mem의 Size로 그 역할을 대신한다.
	*	
	*	이슈: DSP 가 항상 먼저 부팅하기 때문에 문제가 생긴다. 정상 부팅한 후
	*	    바로 Power Down -> Up 하게 되면 DDR이 바로 data가 날라가는게 아니기 때문에
	*		기존에 Write했던 Shared mem size가 남아있어 ARM이 부팅전에 DSP가 인식해서
	*		부팅하게 된다.. 
	*		따라서 SDK Init Routine이 끝난 후 0x0으로 초기화 해준다.
	*/
	if(g_pAISharedMem->m_u32dsSize[1] != g_pAISharedMem->m_u32dsSize[0])
	{

		printf("[%s] MPU <> DSP IF ERROR (Data Structure size Invalid!)\n", __FUNCTION__);
		printf("[%s] MPU: 0x%x <> DSP: 0x%x \n",__FUNCTION__, g_pAISharedMem->m_u32dsSize[0], g_pAISharedMem->m_u32dsSize[1]);

		g_pAISharedMem->m_u32dsSize[0] = DEF_SHARED_MEM_INIT_VALUE;
		return -1;
	}
	else
	{
		g_pAISharedMem->m_u32dsSize[0] = DEF_SHARED_MEM_INIT_VALUE;
		return 0;
	}

}

void DspManager_DSPOff(void)
{
	//Reset FPGA
	gpio_export(GPIO_FPGA_N_RESET);
	gpio_set_dir(GPIO_FPGA_N_RESET, GPIO_OUTPUT, 1);
	gpio_set_val(GPIO_FPGA_N_RESET, 0);
	gpio_unexport(GPIO_FPGA_N_RESET);

	//DSP off
	system("/apps/dspUnbind.sh");
	g_pAISharedMem->m_u32dsSize[0] = DEF_SHARED_MEM_INIT_VALUE;	
}

void DspManager_DSPOn(void)
{
	g_pAISharedMem->m_u32dsSize[0] = sizeof(dsAISharedMemory);
	//printf("----------------------SIZE[0] %X SIZE[1] %X \n",g_pAISharedMem->m_u32dsSize[0], g_pAISharedMem->m_u32dsSize[1]);
	system("/apps/dspBind.sh");		//FPGA set by code in DSP
}

int CCM_AllocShareMem(Void)
{
	int status = -1;

	alloc_params.type = CMEM_POOL;
	alloc_params.alignment = 0;
	alloc_params.flags = CMEM_NONCACHED;  //  CMEM_CACHED

	/* First initialize the CMEM module */	
	if (CMEM_init() == -1) {
		printf("[%s]Failed to initialize CMEM\n", __FUNCTION__);
		return status;
	}

	/* First allocate a buffer from the pool that best fits */
	g_pCMemVAddr = (U8 *)CMEM_alloc(1024*1024*192, &alloc_params); //192MB Alloc
	if (g_pCMemVAddr == NULL) 
	{
		printf("[%s] Failed to allocate\n", __FUNCTION__);
		CMEM_exit();
		return status;
	}
	else
		InitSharedMemory(g_pCMemVAddr);

	/* Vir Addr -> Phy Addr */
	Shared_PhyAddr = CMEM_getPhys((U8 *)g_pCMemVAddr); //CMEM Get Phy Addr
	if (Shared_PhyAddr == 0) 
	{
		printf("[%s] Failed to get physical address of %#x\n", __FUNCTION__, (unsigned int) Shared_PhyAddr);
		return status;
	}
	else
		printf("[%s] Physical address of allocated buffer is %#llx.\n", __FUNCTION__, (unsigned long long)Shared_PhyAddr);
		
	return 0;
}

