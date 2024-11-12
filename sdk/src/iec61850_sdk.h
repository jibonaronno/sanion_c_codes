/******************************************************************************
 **                     Copyright(C) 2019 SANION Co. Ltd.                    **
 **                  --------------------------------------                  **
 **                              WWW.SANION.COM                              **
 ******************************************************************************
    PROJECT     : UR-IED
 ******************************************************************************
    FILE NAME   : iec61850_sdk.h
    AUTHOR      : jlee431
    DATE        : 2019.12.27
    REVISION    : V1.00
    DESCRIPTION : IEC 61850 
 ******************************************************************************
    HISTORY     :
        2019-12-27 Create
 ******************************************************************************/


#ifndef __IEC61850_SDK_HEADER_H
#define __IEC61850_SDK_HEADER_H

#ifdef __cplusplus
extern "C" {
#endif


// char* gps8IEDName;

/*******************************************************************************
    IEC61850 Support Function
*******************************************************************************/
void Run_61850(void);
int Kill_61850(void);
unsigned char Init_61850(void);
extern void UpdateDIVMDFromBitmap(int* ps32mapping, unsigned char u8val);
//extern void UpdateEventDOArray(unsigned long u32triggeridx);
void Update61850GCBArray(void);
void* GetVmdAddr(char* ps8leafname);
void* Get61850IfcInfo(void);
void UpdateVMDWithNewVal(char* ps8leaftype, void* pvmdptr, unsigned int* u32newval);

#ifdef __cplusplus
}
#endif /* defined (__cplusplus) */

#endif
