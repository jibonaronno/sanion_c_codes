#ifdef __cplusplus
extern “C” {
#endif

#include <pthread.h>
#include <sys/time.h>

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
    float    contact_op_time_A;
    float    contact_op_time_B;
    float    block_close_time_A;
    float    block_close_time_B;
    float    block_close_time_C;
}CBEVENT;


#define ACQ_ADDR    "192.168.10.100"
#define ACQ_PORT    100

//char* modbus_error(int err_code);
//int connect_tcp(unsigned char *ipaddr, int iPort);

int openAcqBus(void);
void closeModbus(void);
int readAll(void);
int readSingleBlock(int sz, u16 address, uint16_t *mems);
int readMultiBlock(int blkSize, int numberOfBlocks, u16 address, uint16_t *mems);
void testAcqConnection(void);
void writeDateTime(int min, int hour, int day, int mon, int year, int sync_systime);
int checkRecordFlags(void);

#ifdef __cplusplus
}
#endif
