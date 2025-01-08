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
#include <arpa/inet.h>
#include <unistd.h>

#include "VT100.h"
#include "sys/types.h"
#include "sys/sysinfo.h"
//#include "spectrum_packet.h"

#include "UR_SDK.h"
#include "main.h"
#include "IOManager.h"
#include "PowerSystem.h"		// added by symoon
#include "tcpipprotocol.h"
#include "PDMonitoring.h"		// 2021. 12. 15. added by symoon
#include "GISMonitoring.h"		// 2021. 12. 15. added by symoon
#include "MTrMonitoring.h"		// added by symoon

/*<USER-CODE-START>*/
//#include "modbus.h"
#include "Acq.h"
#include "pddau.h"
#include "oltc.h"
#include "cJSON.h"
char json_str[500];
//cJSON *json = cJSON_Parse(json_str);

#include "cidcsvmanager.h"
#include "bushing.h"

/**
 * @file This file contains most of the instructions and operations performed by various functions 
 * and threads across the project. 
 */

namebufferstruct PDfilenamebuffs[4];
U8 totdatfile1[PDdatMax];
U8 totdatfile2[PDdatMax];
U8 totdatfile3[PDdatMax];
U8 totdatfile4[PDdatMax];
bool datflag;
bool rmvflag;

SPECTRUM_PACKET_HEADER PDdatreq;
SPECTRUM_PACKET_HEADER PDdatack;
SPECTRUM_PACKET_HEADER PDstopreq;
SPECTRUM_PACKET_HEADER PDstopack;
SPECTRUM_PACKET_HEADER PDdatsend;
SPECTRUM_PACKET_HEADER RFInfoset;
SPECTRUM_PACKET_HEADER RFInfoack;
SPECTRUM_PACKET_HEADER RFInforeq;
SPECTRUM_PACKET_HEADER RFInforeqack;
SPECTRUM_PACKET_HEADER PDDAUset;
SPECTRUM_PACKET_HEADER PDDAUsetack;
SPECTRUM_PACKET_HEADER PDDAUreq;
SPECTRUM_PACKET_HEADER PDDAUreqack;
SPECTRUM_PACKET_HEADER Alarm;
SPECTRUM_PACKET_HEADER Alarmack;
SPECTRUM_PACKET_HEADER Keepalive;
SPECTRUM_PACKET_HEADER Keepaliveack;

extern LEAFLIST sbsh_leaflist[6];
extern LEAFLIST scbr_leaflist[6];
extern LEAFLIST oltc_leaflist[6];
extern LEAFLIST siml_leaflist[6];
extern LEAFLIST spdc_leaflist[6];

char leaf_key[200];

extern SBSH_DATA_CISCO sbsh_data_cisco;
extern SBSH_SENSOR_DATA sbsh_sensors[6];

OLTC_SENSOR_DATA oltc_sensors[3];

extern CB_FILE cb_file;

extern int LU;

//Extension
//#define ONE_CYCLE 256
#define ONE_CYCLE 128

extern ds61850IfcInfo* g_p61850IfcInfo;

volatile int countex = 0;
pthread_cond_t run_cond_a = PTHREAD_COND_INITIALIZER;
pthread_cond_t run_cond_b = PTHREAD_COND_INITIALIZER;

char line01[200];
char line02[200];
char line03[200];
char line04[200];
char line05[200];

float volts[100];

unsigned char ch1mem[60][128];
unsigned char ch2mem[60][128];
unsigned char ch3mem[60][128];
unsigned char ch4mem[60][128];

typedef struct node {
    MSG_PD_BODY data;
	unsigned char caliData[128];
    struct node* next;
} Node;

typedef struct {
    Node* head;
    Node* tail;
    int size;
    pthread_mutex_t mutex;
	//pthread_mutex_t mutexCali;
} Queue;

volatile int stopIssued = 0;
pthread_mutex_t stopMutex;

_PDDAU Pddau;

/**
 * Check for a mutex if program stop has been issued.
 */
int getStopIssued(void) {
  int ret = 0;
  pthread_mutex_lock(&stopMutex);
  ret = stopIssued;
  pthread_mutex_unlock(&stopMutex);
  return ret;
}

/**
 * Set a mutex for program stop has been issued.
 * @param val Assign a positive integer value to stop program.
 */
void setStopIssued(int val) {
  pthread_mutex_lock(&stopMutex);
  stopIssued = val;
  pthread_mutex_unlock(&stopMutex);
}

/**
 * Initialize a Queue object and its members.
 * @param queue Pass a new Queue instance to initialize.
 */
void initQueue(Queue* queue) {
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    pthread_mutex_init(&queue->mutex, NULL);
    //pthread_mutex_init(&queue->mutexCali, NULL);	
}

/**
 * This function pushes data into Queue object.
 * @param queue The Queue instance where to push the data.
 * @param data The data to push.
 */
void enqueue(Queue* queue, MSG_PD_BODY data) 
{
    ////pthread_mutex_lock(&queue->mutex);
    pthread_mutex_trylock(&queue->mutex);
    //printf("EL ");
    Node* node = (Node*) malloc(sizeof(Node));
    node->data = data;
    node->next = NULL;
    if (queue->size == 0) 
    {
        queue->head = node;
    } 
    else 
    {
        queue->tail->next = node;
    }
    queue->tail = node;
    queue->size++;
    pthread_mutex_unlock(&queue->mutex);
    //printf("EU ");
    //sleep(0.1);
}


/**
 * This function pops the last data from a Queue instance.
 * @param queue The pop operation will be done on the Queue instance.
 */
unsigned char* dequeue(Queue* queue) 
{
    ////pthread_mutex_lock(&queue->mutex);
    pthread_mutex_trylock(&queue->mutex);
    ////printf("<> ");
    if (queue->size <= 0) 
    {
        unsigned char* empty;
        pthread_mutex_unlock(&queue->mutex);
        //printf("... ");
        //sleep(0.1);
        //return empty;
        ////printf("<%d \n",queue->size);
        ////printf("<<\n");
        ////printf("dq --! ");
        return 0;
    }
    ////printf("\%");
    unsigned char* data = queue->head->data.data;
    //printf("<<\n");
    Node* oldHead = queue->head;
    ////printf("<%d \n",queue->size);
    queue->head = oldHead->next;
    ////printf("<<\n");
    free(oldHead);
    ////printf("dq --! ");
    if (queue->size > 0)
    {
    	/* code */
    	queue->size--;
    }
    
    pthread_mutex_unlock(&queue->mutex);
    //printf(".. ");
    //sleep(0.1);
    return data;
}

/**
 * Checks for empty queue.
 * @param queue Operation perform on this Queue instance
 */
int isEmpty(Queue* queue) {
    pthread_mutex_lock(&queue->mutex);
    int empty = (queue->size == 0);
    pthread_mutex_unlock(&queue->mutex);
    return empty;
}

/**
 * Checks for queue size.
 * @param queue Operation perform on this Queue instance
 */

int getSize(Queue* queue) {
    pthread_mutex_lock(&queue->mutex);
    int size = queue->size;
    pthread_mutex_unlock(&queue->mutex);
    return size;
}

Queue *gpch1connectionQ;
Queue *gpch2connectionQ;
Queue *gpch3connectionQ;
Queue *gpch4connectionQ;

Queue *gpch1connectionCQ;
Queue *gpch2connectionCQ;
Queue *gpch3connectionCQ;
Queue *gpch4connectionCQ;

