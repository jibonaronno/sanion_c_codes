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

#include "VT100.h"
#include "sys/types.h"
#include "sys/sysinfo.h"
#include "spectrum_packet.h"


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

//modbus_t *ctx;
/*<USER-CODE-END>*/

//Extension
//#define ONE_CYCLE 256
#define ONE_CYCLE 128

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

void initQueue(Queue* queue) {
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    pthread_mutex_init(&queue->mutex, NULL);
    //pthread_mutex_init(&queue->mutexCali, NULL);	
}

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
/*
void enqueueCali(Queue* queue, unsigned char* data) {
    pthread_mutex_lock(&queue->mutexCali);
    Node* node = (Node*) malloc(sizeof(Node));
    memcpy(node->caliData, data, sizeof(node->caliData));
    node->next = NULL;
    if (queue->size == 0) {
        queue->head = node;
    } else {
        queue->tail->next = node;
    }
    queue->tail = node;
    queue->size++;
    pthread_mutex_unlock(&queue->mutexCali);
}
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
    ////free(oldHead);
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

/*
unsigned char* dequeueCali(Queue* queue) {
	printf("dequeueCali >>>\n");
    pthread_mutex_lock(&queue->mutexCali);
	printf("Here 1\n");
    if (queue->size == 0) {
        pthread_mutex_unlock(&queue->mutexCali);
        return NULL;
    }
    Node* oldHead = queue->head;
    unsigned char* data = (unsigned char*) malloc(sizeof(unsigned char) * 128);
	printf("Here 2\n");	
    memcpy(data, oldHead->caliData, 128);
    queue->head = oldHead->next;
    //free(oldHead->caliData);
    free(oldHead);
    queue->size--;
	printf("Here 3\n");	
    pthread_mutex_unlock(&queue->mutexCali);
    return data;
}
*/

int isEmpty(Queue* queue) {
    pthread_mutex_lock(&queue->mutex);
    int empty = (queue->size == 0);
    pthread_mutex_unlock(&queue->mutex);
    return empty;
}

int getSize(Queue* queue) {
    pthread_mutex_lock(&queue->mutex);
    int size = queue->size;
    pthread_mutex_unlock(&queue->mutex);
    return size;
}
/*
int getSizeCali(Queue* queue) {
    pthread_mutex_lock(&queue->mutexCali);
    int size = queue->size;
	printf("size : %d\n", size);
    pthread_mutex_unlock(&queue->mutexCali);
    return size;
}
*/

Queue *gpch1connectionQ;
Queue *gpch2connectionQ;
Queue *gpch3connectionQ;
Queue *gpch4connectionQ;

Queue *gpch1connectionCQ;
Queue *gpch2connectionCQ;
Queue *gpch3connectionCQ;
Queue *gpch4connectionCQ;

/************
It is preparing a data packet to send to the PD Simulator. So PD simulator starts 
sending data over the connected TCP connection continiously.
*************/
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

int request(void) {
    char buffer[BUFSIZ];
    enum CONSTEXPR { MAX_REQUEST_LEN = 1024};
    char request[MAX_REQUEST_LEN];
    //char request_template[] = "GET /api/Common/StartDataCollector?luName=YHGLU&ipAddress=192.168.10.111&luType=G&equipmentName=SCBR1 HTTP/1.1\r\nHost: %s\r\n\r\n";
    char request_template[] = "GET /api/Common/StartDataCollector?luName=YHGLU&ipAddress=192.168.247.104&luType=G&equipmentName=SCBR1 HTTP/1.0\r\n\r\n";
	struct protoent *protoent;
    char *hostname = "192.168.10.132";
    in_addr_t in_addr;
    int request_len;
    int socket_file_descriptor;
    ssize_t nbytes_total, nbytes_last;
    struct hostent *hostent;
    struct sockaddr_in sockaddr_in;
    unsigned short server_port = 90;

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
    in_addr = inet_addr("192.168.10.132");

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
    fprintf(stderr, "debug: before first read\n");
    while ((nbytes_total = read(socket_file_descriptor, buffer, BUFSIZ)) > 0) {
        fprintf(stderr, "debug: after a read\n");
        //write(STDOUT_FILENO, buffer, nbytes_total);
		printf("%s", buffer);
    }
    fprintf(stderr, "debug: after last read\n");
    if (nbytes_total == -1) {
        perror("read");
        //exit(EXIT_FAILURE);
		return -1;
    }

    close(socket_file_descriptor);

	return 0;
}

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


