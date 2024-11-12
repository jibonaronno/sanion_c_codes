#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>

#include <math.h>

#include "Common.h"
#include "global.h"

#include <pthread.h>

#include "PowerSystem.h"
#include "PDMonitoring.h"

extern dsTotalDataToCU*	pd1data;
extern dsTotalDataToCU*	pd2data;
extern dsTotalDataToCU*	pd3data;
extern dsTotalDataToCU*	pd4data;
extern dsTotalDataToCU*	pd5data;
extern dsTotalDataToCU*	pd6data;
extern dsTotalDataToCU*	pd7data;
extern dsTotalDataToCU*	pd8data;
extern dsTotalDataToCU*	pd9data;
extern dsTotalDataToCU*	pd10data;
extern dsTotalDataToCU*	pd11data;
extern dsTotalDataToCU*	pd12data;
extern dsTotalDataToCU*	pd13data;
extern dsTotalDataToCU*	pd14data;
extern dsTotalDataToCU*	pd15data;
extern dsTotalDataToCU*	pd16data;
extern dsTotalDataToCU*	pd17data;
extern dsTotalDataToCU*	pd18data;
extern dsTotalDataToCU*	pd19data;
extern dsTotalDataToCU*	pd20data;
extern dsTotalDataToCU*	pd21data;
extern dsTotalDataToCU*	pd22data;
extern dsTotalDataToCU*	pd23data;
extern dsTotalDataToCU*	pd24data;

// for DGAs
extern dsTotalDataToCU*	dga1data;
extern dsTotalDataToCU*	dga2data;
extern dsTotalDataToCU*	dga3data;
extern dsTotalDataToCU*	dga4data;
extern dsTotalDataToCU*	dga5data;
extern dsTotalDataToCU*	dga6data;

// for OLTCs
extern dsTotalDataToCU*	oltc1data;
extern dsTotalDataToCU*	oltc2data;
extern dsTotalDataToCU*	oltc3data;

// for Bushings
extern dsTotalDataToCU*	bsh1data;
extern dsTotalDataToCU*	bsh2data;

// for ACQs
extern dsTotalDataToCU*	acq1data;
extern dsTotalDataToCU*	acq2data;
extern dsTotalDataToCU*	acq3data;
extern dsTotalDataToCU*	acq4data;
extern dsTotalDataToCU*	acq5data;
extern dsTotalDataToCU*	acq6data;
extern dsTotalDataToCU*	acq7data;
extern dsTotalDataToCU*	acq8data;
extern dsTotalDataToCU*	acq9data;


dsPDBufferForIEC61850	gdsPDBufferForIEC61850[MAX_PD_CHANNELS][SIZE_OF_PD_DATA_LIST_TO_CU];


dsTotalDataToCU *mallocPDmemorypart()
{
	int i;
	dsTotalDataToCU* datastruct=0;


	datastruct = malloc((sizeof(U32*)*SIZE_OF_PD_DATA_LIST_TO_CU)+16 + sizeof(MMS_UTC_TIME2)*SIZE_OF_PD_DATA_LIST_TO_CU);
	datastruct->m_u32WriteIndex = 0;
	datastruct->m_u32ReadIndex = 0;
	datastruct->m_u32ListSize = SIZE_OF_PD_DATA_LIST_TO_CU;		// 100
	datastruct->m_u32Totallen = SIZE_OF_PD_DATA_UNIT;			// 72+6+1=79
	for (i=0; i<SIZE_OF_PD_DATA_LIST_TO_CU; i++) {
		datastruct->m_dsDataList[i].m_pData = malloc(SIZE_OF_PD_DATA_UNIT_IN_BYTES);
		//datastruct->m_timelist[i].secs = 0;
	}
	
	return datastruct;
}


void freePDmemorypart(dsTotalDataToCU* datastruct)
{
	int i;


	for (i=0; i<SIZE_OF_PD_DATA_LIST_TO_CU; i++) {
		free(datastruct->m_dsDataList[i].m_pData);
	}

	free(datastruct);
}