/**
It is preparing a data packet to send to the PD Simulator PDDAU? . So PD simulator starts 
sending data over the connected TCP connection continiously.
*/
void InitSensor(){
	PDdatreq.body_len = 0x0000;
	PDdatreq.msg_id = MSG_PD_START_ID;
	PDdatreq.msg_type = MSG_REQUEST_TYPE;
	
	PDdatack.body_len = 0x0000;
	PDdatack.msg_id = MSG_PD_START_ID;
	PDdatack.msg_type = MSG_REQUEST_ACK_TYPE;
	
	PDstopreq.body_len = 0x0000;
	PDstopreq.msg_id = MSG_PD_STOP_ID;
	PDstopreq.msg_type = MSG_REQUEST_TYPE;
	
	PDstopack.msg_id = MSG_PD_STOP_ID;
	PDstopack.msg_type = MSG_REQUEST_ACK_TYPE;
	PDstopack.body_len = 0x0000;
	
	PDdatsend.msg_id = MSG_PD_SEND_ID;
	PDdatsend.msg_type = MSG_SEND_TYPE;
	PDdatsend.body_len = 0x0410;
	
	RFInfoset.msg_id = MSG_RF_INFO_ID;
	RFInfoset.msg_type = MSG_REQUEST_TYPE;
	RFInfoset.body_len = 0x001A;
	
	RFInfoack.msg_id = MSG_RF_INFO_ID;
	RFInfoack.msg_type = MSG_REQUEST_ACK_TYPE;
	RFInfoack.body_len = 0x0000;
	
	RFInforeq.msg_id = MSG_RF_INFO_ID;
	RFInforeq.msg_type = MSG_READ_INFO_TYPE;
	RFInforeq.body_len = 0x0000;
	
	RFInforeqack.msg_id = MSG_RF_INFO_ID;
	RFInforeqack.msg_type = MSG_READ_INFO_RESPONSE_TYPE;
	RFInforeqack.body_len = 0x001A;
	
	PDDAUset.msg_id = MSG_PDDAU_INFO_ID;
	PDDAUset.msg_type = MSG_REQUEST_TYPE;
	PDDAUset.body_len = 0x0023;
	
	PDDAUsetack.msg_id = MSG_PDDAU_INFO_ID;
	PDDAUsetack.msg_type = MSG_REQUEST_ACK_TYPE;
	PDDAUsetack.body_len = 0x0000;
	
	PDDAUreq.msg_id = MSG_PDDAU_INFO_ID;
	PDDAUreq.msg_type = MSG_READ_INFO_TYPE;
	PDDAUreq.body_len = 0x0000;
	
	PDDAUreqack.msg_id = MSG_PDDAU_INFO_ID;
	PDDAUreqack.msg_type = MSG_READ_INFO_RESPONSE_TYPE;
	PDDAUreqack.body_len = 0x0023;
	
	Alarm.msg_id = MSG_ALARM_ID;
	Alarm.msg_type = MSG_ALARM_TYPE;
	
	Alarmack.msg_id = MSG_ALARM_ID;
	Alarmack.msg_type = MSG_ALARM_ACK_TYPE;
	
	Keepalive.msg_id = MSG_KEEP_ALIVE_ID;
	Keepalive.msg_type = MSG_REQUEST_TYPE;
	
	Keepaliveack.msg_id = MSG_KEEP_ALIVE_ID;
	Keepaliveack.msg_type = MSG_REQUEST_ACK_TYPE;
	
	memset(totdatfile1,'\0',sizeof(totdatfile1));
	memset(totdatfile1,'\0',sizeof(totdatfile2));
	memset(totdatfile1,'\0',sizeof(totdatfile3));
	memset(totdatfile1,'\0',sizeof(totdatfile4));
	datflag = 0;
	rmvflag = 0;
}

/**
 * This is a pretty basic function to make a HTTP request to a HTTP server. 
 */

int request(void) {
    char buffer[BUFSIZ];
    enum CONSTEXPR { MAX_REQUEST_LEN = 1024};
    char request[MAX_REQUEST_LEN];
    //char request_template[] = "GET /api/Common/StartDataCollector?luName=YHGLU&ipAddress=192.168.10.111&luType=G&equipmentName=SCBR1 HTTP/1.1\r\nHost: %s\r\n\r\n";
    // char request_template[] = "GET /api/Common/StartDataCollector?luName=YHGLU&ipAddress=192.168.247.104&luType=G&equipmentName=SCBR1 HTTP/1.0\r\n\r\n";
	char request_template[] = "GET / HTTP/1.0\r\n\r\n";
	struct protoent *protoent;
    char *hostname = "192.168.246.132";
    in_addr_t in_addr;
    int request_len;
    int socket_file_descriptor;
    ssize_t nbytes_total, nbytes_last;
    struct hostent *hostent;
    struct sockaddr_in sockaddr_in;
    unsigned short server_port = 80;

    request_len = snprintf(request, MAX_REQUEST_LEN, request_template);
    if (request_len >= MAX_REQUEST_LEN) {
        fprintf(stderr, "request length large: %d\n", request_len);
        //exit(EXIT_FAILURE);
		return -1;
    }

    /* Build the socket. */
    protoent = getprotobyname("tcp");
    if (protoent == NULL) {
        perror("getprotobyname");
		return -1;
        //exit(EXIT_FAILURE);
    }
    socket_file_descriptor = socket(AF_INET, SOCK_STREAM, protoent->p_proto);
    if (socket_file_descriptor == -1) {
        perror("socket");
        //exit(EXIT_FAILURE);
		return -1;
    }

    /* Build the address. */
    hostent = gethostbyname(hostname);
    if (hostent == NULL) {
        fprintf(stderr, "error: gethostbyname(\"%s\")\n", hostname);
        //exit(EXIT_FAILURE);
		return -1;
    }
    
	//in_addr = inet_addr(inet_ntoa(*(struct in_addr*)*(hostent->h_addr_list)));
    in_addr = inet_addr("192.168.246.177");

	if (in_addr == (in_addr_t)-1) {
        fprintf(stderr, "error: inet_addr(\"%s\")\n", *(hostent->h_addr_list));
        //exit(EXIT_FAILURE);
		return -1;
    }
    sockaddr_in.sin_addr.s_addr = in_addr;
    sockaddr_in.sin_family = AF_INET;
    sockaddr_in.sin_port = htons(server_port);

    /* Actually connect. */
    if (connect(socket_file_descriptor, (struct sockaddr*)&sockaddr_in, sizeof(sockaddr_in)) == -1) {
        perror("connect");
        //exit(EXIT_FAILURE);
		return -1;
    }

    /* Send HTTP request. */
    nbytes_total = 0;
    while (nbytes_total < request_len) {
        nbytes_last = write(socket_file_descriptor, request + nbytes_total, request_len - nbytes_total);
        if (nbytes_last == -1) {
            perror("write");
            //exit(EXIT_FAILURE);
			return -1;
        }
        nbytes_total += nbytes_last;
    }

    /* Read the response. */
    ////fprintf(stderr, "debug: before first read\n");
    while ((nbytes_total = read(socket_file_descriptor, buffer, BUFSIZ)) > 0) {
        ////fprintf(stderr, "debug: after a read\n");
        //write(STDOUT_FILENO, buffer, nbytes_total);
		printf("%s", buffer);
    }
    
	//// fprintf(stderr, "debug: after last read\n");
    if (nbytes_total == -1) {
        perror("read");
        //exit(EXIT_FAILURE);
		return -1;
    }

    close(socket_file_descriptor);

	return 0;
}


/*
void CreatePDdat(U8 channum, U8 pddata[PDdatMax], int * PDcounting)
{
	FILE *fileToWrite;
    time_t now;
    struct tm *tm;
    now = time(0);
	//printf("111111 %d filemax:%d and filebuff: %d\n", channum,filebuffermax,PDcounting[channum]);
    if ((tm = localtime (&now)) == NULL) {
        printf ("Error extracting time stuff\n");
        return 1;
    }
	char filename[50];
	snprintf(filename,sizeof(filename),"/data/COMTRADE/%d_00_%04d%02d%02d%02d%02d%02d.dat",channum+1,tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,tm->tm_hour, tm->tm_min, tm->tm_sec);
	if (PDcounting[channum] == filebuffermax){ ///////////////////Remove files if the number of the files over filebuffermax for each sensors
		U8 temp[50];
		strcpy(temp, PDfilenamebuffs[channum].namebuffer[0]);
		for(int i=1;i<filebuffermax;i++){
			strcpy(PDfilenamebuffs[channum].namebuffer[i-1],PDfilenamebuffs[channum].namebuffer[i]);
			}
		//printf("filename: %s\n",temp);
		//printf("222222 %d filemax:%d and filebuff: %d\n",channum,filebuffermax,PDcounting[channum]);
		int status = remove(temp);
	    if( status != 0 ){
	       printf("Unable to delete the file\n");
	       perror("Error");
	       }
		strcpy(PDfilenamebuffs[channum-1].namebuffer[filebuffermax-1],filename);
    }
	else if(PDcounting[channum]<filebuffermax){
		//printf("333333 %s %d filemax:%d and filebuff: %d\n",filename, channum,filebuffermax,PDcounting[channum]);
		strcpy(PDfilenamebuffs[channum].namebuffer[PDcounting[channum]],filename);
		PDcounting[channum]++;
		}	
	else{
		//////should return////
		return;
		}
		
	if ((fileToWrite = fopen(filename, "wb+")) != NULL) {
		//printf("%s pd data \n",filename);
		fwrite(pddata, PDdatMax, 1, fileToWrite);
		fclose(fileToWrite);
		}
}
*/

