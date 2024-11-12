/******************************************************************************
 **                     Copyright(C) 2014 SANION Co. Ltd.                    **
 **                  --------------------------------------                  **
 **                              WWW.SANION.COM                              **
 ******************************************************************************
    PROJECT     : UR-IED
 ******************************************************************************
    FILE NAME   : Main.c
    AUTHOR      : sootoo23
    DATE        : 2016.11.04
    REVISION    : V1.00
    DESCRIPTION : Main Function (Device Init, Function Init)
 ******************************************************************************
    HISTORY     :
        2016-11-04 Create
 ******************************************************************************/

#define _XOPEN_SOURCE 500

//Includes
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stddef.h>
#include <glob.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <ti/cmem.h>
#include <sys/shm.h>
#include "sys/sysinfo.h"
#include <ftw.h>
#include <arpa/inet.h>


#include "Common.h"
#include "global.h"
#include "UR_SDK.h"
#include "SharedMemMgr.h"
#include "wdt_ds1832.h"
#include "IEC61850Manager.h"
#include "iec61850_sdk.h"
#include "ConsoleManager.h"
#include "DBManager.h"
#include "SystemSettingModule.h"
#include "main.h"
#include "IOManager.h"
#include "PowerSystem.h"		// added by symoon
#include "tcpipprotocol.h"
#include "PDMonitoring.h"		// 2021. 12. 15. added by symoon
#include "GISMonitoring.h"		// 2021. 12. 15. added by symoon
#include "MTrMonitoring.h"		// added by symoon
//#include "spectrum_packet.h"
#include "TimeSyncModule.h"
#include "cidcsvmanager.h"
#include "pddau.h"

//#include "Acq.h"
#define INTERVAL_MS 10 // Interval for usleep
#define MINUTE_IN_MILLISECONDS 60000 // One minute in milliseconds

/*!
 * @file main.c
 * Entry Point file main.c contains the main() entry point functions, Shared Memories, Global Variables, 
 * Global functions for VMD pointers operations. 
 */

/*
*    global variable
*/
U8 *g_pCMemVAddr;
volatile dsIOData				*g_pIOSharedMem;
volatile dsAISharedMemory		*g_pAISharedMem; 
volatile dsLUAppSharedMemory 	*g_pLUAppSharedMem;
volatile ds61850SharedMemory	*g_pSTBSharedMem;

//sootoo23 - Thread Value
pthread_t DAU_threadID=0,PD_threadID=0, GIS_threadID=0, MTR_threadID=0, Console_threadID=0, DIO_threadID=0, IEC61850_threadID=0, Sensor_threadID=0, Watchdog_threadID=0;
pthread_t tcpip_threadID = 0;
pthread_attr_t tcpip_init_attr;
pthread_attr_t	DAU_init_attr,PD_init_attr, GIS_init_attr,MTR_init_attr,Console_init_attr, DIO_init_attr, IEC61850_init_attr,Sensor_init_attr, Watchdog_init_attr;
pthread_mutex_t dsUTCTimeMutex = PTHREAD_MUTEX_INITIALIZER;


pthread_t SUBSYSTEM_threadID = 0;
pthread_attr_t SUBSYSTEM_thread_init_attr;

pthread_t ACQ_threadID = 0, Time_threadID = 0;
pthread_attr_t ACQ_thread_init_attr, Time_init_attr;

U8 gu8is61850 = 0;
U8 gu8changedetected = 0;
int* gaps32phydi61850[MAX_DI_CHANNEL] = {0,};
S32	gs32SntpTimeQuality=0;
S32 gs32lln0idx = 0;
U8 g_u8WDTCtrlFlag=0;
U8 g_u8WDTSignalFlag=0;


dsLUConfig pdsluConfig;


