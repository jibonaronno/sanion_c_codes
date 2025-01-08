
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
#include <unistd.h>
#include "pddau.h"

#ifdef CPP_BLOCK_ENABLED

static dsLUConfig pdsluConfig;
extern ds61850IfcInfo* g_p61850IfcInfo;

void dumphex(char *buf, int len, int nCol)
{
    int i,j;
    j = 0;
    printf("\n\n\n\n\n\n\n\n\n");

    for(i=0;i<len;i++)
    {
        printf("%02X ", buf[i]);
        if(j > 3){
            printf("\n");
            j = 0;
        }
        else
        {
            j++;
        }
    }

    printf("\n\n\n");
}


int setAmplification(int channel, int sockfd, MSG_RF_INFO_PACKET *msg_rf_info_packet)
{
    int idxA = 0;
    int iResult = 0;
    //MSG_RF_INFO_PACKET *msg_rf_info_packet = malloc(sizeof(MSG_RF_INFO_PACKET));

    msg_rf_info_packet->header.msg_id = MSG_RF_INFO_ID;
    msg_rf_info_packet->header.msg_type = MSG_REQUEST_TYPE; //MSG_SEND_TYPE; // MSG_REQUEST_TYPE;
    msg_rf_info_packet->header.body_len = 0x3300;
    msg_rf_info_packet->amp_enable = 0x01;

    for(idxA=0;idxA < 24; idxA++)
    {
        msg_rf_info_packet->amp[idxA] = 20; //was 5
    }

    msg_rf_info_packet->amp[3] = 20;
    /*
    msg_rf_info_packet->ch_enable = 0x01;
    msg_rf_info_packet->ch_type[0] = 0xFF;
    msg_rf_info_packet->gatingch_enable = 0x00;
    msg_rf_info_packet->gatingth_enable = 0x00;
    msg_rf_info_packet->cal_enable = 0x00;
    
    msg_rf_info_packet->amp[0] = 1;
    msg_rf_info_packet->amp[1] = 1;
    msg_rf_info_packet->amp[2] = 1;
    
    */
    
    iResult = write(sockfd, msg_rf_info_packet, 55); //sizeof(msg_rf_info_packet));
    if (iResult == -1)
    {
        fprintf(stderr, "AMP SET - send Error Occurred %s (%d)\n", strerror(errno), errno);
        return -1;
    }
    else
    {
        printf("\n -- AMP SET - RF INFO Written iResult = %d \n", iResult);
    }

    iResult = read(sockfd, msg_rf_info_packet, 4);
    if(iResult == -1)
    {
        fprintf(stderr, "AMP SET - recv Error Occurred %s (%d)\n", strerror(errno), errno);
        return -1;
    }
    else
    {
        printf("AMP SET ACK Received Success \n");
    }

    if (msg_rf_info_packet->header.msg_type != MSG_REQUEST_ACK_TYPE)
    {
        printf(" AMP SET Info Packet Type Not Matched \n");
        return -1;
    }
    else
    {
        printf("AMP SET Info Matched \n");

        printf("ch_enable = %d\n", msg_rf_info_packet->ch_enable);
    }

    return 0;
}

/**
 * Get the Amplification settings from PDDAU. 
 * @param channel : This parameter is not effective yet.
 * @param sockfd : Already connected socket 
 */
int getAmplification(int channel, int sockfd, MSG_RF_INFO_PACKET *msg_rf_info_packet)
{
    int idxA = 0;
    int iResult = 0;
    // MSG_RF_INFO_PACKET *msg_rf_info_packet = malloc(sizeof(MSG_RF_INFO_PACKET));

    msg_rf_info_packet->header.msg_id = MSG_RF_INFO_ID;
    msg_rf_info_packet->header.msg_type = MSG_READ_INFO_TYPE; //MSG_SEND_TYPE; // MSG_REQUEST_TYPE;
    msg_rf_info_packet->header.body_len = 0; //ntohs(0x0033);
    
    iResult = write(sockfd, msg_rf_info_packet, 4); //sizeof(msg_rf_info_packet));
    if (iResult == -1)
    {
        fprintf(stderr, "AMP GET INFO - send Error Occurred %s (%d)\n", strerror(errno), errno);
        return -1;
    }
    else
    {
        printf("\n -- AMP GET INFO - RF INFO Written iResult = %d \n", iResult);
    }

    iResult = read(sockfd, msg_rf_info_packet, 55);
    if(iResult == -1)
    {
        fprintf(stderr, "recv Error Occurred %s (%d)\n", strerror(errno), errno);
        return -1;			
    }
    else
    {
        printf("Info Read Success \n");
    }


    if (msg_rf_info_packet->header.msg_type != MSG_READ_INFO_RESPONSE_TYPE)
    {
        printf(" Info Packet Type Not Matched \n");
        return -1;
    }
    else
    {
        printf("Info Matched \n");

        //printf("\nch_enable = %d\namp[0] = %d\namp[1] = %d\namp[2] = %d\namp[3] = %d\n\n", msg_rf_info_packet->ch_enable, msg_rf_info_packet->amp[0], msg_rf_info_packet->amp[2], msg_rf_info_packet->amp[3], msg_rf_info_packet->amp[4]);
        printf("\n");
        printf("AMP SET --amp enable %d\n", msg_rf_info_packet->amp_enable);
        printf("\n");
        for(idxA=0;idxA<24;idxA++)
        {
            printf("amp[%d] = %d\n", idxA, msg_rf_info_packet->amp[idxA]);
        }

        printf("\n");

        for(idxA=0;idxA<6;idxA++)
        {
            printf("ch_type[%d] = %d\n", idxA, msg_rf_info_packet->ch_type[idxA]);
        }
    }

    return 0;
}

