#ifdef __cplusplus
extern “C” {
#endif

#include <pthread.h>
#include <sys/time.h>

#include "modbus_list.h"  // doubly linked lists - a slightly modified <linux/list.h>

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

#define MB_FN_READ_COILS     0x01
#define MB_FN_READ_DISCRETES 0x02
#define MB_FN_READ_HOLD_REGS 0x03
#define MB_FN_READ_IN_REGS   0x04
#define MB_FN_FORCE_SGL_COIL 0x05
#define MB_FN_PRESET_SGL_REG 0x06
#define MB_FN_WRITE_MUL_COIL 0x0F
#define MB_FN_WRITE_MUL_REGS 0x10

// The generic Modbus code in modbus.c will pre-allocate enough
// space in the frame buffers for the RTU checksum and Modbus/TCP
// headers, so that the hardware-specific code can use the same
// buffer without memcopies.
#define TCP_HEADER_SIZE    6
#define RTU_CHECKSUM_SIZE  2     // CRC = 2 bytes
#define ASC_CHECKSUM_SIZE  1     // LRC = 1 byte (= 2 characters)
#define ASC_OVERHEAD       3     // 1 colon + CR + LF = 3 characters

#define REQUEST_QUERY_SIZE 6     // the basic request
#define BASIC_RESPONSE_SIZE 6    // a bare response: UID=1B, FN=1B, START=2B, COUNT=2B
#define MAX_QUERY_LENGTH 254     // as per Modbus spec, 253 B PDU + 1 B slave address
#define MAX_RESPONSE_LENGTH 254  // as per Modbus spec, 253 B PDU + 1 B slave address
#define MAX_ASC_FRAME_SIZE 513   // (MAX_QUERY_LENGTH + LRC) * 2 + ASC_OVERHEAD

#define MAX_READ_REGS    125 // == (MAX_RESPONSE_LENGTH - slave - fn - byte_cnt) / 2
#define MAX_READ_COILS  2000 // == (MAX_RESPONSE_LENGTH - slave - fn - byte_cnt) * 8
#define MAX_WRITE_COILS 1968 // == (MAX_QUERY_LENGTH    - slave - fn - start - count - byte_cnt) * 8
#define MAX_WRITE_REGS   123 // == (MAX_QUERY_LENGTH    - slave - fn - start - count - byte_cnt) / 2



// benign errors - reopen perhaps wouldn't help
#define ERR_OK 0  // may also mean zero bytes sent / rcvd at the level of send_frame()/recv_frame()
// vvv the errors below correspond to recognized Modbus error codes:
// These (plus ERR_OK) are the only error responses that make sense in slave callbacks.
#define ILLEGAL_FUNCTION       -1
#define ILLEGAL_DATA_ADDRESS   -2
#define ILLEGAL_DATA_VALUE     -3
#define SLAVE_DEVICE_FAILURE   -4
#define ACKNOWLEDGE            -5
#define SLAVE_DEVICE_BUSY      -6
#define NEGATIVE_ACKNOWLEDGE   -7
#define MEMORY_PARITY_ERROR    -8
#define GW_PATH_UNAVAILABLE  -0xA    // gateway error code
#define GW_TRGT_NO_RESPONSE  -0xB    // gateway error code
// ^^^ the errors above correspond to recognized Modbus error codes
// low-level framing and other generic errors:
#define RCVD_FRAME_INVALID    -16
#define QRY_RESP_LEN_MISMATCH -17
#define TOO_FEW_BYTES_SENT    -18
#define CRC_ERROR             -19
#define ERR_TIMEOUT           -20
#define ERR_BUS_DOWN          -21
#define INTERNAL_ERR          -22
// slave processing result codes:
#define UPPER_FRAMING_ERROR   -23
#define RESP_WHERE_REQ_XPCTD  -24

// serious errors - reopen() is due
//  The first one, REOPEN_DUE, is a "pseudo-target", not an actual error
#define REOPEN_DUE           -100
#define PORT_FAILURE         -110
#define SOCKET_FAILURE       -111

// Each open bus, master or slave, has its own garbage-collector thread.
// The garbage collector wakes up every once in a while (see the periods
// below) and scans all queues for transactions that have timed out.
#define MASTER_GARBAGE_PERIOD 200000   // period of the garbage collector loop, in microseconds
#define SLAVE_GARBAGE_PERIOD  500000   // dtto.

// Each transaction has a timestamp - the time when it was enqueued.
// The garbage collector checks the timestamps against current system time
// and cleans up transactions that have timed out waiting in some queue.
#define SLAVE_RX_TRANS_TIMEOUT      20000  // should be dispatched within a few ms
#define MASTER_TX_TRANS_TIMEOUT    500000  // shouldn't take long - slow lines mean long frame times 
#define MASTER_PEND_TRANS_TIMEOUT 2000000  // slow lines, GSM links etc can add a lot of delay
#define MASTER_RX_TRANS_TIMEOUT     20000  // should be dispatched within a few ms
#define SLAVE_TX_TRANS_TIMEOUT     500000  // shouldn't take long - slow lines mean long frame times 




// Symbollic names for the various common fields in the Modbus
// frame structure or modbus data array.
#define MB_UID     0
#define MB_FN      1
#define MB_BYTECNT 2    // in read_multi response
#define MB_ERR     2    // in error response - error value
#define MB_PAYLOAD 3
#define MB_START   2    // in a request -- [MB_START] = msB, [MB_START+1] = lsB
#define MB_COUNT   4    // in a read_multi or set_multi request -- [MB_COUNT] = msB, [MB_COUNT+1] = lsB
#define MB_MULTI_BYTECNT 6    // in write_multi request
#define MB_MULTI_PAYLOAD 7    // in write_multi request

struct modbus_msg
{
   u8 buf[ TCP_HEADER_SIZE + MAX_QUERY_LENGTH + RTU_CHECKSUM_SIZE ]; // fortunately, MAX_QUERY_LENGTH == MAX_RESPONSE_LENGTH
   u8* base;
   int len;
};


/* Following modbus_trans object will be used for a single transaction tracking to keep 
 * record of request and answer.
 */
struct modbus_c;     // a forward declaration

struct modbus_trans  // a modbus transaction
{
   struct list_head hook;
   // Two buffers per direction instead of one,
   // to allow easier mangling and un-mangling in relay-style applications
   struct modbus_msg rx_req;
   struct modbus_msg tx_req;
   struct modbus_msg rx_resp;
   struct modbus_msg tx_resp;

   struct modbus_c* src_bus;  // for relayed transactions
   u8 am_local;               // nonzero value indicates a locally originated transaction
   //-pthread_t src_thread;    // for locally originated transaction - not necessary with cond+mutex
   pthread_cond_t cond;
   pthread_mutex_t mutex;
   struct timeval time_of_enq;
   int err_code;
   u8* pending_latch;
};



struct modbus_trans_q
{
   struct list_head q;
   pthread_mutex_t mutex;
   pthread_cond_t cond;
};



#define TCP_BUS 1
#define RTU_BUS 2
#define ASCII_BUS 3

#define MASTER_BUS 1
#define SLAVE_BUS 2
#define LISTENER_BUS 3 // a special case with Modbus/TCP

struct modbus_c
{
   int use_count;
   int fd;
   u8 finished;
   u8 explicit_close;  // for TCP slave children, that can be closed explicitly or die implicitly
   u8 link_type;  // TCP, RTU, ASCII - runtime type identification
   u8 m_s;        // master/slave
   pthread_mutex_t bus_init_mutex;
   pthread_mutex_t use_count_mutex;

   struct modbus_trans_q rx_q;
   struct modbus_trans_q tx_q;
   struct modbus_trans_q pend_q;  // only any use on a master bus

   pthread_mutex_t tcq_mutex;     // If we want q_trans() to use the generic per-queue cond+mutex,
   pthread_cond_t tcq_cond;       // we need another set for TCQ alone, though their logic will
   int tcq_depth; // 0=unlimited  // overlap with that of pend_q->{cond+mutex}
   int tcq_pending;
   u8 trans_still_pending;        // for TCQ sync on master busses
   u8 return_nonlocal_timeouts;   // for relay apps (transaction level, avoiding run_trans())
   u8 return_all_trans_errs;   // for generic transaction-level apps, interested in trans.errors

   u32 frame_brk_timeout;         // in microseconds - the maximum delay between two characters rcvd
   u32 tx_timeout;                // in microseconds - the maximum time before giving up send_frame()
   u32 pend_timeout;              // in microseconds
   u8 dont_retry;                 // 1 = modbus_open() shall return with an error immediately
                                  //     if device open() or socket connect() fails.
                                  // 0 = keep retrying forever (for fault tolerance)

   // the following functions are polymorphic between TCP/RTU/ASCII
   int (*init_modbus) (struct modbus_c* bus);
   int (*inval_modbus)(struct modbus_c* bus); // "down" the port
   int (*send_frame)  (struct modbus_c* bus, u8 *tx_buf, size_t string_length);
   // timeout in microseconds, 0 = wait forever
   int (*recv_frame)  (struct modbus_c* bus, u8 *rx_buf, u32 timeout);
   int (*dtor)        (struct modbus_c* bus);

   void* (*rx_thr)(void* owner_bus); // always present but polymorphic  (master/slave)
   void* (*tx_thr)(void* owner_bus); // always present but polymorphic  (master/slave)
   void* (*dispatch_thr)(void* owner_bus); // always present but polymorphic  (master/slave)
   void* (*garbage_thr)(void* owner_bus); // always present but polymorphic  (master/slave)
   pthread_t rx_ID;
   pthread_t tx_ID;
   pthread_t dispatch_ID;
   pthread_t garbage_ID;
   
   // The following callbacks should be provided by the library user.
   // For slave busses, either specify a custom dispatch_func() using
   // the respective new_modbus*(), or set the individual callbacks using
   // set_slave_callbacks().
   void (*dispatch_func)(struct modbus_trans* this_trans);
   int (*our_id)(struct modbus_trans* this_trans); // used by generic_slave_dispatch(); 0 = yes, >0 = no
   int (*read_coil_status_cback)(         struct modbus_c* bus, u8 slave, u16 start_addr, u16 count, bool_type *dest);
   int (*read_discrete_input_cback)(      struct modbus_c* bus, u8 slave, u16 start_addr, u16 count, bool_type *dest);
   int (*read_holding_registers_cback)(   struct modbus_c* bus, u8 slave, u16 start_addr, u16 count, u16 *dest);
   int (*read_input_registers_cback)(     struct modbus_c* bus, u8 slave, u16 start_addr, u16 count, u16 *dest);
   int (*force_single_coil_cback)(        struct modbus_c* bus, u8 slave, u16 addr, bool_type state);
   int (*preset_single_register_cback)(   struct modbus_c* bus, u8 slave, u16 reg_addr, u16 value);
   int (*set_multiple_coils_cback)(       struct modbus_c* bus, u8 slave, u16 start_addr, u16 coil_count, bool_type *data);
   int (*preset_multiple_registers_cback)(struct modbus_c* bus, u8 slave, u16 start_addr, u16 reg_count, u16 *data);
};


#define ACQ_ADDR    "192.168.10.100"
#define ACQ_PORT    100

char* modbus_error(int err_code);

int connect_tcp(unsigned char *ipaddr, int iPort);

#ifdef __cplusplus
}
#endif
