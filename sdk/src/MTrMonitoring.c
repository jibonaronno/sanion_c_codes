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
#include <time.h>

#include "PowerSystem.h"
#include "MTrMonitoring.h"

//#include "modbus_tcp.h"

//#include "modbus.h"

int OLTCcounting[MAX_MTR_OLTCS];

namebufferstruct MTRfilenamebuffs[MAX_MTR_OLTCS];

extern dsTotalDataToCU*	dga1data;
extern dsTotalDataToCU*	dga2data;
extern dsTotalDataToCU*	dga3data;
extern dsTotalDataToCU*	dga4data;
extern dsTotalDataToCU*	dga5data;
extern dsTotalDataToCU*	dga6data;

extern dsTotalDataToCU*	oltc1data;
extern dsTotalDataToCU*	oltc2data;
extern dsTotalDataToCU*	oltc3data;

extern dsTotalDataToCU*	bsh1data;
extern dsTotalDataToCU*	bsh2data;


dsPowerSystem	gdsPowerSystem;

// DAU ���ο��� ����ϴ� ����
dsDgaMonCalRes	gdsDgaMonCalRes[MAX_MTR_TANKS];					// 154kV:3��, 345kV:6��

dsOLTCMonInfo	gdsOLTCMonInfo[MAX_MTR_OLTCS];

dsOLTCWave		gdsOLTCWave[MAX_MTR_OLTCS];

dsBshMonCalRes	gdsBshMonCalRes[MAX_MTR_BUSHINGS];

dsDgaBufferForIEC61850	gdsDgaBufferForIEC61850[MAX_MTR_TANKS][SIZE_OF_DGA_DATA_LIST_TO_CU];

dsOLTCBufferForIEC61850	gdsOLTCBufferForIEC61850[MAX_MTR_OLTCS][SIZE_OF_OLTC_DATA_LIST_TO_CU];

dsBSHBufferForIEC61850	gdsBSHBufferForIEC61850[MAX_MTR_BUSHINGS][SIZE_OF_BSH_DATA_LIST_TO_CU];

dsModbusTCPReadCommand	gdsModbusTCPReadCommand[MAX_MTR_TANKS];

// 2021. 12. 13. �ʿ� ���� �ڵ������� ������ ���� ���� ��
#if (0)
// CU���� �������̽��� ���� ����
dsShMemFD		gdsShMemFD;
dsTotalDataToCU	gdsDgaTotalDataToCU[MAX_MTR_TANKS];
dsTotalDataToCU	gdsOLTCTotalDataToCU[MAX_MTR_OLTCS];
dsTotalDataToCU	gdsBSHTotalDataToCU[MAX_MTR_BUSHINGS];
#endif

extern volatile dsAISharedMemory*	g_pAISharedMem; 


/* MODBUS TCP START */

#define BUF_SIZE 8

#define VOLTS_SIZE 48

/*
int newConnect(void)
{

	int tmp_result;
 	struct modbus_c* bus = NULL;

	int i;
	u16 my_response_data[BUF_SIZE];

	char *response = (char *)my_response_data;

   modbus_init();

   bus = new_modbus_tcp_master_hostname( "192.168.10.100", 100, 0, NULL);
   //bus = new_modbus_tcp_master_hostname( "localhost", 502, 0, NULL);
   //bus = new_modbus_tcp_master_hostname( "192.168.2.19", 502, 0, NULL);
   //bus = new_modbus_tcp_master_hostname( "localhost", 502, 0, NULL);
   if (bus == NULL)
   {
      //syslog(LOG_ERR, "Error - couldn't malloc modbus_c. \n");
	  printf("Error - 1\n");
	  return 1;
   }

   adjust_pend_timeout(bus, 4000000); // 4 s, default is 2 s
   
   if ( modbus_open(bus,0,1) != 0)
   {
      //syslog(LOG_ERR, "Error opening TCP socket.\n");
      modbus_destroy(bus);
	  ////printf("Error 2\n");
	  return 1;
   }
   else
   {
	printf("Modbus Opened \n");
	//modbus_close(bus);
	//printf("Modbus Closed\n\n");
   	//modbus_destroy(bus);
   }

   tmp_result = read_holding_registers(
                 bus,   // modbus to use
                 1,    // device ID to ask
                 1000,     // start address 
                 BUF_SIZE,     // datapoint count
                 my_response_data
                );

	if (tmp_result == 0)
   	{
      	for (i=0; i < BUF_SIZE; i++)
      	{
         	printf("Slot %02d, value %04x\n", i, my_response_data[i]);
      	}

		response[(BUF_SIZE * 2)-1] = 0;
		printf("%s\n\n",response);
   	}
   	else
   	{
      	printf("Hmm... error? : %s\n", modbus_error(tmp_result) );
   	}

   
	modbus_close(bus);
	modbus_destroy(bus);
   	modbus_cleanup();

   return 0;
}

int getVolts(float *ret)
{

	int tmp_result;
 	struct modbus_c* bus = NULL;

	int i;
	u16 my_response_data[VOLTS_SIZE];

	char *response = (char *)my_response_data;

   modbus_init();

   bus = new_modbus_tcp_master_hostname( "192.168.10.100", 100, 0, NULL);
   //bus = new_modbus_tcp_master_hostname( "localhost", 502, 0, NULL);
   //bus = new_modbus_tcp_master_hostname( "192.168.2.19", 502, 0, NULL);
   //bus = new_modbus_tcp_master_hostname( "localhost", 502, 0, NULL);
   if (bus == NULL)
   {
      //syslog(LOG_ERR, "Error - couldn't malloc modbus_c. \n");
	  printf("Error - 1\n");
	  return 1;
   }

   adjust_pend_timeout(bus, 4000000); // 4 s, default is 2 s
   
   if ( modbus_open(bus,0,1) != 0)
   {
      //syslog(LOG_ERR, "Error opening TCP socket.\n");
      modbus_destroy(bus);
	  ////printf("Error 2\n");
	  return 1;
   }
   else
   {
	printf("Modbus Opened \n");
	
   }

   tmp_result = read_holding_registers(
                 bus,   // modbus to use
                 1,    // device ID to ask
                 1100,     // start address 
                 24,     // datapoint count
                 my_response_data
                );

	if (tmp_result == 0)
   	{

		memcpy((void *)ret, (void *)my_response_data, 24);
   	}
   	else
   	{
      	printf("Hmm... error? : %s\n", modbus_error(tmp_result) );
		return 1;
   	}

   
	modbus_close(bus);
	modbus_destroy(bus);
   	modbus_cleanup();

   return 0;
}
*/

/* MODBUS TCP END */

/* MODBUS TCP START */
/*
modbus_t *ctx;

int newConnect(void)
{
	ctx = modbus_new_tcp("192.168.10.100", 100);
	if (modbus_connect(ctx) == -1) 
	{
		//fprintf(stderr, "Connection failed: \n", modbus_strerror(errno));
		printf("Modbus TCP Connection Failed \n");
		modbus_free(ctx);
		return -1;
	}
	
	return 1;
}

int readxxxxRegisters(void)
{
	uint16_t tab_reg[3];
	tab_reg[0] = -1;
	if(modbus_read_registers(ctx, 1024, 1, tab_reg) == -1)
	{
		printf("Modbus TCP Read Failed \n");
		return -1;
	}
	return tab_reg[0];
}
*/
/* MODBUS TCP END */


dsTotalDataToCU *mallocdgamemorypart()
{
	int i;
	dsTotalDataToCU* datastruct=0;


	datastruct = malloc((sizeof(U32*)*SIZE_OF_DGA_DATA_LIST_TO_CU)+16+ sizeof(MMS_UTC_TIME2)*SIZE_OF_DGA_DATA_LIST_TO_CU);
	datastruct->m_u32WriteIndex = 0;
	datastruct->m_u32ReadIndex = 0;
	datastruct->m_u32ListSize = SIZE_OF_DGA_DATA_LIST_TO_CU;	// 24
	datastruct->m_u32Totallen = SIZE_OF_DGA_DATA_UNIT;			// 81

	for (i=0; i<SIZE_OF_DGA_DATA_LIST_TO_CU; i++) {
		datastruct->m_dsDataList[i].m_pData = malloc(SIZE_OF_DGA_DATA_UNIT_IN_BYTES);
		//datastruct->m_timelist[i].secs = 0;
	}

	return datastruct;
}


dsTotalDataToCU *mallocoltcmemorypart()
{
	int i;
	dsTotalDataToCU* datastruct=0;


	datastruct = malloc((sizeof(U32*)*SIZE_OF_OLTC_DATA_LIST_TO_CU)+16+ sizeof(MMS_UTC_TIME2)*SIZE_OF_OLTC_DATA_LIST_TO_CU);
	datastruct->m_u32WriteIndex = 0;
	datastruct->m_u32ReadIndex = 0;
	datastruct->m_u32ListSize = SIZE_OF_OLTC_DATA_LIST_TO_CU;	// 100
	datastruct->m_u32Totallen = SIZE_OF_OLTC_DATA_UNIT;			// 83
	for (i=0; i<SIZE_OF_OLTC_DATA_LIST_TO_CU; i++) {
		datastruct->m_dsDataList[i].m_pData = malloc(SIZE_OF_OLTC_DATA_UNIT_IN_BYTES);
		//datastruct->m_timelist[i].secs = 0;
	}

	return datastruct;
}


dsTotalDataToCU *mallocbshmemorypart()
{
	int i;
	dsTotalDataToCU* datastruct=0;

	datastruct = malloc((sizeof(U32*)*SIZE_OF_BSH_DATA_LIST_TO_CU)+16 + sizeof(MMS_UTC_TIME2)*SIZE_OF_BSH_DATA_LIST_TO_CU);
	datastruct->m_u32WriteIndex = 0;
	datastruct->m_u32ReadIndex = 0;
	datastruct->m_u32ListSize = SIZE_OF_BSH_DATA_LIST_TO_CU;	// 100
	datastruct->m_u32Totallen = SIZE_OF_BSH_DATA_UNIT;			// 37
	for (i=0; i<SIZE_OF_BSH_DATA_LIST_TO_CU; i++) {
		datastruct->m_dsDataList[i].m_pData = malloc(SIZE_OF_BSH_DATA_UNIT_IN_BYTES);
		//datastruct->m_timelist[i].secs = 0;
	}

	return datastruct;
}


void freedgamemorypart(dsTotalDataToCU* datastruct)
{
	int i;


	for (i=0; i<SIZE_OF_DGA_DATA_LIST_TO_CU; i++) {
		free(datastruct->m_dsDataList[i].m_pData);
	}

	free(datastruct);
}


void freeoltcmemorypart(dsTotalDataToCU* datastruct)
{
	int i;


	for (i=0; i<SIZE_OF_OLTC_DATA_LIST_TO_CU; i++) {
		free(datastruct->m_dsDataList[i].m_pData);
	}

	free(datastruct);
}


void freebshmemorypart(dsTotalDataToCU* datastruct)
{
	int i;


	for (i=0; i<SIZE_OF_BSH_DATA_LIST_TO_CU; i++) {
		free(datastruct->m_dsDataList[i].m_pData);
	}

	free(datastruct);
}


U8 InitPDASharedVariables()
{
	// for DGAs
	dga1data=0;
	dga2data=0;
	dga3data=0;
	dga4data=0;
	dga5data=0;
	dga6data=0;	
	dga1data = mallocdgamemorypart();
	dga2data = mallocdgamemorypart();
	dga3data = mallocdgamemorypart();
	dga4data = mallocdgamemorypart();
	dga5data = mallocdgamemorypart();
	dga6data = mallocdgamemorypart();

	// for OLTCs
	oltc1data=0;
	oltc2data=0;
	oltc3data=0;
	oltc1data = mallocoltcmemorypart();
	oltc2data = mallocoltcmemorypart();
	oltc3data = mallocoltcmemorypart();

	// for Bushings
	bsh1data=0;
	bsh2data=0;
	bsh1data = mallocbshmemorypart();
	bsh2data = mallocbshmemorypart();

	for (int i=0;i<MAX_MTR_OLTCS;i++){
		OLTCcounting[i] = 0;
		}
	return 1;
}