/**
 * Simple File writing function.
 * @param filename File name to write.
 */
void saveIntoRamFs(char *filename)
{
	FILE *file;
	printf("Filename: %s\n", filename);		
	file = fopen(filename, "wb");
	////fwrite(&p8data, sizeof(unsigned char), 128, file);
}

#define DEV_FACTOR	230.0 // 279.0 before
#define OFFSET_FACTOR 71.91

#define SVFILE1() sprintf(filename, "/data/COMTRADE/REALTIME/PDCM/01_10_%s.dat", time_string)
#define SVFILE2() sprintf(filename, "/data/COMTRADE/REALTIME/PDCM/02_10_%s.dat", time_string)
#define SVFILE3() sprintf(filename, "/data/COMTRADE/REALTIME/PDCM/03_10_%s.dat", time_string)
#define SVFILE4() sprintf(filename, "/data/COMTRADE/REALTIME/PDCM/04_10_%s.dat", time_string)

#define QUEUE_SIZE_THR		2

/**
 * Function to process data coming from PDDAU. This is a thread function. 
 */
void* ProcessPDDataThread(void *params)
{
	float f32dataA = 0.0;
	float f32dataA1 = 0.0;
	float f32dataA2 = 0.0;
	float f32dataA3 = 0.0;
	float f32dataA4 = 0.0;
	int s32data = 0;
	unsigned char* ps32data;
	unsigned char ps32CaliData[ONE_CYCLE];
	//char time_string[15];

	int ch1_counter = 0;
	int ch2_counter = 0;
	int ch3_counter = 0;
	int ch4_counter = 0;

	int itest = 0;

	unsigned char *p8data;

	VT100_scrclr();

	time_t current_time;
	struct tm *time_info;
	char time_string[45];
	char filename[60];
	ARM_MMS_UTC_TIME dsUTCTime;

	//Extension
	//strftime(time_string, sizeof(time_string), "%Y%m%d%H%M%S", time_info);

	while(1)
	{

		dsUTCTime = g_ds61850SharedMem.dsArmDestUTC;
    	current_time = (time_t)dsUTCTime.secs;
		// Add 9 hours (in seconds) to convert UTC to UTC+9
		current_time += 9 * 3600;		
    	time_info = gmtime(&current_time);
    	strftime(time_string, sizeof(time_string), "%Y%m%d%H%M%S", time_info);

		if (getSize(gpch1connectionQ) > QUEUE_SIZE_THR )
		{
			ps32data = dequeue(gpch1connectionQ);
			for(int i = 0; i < (ONE_CYCLE * 2); i++)
			{
				if (i % 2 == 0)
					s32data = ps32data[i];
				else
				{
					s32data <<= 8;
					s32data |= ps32data[i];
					f32dataA = (float)s32data;
					//s32data= s32data * (5/279) - 64.3; // Conversion to dbm unit. -65dbm to 0dbm
					f32dataA = (f32dataA * (5.0/DEV_FACTOR)) - OFFSET_FACTOR;
					ps32CaliData[i/2] = (int)f32dataA;
					
					ch1mem[ch1_counter][i/2] = (int)f32dataA;
				}
			}			
			MSG_PD_BODY dstdata;
			dstdata.ch_idx = 1;
			//memcpy(dstdata.data, ps32CaliData, ONE_CYCLE);
			//enqueue(gpch1connectionCQ, dstdata);
			
			f32dataA1 = f32dataA;
			//VT100_goto(1, 12);
			//printf("CH1 - %f", f32dataA);

			ch1_counter++;
			if(ch1_counter >= 60)
			{
				FILE *file;
				SVFILE1();
				// sprintf(filename, "/data/COMTRADE/01_00_%s.dat", time_string);
				sprintf(line02, "New Filename: %s  \n", filename);
				file = fopen(filename, "wb");
				fwrite(ch1mem, sizeof(unsigned char), 7680, file);
				fclose(file);
				ch1_counter = 0;
			}
			
		}
		
		if (getSize(gpch2connectionQ) > QUEUE_SIZE_THR )
		{		
			ps32data = dequeue(gpch2connectionQ);
			for(int i = 0; i < 256; i++)
			{			
				if (i %2 == 0)
					s32data = ps32data[i];
				else
				{
					s32data <<= 8;
					s32data |= ps32data[i];
					f32dataA = (float)s32data;
					//s32data= s32data * (5/279) - 64.3; // Conversion to dbm unit. -65dbm to 0dbm
					f32dataA = (f32dataA * (5.0/DEV_FACTOR)) - OFFSET_FACTOR;
					ps32CaliData[i/2] = (int)f32dataA;

					ch2mem[ch2_counter][i/2] = (int)f32dataA;
				}
			}			
			/*
			MSG_PD_BODY dstdata;
			dstdata.ch_idx = 2;
			memcpy(dstdata.data, ps32CaliData, ONE_CYCLE);
			enqueue(gpch2connectionCQ, dstdata);			
			*/
			f32dataA2 = f32dataA;
			//VT100_goto(12, 12);
			//printf("CH2 - %f", f32dataA);

			ch2_counter++;
			if(ch2_counter >= 60)
			{
				FILE *file;
				SVFILE2();
				// sprintf(filename, "/data/COMTRADE/02_00_%s.dat", time_string);				
				sprintf(line03, "New Filename: %s  \n", filename);
				file = fopen(filename, "wb");
				fwrite(ch2mem, sizeof(unsigned char), 7680, file);
				fclose(file);
				ch2_counter = 0;
			}
		}	
		
		if (getSize(gpch3connectionQ) > QUEUE_SIZE_THR )
		{		
			ps32data = dequeue(gpch3connectionQ);
			for(int i = 0; i < 256; i++)
			{			
				if (i %2 == 0)
					s32data = ps32data[i];
				else
				{
					s32data <<= 8;
					s32data |= ps32data[i];
					f32dataA = (float)s32data;
					//s32data= s32data * (5/279) - 64.3; // Conversion to dbm unit. -65dbm to 0dbm
					f32dataA = (f32dataA * (5.0/DEV_FACTOR)) - OFFSET_FACTOR;
					ps32CaliData[i/2] = (int)f32dataA;

					ch3mem[ch3_counter][i/2] = (int)f32dataA;
				}
			}			
			/*
			MSG_PD_BODY dstdata;
			dstdata.ch_idx = 3;
			memcpy(dstdata.data, ps32CaliData, ONE_CYCLE);
			enqueue(gpch3connectionCQ, dstdata);					
			*/
			f32dataA3 = f32dataA;
			//VT100_goto(24, 12);
			//printf("CH3 - %f", f32dataA);

			ch3_counter++;
			if(ch3_counter >= 60)
			{
				FILE *file;
				SVFILE3();
				// sprintf(filename, "/data/COMTRADE/03_00_%s.dat", time_string);				
				sprintf(line04, "New Filename: %s  \n", filename);
				file = fopen(filename, "wb");
				fwrite(ch3mem, sizeof(unsigned char), 7680, file);
				fclose(file);
				ch3_counter = 0;
			}
		}	

		if (getSize(gpch4connectionQ) > QUEUE_SIZE_THR )
		{		
			ps32data = dequeue(gpch4connectionQ);
			for(int i = 0; i < 256; i++)
			{			
				if (i %2 == 0)
					s32data = ps32data[i];
				else
				{
					s32data <<= 8;
					s32data |= ps32data[i];
					f32dataA = (float)s32data;
					//s32data= s32data * (5/279) - 64.3; // Conversion to dbm unit. -65dbm to 0dbm
					f32dataA = (f32dataA * (5.0/DEV_FACTOR)) - OFFSET_FACTOR;
					ps32CaliData[i/2] = (int)f32dataA;

					ch4mem[ch4_counter][i/2] = (int)f32dataA;
				}
			}			
			/*
			MSG_PD_BODY dstdata;
			dstdata.ch_idx = 4;
			memcpy(dstdata.data, ps32CaliData, ONE_CYCLE);
			enqueue(gpch4connectionCQ, dstdata);
			*/
			f32dataA4 = f32dataA;
			//VT100_goto(36, 12);
			//printf("CH4 - %f", f32dataA);

			ch4_counter++;
			if(ch4_counter >= 60)
			{
				FILE *file;
				SVFILE4();
				// sprintf(filename, "/data/COMTRADE/04_00_%s.dat", time_string);				
				sprintf(line05, "New Filename: %s  \n", filename);
				file = fopen(filename, "wb");
				fwrite(ch4mem, sizeof(unsigned char), 7680, file);
				fclose(file);
				ch4_counter = 0;
			}
		}			
		//printf("size of CQ1 : %d, CQ2 : %d, CQ3 : %d, CQ4 : %d\n",getSize(gpch1connectionCQ), getSize(gpch2connectionCQ), getSize(gpch3connectionCQ), getSize(gpch4connectionCQ));
		//printf("CQ1 - %d\n", getSize(gpch1connectionCQ));
		if(countex < 5590)
		{
			countex++;
		}
		else
		{

			//getVolts(volts);
			
			countex = 0;
			//VT100_goto(1, 2);
			//printf("size of CQ1 : %d, CQ2 : %d, CQ3 : %d, CQ4 : %d\n",getSize(gpch1connectionCQ), getSize(gpch2connectionCQ), getSize(gpch3connectionCQ), getSize(gpch4connectionCQ));
			
			VT100_goto(1, 12);
			printf("CH1: %f CH2: %f CH3: %f CH4: %f ", f32dataA1, f32dataA2, f32dataA3, f32dataA4);
			
			// sprintf(line01, "size of CQ1 : %d, CQ2 : %d, CQ3 : %d, CQ4 : %d\n",getSize(gpch1connectionCQ), getSize(gpch2connectionCQ), getSize(gpch3connectionCQ), getSize(gpch4connectionCQ));

			VT100_goto(1,5);
			printf("%s", line02);
			VT100_goto(1,6);
			printf("%s", line03);
			VT100_goto(1,7);
			printf("%s", line04);
			VT100_goto(1,8);
			printf("%s", line05);
			VT100_goto(1,10);
			printf("%s \n", line01);
			VT100_goto(1, 11);
			printf("AC Volt: %f", volts[0]);
		}

		if(getStopIssued() > 0)
		{
			setStopIssued(0);
			printf("------------xxxxxxxxxx------------STOPPED : ProcessPDDataThread()\r\n\r\n");
			break;
		}
		////sleep(0.3);
	}
}

