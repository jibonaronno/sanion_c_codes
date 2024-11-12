# Code segment description

### lib61850 Usage
Untill Feb 3 

LU code is calling functions from lib61850 to parse the leaves defined in the cid or csv file. 

Here is the global variable for accessing most features of the Lib:
```C
extern ds61850IfcInfo* g_p61850IfcInfo;
```
Primarily declared in main.c file and externed at other places. 
File cidcsvmanager.c .
Function from this file used in sensorcomm.c as :
```C
GetKeyFromLeafString(leaflist->lines[lidx], leaf_key, g_p61850IfcInfo->ps8LDname);
```
There could be more than 3000 keys in g_p61850IfcInfo mapping table. In cidcsvmanager.c the mapping table accessed as below:

```C
void SearchLeafsX(char *keyword, LEAFLIST *leaf_list)
{
    int idx;
    char *p;
    Free(leaf_list);
    for(idx=0;idx<g_p61850IfcInfo->s32leafnum;idx++)
    {
        p = strstr(g_p61850IfcInfo->p61850mappingtbl[idx].ps8leafname, keyword);
        if(p)
        {
            InsertLine(g_p61850IfcInfo->p61850mappingtbl[idx].ps8leafname, leaf_list);
        }
    }
}
```
This function SearchLeafsX is called in sensorcomm.c inside void sbcr_acq_modbus_run(void) function as below:

```C
            SearchLeafsX("SBSH1", &sbsh1_leaflist[0]);
			SearchLeafsX("SBSH2", &sbsh1_leaflist[1]);
			SearchLeafsX("SBSH3", &sbsh1_leaflist[2]);
			SearchLeafsX("SBSH4", &sbsh1_leaflist[3]);
			SearchLeafsX("SBSH5", &sbsh1_leaflist[4]);
			SearchLeafsX("SBSH6", &sbsh1_leaflist[5]);
			PrintLeafs(&sbsh1_leaflist[0]);
			SearchLeafs("SLTC1", &oltc_leaflist[0]);
			SearchLeafs("SLTC2", &oltc_leaflist[1]);
			SearchLeafs("SLTC3", &oltc_leaflist[2]);
			PrintLeafs(&oltc_leaflist[0]);
```

So total 9 list is created for SBSH1, SBSH2, SBSH3, SBSH4, SBSH5. SBSH6, SLTC1, SLTC2 and SLTC3 to the corresponding global variables. Then These variables are used to access the specific keys by keys end string name. 

Following function in sensorcomm.c gets the full key string by end name from any given LEAFLIST array:

```C
void GetKeyStringByEndName(LEAFLIST *leaflist, char *endName, char *matchedKey)
```

### Acq.h - Generation of ACQ event file

Each Event File is 30050 bytes long. First 98 byte is header. In Acq.h CBEVENT 
is the structure that ACQ hardware is giving through Modbus-TCP. It is 68 byte 
long. But the header must be 98 byte long. So CBFILE structure contains a Full 
File data structure. <br>
```C
typedef struct _CB_FILE
{
    uint8_t  event_type;
    uint32_t event_datetime;
    uint32_t event_millisec;
    uint8_t  alert_level;
    float    contact_duty_A;
    float    contact_duty_B;
    float    contact_duty_C;
    float    accum_contact_duty_A;
    float    accum_contact_duty_B;
    float    accum_contact_duty_C;
    float    coil_integral_t1;
    float    coil_max_current_t1;
    float    coil_female_time_t1;
    float    coil_integral_t2;
    float    coil_max_current_t2;
    float    coil_female_time_t2;
    float    coil_integral_close;
    float    coil_max_current_close;
    float    coil_female_time_close;
    float    contact_optime_A;
    float    contact_optime_B;
    float    block_close_time_A;
    float    block_close_time_B;
    float    block_close_time_C;
    uint32_t op_cnt;
    uint16_t smp_per_cyc;
    uint16_t cyc_count;

    CB_DATA  cb_data;
}CB_FILE;
```
So we can fill up CBEVENT from the ACQ hardware but we still need to fill up all 
the fields of CB_FILE header elements.
___

## OLTC File Transfer ::
OLTC data file transfer is applied by oltc.c and oltc.h . Data structure for the file is as below. 

```C
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
```
Here is the function that saves files for OLTC data from DSP shared memory: 

```C
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

    oltc_file_data.event_datetime = 3276887;
    oltc_file_data.alert_level = 5;
    oltc_file_data.op_time = 12;
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
    sprintf(filename, "/COMTRADE/01_40_%s%03d.dat", time_string, 111);
    printf("--time string : %s\n", time_string);
    printf("--tmm->hour : %d\n", tmm->tm_hour);

    for(lidxA=0;lidxA<100;lidxA++)
    {
        printf("Phase A : %f -- %d\n", g_pAISharedMem->m_dsMeasQueueData.f32OCT1mag[lidxA], oct1[lidxA]);
    }

    printf("Size Of OLTC_FILE_FORMAT %s : %d \r\n", filename, sizeof(OLTC_FILE_FORMAT));

    FILE *file;
    file = fopen(filename, "w+b");
    //size_file = fwrite(&cb_file, 1 /*sizeof(unsigned char)*/, 30050, file);
    size_file = fwrite(&oltc_file_data, 1 /*sizeof(unsigned char)*/, 691236, file);
    printf("fwrite->%d\n", size_file);
    fclose(file);

}
```
ACQ aslo save files with simalar function. Here data is coming from the DSP shared memory. 

```C
for(lidxA=0;lidxA<DATA_QUEUE_SIZE;lidxA++)
{
    oct1[lidxA] = (int16_t)(g_pAISharedMem->m_dsMeasQueueData.f32OCT1mag[lidxA] * 1000);
    oct2[lidxA] = (int16_t)(g_pAISharedMem->m_dsMeasQueueData.f32OCT2mag[lidxA] * 1000);
    oct3[lidxA] = (int16_t)(g_pAISharedMem->m_dsMeasQueueData.f32OCT3mag[lidxA] * 1000);
}
```
This g_pAISharedMem variable is declared as external in global.h file and main declaration is in main.h file. 

Observe that the data array from the shared mem is 32 bit float. I had to convert it to 16bit integer for file structure compatibility.