// 2021. 12. 13. �ʿ� ���� �ڵ������� ������ ���� ���� ��
#if (0)
U8 InitPreventionAndMonitoringForDGA()
{
	int i;


	for (i=0; i<MAX_MTR_TANKS; i++) {
		memset(&(gdsDgaTotalDataToCU[i]), 0, sizeof(dsTotalDataToCU));

		gdsDgaTotalDataToCU[i].m_u32ListSize	= SIZE_OF_DGA_DATA_LIST_TO_CU;		
		gdsDgaTotalDataToCU[i].m_u32Totallen	= SIZE_OF_DGA_DATA_UNIT;
	}

	gdsShMemFD.m_ifd_DGAData[0] = shm_open(DGANAME1, O_CREAT | O_EXCL | O_RDWR, 0600);
	if (gdsShMemFD.m_ifd_DGAData[0] < 0) {
		printf("File descriptor for %s failed to be assigned", DGANAME1);
		close(gdsShMemFD.m_ifd_DGAData[0]);
		return 0;
	}
	else {
		ftruncate(gdsShMemFD.m_ifd_DGAData[0], TOTAL_SIZE_OF_DGA_DATA_LIST_TO_CU_IN_BYTES);
		gdsDgaTotalDataToCU[0].m_dsDataList[0].m_pData = (U32*)mmap(0, TOTAL_SIZE_OF_DGA_DATA_LIST_TO_CU_IN_BYTES, PROT_READ | PROT_WRITE, MAP_SHARED, gdsShMemFD.m_ifd_DGAData[0], 0);
		printf("Mapped address for %s with size %d of each data : %p\n", DGANAME1, sizeof(*gdsDgaTotalDataToCU[0].m_dsDataList[0].m_pData), gdsDgaTotalDataToCU[0].m_dsDataList[0].m_pData);
	}
	for (i=1; i<SIZE_OF_DGA_DATA_LIST_TO_CU; i++) {
		gdsDgaTotalDataToCU[0].m_dsDataList[i].m_pData	= (U32*)((U32)(gdsDgaTotalDataToCU[0].m_dsDataList[i-1].m_pData) + (U32)SIZE_OF_DGA_DATA_UNIT_IN_BYTES);
	}

	gdsShMemFD.m_ifd_DGAData[1] = shm_open(DGANAME2, O_CREAT | O_EXCL | O_RDWR, 0600);
	if (gdsShMemFD.m_ifd_DGAData[1] < 0) {
		printf("File descriptor for %s failed to be assigned", DGANAME2);
		close(gdsShMemFD.m_ifd_DGAData[1]);
		return 0;
	}
	else {
		ftruncate(gdsShMemFD.m_ifd_DGAData[1], TOTAL_SIZE_OF_DGA_DATA_LIST_TO_CU_IN_BYTES);
		gdsDgaTotalDataToCU[1].m_dsDataList[0].m_pData = (U32*)mmap(0, TOTAL_SIZE_OF_DGA_DATA_LIST_TO_CU_IN_BYTES, PROT_READ | PROT_WRITE, MAP_SHARED, gdsShMemFD.m_ifd_DGAData[1], 0);
		printf("Mapped address for %s with size %d of each data : %p\n", DGANAME2, sizeof(*gdsDgaTotalDataToCU[1].m_dsDataList[0].m_pData), gdsDgaTotalDataToCU[1].m_dsDataList[0].m_pData);
	}
	for (i=1; i<SIZE_OF_DGA_DATA_LIST_TO_CU; i++) {
		gdsDgaTotalDataToCU[1].m_dsDataList[i].m_pData	= (U32*)((U32)(gdsDgaTotalDataToCU[1].m_dsDataList[i-1].m_pData) + (U32)SIZE_OF_DGA_DATA_UNIT_IN_BYTES);
	}

	gdsShMemFD.m_ifd_DGAData[2] = shm_open(DGANAME3, O_CREAT | O_EXCL | O_RDWR, 0600);
	if (gdsShMemFD.m_ifd_DGAData[2] < 0) {
		printf("File descriptor for %s failed to be assigned", DGANAME3);
		close(gdsShMemFD.m_ifd_DGAData[2]);
		return 0;
	}
	else {
		ftruncate(gdsShMemFD.m_ifd_DGAData[2], TOTAL_SIZE_OF_DGA_DATA_LIST_TO_CU_IN_BYTES);
		gdsDgaTotalDataToCU[2].m_dsDataList[0].m_pData = (U32*)mmap(0, TOTAL_SIZE_OF_DGA_DATA_LIST_TO_CU_IN_BYTES, PROT_READ | PROT_WRITE, MAP_SHARED, gdsShMemFD.m_ifd_DGAData[2], 0);
		printf("Mapped address for %s with size %d of each data : %p\n", DGANAME3, sizeof(*gdsDgaTotalDataToCU[2].m_dsDataList[0].m_pData), gdsDgaTotalDataToCU[2].m_dsDataList[0].m_pData);
	}
	for (i=1; i<SIZE_OF_DGA_DATA_LIST_TO_CU; i++) {
		gdsDgaTotalDataToCU[2].m_dsDataList[i].m_pData	= (U32*)((U32)(gdsDgaTotalDataToCU[2].m_dsDataList[i-1].m_pData) + (U32)SIZE_OF_DGA_DATA_UNIT_IN_BYTES);
	}

	gdsShMemFD.m_ifd_DGAData[3] = shm_open(DGANAME4, O_CREAT | O_EXCL | O_RDWR, 0600);
	if (gdsShMemFD.m_ifd_DGAData[3] < 0) {
		printf("File descriptor for %s failed to be assigned", DGANAME4);
		close(gdsShMemFD.m_ifd_DGAData[3]);
		return 0;
	}
	else {
		ftruncate(gdsShMemFD.m_ifd_DGAData[3], TOTAL_SIZE_OF_DGA_DATA_LIST_TO_CU_IN_BYTES);
		gdsDgaTotalDataToCU[3].m_dsDataList[0].m_pData = (U32*)mmap(0, TOTAL_SIZE_OF_DGA_DATA_LIST_TO_CU_IN_BYTES, PROT_READ | PROT_WRITE, MAP_SHARED, gdsShMemFD.m_ifd_DGAData[3], 0);
		printf("Mapped address for %s with size %d of each data : %p\n", DGANAME4, sizeof(*gdsDgaTotalDataToCU[3].m_dsDataList[0].m_pData), gdsDgaTotalDataToCU[3].m_dsDataList[0].m_pData);
	}
	for (i=1; i<SIZE_OF_DGA_DATA_LIST_TO_CU; i++) {
		gdsDgaTotalDataToCU[3].m_dsDataList[i].m_pData	= (U32*)((U32)(gdsDgaTotalDataToCU[3].m_dsDataList[i-1].m_pData) + (U32)SIZE_OF_DGA_DATA_UNIT_IN_BYTES);
	}

	gdsShMemFD.m_ifd_DGAData[4] = shm_open(DGANAME5, O_CREAT | O_EXCL | O_RDWR, 0600);
	if (gdsShMemFD.m_ifd_DGAData[4] < 0) {
		printf("File descriptor for %s failed to be assigned", DGANAME5);
		close(gdsShMemFD.m_ifd_DGAData[4]);
		return 0;
	}
	else {
		ftruncate(gdsShMemFD.m_ifd_DGAData[4], TOTAL_SIZE_OF_DGA_DATA_LIST_TO_CU_IN_BYTES);
		gdsDgaTotalDataToCU[4].m_dsDataList[0].m_pData = (U32*)mmap(0, TOTAL_SIZE_OF_DGA_DATA_LIST_TO_CU_IN_BYTES, PROT_READ | PROT_WRITE, MAP_SHARED, gdsShMemFD.m_ifd_DGAData[4], 0);
		printf("Mapped address for %s with size %d of each data : %p\n", DGANAME5, sizeof(*gdsDgaTotalDataToCU[4].m_dsDataList[0].m_pData), gdsDgaTotalDataToCU[4].m_dsDataList[0].m_pData);
	}
	for (i=1; i<SIZE_OF_DGA_DATA_LIST_TO_CU; i++) {
		gdsDgaTotalDataToCU[4].m_dsDataList[i].m_pData	= (U32*)((U32)(gdsDgaTotalDataToCU[4].m_dsDataList[i-1].m_pData) + (U32)SIZE_OF_DGA_DATA_UNIT_IN_BYTES);
	}

	gdsShMemFD.m_ifd_DGAData[5] = shm_open(DGANAME6, O_CREAT | O_EXCL | O_RDWR, 0600);
	if (gdsShMemFD.m_ifd_DGAData[5] < 0) {
		printf("File descriptor for %s failed to be assigned", DGANAME6);
		close(gdsShMemFD.m_ifd_DGAData[5]);
		return 0;
	}
	else {
		ftruncate(gdsShMemFD.m_ifd_DGAData[5], TOTAL_SIZE_OF_DGA_DATA_LIST_TO_CU_IN_BYTES);
		gdsDgaTotalDataToCU[5].m_dsDataList[0].m_pData = (U32*)mmap(0, TOTAL_SIZE_OF_DGA_DATA_LIST_TO_CU_IN_BYTES, PROT_READ | PROT_WRITE, MAP_SHARED, gdsShMemFD.m_ifd_DGAData[5], 0);
		printf("Mapped address for %s with size %d of each data : %p\n", DGANAME6, sizeof(*gdsDgaTotalDataToCU[5].m_dsDataList[0].m_pData), gdsDgaTotalDataToCU[5].m_dsDataList[0].m_pData);
	}
	for (i=1; i<SIZE_OF_DGA_DATA_LIST_TO_CU; i++) {
		gdsDgaTotalDataToCU[5].m_dsDataList[i].m_pData	= (U32*)((U32)(gdsDgaTotalDataToCU[5].m_dsDataList[i-1].m_pData) + (U32)SIZE_OF_DGA_DATA_UNIT_IN_BYTES);
	}

	return 1;
}