// void rmvfunc(){//////////////////Not Using///////////////////////////
// 	while(1){
// 		struct dirent *de;  // Pointer for directory entry
  
// 	    // opendir() returns a pointer of DIR type. 
// 	    DIR *dr = opendir("/data/COMTRADE");
	  
// 	    if (dr == NULL)  // opendir returns NULL if couldn't open directory
// 	    {
// 	        printf("Could not open current directory" );
// 	        return 0;
// 	    }
// 	  	usleep(500);
// 	    // Refer http://pubs.opengroup.org/onlinepubs/7990989775/xsh/readdir.html
// 	    // for readdir()
// 	    while ((de = readdir(dr)) != NULL){
// 	            printf("%s\n", de->d_name);
// 				//int status = remove(de->d_name);
// 			   // if( status != 0 ){
// 			   //    printf("Unable to delete the file\n");
// 			   //    perror("Error");
// 			   //    }
// 	    	}
	  
// 	    closedir(dr);    
// 		free(de);
// 		}
// }


/*<USER_CODE_START> <description> This function will employ a while loop to receive data
from PD over ethernet 192.168.10.142:5000. It sends a command string first, then starts 
receiving data. Data format.... </description> */

// void showPdStream(int sockfd){

// 	//Extension
// 	//ret = pthread_attr_setstacksize(&tattr, size);

// 	MSG_PD_Tot recvdata;
// 	memset(&recvdata, '\0', sizeof(MSG_PD_Tot));
// 	U8 processed[1024];
// 	U8 totdata [SensorMAX];
// 	U32 count = 0;
// 	pthread_t FileThread, CaliThread; 
// 	MSG_PD_FULL_PACKET *pu8fulldata = malloc(sizeof(MSG_PD_FULL_PACKET));

// 	gpch1connectionQ = malloc(sizeof(Queue));
// 	if (gpch1connectionQ == NULL) {
// 		fprintf(stderr, "Error: failed to allocate memory for gpch1connectionQ\n");
// 		exit(EXIT_FAILURE);
// 	}
	
// 	gpch2connectionQ = malloc(sizeof(Queue));
// 	if (gpch2connectionQ == NULL) {
// 		fprintf(stderr, "Error: failed to allocate memory for gpch2connectionQ\n");
// 		exit(EXIT_FAILURE);
// 	}
	
// 	gpch3connectionQ = malloc(sizeof(Queue));
// 	if (gpch3connectionQ == NULL) {
// 		fprintf(stderr, "Error: failed to allocate memory for gpch3connectionQ\n");
// 		exit(EXIT_FAILURE);
// 	}
	
// 	gpch4connectionQ = malloc(sizeof(Queue));
// 	if (gpch4connectionQ == NULL) {
// 		fprintf(stderr, "Error: failed to allocate memory for gpch4connectionQ\n");
// 		exit(EXIT_FAILURE);
// 	}

// 	gpch1connectionCQ = malloc(sizeof(Queue));
// 	if (gpch1connectionCQ == NULL) {
// 		fprintf(stderr, "Error: failed to allocate memory for gpch1connectionQ\n");
// 		exit(EXIT_FAILURE);
// 	}
	
// 	gpch2connectionCQ = malloc(sizeof(Queue));
// 	if (gpch2connectionCQ == NULL) {
// 		fprintf(stderr, "Error: failed to allocate memory for gpch2connectionQ\n");
// 		exit(EXIT_FAILURE);
// 	}
	
// 	gpch3connectionCQ = malloc(sizeof(Queue));
// 	if (gpch3connectionCQ == NULL) {
// 		fprintf(stderr, "Error: failed to allocate memory for gpch3connectionQ\n");
// 		exit(EXIT_FAILURE);
// 	}
	
// 	gpch4connectionCQ = malloc(sizeof(Queue));
// 	if (gpch4connectionCQ == NULL) {
// 		fprintf(stderr, "Error: failed to allocate memory for gpch4connectionQ\n");
// 		exit(EXIT_FAILURE);
// 	}


// 	initQueue(gpch1connectionQ);
// 	initQueue(gpch2connectionQ);	
// 	initQueue(gpch3connectionQ);	
// 	initQueue(gpch4connectionQ);	
// 	initQueue(gpch1connectionCQ);
// 	initQueue(gpch2connectionCQ);	
// 	initQueue(gpch3connectionCQ);	
// 	initQueue(gpch4connectionCQ);


	
// 	//pthread_t rmthread;
// 	//pthread_create(&CaliThread, NULL, CalibratePDdatthread, NULL);
// 	//pthread_create ( &FileThread, NULL, CreatePDdatthread, NULL );
// 	//pthread_create ( &rmthread, NULL, rmvfunc, NULL );
// 	int dbm = 0;
// 	float fdbm = 0;
// 	U32 iResult;
// 	while(1){
// 		iResult = read(sockfd, pu8fulldata, sizeof(MSG_PD_FULL_PACKET));
		
//         if (iResult == -1)
//         {
//         	fprintf(stderr, "recv Error Occurred %s (%d)\n", strerror(errno), errno);
//             return -1;
//         }
		
// 		if(pu8fulldata->header.msg_type != MSG_SEND_TYPE || pu8fulldata->header.msg_id != MSG_PD_SEND_ID)
//         {
//             printf("PD Sending failed. type : %x, id : %x\n", pu8fulldata->header.msg_type, pu8fulldata->header.msg_id);
//             return -1;
//         } 