// 2021.06.28 $)Cj80l!4 SUR5000 61850Lib ?o?=m??o?=l? l;4m????o?=k?o??l6??=???k6�o??//
// LocalUnit $)Cj0?? ??61850 l=??o??k3�j2=m?k)4l? ?o?=j10?o?=l???                 ///
ds61850SharedMemory g_ds61850SharedMem;
ds61850IfcInfo* g_p61850IfcInfo; /** 61850 Interface Info. */
ds61850MappingTbl* g_p61850MappingTbl; /** 61850 Mapping Table of Leaf List. */
volatile dsCoreSharedMemory* g_pCoreSharedMem; /** DSP Core Shared Memory. Shared between DSP and MPU. */
volatile dsLogicMem* g_ptLogicMemory;
volatile dsWaveQ* g_ptWaveBuffer; 

/** Function has no content. */
int LGS_ChangeActiveGroup(U8* u8DataSrc)
{
	return 0;
}

/** Function has no content. */
void ERM_EvtQueue_Insert(dsEvtFrmDspIfc dsEvtData)
{

}

/*!
 * Function: SignalBlocking() . Blocks Ctrl+C kill signal. 
 */

int SignalBlocking()
{
	sigset_t SignalSet;

	//signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_IGN); // terminal
	sigaddset (&SignalSet, SIGALRM); // alarm
	sigaddset (&SignalSet, SIGQUIT); // exit
	sigaddset (&SignalSet, SIGINT);  // Interrupt
	sigaddset (&SignalSet, SIGUSR1); // user define
	sigaddset (&SignalSet, SIGUSR2); // user define
	//sigaddset (&SignalSet, SIGIO); // ReatTime Signal define

	pthread_sigmask (SIG_BLOCK, &SignalSet, NULL);
}

/*!
 * File: main.c Function: g_start_Thread(pthread_id, init_attr, func, thread_arg).
 * Major function to create new thread across the project anywhere. 
 * 
 * @param pthread_id A numeric value assigned by the function for later use of thread related
 * operations. 
 * 
 * @param init_attr Thread Attributes Data Structure Instance. Non initialised members will be assigned
 * with default values. In this project most are default.
 * 
 * @param func The main thread function that run an endless loop.
 * 
 * @param thread_arg If the thread loop function receives a parameter then the pointer of the values 
 * passed through this parameter. Suppose the thraed tcpserverrun(...) needs a parameter containing an 
 * ip address.
 */
int g_Start_Thread(pthread_t* pthread_id, pthread_attr_t init_attr, void *(func)(void *), void * thread_arg)
{
	int nReturn = -1;
    nReturn = pthread_attr_init(&init_attr);
    if(nReturn != 0 )
    {
        perror("Attribute 1 creation failed");
        return -1;
    }

    nReturn = pthread_attr_setdetachstate( &init_attr, PTHREAD_CREATE_DETACHED );
    if(nReturn != 0 )
    {
        perror("Setting detached attribute 1 failed");
        return -1;
    }

    nReturn = pthread_create(pthread_id, &init_attr, func, thread_arg);
    if(nReturn != 0)
    {
        perror("pthread create error");
        return -1;
    }

	return 0;
}

/** Function has no content. */
int Get_61850PortStsValue(int ArrayIdx)
{
	//return g_pCfgSharedMem->m_s3261850ProtSts[ArrayIdx];
	return 0;
}

/** Function has no content. */
int Get_is61850Flag(void)
{
	return gu8is61850;
}

/*!
 * Function: Thread Function TimeThread(argv) . At a regular time interval it updates the time 
 * related member of 61850 data structure instance from shared memory. 
 * The function gettimeofday(&dsSysTime, NULL); 
 * reads time from RTC and update or sync with the system time. 
 * @param argv Not doing anything yet.
 */

