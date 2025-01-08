#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <sys/time.h>
#include <stdint.h>
#include "global.h"
#include "modbus.h"
#include "main.h"

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
#define DT 0.1302

#define MODBUS_PORT     100

#define BLOCKSIZE       64
#define NUMBEROFBLOCKS_A  36
#define NUMBEROFBLOCKS_B  18

#define CBEVENT_BLOCK_SIZE          34
#define CBEVENT_NUMBEROFBLOCKS      1
#define CBEVENT_BLOCK_OFFSET        1200

#define SETTINGS_BLOCK_SIZE         13
#define SETTINGS_BLOCK_OFFSET       1029
#define SETTINGS_ARRAY_SIZE         30

#define ACQ_RAWDATA_ALLOCATION_SIZE     3664
#define CB_ELEMENT_ARRAY_SIZE           2304

#define TRIP1_OFFSET            1300
#define TRIP2_OFFSET            3604
#define CLOSECOIL_OFFSET        5908
#define VOLTAGE_OFFSET          8212
#define PHASEA_CURR_OFFSET      10516
#define PHASEB_CURR_OFFSET      12820
#define PHASEC_CURR_OFFSET      15124
#define INITANDCONTACT_OFFSET   17428

typedef struct _CB_DATA
{
    uint16_t trip1_coil_current[CB_ELEMENT_ARRAY_SIZE];
    int16_t trip2_coil_current[CB_ELEMENT_ARRAY_SIZE];
    uint16_t close_coil_current[CB_ELEMENT_ARRAY_SIZE];
    uint16_t phase_current_A[CB_ELEMENT_ARRAY_SIZE];
    uint16_t phase_current_B[CB_ELEMENT_ARRAY_SIZE];
    uint16_t phase_current_C[CB_ELEMENT_ARRAY_SIZE];
    char     initiate_and_contact[CB_ELEMENT_ARRAY_SIZE];
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
    float    coil_female_time_t1;
    float    coil_integral_t2;
    float    coil_max_current_t2;
    float    coil_female_time_t2;
    float    coil_integral_close;
    float    coil_max_current_close;
    float    coil_female_time_close;
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

int openAcqBus(char *ap_addr);
void closeModbus(void);
int readAll(void);
int readSingleBlock(int sz, u16 address, uint16_t *mems);
int readMultiBlock(int blkSize, int numberOfBlocks, u16 address, uint16_t *mems);
void testAcqConnection(void);
void writeDateTime(int min, int hour, int day, int mon, int year, int sync_systime);
int checkRecordFlags(void);
int prepareFileData(void);
void save_file(struct tm *tmm);
time_t readAcqDateTime(struct tm *tvlu);
time_t readDateTime2(void);
void Construct(void *acqobj);

typedef struct _ACQ_STRUCT
{
    uint16_t setsA[30];
    uint16_t flag_Records[2];
    u16 tbl[ACQ_RAWDATA_ALLOCATION_SIZE];
    u16 t1_buf[ACQ_RAWDATA_ALLOCATION_SIZE];
    u16 t2_buf[ACQ_RAWDATA_ALLOCATION_SIZE];
    u16 close_buf[ACQ_RAWDATA_ALLOCATION_SIZE];
    u16 currA_buf[ACQ_RAWDATA_ALLOCATION_SIZE];
    u16 currB_buf[ACQ_RAWDATA_ALLOCATION_SIZE];
    u16 currC_buf[ACQ_RAWDATA_ALLOCATION_SIZE];
    u16 init_cont_buf[ACQ_RAWDATA_ALLOCATION_SIZE];
    u8 *init_buf;
    modbus_t *busA;
    bool_type is_modbus_opened;
    void (*_Construct)(void *);

}ACQ_STRUCT;

#ifdef CPP_BLOCK_ENABLED
class _ACQ
{
    public:
        _ACQ(void);
        int openAcqBus(const char *ip_addr, int Port);
        void closeModbus(void);
        int readAll(void);
        int writeSingleBlock(int sz, u16 address, uint16_t *mems);
        int readMultiBlock(int blkSize, int numberOfBlocks, u16 address, uint16_t *mems);
        int readTrip1(void);
        int readTrip2(void);
        int readCloseCoil(void);
        int readVoltage(void);
        int readPhaseACurr(void);
        int readPhaseBCurr(void);
        int readPhaseCCurr(void);
        int readInitiateAndContact(void);
        void parseIPAddressesFromJson(const char *_filename);

        modbus_t *busA;
        bool_type is_modbus_opened;

        int acq_index;
        void printSample(int lidxA);
        uint16_t setsA[30];
        uint16_t flag_Records[2];
        u16 tbl[ACQ_RAWDATA_ALLOCATION_SIZE];
        u16 t1_buf[ACQ_RAWDATA_ALLOCATION_SIZE];
        u16 t2_buf[ACQ_RAWDATA_ALLOCATION_SIZE];
        u16 close_buf[ACQ_RAWDATA_ALLOCATION_SIZE];
        u16 currA_buf[ACQ_RAWDATA_ALLOCATION_SIZE];
        u16 currB_buf[ACQ_RAWDATA_ALLOCATION_SIZE];
        u16 currC_buf[ACQ_RAWDATA_ALLOCATION_SIZE];
        u16 init_cont_buf[ACQ_RAWDATA_ALLOCATION_SIZE];
        char ip_address[16];
        u8 *init_buf;
};

#endif

#ifdef __cplusplus
}
#endif
