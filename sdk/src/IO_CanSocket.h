/******************************************************************************
 **                     Copyright(C) 2017 SANION Co. Ltd.                    **
 **                  --------------------------------------                  **
 **                              WWW.SANION.COM                              **
 ******************************************************************************
    PROJECT     : UR-IED MPU
 ******************************************************************************
    FILE NAME   : IO_CanSocket.h
    AUTHOR      : sootoo23
    DATE        : 2017.02.28
    REVISION    : V1.00
    DESCRIPTION : IO CAN Tx/Rx
 ******************************************************************************
    HISTORY     :
        2017-02-28 �ʾ� �ۼ�
        2017-03-07 CAN Packet ���� �� ����.
 ******************************************************************************/

#ifndef IOCANSOCKET_H_
#define IOCANSOCKET_H_

#ifdef __cplusplus
extern "C" {
#endif


void IO_CAN_SOCKET_INIT();
int IO_CAN_GetDevice(int devceNum);
int IO_CAN_TX(int canCH, unsigned int CanID, char *pData, unsigned int DataSize);
int IO_CAN_RX(void *pFrameData);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* IOCANSOCKET_H_ */