U8 InitPreventionAndMonitoringForOLTC()
{
	int i;


	for (i=0; i<MAX_MTR_OLTCS; i++) {
		memset(&(gdsOLTCTotalDataToCU[i]), 0, sizeof(dsTotalDataToCU));

		gdsOLTCTotalDataToCU[i].m_u32ListSize	= SIZE_OF_OLTC_DATA_LIST_TO_CU;		
		gdsOLTCTotalDataToCU[i].m_u32Totallen	= SIZE_OF_OLTC_DATA_UNIT;
	}

	gdsShMemFD.m_ifd_OLTCData[0] = shm_open(OLTCNAME1, O_CREAT | O_EXCL | O_RDWR, 0600);
	if (gdsShMemFD.m_ifd_OLTCData[0] < 0) {
		printf("File descriptor for %s failed to be assigned", OLTCNAME1);
		close(gdsShMemFD.m_ifd_OLTCData[0]);
		return 0;
	}
	else {
		ftruncate(gdsShMemFD.m_ifd_OLTCData[0], TOTAL_SIZE_OF_OLTC_DATA_LIST_TO_CU_IN_BYTES);
		gdsOLTCTotalDataToCU[0].m_dsDataList[0].m_pData = (U32*)mmap(0, TOTAL_SIZE_OF_OLTC_DATA_LIST_TO_CU_IN_BYTES, PROT_READ | PROT_WRITE, MAP_SHARED, gdsShMemFD.m_ifd_OLTCData[0], 0);
		printf("Mapped address for %s with size %d of each data : %p\n", OLTCNAME1, sizeof(*gdsOLTCTotalDataToCU[0].m_dsDataList[0].m_pData), gdsOLTCTotalDataToCU[0].m_dsDataList[0].m_pData);
	}
	for (i=1; i<SIZE_OF_OLTC_DATA_LIST_TO_CU; i++) {
		gdsOLTCTotalDataToCU[0].m_dsDataList[i].m_pData	= (U32*)((U32)(gdsOLTCTotalDataToCU[0].m_dsDataList[i-1].m_pData) + (U32)SIZE_OF_OLTC_DATA_UNIT_IN_BYTES);
	}

	gdsShMemFD.m_ifd_OLTCData[1] = shm_open(OLTCNAME2, O_CREAT | O_EXCL | O_RDWR, 0600);
	if (gdsShMemFD.m_ifd_OLTCData[1] < 0) {
		printf("File descriptor for %s failed to be assigned", OLTCNAME2);
		close(gdsShMemFD.m_ifd_OLTCData[1]);
		return 0;
	}
	else {
		ftruncate(gdsShMemFD.m_ifd_OLTCData[1], TOTAL_SIZE_OF_OLTC_DATA_LIST_TO_CU_IN_BYTES);
		gdsOLTCTotalDataToCU[1].m_dsDataList[0].m_pData = (U32*)mmap(0, TOTAL_SIZE_OF_OLTC_DATA_LIST_TO_CU_IN_BYTES, PROT_READ | PROT_WRITE, MAP_SHARED, gdsShMemFD.m_ifd_OLTCData[1], 0);
		printf("Mapped address for %s with size %d of each data : %p\n", OLTCNAME2, sizeof(*gdsOLTCTotalDataToCU[1].m_dsDataList[0].m_pData), gdsOLTCTotalDataToCU[1].m_dsDataList[0].m_pData);
	}
	for (i=1; i<SIZE_OF_OLTC_DATA_LIST_TO_CU; i++) {
		gdsOLTCTotalDataToCU[1].m_dsDataList[i].m_pData	= (U32*)((U32)(gdsOLTCTotalDataToCU[1].m_dsDataList[i-1].m_pData) + (U32)SIZE_OF_OLTC_DATA_UNIT_IN_BYTES);
	}

	gdsShMemFD.m_ifd_OLTCData[2] = shm_open(OLTCNAME3, O_CREAT | O_EXCL | O_RDWR, 0600);
	if (gdsShMemFD.m_ifd_OLTCData[2] < 0) {
		printf("File descriptor for %s failed to be assigned", OLTCNAME3);
		close(gdsShMemFD.m_ifd_OLTCData[2]);
		return 0;
	}
	else {
		ftruncate(gdsShMemFD.m_ifd_OLTCData[2], TOTAL_SIZE_OF_OLTC_DATA_LIST_TO_CU_IN_BYTES);
		gdsOLTCTotalDataToCU[2].m_dsDataList[0].m_pData = (U32*)mmap(0, TOTAL_SIZE_OF_OLTC_DATA_LIST_TO_CU_IN_BYTES, PROT_READ | PROT_WRITE, MAP_SHARED, gdsShMemFD.m_ifd_OLTCData[2], 0);
		printf("Mapped address for %s with size %d of each data : %p\n", OLTCNAME3, sizeof(*gdsOLTCTotalDataToCU[2].m_dsDataList[0].m_pData), gdsOLTCTotalDataToCU[2].m_dsDataList[0].m_pData);
	}
	for (i=1; i<SIZE_OF_OLTC_DATA_LIST_TO_CU; i++) {
		gdsOLTCTotalDataToCU[2].m_dsDataList[i].m_pData	= (U32*)((U32)(gdsOLTCTotalDataToCU[2].m_dsDataList[i-1].m_pData) + (U32)SIZE_OF_OLTC_DATA_UNIT_IN_BYTES);
	}

	return 1;
}


U8 InitPreventionAndMonitoringForBSH()
{
	int i;


	for (i=0; i<MAX_MTR_BUSHINGS; i++) {
		memset(&(gdsBSHTotalDataToCU[i]), 0, sizeof(dsTotalDataToCU));

		gdsBSHTotalDataToCU[i].m_u32ListSize	= SIZE_OF_BSH_DATA_LIST_TO_CU;		
		gdsBSHTotalDataToCU[i].m_u32Totallen	= SIZE_OF_BSH_DATA_UNIT;
	}

	gdsShMemFD.m_ifd_BSHData[0] = shm_open(BSHNAME1, O_CREAT | O_EXCL | O_RDWR, 0600);
	if (gdsShMemFD.m_ifd_BSHData[0] < 0) {
		printf("File descriptor for %s failed to be assigned", BSHNAME1);
		close(gdsShMemFD.m_ifd_BSHData[0]);
		return 0;
	}
	else {
		ftruncate(gdsShMemFD.m_ifd_BSHData[0], TOTAL_SIZE_OF_BSH_DATA_LIST_TO_CU_IN_BYTES);
		gdsBSHTotalDataToCU[0].m_dsDataList[0].m_pData = (U32*)mmap(0, TOTAL_SIZE_OF_BSH_DATA_LIST_TO_CU_IN_BYTES, PROT_READ | PROT_WRITE, MAP_SHARED, gdsShMemFD.m_ifd_BSHData[0], 0);
		printf("Mapped address for %s with size %d of each data : %p\n", BSHNAME1, sizeof(*gdsBSHTotalDataToCU[0].m_dsDataList[0].m_pData), gdsBSHTotalDataToCU[0].m_dsDataList[0].m_pData);
	}
	for (i=1; i<SIZE_OF_BSH_DATA_LIST_TO_CU; i++) {
		gdsBSHTotalDataToCU[0].m_dsDataList[i].m_pData	= (U32*)((U32)(gdsBSHTotalDataToCU[0].m_dsDataList[i-1].m_pData) + (U32)SIZE_OF_BSH_DATA_UNIT_IN_BYTES);
	}

	gdsShMemFD.m_ifd_BSHData[1] = shm_open(BSHNAME2, O_CREAT | O_EXCL | O_RDWR, 0600);
	if (gdsShMemFD.m_ifd_BSHData[1] < 0) {
		printf("File descriptor for %s failed to be assigned", BSHNAME2);
		close(gdsShMemFD.m_ifd_BSHData[1]);
		return 0;
	}
	else {
		ftruncate(gdsShMemFD.m_ifd_BSHData[1], TOTAL_SIZE_OF_BSH_DATA_LIST_TO_CU_IN_BYTES);
		gdsBSHTotalDataToCU[1].m_dsDataList[0].m_pData = (U32*)mmap(0, TOTAL_SIZE_OF_BSH_DATA_LIST_TO_CU_IN_BYTES, PROT_READ | PROT_WRITE, MAP_SHARED, gdsShMemFD.m_ifd_BSHData[1], 0);
		printf("Mapped address for %s with size %d of each data : %p\n", BSHNAME2, sizeof(*gdsBSHTotalDataToCU[1].m_dsDataList[0].m_pData), gdsBSHTotalDataToCU[1].m_dsDataList[0].m_pData);
	}
	for (i=1; i<SIZE_OF_BSH_DATA_LIST_TO_CU; i++) {
		gdsBSHTotalDataToCU[1].m_dsDataList[i].m_pData	= (U32*)((U32)(gdsBSHTotalDataToCU[1].m_dsDataList[i-1].m_pData) + (U32)SIZE_OF_BSH_DATA_UNIT_IN_BYTES);
	}

	return 1;
}

U8 InitPreventionAndMonitoringForMTr()
{
	U8 u8RV;
	int i, j;


	gdsPowerSystem.m_u8GridVoltage	= GRID_V_154kV;

	memset(&(gdsShMemFD), 0, sizeof(dsShMemFD));

	// Initialization for DGA monitoring
	u8RV = InitPreventionAndMonitoringForDGA();

	// Initialization for OLTC monitoring
	u8RV = InitPreventionAndMonitoringForOLTC();

	// Initialization for bushing monitoring
	u8RV = InitPreventionAndMonitoringForBSH();

	return(u8RV);
}
#else
U8 InitPreventionAndMonitoringForMTr()
{
	U8 u8RV;
	int i, j;

	gdsPowerSystem.m_u8GridVoltage	= GRID_V_154kV;

	for (i=0; i<MAX_MTR_OLTCS; i++) {
		memset(&(gdsOLTCWave[i]), 0, sizeof(dsOLTCWave));

		for (j=0; j<SIZE_OF_OLTC_DATA_LIST_TO_CU; j++) {
			memset(&(gdsOLTCBufferForIEC61850[i][j]), 0, sizeof(dsOLTCBufferForIEC61850));
		}
	}

	for (i=0; i<MAX_MTR_BUSHINGS; i++) {
		for (j=0; j<SIZE_OF_BSH_DATA_LIST_TO_CU; j++) {
			memset(&(gdsBSHBufferForIEC61850[i][j]), 0, sizeof(dsBSHBufferForIEC61850));
		}
	}

	for (i=0; i<MAX_MTR_TANKS; i++) {
		memset(&(gdsModbusTCPReadCommand[i]), 0, sizeof(dsModbusTCPReadCommand));

		gdsModbusTCPReadCommand[i].m_u16ProtocolID	= 0;	// Modbus-TCP = 0
		gdsModbusTCPReadCommand[i].m_u8UnitID		= 1;	// Tcpport = 1
		gdsModbusTCPReadCommand[i].m_u16Length		= 6;	// Length = 6 [= Unit ID (1) + Function code (1) + Data (4)]
	}

	return 1;
}
#endif


void TerminatePreventionAndMonitoringForMTr()
{
	int i;


	freedgamemorypart(dga1data);
	freedgamemorypart(dga2data);
	freedgamemorypart(dga3data);
	freedgamemorypart(dga4data);
	freedgamemorypart(dga5data);
	freedgamemorypart(dga6data);

	freeoltcmemorypart(oltc1data);
	freeoltcmemorypart(oltc2data);
	freeoltcmemorypart(oltc3data);

	freebshmemorypart(bsh1data);
	freebshmemorypart(bsh2data);
}


// Modbus-TCP �������̽� ���� ���̶� �ϴ� �ּ� ó��
#if (0)
void SendModbusTCPReadCommand(U8 u8DGANo, U16 u16TransactionID, U8 u8FC, U16 u16StartAddr, U16 u16ReadLength)
{
	// Transaction ID ����
	gdsModbusTCPReadCommand[u8DGANo].m_u16TransactionID	= u16TransactionID;
	// Function code
	gdsModbusTCPReadCommand[u8DGANo].m_u8FunctionCode	= u8FC;
	// Start address
	gdsModbusTCPReadCommand[u8DGANo].m_u16StartAddr		= u16StartAddr;
	// Length of reading
	gdsModbusTCPReadCommand[u8DGANo].m_u16ReadLength	= u16ReadLength;

	// WRITE_TO_TCP
}


void ReadModbusTCPReadResponse(U16 u16ReadLength)
{
	// READ_FROM_TCP : [9+(2*u16ReadLength)] ��ŭ�� ����Ʈ�� �д´�.

	gdsModbusTCPReadRes
}


void ReadDGAMeasurement()
{
	int i;


	for (i=0; i<MAX_MTR_TANKS; i++) {
		SendModbusTCPReadCommand(i, U16 u16TransactionID, MODBUS_TCP_FC_READ_REGISTERS, MINITRANS_ADDR_OIL_SOURCE_A_MEAS, MINITRANS_LENGTH_OIL_SOURCE_A_MEAS);
	}
}
#endif

