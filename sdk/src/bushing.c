
#include <math.h>
#include "global.h"
#include "bushing.h"

SBSH_DATA_CISCO sbsh_data_cisco;
SBSH_SENSOR_DATA sbsh_sensors[6];

void ColllectSbshDataFromSharedMem(SBSH_SENSOR_DATA *sdata)
{
    int i;
    for(i=0;i<6;i++)
    {
        sdata[i]._It = g_pAISharedMem->m_dsMeasValueData.m_dsBCTDataInfo.m_f32Mag[i];
        sdata[i]._Vol = g_pAISharedMem->m_dsMeasValueData.m_dsPTDataInfo.m_f32Mag[i];
    }
}

void SbshSensorCollect(void)
{
    float ptAng[MAX_OCT_COUNT];
	float fBctAng[MAX_BCT_COUNT];
    float fPowerFactor[MAX_BCT_COUNT];
    double pfRadians[MAX_BCT_COUNT];
    float fLossFactor[MAX_BCT_COUNT];
    double lfRadians[MAX_BCT_COUNT];
    float fDiffAngle[MAX_BCT_COUNT];
    int i = 0;
    for(i=0;i<6;i++)
    {
        sbsh_sensors[i]._capacitance = 0.000000000001 * 1000; // Put to a fixed value as Capacitance is not used in the calculation yet.
        sbsh_sensors[i]._Vol = g_pAISharedMem->m_dsMeasValueData.m_dsPTDataInfo.m_f32Mag[i];
        sbsh_sensors[i]._It = g_pAISharedMem->m_dsMeasValueData.m_dsBCTDataInfo.m_f32Mag[i];
        sbsh_sensors[i]._voltage_angle = g_pAISharedMem->m_dsPhasorDataInfo._pt[i];
        sbsh_sensors[i]._current_angle = g_pAISharedMem->m_dsPhasorDataInfo._bct[i];
        sbsh_sensors[i]._Mod = 1;
        sbsh_sensors[i]._Beh = 0;
        sbsh_sensors[i]._freq = 60;

        /* Getting Theta Voltage Angle - Current Angle */
        fDiffAngle[i] = sbsh_sensors[i]._voltage_angle - sbsh_sensors[i]._current_angle;

        /* Convert Degree to Radian */
        pfRadians[i] = fDiffAngle[i] * M_PI / 180.0;

        /* Getting Power Factor Cos Theta */
        fPowerFactor[i] = cos(pfRadians[i]);

        /* Getting Delta = 90 - Theta in Radian */
        lfRadians[i] = (90 - fDiffAngle[i]) * M_PI / 180.0;

        /* Getting Tan Delta (Loss Factor) */
        fLossFactor[i] = tan(lfRadians[i]);

        /* Assigning the values for MMS Packets. */
        sbsh_sensors[i]._LosFact = fLossFactor[i];
        sbsh_sensors[i]._PF = fPowerFactor[i];
        sbsh_sensors[i]._Ir = fPowerFactor[i] * sbsh_sensors[i]._It;
        sbsh_sensors[i]._Ic = sbsh_sensors[i]._It - sbsh_sensors[i]._Ir;
        sbsh_sensors[i]._React = sbsh_sensors[i]._Vol / sbsh_sensors[i]._Ic;

    }
}

void SbshDataInit(void)
{
    sbsh_data_cisco.AbsReact = 1;
    sbsh_data_cisco.Beh = 2;
    sbsh_data_cisco.BshAlm = 0;
    sbsh_data_cisco.DspA = 12;
    sbsh_data_cisco.Health = 1;
    sbsh_data_cisco.LeakA = 4;
    sbsh_data_cisco.LostFact = 1;
    sbsh_data_cisco.Mod = 1;
    sbsh_data_cisco.OpCnt = 122;
    sbsh_data_cisco.React = 22;
    sbsh_data_cisco.RefReact = 12;
    sbsh_data_cisco.RefPF = 25;
    sbsh_data_cisco.RTTransF = 1;
    sbsh_data_cisco.Vol = 100;
}

void _ProcessBushingSensorData()
{
    // _sensor_data->_It = g_pAISharedMem->m_dsMeasValueData.m_dsBCTDataInfo.m_f32Mag[i];

    //_sensor_data->_React = 1/(2 * _sensor_data->_capacitance * _sensor_data->_freq * 3.14 );
    //_sensor_data->_React = _sensor_data->_Vol / _sensor_data->_Ic;
    //_sensor_data->_Ic = _sensor_data->_Vol / _sensor_data->_React;
    //_sensor_data->_Ir = _sensor_data->_It - _sensor_data->_Ic;
    // if(_sensor_data->_Ic > 0)
    // {
    //     _sensor_data->_LosFact = _sensor_data->_Ir / _sensor_data->_Ic;
    // }
    // else
    // {
    //     _sensor_data->_LosFact = 1;
    // }

    SbshSensorCollect();
}

void ProcessBushingSensorData()
{
    int i=0;
    ColllectSbshDataFromSharedMem(sbsh_sensors);
    _ProcessBushingSensorData();
}