// 		for(int i = 0; i < 4; i++)
// 		{
// 			pu8fulldata->data[i].ch_idx = i+1;
// 			if(i == 0)
// 			{
// 				//enqueue(gpch1connectionQ, pu8fulldata->data[i]);
// 				VT100_goto(1, 12);
// 				dbm = pu8fulldata->data[i].data[0];
// 				dbm <<= 8;
// 				dbm |= pu8fulldata->data[i].data[1];
// 				fdbm = (float)dbm;
// 				fdbm = (fdbm * (5.0/279.0)) - 64.3;
// 				//printf(" %d ", pu8fulldata->data[i].data[0]);
// 				printf(" %f ", fdbm);
// 			}
// 			else if(i == 1)
// 			{
// 				//enqueue(gpch2connectionQ, pu8fulldata->data[i]);
// 				VT100_goto(10, 12);
// 				dbm = pu8fulldata->data[i].data[0];
// 				dbm <<= 8;
// 				dbm |= pu8fulldata->data[i].data[1];
// 				fdbm = (float)dbm;
// 				fdbm = (fdbm * (5.0/279.0)) - 64.3;
// 				//printf(" %d ", pu8fulldata->data[i].data[0]);
// 				printf(" %f ", fdbm);
// 			}
// 			else if(i == 2)
// 			{
// 				//enqueue(gpch3connectionQ, pu8fulldata->data[i]);
// 				VT100_goto(20, 12);
// 				dbm = pu8fulldata->data[i].data[0];
// 				dbm <<= 8;
// 				dbm |= pu8fulldata->data[i].data[1];
// 				fdbm = (float)dbm;
// 				fdbm = (fdbm * (5.0/279.0)) - 64.3;
// 				//printf(" %d ", pu8fulldata->data[i].data[0]);
// 				printf(" %f ", fdbm);
// 			}
// 			else if(i == 3)
// 			{
// 				enqueue(gpch4connectionQ, pu8fulldata->data[i]);
// 				VT100_goto(30, 12);
// 				dbm = pu8fulldata->data[i].data[0];
// 				dbm <<= 8;
// 				fdbm = (float)dbm;
// 				fdbm = (fdbm * (5.0/279.0)) - 64.3;
// 				//printf(" %d ", pu8fulldata->data[i].data[0]);
// 				printf(" %f ", fdbm);
// 			}
// 		}
		
// 		//usleep(1);
// 	}
// }


/**
 * This function to push PDDAU data into queues for further processing. This is also an 
 * infinite loop called by another Thread function RunPDDAU(...) . RunPDDAU(...) is initialized 
 * in main.c main(...) Entrypoint function.
 * @param sockfd An already established TCP Socket connected to the PDDAU unit.
 */
void EnqueuePDData(int sockfd){

	//Extension
	//ret = pthread_attr_setstacksize(&tattr, size);

	MSG_PD_Tot recvdata;
	memset(&recvdata, '\0', sizeof(MSG_PD_Tot));
	U8 processed[1024];
	U8 totdata [SensorMAX];
	U32 count = 0;
	pthread_t FileThread, CaliThread; 
	MSG_PD_FULL_PACKET *pu8fulldata = (MSG_PD_FULL_PACKET *)malloc(sizeof(MSG_PD_FULL_PACKET));
	gpch1connectionQ = (Queue *)malloc(sizeof(Queue));
	if (gpch1connectionQ == NULL) {
		fprintf(stderr, "Error: failed to allocate memory for gpch1connectionQ\n");
		exit(EXIT_FAILURE);
	}
	
	gpch2connectionQ = (Queue *)malloc(sizeof(Queue));
	if (gpch2connectionQ == NULL) {
		fprintf(stderr, "Error: failed to allocate memory for gpch2connectionQ\n");
		exit(EXIT_FAILURE);
	}
	
	gpch3connectionQ = (Queue *)malloc(sizeof(Queue));
	if (gpch3connectionQ == NULL) {
		fprintf(stderr, "Error: failed to allocate memory for gpch3connectionQ\n");
		exit(EXIT_FAILURE);
	}
	
	gpch4connectionQ = (Queue *)malloc(sizeof(Queue));
	if (gpch4connectionQ == NULL) {
		fprintf(stderr, "Error: failed to allocate memory for gpch4connectionQ\n");
		exit(EXIT_FAILURE);
	}

/*
	gpch1connectionCQ = malloc(sizeof(Queue));
	if (gpch1connectionCQ == NULL) {
		fprintf(stderr, "Error: failed to allocate memory for gpch1connectionQ\n");
		exit(EXIT_FAILURE);
	}
	
	gpch2connectionCQ = malloc(sizeof(Queue));
	if (gpch2connectionCQ == NULL) {
		fprintf(stderr, "Error: failed to allocate memory for gpch2connectionQ\n");
		exit(EXIT_FAILURE);
	}
	
	gpch3connectionCQ = malloc(sizeof(Queue));
	if (gpch3connectionCQ == NULL) {
		fprintf(stderr, "Error: failed to allocate memory for gpch3connectionQ\n");
		exit(EXIT_FAILURE);
	}
	
	gpch4connectionCQ = malloc(sizeof(Queue));
	if (gpch4connectionCQ == NULL) {
		fprintf(stderr, "Error: failed to allocate memory for gpch4connectionQ\n");
		exit(EXIT_FAILURE);
	}
*/

	initQueue(gpch1connectionQ);
	initQueue(gpch2connectionQ);	
	initQueue(gpch3connectionQ);	
	initQueue(gpch4connectionQ);	
	// initQueue(gpch1connectionCQ);
	// initQueue(gpch2connectionCQ);	
	// initQueue(gpch3connectionCQ);	
	// initQueue(gpch4connectionCQ);


	//connect_tcp(ACQ_ADDR,ACQ_PORT);
	
	//pthread_t rmthread;
	pthread_create(&CaliThread, NULL, ProcessPDDataThread, NULL);
	
	////pthread_create ( &FileThread, NULL, CreatePDdatthread, NULL );
	
	//pthread_create ( &rmthread, NULL, rmvfunc, NULL );
	U32 iResult;
	while(1){
		iResult = read(sockfd, pu8fulldata, sizeof(MSG_PD_FULL_PACKET));
		
        if (iResult == -1)
        {
        	fprintf(stderr, "recv Error Occurred %s (%d)\n", strerror(errno), errno);
            return;
        }
		
		if(pu8fulldata->header.msg_type != MSG_SEND_TYPE || pu8fulldata->header.msg_id != MSG_PD_SEND_ID)
        {
            printf("PD Sending failed. type : %x, id : %x\n", pu8fulldata->header.msg_type, pu8fulldata->header.msg_id);
            return;
        } 

		for(int i = 0; i < 4; i++)
		{
			pu8fulldata->data[i].ch_idx = i+1;
			if(i == 0)
			{
				enqueue(gpch1connectionQ, pu8fulldata->data[i]);
			}
			else if(i == 1)
			{
				enqueue(gpch2connectionQ, pu8fulldata->data[i]);
			}
			else if(i == 2)
			{
				enqueue(gpch3connectionQ, pu8fulldata->data[i]);
			}
			else if(i == 3)
			{
				enqueue(gpch4connectionQ, pu8fulldata->data[i]);
			}
		}
		
		usleep(1);
		//sleep(0.3);
		//printf(".");
	}
}

/**
 * This is just a string search function iterating through a short list of leafs called LEAFLIST. 
 * @param leaflist An instance of a structure name LEAFLIST. Generally there are a few number of 
 * this types of list available in the project. These lists are divided by the sensor names like
 * SPDC, SCBR, SLTC .... 
 * @param endName The niddle to find from the list.
 * @param matchedKey return the whole leaf here.
 */

void GetKeyStringByEndName(LEAFLIST *leaflist, char *endName, char *matchedKey)
{
	int lidx = 0;
	char *p;
	
		
		for(lidx=0;lidx<leaflist->count;lidx++)
		//for(lidx=0;lidx<g_p61850IfcInfo->s32leafnum;lidx++)
		{
			//printf("%s == %s ", endName, leaflist->lines[lidx]);
			p = strstr(leaflist->lines[lidx], endName);
			//p = strstr(&g_p61850IfcInfo->p61850mappingtbl[lidx], endName);
			if(p)
			{
				//printf("Full Leaf String :%s : %s", endName, leaflist->lines[lidx]);
				//GetKeyFromLeafString(leaflist->lines[lidx], leaf_key, "Sanion_154kV_GIS_D01_2AMLU");
				//GetKeyFromLeafString(leaflist->lines[lidx], leaf_key, "Sanion_154kV_Mtr_D01_1AMLU");
				//printf("\n\n------ g_p61850IfcInfo->ps8LDname ------ %s ------------\n\n", g_p61850IfcInfo->ps8LDname);
				//printf("LU : %s\n", g_p61850IfcInfo->ps8LDname);
				GetKeyFromLeafString(leaflist->lines[lidx], leaf_key, (char *)g_p61850IfcInfo->ps8LDname);
				strcpy(matchedKey, leaf_key);
				//printf("leaf_key : %s\n", leaf_key);
				return;
				break;
			}
		}

	//// Disabled Temporary 
	//// printf("%s Does Not Match\n", endName);
	matchedKey[0] = 0;
}