// 2021. 12. 13. �ʿ� ���� �ڵ������� ������ ���� ���� ��
#if (0)
void WriteEachDGAMeasurementToIEC61850(U8 u8DGANo)
{
	// �ӽ÷� ���� ����
	#if (1)
	gdsDgaMonCalRes[u8DGANo].m_fTemperature	= (100.*u8DGANo)+(float)gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex+0.12;
	gdsDgaMonCalRes[u8DGANo].m_fLevel		= (100.*u8DGANo)+(float)gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex+0.34;
	gdsDgaMonCalRes[u8DGANo].m_fPressure	= (100.*u8DGANo)+(float)gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex+0.56;
	gdsDgaMonCalRes[u8DGANo].m_fH2Oppm		= (100.*u8DGANo)+(float)gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex+0.78;
	gdsDgaMonCalRes[u8DGANo].m_fH2ppm		= (100.*u8DGANo)+(float)gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex+0.90;
	gdsDgaMonCalRes[u8DGANo].m_fN2ppm		= (100.*u8DGANo)+(float)gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex+0.13;
	gdsDgaMonCalRes[u8DGANo].m_fCOppm		= (100.*u8DGANo)+(float)gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex+0.24;
	gdsDgaMonCalRes[u8DGANo].m_fCO2ppm		= (100.*u8DGANo)+(float)gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex+0.35;
	gdsDgaMonCalRes[u8DGANo].m_fCH4ppm		= (100.*u8DGANo)+(float)gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex+0.46;
	gdsDgaMonCalRes[u8DGANo].m_fC2H2ppm		= (100.*u8DGANo)+(float)gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex+0.57;
	gdsDgaMonCalRes[u8DGANo].m_fC2H4ppm		= (100.*u8DGANo)+(float)gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex+0.68;
	gdsDgaMonCalRes[u8DGANo].m_fC2H6ppm		= (100.*u8DGANo)+(float)gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex+0.79;
	gdsDgaMonCalRes[u8DGANo].m_fO2ppm		= (100.*u8DGANo)+(float)gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex+0.80;
	gdsDgaMonCalRes[u8DGANo].m_fC3H8ppm		= (100.*u8DGANo)+(float)gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex+0.91;

	gdsDgaBufferForIEC61850[u8DGANo][gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_TEMPERATURE].m_u32Value		= (U32)(gdsDgaMonCalRes[u8DGANo].m_fTemperature*100.);
	gdsDgaBufferForIEC61850[u8DGANo][gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_LEVEL].m_u32Value			= (U32)(gdsDgaMonCalRes[u8DGANo].m_fLevel*100.);
	gdsDgaBufferForIEC61850[u8DGANo][gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_PRESSURE].m_u32Value		= (U32)(gdsDgaMonCalRes[u8DGANo].m_fPressure*100.);
	gdsDgaBufferForIEC61850[u8DGANo][gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_H2O_PPM].m_u32Value			= (U32)(gdsDgaMonCalRes[u8DGANo].m_fH2Oppm*100.);
	gdsDgaBufferForIEC61850[u8DGANo][gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_H2_PPM].m_u32Value			= (U32)(gdsDgaMonCalRes[u8DGANo].m_fH2ppm*100.);
	gdsDgaBufferForIEC61850[u8DGANo][gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_N2_PPM].m_u32Value			= (U32)(gdsDgaMonCalRes[u8DGANo].m_fN2ppm*100.);
	gdsDgaBufferForIEC61850[u8DGANo][gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_CO_PPM].m_u32Value			= (U32)(gdsDgaMonCalRes[u8DGANo].m_fCOppm*100.);
	gdsDgaBufferForIEC61850[u8DGANo][gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_CO2_PPM].m_u32Value			= (U32)(gdsDgaMonCalRes[u8DGANo].m_fCO2ppm*100.);
	gdsDgaBufferForIEC61850[u8DGANo][gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_CH4_PPM].m_u32Value			= (U32)(gdsDgaMonCalRes[u8DGANo].m_fCH4ppm*100.);
	gdsDgaBufferForIEC61850[u8DGANo][gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_C2H2_PPM].m_u32Value		= (U32)(gdsDgaMonCalRes[u8DGANo].m_fC2H2ppm*100.);
	gdsDgaBufferForIEC61850[u8DGANo][gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_C2H4_PPM].m_u32Value		= (U32)(gdsDgaMonCalRes[u8DGANo].m_fC2H4ppm*100.);
	gdsDgaBufferForIEC61850[u8DGANo][gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_C2H6_PPM].m_u32Value		= (U32)(gdsDgaMonCalRes[u8DGANo].m_fC2H6ppm*100.);
	gdsDgaBufferForIEC61850[u8DGANo][gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_O2_PPM].m_u32Value			= (U32)(gdsDgaMonCalRes[u8DGANo].m_fO2ppm*100.);
	gdsDgaBufferForIEC61850[u8DGANo][gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_C3H8_PPM].m_u32Value		= (U32)(gdsDgaMonCalRes[u8DGANo].m_fC3H8ppm*100.);
	#else
	gdsDgaBufferForIEC61850[u8DGANo][gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_TEMPERATURE].m_u32Value		= (U32)((100*u8DGANo)+gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex);
	gdsDgaBufferForIEC61850[u8DGANo][gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_LEVEL].m_u32Value			= (U32)((100*u8DGANo)+gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex);
	gdsDgaBufferForIEC61850[u8DGANo][gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_PRESSURE].m_u32Value		= (U32)((100*u8DGANo)+gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex);
	gdsDgaBufferForIEC61850[u8DGANo][gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_H2O_PPM].m_u32Value			= (U32)((100*u8DGANo)+gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex);
	gdsDgaBufferForIEC61850[u8DGANo][gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_H2_PPM].m_u32Value			= (U32)((100*u8DGANo)+gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex);
	gdsDgaBufferForIEC61850[u8DGANo][gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_N2_PPM].m_u32Value			= (U32)((100*u8DGANo)+gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex);
	gdsDgaBufferForIEC61850[u8DGANo][gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_CO_PPM].m_u32Value			= (U32)((100*u8DGANo)+gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex);
	gdsDgaBufferForIEC61850[u8DGANo][gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_CO2_PPM].m_u32Value			= (U32)((100*u8DGANo)+gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex);
	gdsDgaBufferForIEC61850[u8DGANo][gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_CH4_PPM].m_u32Value			= (U32)((100*u8DGANo)+gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex);
	gdsDgaBufferForIEC61850[u8DGANo][gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_C2H2_PPM].m_u32Value		= (U32)((100*u8DGANo)+gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex);
	gdsDgaBufferForIEC61850[u8DGANo][gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_C2H4_PPM].m_u32Value		= (U32)((100*u8DGANo)+gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex);
	gdsDgaBufferForIEC61850[u8DGANo][gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_C2H6_PPM].m_u32Value		= (U32)((100*u8DGANo)+gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex);
	gdsDgaBufferForIEC61850[u8DGANo][gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_O2_PPM].m_u32Value			= (U32)((100*u8DGANo)+gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex);
	gdsDgaBufferForIEC61850[u8DGANo][gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_C3H8_PPM].m_u32Value		= (U32)((100*u8DGANo)+gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex);
	#endif

	gdsDgaBufferForIEC61850[u8DGANo][gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex].m_u32CRC	= (U32)CalculateCRC((U8*)(&(gdsDgaBufferForIEC61850[u8DGANo][gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex])), (SIZE_OF_DGA_DATA_UNIT_IN_BYTES-4));

	memcpy(gdsDgaTotalDataToCU[u8DGANo].m_dsDataList[gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex].m_pData, &(gdsDgaBufferForIEC61850[u8DGANo][gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex]), SIZE_OF_DGA_DATA_UNIT_IN_BYTES);

	if (++gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex == SIZE_OF_DGA_DATA_LIST_TO_CU) {
		gdsDgaTotalDataToCU[u8DGANo].m_u32WriteIndex = 0;
	};
}
#else
void WriteEachDGAMeasurementToIEC61850(dsTotalDataToCU* datalist, U8 u8DGANo)
{
	int i;

	// �ӽ÷� ���� ����
	#if (1)
	gdsDgaMonCalRes[u8DGANo].m_fTemperature	= (100.*u8DGANo)+(float)datalist->m_u32WriteIndex+0.12;
	gdsDgaMonCalRes[u8DGANo].m_fLevel		= (100.*u8DGANo)+(float)datalist->m_u32WriteIndex+0.34;
	gdsDgaMonCalRes[u8DGANo].m_fPressure	= (100.*u8DGANo)+(float)datalist->m_u32WriteIndex+0.56;
	gdsDgaMonCalRes[u8DGANo].m_fH2Oppm		= (100.*u8DGANo)+(float)datalist->m_u32WriteIndex+0.78;
	gdsDgaMonCalRes[u8DGANo].m_fH2ppm		= (100.*u8DGANo)+(float)datalist->m_u32WriteIndex+0.90;
	gdsDgaMonCalRes[u8DGANo].m_fN2ppm		= (100.*u8DGANo)+(float)datalist->m_u32WriteIndex+0.13;
	gdsDgaMonCalRes[u8DGANo].m_fCOppm		= (100.*u8DGANo)+(float)datalist->m_u32WriteIndex+0.24;
	gdsDgaMonCalRes[u8DGANo].m_fCO2ppm		= (100.*u8DGANo)+(float)datalist->m_u32WriteIndex+0.35;
	gdsDgaMonCalRes[u8DGANo].m_fCH4ppm		= (100.*u8DGANo)+(float)datalist->m_u32WriteIndex+0.46;
	gdsDgaMonCalRes[u8DGANo].m_fC2H2ppm		= (100.*u8DGANo)+(float)datalist->m_u32WriteIndex+0.57;
	gdsDgaMonCalRes[u8DGANo].m_fC2H4ppm		= (100.*u8DGANo)+(float)datalist->m_u32WriteIndex+0.68;
	gdsDgaMonCalRes[u8DGANo].m_fC2H6ppm		= (100.*u8DGANo)+(float)datalist->m_u32WriteIndex+0.79;
	gdsDgaMonCalRes[u8DGANo].m_fO2ppm		= (100.*u8DGANo)+(float)datalist->m_u32WriteIndex+0.80;
	gdsDgaMonCalRes[u8DGANo].m_fC3H8ppm		= (100.*u8DGANo)+(float)datalist->m_u32WriteIndex+0.91;

	gdsDgaBufferForIEC61850[u8DGANo][datalist->m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_TEMPERATURE].m_u32Value	= (U32)(gdsDgaMonCalRes[u8DGANo].m_fTemperature*100.);
	gdsDgaBufferForIEC61850[u8DGANo][datalist->m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_LEVEL].m_u32Value		= (U32)(gdsDgaMonCalRes[u8DGANo].m_fLevel*100.);
	gdsDgaBufferForIEC61850[u8DGANo][datalist->m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_PRESSURE].m_u32Value		= (U32)(gdsDgaMonCalRes[u8DGANo].m_fPressure*100.);
	gdsDgaBufferForIEC61850[u8DGANo][datalist->m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_H2O_PPM].m_u32Value		= (U32)(gdsDgaMonCalRes[u8DGANo].m_fH2Oppm*100.);
	gdsDgaBufferForIEC61850[u8DGANo][datalist->m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_H2_PPM].m_u32Value		= (U32)(gdsDgaMonCalRes[u8DGANo].m_fH2ppm*100.);
	gdsDgaBufferForIEC61850[u8DGANo][datalist->m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_N2_PPM].m_u32Value		= (U32)(gdsDgaMonCalRes[u8DGANo].m_fN2ppm*100.);
	gdsDgaBufferForIEC61850[u8DGANo][datalist->m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_CO_PPM].m_u32Value		= (U32)(gdsDgaMonCalRes[u8DGANo].m_fCOppm*100.);
	gdsDgaBufferForIEC61850[u8DGANo][datalist->m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_CO2_PPM].m_u32Value		= (U32)(gdsDgaMonCalRes[u8DGANo].m_fCO2ppm*100.);
	gdsDgaBufferForIEC61850[u8DGANo][datalist->m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_CH4_PPM].m_u32Value		= (U32)(gdsDgaMonCalRes[u8DGANo].m_fCH4ppm*100.);
	gdsDgaBufferForIEC61850[u8DGANo][datalist->m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_C2H2_PPM].m_u32Value		= (U32)(gdsDgaMonCalRes[u8DGANo].m_fC2H2ppm*100.);
	gdsDgaBufferForIEC61850[u8DGANo][datalist->m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_C2H4_PPM].m_u32Value		= (U32)(gdsDgaMonCalRes[u8DGANo].m_fC2H4ppm*100.);
	gdsDgaBufferForIEC61850[u8DGANo][datalist->m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_C2H6_PPM].m_u32Value		= (U32)(gdsDgaMonCalRes[u8DGANo].m_fC2H6ppm*100.);
	gdsDgaBufferForIEC61850[u8DGANo][datalist->m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_O2_PPM].m_u32Value		= (U32)(gdsDgaMonCalRes[u8DGANo].m_fO2ppm*100.);
	gdsDgaBufferForIEC61850[u8DGANo][datalist->m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_C3H8_PPM].m_u32Value		= (U32)(gdsDgaMonCalRes[u8DGANo].m_fC3H8ppm*100.);

	for (i=0; i<NUM_KIND_DGA_SETTING; i++) {
		gdsDgaBufferForIEC61850[u8DGANo][datalist->m_u32WriteIndex].m_u32DgaSetting[ENUM_DGA_H2_SETTING+i]		= 100+i;
	}
	#else
	gdsDgaBufferForIEC61850[u8DGANo][datalist->m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_TEMPERATURE].m_u32Value	= (U32)((100*u8DGANo)+datalist->m_u32WriteIndex);
	gdsDgaBufferForIEC61850[u8DGANo][datalist->m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_LEVEL].m_u32Value		= (U32)((100*u8DGANo)+datalist->m_u32WriteIndex);
	gdsDgaBufferForIEC61850[u8DGANo][datalist->m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_PRESSURE].m_u32Value		= (U32)((100*u8DGANo)+datalist->m_u32WriteIndex);
	gdsDgaBufferForIEC61850[u8DGANo][datalist->m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_H2O_PPM].m_u32Value		= (U32)((100*u8DGANo)+datalist->m_u32WriteIndex);
	gdsDgaBufferForIEC61850[u8DGANo][datalist->m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_H2_PPM].m_u32Value		= (U32)((100*u8DGANo)+datalist->m_u32WriteIndex);
	gdsDgaBufferForIEC61850[u8DGANo][datalist->m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_N2_PPM].m_u32Value		= (U32)((100*u8DGANo)+datalist->m_u32WriteIndex);
	gdsDgaBufferForIEC61850[u8DGANo][datalist->m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_CO_PPM].m_u32Value		= (U32)((100*u8DGANo)+datalist->m_u32WriteIndex);
	gdsDgaBufferForIEC61850[u8DGANo][datalist->m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_CO2_PPM].m_u32Value		= (U32)((100*u8DGANo)+datalist->m_u32WriteIndex);
	gdsDgaBufferForIEC61850[u8DGANo][datalist->m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_CH4_PPM].m_u32Value		= (U32)((100*u8DGANo)+datalist->m_u32WriteIndex);
	gdsDgaBufferForIEC61850[u8DGANo][datalist->m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_C2H2_PPM].m_u32Value		= (U32)((100*u8DGANo)+datalist->m_u32WriteIndex);
	gdsDgaBufferForIEC61850[u8DGANo][datalist->m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_C2H4_PPM].m_u32Value		= (U32)((100*u8DGANo)+datalist->m_u32WriteIndex);
	gdsDgaBufferForIEC61850[u8DGANo][datalist->m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_C2H6_PPM].m_u32Value		= (U32)((100*u8DGANo)+datalist->m_u32WriteIndex);
	gdsDgaBufferForIEC61850[u8DGANo][datalist->m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_O2_PPM].m_u32Value		= (U32)((100*u8DGANo)+datalist->m_u32WriteIndex);
	gdsDgaBufferForIEC61850[u8DGANo][datalist->m_u32WriteIndex].m_dsDgaStatus[ENUM_DGA_C3H8_PPM].m_u32Value		= (U32)((100*u8DGANo)+datalist->m_u32WriteIndex);
	#endif

	gdsDgaBufferForIEC61850[u8DGANo][datalist->m_u32WriteIndex].m_u32CRC	= (U32)CalculateCRC((U8*)(&(gdsDgaBufferForIEC61850[u8DGANo][datalist->m_u32WriteIndex])), (SIZE_OF_DGA_DATA_UNIT_IN_BYTES-4));

	memcpy(datalist->m_dsDataList[datalist->m_u32WriteIndex].m_pData, &(gdsDgaBufferForIEC61850[u8DGANo][datalist->m_u32WriteIndex]), SIZE_OF_DGA_DATA_UNIT_IN_BYTES);

	struct timeval dsSysTime;
	gettimeofday(&dsSysTime, NULL);
		
	//memcpy(&datalist->m_timelist[datalist->m_u32WriteIndex],&dsSysTime, sizeof(MMS_UTC_TIME2));
	datalist->m_timelist[datalist->m_u32WriteIndex].secs = dsSysTime.tv_sec;
	datalist->m_timelist[datalist->m_u32WriteIndex].fraction = dsSysTime.tv_usec;

	if (++datalist->m_u32WriteIndex == SIZE_OF_DGA_DATA_LIST_TO_CU) {
		datalist->m_u32WriteIndex = 0;
	}
}
#endif


