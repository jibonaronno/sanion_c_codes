#ifndef MODBUS_RTU_H
#define MODBUS_RTU_H

// ################ //
//                  //
// Modbus/RTU for C //
//                  //
// ################ //

#include "modbus_serial.h"

#ifdef __cplusplus
extern "C" {
#endif

// dev_name  = device name, including full path
// baud      = baud rate in bps
// data_bits = integer or character, i.e. one of 8,7,6,5 or '8','7','6','5'
// parity    = character, one of 'o','O','e','E','n','N'
// stop_bits = integer or character, i.e. one of 1,2 or '1','2'
// flow_ctl  = >=1 or 0 == on or off (hardware flow control only, RTS/CTS)
// dispatch_func = a callback yielding transactions that have been received
//                  (use NULL if not interested, e.g. if using the query functions)
#define new_modbus_rtu_master(dev_name, baud, data_bits, parity, stop_bits, flow_ctl, dispatch_func) \
         new_modbus_rtu(dev_name, baud, data_bits, parity, stop_bits, flow_ctl, MASTER_BUS, dispatch_func)
#define new_modbus_rtu_slave(dev_name, baud, data_bits, parity, stop_bits, flow_ctl, dispatch_func) \
         new_modbus_rtu(dev_name, baud, data_bits, parity, stop_bits, flow_ctl, SLAVE_BUS, dispatch_func)

// Perhaps new_modbus_rtu() should not be called directly alone. Use the two macros above.
struct modbus_c* new_modbus_rtu(char* dev_name, int baud,
           char data_bits, char parity, char stop_bits, u8 flow_ctl,
           u8 _m_s, void (*dispatch_func)(struct modbus_trans* this_trans));


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* MODBUS_RTU_H */