/**
 * Reset PDDAU before connect at first start and also reset PDDAU when connection disrupted. 
 * @param sockfd : Already connected socket.
 * @param msg_pddau_info_packet : Instance of MSG_PDDAU_INFO_PACKET containing reset command.
 */
int resetPDDAU(int sockfd, MSG_PDDAU_INFO_PACKET *msg_pddau_info_packet)
{

    int iResult = 0;
    msg_pddau_info_packet->header.msg_id = MSG_PDDAU_INFO_ID;
    msg_pddau_info_packet->header.msg_type = MSG_REQUEST_TYPE;
    msg_pddau_info_packet->header.body_len = 0x2300;
    msg_pddau_info_packet->firmware_ver_enable = 0;
    msg_pddau_info_packet->pdDAU_IPinfo_enable = 0;
    msg_pddau_info_packet->pdDAU_MACinfo_enable = 0;
    msg_pddau_info_packet->pdDAU_Port_enable = 0;
    msg_pddau_info_packet->pwrrst_enable = 0x80;

    iResult = write(sockfd, msg_pddau_info_packet, 0x27); //sizeof(msg_rf_info_packet));
    if (iResult == -1)
    {
        fprintf(stderr, "PDDU INFO RESET - send Error Occurred %s (%d)\n", strerror(errno), errno);
        return -1;
    }
    else
    {
        printf("\n -- PDDU INFO RESET - PDDU INFO Written iResult = %d \n", iResult);
    }

    dumphex((char *)msg_pddau_info_packet, 0x27, 16);

    return 0;
}

_PDDAU::_PDDAU(void)
{
    pddau_index = 0;
}

/**
 * Print the IP Address of the PDDAU.
 */
void _PDDAU::parseIPAddressesFromJson(const char *_filename)
{
    int lidxA = 0;
    const ST_CHAR *ps8configfile = "LUConfig.json";

	if(!parseLUConfig(ps8configfile, &pdsluConfig))
	{
        printf("[pddau.c] - Error parsing 'LUConfig.json'\n");
	}
	else
	{
		printf("-----\n");
		printf("----------------\n");
		printf("[pddau.c] - SUCCESS parsing 'LUConfig.json'\n");
		printf("[pddau.c] - LU Count :  %d\n\n", pdsluConfig.s8numLUs);
        printf("----------------\n");
        printf("-----\n");

        for (ST_INT i = 0; i < pdsluConfig.s8numLUs; i++)
        {
            //g_p61850IfcInfo->ps8IPAddr
            if (strcmp(pdsluConfig.LUList[i].ps8luIP, g_p61850IfcInfo->ps8IPAddr) == 0)
            {
                printf("[pddau.c] - #################################################\n  Found IP : %s\n", pdsluConfig.LUList[i].ps8luIP);
                for (int j = 0; j < pdsluConfig.LUList[i].s8numLNs; j++)
                {
                    // printf("  LN IP Address: %s Name: %s ID : %d\n", pdsluConfig.LUList[i].LNList[j].ps8ip_in, pdsluConfig.LUList[i].LNList[j].ps8name, pdsluConfig.LUList[i].LNList[j].s16id_ln);
                    if(strstr(pdsluConfig.LUList[i].LNList[j].ps8name, "SPDC1"))
                    {
                        //printf("  LN IP Address: %s Name: %s ID : %d\n", pdsluConfig.LUList[i].LNList[j].ps8ip_in, pdsluConfig.LUList[i].LNList[j].ps8name, pdsluConfig.LUList[i].LNList[j].s16id_ln);
                        strcpy(this->ip_address, pdsluConfig.LUList[i].LNList[j].ps8ip_in);
                        printf("[pddau.c] -  LN IP Address: %s Name: %s ID : %d\n", this->ip_address, pdsluConfig.LUList[i].LNList[j].ps8name, pdsluConfig.LUList[i].LNList[j].s16id_ln);

                        break;
                    }
                }
                break;
            }
        }
	}
}

/**
 * Get the Amplification settings from PDDAU. 
 * @param channel : This parameter is not effective yet.
 * @param sockfd : Already connected socket 
 */
