/******************************************************************************************************
 **                     Copyright(C) 2016 SANION Co. Ltd.                    **
 **                  --------------------------------------                  **
 **                              WWW.SANION.COM                              **
 ******************************************************************************************************
    PROJECT     : UR-IED
 ******************************************************************************************************
    FILE NAME   : Common.c
    AUTHOR      : sootoo23
    DATE        : 2016.11.21
    REVISION    : V1.00
    DESCRIPTION : Common Function
 ******************************************************************************************************
    HISTORY     :
        2016-11-21 sootoo23 - Create
 ******************************************************************************************************/


#include <stdio.h>
#include <termios.h>
#include <sys/sem.h>
#include <unistd.h>
#include <iconv.h>
#include <sys/time.h>		// added by symoon

#include "Common.h"
#include "PowerSystem.h"


const U16 gu16Crc16Code[256] =
{
	0x0000, 0x365e, 0x6cbc, 0x5ae2, 0xd978, 0xef26, 0xb5c4, 0x839a,
	0xff89, 0xc9d7, 0x9335, 0xa56b, 0x26f1, 0x10af, 0x4a4d, 0x7c13,
	0xb26b, 0x8435, 0xded7, 0xe889, 0x6b13, 0x5d4d, 0x07af, 0x31f1,
	0x4de2, 0x7bbc, 0x215e, 0x1700, 0x949a, 0xa2c4, 0xf826, 0xce78,
	0x29af, 0x1ff1, 0x4513, 0x734d, 0xf0d7, 0xc689, 0x9c6b, 0xaa35,
	0xd626, 0xe078, 0xba9a, 0x8cc4, 0x0f5e, 0x3900, 0x63e2, 0x55bc,
	0x9bc4, 0xad9a, 0xf778, 0xc126, 0x42bc, 0x74e2, 0x2e00, 0x185e,
	0x644d, 0x5213, 0x08f1, 0x3eaf, 0xbd35, 0x8b6b, 0xd189, 0xe7d7,
	0x535e, 0x6500, 0x3fe2, 0x09bc, 0x8a26, 0xbc78, 0xe69a, 0xd0c4,
	0xacd7, 0x9a89, 0xc06b, 0xf635, 0x75af, 0x43f1, 0x1913, 0x2f4d,
	0xe135, 0xd76b, 0x8d89, 0xbbd7, 0x384d, 0x0e13, 0x54f1, 0x62af,
	0x1ebc, 0x28e2, 0x7200, 0x445e, 0xc7c4, 0xf19a, 0xab78, 0x9d26,
	0x7af1, 0x4caf, 0x164d, 0x2013, 0xa389, 0x95d7, 0xcf35, 0xf96b,
	0x8578, 0xb326, 0xe9c4, 0xdf9a, 0x5c00, 0x6a5e, 0x30bc, 0x06e2,
	0xc89a, 0xfec4, 0xa426, 0x9278, 0x11e2, 0x27bc, 0x7d5e, 0x4b00,
	0x3713, 0x014d, 0x5baf, 0x6df1, 0xee6b, 0xd835, 0x82d7, 0xb489,
	0xa6bc, 0x90e2, 0xca00, 0xfc5e, 0x7fc4, 0x499a, 0x1378, 0x2526,
	0x5935, 0x6f6b, 0x3589, 0x03d7, 0x804d, 0xb613, 0xecf1, 0xdaaf,
	0x14d7, 0x2289, 0x786b, 0x4e35, 0xcdaf, 0xfbf1, 0xa113, 0x974d,
	0xeb5e, 0xdd00, 0x87e2, 0xb1bc, 0x3226, 0x0478, 0x5e9a, 0x68c4,
	0x8f13, 0xb94d, 0xe3af, 0xd5f1, 0x566b, 0x6035, 0x3ad7, 0x0c89,
	0x709a, 0x46c4, 0x1c26, 0x2a78, 0xa9e2, 0x9fbc, 0xc55e, 0xf300,
	0x3d78, 0x0b26, 0x51c4, 0x679a, 0xe400, 0xd25e, 0x88bc, 0xbee2,
	0xc2f1, 0xf4af, 0xae4d, 0x9813, 0x1b89, 0x2dd7, 0x7735, 0x416b,
	0xf5e2, 0xc3bc, 0x995e, 0xaf00, 0x2c9a, 0x1ac4, 0x4026, 0x7678,
	0x0a6b, 0x3c35, 0x66d7, 0x5089, 0xd313, 0xe54d, 0xbfaf, 0x89f1,
	0x4789, 0x71d7, 0x2b35, 0x1d6b, 0x9ef1, 0xa8af, 0xf24d, 0xc413,
	0xb800, 0x8e5e, 0xd4bc, 0xe2e2, 0x6178, 0x5726, 0x0dc4, 0x3b9a,
	0xdc4d, 0xea13, 0xb0f1, 0x86af, 0x0535, 0x336b, 0x6989, 0x5fd7,
	0x23c4, 0x159a, 0x4f78, 0x7926, 0xfabc, 0xcce2, 0x9600, 0xa05e,
	0x6e26, 0x5878, 0x029a, 0x34c4, 0xb75e, 0x8100, 0xdbe2, 0xedbc,
	0x91af, 0xa7f1, 0xfd13, 0xcb4d, 0x48d7, 0x7e89, 0x246b, 0x1235
};


