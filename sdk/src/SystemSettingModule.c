/******************************************************************************************************
 **                     Copyright(C) 2014 SANION Co. Ltd.                    **
 **                  --------------------------------------                  **
 **                              WWW.SANION.COM                              **
 ******************************************************************************************************
    PROJECT     : UR-IED
 ******************************************************************************************************
    FILE NAME   : SystemSettingModule.c
    AUTHOR      : sootoo23
    DATE        : 2015.05.27
    REVISION    : V1.00
    DESCRIPTION : System Setting Module
 ******************************************************************************************************
    HISTORY     :
        2015-05-27 sootoo23 - Create
        2015-06-09 sootoo23 - IP, Mac Setting Add
        2015-07-17 sootoo23 - System Setting Module Initialize Func Add
        2016-08-31 sootoo23 - Partition Mount / UnMount Func Add
 ******************************************************************************************************/


#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "UR_SDK.h"

#include "Common.h"
#include "DBManager.h"
#include "SystemSettingModule.h"

void SSM_DisplayOsTime(void)
{
    FILE *read_fp;
	int chars_read;
    char buffer[20];

    read_fp = popen("date \"+%Y/%m/%d %X\"", "r");
    if(read_fp != NULL)
    {
        memset(buffer, '\0', sizeof(buffer));
        chars_read = fread(buffer, sizeof(char), 20+1, read_fp);
        pclose(read_fp);

        if(chars_read > 0)
        {
        	printf("%s", buffer);
        }
    }
}

void SSM_InitIPSetting(void)
{
	U8 IPAddr[20]={0,};
	char buffer[200];
	U32	u32IpAddrTemp=0,u32GatewayTemp=0,u32IPAddr=0,u32Gateway=0;	///< gateway

	if(DBM_GetSetting("DevInfo", "MPU IPAddr",(char *)IPAddr) == 0)
	{
		inet_pton( AF_INET, (char *)IPAddr, (void*)&u32IpAddrTemp);
	}
	u32IPAddr = ntohl(u32IpAddrTemp);

	if(DBM_GetSetting("DevInfo", "MPU Gateway",(char *)IPAddr) == 0)
	{
		inet_pton( AF_INET, (char *)IPAddr, (void*)&u32GatewayTemp);
	}
	u32Gateway = ntohl(u32GatewayTemp);
	
	sprintf(buffer, "ifconfig eth0 up;");
	system(buffer);

	sprintf(buffer, "ifconfig eth1 down;");
	system(buffer);

	usleep(1000000);

	sprintf(buffer, "ethtool -s eth0 speed 100 duplex full;");
	system(buffer);

	usleep(1000000);
		/*
	sprintf(buffer, "ifconfig eth0 0x%x up;", u32IPAddr);
	system(buffer);
*/
	////system("ifconfig eth0 192.168.10.102 up;");
		
	sprintf(buffer, "route add default gw 0x%x;", u32Gateway);
	system(buffer);

	printf("[%s:%d] eth0 IP:0x%x, GW:0x%x Change\n", __func__, __LINE__, u32IPAddr, u32Gateway);
}

U32 SSM_GetKernelIpAddr(void)
{
    FILE *read_fp;
    char buffer[1024], cmd[1024] = {0,};
	char* lineNum;
	int lineNumInt;
    int chars_read;
    U32 u32IpAddr;
	char bufferCut[1024];

	read_fp = popen("ifconfig | grep -n \"eth0\"", "r");

	if(read_fp == NULL) return -1;
	
	memset(buffer, '\0', sizeof(buffer));
	chars_read = fread(buffer, sizeof(char), 1024+1, read_fp);
	pclose(read_fp);

	//':' 기준으로 문자열 자르기
	lineNum = strtok(buffer, ":");

	//문자열을 정수로 변화
	lineNumInt = atoi(lineNum);

	snprintf(cmd, sizeof(cmd), "ifconfig | sed -n '%dp' | cut -d: -f2 | cut -d\" \" -f1", lineNumInt + 1);

    read_fp = popen(cmd, "r");
    if(read_fp == NULL) return -1;

    memset(buffer, '\0', sizeof(buffer));
    chars_read = fread(buffer, sizeof(char), 1024+1, read_fp);
    pclose(read_fp);

    if(chars_read <= 0) return -1;

	u32IpAddr = ntohl(inet_addr(buffer));

    return u32IpAddr;
}

S32 SSM_UpdateIpAddr(U32 u32IPAddr)
{
	char buffer[200];
	
	sprintf(buffer, "ethtool -s eth0 speed 100 duplex full;");
	system(buffer);

	usleep(1000000);
		
	sprintf(buffer, "ifconfig eth0 0x%x up;", u32IPAddr);
	system(buffer);

	printf("[%s:%d] eth0 IP:0x%x\n", __func__, __LINE__, u32IPAddr);
	return 0;
}

S32 SSM_UpdateSubnet(U32 u32SubNet)
{
	char buffer[200];

	sprintf(buffer, "ifconfig eth0 netmask 0x%x up;", u32SubNet);
	system(buffer);

	printf("Subnet Mask: 0x%x\n", u32SubNet);
	return 0;
}

