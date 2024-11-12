/******************************************************************************
 **                     Copyright(C) 2017 SANION Co. Ltd.                    **
 **                  --------------------------------------                  **
 **                              WWW.SANION.COM                              **
 ******************************************************************************
    PROJECT     : UR-IED MPU
 ******************************************************************************
    FILE NAME   : IO_CanSocket.c
    AUTHOR      : sootoo23
    DATE        : 2017.02.28
    REVISION    : V1.00
    DESCRIPTION : IO CAN Tx/Rx
 ******************************************************************************
    HISTORY     :
        2017-02-28 초안 작성
        2017-03-07 CAN Packet 설계 및 구현.
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/can.h>
#include <sys/socket.h>
#include <unistd.h>

#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <fcntl.h>
#include <poll.h>

#include "ConsoleManager.h"
#include "IO_CanSocket.h"

#define CAN_RECV_MASK 0x1F

int CanSock[2];

void IO_CAN_SOCKET_INIT()
{
	struct sockaddr_can addr[2];
	struct can_frame frame;
	struct can_filter 	rfilter[19];
	int canFlag = 0, i=0, can_id =0;

	//Can Init
	CanSock[0] = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	CanSock[1] = socket(PF_CAN, SOCK_RAW, CAN_RAW);

	//Nonblocking Setting
	canFlag = fcntl(CanSock[0], F_GETFL, 0);
	fcntl(CanSock[0], F_SETFL, canFlag | O_NONBLOCK);
	canFlag = fcntl(CanSock[1], F_GETFL, 0);
	fcntl(CanSock[1], F_SETFL, canFlag | O_NONBLOCK);

	addr[0].can_family = AF_CAN;
	addr[0].can_ifindex = if_nametoindex("can0");
	addr[1].can_family = AF_CAN;
	addr[1].can_ifindex = if_nametoindex("can1");

	bind(CanSock[0], (struct sockaddr *)&addr[0], sizeof(struct sockaddr_can));
	bind(CanSock[1], (struct sockaddr *)&addr[1], sizeof(struct sockaddr_can));

	can_id = 0x11;
	for(i=0; i<16; i++, can_id++)
	{
		rfilter[i].can_id   = can_id;
		rfilter[i].can_mask = CAN_RECV_MASK;
	}
	
	setsockopt(CanSock[0], SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));
	setsockopt(CanSock[1], SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));

	//addr.can_family = AF_CAN;
	//addr.can_ifindex = 0; // 0 -> all interface
}

int IO_CAN_GetDevice(int deviceNum)
{
	return CanSock[deviceNum];
}

int IO_CAN_TX(int canCH, unsigned int CanID, char *pData, unsigned int DataSize)
{
	int nbytes = -1, i=0;
	struct can_frame frame;
	unsigned char u8TxBuf[8] = {0,};

	//send can msg
	frame.can_id = CanID;

	if(DataSize <= 8)
	{
		frame.can_dlc = DataSize;
		
		//Single Packet
		//u8TxBuf[DATA_SEQ] = 0;

		if(pData)
			memcpy(u8TxBuf, pData, DataSize);
	}
	else
	{
		frame.can_dlc = 8;
		
		//Multi Packet의 경우 Data Seq를 직접 data에 
		//실어 준다. 때문에 그냥 memcpy하여 사용한다.
		memcpy(u8TxBuf, pData, sizeof(u8TxBuf));
	}

	memcpy(frame.data, u8TxBuf, frame.can_dlc);

	if(CDM_GetViewMode(CDM_DISP_VIEW_IO))
	{
		printf("[%s:%d] ID %x TX %d byte (hex)>>\n", __func__, __LINE__,frame.can_id, frame.can_dlc);
		for(i=0; i<frame.can_dlc; i++)
		{
			printf(" %02X", frame.data[i]);
		}
		printf("\n");
	}

	nbytes = write(CanSock[canCH], &frame, sizeof(frame));
	//printf("Write 0x%x byte.\n", nbytes);

	return nbytes;
}

int IO_CAN_RX(void* pFrameData)
{
	int nbytes = -1, ret = -1, i=0;
	struct can_frame frame;
	struct pollfd pollfds[2];

	/*
	*	sootoo23 - 180830: IOM 100us 마다 read()동작은
	*	너무 리소스 낭비가 크다.
	*	poll로 kernel에 해당 fd에 data가 들어왓는지 
	*	확인하는 동작으로 부하를 줄인다. (6% CPU Load 감소)
	*	blocking 처리를 하게되면 CPU 부하가 20% 감소 하지만,
	*	100us 마다 Logic DO를 항상 처리해야 하므로 Nonblocking을 해야한다.
	*	blocking 될 경우 해당 Task가 리소스 점유를 너무 많이 하게된다.
	*/
	pollfds[0].fd = CanSock[0];
	pollfds[0].events = POLLIN;
	pollfds[1].fd = CanSock[1];
	pollfds[1].events = POLLIN;

	/*
	*	poll param: (fd, nfds, tmout)
	*	timeout-> -1: blocking, 0: nonblocking, 0>: timeout setting (ms)
	*	return ->  -1: err, 0:timeout, 0>: read fd count
	*/
	ret = poll(pollfds, 2, 0);
	if(ret == -1)
	{
		printf("[%s:%d] ERR!\n", __func__, __LINE__);
	}
	else if(ret > 0)
	{
		if(pollfds[0].revents & POLLIN)
			nbytes = read(pollfds[0].fd, &frame, sizeof(frame));
		else if(pollfds[1].revents & POLLIN)
			nbytes = read(pollfds[1].fd, &frame, sizeof(frame));

		if(nbytes > 0)
		{
			//printf("ID=0x%x DLC=0x%x data[0]=0x%x data[1]=0x%x\n", frame.can_id, frame.can_dlc, frame.data[0], frame.data[1]);
			memcpy((struct can_frame *)pFrameData, &frame, sizeof(struct can_frame));
			if(CDM_GetViewMode(CDM_DISP_VIEW_IO))
			{
				printf("[%s:%d] [ID %x] RX %d byte (hex)>>\n", __func__, __LINE__,frame.can_id , frame.can_dlc);
				for(i=0; i<frame.can_dlc; i++)
				{
					printf(" %02X", frame.data[i]);
				}
				printf("\n\n");
			}
		}
	}

	return nbytes;
}


