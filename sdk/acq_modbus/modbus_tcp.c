
// ################ //
//                  //
// Modbus/TCP for C //
//                  //
// ################ //

#include <fcntl.h>	/* File control definitions */
//#include <stdio.h>	/* Standard input/output */
#include <string.h>
#include <termio.h>	/* POSIX terminal control definitions */
#include <sys/time.h>	/* Time structures for select() */
#include <unistd.h>	/* POSIX Symbolic Constants */
#include <errno.h>	/* Error definitions */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>      // free()
#include <syslog.h>
#include <sys/stat.h>
#include <netdb.h>       // gethostbyname()

#include "modbus_tcp.h"

#define FRAME_BREAK_TIMEOUT 100000   // microseconds

#ifdef DEBUG
 char debug_scratchpad[(TCP_HEADER_SIZE + MAX_QUERY_LENGTH + RTU_CHECKSUM_SIZE) * 4 + 1];
#endif

//   send_frame()
//   Function to send a query to a modbus slave,
//   or to send a response to a modbus master (if this is a slave device)

static int send_frame( struct modbus_c* bus, u8 *tx_buf, size_t string_length )
{
 int bytes_sent;
 struct modbus_tcp_c* tcp_bus;

#ifdef DEBUG
 unsigned int i;
#endif

#ifdef DEBUG
   syslog(LOG_INFO, "TCP::send_frame(bus %p, tx_buf %p, len %d)\n", bus, tx_buf, string_length);
#endif

   // The generic Modbus code calling send_frame() has pre-allocated
   // just enough space for the Modbus/TCP header.
   tx_buf -= 6;

   tcp_bus = (struct modbus_tcp_c*) bus;
   
   // Prepend Transaction ID, Protocol ID, Length
   if (bus->m_s == MASTER_BUS)
   {
      // Ascending transaction ID's (sequence numbers)
      tx_buf[0] = (((struct modbus_tcp_c*)bus)->seq_num & 0xFF00) >> 8; // Transaction ID, msB
      tx_buf[1] =  ((struct modbus_tcp_c*)bus)->seq_num & 0x00FF;       // Transaction ID, lsB
      ((struct modbus_tcp_c*)bus)->seq_num++;                  // increment the transaction ID
   }
   // else this is a slave bus - the calling thread has set up transaction ID
   
   tx_buf[2] = 0; // Protocol ID, msB
   tx_buf[3] = 0; // Protocol ID, lsB
   tx_buf[4] = (string_length & 0x0000FF00) >> 8; // Length, msB
   tx_buf[5] =  string_length & 0x000000FF;       // Length, lsB

#ifdef DEBUG
   syslog(LOG_INFO, "TCP::send_frame(): Sending a frame to socket fd no. %d\n", bus->fd);

   /* Print to stderr the hex value of each character that is about to be */
   /* sent to the modbus slave.					       */

   for( i = 0; i < string_length + 6; i++ )
   {
      sprintf(debug_scratchpad + 4*i, "[%02X]", tx_buf[ i ] );
   }
   debug_scratchpad[4*i] = 0;
   syslog(LOG_INFO, " %s\n", debug_scratchpad );
#endif

   bytes_sent = send( bus->fd, tx_buf, string_length + 6, 0);
   //bytes_sent = send( bus->fd, tx_buf, string_length + 6, MSG_DONTWAIT); // didn't help
   //bytes_sent = write( bus->fd, tx_buf, string_length + 6);
   //tcflush(bus->fd, TCOFLUSH);

   if (bus->finished != 0)
   {
      bytes_sent = ERR_BUS_DOWN;
   }
   else if (bytes_sent >= 6)
   {
      bytes_sent -= 6;   // -= Transaction ID, Protocol ID, Length 
   }
   else if (bytes_sent > 0) // not even the header
   {
      bytes_sent = TOO_FEW_BYTES_SENT; 
   }
   else
   {
      bytes_sent = SOCKET_FAILURE; 
   }

   if (bytes_sent == SOCKET_FAILURE) modbus_invalidate(bus);

   return( bytes_sent );
}



