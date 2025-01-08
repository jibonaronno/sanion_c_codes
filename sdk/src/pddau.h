
#include "global.h"
#include "Common.h"
#include "main.h"
#include "spectrum_packet.h"

#ifdef __cplusplus
extern "C" {
#endif

// #pragma pack(push,1)
// typedef struct
// {
// 	unsigned char msg_id;
// 	unsigned char msg_type;
// 	short		  body_len;
// } SPECTRUM_PACKET_HEADER;
// #pragma pack(pop)

// #pragma pack(push,1)
// typedef struct
// {
// 	SPECTRUM_PACKET_HEADER header;	
//     unsigned char time_enable;
//     unsigned char datetime[7];
// 	unsigned char pdDAUnum_enable;
// 	unsigned char pdDAUnum;
//     unsigned char pwrrst_enable;
//     unsigned char pwrrst_status;
//     unsigned char firmware_ver_enable;
// 	unsigned char pdDAU_ver[7];
// 	unsigned char pdDAU_IPinfo_enable;
// 	unsigned char pdDAU_IPAdr[4];
// 	unsigned char pdDAU_MACinfo_enable;
// 	unsigned char pdDAU_MACinfo[6];
// 	unsigned char pdDAU_Port_enable;
// 	unsigned char pdDAU_Port_Adrr[2];
// } MSG_PDDAU_INFO_PACKET;
// #pragma pack(pop)

// #pragma pack(push,1)

// typedef struct
// {
//     SPECTRUM_PACKET_HEADER header;
//     unsigned char ch_enable;
// 	unsigned char ch_type[6];
// 	unsigned char gatingch_enable;
// 	unsigned char gatingch_onoff[3];
// 	unsigned char gatingth_enable;
// 	short gatingth_value[6];
// 	unsigned char cal_enable;
// 	unsigned char cal_onoff;
// 	unsigned char amp_enable;
// 	unsigned char amp[24];
// } MSG_RF_INFO_PACKET;

// #pragma pack(pop)

// #pragma pack(push,1)
// typedef struct
// {
// 	U8 ch_idx;
// 	U8 reserve[3];
// 	U8 data[256];
// } MSG_PD_PACKET;


// typedef struct
// {
// 	SPECTRUM_PACKET_HEADER header;
// 	MSG_PD_PACKET datafile [4];
// } MSG_PD_Tot;

// #pragma pack(pop)

// /****************************
//   * MSG_PD_BODY struct declared in spectrum_packet.h
//  *****************************/
// #pragma pack(push,1)
// typedef struct
// {
// 	unsigned char ch_idx;
// 	signed char EventAmpTh1;
// 	signed char EventAmpTh2;
// 	unsigned char EventPpsTh;
// 	unsigned char data[256];
// } MSG_PD_BODY;
// #pragma pack(pop)

// #pragma pack(push,1)
// typedef struct
// {
// 	SPECTRUM_PACKET_HEADER header;
// 	MSG_PD_BODY data[4];
// } MSG_PD_FULL_PACKET;
// #pragma pack(pop)

#pragma pack(push,1)
void* RunPDDAU(void *params);
int setAmplification(int channel, int sockfd, MSG_RF_INFO_PACKET *msg_rf_info_packet);
int getAmplification(int channel, int sockfd, MSG_RF_INFO_PACKET *msg_rf_info_packet);
int resetPDDAU(int sockfd, MSG_PDDAU_INFO_PACKET *msg_pddau_info_packet);
#pragma pack(pop)

#ifdef CPP_BLOCK_ENABLED

class _PDDAU
{
    public:
    _PDDAU(void);
    int getAmplification(int channel, int sockfd, MSG_RF_INFO_PACKET *msg_rf_info_packet);
    int setAmplification(int channel, int sockfd, MSG_RF_INFO_PACKET *msg_rf_info_packet);
    void parseIPAddressesFromJson(const char *_filename);

    int pddau_index;
    char ip_address[16];

};


#endif

#ifdef __cplusplus
}
#endif