int ModScbr = 0;
int CbAlm = 0;
float bcta = 0.0;
float bctb = 0.0;
float bctc = 0.0;

/**
 * Updates SCBR related mms packets. 
 */
void UpdateScbrVmd(void)
{
	char matchedKey[200];
	int lidx = 0;

	ModScbr++;
	CbAlm++;

	bcta += 1.0;
	bctb += 1.5;
	bctc += 1.7;

	GetKeyStringByEndName(&scbr_leaflist[lidx], "$CbAlm$stVal", matchedKey);
	if(matchedKey != NULL)
	{
		//printf("SCBR - Leaf Name : %s\n", matchedKey);
		writeVmdPointerByLeafName_int32(matchedKey, CbAlm);
	}

	GetKeyStringByEndName(&scbr_leaflist[lidx], "$Mod$stVal", matchedKey);
	if(matchedKey != NULL)
	{
		//printf("SCBR - Leaf Name : %s\n", matchedKey);
		writeVmdPointerByLeafName_int32(matchedKey, ModScbr);
	}

	GetKeyStringByEndName(&scbr_leaflist[lidx], "$MX$BctA$mag$f", matchedKey);
	if(matchedKey != NULL)
	{
		//printf("SCBR - Leaf Name : %s\n", matchedKey);
		//writeVmdPointerByLeafName_float32(matchedKey, cb_file.cb_data.phase_current_A[0]);
		writeVmdPointerByLeafName_float32(matchedKey, bcta);
	}

	GetKeyStringByEndName(&scbr_leaflist[lidx], "$MX$BctB$mag$f", matchedKey);
	if(matchedKey != NULL)
	{
		//printf("SCBR - Leaf Name : %s\n", matchedKey);
		//writeVmdPointerByLeafName_float32(matchedKey, cb_file.cb_data.phase_current_B[0]);
		writeVmdPointerByLeafName_float32(matchedKey, bctb);
	}

	GetKeyStringByEndName(&scbr_leaflist[lidx], "$MX$BctC$mag$f", matchedKey);
	if(matchedKey != NULL)
	{
		//printf("SCBR - Leaf Name : %s\n", matchedKey);
		//writeVmdPointerByLeafName_float32(matchedKey, cb_file.cb_data.phase_current_C[0]);
		writeVmdPointerByLeafName_float32(matchedKey, bctc);
	}

	GetKeyStringByEndName(&scbr_leaflist[lidx], "$EvtTransF$stVal", matchedKey);
	if(matchedKey != NULL)
	{
		//printf("SCBR - Leaf Name : %s\n", matchedKey);
		writeVmdPointerByLeafName_int32(matchedKey, ModScbr);
	}

}

/**
 * Updates SBSH related mms packets. 
 */

void UpdateSbshVmd(void)
{
	char matchedKey[200];
	int lidx = 0;

	for(lidx=0;lidx<6;lidx++)
	{
		GetKeyStringByEndName(&sbsh_leaflist[lidx], "$Mod$stVal", matchedKey);
		if(matchedKey != NULL)
		{
			//printf("Leaf Name : %s\n", matchedKey);
			writeVmdPointerByLeafName_int32(matchedKey, sbsh_sensors[lidx]._Mod);
		}

		GetKeyStringByEndName(&sbsh_leaflist[lidx], "$RTTransF$stVal", matchedKey);
		if(matchedKey != NULL)
		{
			//printf("Leaf Name : %s\n", matchedKey);
			writeVmdPointerByLeafName_int32(matchedKey, 1);
		}

		GetKeyStringByEndName(&sbsh_leaflist[lidx], "$MX$Vol$mag$f", matchedKey);
		if(matchedKey != NULL)
		{
			////printf("Writimg Leaf Name : %s Value: %f\n", matchedKey, sbsh_sensors[lidx]._Vol);
			writeVmdPointerByLeafName_float32(matchedKey, sbsh_sensors[lidx]._Vol);
		}

		GetKeyStringByEndName(&sbsh_leaflist[lidx], "$MX$React$mag$f", matchedKey);
		if(matchedKey != NULL)
		{
			//printf("Leaf Name Vol : %s\n", matchedKey);
			writeVmdPointerByLeafName_float32(matchedKey, sbsh_sensors[lidx]._React);
		}

		GetKeyStringByEndName(&sbsh_leaflist[lidx], "$MX$PF$mag$f", matchedKey);
		if(matchedKey != NULL)
		{
			//printf("Leaf Name Vol : %s\n", matchedKey);
			writeVmdPointerByLeafName_float32(matchedKey, sbsh_sensors[lidx]._PF);
		}

		//printf("-----------------------------------------------------------------------------\n\n");

		GetKeyStringByEndName(&sbsh_leaflist[lidx], "$MX$LeakA$mag$f", matchedKey);
		if(matchedKey != NULL)
		{
			//printf("Leaf Name Vol : %s\n", matchedKey);
			writeVmdPointerByLeafName_float32(matchedKey, sbsh_sensors[lidx]._It);
			//writeVmdPointerByLeafName_float32("Sanion_154kV_GIS_D01_2AMLU/SBSH1$MX$Vol$mag$f", 1);
			//writeVmdPointerByLeafName_float32("Sanion_154kV_GIS_D01_2AMLU/SBSH1$MX$LeakA$mag$f", sbsh_sensors[0]._It);
		}

		GetKeyStringByEndName(&sbsh_leaflist[lidx], "$MX$LosFact$mag$f", matchedKey);
		if(matchedKey != NULL)
		{
			//printf("Leaf Name Vol : %s\n", matchedKey);
			writeVmdPointerByLeafName_float32(matchedKey, sbsh_sensors[lidx]._LosFact);
			//writeVmdPointerByLeafName_float32("Sanion_154kV_GIS_D01_2AMLU/SBSH1$MX$Vol$mag$f", 1);
			//writeVmdPointerByLeafName_float32("Sanion_154kV_GIS_D01_2AMLU/SBSH1$MX$LosFact$mag$f", sbsh_sensors[0]._LosFact);
		}
	}

	if(lidx > 4)
	{
		; // printf("-------- VMD Pointers Updated\n");
	}
}

/**
 * Updates SLTC related mms packets. 
 */

void UpdateOltcVmd(void)
{
	char matchedKey[200];
	int lidx = 0;

	for(lidx=0;lidx<3;lidx++)
	{
		GetKeyStringByEndName(&oltc_leaflist[lidx], "$MX$MotDrvAPhsA$mag$f", matchedKey);
		if(matchedKey != NULL)
		{
			printf("Leaf Name : %s = %f\n", matchedKey, oltc_sensors[lidx]._MotDrvAPhsA);
			writeVmdPointerByLeafName_float32(matchedKey, oltc_sensors[lidx]._MotDrvAPhsA);
			//writeVmdPointerByLeafName_int32("Sanion_154kV_GIS_D01_2AMLU/SBSH1$ST$Mod$stVal", sbsh_sensors[0]._Mod);
		}

		GetKeyStringByEndName(&oltc_leaflist[lidx], "$MX$MotDrvAPhsB$mag$f", matchedKey);
		if(matchedKey != NULL)
		{
			//printf("Leaf Name : %s\n", matchedKey);
			writeVmdPointerByLeafName_float32(matchedKey, oltc_sensors[lidx]._MotDrvAPhsB);
			//writeVmdPointerByLeafName_int32("Sanion_154kV_GIS_D01_2AMLU/SBSH1$ST$Mod$stVal", sbsh_sensors[0]._Mod);
		}

		GetKeyStringByEndName(&oltc_leaflist[lidx], "$MX$MotDrvAPhsC$mag$f", matchedKey);
		if(matchedKey != NULL)
		{
			//printf("Leaf Name : %s\n", matchedKey);
			writeVmdPointerByLeafName_float32(matchedKey, oltc_sensors[lidx]._MotDrvAPhsC);
			//writeVmdPointerByLeafName_int32("Sanion_154kV_GIS_D01_2AMLU/SBSH1$ST$Mod$stVal", sbsh_sensors[0]._Mod);
		}

		printf("OLTC %d : A: %f  B: %f  C: %f \n", lidx, oltc_sensors[lidx]._MotDrvAPhsA, oltc_sensors[lidx]._MotDrvAPhsB, oltc_sensors[lidx]._MotDrvAPhsC);
	}
}