//      recv_frame()
//      Function to wait for a reply from the modbus slave, or to wait
//      for a request if this is a slave port.
//      This function may optionally block for (timeout) microseconds if there
//      is no reply.
//      If timeout is FALSE (==0), the function will block forever.
//
//      Return value: total number of characters received.

static int recv_frame( struct modbus_c* bus, unsigned char *rx_buf, u32 timeout )
{
 int rxchar = -1;
 int data_avail = FALSE;
 int bytes_received = 0;
 int read_stat;

#ifdef DEBUG
 int i = 0;
 //int tmp_errno;
#endif

#ifdef ACCEL_TCP_FRAMING
 u16 bytes_awaited = 0;
#endif

 int max_fds = 32;
 fd_set rfds;

 struct timeval tv;
 div_t tmp_div;

   tmp_div = div(timeout, 1000000);
   tv.tv_sec = tmp_div.quot;
   tv.tv_usec = tmp_div.rem;

   // The generic Modbus code calling recv_frame() has pre-allocated
   // just enough space for the Modbus/TCP header.
   rx_buf -= 6;

   FD_ZERO( &rfds );
   FD_SET( bus->fd, &rfds );


#ifdef DEBUG
   syslog(LOG_INFO, "TCP::recv_frame(): Waiting for a frame to receive, timeout=%lu.\n", timeout);
#endif	

   /* wait for a response */
   data_avail = select( max_fds, &rfds, NULL, NULL,
                        (timeout > 0) ? &tv : NULL );

#ifdef DEBUG
   syslog(LOG_INFO, "TCP::recv_frame(): frame arriving?\n");
#endif	

   if (bus->finished != 0)
   {
      bytes_received = ERR_BUS_DOWN;
   }
   else if( !data_avail )
   {
      bytes_received = ERR_TIMEOUT; 
#ifdef DEBUG
      syslog(LOG_INFO, "Socket timeout\n" );
#endif
   }

   FD_ZERO( &rfds );
   FD_SET( bus->fd, &rfds );

   // While there's more to receive and max frame size is not exceeded
   while (data_avail && (bytes_received < MAX_RESPONSE_LENGTH + 6) && (bus->finished == 0))
   {
      /* if no character at the buffer wait frame_brk_timeout */
      /* before accepting end of response			    */
      /* Do we really need to wait, even on TCP? see ACCEL_TCP_FRAMING */
      tmp_div = div(bus->frame_brk_timeout, 1000000);
      tv.tv_sec = tmp_div.quot;
      tv.tv_usec = tmp_div.rem;


      if( select( max_fds, &rfds, NULL, NULL, &tv) > 0 )
      {
         read_stat = recv( bus->fd, &rxchar, 1, 0);
//#ifdef DEBUG
#if 0
         tmp_errno = errno;
         if ((tmp_errno != 4) || (read_stat != 1))
         {
            syslog(LOG_INFO, "TCP::recv_frame(): recv() over, read_stat=%d, errno=%d.\n", read_stat, tmp_errno);
            syslog(LOG_INFO, "TCP::recv_frame(): err: %s.\n", strerror(tmp_errno));
         }
#endif	

         if (bus->finished != 0)
         {
            bytes_received = ERR_BUS_DOWN;
         }
         else if (read_stat <= 0) // recv() actually returns 0
         {                        // if the remote party closes the socket
            bytes_received = SOCKET_FAILURE;
            data_avail = FALSE;
         }
         else if (read_stat > 0)
         {
            rxchar = rxchar & 0xFF;
            rx_buf[ bytes_received ++ ] = rxchar;

#ifdef ACCEL_TCP_FRAMING
            if (bytes_received == 6)
            {
               bytes_awaited = rx_buf[4];
               bytes_awaited <<= 8;
               bytes_awaited += rx_buf[5];
               bytes_awaited += 6;  // add headers len to payload len
            }

            if (bytes_received == bytes_awaited)
            {
               data_avail = FALSE; // superfluous?
#ifdef DEBUG
               syslog(LOG_INFO, "TCP::recv_frame(): Accelerated frame break!\n");
#endif //DEBUG
            }
#endif //ACCEL_TCP_FRAMING

            if( bytes_received > MAX_RESPONSE_LENGTH + 6)
            {
               bytes_received = RCVD_FRAME_INVALID;
               data_avail = FALSE;
#ifdef DEBUG
               syslog(LOG_INFO, "TCP::recv_frame(): bytes_received > MAX_RESP_LEN + 6 !\n");
#endif //DEBUG
            }
#ifdef DEBUG
            /* display the hex code of each character received */
            sprintf(debug_scratchpad + 4*i, "<%02X>", rxchar );
            i++;
#endif
         }
      }
      else
      {
         data_avail = FALSE;
      }
   }	

#ifdef DEBUG
   debug_scratchpad[4*i] = 0;
   syslog(LOG_INFO, "TCP::recv_frame() got a frame:\n");
   syslog(LOG_INFO, " %s\n", debug_scratchpad );
   syslog(LOG_INFO, "\n" );
#endif

   // subtract the leading TCP header, only if enough data received
   if (bytes_received >= 6)
   {
      bytes_received -= 6;   // -= Transaction ID, Protocol ID, Length 
   }
   else if (bytes_received > 0) // not even the header
   {
      bytes_received = RCVD_FRAME_INVALID; 
#ifdef DEBUG
      syslog(LOG_INFO, "TCP::recv_frame(): 0 < bytes_received < 6 !\n");
#endif //DEBUG
   }
   // else just pass the error code through

   if (bytes_received == SOCKET_FAILURE) modbus_invalidate(bus);

   return( bytes_received );
}