void saveIntoRamFs(char *filename)
{
	FILE *file;
	//char filename[60];
	
	//sprintf(filename, "/COMTRADE/1_%s.dat", time_string);
	printf("Filename: %s\n", filename);		
	file = fopen(filename, "wb");
	////fwrite(&p8data, sizeof(unsigned char), 128, file);
	
}

#define DEV_FACTOR	230.0 // 279.0 before
#define OFFSET_FACTOR 71.91

#define SVFILE1() sprintf(filename, "/COMTRADE/1_%s_1.dat", time_string)
#define SVFILE2() sprintf(filename, "/COMTRADE/2_%s_1.dat", time_string)
#define SVFILE3() sprintf(filename, "/COMTRADE/3_%s_1.dat", time_string)
#define SVFILE4() sprintf(filename, "/COMTRADE/4_%s_1.dat", time_string)

void CalibratePDdatthread()
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

	//Extension
	//strftime(time_string, sizeof(time_string), "%Y%m%d%H%M%S", time_info);

	while(1)
	{

		time(&current_time);
    	time_info = localtime(&current_time);
    	strftime(time_string, sizeof(time_string), "%Y%m%d%H%M%S", time_info);

		if (getSize(gpch1connectionQ) > 0 )
		{
			ps32data = dequeue(gpch1connectionQ);
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
					
					ch1mem[ch1_counter][i/2] = (int)f32dataA;
				}
			}			
			MSG_PD_BODY dstdata;
			dstdata.ch_idx = 1;
			//memcpy(dstdata.data, ps32CaliData, ONE_CYCLE);
			//enqueueCali(gpch1connectionCQ, ps32CaliData);
			//enqueue(gpch1connectionCQ, dstdata);
			
			f32dataA1 = f32dataA;
			//VT100_goto(1, 12);
			//printf("CH1 - %f", f32dataA);

			ch1_counter++;
			if(ch1_counter >= 60)
			{
				FILE *file;
				SVFILE1();
				sprintf(line02, "New Filename: %s  \n", filename);
				file = fopen(filename, "wb");
				fwrite(ch1mem, sizeof(unsigned char), 7680, file);
				fclose(file);
				ch1_counter = 0;
			}
			
		}
		
		if (getSize(gpch2connectionQ) > 0 )
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
				sprintf(line03, "New Filename: %s  \n", filename);
				file = fopen(filename, "wb");
				fwrite(ch2mem, sizeof(unsigned char), 7680, file);
				fclose(file);
				ch2_counter = 0;
			}
		}	
		
		if (getSize(gpch3connectionQ) > 0 )
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
				sprintf(line04, "New Filename: %s  \n", filename);
				file = fopen(filename, "wb");
				fwrite(ch3mem, sizeof(unsigned char), 7680, file);
				fclose(file);
				ch3_counter = 0;
			}
		}	

		if (getSize(gpch4connectionQ) > 0 )
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
			
			sprintf(line01, "size of CQ1 : %d, CQ2 : %d, CQ3 : %d, CQ4 : %d\n",getSize(gpch1connectionCQ), getSize(gpch2connectionCQ), getSize(gpch3connectionCQ), getSize(gpch4connectionCQ));

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

		////sleep(0.3);
	}
}

