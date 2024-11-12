/******************************************************************************************************
 **                     Copyright(C) 2014 SANION Co. Ltd.                    **
 **                  --------------------------------------                  **
 **                              WWW.SANION.COM                              **
 ******************************************************************************************************
    PROJECT     : UR-IED
 ******************************************************************************************************
    FILE NAME   : Common.h
    AUTHOR      : sootoo23
    DATE        : 2015.07.22
    REVISION    : V1.00
    DESCRIPTION : Shared Common Function
 ******************************************************************************************************
    HISTORY     :
        2015-07-22 sootoo23 - Create
 ******************************************************************************************************/

#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdlib.h>
#include <string.h>

#define filebuffermax 5

/*******************************************************************************
 data type
*******************************************************************************/
typedef unsigned char  			bool;      ///< unint8_t		// added by symoon
typedef unsigned char  			U8;        ///< unint8_t
typedef unsigned short  		U16;       ///< uint16_t
typedef unsigned int    		U32;       ///< uint32_t
typedef unsigned long long int  U64;       ///< uint64_t
typedef signed char     		S8;        ///< int8_t
typedef signed short    		S16;       ///< int16_t
typedef signed int     			S32;       ///< int32_t
typedef float           		F32;       ///< float
typedef double          		D64;       ///< double
////////////////////////////////////////////////////////////////////////////////

#define KEY_DEL        		0x7F
#define KEY_BACKSPACE  		0x08
#define KEY_CR         		0x0D
#define KEY_LEFT			0x44
#define KEY_RIGHT			0x43
#define KEY_UP				0x41
#define KEY_DOWN			0x42

#define SAMPLES_PER_CYCLE	128

#define SAMPLING_INTERVAL_IN_SEC		0.000130211

#define MAX_SIZE_OF_DATA_LIST_TO_CU		200


typedef struct {
	U32	m_u32Value;
	U32 m_u32Qual;
} dsBasicStrForIEC61850;

typedef struct{
	char namebuffer[filebuffermax][50];
}namebufferstruct;


extern int  AnsiToUTF8(char* lpcSrc, char*utf8Str);
extern int getch();
extern int CreateSemaphore(int);
extern int SemPost(int);
extern int SemWait(int);
extern void DeleteSemaphore(int);
extern unsigned char convert_BCD_to_DEC(unsigned char _BCD);
extern unsigned char convert_DEC_to_BCD(unsigned char _DEC);
extern int convert_Str_to_DEC(char *_str);
extern int convert_Str_to_HEX(char *_str);
extern unsigned short convert_Endian_16bit(unsigned short data);
extern void convert_Word_To_Byte( unsigned char *pu8TxBuffer, unsigned short u16TempWord ); // Word to Byte 
extern void convert_Byte_To_Word( unsigned char *pu8TempBuff, unsigned short *pu16TempWord ); //Byte to Word(Big Edian)

U16 CalculateCRC(U8* pu8PtrStart, U16 u16Size);						// added by symoon
bool CheckCRC(U8* pu8Data, U16 u16Length, U16 u16DesiredCrc, U16* pu16CalculatedCRC);		// added by symoon

U32 CalculateTimeDiffInMsec(struct timeval* start, struct timeval* end);					// added by symoon

float MaxFloat3(float fV1, float fV2, float fV3, int* pIdx);								// added by symoon
#endif
