
#ifndef TIMESYNCMODULE_H_
#define TIMESYNCMODULE_H_

#include <fcntl.h>
#include <stdio.h>
#include <time.h>

#include "global.h"


#define DSP_TICK_TIME       DSP_SPS

// 외부 시각 소스에서 OS 내부 시각을 동기
#define CMD_TIMEFROMNET      "rdate "
#define CMD_TIMEFROMRTC      "hwclock -s"
#define CMD_TIMETORTC        "hwclock -w"
#define CMD_SETTIME          "date -s "

//sootoo23 - 170329: DSP Tick Sync..
#define MILLISEC_TO_TICK50Hz	6.40
#define MILLISEC_TO_TICK60Hz	7.68

#define MILLISEC_TO_FSTICK50Hz	64.00
#define MILLISEC_TO_FSTICK60Hz	76.80

#define FSTICK50Hz_TO_MILLISEC	0.01000
#define FSTICK60Hz_TO_MILLISEC	0.009920634921

#define NTP_PORT		123
#define MY_PORT			7000
#define JAN_1970    2208988800UL	 	// 197 - 1900 in secs, 0x83aa7e80
#define TWO_BIT_MASK		0XC0

#define MIN_TO_SEC_UNIT		60
#define HOUR_TO_SEC_UNIT	3600 

 // DEVINFO
#define SMO_GENSET_UTC_HOUR_MIN			-24
#define SMO_GENSET_UTC_HOUR_MAX			24
#define SMO_GENSET_UTC_MINUTE_MIN		0
#define SMO_GENSET_UTC_MINUTE_MAX		59
#define SMO_GENSET_TIME_SYNC_MIN		0
#define SMO_GENSET_TIME_SYNC_MAX		1

 /*************************************/
 /*  SNTP Time Quality 정보			  */
 /*************************************/
#define SNTP_TIME_GOOD						0x8A		// 정상
#define SNTP_CLOCK_FAILURE					0x6A //0x4A		// 이상 : 시각동기는 됬는데 sync가 맞지 않음
#define SNTP_CLOCK_NOT_SYNCHRONIZED			0x6A //0x2A		// 이상 : 시각동기 안됨

 typedef struct
{
    U32   lMostSignificant;
    U16   wLeastSignificant;
}dsDNPTIME;

typedef struct
{
    U16   wYear;
    U8   bMonth;
    U8   bDay;
    U8   bHour;
    U8   bMin;
    U8   bSec;
    U8   bWeek;
    U8   wMsec;
}dsDATETIME;

/* on Unix/Linux systems, gettimeofday() is defined in */
/* <time.h> or <sys/time.h> */

struct 
{
	unsigned char livnmode, stratum, poll, prec;
	unsigned long delay, disp, refid;
	unsigned long reftime[2], origtime[2], rcvtime[2], txtime[2];
}pkt;

int sntp_client(U32	m_u32IPAddr);
int TSM_CalculateUTCOffsetValue(void);
void TSM_TimeSyncExe(void);
void TSM_UpdateTimeSyncInfo(void);


#endif /* TIMESYNCMODULE_H_ */