/*****************************************************************************
 **                                               Convert Incoding                                                        **
 *****************************************************************************/
#define ANSI2UTF8(a)    AnsiToUTF8(a) 
 
/**
@brief      ANSI to UTF-8
@author   sootoo23
@date      2018-08-16
@param      lpcSrc  :[in] ANSI문자열
@retval     String  : UTF8로 변환된 문자열
*/
int  AnsiToUTF8(char* lpcSrc, char*utf8Str)
{
	size_t      nSrcLen = strlen(lpcSrc);
	char      strUtf8[nSrcLen*2];
	size_t      nOutLen=0;
	iconv_t     cd=NULL;


	// UTF-8 로 인코딩하여 저장하기 
	// UTF-8 의 크기는 EUC-KR 의 약 3배 정도이다.
	size_t szIn = nSrcLen + 10;
	size_t szOut = szIn * 3;

	char *pIn = (char*)malloc(szIn);
	char *pOut = (char*)malloc(szOut);

	memset(pIn, '\0', sizeof(char)*szIn);
	memset(pOut, '\0', sizeof(char)*szOut);
	strcpy(pIn, lpcSrc);

	char *pIn2 = pIn;
	char *pOut2 = pOut;

	if ( (cd = iconv_open("UTF-8//IGNORE", "ANSI")) < 0 )
	{
		// iconv_open 에서 error 나면 errno 을 셋팅한다.
		//fprintf(stderr, "Fail to iconv_open() errorno : %d\n", errno);
		printf("[%s] ERR: Fail to iconv_open().\n", __func__);

		exit(-1);
	}
	
	// 변환하기
	size_t  ret=0;
	if ( (ret = iconv(cd, &pIn2, &szIn, &pOut2, &szOut)) < 0 )
	{
		// iconv 에서 error 나면 errno 을 셋팅한다.
		// E2BIG *outbuf에 공간이 부족합니다
		// EILSEQ 부정확한 다중바이트 문자열이 입력값으로 들어왔습니다.
		// EINVAL 완료되지않은 다중바이트문자열이 입력값으로 들어왔습니다. 
		//char errmsg[10] = "";
		//if (errno == E2BIG) sprintf(errmsg, "E2BIG");
		//if (errno == EILSEQ) sprintf(errmsg, "EILSEQ");
		//if (errno == EINVAL) sprintf(errmsg, "EINVAL");
		//fprintf(stderr, "Fail to iconv() errorno : %s(%d)\n", errmsg, errno);
		printf("[%s] ERR: Fail.\n", __func__);
	}
	iconv_close(cd);

	// iconv() 처리 후 szOut 크기를 재계산해야 한다.
	// iconv() 는 pIn2 를 szIn 크기가 0이 될 때까지 처리한다.
	//szOut = pOut2 - pOut;
	//strUtf8 = szOut;

	memcpy(utf8Str, pOut, sizeof(strUtf8));
	
	free(pIn);
	free(pOut);
 
	return 0;
}
 
