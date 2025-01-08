#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>

#include <math.h>

#include "Common.h"
#include "global.h"
#include "UR_SDK.h"

#include <pthread.h>

#include "PowerSystem.h"
#include "GISMonitoring.h"

int ACQcounting[MAX_GIS_ACQS];
namebufferstruct filenamebuffs[MAX_GIS_ACQS];

extern dsTotalDataToCU*	acq1data;
extern dsTotalDataToCU*	acq2data;
extern dsTotalDataToCU*	acq3data;
extern dsTotalDataToCU*	acq4data;
extern dsTotalDataToCU*	acq5data;
extern dsTotalDataToCU*	acq6data;
extern dsTotalDataToCU*	acq7data;
extern dsTotalDataToCU*	acq8data;
extern dsTotalDataToCU*	acq9data;


dsACQBufferForIEC61850	gdsACQBufferForIEC61850[MAX_GIS_ACQS][SIZE_OF_ACQ_DATA_LIST_TO_CU];

dsACQWave	gdsACQWave[MAX_GIS_ACQS];


extern volatile dsAISharedMemory*	g_pAISharedMem; 


dsTotalDataToCU *mallocACQmemorypart()
{
	int i;
	dsTotalDataToCU* datastruct=0;


	datastruct = malloc((sizeof(U32*)*SIZE_OF_ACQ_DATA_LIST_TO_CU)+16+ sizeof(MMS_UTC_TIME2)*SIZE_OF_ACQ_DATA_LIST_TO_CU);
	datastruct->m_u32WriteIndex = 0;
	datastruct->m_u32ReadIndex = 0;
	datastruct->m_u32ListSize = SIZE_OF_ACQ_DATA_LIST_TO_CU;	// 100
	datastruct->m_u32Totallen = SIZE_OF_ACQ_DATA_UNIT;			// 72+6+1=79
	for (i=0; i<SIZE_OF_ACQ_DATA_LIST_TO_CU; i++) {
		datastruct->m_dsDataList[i].m_pData = malloc(SIZE_OF_ACQ_DATA_UNIT_IN_BYTES);
		//datastruct->m_timelist[i].secs = 0;
	}

	return datastruct;
}


void freeACQmemorypart(dsTotalDataToCU* datastruct)
{
	int i;


	for (i=0; i<SIZE_OF_ACQ_DATA_LIST_TO_CU; i++) {
		free(datastruct->m_dsDataList[i].m_pData);
	}

	free(datastruct);
}


U8 InitGISSharedVariables()
{
	// for ACQs
	acq1data=0;
	acq2data=0;
	acq3data=0;
	acq4data=0;
	acq5data=0;
	acq6data=0;
	acq7data=0;
	acq8data=0;
	acq9data=0;
	acq1data = mallocACQmemorypart();
	acq2data = mallocACQmemorypart();
	acq3data = mallocACQmemorypart();
	acq4data = mallocACQmemorypart();
	acq5data = mallocACQmemorypart();
	acq6data = mallocACQmemorypart();
	acq7data = mallocACQmemorypart();
	acq8data = mallocACQmemorypart();
	acq9data = mallocACQmemorypart();
	for (int i=0;i<MAX_GIS_ACQS;i++){
		ACQcounting[i] = 0;
		}
	
	return 1;
}


U8 InitPreventionAndMonitoringForGIS()
{
	U8 u8RV;
	int i, j;


	return 1;
}


void TerminatePreventionAndMonitoringForGIS()
{
	int i;


	freeACQmemorypart(acq1data);
	freeACQmemorypart(acq2data);
	freeACQmemorypart(acq3data);
	freeACQmemorypart(acq4data);
	freeACQmemorypart(acq5data);
	freeACQmemorypart(acq6data);
	freeACQmemorypart(acq7data);
	freeACQmemorypart(acq8data);
	freeACQmemorypart(acq9data);
}


// OLTC 구동 모터 전류 감시를 위한 계측값 읽기
void ReadMeasurementForACQMon()
{
	int i;
}