void *TimeThread(void* argv)
{
	printf("Enter TimeThread\n");
    struct timeval dsSysTime;
    unsigned long lastSyncTime = 0;  // Last time we did a sync
    unsigned long currentTime;
    unsigned int syncInterval = MINUTE_IN_MILLISECONDS;  // Sync interval in milliseconds
	struct sched_param param; 
	param.sched_priority = 2; 	/* important, running above kernel RT or below? */ 

	// if(sched_setscheduler(0, SCHED_RR, &param) == -1) 
	// 	{ perror("sched_setscheduler failed"); exit(-1); }
	// if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) 
	// 	{ perror("mlockall failed"); }

    printf(" ==========================================TimeThread PID: %lu\n", pthread_self());

    while (1)
    {
        gettimeofday(&dsSysTime, NULL);
        currentTime = dsSysTime.tv_sec * 1000 + dsSysTime.tv_usec / 1000;  // Current time in milliseconds

        // Time sync execution
        if (currentTime - lastSyncTime >= syncInterval)
        {
            TSM_TimeSyncExe();
            lastSyncTime = currentTime;  // Update last sync time
        }
		
        // Continuous update of time in shared memory
        pthread_mutex_lock(&g_ds61850SharedMem.mutex);
        gettimeofday(&dsSysTime, NULL);  // Get current time again to be precise
        g_ds61850SharedMem.dsArmDestUTC.secs = dsSysTime.tv_sec;
        g_ds61850SharedMem.dsArmDestUTC.usecs = (unsigned int)dsSysTime.tv_usec;
        g_ds61850SharedMem.dsArmDestUTC.fraction = (U32)((F32)(dsSysTime.tv_usec) / 1000000 * (F32)0x01000000);
        g_ds61850SharedMem.dsArmDestUTC.qflags = gs32SntpTimeQuality;
        pthread_mutex_unlock(&g_ds61850SharedMem.mutex);

        struct timespec req = {0};
        req.tv_sec = 0;
        req.tv_nsec = 5 * 100000000L;  // 500 millisecond
        nanosleep(&req, (struct timespec *)NULL);
    }

    return NULL;
}

/*!
 * Function: RunThread(void) .This function start all the treads needed for this executable firmware project. 
 * Other threads may exist in the lib61850 library. To run all the threads it sequencially calls g_Start_Thread(...) 
 * function several times. 
 */

int RunThread(void)
{
	int nReturn = -1;
	glob_t stFileSearchResult;
	U8 au8IpAddr[20]={0,};
	
	//system("rm -rf /COMTRADE/COMTRADE/*");
	printf("The data directory has been cleared!!\n");
	
	printf("start DIO thread\n");
	nReturn = g_Start_Thread(&DIO_threadID, DIO_init_attr, IOM_ProcessMain, NULL);
	if(nReturn)
	{
		perror("DIO CAN Task Thread error");
		return -1;
	}
	//S32 LLN0val = 1;
	//UpdateVMDWithNewVal("INT32",g_p61850MappingTbl[gs32lln0idx].pvmdptr, &LLN0val);
	//printf("From LU 1, LLN0 Mod : %d\n",*(char*)g_p61850MappingTbl[gs32lln0idx].pvmdptr); 
	
	printf("start console thread\n");
	nReturn = g_Start_Thread(&Console_threadID, Console_init_attr, Console_ProcessMain, NULL);
	if(nReturn)
	{
		perror("CDM Process Thread error");
		return -1;
	}
	
	//printf("From LU 2, LLN0 Mod : %d\n",*(char*)g_p61850MappingTbl[gs32lln0idx].pvmdptr); 
	printf("start tcpip task thread\n");
	nReturn = g_Start_Thread(&tcpip_threadID, tcpip_init_attr, tcpserverrun, g_p61850IfcInfo->ps8IPAddr);
	if(nReturn)
	{
		perror("tcpip Process Thread error");
		return -1;
	}
	
	//printf("From LU 3, LLN0 Mod : %d\n",*(char*)g_p61850MappingTbl[gs32lln0idx].pvmdptr); 
	printf("\n start DAU thread \n");
	//nReturn = g_Start_Thread(&ACQ_threadID, ACQ_thread_init_attr, sbcr_acq_modbus_run, NULL);
	nReturn = g_Start_Thread(&SUBSYSTEM_threadID, SUBSYSTEM_thread_init_attr, subsystem_thread, NULL);
	if(nReturn)
	{
		perror("DAU Process Thread error");
		return -1;
	}
	
	//printf("From LU 4, LLN0 Mod : %d\n",*(char*)g_p61850MappingTbl[gs32lln0idx].pvmdptr); 
	printf("start 61850 thread %d\n");
	nReturn = g_Start_Thread(&IEC61850_threadID, IEC61850_init_attr, IEC61850_ProcessMain, NULL);
	if(nReturn)
	{
		perror("61850 Process Thread error");			
		return -1;
	}			
	//printf("From LU 5, LLN0 Mod : %d\n",*(char*)g_p61850MappingTbl[gs32lln0idx].pvmdptr); 

	printf("start Time thread\n");
	nReturn = g_Start_Thread(&Time_threadID, Time_init_attr, TimeThread, NULL);
	if(nReturn)
	{
		perror("TimeThread error");
		return -1;
	}	
	
	//printf("From LU 6, LLN0 Mod : %d\n",*(char*)g_p61850MappingTbl[gs32lln0idx].pvmdptr); 
/*
	printf("start watchdog thread\n");
	nReturn = g_Start_Thread(&Watchdog_threadID, Watchdog_init_attr, Watchdog, NULL);
	if(nReturn)
	{
		perror("Watchdog Thread error");			
		return -1;
	}	
*/
	/*
	printf("start DAU task thread\n");
	nReturn = g_Start_Thread(&DAU_threadID, DAU_init_attr, DAU_ProcessMain, NULL);
	if(nReturn)
	{
		perror("DAU Process Thread error");
		return -1;
	}
	*/
	
	/*
	printf("start PD task thread\n");
	nReturn = g_Start_Thread(&PD_threadID, PD_init_attr, PD_ProcessMain, NULL);
	if(nReturn)
	{
		perror("PD Process Thread error");
		return -1;
	}
		printf("start GISMTR task thread\n");
	nReturn = g_Start_Thread(&GIS_threadID, GIS_init_attr, GISMTR_ProcessMain, NULL);
	if(nReturn)
	{
		perror("GISMTR Process Thread error");
		return -1;
	}
*/

	printf("start PDDAU task thread\n");
	nReturn = g_Start_Thread(&Sensor_threadID, Sensor_init_attr, RunPDDAU, NULL);
	if(nReturn)
	{
		perror("PDDAU Process Thread error");
		return -1;
	}

/*
	if(DBM_GetSetting("DevInfo", "MPU IPAddr",(char *)au8IpAddr) == 0)
	{
		printf("Current MPU IP is set to be %s\n", au8IpAddr);
	}
*/
 
	return 0;
}