U8 InitPDSharedVariables()
{
	// for PDs
	pd1data=0;
	pd2data=0;
	pd3data=0;
	pd4data=0;
	pd5data=0;
	pd6data=0;
	pd7data=0;
	pd8data=0;
	pd9data=0;
	pd10data=0;
	pd11data=0;
	pd12data=0;
	pd13data=0;
	pd14data=0;
	pd15data=0;
	pd16data=0;
	pd17data=0;
	pd18data=0;
	pd19data=0;
	pd20data=0;
	pd21data=0;
	pd22data=0;
	pd23data=0;
	pd24data=0;

	pd1data = mallocPDmemorypart();
	pd2data = mallocPDmemorypart();
	pd3data = mallocPDmemorypart();
	pd4data = mallocPDmemorypart();
	pd5data = mallocPDmemorypart();
	pd6data = mallocPDmemorypart();
	pd7data = mallocPDmemorypart();
	pd8data = mallocPDmemorypart();
	pd9data = mallocPDmemorypart();
	pd10data = mallocPDmemorypart();
	pd11data = mallocPDmemorypart();
	pd12data = mallocPDmemorypart();
	pd13data = mallocPDmemorypart();
	pd14data = mallocPDmemorypart();
	pd15data = mallocPDmemorypart();
	pd16data = mallocPDmemorypart();
	pd17data = mallocPDmemorypart();
	pd18data = mallocPDmemorypart();
	pd19data = mallocPDmemorypart();
	pd20data = mallocPDmemorypart();
	pd21data = mallocPDmemorypart();
	pd22data = mallocPDmemorypart();
	pd23data = mallocPDmemorypart();
	pd24data = mallocPDmemorypart();

	return 1;
}


U8 InitPDMonitoring()
{
	U8 u8RV;
	int i, j;


	return 1;
}


void TerminatePDMonitoring()
{
	int i;


	freePDmemorypart(pd1data);
	freePDmemorypart(pd2data);
	freePDmemorypart(pd3data);
	freePDmemorypart(pd4data);
	freePDmemorypart(pd5data);
	freePDmemorypart(pd6data);
	freePDmemorypart(pd7data);
	freePDmemorypart(pd8data);
	freePDmemorypart(pd9data);
	freePDmemorypart(pd10data);
	freePDmemorypart(pd11data);
	freePDmemorypart(pd12data);
	freePDmemorypart(pd13data);
	freePDmemorypart(pd14data);
	freePDmemorypart(pd15data);
	freePDmemorypart(pd16data);
	freePDmemorypart(pd17data);
	freePDmemorypart(pd18data);
	freePDmemorypart(pd19data);
	freePDmemorypart(pd20data);
	freePDmemorypart(pd21data);
	freePDmemorypart(pd22data);
	freePDmemorypart(pd23data);
	freePDmemorypart(pd24data);
}


// OLTC ���� ���� ���� ���ø� ���� ������ �б�
void ReadPDMeasurement()
{
	int i;
}


