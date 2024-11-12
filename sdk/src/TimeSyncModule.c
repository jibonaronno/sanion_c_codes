

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <netdb.h>  
#include <signal.h>   
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>   
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <ti/cmem.h>
#include <netinet/in.h>   
#include <netinet/tcp.h>   
#include <arpa/inet.h>   
#include <errno.h>
#include <fcntl.h>

#include "UR_SDK.h"
#include "global.h"
#include "DBManager.h"
#include "ConsoleManager.h"
#include "TimeSyncModule.h"

/*
 * Tick Count
 */

static struct timeval   s_dsDspTimeStamp;
static U32              s_u32DspTickStamp;
struct tm CurTm;

int sntp_client(U32	m_u32IPAddr)
{
	struct sockaddr_in saddr, myaddr;
	struct timeval tv, tp;
	int sntp_fd=-1;
	int len;
	unsigned int pkt_len;
	fd_set readfds;
	U32 u32NowIPAddr=0;
	
	char hostname[16];
	
	u32NowIPAddr = ntohl(m_u32IPAddr);
	inet_ntop( AF_INET, (void*)&u32NowIPAddr, (char *)hostname, sizeof(hostname));	

	// sntp ip 가 0.0.0.0 즉, 설정되지 않으면 시각동기를 하지 않는다.
	if(!strcmp(hostname, "0.0.0.0"))
	{
		printf("host ip error - 0.0.0.0\n");
		return -1;
	}

	
	// sntp 서버 ip 가져오기
	if((len = strlen(hostname)) <= 0)
	{
		printf("sntp : get host name error\n");
		return -1;
	}

  
	// socket 생성
	bzero((char *)&saddr, sizeof(saddr)); 
	saddr.sin_family = AF_INET;
	saddr.sin_port   = htons(NTP_PORT);
	saddr.sin_addr.s_addr = inet_addr(hostname);

	if((sntp_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("sntp make socket error\n");
		return(sntp_fd);
	}
  
	bzero((char *)&myaddr, sizeof(myaddr)); 
	myaddr.sin_family = AF_INET;
	myaddr.sin_port = htons(3030); 
	myaddr.sin_addr.s_addr = INADDR_ANY;
  
	if(bind(sntp_fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) 
	{
		printf("sntp bind socket error\n");
		
		close(sntp_fd);
		return -1;
	}

	/* Fill in the request packet */
    bzero((char *)&pkt, sizeof pkt);


	// 서우현 박사에 의하면 client에서 txtime을 0으로 셋팅하면 sntp 규격에 어긋나므로 
	// IED의 현재 시간을 넣는다. 
	gettimeofday(&tp, NULL);
	pkt.txtime[0] = htonl(tp.tv_sec + JAN_1970);
	pkt.txtime[1] = htonl(tp.tv_usec * 4295);			
	
    pkt.livnmode = 0x1b;        /* version 1, mode 3 (client) */

 
	// send request packet to server
	if((len=sendto(sntp_fd, (char *)&pkt, sizeof(pkt), 0, 
                    (struct sockaddr *)&saddr, sizeof(saddr))) != sizeof(pkt))
	{
		printf("sntp service sendto() error\n");
		
		close(sntp_fd);
		return -1; 
	}
  
	//read reply packet from server
	pkt_len = sizeof(pkt);


	// montavistadpsms FD_ZERO가 꼭 있어야 한다. 
	FD_ZERO(&readfds);
	FD_SET(sntp_fd, &readfds);  // FD_SET 메크로 사용
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	// Using select to wait for data to be available on the socket
	int res;
	if (res = select(sntp_fd + 1, &readfds, NULL, NULL, &tv) > 0) {
		// Data is available; proceed to read
		if ((len = recvfrom(sntp_fd, (char *)&pkt, sizeof(pkt), 0, (struct sockaddr *)&saddr, &pkt_len)) != sizeof(pkt)) {
			perror("Failed to receive the expected amount of data");  // More detailed error
			close(sntp_fd);
			return -1;
		}
	} else {
		// Checking why select failed: was it a timeout or a socket error?
		if (errno == EINTR || errno == EAGAIN) {
			printf("SNTP select call was interrupted or timed out\n");
		} else {
			perror("SNTP select call failed with an error ");
			printf("Errno : %d, result : %d\n", errno, res);
		}
		close(sntp_fd);
		return -1;
	}


	if((pkt.livnmode & TWO_BIT_MASK) == TWO_BIT_MASK)
	{
		printf("Clock Not Synchronized\n");
		gs32SntpTimeQuality = SNTP_CLOCK_FAILURE;	
		g_ds61850SharedMem.u8TimeSync = FALSE;
	}
	else
	{
		// diebu modified - 2020.05.15. : SNTP_CLOCK_NOT_SYNCHRONIZED -> SNTP_CLOCK_FAILURE
		// 시각동기는 됬지만, synch가 맞지 않음 - clock not synchronized 
		// GPS등과 연결이 안된 시간을 의미..
		gs32SntpTimeQuality = SNTP_TIME_GOOD;	
		g_ds61850SharedMem.u8TimeSync = TRUE;
	}
	
	// STB는 UTC 시간 그대로 받는다.
	// STB는 big endian이기 때문에 ntohl이나 htonl이 모두 같은 값이다.(network도 big endian)
	// 따라서 usec을 할 때 원래 ntohl(pkt.txtime[1])/4295를 해야 한다.그런데 그렇게 안해도 되었다.
	tv.tv_sec = ntohl(pkt.txtime[0]) - JAN_1970;
	tv.tv_usec = ntohl(pkt.txtime[1]) /4295;		//	--> usec을 만들기 위해 
	
	settimeofday(&tv, NULL);
	//printf("tv_sec : %d, tv_usec : %d\t", tv.tv_sec, tv.tv_usec);	
	if(sntp_fd > 0)
		close(sntp_fd);

	return 1;
}

void TSM_TimeSyncExe(void)
{
	struct tm Nowtm1,*Nowtm2;
	time_t ChangeTimet;
	char buffer[1024];
	FILE *read_fp;
	int chars_read;
	
	if(g_ds61850SharedMem.dsDeviceInfo.dsTimeSyncInfo.s32TimeSyncMode == TMSYNC_NTP)
	{
		//printf("[%s] Time sync from Network\n", __FUNCTION__);
		if(sntp_client(g_ds61850SharedMem.dsDeviceInfo.dsNetWorkCfg[1].m_u32IPAddr) == -1)
		{
			printf("Clock not Synchronized for some reason\n");
			gs32SntpTimeQuality = SNTP_CLOCK_NOT_SYNCHRONIZED;
			g_ds61850SharedMem.u8TimeSync = FALSE;
		}
		else
		{
			// RTC Update
			read_fp = popen(CMD_TIMETORTC, "r");
			if(read_fp != NULL)
			{
				memset(buffer, '\0', sizeof(buffer));
				chars_read = fread(buffer, sizeof(char), 1024+1, read_fp);
				pclose(read_fp);
			}
		}
	}
	/*
	else if(g_ds61850SharedMem.dsDeviceInfo.dsTimeSyncInfo.s32TimeSyncMode == TMSYNC_IRIGB) 
	{
		//printf("[%s] Time sync from IRIGB\n", __FUNCTION__);
		if(KSM_IrigBTime((ST_dsIrigBTime*)&dsIrigBVal) == 0)
		{
			memset(&Nowtm1,0x0,sizeof(Nowtm1));
			Nowtm1.tm_year = (int)(dsIrigBVal.year - 1900);    
			Nowtm1.tm_mon  = (int)(dsIrigBVal.month - 1);         
			Nowtm1.tm_mday = (int)dsIrigBVal.day;            
			Nowtm1.tm_hour = (int)dsIrigBVal.hour;       	
			Nowtm1.tm_min  = (int)dsIrigBVal.min;     
			Nowtm1.tm_sec  = (int)dsIrigBVal.sec;      

			ChangeTimet = mktime(&Nowtm1);
			ChangeTimet -= g_pCfgSharedMem.m_dsTimeInfo.s32TmUtcOffset;
			Nowtm2 = localtime(&ChangeTimet);
			// OS Time update
			strftime(buffer, sizeof(buffer), "date -s %Y%m%d%H%M.%S", Nowtm2);
			read_fp = popen(buffer, "r");
			if(read_fp != NULL)
			{
				fread(buffer, sizeof(char), 1024+1, read_fp);
				pclose(read_fp);
			}
			// RTC Update
			read_fp = popen(CMD_TIMETORTC, "r");
			if(read_fp != NULL)
			{
				memset(buffer, '\0', sizeof(buffer));
				chars_read = fread(buffer, sizeof(char), 1024+1, read_fp);
				pclose(read_fp);
			}
		}
	}
	*/
	else
	{
		printf("[%s] Time sync Type Err [%d]\n", __FUNCTION__,g_ds61850SharedMem.dsDeviceInfo.dsTimeSyncInfo.s32TimeSyncMode);
	}
}

void TSM_UpdateTimeSyncInfo(void)
{
	//g_TSMInfo.m_u32TmSyncInterval = DBM_GetSetting_int("NomalSettomg", "Time Sync Interval");
	g_ds61850SharedMem.dsDeviceInfo.dsTimeSyncInfo.u32TmSyncInterval = 60;
	g_ds61850SharedMem.dsDeviceInfo.dsTimeSyncInfo.s32TmUtcOffset = TSM_CalculateUTCOffsetValue();
	printf("Time Sync Interval [%d] UTC Offset(sec) [%d]\n",g_ds61850SharedMem.dsDeviceInfo.dsTimeSyncInfo.u32TmSyncInterval,g_ds61850SharedMem.dsDeviceInfo.dsTimeSyncInfo.s32TmUtcOffset);
}


int TSM_CalculateUTCOffsetValue(void)
{
	int s32UTCVal=0;
	int s32OffsetHour=0, s32OffsetMin=0;

	s32OffsetHour = g_ds61850SharedMem.dsDeviceInfo.s32UTCOffsetHour;
	s32OffsetMin = g_ds61850SharedMem.dsDeviceInfo.s32UTDOffsetMinute;

	if((s32OffsetHour >= SMO_GENSET_UTC_HOUR_MIN) && (s32OffsetHour <= SMO_GENSET_UTC_HOUR_MAX))
	{
		if((s32OffsetMin >= SMO_GENSET_UTC_MINUTE_MIN) && (s32OffsetMin <= SMO_GENSET_UTC_MINUTE_MAX))
		{
			if(s32OffsetHour < 0)
			{
				s32UTCVal = (s32OffsetHour * HOUR_TO_SEC_UNIT) - (s32OffsetMin * MIN_TO_SEC_UNIT);
			}
			else
			{
				s32UTCVal = (s32OffsetHour * HOUR_TO_SEC_UNIT) + (s32OffsetMin * MIN_TO_SEC_UNIT);
			}
		}
	}
	
	return s32UTCVal;
}