/**
 * It is actually a thread function that calls to another infinite loop function 
 * EnqueuePDData(...) . If EnqueuePDData(...) exits then this function recreates another 
 * TCP connection and start EnqueuePDData(...) again. This process will continue for seamless
 * PDDAU connectivity. 
 */
void* RunPDDAU(void *params)
{
	InitSensor(); // It prepares a data packet for PD Simulator to get data continiously.
	// #define SenPORT 5000
    int sockfd, connfd, iResult, idx, connected_with_sensor=0;
	U32 TmCntMsec = 0; // msec 단위 Tm count
	U32 TmInterval = 10; // msec
	//char *ipaddress = "192.168.246.143";
	char ipaddress[16];
	#ifdef CPP_BLOCK_ENABLED
	//Pddau.parseIPAddressesFromJson("LUConfig.json");
	//strcpy(ipaddress, Pddau.ip_address);
	#else
	strcpy(ipaddress, "192.168.246.143");
	#endif
	strcpy(ipaddress, "192.168.246.143");	
	char matchedKey[200];
	struct in_addr ip_addr;
	ip_addr.s_addr = htonl(g_ds61850SharedMem.dsDeviceInfo.dsNetWorkCfg[2].m_u32IPAddr);
	// printf("PDDAU IP : %s\n", inet_ntoa(ip_addr));	
	// char *ipaddress = inet_ntoa(ip_addr);
	printf("PDDAU IP : %s\n", ipaddress);

	SPECTRUM_PACKET_HEADER *ps8PDhdr = (SPECTRUM_PACKET_HEADER *)malloc(sizeof(SPECTRUM_PACKET_HEADER));
	MSG_PD_FULL_PACKET* pu8fulldata = (MSG_PD_FULL_PACKET *)malloc(sizeof(MSG_PD_FULL_PACKET));
	MSG_PDDAU_INFO_PACKET msg_pddau_info_packet;
	MSG_RF_INFO_PACKET *msg_rf_info_packet = (MSG_RF_INFO_PACKET *)malloc(sizeof(MSG_RF_INFO_PACKET));
	bzero(msg_rf_info_packet, 55);
	
	/*
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) 
	{
		printf("sensor socket creation failed...\n");
		exit(0);
	}
	else
	{
		;//printf("sensor Socket successfully created.. \n");
	}
	bzero(&servaddr, sizeof(servaddr));
	*/

    // socket create and varification
    while(1)
    {
		
		TmCntMsec += TmInterval;

		struct sockaddr_in servaddr, cli;
	    sockfd = socket(AF_INET, SOCK_STREAM, 0);
	    if (sockfd == -1) 
	    {
	        printf("PDDAU-001 : void* RunPDDAU(void *params) : sensor socket creation failed...\n");
	        //exit(0);
	    }
	    else
		{
	        printf("sensor Socket successfully created.. \n");
		}
	    bzero(&servaddr, sizeof(servaddr));
		
	    // assign IP, PORT
	    if(ipaddress)
	    {
		    servaddr.sin_family = AF_INET;
		    servaddr.sin_addr.s_addr = inet_addr(ipaddress);
		    servaddr.sin_port = htons(SenPORT);
		   
		    // connect the client socket to server socket
		    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0)
		    {
				//// Commented Code may be needed later
		        printf("connection with the sensor failed (1)...\n");
				connected_with_sensor = 0;
				sleep(10);
				continue;
		        ////exit(0);
				//return -1;
		    }
		    else
		    {
		    	printf("connected to the sensor (1)..\n");
		    }

			//setAmplification(0, sockfd);

			struct timeval tv;
			tv.tv_sec = 10;        // 30 Secs Timeout
			tv.tv_usec = 0;        // Not init'ing this can cause strange errors
			setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));
			

			getAmplification(0, sockfd, msg_rf_info_packet);
			system("clear");
			printf("Waiting 10 Sec. \n");
			sleep(10);
			printf("Calling setAmplification() \n");
			msg_rf_info_packet->amp[3] = 20;

			setAmplification(0, sockfd, msg_rf_info_packet);

			//while(1)
			{
				resetPDDAU(sockfd, &msg_pddau_info_packet);
				printf("Waiting 5 sec pddau reset \n");
				//printf("Waiting 5 sec \n");
				close(sockfd);
				sleep(5);

					sockfd = socket(AF_INET, SOCK_STREAM, 0);
					if (sockfd == -1) 
					{
						printf("PDDAU-002 : void* RunPDDAU(void *params) : sensor socket creation failed...\n");
						//exit(0);
					}
					else
					{
						;//printf("sensor Socket successfully created.. \n");
					}
					bzero(&servaddr, sizeof(servaddr));

					servaddr.sin_family = AF_INET;
					servaddr.sin_addr.s_addr = inet_addr(ipaddress);
					servaddr.sin_port = htons(SenPORT);
				
				
				if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0)
				{
					//// Commented Code may be needed later
					//// printf("connection with the sensor failed (2)...\n");
					connected_with_sensor = 0;
					sleep(10);
					continue;
					////exit(0);
					//return -1;
				}
				else
				{
					printf("connected to the sensor (2)..\n");
				}

			}

			setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));

		    // function for chat
		    //printf("Function Starts....\n");
			ps8PDhdr->msg_id = MSG_PD_START_ID;
			ps8PDhdr->msg_type = MSG_REQUEST_TYPE;
			ps8PDhdr->body_len = 0;
			
			
			iResult = write(sockfd, ps8PDhdr, sizeof(ps8PDhdr));
            if (iResult == -1)
            {
            	fprintf(stderr, "send Error Occurred %s (%d)\n", strerror(errno), errno);
				continue;
                //return -1;
            }
			iResult = read(sockfd, pu8fulldata, sizeof(pu8fulldata));
			if(iResult == -1)
			{
                fprintf(stderr, "recv Error Occurred %s (%d)\n", strerror(errno), errno);
				continue;
                //return -1;
			}
            if (pu8fulldata->header.msg_type != MSG_REQUEST_ACK_TYPE)
            {
                ; //// printf("PD Start failed\n");
                return (void *)-1;
            }
			
			

			printf("Function Starts....\n");

			//sleep(20);

			//setAmplification(0, sockfd);

			system("clear");
			system("clear");

			/* Following function commented by jibon */
		    EnqueuePDData(sockfd);
			
			////showPdStream(sockfd); // This line is added by jibon

		    printf("\nComm Finished!!!!\n");
			setStopIssued(1);
		    // close the socket
		    close(sockfd);

			sleep(40);
	    }
    }
}

CBEVENT cbevent;
extern CBEVENT cbevnt;


char *event_list[] = {\
    "event_number",\
    "event_type",\
    "event_datetime",\
    "event_millisec",\
    "contact_duty_A",\
    "contact_duty_B",\
    "contact_duty_C",\
    "tc1_peak_curr",\
    "tc1_curr_flow",\
    "tc2_peak_curr",\
    "tc2_curr_flow",\
    "cc_peak_curr",\
    "cc_curr_flow",\
    "contact_op_time_A",\
    "contact_op_time_B",\
    "block_close_time_A",\
    "block_close_time_B",\
    "block_close_time_C"\
};


/**
 * Just a test function to print SCBR data from ACQ (External Device to gather data from Breaker).
 */
void printCBFile(CB_FILE evnt)
{
	printf("\
		coil_max_current_t1:%X\n\
		cc_curr_flow:%f\n\
		cc_curr_flow:%X\n\
		",\
		evnt.coil_max_current_t1,\
		evnt.coil_integral_close,\
		(int32_t)evnt.coil_integral_close);
}

/**
 * Test function to print out CB Event.
 */