/*
		event_number:18
		event_type:170
		event_datetime1000515120
		event_millisec:537
		contact_duty_A:0.996767
		evnt.contact_duty_B:0.000000
		evnt.contact_duty_C:0.000000
		tc1_peak_curr:1.297569
		tc1_curr_flow:0.000000
		tc2_peak_curr:1.303865
		tc2_curr_flow:0.000000
		cc_peak_curr:1.201072
		cc_curr_flow:2.083333
		contact_op_time_A:260.000000
		contact_op_time_B:0.000000
		block_close_time_A:0.000000
		block_close_time_B:260.000000
		block_close_time_C:260.000000
		The leaf to test is YHGLU/SCBR1$ST$EvtTransF$stVal

*/

int globA = 1;
int cbAlm = 0;
float vlu = 0.5;

extern int16_t gs16leafnum;


/**
 * Write shared mem data for mms packet 32 bit time value.
 * @param leafName: Name specified by the CID file.
 * @param vlu: 32 bit floating time value.
 */
int writeVmdPointerByLeafName_time(char *leafName, time_t vlu)
{
	int idxA = 0;
	for(idxA=0;idxA < (gs16leafnum - 2);idxA++)
	{
		if(strstr((char *)g_p61850MappingTbl[idxA].ps8leafname, leafName))
		{
			//printf("Found Leaf %s at %d index \n", g_p61850MappingTbl[idxA].ps8leafname, idxA);
			UpdateVMDWithNewVal((char *)g_p61850MappingTbl[idxA].ps8leaftype, g_p61850MappingTbl[idxA].pvmdptr, (unsigned int*) &vlu);
			return idxA;
		}
	}
	//printf("Leaf %s Not Found \n", leafName);
	return -1;
}