//sootoo23-20150716 : Linux getch function add
int getch()
{
	int ch=0;
	struct termios buf, save;
	tcgetattr(0, &save);
	buf = save;
	buf.c_lflag &= ~ (ICANON | ECHO);
	buf.c_cc[VMIN] = 1;
	buf.c_cc[VTIME] = 0;
	tcsetattr(0,TCSAFLUSH, &buf);
	ch = getchar();
	
	tcsetattr(0, TCSAFLUSH, &save);
	return ch;
}

int CreateSemaphore(int count) 
{
	int semid;

#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
	union semun parm /* union is defined by including <sys/sem.h>*/
#else
	/* according to X/OPEN we have to define it ourselves */
	union semun {
		int val; /* value for SETVAL */
		struct semid_ds *buf; /* buffer for IPC_STAT, IPC_SET */
		unsigned short *array; /* array for GETALL, SETALL */
		/* Linux specific part: */
		struct seminfo *__buf; /* buffer for IPC_INFO */
	} parm;
#endif

	semid = semget(IPC_PRIVATE, 1, 0664 | IPC_CREAT | IPC_EXCL);
	if (semid < 0)
		return (-1);
	parm.val = count;
	if (semctl(semid, 0, SETVAL, parm) < 0) {
		DeleteSemaphore(semid);
		return (-1);
	}
	return (semid);
}

void DeleteSemaphore(int semid) 
{
	semctl(semid, IPC_RMID, 0);
	return;
}

int SemPost(int semid) 
{
	//sootoo23-UR Base AM5728...Not Oper...
	//return 0;
	
	struct sembuf sem;

	sem.sem_num = 0;
	sem.sem_op = 1;
	sem.sem_flg = 0;

	if (semop(semid, &sem, 1) == -1)
		return (-1);
	else
		return (0);
}

int SemWait(int semid) 
{
	//sootoo23-UR Base AM5728...Not Oper...
	//return 0;
	
	struct sembuf sem;

	sem.sem_num = 0;
	sem.sem_op = -1;
	sem.sem_flg = 0;

	if (semop(semid, &sem, 1) == -1)
		return (-1);
	else
		return (0);
}

unsigned char convert_BCD_to_DEC(unsigned char _BCD)
{
	unsigned char _dec;
	unsigned char _tmpH, _tmpL;

	_tmpH = (_BCD & 0xF0) >> 4;
	_tmpL = (_BCD & 0x0F);
	_dec = _tmpH * 10 + _tmpL;
	return _dec;
}

unsigned char convert_DEC_to_BCD(unsigned char _DEC) 
{
	unsigned char _bcd;
	unsigned char _tmpH, _tmpL;

	_tmpH = (unsigned char) (_DEC / 10) << 4;
	_tmpL = (unsigned char) (_DEC % 10);
	_bcd = _tmpH | _tmpL;
	return _bcd;
}

int convert_Str_to_DEC(char *_str) 
{
	return (int)atoi(_str);
}

