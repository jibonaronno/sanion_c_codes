#ifndef MODBUS_TCP_H
#define MODBUS_TCP_H

// ################ //
//                  //
// Modbus/TCP for C //
//                  //
// ################ //


#include "modbus.h"

#ifdef __cplusplus
extern "C" {
#endif


#define ACCEL_TCP_FRAMING
#define DEFAULT_TCP_Q_DEPTH 30

// To allow for some arcane polymorphism, I have made a design decision that
// the slave listener (doing just accept() and pthread_create()) should
// be a struct modbus_c work-alike in some operational aspects, and a slave
// modbus_c template in other aspects.

struct modbus_tcp_c
{
// public:
   struct modbus_c bus;
   unsigned long int ip_addr;   // in network order
   unsigned short int tcp_port; // in host order

// private:
   // master
   u16 seq_num;  // used to tag the Modbus/TCP transactions in "tagged queuing" mode (master only)
   // slave
   struct modbus_tcp_c* mother; // the slave bus template, a pseudo-bus (slave only)
   struct list_head* child_head;
   // slave listener / server - uses bus->rx_ID as the listener thread ID
   struct list_head children;
};



struct modbus_tcp_child
{
   struct list_head hook;
   struct modbus_tcp_c bus;
};

 

// host  == DNS hostname or IP address in dotted decimal notation
// ip_addr  == IP address: unsigned 32bit integer, network order (BSD sockets' endian)
// tcp_port  == tcp port to connect to, host order (local machine's endian)
// q_depth  == Modbus/TCP TCQ depth; set to zero to use the default value (30)
// dispatch_func == a callback yielding transactions that have been received
//                  (use NULL if not interested, e.g. if using the query functions)
struct modbus_c* new_modbus_tcp_master_hostname(char* host, unsigned short int tcp_port,
                      int _q_depth, void (*dispatch_func)(struct modbus_trans* this_trans));
struct modbus_c* new_modbus_tcp_master_ipaddr(unsigned int ip_addr, unsigned short int tcp_port,
                      int _q_depth, void (*dispatch_func)(struct modbus_trans* this_trans));

struct modbus_c* new_modbus_tcp_slave_hostname(char* host, unsigned short int tcp_port,
                                   void (*dispatch_func)(struct modbus_trans* this_trans));
struct modbus_c* new_modbus_tcp_slave_ipaddr(unsigned int ip_addr, unsigned short int tcp_port,
                                   void (*dispatch_func)(struct modbus_trans* this_trans));



// use this if you want to check if the IP exists or not...
unsigned int host_to_ip(char* host);


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* MODBUS_TCP_H */