// === common helper funcs ===

static int dtor(struct modbus_c* bus)
{  // not a whole lot to do
   return(0);
}



static char* h_perror()
{
 char* result = NULL;

   switch (h_errno)
   {
      case HOST_NOT_FOUND: result = "Host not found"; break;
      case NO_ADDRESS: result = "Name exists, no IP address"; break;
      case NO_RECOVERY: result = "Unrecoverable name server error"; break;
      case TRY_AGAIN: result = "Temporary name server error"; break;
      default: result = "Unspecified hostname resolution error"; break;
   }

   return(result);
}
 


unsigned int host_to_ip(char* host)
{
 unsigned int result = 0;
 struct hostent* tmp_ip_stuff;

   if (host == NULL) return(-1);  // check that the input is not outright invalid
      
   tmp_ip_stuff = gethostbyname(host);
   if (tmp_ip_stuff == NULL)
   {
      //syslog(LOG_ERR, "TCP::host_to_ip(): gethostbyname() failed with code: %s\n", h_perror());
      return(-1); // check that a valid value was returned
   }

   memcpy( (void *) &result, (void *) tmp_ip_stuff->h_addr_list[0], 4);
   
   return(result);
}







// === master init funcs ===

static int set_up_tcp_master( struct modbus_tcp_c* bus )
{
 int sfd;
 struct sockaddr_in server;
 int connect_stat;

   sfd = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP );

   server.sin_family = AF_INET;
   server.sin_port = htons( bus->tcp_port );
   server.sin_addr.s_addr = bus->ip_addr;

   //flags = fcntl(sfd, F_GETFL, 0);   // setting up the TCP socket as nonblocking doesn't prevent the queuing paradox.
   //flags |= O_NONBLOCK;
   //fcntl(sfd, F_SETFL, flags);

   if( sfd >= 0 )
   {
      connect_stat = connect( sfd, (struct sockaddr *)&server,
                              sizeof(struct sockaddr_in) );

      if( connect_stat < 0 )
      {
         close( sfd );
         sfd = -1;
      }
      else
      {
         bus->bus.fd = sfd;
#ifdef DEBUG
         syslog( LOG_INFO, "Socket connected, bus->fd = %d\n", bus->bus.fd);
#endif
      }
   }

   return( sfd );
}