void CreatePDdatthread()
{
	time_t current_time;
	struct tm *time_info;
	char time_string[45];
	//FILE *file;
	volatile int flagex = 0;
	int ch_flag = 0;
	char try_data[66] = {0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5};
	bool halfSecond = FALSE;
	//VT100_scrclr();

	

	while(1)
	{
    	time(&current_time);
    	time_info = localtime(&current_time);
    	strftime(time_string, sizeof(time_string), "%Y%m%d%H%M%S", time_info);

    	if(flagex == 0)
    	{
    		flagex = 1;
    		printf("Current time: %s\n", time_string);
    	}

		//VT100_scrclr();
		//VT100_goto(1,5);

    	char filename[60];
		////if (getSize(gpch1connectionCQ) >= 60)
		if(ch_flag == 0)
		{	
			ch_flag = 1;
			if (getSize(gpch1connectionCQ) >= 30)
			{
				FILE *file;
				//char filename[50];
		   	 	sprintf(filename, "/COMTRADE/1_%s.dat", time_string);
		   	 	if(halfSecond)
					SVFILE1();
				else
			   	 	SVFILE2();
				//VT100_goto(1,5);
		    	sprintf(line02, "Filename: %s  \n", filename);	
				file = fopen(filename, "wb");

			    // Check if the file was opened successfully
			    
			    if (file == NULL) 
			    {
			        printf("Error opening file!\n");
			        return 1;
			    }
			    
		    	unsigned char* p8data;
			    // Dequeue and write data to the file for 60 times
			    for (int i = 0; i < 30; i++) 
				{
					////printf("%d  \n",i);
					////printf("DQ -> \n");
			        ////unsigned char* p8data = dequeue(&gpch1connectionCQ);
			        p8data = dequeue(&gpch1connectionCQ);
			        fwrite(&p8data, sizeof(unsigned char), ONE_CYCLE, file);
			        ////printf("DQ ?\n");
			        //sleep(0.1);
					////free(p8data);
					////delete p8data;
					////printf("fwrite Done\n");
			    }
				//////printf("Here 0\n");
			    // Close the file
			    fclose(file);
				//////printf("Here 1\n");
			}
		}
		
		if(ch_flag == 1)
		{

			ch_flag = 2;
			if (getSize(gpch2connectionCQ) >= 30)
			{
				FILE *file;
		   	 	if(halfSecond)
					SVFILE1();
				else
			   	 	SVFILE2();
				//VT100_goto(1,6);
		    	sprintf(line03, "Filename: %s  \n", filename);
				file = fopen(filename, "wb");

			    // Check if the file was opened successfully
			    if (file == NULL) {
			        printf("Error opening file!\n");
			        return 1;
			    }

			    // Dequeue and write data to the file for 60 times
			    unsigned char* p8data;
			    for (int i = 0; i < 30; i++) {
					////printf("%d  \n",i);
			        p8data = dequeue(&gpch2connectionCQ);				
			        fwrite(&p8data, sizeof(unsigned char), ONE_CYCLE, file);
			        //sleep(0.5);
					////free(p8data);								
					////printf("fwrite Done");
			    }
			    // Close the file
			    fclose(file);			
			}
		}

		if(ch_flag == 2)
		{
			ch_flag = 0;
			if (getSize(gpch3connectionCQ) >= 30)
			{
				FILE *file;
		   	 	if(halfSecond)
					SVFILE1();
				else
			   	 	SVFILE2();
				//VT100_goto(1,7);
		    	sprintf(line04, "Filename: %s  \n", filename);		
				file = fopen(filename, "wb");

			    // Check if the file was opened successfully
			    if (file == NULL) {
			        printf("Error opening file!\n");
					
			        return 1;
			    }

			    // Dequeue and write data to the file for 60 times
			    unsigned char* p8data;
			    for (int i = 0; i < 30; i++) 
			    {
					////printf("%d  \n",i);
			        p8data = dequeue(&gpch3connectionCQ);
			        fwrite(&p8data, sizeof(unsigned char), ONE_CYCLE, file);
					////free(p8data);								
					////printf("fwrite Done");
			    }
			    // Close the file
			    fclose(file);			
			}
		}
				
		if(ch_flag == 3)
		{
			ch_flag = 0;
			if (getSize(gpch4connectionCQ) >= 30)
			{
				FILE *file;
				char filenameX[60];
		   	 	if(halfSecond)
					SVFILE1();
				else
			   	 	SVFILE2();
				//VT100_goto(1,8);
		    	sprintf(line05, "Filename: %s  \n", filename);
				file = fopen(filename, "wb");

			    // Check if the file was opened successfully
			    if (file == NULL) {
			        printf("Error opening file!\n");
			        return 1;
			    }

			    // Dequeue and write data to the file for 60 times
			    unsigned char* p8data;
			    for (int i = 0; i < 30; i++) 
			    {
					////printf("%d  \n",i);
			        ////p8data = dequeue(&gpch4connectionCQ);
			        
			        fwrite(&try_data, sizeof(unsigned char), ONE_CYCLE, file);

					////free(p8data);								
					////printf("fwrite Done");
			    }
			    // Close the file
			    fclose(file);
			}
		}
		halfSecond = !halfSecond;
		
		//VT100_goto(1,10);
		//printf("%s \n", line01);

		sleep(1.5);
	}
}