void WriteEachACQMeasurementToIEC61850(dsTotalDataToCU* datalist, U8 u8ACQNo)
{
	int i;
	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_CLOSE_OPERATION].m_u32Value			= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+1;
	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_ABRASION_ALARM].m_u32Value			= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+2;
	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_ABRASION_WARNING].m_u32Value			= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+3;
	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_OP_TIME_ALARM].m_u32Value			= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+4;
	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_COIL_ALARM].m_u32Value				= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+5;

	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_OP_CNT_ALARM].m_u32Value				= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+6;
	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_OP_CNT_WARNING].m_u32Value			= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+7;
	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_OP_TIME_WARNING].m_u32Value			= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+8;
	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_OP_TIME_SETTING_VALUE].m_u32Value	= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+9;
	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_EVTTRANSF].m_u32Value				= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+10;

	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_RSOPCNT].m_u32Value					= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+11;
	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_MODEVCOMF].m_u32Value				= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+12;
	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_MODEVFLT].m_u32Value					= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+13;
	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_CB_ALARM].m_u32Value					= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+14;
	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_ACCUMULATED_ABRASION].m_u32Value		= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+15;

	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_SWITCHING_I].m_u32Value				= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+16;
	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_RECENT_ABRASION].m_u32Value			= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+17;
	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_OPEN_AUX_SWITCH_TIME].m_u32Value		= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+18;
	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_CLOSE_AUX_SWITCH_TIME].m_u32Value	= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+19;
	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_OPEN_REACTION_TIME].m_u32Value		= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+20;

	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_CLOSE_REACTION_TIME].m_u32Value		= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+21;
	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_OPEN_OP_SPEED].m_u32Value			= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+22;
	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_CLOSE_OP_SPEED].m_u32Value			= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+23;
	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_OPEN_OP_DURATION].m_u32Value			= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+24;
	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_CLOSE_OP_DURATION].m_u32Value		= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+25;

	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_RELAY_CONTACT].m_u32Value			= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+26;
	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_OPEN_RELAY_OVERCONTACT].m_u32Value	= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+27;
	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_CLOSE_RELAY_OVERCONTACT].m_u32Value	= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+28;
	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_COIL_I].m_u32Value					= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+29;
	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_TEMPERATURE].m_u32Value				= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+30;

	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_COIL_V].m_u32Value					= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+31;
	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_RECENT_CLOSE_COIL_I].m_u32Value		= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+32;
	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_RECENT_OPEN_COIL_I].m_u32Value		= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+33;
	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_BUS_IA].m_u32Value					= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+34;
	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_BUS_IB].m_u32Value					= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+35;

	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_dsACQStatus[ENUM_ACQ_BUS_IC].m_u32Value					= (10000*u8ACQNo)+(100*datalist->m_u32WriteIndex)+36;

	for (i=0; i<NUM_KIND_ACQ_SETTING; i++) {
		gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_u32ACQSetting[ENUM_ACQ_ABRASION_ALARM_LEVEL+i]		= 400+i;
	}

	gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex].m_u32CRC	= (U32)CalculateCRC((U8*)(&(gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex])), (SIZE_OF_ACQ_DATA_UNIT_IN_BYTES-4));

	

	memcpy(datalist->m_dsDataList[datalist->m_u32WriteIndex].m_pData, &(gdsACQBufferForIEC61850[u8ACQNo][datalist->m_u32WriteIndex]), SIZE_OF_ACQ_DATA_UNIT_IN_BYTES);
	struct timeval dsSysTime;

	gettimeofday(&dsSysTime, NULL);
	
	//memcpy(&datalist->m_timelist[datalist->m_u32WriteIndex],&dsSysTime, sizeof(MMS_UTC_TIME2));
	datalist->m_timelist[datalist->m_u32WriteIndex].secs = dsSysTime.tv_sec;
	datalist->m_timelist[datalist->m_u32WriteIndex].fraction = dsSysTime.tv_usec;

	if (++datalist->m_u32WriteIndex == SIZE_OF_ACQ_DATA_LIST_TO_CU) {
		datalist->m_u32WriteIndex = 0;
	}
}


