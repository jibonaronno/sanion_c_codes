
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>
#include <dirent.h>
#include <time.h>
#include <limits.h>
//#include <curses.h>
#include <dirent.h>

#include "Acq.h"

int connect_tcp(unsigned char *ipaddr, int iPort)
{
    int sockfd, connfd, iResult;

    struct sockaddr_in servaddr, cli;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) 
    {
        printf("sensor socket creation failed...\n");
        exit(0);
    }
    else
        printf("sensor Socket successfully created.. \n");

    return 0;
}

/* We are only use READ_HOLDING_REGISTERS function of modbus which is 0X03.
    It looks like READ_HOLDING_REGISTERS and READ_INPUT_REGISTERS are similar.

 */

int modbus_reopen(struct modbus_c* bus) // don't mess with the use count
{
 int retval;

   pthread_mutex_lock(&bus->bus_init_mutex);
   if (bus->finished == 0) // not shutting down yet
   {
      retval = bus->init_modbus(bus); // so that init_modbus() is guaranteed to be free of races
   }
   else retval = -1;
   pthread_mutex_unlock(&bus->bus_init_mutex);

   return(retval);
}

char* verbose_frame_type(unsigned short type)
{
 char* result = NULL;

   switch (type)
   {
      case 0x0100 : result = "Read coils' status request"; break;
      case 0x0101 : result = "Read coils' status response"; break;
      case 0x8101 : result = "Read coils' status error response"; break;
      
      case 0x0200 : result = "Read discrete inputs request"; break;
      case 0x0201 : result = "Read discrete inputs response"; break;
      case 0x8201 : result = "Read discrete inputs error response"; break;
      
      case 0x0300 : result = "Read holding registers request"; break;
      case 0x0301 : result = "Read holding registers response"; break;
      case 0x8301 : result = "Read holding registers error response"; break;
      
      case 0x0400 : result = "Read input registers request"; break;
      case 0x0401 : result = "Read input registers response"; break;
      case 0x8401 : result = "Read input registers error response"; break;
      
      case 0x0500 : result = "Force single coil request (response has identical length)"; break;
      case 0x0501 : result = "Force single coil response"; break;
      case 0x8501 : result = "Force single coil error response"; break;
      
      case 0x0600 : result = "Write single register request (response has identical length)"; break;
      case 0x0601 : result = "Write single register response"; break;
      case 0x8601 : result = "Write single register error response"; break;
      
      case 0x0F00 : result = "Write multiple coils request"; break;
      case 0x0F01 : result = "Write multiple coils response"; break;
      case 0x8F01 : result = "Write multiple coils error response"; break;
      
      case 0x1000 : result = "Write multiple registers request"; break;
      case 0x1001 : result = "Write multiple registers response"; break;
      case 0x9001 : result = "Write multiple registers error response"; break;

      case 0xFFFF : result = "Error - malformed frame"; break;
      default:      result = "Unrecognized other function (not an error)"; break;
   }

   return(result);
}



char* modbus_error(int err_code)
{
   switch (err_code)
   {
      case ERR_OK: return("internal: OK"); break;

      case ILLEGAL_FUNCTION:      return("modbus err response: Illegal or unsupported function"); break;
      case ILLEGAL_DATA_ADDRESS:  return("modbus err response: Illegal data address"); break;
      case ILLEGAL_DATA_VALUE:    return("modbus err response: Illegal data value"); break;
      case SLAVE_DEVICE_FAILURE:  return("modbus err response: Slave device failure"); break;
      case ACKNOWLEDGE:           return("modbus err response: Acknowledge"); break;
      case SLAVE_DEVICE_BUSY:     return("modbus err response: Slave device busy"); break;
      case NEGATIVE_ACKNOWLEDGE:  return("modbus err response: Negative acknowledge"); break;
      case MEMORY_PARITY_ERROR:   return("modbus err response: Memory parity error"); break;
      case GW_PATH_UNAVAILABLE:   return("modbus err response: Gateway says: path unavailable (unconfigured?)"); break;
      case GW_TRGT_NO_RESPONSE:   return("modbus err response: Gateway says: no response from target"); break;

      case RCVD_FRAME_INVALID:    return("protocol: Rcvd frame invalid"); break;
      case QRY_RESP_LEN_MISMATCH: return("protocol: Qry response length mismatch"); break;
      case TOO_FEW_BYTES_SENT:    return("protocol: Too few bytes sent"); break;
      case CRC_ERROR:             return("protocol: CRC error"); break;
      case ERR_TIMEOUT:           return("queuing: transaction timed out"); break;
      case ERR_BUS_DOWN:          return("bus/queuing: the bus is shut down"); break;
      case INTERNAL_ERR:          return("internal: internal error (libmodbus API misuse)"); break;
      case UPPER_FRAMING_ERROR:   return("slave processing: framing error (malformed request)"); break;
      case RESP_WHERE_REQ_XPCTD:  return("slave processing: got resp, req expected (internal status code)"); break;
      case PORT_FAILURE:          return("bus: serial port failure"); break;
      case SOCKET_FAILURE:        return("bus: TCP socket failure"); break;

      default:                    return("unknown error code"); break;
   }
}



