/******************************************************************************
 **                     Copyright(C) 2016 SANION Co. Ltd.                    **
 **                  --------------------------------------                  **
 **                              WWW.SANION.COM                              **
 ******************************************************************************
    PROJECT     : UR-IED MPU
 ******************************************************************************
    FILE NAME   : wdt_ds1832.h
    AUTHOR      : sootoo23
    DATE        : 2016-11-17
    REVISION    : V1.00
    DESCRIPTION : UR-IED Watch Dog Timer Driver
 ******************************************************************************
    HISTORY     :
    2015-11-17 �ʾ��ۼ�
 ******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

void WDT_Init(void);
 void WDT_DeInit(void);
 void WDT_Reset(void);
 void WDT_AppStart(void);
 void WDT_AppStop(void);
 void WDT_InputSignal(void);

#ifdef __cplusplus
}
#endif
 