int ch1_count = 0;
int ch2_count = 0;
int ch3_count = 0;
int ch4_count = 0;

void SaveFiles(void)
{

}


void rmvfunc(){//////////////////Not Using///////////////////////////
	while(1){
		struct dirent *de;  // Pointer for directory entry
  
	    // opendir() returns a pointer of DIR type. 
	    DIR *dr = opendir("/data/COMTRADE");
	  
	    if (dr == NULL)  // opendir returns NULL if couldn't open directory
	    {
	        printf("Could not open current directory" );
	        return 0;
	    }
	  	usleep(500);
	    // Refer http://pubs.opengroup.org/onlinepubs/7990989775/xsh/readdir.html
	    // for readdir()
	    while ((de = readdir(dr)) != NULL){
	            printf("%s\n", de->d_name);
				//int status = remove(de->d_name);
			   // if( status != 0 ){
			   //    printf("Unable to delete the file\n");
			   //    perror("Error");
			   //    }
	    	}
	  
	    closedir(dr);    
		free(de);
		}
}

/*************************************************************************/
/*<USER_CODE_START> <description> This function will employ a while loop to receive data
from PD over ethernet 192.168.10.142:5000. It sends a command string first, then starts 
receiving data. Data format.... </description> */
void showPdStream(int sockfd){

	//Extension
	//ret = pthread_attr_setstacksize(&tattr, size);

	MSG_PD_Tot recvdata;
	memset(&recvdata, '\0', sizeof(MSG_PD_Tot));
	U8 processed[1024];
	U8 totdata [SensorMAX];
	U32 count = 0;
	pthread_t FileThread, CaliThread; 
	MSG_PD_FULL_PACKET *pu8fulldata = malloc(sizeof(MSG_PD_FULL_PACKET));

	gpch1connectionQ = malloc(sizeof(Queue));
	if (gpch1connectionQ == NULL) {
		fprintf(stderr, "Error: failed to allocate memory for gpch1connectionQ\n");
		exit(EXIT_FAILURE);
	}
	
	gpch2connectionQ = malloc(sizeof(Queue));
	if (gpch2connectionQ == NULL) {
		fprintf(stderr, "Error: failed to allocate memory for gpch2connectionQ\n");
		exit(EXIT_FAILURE);
	}
	
	gpch3connectionQ = malloc(sizeof(Queue));
	if (gpch3connectionQ == NULL) {
		fprintf(stderr, "Error: failed to allocate memory for gpch3connectionQ\n");
		exit(EXIT_FAILURE);
	}
	
	gpch4connectionQ = malloc(sizeof(Queue));
	if (gpch4connectionQ == NULL) {
		fprintf(stderr, "Error: failed to allocate memory for gpch4connectionQ\n");
		exit(EXIT_FAILURE);
	}

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


	initQueue(gpch1connectionQ);
	initQueue(gpch2connectionQ);	
	initQueue(gpch3connectionQ);	
	initQueue(gpch4connectionQ);	
	initQueue(gpch1connectionCQ);
	initQueue(gpch2connectionCQ);	
	initQueue(gpch3connectionCQ);	
	initQueue(gpch4connectionCQ);


	
	//pthread_t rmthread;
	//pthread_create(&CaliThread, NULL, CalibratePDdatthread, NULL);
	//pthread_create ( &FileThread, NULL, CreatePDdatthread, NULL );
	//pthread_create ( &rmthread, NULL, rmvfunc, NULL );
	int dbm = 0;
	float fdbm = 0;
	U32 iResult;
	while(1){
		iResult = read(sockfd, pu8fulldata, sizeof(MSG_PD_FULL_PACKET));
		
        if (iResult == -1)
        {
        	fprintf(stderr, "recv Error Occurred %s (%d)\n", strerror(errno), errno);
            return -1;
        }
		
		if(pu8fulldata->header.msg_type != MSG_SEND_TYPE || pu8fulldata->header.msg_id != MSG_PD_SEND_ID)
        {
            printf("PD Sending failed. type : %x, id : %x\n", pu8fulldata->header.msg_type, pu8fulldata->header.msg_id);
            return -1;
        } 

		for(int i = 0; i < 4; i++)
		{
			pu8fulldata->data[i].ch_idx = i+1;
			if(i == 0)
			{
				//enqueue(gpch1connectionQ, pu8fulldata->data[i]);
				VT100_goto(1, 12);
				dbm = pu8fulldata->data[i].data[0];
				dbm <<= 8;
				dbm |= pu8fulldata->data[i].data[1];
				fdbm = (float)dbm;
				fdbm = (fdbm * (5.0/279.0)) - 64.3;
				//printf(" %d ", pu8fulldata->data[i].data[0]);
				printf(" %f ", fdbm);
			}
			else if(i == 1)
			{
				//enqueue(gpch2connectionQ, pu8fulldata->data[i]);
				VT100_goto(10, 12);
				dbm = pu8fulldata->data[i].data[0];
				dbm <<= 8;
				dbm |= pu8fulldata->data[i].data[1];
				fdbm = (float)dbm;
				fdbm = (fdbm * (5.0/279.0)) - 64.3;
				//printf(" %d ", pu8fulldata->data[i].data[0]);
				printf(" %f ", fdbm);
			}
			else if(i == 2)
			{
				//enqueue(gpch3connectionQ, pu8fulldata->data[i]);
				VT100_goto(20, 12);
				dbm = pu8fulldata->data[i].data[0];
				dbm <<= 8;
				dbm |= pu8fulldata->data[i].data[1];
				fdbm = (float)dbm;
				fdbm = (fdbm * (5.0/279.0)) - 64.3;
				//printf(" %d ", pu8fulldata->data[i].data[0]);
				printf(" %f ", fdbm);
			}
			else if(i == 3)
			{
				enqueue(gpch4connectionQ, pu8fulldata->data[i]);
				VT100_goto(30, 12);
				dbm = pu8fulldata->data[i].data[0];
				dbm <<= 8;
				fdbm = (float)dbm;
				fdbm = (fdbm * (5.0/279.0)) - 64.3;
				//printf(" %d ", pu8fulldata->data[i].data[0]);
				printf(" %f ", fdbm);
			}
		}
		
		//usleep(1);
	}
}