#if (0)
void ReadOLTCOpWaveCurrent(U8 u8OLTCNo)
{
	int i;

	if (u8OLTCNo == 0) {
		for (i=0; i<gdsOLTCWave[0].m_u32NumberOfTotalSamples; i++) {
			gdsOLTCWave[0].m_dsData.m_u16PhaseACurrent[i]	= g_pAISharedMem->m_dsMeasQueueData.f32OCT1mag[(gdsOLTCWave[0].m_u32StartIndex+i)%DATA_QUEUE_SIZE];
			gdsOLTCWave[0].m_dsData.m_u16PhaseBCurrent[i]	= g_pAISharedMem->m_dsMeasQueueData.f32OCT2mag[(gdsOLTCWave[0].m_u32StartIndex+i)%DATA_QUEUE_SIZE];
			gdsOLTCWave[0].m_dsData.m_u16PhaseCCurrent[i]	= g_pAISharedMem->m_dsMeasQueueData.f32OCT3mag[(gdsOLTCWave[0].m_u32StartIndex+i)%DATA_QUEUE_SIZE];
		}
	}
	else if (u8OLTCNo == 1) {
		for (i=0; i<gdsOLTCWave[1].m_u32NumberOfTotalSamples; i++) {
			gdsOLTCWave[1].m_dsData.m_u16PhaseACurrent[i]	= g_pAISharedMem->m_dsMeasQueueData.f32OCT4mag[(gdsOLTCWave[1].m_u32StartIndex+i)%DATA_QUEUE_SIZE];
			gdsOLTCWave[1].m_dsData.m_u16PhaseBCurrent[i]	= g_pAISharedMem->m_dsMeasQueueData.f32OCT5mag[(gdsOLTCWave[1].m_u32StartIndex+i)%DATA_QUEUE_SIZE];
			gdsOLTCWave[1].m_dsData.m_u16PhaseCCurrent[i]	= g_pAISharedMem->m_dsMeasQueueData.f32OCT6mag[(gdsOLTCWave[1].m_u32StartIndex+i)%DATA_QUEUE_SIZE];
		}
	}
	else {
		for (i=0; i<gdsOLTCWave[2].m_u32NumberOfTotalSamples; i++) {
			gdsOLTCWave[2].m_dsData.m_u16PhaseACurrent[i]	= g_pAISharedMem->m_dsMeasQueueData.f32OCT7mag[(gdsOLTCWave[2].m_u32StartIndex+i)%DATA_QUEUE_SIZE];
			gdsOLTCWave[2].m_dsData.m_u16PhaseBCurrent[i]	= g_pAISharedMem->m_dsMeasQueueData.f32OCT8mag[(gdsOLTCWave[2].m_u32StartIndex+i)%DATA_QUEUE_SIZE];
			gdsOLTCWave[2].m_dsData.m_u16PhaseCCurrent[i]	= g_pAISharedMem->m_dsMeasQueueData.f32OCT9mag[(gdsOLTCWave[2].m_u32StartIndex+i)%DATA_QUEUE_SIZE];
		}
	}
}


// OLTC ���� ���� ���� ���ø� ���� ������ �б�
void ReadOLTCOpWaveData()
{
	int i, j;


	/* ȣ��Ǿ��� ��
	   1. ���� ���� ���¿���
	      ��. ���� ������ 0�̸� (�ƹ��� �ϵ� �� �Ͼ ���̹Ƿ�) ���� ����
	      ��. ���� ������ 1�̸� OLTC ���� ������ ������ ���۵� ���̹Ƿ� �Ʒ��� �ڵ带 ����
	         - Start index�� �о ����
	         - Start �ð� ���
             - ���� ���� ���·� ����

       2. ���� ���� ���� ���¿���
          ��. ���� ������ 0�̸� ���� ������ ����� ���̹Ƿ�
             - Stop index�� �о ����
             - Stop �ð� ���
             - ���� �ð� ��� (ms ����)
             - Start index���� Stop index������ ���� ������ �о ����
             - ���� ���� �Ϸ� ��� �� CU�� ������ �غ� (�÷��� ����?)
             - ���� ���� ���·� ����
          ��. ���� ������ 1�̸� ���Ͱ� ��� ���� ���� ���̹Ƿ�
             - ������ �� ���� ����
	*/

	for (i=0; i<MAX_MTR_OLTCS; i++) {
		if (!gdsOLTCMonInfo[i].m_bMotorInOperation) {	// ���� ���� ���¿���
			if (!g_pIOSharedMem->m_dsIOInfo.u8DIStsVal[DI_OLTC_MOTOR_OP+i]) {		// ��� ���Ͱ� ���� ������ ���̹Ƿ�
				// ������ ������ ����.
			}
			else {		// ���Ͱ� �����ϱ� �����Ͽ����Ƿ�
				// Start index�� �о ����
				gdsOLTCWave[i].m_u32StartIndex	= g_pAISharedMem->m_dsMeasQueueData.u32Index;
				// Start �ð� ���
				gettimeofday(&(gdsOLTCMonInfo[i].m_dsMotorStartTime), NULL);

				// ���� ���� ���·� ����
				gdsOLTCMonInfo[i].m_bMotorInOperation = TRUE;
			}
		}
		else {		// ���Ͱ� ���� ���� ���¿���
			if (!g_pIOSharedMem->m_dsIOInfo.u8DIStsVal[DI_OLTC_MOTOR_OP+i]) { 	// ���� ������ ������ ���̹Ƿ�
				// Stop index�� �о ����
				gdsOLTCWave[i].m_u32StopIndex	= g_pAISharedMem->m_dsMeasQueueData.u32Index;
				// Stop �ð� ���
				gettimeofday(&(gdsOLTCMonInfo[i].m_dsMotorStopTime), NULL);

				// ���� �ð� ��� (index ����)
				gdsOLTCWave[i].m_u32NumberOfTotalSamples = (gdsOLTCWave[i].m_u32StopIndex+DATA_QUEUE_SIZE-gdsOLTCWave[i].m_u32StartIndex) % DATA_QUEUE_SIZE;
				gdsOLTCWave[i].m_u32OLTCOpDurationCalculatedByIndex = (U32)((gdsOLTCWave[i].m_u32NumberOfTotalSamples)*SAMPLING_INTERVAL_IN_SEC*1000.);
				// ���� �ð� ��� (ms ����)
				gdsOLTCMonInfo[i].m_u32MotorOperationTime = CalculateTimeDiffInMsec(&(gdsOLTCMonInfo[i].m_dsMotorStartTime), &(gdsOLTCMonInfo[i].m_dsMotorStopTime));

				// Start index���� Stop index������ ���� ������ �о ����				
				ReadOLTCOpWaveCurrent(i);

				// ���� ���� �Ϸ� ��� �� CU�� ������ �غ� (�÷��� ����?)
				gdsOLTCMonInfo[i].m_bMotorOpEventExists = TRUE;

				// ���� ���� ���·� ����
				gdsOLTCMonInfo[i].m_bMotorInOperation = FALSE;
			}
			else {
				// ������ ������ ����.
			}
		}
	}
}
#endif