void printCBEvent(CBEVENT evnt)
{
	printf("\
		event_number:%d\n\
		event_type:%d\n\
		event_datetime:%d\n\
		event_millisec:%d\n\
		contact_duty_A:%f\n\
		evnt.contact_duty_B:%f\n\
		evnt.contact_duty_C:%f\n\
		tc1_peak_curr:%f\n\
		tc1_curr_flow:%f\n\
		tc2_peak_curr:%f\n\
		tc2_curr_flow:%f\n\
		cc_peak_curr:%f\n\
		cc_curr_flow:%f\n\
		cc_curr_flow:%X\n\
		contact_op_time_A:%f\n\
		contact_op_time_B:%f\n\
		block_close_time_A:%f\n\
		block_close_time_B:%f\n\
		block_close_time_C:%f\n\
		cb_file.coil_integral_t1:%f\n\
		cb_file.coil_integral_t2:%f\n\
		cb_file.coil_integral_close:%f\n\
		",\
		evnt.event_number, \
		evnt.event_type, \
		evnt.event_datetime, \
		evnt.event_millisec, \
		evnt.contact_duty_A,\
		evnt.contact_duty_B,\
		evnt.contact_duty_C,\
		evnt.tc1_peak_curr,\
		evnt.tc1_curr_flow,\
		evnt.tc2_peak_curr,\
		evnt.tc2_curr_flow,\
		evnt.cc_peak_curr,\
		evnt.cc_curr_flow,\
		(int32_t)evnt.cc_curr_flow,\
		evnt.contact_optime_A,\
		evnt.contact_optime_B,\
		evnt.block_close_time_A,\
		evnt.block_close_time_B,\
		evnt.block_close_time_C,\
		cb_file.coil_integral_t1,\
		cb_file.coil_integral_t2,\
		cb_file.coil_integral_close\
		);
}

extern u16 t1_buf[];
extern u16 t2_buf[];
extern CB_FILE cb_file;


extern LEAFLIST sbsh_leaflist[];
int oltc_saving_counter = 0;

/*! \Brief Description void subsystem_thread(void) 
 * This functions previous name was void sbcr_acq_modbus_run(void)
 * Now it is a main thread function to collect all sensor data.
 * Sensor Types SBSH, SCBR, SLTC 
 */
void* subsystem_thread(void *params)
{
#ifdef CPP_BLOCK_ENABLED
	_ACQ Acq;
	// _PDDAU Pddau;
	#ifdef ISPDASYSTEM
	Acq.parseIPAddressesFromJson("LUConfig.json");
	Pddau.parseIPAddressesFromJson("LUConfig.json");
	#endif
#endif

	int flag_recs = 0;
	int records_count = 0;
	int lidxA = 0;
	int lidxB = 1;
	int lidx = 0;

	int flagA = 0;

	int tmm = 0;

	struct tm tvl;
	time_t tme;
	struct timeval tmval;

	time_t now;
	struct tm *tm;

	struct tm *_tm_;
	time_t _now_;

	uint32_t _now = 12345;

	struct tm *time_info;
	char time_string[45];
    char filename[60];
    time(&tme);
    time_info = localtime(&tme);
    strftime(time_string, sizeof(time_string), "%Y%m%d%H%M%S", time_info);

	/*
	if(openAcqBus() == 0)
	{
		tme = readAcqDateTime(&tvl);
		tmval.tv_sec = tme;
		tmval.tv_usec = 100;
		settimeofday(&tmval, NULL);
		closeModbus();
	}
	*/
	struct timeval dsSysTime;	
	gettimeofday(&dsSysTime, NULL);   
	_now_ = dsSysTime.tv_sec;  
	//time(0);   
	_tm_ = localtime(&_now_);    
	now = dsSysTime.tv_sec;; 
	//time(0);	 
	tm = localtime (&now);

    printf(" ==========================================subsystem_thread PID: %lu\n", pthread_self());

	////writeVmdPointerByLeafName_int32("Sanion_154kV_GIS_D01_2AGLU/SCBR1$ST$EvtTransF$stVal", 0);
	printf("Initiating Leaf Search g_p61850IfcInfo->s32leafnum : %d \r\n", g_p61850IfcInfo->s32leafnum);

	if(LU == MLU)
	{
		SearchLeafsX("SBSH", &sbsh_leaflist[0]);
		SearchLeafsX("SPDC", &spdc_leaflist[0]);
		SearchLeafsX("SLTC", &oltc_leaflist[0]);
		SearchLeafsX("SIML", &siml_leaflist[0]);

		PrintLeafs(&sbsh_leaflist[0]);
	}
	else
	{
		SearchLeafsX("SCBR", &scbr_leaflist[0]);
		SearchLeafsX("SPDC", &spdc_leaflist[0]);

		PrintLeafs(&scbr_leaflist[0]);
	}
		
	SbshSensorCollect();
	
	while(1)
	{
		if(openAcqBus(Acq.ip_address) == 0)
		{
			flag_recs = checkRecordFlags();
			////printf("....... FLAG : %04X\n", flag_recs);
			
			if((flag_recs & (0x00FF)) > 0x0000)
			{
				records_count = (flag_recs & (0x00FF));
				for(lidxA=0;lidxA < records_count; lidxA++)
				{
					prepareFileData();
					now = cb_file.event_datetime + (9 * 3600); // (9 * 3600) to match the time zone UTC+9 hours
					gettimeofday(&dsSysTime, NULL);
					now = dsSysTime.tv_sec;
					tm = localtime(&now);
					//time(0);tm = localtime (&now);
					now = dsSysTime.tv_sec;					
					//printSample_int16(cb_file.cb_data.trip1_coil_current);
					save_file(tm);
					printCBEvent(cbevnt);
				}
				gettimeofday(&dsSysTime, NULL);
				now = dsSysTime.tv_sec; //time(0);
				tm = localtime (&now);
				now = dsSysTime.tv_sec;
				_now = dsSysTime.tv_sec;
				readMultiBlock(CBEVENT_BLOCK_SIZE, CBEVENT_NUMBEROFBLOCKS, CBEVENT_BLOCK_OFFSET, (u16 *)&cbevent);
				//printCBEvent(cbevent);
				//printCBFile(cb_file);
				//system("printf \"GET /api/Common/StartDataCollector?luName=YHGLU&ipAddress=192.168.10.111&luType=G&equipmentName=SCBR1 HTTP/1.0\r\n\r\n\" | nc 192.168.10.132 90");
				//system("curl -X GET \"http://192.168.10.132:90/api/Common/StartDataCollector?luName=YHGLU&ipAddress=192.168.10.111&luType=G&equipmentName=SCBR1\" -H  \"accept: */*\"");
				tme = readAcqDateTime(&tvl);
				printf("\ntmm : %d\n", tmm);
				lidxB++;
				printf("BctA : %f \n, BctB : %f \n, BctC : %f \n", cb_file.cb_data.phase_current_A[0], cb_file.cb_data.phase_current_B[0], cb_file.cb_data.phase_current_C[0]);
				UpdateScbrVmd();
			}

			tme = readAcqDateTime(&tvl);
			//printf("now: %d-%02d-%02d %02d:%02d:%02d\n", (tvl.tm_year + 1900), tvl.tm_mon + 1, tvl.tm_mday, tvl.tm_hour, tvl.tm_min, tvl.tm_sec);
			tme = readAcqDateTime(&tvl);
			closeModbus();
			// now = time(NULL);
			// time_info = localtime(&now);
    		// strftime(time_string, sizeof(time_string), "\n\n\n TIME : %Y%m%d%H%M%S ", time_info);
			// printf(time_string);
			sleep(2);
		}
		else
		{
			tme = readDateTime2();
			// lidxB++;
			sleep(1); //was 4
		}

		sleep(1);
		ProcessBushingSensorData();
		CollectOltcDataFromSharedMem(oltc_sensors);
		// UpdateSbshVmd();
#ifdef CPP_BLOCK_ENABLED
		// Acq.printSample(0);
#endif
		if(oltc_saving_counter > 2)
		{
			oltc_saving_counter = 0;
			//// save_oltc_file(_tm_);
		}

		//// request();

		/* For Test Only. Delete Later */
		UpdateScbrVmd();

		if(g_pAISharedMem->m_dsMeasQueueData.u32HFlag == 1)
		{
			printf("--------------------  Flag Detected --------------------------");
			save_oltc_file(_tm_);
			g_pAISharedMem->m_dsMeasQueueData.u32HFlag = 0;
		}

		oltc_saving_counter++;
		//UpdateOltcVmd();
	}
}