int convert_Str_to_HEX(char *_str) 
{
	if (_str == NULL)
		return 0;

	int i = 0;
	int _ret_value = 0;
	int _str_length = strlen(_str);

	for (i = 0; i < _str_length; i++) {
		_ret_value *= 16;
		if ((_str[i] > 0x60) && (_str[i] < 0x67)) {
			_ret_value += (_str[i] - 0x61) + 10;
		} else if ((_str[i] > 0x40) && (_str[i] < 0x47)) {
			_ret_value += (_str[i] - 0x41) + 10;
		} else if ((_str[i] > 0x30) && (_str[i] < 0x3a)) {
			_ret_value += (_str[i] - 0x30);
		}
	}
	return _ret_value;
}

unsigned short convert_Endian_16bit(unsigned short data)
{
	return ((((data) & 0xff) << 8) | (((data) >> 8) & 0xff));
}

/*****************************************************************************
 **                     Word to Byte(Big Edian)                             **
 *****************************************************************************/
void convert_Word_To_Byte( unsigned char *pu8TxBuffer, unsigned short u16TempWord )
{
	unsigned char u8TmpCnt = 0;

	/* | MSB(n) | LSB(n+1) | */
	pu8TxBuffer[u8TmpCnt++] = (unsigned char)( (u16TempWord & 0xFF00) >> 8 );
	pu8TxBuffer[u8TmpCnt++] = (unsigned char)(	u16TempWord & 0x00FF);
}



/*****************************************************************************
 **                     Byte to Word(Big Edian)                             **
 *****************************************************************************/
void convertB_Byte_To_Word( unsigned char *pu8TempBuff, unsigned short *pu16TempWord )
{
	/* | MSB(n) | LSB(n+1) | */
	*pu16TempWord =  *pu8TempBuff  & 0xFF;
	*pu16TempWord =  *pu16TempWord << 8;
	*pu16TempWord |= *(pu8TempBuff+1) & 0xFF;
}

/*****************************************************************************
 **                     Byte to Word(Little Edian)                             **
 *****************************************************************************/
void convert_Byte_To_Word( unsigned char *pu8TempBuff, unsigned short *pu16TempWord )
{
	/* | MSB(n+1) | LSB(n) | */
	*pu16TempWord = *pu8TempBuff & 0xFF;
	*pu16TempWord =  *pu8TempBuff  & 0xFF;
	*pu16TempWord |=  *(pu8TempBuff+1) << 8;
}


// added by symoon
U16 CalculateCRC(U8* pu8PtrStart, U16 u16Size)
{
	U16 u16Crc = 0;


	while (u16Size--) {
		u16Crc=(u16Crc>>8)^(gu16Crc16Code[((u16Crc^*pu8PtrStart++) & 0x00ff)]);
	}

	return((U16)~u16Crc);
}



// added by symoon
bool CheckCRC(U8* pu8Data, U16 u16Length, U16 u16DesiredCrc, U16* pu16CalculatedCRC)
{
	U16 u16Crc = 0;


	while (u16Length--) {
		u16Crc=(u16Crc>>8)^(gu16Crc16Code[((u16Crc^*pu8Data++) & 0x00ff)]);
	}

	u16Crc = (U16)(~u16Crc);

	*pu16CalculatedCRC = u16Crc;
	
	if (u16Crc != u16DesiredCrc)
		return(0);
	else
		return(1);
}


// added by symoon
U32 CalculateTimeDiffInMsec(struct timeval* start, struct timeval* end)
{
	U32 u32TimeDiffInMsec;

	
    u32TimeDiffInMsec = (U32)((1000000*(end->tv_sec - start->tv_sec) + (end->tv_usec - start->tv_usec))/1000);

    return(u32TimeDiffInMsec);
}


// added by symoon
float MaxFloat3(float fV1, float fV2, float fV3, int* pIdx)
{	
	if (fV1 > fV2) {
		if (fV1 > fV3) {
			*pIdx = 0;
			return(fV1);
		}
		else {
			*pIdx = 2;
			return(fV3);
		}
	}
	else {
		if (fV2 > fV3) {
			*pIdx = 1;
			return(fV2);
		}
		else {
			*pIdx = 2;
			return(fV3);
		}
	}
}