static int init_modbus_master(struct modbus_c* bus)
{
 int retval = 0;
 u8 init_necessary = FALSE;
 struct stat tmp_stat;
 struct modbus_tcp_c* tcp_bus;
 
   tcp_bus = (struct modbus_tcp_c*) bus; // just a typecast

   // Check some signs that the socket could possibly be closed.
   // If there's nothing wrong with it, just leave it open.
   if (bus->fd == -1) init_necessary = TRUE;
   else
   {
      if (fstat(bus->fd, &tmp_stat) != 0)
      {
         syslog(LOG_ERR,"init_modbus_master(): fstat(fd) failed, TCP socket not open? Error msg %m\n");
         close(bus->fd);
         init_necessary = TRUE;
         bus->fd = -1;
      }
   }
   
   while (init_necessary == TRUE)
   {
      // Try to connect() a TCP session to the target.
      // If connect() fails, sleep and retry; else connect() successful.
      if (set_up_tcp_master(tcp_bus) < 0)
      {
         if (bus->finished != 0 || bus->dont_retry != 0)
         {
            init_necessary = FALSE;
            retval = 1;
         }
         else
         {
            syslog(LOG_ERR,"Can't open TCP session to %lu.%lu.%lu.%lu port %u.\n"
                           " Sleeping 10 seconds before retry...\n",
                            tcp_bus->ip_addr & 0x000000FF,
                           (tcp_bus->ip_addr & 0x0000FF00) >> 8,
                           (tcp_bus->ip_addr & 0x00FF0000) >> 16,
                           (tcp_bus->ip_addr & 0xFF000000) >> 24,
                            tcp_bus->tcp_port );
            sleep(10);
            retval = 1;
         }
      }
      else
      {
         init_necessary = FALSE;
         retval = 0;
      }
   }
     
   return(retval);
}



static int inval_modbus_master(struct modbus_c* bus)
{
 int retval = 0;
 
   if (bus->fd != -1) retval = close(bus->fd);
   bus->fd = -1;

   return(retval);
}

struct modbus_c* new_modbus_tcp_master_hostname(char* host, unsigned short int tcp_port,
                     int _q_depth, void (*dispatch_func)(struct modbus_trans* this_trans))
{
 unsigned int ip_addr = 0;
 struct modbus_c* retval = NULL;

   ip_addr = host_to_ip(host);
   if (ip_addr != -1) retval = new_modbus_tcp_master_ipaddr(ip_addr, tcp_port, _q_depth, dispatch_func);

   return(retval);
}



struct modbus_c* new_modbus_tcp_master_ipaddr(unsigned int ip_addr, unsigned short int tcp_port,
                           int _q_depth, void (*dispatch_func)(struct modbus_trans* this_trans))
{
 struct modbus_tcp_c* new_bus = NULL;
   
   new_bus = (struct modbus_tcp_c*) malloc(sizeof(struct modbus_tcp_c));
   if (new_bus != NULL)
   {
      if (modbus_create(&new_bus->bus, MASTER_BUS, TCP_BUS, dispatch_func) != 0)
      {
         free(new_bus);
         new_bus = NULL;
      }
      // else OK, the modbus common guts are initialized
   }

   if (new_bus != NULL)
   {
      if (_q_depth == 0) adjust_q_depth(&new_bus->bus, DEFAULT_TCP_Q_DEPTH);
      else adjust_q_depth(&new_bus->bus, _q_depth);

      new_bus->bus.init_modbus = init_modbus_master;
      new_bus->bus.inval_modbus = inval_modbus_master;
      new_bus->bus.send_frame = send_frame;
      new_bus->bus.recv_frame = recv_frame;
      new_bus->bus.dtor = dtor;

      new_bus->ip_addr = ip_addr;
      new_bus->tcp_port = tcp_port;

      new_bus->mother = NULL;
      new_bus->seq_num = 0;
#ifdef DEBUG
      syslog( LOG_INFO, "Using a default frame break timeout of %d microseconds.\n", FRAME_BREAK_TIMEOUT);
#endif
      new_bus->bus.frame_brk_timeout = FRAME_BREAK_TIMEOUT;
   }

   return((struct modbus_c*) new_bus);
}






// === listener init funcs ===

