
#include <math.h>
#include "global.h"
#include "oltc.h"

#define OLTC_FILE_SIZE  691230

OLTC_FILE_FORMAT oltc_file_data;

void save_oltc_file(struct tm *tmm)
{
    char time_string[45];
    char filename[60];
    size_t size_file =0;

    int lidxA = 0;

    int16_t oct1[DATA_QUEUE_SIZE];
    int16_t oct2[DATA_QUEUE_SIZE];
    int16_t oct3[DATA_QUEUE_SIZE];

    uint8_t fbuf[OLTC_FILE_SIZE];
    uint8_t *ubuf = (uint8_t *)&oltc_file_data;

    time_t _now;

    _now = time(0) + (9 * 3600);

    oltc_file_data.event_datetime = _now;
    oltc_file_data.alert_level = 5;
    oltc_file_data.op_time = 2;
    oltc_file_data.number_moves = 30;
    oltc_file_data.max_current = 100;
    oltc_file_data.avg_current = 88;
    oltc_file_data.phase_number = 3;
    oltc_file_data.single_data_len = 320400;
    oltc_file_data.number_samples = 115200;

    for(lidxA=0;lidxA<DATA_QUEUE_SIZE;lidxA++)
    {
        oct1[lidxA] = (int16_t)(g_pAISharedMem->m_dsMeasQueueData.f32OCT1mag[lidxA] * 1000);
        oct2[lidxA] = (int16_t)(g_pAISharedMem->m_dsMeasQueueData.f32OCT2mag[lidxA] * 1000);
        oct3[lidxA] = (int16_t)(g_pAISharedMem->m_dsMeasQueueData.f32OCT3mag[lidxA] * 1000);
    }

    memcpy(oltc_file_data.samplesA, oct1, DATA_QUEUE_SIZE);
    memcpy(oltc_file_data.samplesB, oct2, DATA_QUEUE_SIZE);
    memcpy(oltc_file_data.samplesC, oct3, DATA_QUEUE_SIZE);

    strftime(time_string, sizeof(time_string), "%Y%m%d%H%M%S", tmm);
    sprintf(filename, "/data/COMTRADE/01_40_%s%03d.dat", time_string, 111);
	printf("--time string : %s\n", time_string);
    printf("--tmm->hour : %d\n", tmm->tm_hour);

    for(lidxA=0;lidxA<100;lidxA++)
    {
        //// printf("Phase A : %f -- %d\n", g_pAISharedMem->m_dsMeasQueueData.f32OCT1mag[lidxA], oct1[lidxA]);
    }

    //// printf("Size Of OLTC_FILE_FORMAT %s : %d \r\n", filename, sizeof(OLTC_FILE_FORMAT));


    FILE *file;
    file = fopen(filename, "w+b");
    size_file = fwrite(&oltc_file_data, 1 /*sizeof(unsigned char)*/, 691236, file);
    //printf("fwrite->%d\n", size_file);
    fclose(file);

}

void CollectOltcDataFromSharedMem(OLTC_SENSOR_DATA *sdata)
{
    int i;
    int j = 0;
    for(i=0;i<3;i++)
    {
        sdata[i]._MotDrvAPhsA = g_pAISharedMem->m_dsMeasValueData.m_dsOCTDataInfo.m_f32Mag[j]; //g_pAISharedMem->m_dsMeasValueData.m_dsBCTDataInfo.m_f32Mag[i];
        j+=1;
        sdata[i]._MotDrvAPhsB = g_pAISharedMem->m_dsMeasValueData.m_dsOCTDataInfo.m_f32Mag[j];
        j+=1;
        sdata[i]._MotDrvAPhsC = g_pAISharedMem->m_dsMeasValueData.m_dsOCTDataInfo.m_f32Mag[j];
        j+=1;
    }
}