/*************************************************************************/
/*<USER_CODE_END>*/
/*************************************************************************/

void sensorfunc(int sockfd){

	//Extension
	//ret = pthread_attr_setstacksize(&tattr, size);

	MSG_PD_Tot recvdata;
	memset(&recvdata, '\0', sizeof(MSG_PD_Tot));
	U8 processed[1024];
	U8 totdata [SensorMAX];
	U32 count = 0;
	pthread_t FileThread, CaliThread; 
	MSG_PD_FULL_PACKET *pu8fulldata = malloc(sizeof(MSG_PD_FULL_PACKET));
	gpch1connectionQ = malloc(sizeof(Queue));
	if (gpch1connectionQ == NULL) {
		fprintf(stderr, "Error: failed to allocate memory for gpch1connectionQ\n");
		exit(EXIT_FAILURE);
	}
	
	gpch2connectionQ = malloc(sizeof(Queue));
	if (gpch2connectionQ == NULL) {
		fprintf(stderr, "Error: failed to allocate memory for gpch2connectionQ\n");
		exit(EXIT_FAILURE);
	}
	
	gpch3connectionQ = malloc(sizeof(Queue));
	if (gpch3connectionQ == NULL) {
		fprintf(stderr, "Error: failed to allocate memory for gpch3connectionQ\n");
		exit(EXIT_FAILURE);
	}
	
	gpch4connectionQ = malloc(sizeof(Queue));
	if (gpch4connectionQ == NULL) {
		fprintf(stderr, "Error: failed to allocate memory for gpch4connectionQ\n");
		exit(EXIT_FAILURE);
	}

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

	initQueue(gpch1connectionQ);
	initQueue(gpch2connectionQ);	
	initQueue(gpch3connectionQ);	
	initQueue(gpch4connectionQ);	
	initQueue(gpch1connectionCQ);
	initQueue(gpch2connectionCQ);	
	initQueue(gpch3connectionCQ);	
	initQueue(gpch4connectionCQ);


	//connect_tcp(ACQ_ADDR,ACQ_PORT);
	
	//pthread_t rmthread;
	pthread_create(&CaliThread, NULL, CalibratePDdatthread, NULL);
	
	////pthread_create ( &FileThread, NULL, CreatePDdatthread, NULL );
	
	//pthread_create ( &rmthread, NULL, rmvfunc, NULL );
	U32 iResult;
	while(1){
		iResult = read(sockfd, pu8fulldata, sizeof(MSG_PD_FULL_PACKET));
		
        if (iResult == -1)
        {
        	fprintf(stderr, "recv Error Occurred %s (%d)\n", strerror(errno), errno);
            return -1;
        }
		
		if(pu8fulldata->header.msg_type != MSG_SEND_TYPE || pu8fulldata->header.msg_id != MSG_PD_SEND_ID)
        {
            printf("PD Sending failed. type : %x, id : %x\n", pu8fulldata->header.msg_type, pu8fulldata->header.msg_id);
            return -1;
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
		//sleep(2);
		//printf(".");
	}
}

int enablePDRFProperties(int socket_fd)
{

	return -0;
}

void sensorserverrun()
{
	InitSensor(); // It prepares a data packet for PD Simulator to get data continiously.
	// #define SenPORT 5000
    int sockfd, connfd, iResult, connected_with_sensor=0;
	//char *ipaddress = "192.168.10.142";
	char *ipaddress = "192.168.247.142";
	SPECTRUM_PACKET_HEADER *ps8PDhdr = malloc(sizeof(SPECTRUM_PACKET_HEADER));
	MSG_PD_FULL_PACKET* pu8fulldata = malloc(sizeof(MSG_PD_FULL_PACKET));
	MSG_PDDAU_INFO_PACKET msg_pddau_info_packet;
	MSG_RF_INFO_PACKET *msg_rf_info_packet = malloc(sizeof(MSG_RF_INFO_PACKET));
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
		
		struct sockaddr_in servaddr, cli;
	    sockfd = socket(AF_INET, SOCK_STREAM, 0);
	    if (sockfd == -1) 
	    {
	        printf("sensor socket creation failed...\n");
	        exit(0);
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
						printf("sensor socket creation failed...\n");
						exit(0);
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
					printf("connection with the sensor failed (2)...\n");
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
                printf("PD Start failed\n");
                return -1;
            }
			
			

			printf("Function Starts....\n");

			//sleep(20);

			//setAmplification(0, sockfd);

			system("clear");
			system("clear");

			/* Following function commented by jibon */
		    sensorfunc(sockfd);
			
			////showPdStream(sockfd); // This line is added by jibon

		    printf("\nComm Finished!!!!\n");
		    // close the socket
		    close(sockfd);

			sleep(40);
	    }
    }
}