static int tcp_server_dtor(struct modbus_c* bus)
{
 //struct modbus_tcp_c* tcp_bus;

   return(0);
}



static int set_up_tcp_server( struct modbus_tcp_c* srv )
{
 int sfd = -1, retval = 0;
 int on = 1; // a dummy flags variable for setsockopt(SO_REUSEADDR)
 struct sockaddr_in server;

   sfd = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP );

   server.sin_family = AF_INET;
   server.sin_port = htons( srv->tcp_port );
   server.sin_addr.s_addr = srv->ip_addr;

   // On the next two lines, the SO_REUSEADDR is really important.
   // This is to avoid the "can't bind socket: address already in use" error
   // when the server is re-run immediately after it has exit()ted.
   setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));

   if( sfd >= 0 )
   {  // socket() OK
#ifdef DEBUG
      syslog(LOG_INFO, "set_up_tcp_server(): socket() OK.\n");
#endif	
      if ( bind(sfd, (struct sockaddr *) &server, sizeof(struct sockaddr_in)) == 0)
      {  // bind() OK
#ifdef DEBUG
         syslog(LOG_INFO, "set_up_tcp_server(): bind() OK.\n");
#endif	
         if (listen(sfd, /*queue length*/5) == 0)
         { // listen() OK
#ifdef DEBUG
            syslog(LOG_INFO, "set_up_tcp_server(): listen() OK.\n");
#endif	
            srv->bus.fd = sfd; // we can start accept()ing clients = masters
         }
         else // listen() failed
         {
            close( sfd );
            retval = -1;
            syslog(LOG_ERR, "set_up_tcp_server(): listen() FAILED. Err msg: %m\n");
         }
      }
      else // bind() failed
      {
         close( sfd );
         retval = -1;
         syslog(LOG_ERR, "set_up_tcp_server(): bind() FAILED. Err msg: %m\n");
      }
   }
   else // socket() failed
   {
      retval = -1;
      syslog(LOG_ERR, "set_up_tcp_server(): socket() FAILED. Err msg: %m\n");
   }
  
   return(retval);
}



static int init_modbus_tcp_server(struct modbus_c* bus)
{
 int retval = 0;
 u8 init_necessary = FALSE;
 struct stat tmp_stat;
 struct modbus_tcp_c* tcp_bus;
 
   tcp_bus = (struct modbus_tcp_c*) bus; // just a typecast

   // Check some signs that the socket could possibly be closed.
   // If there's nothing wrong with it, just leave it open.
   if (bus->fd == -1) init_necessary = TRUE;
   else
   {
      if (fstat(bus->fd, &tmp_stat) != 0)
      {
         syslog(LOG_ERR,"init_modbus_tcp_server(): fstat(fd) failed, listener socket not open? Error msg %m\n");
         close(bus->fd);
         init_necessary = TRUE;
         bus->fd = -1;
      }
   }
   
   while (init_necessary == TRUE)
   {
      // Try to open a listening socket.
      // If that fails, sleep and retry; else socket entered the listening state successfully.
      if (set_up_tcp_server(tcp_bus) < 0)
      {
         if (bus->finished != 0 || bus->dont_retry != 0)
         {
            init_necessary = FALSE;
            retval = 1;
         }
         else
         {
            syslog(LOG_ERR,"Can't open a listening socket at %lu.%lu.%lu.%lu port %u.\n"
                           " Sleeping 10 seconds before retry...\n",
                            tcp_bus->ip_addr & 0x000000FF,
                           (tcp_bus->ip_addr & 0x0000FF00) >> 8,
                           (tcp_bus->ip_addr & 0x00FF0000) >> 16,
                           (tcp_bus->ip_addr & 0xFF000000) >> 24,
                            tcp_bus->tcp_port );
            sleep(10);
            retval = 1;
         }
      }
      else
      {
         init_necessary = FALSE;
         retval = 0;
      }
   }
     
   return(retval);
}