void CreateOLTCdat(U8 u8OLTCNo, dsOLTCWaveData a_dsOLTCWaveData)
{
	FILE *fileToWrite;
    time_t now;
    struct tm *tm;
    now = time(0);
    if ((tm = localtime (&now)) == NULL) {
        printf ("Error extracting time stuff\n");
        return 1;
    }

	U8 *buffer=(char*)malloc(sizeof(a_dsOLTCWaveData));
	char filename[50];
	snprintf(filename,sizeof(filename),"/data/COMTRADE/%d_42_%04d%02d%02d%02d%02d%02d.dat",u8OLTCNo,tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,tm->tm_hour, tm->tm_min, tm->tm_sec);
	//printf("filename: %s\n",filename);
	
	if (OLTCcounting[u8OLTCNo] == filebuffermax){
		char temp[50];
		strcpy(temp, MTRfilenamebuffs[u8OLTCNo].namebuffer[0]);
		//printf("filename: %s\n",temp);

		for(int i=1;i<filebuffermax;i++){
			strcpy(MTRfilenamebuffs[u8OLTCNo].namebuffer[i-1],MTRfilenamebuffs[u8OLTCNo].namebuffer[i]);
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

		strcpy(MTRfilenamebuffs[u8OLTCNo].namebuffer[filebuffermax-1],filename);
		}
	else if(OLTCcounting[u8OLTCNo]<filebuffermax){
		strcpy(MTRfilenamebuffs[u8OLTCNo].namebuffer[OLTCcounting[u8OLTCNo]],filename);
		OLTCcounting[u8OLTCNo]++;
		}	
	
	memcpy(buffer, (const unsigned char*)&a_dsOLTCWaveData, sizeof(a_dsOLTCWaveData));

	if ((fileToWrite = fopen(filename, "wb+")) != NULL) {
	    fwrite(buffer, 1, sizeof(a_dsOLTCWaveData), fileToWrite);

	    fclose(fileToWrite);
	}

	free(buffer);
}

void WriteEachOLTCWaveToIEC61850(U8 u8OLTCNo)
{
	int i;

	// for test
	#if (1)
	gdsOLTCWave[u8OLTCNo].m_dsData.m_u32EventTime	= (u8OLTCNo+1)*0x11110000;
	gdsOLTCWave[u8OLTCNo].m_dsData.m_u8AlarmLevel	= u8OLTCNo;

	gdsOLTCWave[u8OLTCNo].m_dsData.m_u32OpDuration	= (u8OLTCNo+1)*0x00000100;
	gdsOLTCWave[u8OLTCNo].m_dsData.m_u32OpCount		= (u8OLTCNo+1)*0x00000200;

	gdsOLTCWave[u8OLTCNo].m_dsData.m_u32MaxCurrent	= (u8OLTCNo+1)*0x00000010;
	gdsOLTCWave[u8OLTCNo].m_dsData.m_u32AvgCurrent	= (u8OLTCNo+1)*0x00000008;

	gdsOLTCWave[u8OLTCNo].m_dsData.m_u32SamplesPerCycle		= 128;
	gdsOLTCWave[u8OLTCNo].m_dsData.m_u8SampleDataUnitLength	= 2;
	gdsOLTCWave[u8OLTCNo].m_dsData.m_u32NumOfSampleData		= (u8OLTCNo+1)*23040;

	for (i=0; i<(OLTC_WAVE_LENGTH_IN_SEC*POWER_FREQ*SAMPLES_PER_CYCLE); i++) {
		gdsOLTCWave[u8OLTCNo].m_dsData.m_u16PhaseACurrent[i]		= (u8OLTCNo+1)*cosf(DEGREE_TO_RADIAN*(i*(360./SAMPLES_PER_CYCLE)));
		gdsOLTCWave[u8OLTCNo].m_dsData.m_u16PhaseBCurrent[i]		= (u8OLTCNo+1)*cosf(DEGREE_TO_RADIAN*((i*(360./SAMPLES_PER_CYCLE))-120.));
		gdsOLTCWave[u8OLTCNo].m_dsData.m_u16PhaseCCurrent[i]		= (u8OLTCNo+1)*cosf(DEGREE_TO_RADIAN*((i*(360./SAMPLES_PER_CYCLE))-240.));
	}

	//CreateOLTCdat(u8OLTCNo, gdsOLTCWave[u8OLTCNo].m_dsData);
	#endif
}


#if (0)
void WriteOLTCOpWaveData()
{
	int i;


	for (i=0; i<MAX_MTR_OLTCS; i++) {
		if (gdsOLTCMonInfo[i].m_bMotorOpEventExists) {		// OLTC ���� ���� �̺�Ʈ�� ������
			// CU�� ����

			// OLTC ���� ���� �̺�Ʈ �÷��� clear
			gdsOLTCMonInfo[i].m_bMotorOpEventExists = FALSE;
		}
	}
}
#endif


// OLTC ���� ���� ���� ���ø� ���� ������ �б�
void ReadMeasurementForOLTCMon()
{
	int i, j;
	

	// �׻� ȣ��Ǵ� ����̾�� ��

	// �ܱ� �µ�
	gdsOLTCMonInfo[0].m_fOutsideTemp	= g_pAISharedMem->m_dsMeasValueData.m_dsTDDataInfo.m_f32Mag[0];

	for (i=0; i<NUM_BASIC_PHASE; i++) {
		// OLTC #1 �������� ����
		gdsOLTCMonInfo[0].m_dsMotorI[i].m_fReal	= g_pAISharedMem->m_dsMeasValueData.m_dsOCTDataInfo.m_f32Real[i];
		gdsOLTCMonInfo[0].m_dsMotorI[i].m_fImag	= g_pAISharedMem->m_dsMeasValueData.m_dsOCTDataInfo.m_f32Imag[i];
		gdsOLTCMonInfo[0].m_dsMotorI[i].m_fMag	= g_pAISharedMem->m_dsMeasValueData.m_dsOCTDataInfo.m_f32Mag[i];
		gdsOLTCMonInfo[0].m_dsMotorI[i].m_fAng	= g_pAISharedMem->m_dsMeasValueData.m_dsOCTDataInfo.m_f32Ang[i];
	}

	if (gdsPowerSystem.m_u8GridVoltage == GRID_V_345kV) {
		for (i=0; i<NUM_BASIC_PHASE; i++) {
			// OLTC #2 �������� ����
			gdsOLTCMonInfo[1].m_dsMotorI[i].m_fReal	= g_pAISharedMem->m_dsMeasValueData.m_dsOCTDataInfo.m_f32Real[NUM_BASIC_PHASE+i];
			gdsOLTCMonInfo[1].m_dsMotorI[i].m_fImag	= g_pAISharedMem->m_dsMeasValueData.m_dsOCTDataInfo.m_f32Imag[NUM_BASIC_PHASE+i];
			gdsOLTCMonInfo[1].m_dsMotorI[i].m_fMag	= g_pAISharedMem->m_dsMeasValueData.m_dsOCTDataInfo.m_f32Mag[NUM_BASIC_PHASE+i];
			gdsOLTCMonInfo[1].m_dsMotorI[i].m_fAng	= g_pAISharedMem->m_dsMeasValueData.m_dsOCTDataInfo.m_f32Ang[NUM_BASIC_PHASE+i];

			// OLTC #3 �������� ����
			gdsOLTCMonInfo[2].m_dsMotorI[i].m_fReal	= g_pAISharedMem->m_dsMeasValueData.m_dsOCTDataInfo.m_f32Real[(2*NUM_BASIC_PHASE)+i];
			gdsOLTCMonInfo[2].m_dsMotorI[i].m_fImag	= g_pAISharedMem->m_dsMeasValueData.m_dsOCTDataInfo.m_f32Imag[(2*NUM_BASIC_PHASE)+i];
			gdsOLTCMonInfo[2].m_dsMotorI[i].m_fMag	= g_pAISharedMem->m_dsMeasValueData.m_dsOCTDataInfo.m_f32Mag[(2*NUM_BASIC_PHASE)+i];
			gdsOLTCMonInfo[2].m_dsMotorI[i].m_fAng	= g_pAISharedMem->m_dsMeasValueData.m_dsOCTDataInfo.m_f32Ang[(2*NUM_BASIC_PHASE)+i];
		}
	}
}

// Bushing �������� ���ø� ���� ������ �б� - �� ���� �� ���� ����
void ReadMeasurementForBshMon()
{
	int i;


	for (i=0; i<NUM_BASIC_PHASE; i++) {
		// ����
		gdsBshMonCalRes[0].m_dsBusV[i].m_fReal	= g_pAISharedMem->m_dsMeasValueData.m_dsPTDataInfo.m_f32Real[i];
		gdsBshMonCalRes[0].m_dsBusV[i].m_fImag	= g_pAISharedMem->m_dsMeasValueData.m_dsPTDataInfo.m_f32Imag[i];
		gdsBshMonCalRes[0].m_dsBusV[i].m_fMag	= g_pAISharedMem->m_dsMeasValueData.m_dsPTDataInfo.m_f32Mag[i];
		gdsBshMonCalRes[0].m_dsBusV[i].m_fAng	= g_pAISharedMem->m_dsMeasValueData.m_dsPTDataInfo.m_f32Ang[i];
		// ��������
		gdsBshMonCalRes[0].m_dsBshLeakI[i].m_fReal	= g_pAISharedMem->m_dsMeasValueData.m_dsBCTDataInfo.m_f32Real[i];
		gdsBshMonCalRes[0].m_dsBshLeakI[i].m_fImag	= g_pAISharedMem->m_dsMeasValueData.m_dsBCTDataInfo.m_f32Imag[i];
		gdsBshMonCalRes[0].m_dsBshLeakI[i].m_fMag	= g_pAISharedMem->m_dsMeasValueData.m_dsBCTDataInfo.m_f32Mag[i];
		gdsBshMonCalRes[0].m_dsBshLeakI[i].m_fAng	= g_pAISharedMem->m_dsMeasValueData.m_dsBCTDataInfo.m_f32Ang[i];
	}

	if (gdsPowerSystem.m_u8GridVoltage == GRID_V_345kV) {
		for (i=0; i<NUM_BASIC_PHASE; i++) {
			// ����
			gdsBshMonCalRes[1].m_dsBusV[i].m_fReal	= g_pAISharedMem->m_dsMeasValueData.m_dsPTDataInfo.m_f32Real[NUM_BASIC_PHASE+i];
			gdsBshMonCalRes[1].m_dsBusV[i].m_fImag	= g_pAISharedMem->m_dsMeasValueData.m_dsPTDataInfo.m_f32Imag[NUM_BASIC_PHASE+i];
			gdsBshMonCalRes[1].m_dsBusV[i].m_fMag	= g_pAISharedMem->m_dsMeasValueData.m_dsPTDataInfo.m_f32Mag[NUM_BASIC_PHASE+i];
			gdsBshMonCalRes[1].m_dsBusV[i].m_fAng	= g_pAISharedMem->m_dsMeasValueData.m_dsPTDataInfo.m_f32Ang[NUM_BASIC_PHASE+i];
			// ��������
			gdsBshMonCalRes[1].m_dsBshLeakI[i].m_fReal	= g_pAISharedMem->m_dsMeasValueData.m_dsBCTDataInfo.m_f32Real[NUM_BASIC_PHASE+i];
			gdsBshMonCalRes[1].m_dsBshLeakI[i].m_fImag	= g_pAISharedMem->m_dsMeasValueData.m_dsBCTDataInfo.m_f32Imag[NUM_BASIC_PHASE+i];
			gdsBshMonCalRes[1].m_dsBshLeakI[i].m_fMag	= g_pAISharedMem->m_dsMeasValueData.m_dsBCTDataInfo.m_f32Mag[NUM_BASIC_PHASE+i];
			gdsBshMonCalRes[1].m_dsBshLeakI[i].m_fAng	= g_pAISharedMem->m_dsMeasValueData.m_dsBCTDataInfo.m_f32Ang[NUM_BASIC_PHASE+i];
		}
	}
}


// ���� �������� ���� �ʿ��� ���� ����
void ExecuteCalculationForBshMon()
{
	int i;


	for (i=0; i<NUM_BASIC_PHASE; i++) {
		// Capacitance ��
		gdsBshMonCalRes[0].m_fCapVal[i]		= gdsBshMonCalRes[0].m_dsBshLeakI[i].m_fMag / (2.*PI*((float)POWER_FREQ)*gdsBshMonCalRes[0].m_dsBusV[i].m_fMag);
		// ����
		gdsBshMonCalRes[0].m_fPwrFactor[i]	= cosf(DEGREE_TO_RADIAN*(gdsBshMonCalRes[0].m_dsBusV[i].m_fAng - gdsBshMonCalRes[0].m_dsBshLeakI[i].m_fAng));
		// �սǰ��
		gdsBshMonCalRes[0].m_fDisFactor[i]	= tanf(DEGREE_TO_RADIAN*(90.-(gdsBshMonCalRes[0].m_dsBusV[i].m_fAng - gdsBshMonCalRes[0].m_dsBshLeakI[i].m_fAng)));
		// ��������
		gdsBshMonCalRes[0].m_fDisplI[i]		= 2.*PI*((float)POWER_FREQ)*gdsBshMonCalRes[0].m_fCapVal[i]*gdsBshMonCalRes[0].m_dsBusV[i].m_fMag;
	}

	// ����������
	// �Ǽ���
	gdsBshMonCalRes[0].m_dsBshLeak3I0.m_fReal	= gdsBshMonCalRes[0].m_dsBshLeakI[PHASE_A].m_fReal + gdsBshMonCalRes[0].m_dsBshLeakI[PHASE_B].m_fReal + gdsBshMonCalRes[0].m_dsBshLeakI[PHASE_C].m_fReal;
	// �����
	gdsBshMonCalRes[0].m_dsBshLeak3I0.m_fImag	= gdsBshMonCalRes[0].m_dsBshLeakI[PHASE_A].m_fImag + gdsBshMonCalRes[0].m_dsBshLeakI[PHASE_B].m_fImag + gdsBshMonCalRes[0].m_dsBshLeakI[PHASE_C].m_fImag;
	// ũ��
	gdsBshMonCalRes[0].m_dsBshLeak3I0.m_fMag	= sqrtf(gdsBshMonCalRes[0].m_dsBshLeak3I0.m_fReal*gdsBshMonCalRes[0].m_dsBshLeak3I0.m_fReal + gdsBshMonCalRes[0].m_dsBshLeak3I0.m_fImag*gdsBshMonCalRes[0].m_dsBshLeak3I0.m_fImag);
	// ����
	gdsBshMonCalRes[0].m_dsBshLeak3I0.m_fAng	= RADIAN_TO_DEGREE*atan2f(gdsBshMonCalRes[0].m_dsBshLeak3I0.m_fImag, gdsBshMonCalRes[0].m_dsBshLeak3I0.m_fReal);

	
	if (gdsPowerSystem.m_u8GridVoltage == GRID_V_345kV) {
		for (i=0; i<NUM_BASIC_PHASE; i++) {
			// Capacitance ��
			gdsBshMonCalRes[1].m_fCapVal[i]		= gdsBshMonCalRes[1].m_dsBshLeakI[i].m_fMag / (2.*PI*((float)POWER_FREQ)*gdsBshMonCalRes[1].m_dsBusV[i].m_fMag);
			// ����
			gdsBshMonCalRes[1].m_fPwrFactor[i]	= cosf(DEGREE_TO_RADIAN*(gdsBshMonCalRes[1].m_dsBusV[i].m_fAng - gdsBshMonCalRes[1].m_dsBshLeakI[i].m_fAng));
			// �սǰ��
			gdsBshMonCalRes[1].m_fDisFactor[i]	= tanf(DEGREE_TO_RADIAN*(90.-(gdsBshMonCalRes[1].m_dsBusV[i].m_fAng - gdsBshMonCalRes[1].m_dsBshLeakI[i].m_fAng)));
			// ��������
			gdsBshMonCalRes[1].m_fDisplI[i]		= 2.*PI*((float)POWER_FREQ)*gdsBshMonCalRes[1].m_fCapVal[i]*gdsBshMonCalRes[1].m_dsBusV[i].m_fMag;
		}

		// ����������
		// �Ǽ���
		gdsBshMonCalRes[1].m_dsBshLeak3I0.m_fReal	= gdsBshMonCalRes[1].m_dsBshLeakI[PHASE_A].m_fReal + gdsBshMonCalRes[1].m_dsBshLeakI[PHASE_B].m_fReal + gdsBshMonCalRes[1].m_dsBshLeakI[PHASE_C].m_fReal;
		// �����
		gdsBshMonCalRes[1].m_dsBshLeak3I0.m_fImag	= gdsBshMonCalRes[1].m_dsBshLeakI[PHASE_A].m_fImag + gdsBshMonCalRes[1].m_dsBshLeakI[PHASE_B].m_fImag + gdsBshMonCalRes[1].m_dsBshLeakI[PHASE_C].m_fImag;
		// ũ��
		gdsBshMonCalRes[1].m_dsBshLeak3I0.m_fMag	= sqrtf(gdsBshMonCalRes[1].m_dsBshLeak3I0.m_fReal*gdsBshMonCalRes[1].m_dsBshLeak3I0.m_fReal + gdsBshMonCalRes[1].m_dsBshLeak3I0.m_fImag*gdsBshMonCalRes[1].m_dsBshLeak3I0.m_fImag);
		// ����
		gdsBshMonCalRes[1].m_dsBshLeak3I0.m_fAng	= RADIAN_TO_DEGREE*atan2f(gdsBshMonCalRes[1].m_dsBshLeak3I0.m_fImag, gdsBshMonCalRes[1].m_dsBshLeak3I0.m_fReal);
	}
}


void WriteEachOLTCMeasurementToIEC61850(dsTotalDataToCU* datalist, U8 u8OLTCNo)
{
	int i;


	// 2021. 12. 24. ���� �ڵ� ����
	#if (0)		// ����� �ڵ�
	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_dsOLTCStatus[ENUM_OLTC_OIL_FILTER].m_u32Value				= (1000*u8OLTCNo)+(10*datalist->m_u32WriteIndex)+1;
	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_dsOLTCStatus[ENUM_OLTC_MOTOR_DRIVE_BLOCK].m_u32Value		= (1000*u8OLTCNo)+(10*datalist->m_u32WriteIndex)+2;
	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_dsOLTCStatus[ENUM_OLTC_VAC_CEL_ALARM].m_u32Value			= (1000*u8OLTCNo)+(10*datalist->m_u32WriteIndex)+3;
	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_dsOLTCStatus[ENUM_OLTC_OIL_FILTER_TRIP].m_u32Value			= (1000*u8OLTCNo)+(10*datalist->m_u32WriteIndex)+4;

	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_dsOLTCStatus[ENUM_OLTC_OIL_FILTER_COUNT].m_u32Value			= (1000*u8OLTCNo)+(10*datalist->m_u32WriteIndex)+5;

	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_dsOLTCStatus[ENUM_OLTC_EVTTRANSF].m_u32Value				= (1000*u8OLTCNo)+(10*datalist->m_u32WriteIndex)+6;
	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_dsOLTCStatus[ENUM_OLTC_RSOPCNT].m_u32Value					= (1000*u8OLTCNo)+(10*datalist->m_u32WriteIndex)+7;
	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_dsOLTCStatus[ENUM_OLTC_MODEVCOMF].m_u32Value				= (1000*u8OLTCNo)+(10*datalist->m_u32WriteIndex)+8;
	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_dsOLTCStatus[ENUM_OLTC_MODEVFLT].m_u32Value					= (1000*u8OLTCNo)+(10*datalist->m_u32WriteIndex)+9;

	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_dsOLTCStatus[ENUM_OLTC_TORQUE].m_u32Value					= (U32)(((100.*u8OLTCNo)+(float)datalist->m_u32WriteIndex+0.11)*100.);
	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_dsOLTCStatus[ENUM_OLTC_MOTOR_DRIVE_I].m_u32Value			= (U32)(((100.*u8OLTCNo)+(float)datalist->m_u32WriteIndex+0.22)*100.);
	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_dsOLTCStatus[ENUM_OLTC_ABRASION_LEVEL].m_u32Value			= (U32)(((100.*u8OLTCNo)+(float)datalist->m_u32WriteIndex+0.33)*100.);
	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_dsOLTCStatus[ENUM_OLTC_OP_DURATION].m_u32Value				= (U32)(((100.*u8OLTCNo)+(float)datalist->m_u32WriteIndex+0.44)*100.);
	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_dsOLTCStatus[ENUM_OLTC_OIL_TEMPERATURE_DIFF].m_u32Value		= (U32)(((100.*u8OLTCNo)+(float)datalist->m_u32WriteIndex+0.55)*100.);
	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_dsOLTCStatus[ENUM_OLTC_OUTSIDE_TEMPERATURE].m_u32Value		= (U32)(((100.*u8OLTCNo)+(float)datalist->m_u32WriteIndex+0.66)*100.);
	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_dsOLTCStatus[ENUM_OLTC_MOTOR_DRIVE_I_PHASE_A].m_u32Value	= (U32)(((100.*u8OLTCNo)+(float)datalist->m_u32WriteIndex+0.77)*100.);
	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_dsOLTCStatus[ENUM_OLTC_MOTOR_DRIVE_I_PHASE_B].m_u32Value	= (U32)(((100.*u8OLTCNo)+(float)datalist->m_u32WriteIndex+0.88)*100.);
	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_dsOLTCStatus[ENUM_OLTC_MOTOR_DRIVE_I_PHASE_C].m_u32Value	= (U32)(((100.*u8OLTCNo)+(float)datalist->m_u32WriteIndex+0.99)*100.);
	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_dsOLTCStatus[ENUM_OLTC_OIL_FILTER_TEMPERATURE].m_u32Value	= (U32)(((100.*u8OLTCNo)+(float)datalist->m_u32WriteIndex+0.15)*100.);
	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_dsOLTCStatus[ENUM_OLTC_OIL_FILTER_PRESSURE].m_u32Value		= (U32)(((100.*u8OLTCNo)+(float)datalist->m_u32WriteIndex+0.26)*100.);
	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_dsOLTCStatus[ENUM_OLTC_OIL_FILTER_MOISTURE].m_u32Value		= (U32)(((100.*u8OLTCNo)+(float)datalist->m_u32WriteIndex+0.37)*100.);

	for (i=0; i<NUM_KIND_OLTC_SETTING; i++) {
		gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_u32OLTCSetting[ENUM_OLTC_MOTOR_DRIVE_I_ALARM_LEVEL+i]		= 200+i;
	}
	#else		// ���� �ڵ�
	// �Ʒ����� ���� ó���ϴ� �׸�� �� �������� ó������ 0���� ä���� �ִ�.

	#if (0)
	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_dsOLTCStatus[ENUM_OLTC_EVTTRANSF].m_u32Value				= (U32)gdsOLTCMonInfo[u8OLTCNo].m_bMotorOpEventExists;
	if (gdsOLTCMonInfo[u8OLTCNo].m_bMotorOpEventExists) {
		gdsOLTCMonInfo[u8OLTCNo].m_bMotorOpEventExists = FALSE;
	}
	#endif

	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_dsOLTCStatus[ENUM_OLTC_RSOPCNT].m_u32Value					= (U32)gdsOLTCMonInfo[u8OLTCNo].m_u16AccumulatedOpCnt;
	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_dsOLTCStatus[ENUM_OLTC_MODEVCOMF].m_u32Value				= 0;	// modification needed (DAU self-monitoring result)
	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_dsOLTCStatus[ENUM_OLTC_MODEVFLT].m_u32Value					= 0;	// modification needed (DAU self-monitoring result)

	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_dsOLTCStatus[ENUM_OLTC_MOTOR_DRIVE_I].m_u32Value			= (U32)(MaxFloat3(gdsOLTCMonInfo[u8OLTCNo].m_dsMotorI[PHASE_A].m_fMag, gdsOLTCMonInfo[u8OLTCNo].m_dsMotorI[PHASE_B].m_fMag, gdsOLTCMonInfo[u8OLTCNo].m_dsMotorI[PHASE_C].m_fMag, &i)*100.);

	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_dsOLTCStatus[ENUM_OLTC_OP_DURATION].m_u32Value				= (U32)gdsOLTCMonInfo[u8OLTCNo].m_u32MotorOperationTime;

	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_dsOLTCStatus[ENUM_OLTC_OIL_TEMPERATURE_DIFF].m_u32Value		= 0;	// checking needed

	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_dsOLTCStatus[ENUM_OLTC_OUTSIDE_TEMPERATURE].m_u32Value		= (U32)(gdsOLTCMonInfo[u8OLTCNo].m_fOutsideTemp*100.);

	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_dsOLTCStatus[ENUM_OLTC_MOTOR_DRIVE_I_PHASE_A].m_u32Value	= (U32)(gdsOLTCMonInfo[u8OLTCNo].m_dsMotorI[PHASE_A].m_fMag*100.);
	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_dsOLTCStatus[ENUM_OLTC_MOTOR_DRIVE_I_PHASE_B].m_u32Value	= (U32)(gdsOLTCMonInfo[u8OLTCNo].m_dsMotorI[PHASE_B].m_fMag*100.);
	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_dsOLTCStatus[ENUM_OLTC_MOTOR_DRIVE_I_PHASE_C].m_u32Value	= (U32)(gdsOLTCMonInfo[u8OLTCNo].m_dsMotorI[PHASE_C].m_fMag*100.);
	#endif

	gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex].m_u32CRC	= (U32)CalculateCRC((U8*)(&(gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex])), (SIZE_OF_OLTC_DATA_UNIT_IN_BYTES-4));

	memcpy(datalist->m_dsDataList[datalist->m_u32WriteIndex].m_pData, &(gdsOLTCBufferForIEC61850[u8OLTCNo][datalist->m_u32WriteIndex]), SIZE_OF_OLTC_DATA_UNIT_IN_BYTES);
	struct timeval dsSysTime;
	gettimeofday(&dsSysTime, NULL);
	//memcpy(&datalist->m_timelist[datalist->m_u32WriteIndex],&dsSysTime, sizeof(MMS_UTC_TIME2));
	datalist->m_timelist[datalist->m_u32WriteIndex].secs = dsSysTime.tv_sec;
	datalist->m_timelist[datalist->m_u32WriteIndex].fraction = dsSysTime.tv_usec;

	if (++datalist->m_u32WriteIndex == SIZE_OF_OLTC_DATA_LIST_TO_CU) {
		datalist->m_u32WriteIndex = 0;
	}
}


