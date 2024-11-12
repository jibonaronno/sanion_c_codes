/*
 * main.h
 *
 *  Created on: Apr 15, 2017
 *      Author: root
 */

#ifndef MAIN2_H_
#define MAIN2_H_

#include "Common.h"
#include "build_defs.h"
// #include "scl.h"
#include <termios.h>

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 **                                                                         **
 **      장치 정보                                                             **
 **                                                                         **
 *****************************************************************************/

#define SANION_NAME			"(주)세니온"
#define BOARD_NAME			"HUR-IED"
#define PRODUCT_NAME     	"HUR-IED" 

#define MPU_FW_VERSION				100
#define MPU_FW_VERSION_YEAR			21
#define MPU_FW_VERSION_MON			01
#define MPU_FW_VERSION_DAY			01
#define MPU_FW_UPDATE_VERSION		13
#define MPU_FW_UPDATE_VERSION_YEAR	BUILD_YEAR
#define MPU_FW_UPDATE_VERSION_MON	BUILD_MONTH
#define MPU_FW_UPDATE_VERSION_DAY	BUILD_DAY
#define MPU_FW_UPDATE_VERSION_HOUR	BUILD_HOUR
#define MPU_FW_UPDATE_VERSION_MIN	BUILD_MIN
#define MPU_FW_UPDATE_VERSION_SEC	BUILD_SEC
#define HARDWARE_VERSION			100

#define	YEAR_MAX_MASK				99 

// 외부 시각 소스에서 OS 내부 시각을 동기
#define CMD_TIMEFROMNET      "rdate "
#define CMD_TIMEFROMRTC      "hwclock -s"
#define CMD_TIMETORTC        "hwclock -w"
#define CMD_SETTIME          "date -s "

/******
 * Following defs came from 61850 lib code <SEPT-9>
*******/

#define SPC 1
#define DPC 2
#define MAX_LUS 250
#define MAX_LNS 250

#define ST_CHAR    char
#define ST_INT     signed int
#define ST_LONG    signed long int
#define ST_UCHAR   unsigned char
#define ST_UINT    unsigned int
#define ST_ULONG   unsigned long
#define ST_VOID    void
#define ST_DOUBLE  double
#define ST_FLOAT   float

/* We need specific sizes for these types                               */
#define ST_INT8     signed char
#define ST_INT16    signed short
#define ST_INT32    signed int                  /* was signed long      */
#define ST_INT64    __int64
#define ST_UINT8    unsigned char
#define ST_UINT16   unsigned short
#define ST_UINT32   unsigned int                /* was unsigned long    */
#define ST_UINT64   unsigned __int64
#define ST_BOOLEAN  unsigned char

typedef struct {
	ST_CHAR* ps8name;
	ST_CHAR* ps8ip_in;
	ST_INT   s16id_ln;
} dsLN;

typedef struct { 
	ST_CHAR ps8luName[32];
	ST_CHAR* ps8luIP;
	ST_INT8 s8numLNs;
	dsLN LNList[MAX_LNS];
} dsLU;

typedef struct {
	ST_INT8 s8numLUs;
	dsLU LUList[MAX_LUS];
} dsLUConfig;

/******
 * Above defs came from 61850 lib code </SEPT-9>
*******/

struct bitmap
{ 
	S8* ps8datatype; 
	void* pbitmapaddr; 
	S32 s32numofbit; 
	S32 s32bitpos; 
	U8   u8dbtype;
	S8   u8order;
	U8 u8isphydi;
	U32  u32dbidx;	
}; 
typedef struct bitmap BITMAP;

typedef struct{
	S8* ps8leafname;	
	S8* ps8leaftype;	
	S8 gcRef [130];	  /* GoCBRef in IEC 61850 */
	U8 u8gcbidx;
	U8  u8mapped;	
	U8* pu8changedetected;
	U8* pu8enabled;	
	U8* pu8justenabled;
	U32 u32oldgooseval;
	void* pvmdptr;
}dsGOOSEPUBMappingTbl;

typedef struct {
	S8* ps8leafname;
	S8* ps8leaftype;
	S8* ps8LN;
	S8  ps8IPAddress[16];
	S32 u32oldval;
	S8 u8PBCUPubtype;
	S8 u8PBCUSubtype;
	S8 u8PBCUnumber;
	S16   s16parsedLNID;
	void* pvmdptr;
	void* pvmdtimeptr;	
	void* pvmdbehptr;
	BITMAP* pbitmap;		
	dsGOOSEPUBMappingTbl* pstgoosemap;	
}ds61850MappingTbl;

typedef struct{  
	U16 s16startidx;  
	U16 s16endidx;  
}dsdbidx;  

typedef struct{
	U16 u16numdata;
	void** pvmdptrs;
}dsLPHDSim;

typedef struct{
	dsLPHDSim* plphdsim; 
}dsLPHDInfo;

typedef struct{
	void* pvmdptr;
	void* pvmdtimeptr;
}dsLLN0Mod;

typedef struct{
	dsLLN0Mod* plln0mod;
}dsLLN0Info;

typedef struct{
	void* pvmdlcch1ptr;
	void* pvmdlcch1timeptr;
	void* pvmdlcch2ptr;
	void* pvmdlcch2timeptr;
}dsLCCHInfo;

typedef struct{  
	dsdbidx astDbidx[4];  
	ds61850MappingTbl* p61850mappingtbl;
	dsLPHDInfo* plphdinfo;
	dsLLN0Info* plln0info;
	dsLCCHInfo* plcchinfo;
	S8*			ps8LDname;
	S8			ps8IPAddr[16];
	S8  		ps8IP_SUBNET[16];
	S8	   		ps8IP_GW[16];	
	int32_t			s32leafnum;
}ds61850IfcInfo;  


#define GLU		12
#define MLU		16
typedef enum __SENSOR_CLASS
{
	SCBR,
	SPDC,
	SBSH,
	SLTC,
	SIML
}SENSOR_CLASS;


void sbcr_acq_modbus_run(void);
int g_Start_Thrad(pthread_t thread_id, pthread_attr_t init_attr, void *(func)(void *), void * thread_arg);
int RunThread(void);
int main(void);
int writeVmdPointerByLeafName_time(char *leafName, time_t vlu);
int writeVmdPointerByLeafName_float32(char *leafName, float vlu);
int writeVmdPointerByLeafName_int32(char *leafName, U32 vlu);
void printSample_int16(short *buf16);

void* subsystem_thread(void *params);

ST_BOOLEAN parseLUConfig(const ST_CHAR* ps8filename, dsLUConfig *pdsconfig);

#ifdef __cplusplus
}
#endif /* defined (__cplusplus) */
#endif /* MAIN2_H_ */
