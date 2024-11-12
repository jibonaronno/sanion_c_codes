#ifdef __cplusplus
extern "C" {
#endif

#define MSG_PD_START_ID					0x01 // CU  <--> PDDAU
#define MSG_PD_STOP_ID					0x02 // CU  <--> PDDAU
#define MSG_PD_SEND_ID					0x03 // CU  <--  PDDAU
#define MSG_RF_INFO_ID					0x04 // CU  <--> PDDAU
#define MSG_PDDAU_INFO_ID				0x05 // CU  <--> PDDAU
#define MSG_ALARM_ID 					0x06 // CU  <--> PDDAU
#define MSG_KEEP_ALIVE_ID				0x07 // CU  <--> PDDAU

#define MSG_REQUEST_TYPE				0x01 // CU  -->  PDDAU
#define MSG_REQUEST_ACK_TYPE			0x11 // CU  <--  PDDAU
#define MSG_SEND_TYPE					0x03 // CU  <--  PDDAU
#define MSG_READ_INFO_TYPE				0x02 // CU  -->  PDDAU
#define MSG_READ_INFO_RESPONSE_TYPE		0x12 // CU  <--  PDDAU
#define MSG_ALARM_TYPE					0x04 // CU  <--  PDDAU
#define MSG_ALARM_ACK_TYPE 				0x14 // CU  -->  PDDAU

#define ORDER_TIME_SETTING				0
#define ORDER_PDDAU_INFO				1
#define ORDER_RF_INFO					2
#define ORDER_PD_DATA					3

#define RF_INFO_DATALEN 				0x001A
#define PD_INFO_DATALEN					0x0023
#define MAX_PD_CHANNELS_Sen		            4

#define body_len_max                    6240
#define SenPORT 5000
#define SensorMAX 6244
#define SA struct sockaddr
#define PDdatMax 15360

#include <time.h>
#include "Common.h"

#pragma pack(push,1)
typedef struct
{
	unsigned char msg_id;
	unsigned char msg_type;
	short		  body_len;
} SPECTRUM_PACKET_HEADER;
#pragma pack(pop)

// SPECTRUM_PACKET_HEADER PDdatreq;
// SPECTRUM_PACKET_HEADER PDdatack;
// SPECTRUM_PACKET_HEADER PDstopreq;
// SPECTRUM_PACKET_HEADER PDstopack;
// SPECTRUM_PACKET_HEADER PDdatsend;
// SPECTRUM_PACKET_HEADER RFInfoset;
// SPECTRUM_PACKET_HEADER RFInfoack;
// SPECTRUM_PACKET_HEADER RFInforeq;
// SPECTRUM_PACKET_HEADER RFInforeqack;
// SPECTRUM_PACKET_HEADER PDDAUset;
// SPECTRUM_PACKET_HEADER PDDAUsetack;
// SPECTRUM_PACKET_HEADER PDDAUreq;
// SPECTRUM_PACKET_HEADER PDDAUreqack;
// SPECTRUM_PACKET_HEADER Alarm;
// SPECTRUM_PACKET_HEADER Alarmack;
// SPECTRUM_PACKET_HEADER Keepalive;
// SPECTRUM_PACKET_HEADER Keepaliveack;


#pragma pack(push,1)
typedef struct
{
	SPECTRUM_PACKET_HEADER header;	
    unsigned char time_enable;
    unsigned char datetime[7];
	unsigned char pdDAUnum_enable;
	unsigned char pdDAUnum;
    unsigned char pwrrst_enable;
    unsigned char pwrrst_status;
    unsigned char firmware_ver_enable;
	unsigned char pdDAU_ver[7];
	unsigned char pdDAU_IPinfo_enable;
	unsigned char pdDAU_IPAdr[4];
	unsigned char pdDAU_MACinfo_enable;
	unsigned char pdDAU_MACinfo[6];
	unsigned char pdDAU_Port_enable;
	unsigned char pdDAU_Port_Adrr[2];
} MSG_PDDAU_INFO_PACKET;
#pragma pack(pop)

#pragma pack(push,1)

typedef struct
{
    SPECTRUM_PACKET_HEADER header;
    unsigned char ch_enable;
	unsigned char ch_type[6];
	unsigned char gatingch_enable;
	unsigned char gatingch_onoff[3];
	unsigned char gatingth_enable;
	short gatingth_value[6];
	unsigned char cal_enable;
	unsigned char cal_onoff;
	unsigned char amp_enable;
	unsigned char amp[24];
} MSG_RF_INFO_PACKET;

#pragma pack(pop)

#pragma pack(push,1)
typedef struct
{
	U8 ch_idx;
	U8 reserve[3];
	U8 data[256];
} MSG_PD_PACKET;


typedef struct
{
	SPECTRUM_PACKET_HEADER header;
	MSG_PD_PACKET datafile [4];
} MSG_PD_Tot;

#pragma pack(pop)

/****************************
  * MSG_PD_BODY struct declared in spectrum_packet.h
 *****************************/
#pragma pack(push,1)
typedef struct
{
	unsigned char ch_idx;
	signed char EventAmpTh1;
	signed char EventAmpTh2;
	unsigned char EventPpsTh;
	unsigned char data[256];
} MSG_PD_BODY;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct
{
	SPECTRUM_PACKET_HEADER header;	
	MSG_PD_BODY data[4];
} MSG_PD_FULL_PACKET;
#pragma pack(pop)


// #pragma pack(push,1)
// void* RunPDDAU(void *params);
// int setAmplification(int channel, int sockfd, MSG_RF_INFO_PACKET *msg_rf_info_packet);
// int getAmplification(int channel, int sockfd, MSG_RF_INFO_PACKET *msg_rf_info_packet);
// int resetPDDAU(int sockfd, MSG_PDDAU_INFO_PACKET *msg_pddau_info_packet);
// #pragma pack(pop)

#ifdef __cplusplus
}
#endif