S32 SSM_UpdateGateway(U32 u32GateWay)
{
	char buffer[200];

	sprintf(buffer, "route add default gw 0x%x;", u32GateWay);
	system(buffer);
	
	printf("Gateway : 0x%x\n", u32GateWay);
	return 0;
}
int SSM_LoadNetworkInfo(sqlite3* DBName,dsNetworkInfo* NetworkData)
{
	U8 IPAddr[20]={0,};
	U32 u32IpAddrTemp=0, u32Hex_IpAddr=0, ret = 0;
	
	ret |= DBM_GetSetting_DBName(DBName,"DevInfo", "MPU IPAddr", (char *)IPAddr);
	if(ret != 0 || (inet_pton( AF_INET, (char *)IPAddr, (void*)&u32IpAddrTemp) <= 0))
	{
		printf("Invalid Ip Address: %s\n", (char *)IPAddr);
		return -1;
	}
	u32Hex_IpAddr = ntohl(u32IpAddrTemp);
	NetworkData[0].m_u32IPAddr = u32Hex_IpAddr;

	ret |= DBM_GetSetting_DBName(DBName,"DevInfo", "MPU Subnet", (char *)IPAddr);
	if(ret != 0 || inet_pton( AF_INET, (char *)IPAddr, (void*)&u32IpAddrTemp) <= 0)
	{
		printf("Invalid Subnet Address: %s\n", (char *)IPAddr);
		return -1;
	}
	u32Hex_IpAddr = ntohl(u32IpAddrTemp);
	NetworkData[0].m_u32Subnet = u32Hex_IpAddr;

	ret |= DBM_GetSetting_DBName(DBName,"DevInfo", "MPU Gateway", (char *)IPAddr);
	if(ret != 0 || inet_pton( AF_INET, (char *)IPAddr, (void*)&u32IpAddrTemp) <= 0)
	{
		printf("Invalid Gateway Address: %s\n", (char *)IPAddr);
		return -1;
	}
	u32Hex_IpAddr = ntohl(u32IpAddrTemp);
	NetworkData[0].m_u32Gateway= u32Hex_IpAddr;

	ret |= DBM_GetSetting_DBName(DBName,"DevInfo", "NTP IPAddr", (char *)IPAddr);
	if(ret != 0 || (inet_pton( AF_INET, (char *)IPAddr, (void*)&u32IpAddrTemp) <= 0))
	{
		printf("Invalid Ip Address: %s\n", (char *)IPAddr);
		return -1;
	}
	u32Hex_IpAddr = ntohl(u32IpAddrTemp);
	NetworkData[1].m_u32IPAddr = u32Hex_IpAddr;

	ret |= DBM_GetSetting_DBName(DBName,"DevInfo", "NTP Subnet", (char *)IPAddr);
	if(ret != 0 || inet_pton( AF_INET, (char *)IPAddr, (void*)&u32IpAddrTemp) <= 0)
	{
		printf("Invalid Subnet Address: %s\n", (char *)IPAddr);
		return -1;
	}
	u32Hex_IpAddr = ntohl(u32IpAddrTemp);
	NetworkData[1].m_u32Subnet = u32Hex_IpAddr;

	ret |= DBM_GetSetting_DBName(DBName,"DevInfo", "NTP Gateway", (char *)IPAddr);
	if(ret != 0 || inet_pton( AF_INET, (char *)IPAddr, (void*)&u32IpAddrTemp) <= 0)
	{
		printf("Invalid Gateway Address: %s\n", (char *)IPAddr);
		return -1;
	}
	u32Hex_IpAddr = ntohl(u32IpAddrTemp);
	NetworkData[1].m_u32Gateway= u32Hex_IpAddr;
	
	ret |= DBM_GetSetting_DBName(DBName,"DevInfo", "PDDAU IPAddr", (char *)IPAddr);
	if(ret != 0 || (inet_pton( AF_INET, (char *)IPAddr, (void*)&u32IpAddrTemp) <= 0))
	{
		printf("Invalid Ip Address: %s\n", (char *)IPAddr);
		return -1;
	}
	u32Hex_IpAddr = ntohl(u32IpAddrTemp);
	NetworkData[2].m_u32IPAddr = u32Hex_IpAddr;

	ret |= DBM_GetSetting_DBName(DBName,"DevInfo", "PDDAU Subnet", (char *)IPAddr);
	if(ret != 0 || inet_pton( AF_INET, (char *)IPAddr, (void*)&u32IpAddrTemp) <= 0)
	{
		printf("Invalid Subnet Address: %s\n", (char *)IPAddr);
		return -1;
	}
	u32Hex_IpAddr = ntohl(u32IpAddrTemp);
	NetworkData[2].m_u32Subnet = u32Hex_IpAddr;

	ret |= DBM_GetSetting_DBName(DBName,"DevInfo", "PDDAU Gateway", (char *)IPAddr);
	if(ret != 0 || inet_pton( AF_INET, (char *)IPAddr, (void*)&u32IpAddrTemp) <= 0)
	{
		printf("Invalid Gateway Address: %s\n", (char *)IPAddr);
		return -1;
	}

	
	u32Hex_IpAddr = ntohl(u32IpAddrTemp);
	NetworkData[2].m_u32Gateway= u32Hex_IpAddr;

	return 0;
}