static int inval_modbus_tcp_server(struct modbus_c* bus)
{
 int retval = 0;
 struct modbus_tcp_c* tcp_bus;
 struct list_head* tmp_child = 0;

   tcp_bus = (struct modbus_tcp_c*) bus;

   if (bus->fd != -1) retval = close(bus->fd);
   bus->fd = -1;

   // kill all the children
   while (! list_empty(&tcp_bus->children))
   {
      tmp_child = tcp_bus->children.next;
      modbus_close(&((struct modbus_tcp_child*)tmp_child)->bus.bus);
      // the modbus_close() should've shut all the threads
      // and should've self-released the modbus_tcp_child
   }

   return(retval);
}



static void add_child(struct modbus_tcp_c* bus, struct modbus_tcp_child* child)
{
   // we'll be parasiting a bit on the pend_q.mutex
   pthread_mutex_lock(&bus->bus.pend_q.mutex);
   list_add_tail((struct list_head*)child, &bus->children);
   pthread_mutex_unlock(&bus->bus.pend_q.mutex);
}



static void del_child(struct modbus_tcp_c* server_bus, struct modbus_tcp_child* child)
{
   // we'll be parasiting a bit on the pend_q.mutex
   pthread_mutex_lock(&server_bus->bus.pend_q.mutex);
   list_del((struct list_head*)child);
   pthread_mutex_unlock(&server_bus->bus.pend_q.mutex);
}




// a forward declaration...
static struct modbus_tcp_child* new_modbus_tcp_child(struct modbus_tcp_c* _mother, int accepted_client_socket);

static void* accept_thr(void* owner_bus)
{
 struct modbus_c* bus = NULL;
 struct modbus_tcp_child* tmp_bus = NULL;
 long retval = 0;
 int accepted_client_socket;
 socklen_t sockaddr_in_len;
 struct sockaddr_in accepted_client_address;

   sockaddr_in_len = sizeof(struct sockaddr_in);
   bus = (struct modbus_c*) owner_bus;

#ifdef DEBUG
   syslog(LOG_INFO,"accept_thr(): starting\n");
#endif // DEBUG

   if (bus->fd == -1) modbus_reopen(bus);    // if not actually open by modbus_open(), do it silently

   while (bus->finished == 0)
   { 
      accepted_client_socket = accept(
                                 bus->fd,
                                 (struct sockaddr *) &accepted_client_address,
                                 &sockaddr_in_len
                               );

      if (bus->finished != 0)
      {
#ifdef DEBUG
         syslog(LOG_ERR,"accept_thr(): TCP slave bus shutting down, going to pthread_exit().\n");
#endif // DEBUG
         continue;
      }
      else if ( accepted_client_socket == -1 )
      {
#ifdef DEBUG
         syslog(LOG_ERR,"accept_thr(): serious TCP listening socket error while trying to accept() a slave.\n");
#endif // DEBUG
         modbus_invalidate(bus);
         modbus_reopen(bus);             // try to reopen the bus
         continue;
      }
      else // OK - what we've got is a valid socket
      {
#ifdef DEBUG
         syslog(LOG_INFO,"accept_thr(): received a client session - going to spawn a child bus.\n");
#endif // DEBUG
         tmp_bus = new_modbus_tcp_child(owner_bus, accepted_client_socket);
         if (tmp_bus != NULL)
         {
            add_child(owner_bus, tmp_bus);
            modbus_open(&tmp_bus->bus.bus,1,0); // & modbus_tcp_child->modbus_tcp_c.modbus_c
            // Yes, the listener pseudo-bus will open() the child busses (run their threads).
            // Correspondingly, a child bus must close() itself when its respective TCP session closes.
         }
         else
         {
            close(accepted_client_socket);
#ifdef DEBUG
            syslog(LOG_ERR,"accept_thr(): failed to spawn a child.\n");
#endif // DEBUG
         }
      }
   }

   bus->rx_ID = -1;
   pthread_exit((void*)retval);
}



