#ifndef MODBUS__H
#define MODBUS__H

            // ################################### //
            //                                     //
            //             Modbus for C            //
            //                                     //
            //          by Frank Rysanek           //
            //   of FCC Prumyslove Systemy s.r.o.  //
            //     [ rysanek AT fccps DOT cz ]     //
            //                                     //
            //      Based on earlier works of      //
            //   Paul McRae and Philip Costigan    //
            //                                     //
            // ################################### //

#if ((! defined MODBUS_ASC_H) && (! defined MODBUS_RTU_H) && (! defined MODBUS_TCP_H) && (! defined MODBUS_OWN) && (! defined MODBUS_CPP_OWN))
#error "Don't #include <modbus.h> directly!"
#error "Include either modbus_asc.h or modbus_rtu.h or modbus_tcp.h or all."
#error "This file (modbus.h) contains some common abstract classes"
#error "that are actually implemented by the two specialized header files."
#endif // direct inclusion check

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <sys/time.h>

#ifdef DEBUG
#include <stdio.h>
#include <syslog.h>
#endif

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
//#define bool_type u8

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

char* modbus_error(int err_code);



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
// frame structure
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


// Functions declared below comprise the uniform public interface
// (can and should be called directly).


// ### Library init and cleanup ###

int modbus_init();    // call this at program startup - this is to set up Modbus memory mngt.
int modbus_cleanup(); // call this at program shutdown - this will clean up Modbus memory mngt.


// ### Bus CTOR and DTOR ###

// create busses with the link-specific new_*()
int modbus_destroy(struct modbus_c* bus); // use this to destroy busses created using new_*()


// ### Open & close ###

// Call modbus_open() e.g. in each thread using this bus,
// or on behalf of every slave if you use per-slave objects.
// The main purpose of modbus_open() it to increase use_count,
// and only the first invocation of modbus_open() actually calls 
// open(device) or connect(socket).
// 
// Even the first invocation of modbus_open() can be told NOT TO actually
// open the socket/node. The worker threads detect this condition and can
// handle this autonomously. This is useful in multi-bus applications
// where the failure to open an individual bus is tolerated, and the
// application must start up quick as a whole in the first place.
//
// On the other hand, you can also tell modbus_open() to give up immediately
// if for some reason the socket or device cannot be open (no such device,
// destination host unreachable). The default behavior (0) is to keep trying.
int modbus_open(struct modbus_c* bus, u8 dont_open_yet, u8 dont_retry);

// Call modbus_open() e.g. in each thread using this bus.
// The main purpose of modbus_close() it to decrease use_count.
// Only the last invocation of modbus_close() actually calls 
// close() on the port descriptor / TCP socket.
int modbus_close(struct modbus_c* bus);


// ### High-level master functionality ###

int read_coil_status(          struct modbus_c* bus, u8 slave, u16 start_addr, u16 count, bool_type *dest);
int read_discrete_input(       struct modbus_c* bus, u8 slave, u16 start_addr, u16 count, bool_type *dest);
int read_holding_registers(    struct modbus_c* bus, u8 slave, u16 start_addr, u16 count, u16 *dest);
int read_input_registers(      struct modbus_c* bus, u8 slave, u16 start_addr, u16 count, u16 *dest);
int force_single_coil(         struct modbus_c* bus, u8 slave, u16 addr, bool_type state);
int preset_single_register(    struct modbus_c* bus, u8 slave, u16 reg_addr, u16 value);
int set_multiple_coils(        struct modbus_c* bus, u8 slave, u16 start_addr, u16 coil_count, bool_type *data);
int preset_multiple_registers( struct modbus_c* bus, u8 slave, u16 start_addr, u16 reg_count, u16 *data);


// ### High-level slave functionality ###

/*
 * set_slave_callbacks() is used to set the high-level slave callback functions.
 * I.e., use this if you need local slave functionality, rather than relay logic.
 *
 * our_id() is mandatory, all the others are optional.
 * 
 * The our_id() callback is used to tell the library (the particular bus),
 * which Modbus ID's on that bus are ours. ID's decoded as "not ours"
 * are silently ignored.
 * 
 * Feel free to copy+paste the following code snippet to save typing.
 * Check the individual callback function prototypes
 * in the actual set_slave_callbacks prototype below.
 * 
   tmp_retval = set_slave_callbacks(
      my_bus,
      my_our_id,      // our_id()     0: yes, 1: no
      NULL, // read_coil_status_cback()
      NULL, // read_discrete_input_cback()
      NULL, // read_holding_registers_cback()
      NULL, // read_input_registers_cback()
      NULL, // force_single_coil_cback()
      NULL, // preset_single_register_cback()
      NULL, // set_multiple_coils_cback()
      NULL  // preset_multiple_registers_cback()
   );
*/