void CreateACQdat(U8 u8ACQNo, dsACQWaveData a_dsACQWaveData)
{
	FILE *fileToWrite;
	time_t now;
    struct tm *tm;
    now = time(0);
    if ((tm = localtime (&now)) == NULL) {
        printf ("Error extracting time stuff\n");
        return 1;
    }

	U8 *buffer=(char*)malloc(sizeof(a_dsACQWaveData));
	char filename[50];
	snprintf(filename,sizeof(filename),"/data/COMTRADE/EVENT/CBCM/%d_32_%04d%02d%02d%02d%02d%02d.dat",u8ACQNo,tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,tm->tm_hour, tm->tm_min, tm->tm_sec);
	
	
	if (ACQcounting[u8ACQNo] == filebuffermax){
		char temp[50];
		strcpy(temp, filenamebuffs[u8ACQNo].namebuffer[0]);
		//printf("filename: %s\n",temp);

		for(int i=1;i<filebuffermax;i++){
			strcpy(filenamebuffs[u8ACQNo].namebuffer[i-1],filenamebuffs[u8ACQNo].namebuffer[i]);
			}

		int status = remove(temp);

	    if( status == 0 ){
	       //printf("%s file deleted successfully.\n",temp);
	       }
	    else
	    {
	       printf("Unable to delete the file\n");
	       perror("Error");
	    }
		strcpy(filenamebuffs[u8ACQNo].namebuffer[filebuffermax-1],filename);
		}
	else if(ACQcounting[u8ACQNo]<filebuffermax){
		strcpy(filenamebuffs[u8ACQNo].namebuffer[ACQcounting[u8ACQNo]],filename);
		ACQcounting[u8ACQNo]++;
		}	

	//memcpy(namebuffer[ACQcounting],filename,sizeof(filename));
	memcpy(buffer, (const unsigned char*)&a_dsACQWaveData, sizeof(a_dsACQWaveData));

	if ((fileToWrite = fopen(filename, "wb+")) != NULL) {
	    fwrite(buffer, 1, sizeof(a_dsACQWaveData), fileToWrite);

	    fclose(fileToWrite);
	}

	free(buffer);
}


void WriteEachACQWaveToIEC61850(U8 u8ACQNo)
{
	int i;

	// for test
	#if (1)
	if ((u8ACQNo%2) == 0) {
		gdsACQWave[u8ACQNo].m_dsData.m_u8EventType	= 0x55;
	}
	else {
		gdsACQWave[u8ACQNo].m_dsData.m_u8EventType	= 0xAA;
	}

	gdsACQWave[u8ACQNo].m_dsData.m_u32EventTime		= (u8ACQNo+1)*0x11110000;
	gdsACQWave[u8ACQNo].m_dsData.m_u32EventTimeMsec	= (u8ACQNo+1)*0x00001111;

	for (i=0; i<(ACQ_WAVE_CYCLES*SAMPLES_PER_CYCLE); i++) {
		gdsACQWave[u8ACQNo].m_dsData.m_u16TripCoil1Current[i]	= 10+u8ACQNo;
		gdsACQWave[u8ACQNo].m_dsData.m_u16TripCoil2Current[i]	= 20+u8ACQNo;
		gdsACQWave[u8ACQNo].m_dsData.m_u16CloseCoilCurrent[i]	= 30+u8ACQNo;

		gdsACQWave[u8ACQNo].m_dsData.m_u16PhaseACurrent[i]		= (u8ACQNo+1)*cosf(DEGREE_TO_RADIAN*(i*(360./SAMPLES_PER_CYCLE)));
		gdsACQWave[u8ACQNo].m_dsData.m_u16PhaseBCurrent[i]		= (u8ACQNo+1)*cosf(DEGREE_TO_RADIAN*((i*(360./SAMPLES_PER_CYCLE))-120.));
		gdsACQWave[u8ACQNo].m_dsData.m_u16PhaseCCurrent[i]		= (u8ACQNo+1)*cosf(DEGREE_TO_RADIAN*((i*(360./SAMPLES_PER_CYCLE))-240.));

		gdsACQWave[u8ACQNo].m_dsData.m_u8Contacts[i]			= u8ACQNo+1;
	}

	//CreateACQdat(u8ACQNo, gdsACQWave[u8ACQNo].m_dsData);
	#endif
}


void ExecuteGISMonitoring()
{
	// for test
	#if (1)
	static int i=0;
	#endif

	// PDDAU I/F
	// OLTC driving motor monitoring
	ReadMeasurementForACQMon();

	// OLTC monitoring0
	WriteEachACQMeasurementToIEC61850(acq1data, 0);
	WriteEachACQMeasurementToIEC61850(acq2data, 1);
	WriteEachACQMeasurementToIEC61850(acq3data, 2);
	WriteEachACQMeasurementToIEC61850(acq4data, 3);
	WriteEachACQMeasurementToIEC61850(acq5data, 4);
	WriteEachACQMeasurementToIEC61850(acq6data, 5);
	WriteEachACQMeasurementToIEC61850(acq7data, 6);
	WriteEachACQMeasurementToIEC61850(acq8data, 7);
	WriteEachACQMeasurementToIEC61850(acq9data, 8);

	// for test
	#if (1)
	for (i = 0;i < MAX_GIS_ACQS;i++) {
		WriteEachACQWaveToIEC61850(i);
	}

	#endif

}

