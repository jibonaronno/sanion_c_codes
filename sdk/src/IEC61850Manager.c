/******************************************************************************
 **                     Copyright(C) 2017 SANION Co. Ltd.                    **
 **                  --------------------------------------                  **
 **                              WWW.SANION.COM                              **
 ******************************************************************************
    PROJECT     : UR-IED
 ******************************************************************************
    FILE NAME   : IEC61850Manager.c
    AUTHOR      : sootoo23
    DATE        : 2017.10.16
    REVISION    : V1.00
    DESCRIPTION : IEC 61850 & MPU IF (Function, Type Define)
 ******************************************************************************
    HISTORY     :
        2017-10-16 Create
 ******************************************************************************/
#include <stdio.h>
#include <pthread.h>
#include "iec61850_sdk.h"

void* IEC61850_ProcessMain(void* argv)
{
    printf(" ==========================================IEC61850_ProcessMain PID: %lu\n", pthread_self());

	Run_61850();
	pthread_exit(NULL);
	printf("[%s] Task Close.\n", __func__);
	return 0;
}