struct modbus_c* new_modbus_tcp_slave_hostname(char* host, unsigned short int tcp_port,
                                   void (*dispatch_func)(struct modbus_trans* this_trans))
{
 unsigned int ip_addr = 0;
 struct modbus_c* retval = NULL;

   if (strncasecmp(host, "any", 3) == 0) // if keyword "any" is given for hostname
   {
      if (host[3] == 0)
         ip_addr = 0;
      else
         ip_addr = host_to_ip(host);
   }
   else ip_addr = host_to_ip(host);

   retval = new_modbus_tcp_slave_ipaddr(ip_addr, tcp_port, dispatch_func);

   return(retval);
}



struct modbus_c* new_modbus_tcp_slave_ipaddr(unsigned int ip_addr, unsigned short int tcp_port,
                                   void (*dispatch_func)(struct modbus_trans* this_trans))
{
 struct modbus_tcp_c* new_bus = NULL;
   
   new_bus = (struct modbus_tcp_c*) malloc(sizeof(struct modbus_tcp_c));

   if (new_bus != NULL)
   {
      if (modbus_create(&new_bus->bus, LISTENER_BUS, TCP_BUS, dispatch_func) != 0)
      {
         free(new_bus);
         new_bus = NULL;
      }
      // else OK, the modbus common guts are initialized
   }

   if (new_bus != NULL)
   {
      // mod the default set of Modbus thread funcs
      new_bus->bus.rx_thr = accept_thr;

      new_bus->bus.init_modbus = init_modbus_tcp_server;
      new_bus->bus.inval_modbus = inval_modbus_tcp_server;
      new_bus->bus.send_frame = NULL;
      new_bus->bus.recv_frame = NULL;
      new_bus->bus.dtor = tcp_server_dtor;

      new_bus->ip_addr = ip_addr;
      new_bus->tcp_port = tcp_port;
      new_bus->bus.rx_ID = -1;
      new_bus->bus.fd = -1;
      INIT_LIST_HEAD(&new_bus->children);
#ifdef DEBUG
      syslog( LOG_INFO, "Using a default frame break timeout of %d microseconds.\n", FRAME_BREAK_TIMEOUT);
#endif
      new_bus->bus.frame_brk_timeout = FRAME_BREAK_TIMEOUT;
   }

   return((struct modbus_c*) new_bus);
}




// === slave (listener child) init funcs ===

static void free_modbus_tcp_child(struct modbus_tcp_child* child); // forward decl.

static int tcp_child_dtor(struct modbus_c* bus)
{
 struct modbus_tcp_c* tcp_bus;

   tcp_bus = (struct modbus_tcp_c*) bus;
 
   del_child(tcp_bus->mother, (struct modbus_tcp_child*) tcp_bus->child_head);
   free_modbus_tcp_child((struct modbus_tcp_child*) tcp_bus->child_head);

   return(0);
}



static int init_modbus_tcp_child(struct modbus_c* bus) // this function is not much use
{
 int retval = 0;
 u8 init_necessary = FALSE;
 struct stat tmp_stat;
 struct modbus_tcp_c* tcp_bus;
 
   tcp_bus = (struct modbus_tcp_c*) bus; // just a typecast

   // Check some signs that the socket could possibly be closed.
   // If there's nothing wrong with it, just leave it open.
   if (bus->fd == -1) init_necessary = TRUE;
   else
   {
      if (fstat(bus->fd, &tmp_stat) != 0)
      {
         syslog(LOG_ERR,"init_modbus_tcp_child(): fstat(fd) failed, TCP socket not open? Error msg %m\n");
         close(bus->fd);
         init_necessary = TRUE;
         bus->fd = -1;
      }
   }
   
   if (init_necessary == TRUE)
      retval = -1;
     
   return(retval);
}



static int inval_modbus_tcp_child(struct modbus_c* bus)
{
 int retval = 0;
 
   if (bus->fd != -1) retval = close(bus->fd);
   bus->fd = -1;

   bus->finished = 1; // a TCP slave bus cannot be reopened

   return(retval);
}



// defined in modbus.c
extern struct list_head child_alloc_pool;
extern pthread_mutex_t child_alloc_mutex;
extern int alloc_pools_inited;


static struct modbus_tcp_child* get_modbus_tcp_child()
{
 struct modbus_tcp_child* retval = NULL;

