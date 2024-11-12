#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <sys/time.h>
#include <stdint.h>


typedef struct _oltc_sensor_data
{
    int _Mod;
    int _Beh;
    int _OilFil;
    int _MotDrvBlk;
    int _$VacCelAlm;
    int _OilFilTr;
    int _MotDrvAAlm;
    int _MotDrvAWrn;
    int _TorqAlm;
    int _TorqWrn;
    int _OpDurAlm;
    int _OpDurWrn;
    int _OpCntAlm;
    int _OpCntWrn;
    int _ArbPrtAlm;
    int _ArbPrtWrn;
    int _OilTmpDifAlm;
    int _OilTmpDifWrn;
    float _Torq;
    float _MotDrvA;
    float _AbrPrt;
    float _OpDur;
    float _OilTmpDif;
    float _OutTmp;
    float _MotDrvAPhsA;
    float _MotDrvAPhsB;
    float _MotDrvAPhsC;
}OLTC_SENSOR_DATA;

typedef struct _oltc_file_format
{
    uint32_t event_datetime;
    uint8_t  alert_level;
    uint32_t op_time;
    uint32_t number_moves;
    uint32_t max_current;
    uint32_t avg_current;
    uint32_t phase_number;
    uint8_t  single_data_len;
    uint32_t number_samples;
    uint16_t samplesA[115200];
    uint16_t samplesB[115200];
    uint16_t samplesC[115200];
}OLTC_FILE_FORMAT;

void CollectOltcDataFromSharedMem(OLTC_SENSOR_DATA *sdata);

void save_oltc_file(struct tm *tmm);

#ifdef __cplusplus
}
#endif