void WriteEachPDMeasurementToIEC61850(dsTotalDataToCU* datalist, U8 u8PDChNo)
{
	int i;

	gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_dsPDStatus[ENUM_PD_DEVICE_TYPE].m_u32Value			= 50000+(1000*u8PDChNo)+(100*datalist->m_u32WriteIndex)+1;
	gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_dsPDStatus[ENUM_PD_PD_ALARM].m_u32Value				= 50000+(1000*u8PDChNo)+(100*datalist->m_u32WriteIndex)+2;
	gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_dsPDStatus[ENUM_PD_EVENT_LEVEL_STATE].m_u32Value		= 50000+(1000*u8PDChNo)+(100*datalist->m_u32WriteIndex)+3;
	gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_dsPDStatus[ENUM_PD_OP_COUNT].m_u32Value				= 50000+(1000*u8PDChNo)+(100*datalist->m_u32WriteIndex)+4;
	gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_dsPDStatus[ENUM_PD_TYPE_0].m_u32Value					= 50000+(1000*u8PDChNo)+(100*datalist->m_u32WriteIndex)+5;

	gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_dsPDStatus[ENUM_PD_TYPE_1].m_u32Value					= 50000+(1000*u8PDChNo)+(100*datalist->m_u32WriteIndex)+6;
	gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_dsPDStatus[ENUM_PD_TYPE_2].m_u32Value					= 50000+(1000*u8PDChNo)+(100*datalist->m_u32WriteIndex)+7;
	gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_dsPDStatus[ENUM_PD_TYPE_3].m_u32Value					= 50000+(1000*u8PDChNo)+(100*datalist->m_u32WriteIndex)+8;
	gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_dsPDStatus[ENUM_PD_TYPE_4].m_u32Value					= 50000+(1000*u8PDChNo)+(100*datalist->m_u32WriteIndex)+9;
	gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_dsPDStatus[ENUM_PD_TYPE_5].m_u32Value					= 50000+(1000*u8PDChNo)+(100*datalist->m_u32WriteIndex)+10;

	gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_dsPDStatus[ENUM_PD_TYPE_6].m_u32Value					= 50000+(1000*u8PDChNo)+(100*datalist->m_u32WriteIndex)+11;
	gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_dsPDStatus[ENUM_PD_TYPE_7].m_u32Value					= 50000+(1000*u8PDChNo)+(100*datalist->m_u32WriteIndex)+12;
	gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_dsPDStatus[ENUM_PD_TYPE_8].m_u32Value					= 50000+(1000*u8PDChNo)+(100*datalist->m_u32WriteIndex)+13;
	gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_dsPDStatus[ENUM_PD_TYPE_9].m_u32Value					= 50000+(1000*u8PDChNo)+(100*datalist->m_u32WriteIndex)+14;
	gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_dsPDStatus[ENUM_PD_TYPE_10].m_u32Value				= 50000+(1000*u8PDChNo)+(100*datalist->m_u32WriteIndex)+15;

	gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_dsPDStatus[ENUM_PD_TYPE_11].m_u32Value				= 50000+(1000*u8PDChNo)+(100*datalist->m_u32WriteIndex)+16;
	gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_dsPDStatus[ENUM_PD_MODEVCOMF].m_u32Value				= 50000+(1000*u8PDChNo)+(100*datalist->m_u32WriteIndex)+17;
	gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_dsPDStatus[ENUM_PD_MODEVFLT].m_u32Value				= 50000+(1000*u8PDChNo)+(100*datalist->m_u32WriteIndex)+18;
	gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_dsPDStatus[ENUM_PD_TRENDTRANSF].m_u32Value			= 50000+(1000*u8PDChNo)+(100*datalist->m_u32WriteIndex)+19;
	gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_dsPDStatus[ENUM_PD_EVTTRANSF].m_u32Value				= 50000+(1000*u8PDChNo)+(100*datalist->m_u32WriteIndex)+20;

	gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_dsPDStatus[ENUM_PD_RTTRANSF].m_u32Value				= 50000+(1000*u8PDChNo)+(100*datalist->m_u32WriteIndex)+21;
	gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_dsPDStatus[ENUM_PD_ACOUSTIC_PD].m_u32Value			= 50000+(1000*u8PDChNo)+(100*datalist->m_u32WriteIndex)+22;
	gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_dsPDStatus[ENUM_PD_MAX_APPARENT_PD_MAG].m_u32Value	= 50000+(1000*u8PDChNo)+(100*datalist->m_u32WriteIndex)+23;
	gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_dsPDStatus[ENUM_PD_MAX_APPARENT_PD_ANG].m_u32Value	= 50000+(1000*u8PDChNo)+(100*datalist->m_u32WriteIndex)+24;
	gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_dsPDStatus[ENUM_PD_AVG_DISCHARGE_I].m_u32Value		= 50000+(1000*u8PDChNo)+(100*datalist->m_u32WriteIndex)+25;

	gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_dsPDStatus[ENUM_PD_UHF_PD].m_u32Value					= 50000+(1000*u8PDChNo)+(100*datalist->m_u32WriteIndex)+26;
	gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_dsPDStatus[ENUM_PD_1ST_HARMONIC_RELATION].m_u32Value	= 50000+(1000*u8PDChNo)+(100*datalist->m_u32WriteIndex)+27;
	gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_dsPDStatus[ENUM_PD_2ND_HARMONIC_RELATION].m_u32Value	= 50000+(1000*u8PDChNo)+(100*datalist->m_u32WriteIndex)+28;
	gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_dsPDStatus[ENUM_PD_MAX_PD_MAG_CNT].m_u32Value			= 50000+(1000*u8PDChNo)+(100*datalist->m_u32WriteIndex)+29;
	gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_dsPDStatus[ENUM_PD_MAX_PD_ANG_CNT].m_u32Value			= 50000+(1000*u8PDChNo)+(100*datalist->m_u32WriteIndex)+30;

	//printf("testing write index: %d\n",datalist->m_u32WriteIndex);
	//printf("testing read index: %d\n",datalist->m_u32ReadIndex);
	//printf("testing: %d\n",gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_dsPDStatus[ENUM_PD_DEVICE_TYPE].m_u32Value);

	for (i=0; i<NUM_KIND_PD_SETTING; i++) {
		gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_u32PDSetting[ENUM_PD_EVENT_AMP_THRESHOLD1+i]		= 500+i;
	}

	gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex].m_u32CRC	= (U32)CalculateCRC((U8*)(&(gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex])), (SIZE_OF_PD_DATA_UNIT_IN_BYTES-4));

	memcpy(datalist->m_dsDataList[datalist->m_u32WriteIndex].m_pData, &(gdsPDBufferForIEC61850[u8PDChNo][datalist->m_u32WriteIndex]), SIZE_OF_PD_DATA_UNIT_IN_BYTES);

	struct timeval dsSysTime;

	gettimeofday(&dsSysTime, NULL);
	//printf("time: %d\n",dsSysTime.tv_sec);
	//memcpy(&datalist->m_timelist[datalist->m_u32WriteIndex],&dsSysTime, sizeof(MMS_UTC_TIME2));
	datalist->m_timelist[datalist->m_u32WriteIndex].secs = dsSysTime.tv_sec;
	datalist->m_timelist[datalist->m_u32WriteIndex].fraction = dsSysTime.tv_usec;
	//printf("time: %d\n",datalist->m_timelist[datalist->m_u32WriteIndex].secs);

	if (++datalist->m_u32WriteIndex == SIZE_OF_PD_DATA_LIST_TO_CU) {
		datalist->m_u32WriteIndex = 0;
	}
}