   if (alloc_pools_inited == 0)
   {
      syslog(LOG_ERR, "Modbus library error: the programmer hasn't called modbus_init() on startup!!!\n");
      exit(-2);
   }

   pthread_mutex_lock(&child_alloc_mutex);
   
   if (list_empty(&child_alloc_pool))
   {
      retval = (struct modbus_tcp_child*) malloc(sizeof(struct modbus_tcp_child));
      retval->bus.child_head = &retval->hook;
   }
   else
   {
      retval = (struct modbus_tcp_child*) child_alloc_pool.next;
      list_del(child_alloc_pool.next); 
   }

   pthread_mutex_unlock(&child_alloc_mutex);
      

   return(retval);
}



static void free_modbus_tcp_child(struct modbus_tcp_child* child)
{
   if (child != NULL)
   {
      pthread_mutex_lock(&child_alloc_mutex);
      list_add_tail((struct list_head*) child, &child_alloc_pool);
      pthread_mutex_unlock(&child_alloc_mutex);
      //free(child); // not anymore
   }
   else
   {
      syslog(LOG_ERR,"internal error: attempted to free() a NULL modbus_tcp_child pointer!\n");
   }

   return;
}



static struct modbus_tcp_child* new_modbus_tcp_child(struct modbus_tcp_c* _mother, int accepted_client_socket)
{
 struct modbus_tcp_child* new_bus = NULL;
   
   new_bus = get_modbus_tcp_child();

   if (new_bus != NULL)
   {
      if (modbus_create(&new_bus->bus.bus, SLAVE_BUS, TCP_BUS, _mother->bus.dispatch_func) != 0)
      {
         free(new_bus);
         new_bus = NULL;
      }
      // else OK, the modbus common guts are initialized
   }

   if (new_bus != NULL)
   {
      new_bus->bus.bus.init_modbus = init_modbus_tcp_child;
      new_bus->bus.bus.inval_modbus = inval_modbus_tcp_child;
      new_bus->bus.bus.send_frame = send_frame;
      new_bus->bus.bus.recv_frame = recv_frame;
      new_bus->bus.bus.dtor = tcp_child_dtor;

      new_bus->bus.bus.fd = accepted_client_socket;
      new_bus->bus.mother = _mother;
      new_bus->bus.ip_addr = _mother->ip_addr;
      new_bus->bus.tcp_port = _mother->tcp_port;

      new_bus->bus.bus.our_id                          = _mother->bus.our_id;
      new_bus->bus.bus.read_coil_status_cback          = _mother->bus.read_coil_status_cback;
      new_bus->bus.bus.read_discrete_input_cback       = _mother->bus.read_discrete_input_cback;
      new_bus->bus.bus.read_holding_registers_cback    = _mother->bus.read_holding_registers_cback;
      new_bus->bus.bus.read_input_registers_cback      = _mother->bus.read_input_registers_cback;
      new_bus->bus.bus.force_single_coil_cback         = _mother->bus.force_single_coil_cback;
      new_bus->bus.bus.preset_single_register_cback    = _mother->bus.preset_single_register_cback;
      new_bus->bus.bus.set_multiple_coils_cback        = _mother->bus.set_multiple_coils_cback;
      new_bus->bus.bus.preset_multiple_registers_cback = _mother->bus.preset_multiple_registers_cback;
#ifdef DEBUG
      syslog( LOG_INFO, "Using an inherited timeouts (in us):\n");
      syslog( LOG_INFO, "\n frame_t: %u, tx_t: %u, pend_t: %u\n",
                         (unsigned int) new_bus->bus.mother->bus.frame_brk_timeout,
                         (unsigned int) new_bus->bus.mother->bus.tx_timeout,
                         (unsigned int) new_bus->bus.mother->bus.pend_timeout);
#endif
      new_bus->bus.bus.frame_brk_timeout = new_bus->bus.mother->bus.frame_brk_timeout;
      new_bus->bus.bus.tx_timeout = new_bus->bus.mother->bus.tx_timeout;
      new_bus->bus.bus.pend_timeout = new_bus->bus.mother->bus.pend_timeout;
   }

   return(new_bus);
}



