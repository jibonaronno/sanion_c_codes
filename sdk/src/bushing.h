#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <sys/time.h>
#include <stdint.h>

typedef struct __sbsh_sesnsor_data
{
    float _Vol;
    float _It;
    float _Ir;
    float _Ic;
    float _PF;
    int _Mod;
    int _Beh;
    float _voltage_angle;
    float _current_angle;
    float _capacitance;
    float _React;
    float _AbsReact;
    float _RefReact;
    float _RefPF;
    float _RefV;
    float _freq;
    float _LosFact; // tan delta
    float _DspA;    // Displacement Current : Apparent current at measuring tap
    float _LeakA;   // Leakage Current : Active current at measuring tap
} SBSH_SENSOR_DATA;

typedef struct __sbsh_data_cisco
{
    char LNName[200];
    int Mod;
    int Beh;
    int Health;
    char *NamPlt;
    char *EEName;
    int BshAlm;
    int EvtTransF;
    int RTTransF;
    int OpCnt;
    int React;
    int AbsReact;
    int LostFact;
    int Vol;
    int DspA;
    int LeakA;
    int RefReact;
    int RefPF;
    int RefV;
}SBSH_DATA_CISCO;

void SbshSensorCollect(void);
void ProcessBushingSensorData();

#ifdef __cplusplus
}
#endif