/**
 * Write shared mem data for mms packet 32 bit floating point.
 * @param leafName: Name specified by the CID file.
 * @param vlu: 32 bit floating point value
 */
int writeVmdPointerByLeafName_float32(char *leafName, float vlu)
{
	int idxA = 0;
	for(idxA=0;idxA < (gs16leafnum - 2);idxA++)
	{
		if(strstr((char *)g_p61850MappingTbl[idxA].ps8leafname, leafName))
		{
			//printf("Found Leaf %s at %d index \n", g_p61850MappingTbl[idxA].ps8leafname, idxA);
			UpdateVMDWithNewVal((char *)g_p61850MappingTbl[idxA].ps8leaftype, g_p61850MappingTbl[idxA].pvmdptr,(unsigned int*) &vlu);
			return idxA;
		}
	}
	//printf("Leaf %s Not Found \n", leafName);
	return -1;
}

/**
 * Write shared mem data for mms packet unsigned 32 bit integer.
 * @param leafName: Name specified by the CID file.
 * @param vlu: 32 bit unsigned integer value
 */

int writeVmdPointerByLeafName_int32(char *leafName, U32 vlu)
{
	int idxA = 0;
	for(idxA=0;idxA < (gs16leafnum - 2);idxA++)
	{
		if(strstr((char *)g_p61850MappingTbl[idxA].ps8leafname, leafName))
		{
			//printf("Found Leaf %s at %d index \n", g_p61850MappingTbl[idxA].ps8leafname, idxA);
			UpdateVMDWithNewVal((char *)g_p61850MappingTbl[idxA].ps8leaftype, g_p61850MappingTbl[idxA].pvmdptr,(unsigned int*)&vlu);
			return idxA;
		}
	}
	//printf("Leaf %s Not Found \n", leafName);
	return -1;
}

// int writeVMDPointerByLeafName(int s32lnId, char* ps8leafName,  U32* u32newval)
// {
// 	char ps8fullLeafName[100];
// 	int s16leafnum = gs16leafnum;
// 	char* ps8pos = NULL;
// 	int s16len = 0;
// 	int s16tmp = 0;

// 	for(int i = 0; i < s16leafnum; i++)
// 	{
// 		if (s32lnId == g_p61850MappingTbl[i].s16parsedLNID)
// 		{		
// 			ps8pos = strchr(g_p61850MappingTbl[i].ps8leafname, '$');
// 			//printf("leafname : %s \n", g_p61850MappingTbl[i].ps8leafname);
// 			if(ps8pos != NULL)
// 			{
// 				s16len = ps8pos - (char*)(g_p61850MappingTbl[i].ps8leafname);
// 				strncpy(ps8fullLeafName, g_p61850MappingTbl[i].ps8leafname, s16len);
// 				ps8fullLeafName[s16len] = '\0';
// 				strcat(ps8fullLeafName, ps8leafName);
// 				//strncpy(ps8fullLeafName+s16len, ps8leafName,strlen(ps8leafName)+1);
// 			}
// 			if(!strcmp(g_p61850MappingTbl[i].ps8leafname, ps8fullLeafName))
// 			{
// 				//printf("full leaf : %s %s type : %s val : %f %f\n", g_p61850MappingTbl[i].ps8leafname,ps8fullLeafName, g_p61850MappingTbl[i].ps8leaftype, *(float*)g_p61850MappingTbl[i].pvmdptr, *(float*)u32newval);			
// 				if(u32newval)
// 					UpdateVMDWithNewVal(g_p61850MappingTbl[i].ps8leaftype, g_p61850MappingTbl[i].pvmdptr, u32newval);
// 				else
// 				{
// 					printf("Type is not known yet.\n");
// 					return -1;
// 				}
// 				return i;
// 			}
// 		}	
// 	}
// 	//printf("Matching ID Not Found!\n");
// 	return -1;
// }

/** Function for printing raw content from a buffer. Testing Purpose Only. */
void printSample_int16(short *buf16)
{
	int idxB = 0;
	printf("\n\n");
	for(idxB=0;idxB<100;idxB++)
	{
		printf("%d\n", buf16[idxB]);
	}
}