int set_slave_callbacks(
   struct modbus_c* bus,
   int (*our_id)(                         struct modbus_trans* this_trans), // 0: yes, 1: no
   int (*read_coil_status_cback)(         struct modbus_c* bus, u8 slave, u16 start_addr, u16 count, bool_type *dest),
   int (*read_discrete_input_cback)(      struct modbus_c* bus, u8 slave, u16 start_addr, u16 count, bool_type *dest),
   int (*read_holding_registers_cback)(   struct modbus_c* bus, u8 slave, u16 start_addr, u16 count, u16 *dest),
   int (*read_input_registers_cback)(     struct modbus_c* bus, u8 slave, u16 start_addr, u16 count, u16 *dest),
   int (*force_single_coil_cback)(        struct modbus_c* bus, u8 slave, u16 addr, bool_type state),
   int (*preset_single_register_cback)(   struct modbus_c* bus, u8 slave, u16 reg_addr, u16 value),
   int (*set_multiple_coils_cback)(       struct modbus_c* bus, u8 slave, u16 start_addr, u16 coil_count, bool_type *data),
   int (*preset_multiple_registers_cback)(struct modbus_c* bus, u8 slave, u16 start_addr, u16 reg_count, u16 *data)
);


// ### Transaction-level functionality ###

// It should be safe to use these if you're experimenting
// at transaction level (trying to issue custom master transactions).
struct modbus_trans* get_modbus_trans();
void free_modbus_trans(struct modbus_trans* this_trans);

// Use this in relay-style applications, to queue requests to the
// master bus and to queue responses to the slave bus.
// Leaves trans->am_local alone => requires a dispatch_func to be set.
// On slave busses, trans->tx_resp will be transmitted.
// On master busses, trans->tx_req will be transmitted and the transaction
// becomes pending - an eventual response will come back in a dispatch_func()
// that was previously provided via the new_*() function.
// Transactions that time out will just vanish, even on master busses.
int q_trans(struct modbus_c* bus, struct modbus_trans* trans);
// Combine q_trans with a custom dispatch_func.

// Use this to queue + wait for custom transactions.
// Set trans->am_local ==> DO NOT use this in relay-style applications.
// Only makes sense on master busses.
// This is what the high-level query functions listed above are using.
// Transactions that time out are returned with an "ERR_TIMEOUT" status code.
int run_trans(struct modbus_c* bus, struct modbus_trans* trans);

// Use this to create an error response in the tx_resp buffer of a transaction
void local_err_response(struct modbus_trans* this_trans, int err_code);

// This can serve as a last-resort slave mode callback
// in mixed-mode slave+relay apps => the application programmer
// specifies a custom dispatch func, which in turn calls this generic
// slave callback, plus some of the standardized higher-level slave callbacks.
void generic_slave_dispatch(struct modbus_trans* this_trans);
   
   
// ### Adjustments to queuing ###

// Adjust pending queue depth (TCQ style) - please only use on TCP master busses
int adjust_q_depth(struct modbus_c* bus, int new_q_depth);

// The following timeouts are in microseconds
// Defaults for the frame_brk_timeout are the following:
//   ASCII bus: 30 seconds
//   RTU bus:    4 equivalent character transmission times (e.g., about 4 ms at 9600 bps)
//   TCP bus:  100 ms
void adjust_frame_brk_timeout(struct modbus_c* bus, u32 new_timeout);
void adjust_tx_timeout(struct modbus_c* bus, u32 new_timeout);
void adjust_pend_timeout(struct modbus_c* bus, u32 new_timeout);

// The following two inlines can be used to permit a full error handling
// capability that's optional with transaction-level apps, that use
// a dispatch_func() on master busses to obtain Modbus responses
// and may or may not be interested to know about failed transactions.
// 
// Please note that these two options do not affect the behavior of
// run_trans() and the high-level query functions depending on it -
// run_trans() does receive and report all errors by default.
static inline void enable_nonlocal_timeouts(struct modbus_c* bus)
{
   bus->return_nonlocal_timeouts = TRUE;   // for relay apps (transaction level, avoiding run_trans())
}

static inline void enable_all_trans_errs(struct modbus_c* bus)
{
   bus->return_all_trans_errs = TRUE;   // for generic transaction-level apps, interested in errors
}



// "private" functions follow:

// Debugging functions, fairly low-level. 
// You can call them explicitly, if you feel they're some use to you.
// Such as, in a simple analyzer app.
u16 decode_frame_type(u8* buffer, int buf_size, u8 req_resp_hint);
char* verbose_frame_type(u16 type);

// This function is private! Use the new_modbus_*() functions from the TCP and RTU headers.
int modbus_create(struct modbus_c* bus, u8 _m_s, u8 _link_type,
            void (*dispatch_func)(struct modbus_trans* this_trans)); // Don't call this alone!

// You can call reopen() if you believe it's due.
// It won't do anything when it's not actually due.
// This function is now called internally by the library
// - explicit use is deprecated.
int modbus_reopen(struct modbus_c* bus); // in case of failure - don't mess with the use count

// invalidate() is called automatically by send_frame() and recv_frame() upon serious errors.
// An application should call this only when it wants to force reopen() to actually do something.
int modbus_invalidate(struct modbus_c* bus); // don't mess with the use count, prepare for reopen()

// Don't call the invalidate_nolock() from an application - it's for library internal use only.
int modbus_invalidate_nolock(struct modbus_c* bus);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif // MODBUS__H