void WriteEachBSHMeasurementToIEC61850(dsTotalDataToCU* datalist, U8 u8BSHNo)
{
	int i;


	// 2021. 12. 24. ���� �ڵ� ����
	#if (0) 	// ����� �ڵ�
	gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex].m_dsBSHStatus[ENUM_BSH_RTTRANSF].m_u32Value			= 5000+(1000*u8BSHNo)+(10*datalist->m_u32WriteIndex)+1;
	gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex].m_dsBSHStatus[ENUM_BSH_MODEVCOMF].m_u32Value		= 5000+(1000*u8BSHNo)+(10*datalist->m_u32WriteIndex)+2;
	gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex].m_dsBSHStatus[ENUM_BSH_MODEVFLT].m_u32Value			= 5000+(1000*u8BSHNo)+(10*datalist->m_u32WriteIndex)+3;

	gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex].m_dsBSHStatus[ENUM_BSH_REACTANCE].m_u32Value		= (U32)((500.+(100.*u8BSHNo)+(float)datalist->m_u32WriteIndex+0.11)*100.);
	gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex].m_dsBSHStatus[ENUM_BSH_ABSREACTANCE].m_u32Value		= (U32)((500.+(100.*u8BSHNo)+(float)datalist->m_u32WriteIndex+0.22)*100.);
	gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex].m_dsBSHStatus[ENUM_BSH_LOSSFACTOR].m_u32Value		= (U32)((500.+(100.*u8BSHNo)+(float)datalist->m_u32WriteIndex+0.33)*100.);
	gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex].m_dsBSHStatus[ENUM_BSH_VOLTAGE].m_u32Value			= (U32)((500.+(100.*u8BSHNo)+(float)datalist->m_u32WriteIndex+0.44)*100.);
	gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex].m_dsBSHStatus[ENUM_BSH_DISPLACEMENT_I].m_u32Value	= (U32)((500.+(100.*u8BSHNo)+(float)datalist->m_u32WriteIndex+0.55)*100.);
	gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex].m_dsBSHStatus[ENUM_BSH_LEAKAGE_I].m_u32Value		= (U32)((500.+(100.*u8BSHNo)+(float)datalist->m_u32WriteIndex+0.66)*100.);
	gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex].m_dsBSHStatus[ENUM_BSH_LEAKAGE_I_ANG].m_u32Value	= (U32)((500.+(100.*u8BSHNo)+(float)datalist->m_u32WriteIndex+0.77)*100.);
	gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex].m_dsBSHStatus[ENUM_BSH_VOLTAGE_ANG].m_u32Value		= (U32)((500.+(100.*u8BSHNo)+(float)datalist->m_u32WriteIndex+0.88)*100.);
	gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex].m_dsBSHStatus[ENUM_BSH_POWER_FACTOR].m_u32Value		= (U32)((500.+(100.*u8BSHNo)+(float)datalist->m_u32WriteIndex+0.99)*100.);
	gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex].m_dsBSHStatus[ENUM_BSH_UNBALANCE_I].m_u32Value		= (U32)((500.+(100.*u8BSHNo)+(float)datalist->m_u32WriteIndex+0.15)*100.);
	gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex].m_dsBSHStatus[ENUM_BSH_UNBALANCE_I_ANG].m_u32Value	= (U32)((500.+(100.*u8BSHNo)+(float)datalist->m_u32WriteIndex+0.26)*100.);

	for (i=0; i<NUM_KIND_BSH_SETTING; i++) {
		gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex].m_u32BSHSetting[ENUM_BSH_REACTANCE_SETTING+i]		= 300+i;
	}
	#else		// ���� �ڵ�
	// �Ʒ����� ���� ó���ϴ� �׸�� �� �������� ó������ 0���� ä���� �ִ�.

	gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex].m_dsBSHStatus[ENUM_BSH_RTTRANSF].m_u32Value			= (U32)gdsBshMonCalRes[u8BSHNo].m_bCalResExists;	// ��𼱰� �� �÷��׸� �����ؾ� ��
	if (gdsBshMonCalRes[u8BSHNo].m_bCalResExists) {
		gdsBshMonCalRes[u8BSHNo].m_bCalResExists = FALSE;
	}

	gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex].m_dsBSHStatus[ENUM_BSH_MODEVCOMF].m_u32Value		= 0;	// modification needed (DAU self-monitoring result)
	gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex].m_dsBSHStatus[ENUM_BSH_MODEVFLT].m_u32Value			= 0;	// modification needed (DAU self-monitoring result)

	gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex].m_dsBSHStatus[ENUM_BSH_REACTANCE].m_u32Value		= 0;	// checking needed
	gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex].m_dsBSHStatus[ENUM_BSH_ABSREACTANCE].m_u32Value		= (U32)(gdsBshMonCalRes[u8BSHNo].m_fCapVal[PHASE_A]*100.);

	gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex].m_dsBSHStatus[ENUM_BSH_LOSSFACTOR].m_u32Value		= (U32)(gdsBshMonCalRes[u8BSHNo].m_fDisFactor[PHASE_A]*100.);

	//printf("LosFact: %d\n",gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex].m_dsBSHStatus[ENUM_BSH_LOSSFACTOR].m_u32Value);
	gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex].m_dsBSHStatus[ENUM_BSH_VOLTAGE].m_u32Value			= (U32)(gdsBshMonCalRes[u8BSHNo].m_dsBusV[PHASE_A].m_fMag*100.);

	gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex].m_dsBSHStatus[ENUM_BSH_DISPLACEMENT_I].m_u32Value	= (U32)(gdsBshMonCalRes[u8BSHNo].m_fDisplI[PHASE_A]*100.);

	gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex].m_dsBSHStatus[ENUM_BSH_LEAKAGE_I].m_u32Value		= (U32)(gdsBshMonCalRes[u8BSHNo].m_dsBshLeakI[PHASE_A].m_fMag*100.);
	gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex].m_dsBSHStatus[ENUM_BSH_LEAKAGE_I_ANG].m_u32Value	= (U32)(gdsBshMonCalRes[u8BSHNo].m_dsBshLeakI[PHASE_A].m_fAng*100.);

	gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex].m_dsBSHStatus[ENUM_BSH_VOLTAGE_ANG].m_u32Value		= (U32)(gdsBshMonCalRes[u8BSHNo].m_dsBusV[PHASE_A].m_fAng*100.);

	gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex].m_dsBSHStatus[ENUM_BSH_POWER_FACTOR].m_u32Value		= (U32)(gdsBshMonCalRes[u8BSHNo].m_fPwrFactor[PHASE_A]*100.);

	gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex].m_dsBSHStatus[ENUM_BSH_UNBALANCE_I].m_u32Value		= (U32)(gdsBshMonCalRes[u8BSHNo].m_dsBshLeak3I0.m_fMag*100.);
	gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex].m_dsBSHStatus[ENUM_BSH_UNBALANCE_I_ANG].m_u32Value	= (U32)(gdsBshMonCalRes[u8BSHNo].m_dsBshLeak3I0.m_fAng*100.);
	#endif

	gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex].m_u32CRC	= (U32)CalculateCRC((U8*)(&(gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex])), (SIZE_OF_BSH_DATA_UNIT_IN_BYTES-4));

	memcpy(datalist->m_dsDataList[datalist->m_u32WriteIndex].m_pData, &(gdsBSHBufferForIEC61850[u8BSHNo][datalist->m_u32WriteIndex]), SIZE_OF_BSH_DATA_UNIT_IN_BYTES);
	struct timeval dsSysTime;
	gettimeofday(&dsSysTime, NULL);
		
	//memcpy(&datalist->m_timelist[datalist->m_u32WriteIndex],&dsSysTime, sizeof(MMS_UTC_TIME2));
	datalist->m_timelist[datalist->m_u32WriteIndex].secs = dsSysTime.tv_sec;
	datalist->m_timelist[datalist->m_u32WriteIndex].fraction = dsSysTime.tv_usec;


	if (++datalist->m_u32WriteIndex == SIZE_OF_BSH_DATA_LIST_TO_CU) {
		datalist->m_u32WriteIndex = 0;
	}
}