float voltss[100];
int initfinish;
int LU = 0;

int main(void)
{
	int i=0;
	int idx;
	int lidxA = 0;
	int nReturn = -1;
	char ps8command[250];
	glob_t stFileSearchResult;
	pthread_t testThreadId;
    struct timeval dsSysTime;
    unsigned long lastSyncTime = 0;  // Last time we did a sync
    unsigned long currentTime;
	initfinish = 0;
	CCM_AllocShareMem();
	/*
	InitPDSharedVariables();	// 2021. 12. 15. 
	InitGISSharedVariables();	// 2021. 12. 15. 
	InitPDASharedVariables();
	*/
	if(DBM_Init() == -1)
	{
		printf("[%s] %d DBM Init Fail!\n", __func__, __LINE__);
		usleep(200*1000);
		DBM_DeInit();
		return -1;
	}

	if(SharedMemMgr_MemCheck() == -1)
	{
		printf("Shared Memory checking is failed\n");
		usleep(200*1000);
		return -1;
	}
	else
	{
		/*
		 * Disable signal blocking. 
		 */
		//SignalBlocking();

		SSM_InitIPSetting();
		SSM_LoadNetworkInfo(SystemDB,(dsNetworkInfo*)&(g_ds61850SharedMem.dsDeviceInfo.dsNetWorkCfg));
		struct in_addr ip_addr;
		ip_addr.s_addr = htonl(g_ds61850SharedMem.dsDeviceInfo.dsNetWorkCfg[1].m_u32IPAddr);
		printf("NTP Server IP : %s\n", inet_ntoa(ip_addr));
		TSM_UpdateTimeSyncInfo();
		//printf("MPU IP : %s\n", inet_ntoa(ip_addr));		
 		// Thread ?$)Co?=m?
		// printf("Starting to read file Sanion_154kV_GIS_D01_1A.csv \n\n");
		// if(ReadFile("Sanion_154kV_GIS_D01_1A.csv") == -1)
		// {
		// 	printf("\n\n\n\nError Reading CSV File\n\n\n\n\n");
		// }
		// else
		// {
		// 	SearchLeafs("SBSH1", &sbsh_leaflist);
		// 	for(idx=0; idx < sbsh_leaflist.count; idx++)
		// 	{
		// 		GetKeyFromLeafString(sbsh_leaflist.lines[idx], leaf_key, "Sanion_154kV_GIS_D01_2AGLU");
		// 	}
		// }

		
		//newConnect();
		//sleep(2);
		//newConnect();
/*
		sleep(2);

		if(getVolts(voltss) == 0)
		{
			printf("Volt : %f", voltss[0]);
		}
		else
		{
			printf("\n\n\n\n\n\n\n\n\n\n\n\n ERROR READING ACQ\n");
		}

		sleep(2);

		if(getVolts(voltss) == 0)
		{
			printf("Volt : %f", voltss[0]);
		}
		else
		{
			printf("ERROR READING ACQ\n");
		}

		sleep(2);

		while(1)
		{
			if(getVolts(voltss) == 0)
			{
				printf("Volt    : %f\n", voltss[0]);
				printf("A-Phase : %f\n", voltss[2]);
				printf("B-Phase : %f\n", voltss[4]);
				printf("C-Phase : %f\n", voltss[6]);
			}
			else
			{
				printf("ERROR READING ACQ\n");
			}
			sleep(1);
		}
*/


/*		int vrsn = readxxxxRegisters();
		printf("Version %0X \n\n\n", vrsn);
*/
  
        gettimeofday(&dsSysTime, NULL);  // Get current time again to be precise
        g_ds61850SharedMem.dsArmDestUTC.secs = dsSysTime.tv_sec;
        g_ds61850SharedMem.dsArmDestUTC.usecs = (unsigned int)dsSysTime.tv_usec;
        g_ds61850SharedMem.dsArmDestUTC.fraction = (U32)((F32)(dsSysTime.tv_usec) / 1000000 * (F32)0x01000000);
        g_ds61850SharedMem.dsArmDestUTC.qflags = gs32SntpTimeQuality;
		if(gs32SntpTimeQuality == 0x8A)
			g_ds61850SharedMem.u8TimeSync = TRUE;
		else
			g_ds61850SharedMem.u8TimeSync = FALSE;
		nReturn  = glob("*.cid", 0, NULL, &stFileSearchResult); //if exist: 0, no match: 3
		if(nReturn == 0) // || nReturn == GLOB_NOMATCH)
		{
			memset((ds61850SharedMemory*)g_pSTBSharedMem->dsPhyDIMem.wPhyDI,0,sizeof(U16)*DEFS_DB_PHY_DI_MAX_NO);
			g_pSTBSharedMem->dsActGrp.m_u32Flag = DATA_SET_BY_MPU;
			g_pSTBSharedMem->dsPhyDIMem.dsArmDestUTC.m_u32Flag = DATA_READ_BY_61850;
			g_pSTBSharedMem->dsArmDestUTC.m_u32Flag = DATA_READ_BY_61850;
			g_ds61850SharedMem.dsArmDestUTC.m_u32Flag == DATA_READ_BY_61850;
			for(int i = 0; i < DEFS_DB_PHY_DO_HALFWORD_MAX_NO; i++)	
				g_pSTBSharedMem->dsTestDO.u16TestDO[i] = 0;
			for(int i = 0; i < DEFS_DB_PHY_DO_WORD_MAX_NO; i++)
				g_pSTBSharedMem->dsTestDO.u32TestDO[i] = 0;
			gu8is61850 = TRUE;
			
			Init_61850();
			g_p61850IfcInfo = (ds61850IfcInfo *)Get61850IfcInfo();
			if(g_p61850IfcInfo)
			{
				g_p61850MappingTbl = g_p61850IfcInfo->p61850mappingtbl;
			}
			
			printf("MPU IP Address updated to %s\n", g_p61850IfcInfo->ps8IPAddr);
			sprintf(ps8command, "./reconnect.sh %s & ", g_p61850IfcInfo->ps8IPAddr);				
			system(ps8command);
			printf("MPU Subnet updated to %s\n", g_p61850IfcInfo->ps8IP_SUBNET);
            printf("MPU GW updated to %s\n", g_p61850IfcInfo->ps8IP_GW);
			
		}
		else
			gu8is61850 = FALSE;
		
		if(RunThread() != 0)
			printf("Thread error\n");
				
	}
	
		////CDM_Logo();
	//}


	/******
	 * <SEPT-9>
	 */
	
	const ST_CHAR *ps8configfile = "LUConfig.json";

	if(!parseLUConfig(ps8configfile, &pdsluConfig))
	{
        printf("Error parsing 'LUConfig.json'\n");
	}
	else
	{
		for (ST_INT i = 0; i < pdsluConfig.s8numLUs; i++)
		{
			printf("-----\n");
			printf("----------------\n");
			printf("SUCCESS parsing 'LUConfig.json'\n");
			printf("LU Count :  %d\n\n-----------------\n---------\n", pdsluConfig.s8numLUs);

			for(lidxA=0;lidxA<pdsluConfig.s8numLUs;lidxA++)
			{
				printf("%s\n", pdsluConfig.LUList[i].ps8luIP);
			}
		}
	}
	
	/******
	 * </SEPT-9>
	 */


	for(lidxA=0;lidxA < g_p61850IfcInfo->s32leafnum;lidxA++)
	{
		if(strstr((char *)g_p61850MappingTbl[lidxA].ps8leafname, "GLU/"))
		{
			LU = GLU;
			printf("\n ----------- THIS IS GLU -----------\n");
			break;
		}
		else if(strstr((char *)g_p61850MappingTbl[lidxA].ps8leafname, "MLU/"))
		{
			LU = MLU;
			printf("\n ----------- THIS IS MLU -----------\n");
			break;
		}
	}

	//WDT_Init();
	//initfinish = 1;
	U16 u16WatchDogCnt = 0;
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

	//system("rm /COMTRADE/*;");
	//system("mkdir /COMTRADE;");

	return 0;
}