void ExecutePDMonitoring()
{
	// PDDAU I/F
	ReadPDMeasurement();
	WriteEachPDMeasurementToIEC61850(pd1data, 0);
	WriteEachPDMeasurementToIEC61850(pd2data, 1);
	WriteEachPDMeasurementToIEC61850(pd3data, 2);
	WriteEachPDMeasurementToIEC61850(pd4data, 3);
	WriteEachPDMeasurementToIEC61850(pd5data, 4);
	WriteEachPDMeasurementToIEC61850(pd6data, 5);
	WriteEachPDMeasurementToIEC61850(pd7data, 6);
	WriteEachPDMeasurementToIEC61850(pd8data, 7);
	WriteEachPDMeasurementToIEC61850(pd9data, 8);
	WriteEachPDMeasurementToIEC61850(pd10data, 9);
	WriteEachPDMeasurementToIEC61850(pd11data, 10);
	WriteEachPDMeasurementToIEC61850(pd12data, 11);
	WriteEachPDMeasurementToIEC61850(pd13data, 12);
	WriteEachPDMeasurementToIEC61850(pd14data, 13);
	WriteEachPDMeasurementToIEC61850(pd15data, 14);
	WriteEachPDMeasurementToIEC61850(pd16data, 15);
	WriteEachPDMeasurementToIEC61850(pd17data, 16);
	WriteEachPDMeasurementToIEC61850(pd18data, 17);
	WriteEachPDMeasurementToIEC61850(pd19data, 18);
	WriteEachPDMeasurementToIEC61850(pd20data, 19);
	WriteEachPDMeasurementToIEC61850(pd21data, 20);
	WriteEachPDMeasurementToIEC61850(pd22data, 21);
	WriteEachPDMeasurementToIEC61850(pd23data, 22);
	WriteEachPDMeasurementToIEC61850(pd24data, 23);
}

