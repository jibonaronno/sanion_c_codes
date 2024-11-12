/******************************************************************************
 **                     Copyright(C) 2017 SANION Co. Ltd.                    **
 **                  --------------------------------------                  **
 **                              WWW.SANION.COM                              **
 ******************************************************************************
    PROJECT     : UR-IED
 ******************************************************************************
    FILE NAME   : MSManger.h
    AUTHOR      : sootoo23
    DATE        : 2017.04.05
    REVISION    : V1.00
    DESCRIPTION : Measurement Manager & Calibration..
 ******************************************************************************
    HISTORY     :
        2017-04-05 Create
 ******************************************************************************/
#ifndef MSMANGER_H_
#define MSMANGER_H_

#include "UR_SDK.h"

void* MSM_Factor(int argc, void* argv[]);
void* MSM_SetAutoOCTCurrFactor(int argc, void* argv[]);
void* MSM_SetAutoBCTCurrFactor(int argc, void* argv[]);
void* MSM_SetAutoVoltFactor(int argc, void* argv[]);
void* MSM_SetACDAutoFactor(int argc, void* argv[]);
void MSM_SetOffsetAutoFactor(U8 u8CTPT);
void MSM_ResetOffsetAutoFactor(void);
void* MSM_FactorReset(void);

void* Measurment_SetTDFactor(int argc, void* argv[]);
void* Measurment_PrintTDFactor(void);


#endif /* MSMANGER_H_ */
