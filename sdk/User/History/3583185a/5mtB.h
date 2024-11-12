#ifdef __cplusplus
extern “C” {
#endif

#include <pthread.h>
#include <sys/time.h>
#include <stdint.h>

#ifndef u32
typedef unsigned long int u32;
#endif

#ifndef u16
typedef unsigned short int u16;
#endif

#ifndef u8
typedef unsigned char u8;
#endif

#define bool_type u16

#define FALSE 0 
#define TRUE 1
#define TIMEOUT_YES 1000000  // one second

typedef struct _CB_DATA
{
    uint16_t trip1_coil_current[2304];
    int16_t trip2_coil_current[2304];
    uint16_t close_coil_current[2304];
    uint16_t phase_current_A[2304];
    uint16_t phase_current_B[2304];
    uint16_t phase_current_C[2304];
    char     initiate_and_contact[2304];
}CB_DATA;

typedef struct _CB_FILE
{
    uint8_t  event_type;
    uint32_t event_datetime;
    uint32_t event_millisec;
    uint8_t  alert_level;
    float    contact_duty_A;
    float    contact_duty_B;
    float    contact_duty_C;
    float    accum_contact_duty_A;
    float    accum_contact_duty_B;
    float    accum_contact_duty_C;
    float    coil_integral_t1;
    float    coil_max_current_t1;
    uint32_t    coil_female_time_t1;
    float    coil_integral_t2;
    float    coil_max_current_t2;
    uint32_t    coil_female_time_t2;
    float    coil_integral_close;
    float    coil_max_current_close;
    uint32_t    coil_female_time_close;
    float    contact_optime_A;
    float    contact_optime_B;
    float    block_close_time_A;
    float    block_close_time_B;
    float    block_close_time_C;
    uint32_t op_cnt;
    uint16_t smp_per_cyc;
    uint16_t cyc_count;

    CB_DATA  cb_data;
}CB_FILE;


typedef struct _CBEVENT
{
    uint16_t event_number;
    uint16_t event_type;
    uint32_t event_datetime;
    uint32_t event_millisec;
    float    contact_duty_A;
    float    contact_duty_B;
    float    contact_duty_C;
    float    tc1_peak_curr;
    float    tc1_curr_flow;
    float    tc2_peak_curr;
    float    tc2_curr_flow;
    float    cc_peak_curr;
    float    cc_curr_flow;
    float    contact_optime_A;
    float    contact_optime_B;
    float    block_close_time_A;
    float    block_close_time_B;
    float    block_close_time_C;
}CBEVENT;

/*
AbrAlm
AbrWrn
OpTmAlm
ColAlm
OpCntAlm
OpCntWrn
OpTmWrn
OpTmh
EvtTransF
RsOpCnt
MoDevComF
MoDevFlt
CbAlm
AccAbr
SwA
ActAbr 
AuxSwTmOp
AuxSwTmCl
RctTmOpn 
RctTmCls 
OpSpdOpn 
OpSpdCls 
OpTmOpn 
OpTmCls 
Stk 
OvStkOpn 
OvStkCls 
ColA
Tmp
ColV
ClsColA
OpnColA
BctA
BctB
BctC
HWflt
AbrAlmLev
AbrWrnLev
OpAlmTmh
OpWrnTmh
OpAlmNum
OpWrnNum
*/

typedef struct _SHM_PRIME
{
int    AbrAlm;
int    AbrWr;
int    OpTmAlm;
int    ColAlm;
int    OpCntAlm;
int    OpCntWrn;
int    OpTmWrn;
int    OpTmh;
int    EvtTransF;
int    RsOpCnt;
int    MoDevComF;
int    MoDevFlt;
int    CbAlm;
int    AccAbr;
int    SwA;
int    ActAbr;
int    AuxSwTmOp;
int    AuxSwTmCl;
int    RctTmOpn;
int    RctTmCls;
int    OpSpdOpn;
int    OpSpdCls;
int    OpTmOpn;
int    OpTmCls;
int    Stk;
int    OvStkOpn;
int    OvStkCls;
int    ColA;
int    Tmp;
int    ColV;
int    ClsColA;
int    OpnColA;
int    BctA;
int    BctB;
int    BctC;
int    HWflt;
int    AbrAlmLev;
int    AbrWrnLev;
int    OpAlmTmh;
int    OpWrnTmh;
int    OpAlmNum;
int    OpWrnNum;
}SHM_PRIME;


#define ACQ_ADDR    "192.168.10.100"
#define ACQ_PORT    100

//char* modbus_error(int err_code);
//int connect_tcp(unsigned char *ipaddr, int iPort);

int openAcqBus(void);
void closeModbus(void);
int readAll(void);
void EraseList(void);
int readSingleBlock(int sz, u16 address, uint16_t *mems);
int readMultiBlock(int blkSize, int numberOfBlocks, u16 address, uint16_t *mems);
void testAcqConnection(void);
void writeDateTime(int min, int hour, int day, int mon, int year, int sync_systime);
int checkRecordFlags(void);
int prepareFileData(void);
void save_file(struct tm *tmm);
time_t readAcqDateTime(struct tm *tvlu);
time_t readDateTime2(void);

#ifdef __cplusplus
}
#endif