int _PDDAU::getAmplification(int channel, int sockfd, MSG_RF_INFO_PACKET *msg_rf_info_packet)
{
    int idxA = 0;
    int iResult = 0;
    // MSG_RF_INFO_PACKET *msg_rf_info_packet = malloc(sizeof(MSG_RF_INFO_PACKET));

    msg_rf_info_packet->header.msg_id = MSG_RF_INFO_ID;
    msg_rf_info_packet->header.msg_type = MSG_READ_INFO_TYPE; //MSG_SEND_TYPE; // MSG_REQUEST_TYPE;
    msg_rf_info_packet->header.body_len = 0; //ntohs(0x0033);
    
    iResult = write(sockfd, msg_rf_info_packet, 4); //sizeof(msg_rf_info_packet));
    if (iResult == -1)
    {
        fprintf(stderr, "AMP GET INFO - send Error Occurred %s (%d)\n", strerror(errno), errno);
        return -1;
    }
    else
    {
        printf("\n -- AMP GET INFO - RF INFO Written iResult = %d \n", iResult);
    }

    iResult = read(sockfd, msg_rf_info_packet, 55);
    if(iResult == -1)
    {
        fprintf(stderr, "recv Error Occurred %s (%d)\n", strerror(errno), errno);
        return -1;			
    }
    else
    {
        printf("Info Read Success \n");
    }


    if (msg_rf_info_packet->header.msg_type != MSG_READ_INFO_RESPONSE_TYPE)
    {
        printf(" Info Packet Type Not Matched \n");
        return -1;
    }
    else
    {
        printf("Info Matched \n");

        //printf("\nch_enable = %d\namp[0] = %d\namp[1] = %d\namp[2] = %d\namp[3] = %d\n\n", msg_rf_info_packet->ch_enable, msg_rf_info_packet->amp[0], msg_rf_info_packet->amp[2], msg_rf_info_packet->amp[3], msg_rf_info_packet->amp[4]);
        printf("\n");
        printf("AMP SET --amp enable %d\n", msg_rf_info_packet->amp_enable);
        printf("\n");
        for(idxA=0;idxA<24;idxA++)
        {
            printf("amp[%d] = %d\n", idxA, msg_rf_info_packet->amp[idxA]);
        }

        printf("\n");

        for(idxA=0;idxA<6;idxA++)
        {
            printf("ch_type[%d] = %d\n", idxA, msg_rf_info_packet->ch_type[idxA]);
        }
    }

    return 0;
}

/**
 * Sets the Amplification of the PDDAU.
 * @param channel : This Parameter is not effective yet.
 * @param sockfd : A socket that is already connected with the PDDAU.
 * @param msg_rf_info_packet : MSG_RF_INFO_PACKET type instance. Contains all the settings of PDDAU.
 */
int _PDDAU::setAmplification(int channel, int sockfd, MSG_RF_INFO_PACKET *msg_rf_info_packet)
{
    int idxA = 0;
    int iResult = 0;
    //MSG_RF_INFO_PACKET *msg_rf_info_packet = malloc(sizeof(MSG_RF_INFO_PACKET));

    msg_rf_info_packet->header.msg_id = MSG_RF_INFO_ID;
    msg_rf_info_packet->header.msg_type = MSG_REQUEST_TYPE; //MSG_SEND_TYPE; // MSG_REQUEST_TYPE;
    msg_rf_info_packet->header.body_len = 0x3300;
    msg_rf_info_packet->amp_enable = 0x01;

    for(idxA=0;idxA < 24; idxA++)
    {
        msg_rf_info_packet->amp[idxA] = 5;
    }

    msg_rf_info_packet->amp[3] = 20;
    /*
    msg_rf_info_packet->ch_enable = 0x01;
    msg_rf_info_packet->ch_type[0] = 0xFF;
    msg_rf_info_packet->gatingch_enable = 0x00;
    msg_rf_info_packet->gatingth_enable = 0x00;
    msg_rf_info_packet->cal_enable = 0x00;
    
    msg_rf_info_packet->amp[0] = 1;
    msg_rf_info_packet->amp[1] = 1;
    msg_rf_info_packet->amp[2] = 1;
    
    */
    
    iResult = write(sockfd, msg_rf_info_packet, 55); //sizeof(msg_rf_info_packet));
    if (iResult == -1)
    {
        fprintf(stderr, "AMP SET - send Error Occurred %s (%d)\n", strerror(errno), errno);
        return -1;
    }
    else
    {
        printf("\n -- AMP SET - RF INFO Written iResult = %d \n", iResult);
    }

    iResult = read(sockfd, msg_rf_info_packet, 4);
    if(iResult == -1)
    {
        fprintf(stderr, "AMP SET - recv Error Occurred %s (%d)\n", strerror(errno), errno);
        return -1;
    }
    else
    {
        printf("AMP SET ACK Received Success \n");
    }

    if (msg_rf_info_packet->header.msg_type != MSG_REQUEST_ACK_TYPE)
    {
        printf(" AMP SET Info Packet Type Not Matched \n");
        return -1;
    }
    else
    {
        printf("AMP SET Info Matched \n");

        printf("ch_enable = %d\n", msg_rf_info_packet->ch_enable);
    }

    return 0;
}

#endif