void ExecuteMTrMonitoring()
{
	// for test
	#if (1)
	static int i=0;
	#endif


	// PDDAU I/F
	// OLTC driving motor monitoring
	ReadMeasurementForOLTCMon();
	#if (0)
	ReadOLTCOpWaveData();
	WriteOLTCOpWaveData();
	#endif

	// Bushing leakage current monitoring
	ReadMeasurementForBshMon();
	ExecuteCalculationForBshMon();

	// DGA monitoring
	WriteEachDGAMeasurementToIEC61850(dga1data, 0);
	WriteEachDGAMeasurementToIEC61850(dga2data, 1);
	WriteEachDGAMeasurementToIEC61850(dga3data, 2);
	WriteEachDGAMeasurementToIEC61850(dga4data, 3);
	WriteEachDGAMeasurementToIEC61850(dga5data, 4);
	WriteEachDGAMeasurementToIEC61850(dga6data, 5);

	// OLTC monitoring
	WriteEachOLTCMeasurementToIEC61850(oltc1data, 0);
	WriteEachOLTCMeasurementToIEC61850(oltc2data, 1);
	WriteEachOLTCMeasurementToIEC61850(oltc3data, 2);

	// Bushing leakage current monitoring
	WriteEachBSHMeasurementToIEC61850(bsh1data, 0);
	WriteEachBSHMeasurementToIEC61850(bsh2data, 1);
	// for test
	#if (1)
	for (i = 0;i < MAX_MTR_OLTCS;i++) {
		WriteEachOLTCWaveToIEC61850(i);
	}

	#endif
}