CBEVENT cbevent;
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
		evnt.block_close_time_C\
		);
}

extern u16 t1_buf[];
extern u16 t2_buf[];
extern CB_FILE cb_file;

void updateCiscoSharedMemory(CB_FILE cb_file)
{
	writeVmdPointerByLeafName_int32("Sanion_154kV_GIS_D01_2AGLU/SCBR1$MX$ClsColA$mag$f", cb_file.coil_max_current_close);
	writeVmdPointerByLeafName_int32("Sanion_154kV_GIS_D01_2AGLU/SCBR1$MX$BctA$mag$f", cb_file.cb_data.phase_current_A[0]);
}

void sbcr_acq_modbus_run(void)
{
	int flag_recs = 0;
	int records_count = 0;
	int lidxA = 0;
	int lidxB = 1;

	int flagA = 0;

	int tmm = 0;

	struct tm tvl;
	time_t tme;
	struct timeval tmval;

	time_t now;
	struct tm *tm;

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


    now = time(0);
    tm = localtime (&now);

	writeVmdPointerByLeafName_int32("Sanion_154kV_GIS_D01_2AGLU/SCBR1$ST$EvtTransF$stVal", 0);
	
	sleep(3);

	while(1)
	{
		if(openAcqBus() == 0)
		{
			flag_recs = checkRecordFlags();
			////printf("....... FLAG : %04X\n", flag_recs);
			if((flag_recs & (0x00FF)) > 0x0000)
			{
				records_count = (flag_recs & (0x00FF));

				//now = time(0);
				
				// _now = cb_file.event_datetime;
				// printf("Local Time Epoch : %d \n", now);
				// printf("Epoch Time From ACQ : %d \n", cb_file.event_datetime);
				// printf("tm->tm_sec %d\n", tm->tm_sec);
				// printf("cb_file.event_datetime _now (1) : %d \n", _now);

    			

				// printf("tm->tm_sec %d\n", tm->tm_sec);
				// printf("tm->tm_hour %d\n", tm->tm_hour);
				// printf("cb_file.event_datetime _now (2) : %d \n", _now);

				for(lidxA=0;lidxA < records_count; lidxA++)
				{
					// if(readAll() == 0)
					// {
					// 	;
					// }
					prepareFileData();
					now = cb_file.event_datetime + (9 * 3600);
					tm = localtime(&now);
					//printSample_int16(cb_file.cb_data.trip1_coil_current);
					save_file(tm);
				}

				now = (cb_file.event_datetime + ((9 * 3600)));
				_now = (cb_file.event_datetime + (9 * 3600));
				/* Following Codes to test EPOCH TIMES */
				/*
				printf("Local Time Epoch : %d \n", now);
				printf("Epoch Time From ACQ : %d \n", cb_file.event_datetime);
				printf("tm->tm_sec %d\n", tm->tm_sec);
				printf("cb_file.event_datetime _now (1) : %d \n", _now);
				*/

    			// tm = localtime(&now);
				// printf("tm->tm_sec %d\n", tm->tm_sec);
				// printf("tm->tm_hour %d\n", tm->tm_hour);
				// printf("cb_file.event_datetime _now (2) : %d \n", _now);

				//printSample_int16(t1_buf);
				//printSample_int16(cb_file.cb_data.trip1_coil_current);

				readMultiBlock(34, 1, 1200, (u16 *)&cbevent);
				//printCBEvent(cbevent);
				printCBFile(cb_file);

				//request();

				//system("printf \"GET /api/Common/StartDataCollector?luName=YHGLU&ipAddress=192.168.10.111&luType=G&equipmentName=SCBR1 HTTP/1.0\r\n\r\n\" | nc 192.168.10.132 90");
				//system("curl -X GET \"http://192.168.10.132:90/api/Common/StartDataCollector?luName=YHGLU&ipAddress=192.168.10.111&luType=G&equipmentName=SCBR1\" -H  \"accept: */*\"");

				tme = readAcqDateTime(&tvl);
				printf("\ntmm : %d\n", tmm);
				//writeVmdPointerByLeafName_time("YHGLU/SCBR1$ST$EvtTransF$t", tme); //tmm);

				lidxB++;

				writeVmdPointerByLeafName_int32("Sanion_154kV_GIS_D01_2AGLU/SCBR1$ST$EvtTransF$stVal", lidxB); //lidxB);

				writeVmdPointerByLeafName_int32("Sanion_154kV_GIS_D01_2AGLU/SCBR1$ST$EvtTransF$t", now); //lidxB);

				writeVmdPointerByLeafName_float32("Sanion_154kV_GIS_D01_2AGLU/SCBR1$MX$BctA$mag$f", cb_file.cb_data.phase_current_A[0]); //lidxB);
				writeVmdPointerByLeafName_float32("Sanion_154kV_GIS_D01_2AGLU/SCBR1$MX$BctB$mag$f", cb_file.cb_data.phase_current_B[0]);
				writeVmdPointerByLeafName_float32("Sanion_154kV_GIS_D01_2AGLU/SCBR1$MX$BctC$mag$f", cb_file.cb_data.phase_current_C[0]);

				writeVmdPointerByLeafName_int32("Sanion_154kV_GIS_D01_2AGLU/SCBR2$ST$EvtTransF$stVal", lidxB + 1000); //lidxB);

				writeVmdPointerByLeafName_int32("Sanion_154kV_GIS_D01_2AGLU/SCBR2$ST$EvtTransF$t", now); //lidxB);

				writeVmdPointerByLeafName_int32("Sanion_154kV_GIS_D01_2AGLU/SCBR3$ST$EvtTransF$stVal", lidxB + 2000); //lidxB);

				writeVmdPointerByLeafName_int32("Sanion_154kV_GIS_D01_2AGLU/SCBR3$ST$EvtTransF$t", now); //lidxB);

				writeVmdPointerByLeafName_int32("Sanion_154kV_GIS_D01_2AGLU/LPHD1$ST$DevTime$stVal", lidxB);

				printf("BctA : %f \n, BctB : %f \n, BctC : %f \n", cb_file.cb_data.phase_current_A[0], cb_file.cb_data.phase_current_B[0], cb_file.cb_data.phase_current_C[0]);
				
				
				writeVmd_cbAlm();
			
			}

			tme = readAcqDateTime(&tvl);
			//printf("now: %d-%02d-%02d %02d:%02d:%02d\n", (tvl.tm_year + 1900), tvl.tm_mon + 1, tvl.tm_mday, tvl.tm_hour, tvl.tm_min, tvl.tm_sec);

			tme = readAcqDateTime(&tvl);
			//printf("\ntmm : %d\n", tmm);
			//writeVmdPointerByLeafName_time("YHGLU/SCBR1$ST$EvtTransF$t", tme);
			//////VT100_goto(1, 14);
			////notifyVmdScbr();
			////writeVmd_BCTA();

			closeModbus();

			//writeVmdPointerByLeafName_float32("YHGLU/SCBR1$MX$BctA$mag$f", )
			//printf("ACQ Time : %s\n", asctime(&tvl));

			// now = time(NULL);
			// time_info = localtime(&now);
    		// strftime(time_string, sizeof(time_string), "\n\n\n TIME : %Y%m%d%H%M%S ", time_info);
			// printf(time_string);

			// if(flagA == 0){
			// 	writeVmdPointerByLeafName_int32("Sanion_154kV_GIS_D01_2AGLU/SCBR1$ST$EvtTransF$stVal", 0); //lidxB);
			// 	flagA = 1;
			// }
			// else
			// {
			// 	writeVmdPointerByLeafName_int32("Sanion_154kV_GIS_D01_2AGLU/SCBR1$ST$EvtTransF$stVal", 0); //lidxB);
			// 	flagA = 0;
			// }

			//lidxB++;

			sleep(2);
		}
		else
		{

			tme = readDateTime2();
			//writeVmdPointerByLeafName_time("Sanion_154kV_GIS_D01_1AGLU/SCBR1$ST$EvtTransF$t", tme);
			// if(flagA == 0)
			// {
			// 	writeVmdPointerByLeafName_time("Sanion_154kV_GIS_D01_2AGLU/SCBR1$ST$EvtTransF$stVal", lidxB);
			// 	flagA = 1;
			// }
			// else
			// {
			// 	writeVmdPointerByLeafName_time("Sanion_154kV_GIS_D01_2AGLU/SCBR1$ST$EvtTransF$stVal", lidxB);
			// 	flagA = 0;
			// }

			// lidxB++;

			sleep(4);
		}
	}
}