// receive a frame as a slave, queue it in the RX_Q, tell the dispatcher
static void* rx_thr_master(void* owner_bus)
{
 struct modbus_c* bus;
 struct modbus_trans* tmp_trans = NULL;
 long retval = 0;
 int recv_retval = 0;
 struct list_head* tmp_head, *tmp_next;

 // We can't pick the right transaction from the pending queue before we know
 // the Modbus/TCP tag. Thus, we can't use a particular transaction's
 // rx_resp buffer for the raw recv_frame() - a chicken and egg kinda problem.
 // Therefore, we need another buffer for the receive operation.
 unsigned char response_base[ TCP_HEADER_SIZE + MAX_RESPONSE_LENGTH + RTU_CHECKSUM_SIZE ];
 unsigned char* response = response_base; // gcc refuses to add (int) to the local array base ptr...

   // We're not really interested in the TCP headers, we only care about
   // the Modbus payload. Thus we don't need the actual buffer's base pointer.
   // The buffer is local (allocated on stack), so deallocation is seamless.
   response += TCP_HEADER_SIZE;

   bus = (struct modbus_c*) owner_bus;
   //modbus_thr_signals_init();

#ifdef DEBUG
   syslog(LOG_INFO,"rx_thr_master(): starting\n");
#endif // DEBUG

   if (bus->fd == -1) modbus_reopen(bus);    // if not actually open by modbus_open(), do it silently

   while (bus->finished == 0)
   {
      // wait for a response frame
      recv_retval = bus->recv_frame(bus, response, /*time out?*/ 0);
      if (bus->finished != 0)
      {
        sleep(0.1);
        continue;
      }
      else if (recv_retval <= REOPEN_DUE) // serious error, bus needs to be reopened
      {
        #ifdef DEBUG
        syslog(LOG_INFO,"rx_thr_master(): serious bus error while trying to receive a response.\n");
        #endif // DEBUG
         // no pending transaction could be inferred - if there is one, it will time out on its own
         clear_pend_q(bus,recv_retval);
         modbus_reopen(bus);             // try to reopen the bus
         continue;
      }
      else if (recv_retval <= 0)       // minor error, framing or some such
      {
#ifdef DEBUG
         syslog(LOG_INFO,"rx_thr_master(): minor error while trying to receive a response.\n");
#endif // DEBUG
         // No pending transaction could be inferred - if there is one, it will time out on its own.
         // BTW, should we cancel the only pending transaction if the bus is untagged ?
         /*
         if (bus->tcq_depth == 1)
            clear_pend_q(bus,recv_retval);
         */
         continue;
      }
      else // OK
      {
#ifdef DEBUG
         syslog(LOG_INFO,"rx_thr_master(): received a response - received just fine.\n");
#endif // DEBUG

         // While we search the pend_q or work with the single transaction on untagged busses,
         // we need to have the pend_q locked.
         // Also, to prevent deadlocks with the TCQ counter mutex,
         // we need to take the TCQ counter mutex first == now.
         pthread_mutex_lock(&bus->tcq_mutex);
         pthread_mutex_lock(&bus->pend_q.mutex);

         tmp_trans = NULL; // we're in a loop, need to re-initalize this here

#ifdef DEBUG
         syslog(LOG_INFO,"rx_thr_master(): checking TCQ. Bus %p: tcq_depth = %d, tcq_pending = %d\n",
                          bus, bus->tcq_depth, bus->tcq_pending);
#endif
         // If the tcq_depth is reset to 1 (untagged) at runtime from a previously higher
         // queue depth, still try to process any remaining pending tagged transactions
         if ((bus->tcq_depth != 1) || (bus->tcq_pending > 1))
         {
#ifdef DEBUG
	    syslog(LOG_INFO,"rx_thr_master(): will have to find our transaction in the pend_q.\n");
#endif // DEBUG
            // find transaction in the pending queue
            list_for_each_safe(tmp_head, tmp_next, &bus->pend_q.q)
            {
               tmp_trans = (struct modbus_trans*) tmp_head;
#ifdef DEBUG
	       syslog(LOG_INFO,"rx_thr_master(): checking transaction %p in the pend_q.\n", tmp_trans);
#endif // DEBUG
               if ( // if the unsigned short transaction ID matches 
                     (response_base[0] == tmp_trans->tx_req.buf[0])
                  && (response_base[1] == tmp_trans->tx_req.buf[1])
                  )
                  break; // break the list_for_each() loop
               else tmp_trans = NULL; // used to signal "no such transaction found"
            }
         }
         else // tcq_depth == 1 && tcq_pending <= 1
         {
            if (bus->tcq_pending == 1) // if there is a pending transaction, it's this one transaction
            {
               tmp_trans = (struct modbus_trans*) bus->pend_q.q.next;
            }
         }

         if (tmp_trans != NULL)
         {
            // delete tmp_trans from the pending queue
            *(tmp_trans->pending_latch) = FALSE;
            tmp_trans->pending_latch = NULL;
            list_del((struct list_head*)tmp_trans); // just remove tmp_trans from the queue - won't deallocate tmp_trans
            pthread_mutex_unlock(&bus->pend_q.mutex);
            bus->tcq_pending--;
            pthread_cond_signal(&bus->tcq_cond);
            pthread_mutex_unlock(&bus->tcq_mutex);
            
            // copy the response into the transaction struct
            memcpy(tmp_trans->rx_resp.buf, response_base,
                    recv_retval + TCP_HEADER_SIZE + RTU_CHECKSUM_SIZE);
            tmp_trans->rx_resp.len = recv_retval;

            // queue transaction
            _q_trans(&bus->rx_q, tmp_trans);
         }
         else
         {
            pthread_mutex_unlock(&bus->pend_q.mutex);
            pthread_mutex_unlock(&bus->tcq_mutex); // don't keep the TCQ waiting until syslog() over
#ifdef DEBUG
            syslog(LOG_INFO,"rx_thr_master(): this response doesn't match any pending request!\n");
#endif // DEBUG
         }
      }
   }

#ifdef DEBUG
         syslog(LOG_INFO,"rx_thr_master(): bus shutting down, going to pthread_exit().\n");
#endif // DEBUG

   bus->rx_ID = -1;
   pthread_exit((void*)retval);
}

