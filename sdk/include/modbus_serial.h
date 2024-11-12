#ifndef MODBUS_SERIAL_H
#define MODBUS_SERIAL_H

// ################# //
//                   //
//   Common serial   //
//  Modbus routines  //
// for RTU and ASCII //
//                   //
//  !internal only!  //
//                   //
// ################# //

#if ((! defined MODBUS_RTU_H) && (! defined MODBUS_ASC_H) && (! defined MODBUS_OWN))
#error "Don't #include <modbus_serial.h> directly!"
#error "Include either modbus_rtu.h or modbus_asc.h (or both)!"
#error "This file (modbus_serial.h) contains some common"
#error "that can't be used alone (maybe except mogrify_*())."
#endif // direct inclusion check

#include "modbus.h"

#ifdef __cplusplus
extern "C" {
#endif

struct modbus_serial
{
   struct modbus_c bus;

   char* dev_name;
   unsigned int baud;
   char data_bits;
   char parity;
   char stop_bits;
   char flow_ctl; // 0 = none, 1 = hardware (RTS/CTS)
};

#define MB_FLOW 1
#define MB_NOFLOW 0

struct modbus_serial* new_modbus_serial(char* dev_name, int baud,
           char data_bits, char parity, char stop_bits, u8 flow_ctl,
           u8 _m_s, u8 _link_type, void (*dispatch_func)(struct modbus_trans* this_trans));

// Just a couple helpers. Their use isn't mandatory,
// but they can be used to verify arguments ahead of
// passing them to new_modbus_rtu()/new_modbus_asc().
// Note that new_modbus_rtu()/new_modbus_asc() do use
// these helpers internally.
int mogrify_baud(int baud);
int mogrify_bits(char cs);
int mogrify_par(char parity);
int mogrify_stopb(char stopbits);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif //MODBUS_SERIAL_H
