
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>
#include <dirent.h>
#include <time.h>
#include <limits.h>
//#include <curses.h>
#include <dirent.h>
#include "modbus.h"
#include "Acq.h"

#define _FILE_SIZE 30050

uint16_t setsA[30];
uint16_t flag_Records[2];
u16 tbl[3664];
u16 t1_buf[3664];
u16 t2_buf[3664];
u16 close_buf[3664];
u16 currA_buf[3664];
u16 currB_buf[3664];
u16 currC_buf[3664];
u16 init_cont_buf[3664];
u8 *init_buf = (u8 *)init_cont_buf;
modbus_t *busA;
bool_type is_modbus_opened;

int openAcqBus(void)
{
    busA = modbus_new_tcp("192.168.247.100", 100);
    is_modbus_opened = FALSE;
    if (modbus_connect(busA) == -1) 
    {
        //fprintf(stderr, "Modbus Connection failed: %s\n", modbus_strerror(errno));
        modbus_free(busA);
        return -1;
    }
    is_modbus_opened = TRUE;
    return 0;
}

void closeModbus(void)
{
    if(is_modbus_opened == TRUE)
    {
        modbus_close(busA);
        modbus_free(busA);
        is_modbus_opened = FALSE;
    }
}

int readSingleBlock(int sz, u16 address, uint16_t *mems)
{
    int rc;

    if(is_modbus_opened != TRUE)
    {
        printf("ERROR modbus did not opened yet");
        return -1;
    }

    rc = modbus_read_registers(busA, address, sz, mems);
    if (rc == 0) 
    {
        printf("ERROR modbus_read_registers multiple (%d)\n", rc);
        printf("Address = %d\n", address);
        return -1;
    }
    //printf("R.");
    return 0;
}

int writeSingleBlock(int sz, u16 address, uint16_t *mems)
{

    if(is_modbus_opened != TRUE)
    {
        printf("ERROR modbus did not opened yet");
        return -1;
    }
    
    if(modbus_write_registers(busA, address, sz, mems) == -1)
    {
        return -1;
    }
    
    return 0;
}

int readMultiBlock(int blkSize, int numberOfBlocks, u16 address, uint16_t *mems)
{
    int i;
    int incr = 0;

    if(is_modbus_opened != TRUE)
    {
        printf("ERROR modbus did not opened yet");
        return -1;
    }

    for(i=0;i<numberOfBlocks;i++)
    {
        readSingleBlock(blkSize, address, mems);
        address += blkSize;
        mems += blkSize;
        incr++;
    }

    return 0;
}

struct tm readDateTime(u32 te)
{
    time_t t = time((time_t *)&te);
    struct tm tim = *localtime(&t);
    //printf("now: %d-%02d-%02d %02d:%02d:%02d\n", tim.tm_year + 1900, tim.tm_mon + 1, tim.tm_mday, tim.tm_hour, tim.tm_min, tim.tm_sec);
    return tim;
}

time_t readDateTime2(void)
{
    time_t t; // = time((time_t *)&te);
    time(&t);
    struct tm tim = *localtime(&t);
    //printf("now: %d-%02d-%02d %02d:%02d:%02d\n", tim.tm_year + 1900, tim.tm_mon + 1, tim.tm_mday, tim.tm_hour, tim.tm_min, tim.tm_sec);
    return t;
}

void readSetsA(void)
{
    u32 tme;
    struct tm tmm;

    if(is_modbus_opened != TRUE)
    {
        printf("ERROR modbus did not opened yet");
        return;
    }

    if(readSingleBlock(13, 1029, setsA) == 0)
    {
        
        tme = ((u32)setsA[9]) << 16;
        tme |= setsA[10];
        //printf("Fault Here ? ..................\n");
        tmm = readDateTime(tme);
        //printf("now: %d-%02d-%02d %02d:%02d:%02d\n", tmm.tm_year + 1900, tmm.tm_mon + 1, tmm.tm_mday, tmm.tm_hour, tmm.tm_min, tmm.tm_sec);
    }
}

time_t readAcqDateTime(struct tm *tvlu)
{
    time_t tme;
    u32 utme;
    struct tm value;

    if(is_modbus_opened != TRUE)
    {
        printf("ERROR modbus did not opened yet");
        return -1;
    }

    readSetsA();

    utme = (u32)((setsA[9] << 16) | setsA[10]);
    tme = (time_t)utme;
    *tvlu = *localtime(&tme);
    return tme;

}

void writeDateTime(int min, int hour, int day, int mon, int year, int sync_systime)
{
    time_t tme;
    u32 utme;
    struct tm value;

    if(is_modbus_opened != TRUE)
    {
        printf("ERROR modbus did not opened yet");
        return;
    }

    tme = time(NULL);
    printf("Present LU Time : \n");
    readDateTime(tme);

    readSetsA();
    value.tm_sec=0;
    value.tm_min=min;
    value.tm_hour=hour;
    value.tm_mday=day;
    value.tm_mon=mon;
    value.tm_year=year;

    if(sync_systime > 0)
    {
        tme = time(NULL);
    }
    else
    {
        tme = mktime(&value);
    }
    
    utme = (u32)tme;
    printf("%08X\n\n", utme);
    setsA[10] = (u16)(utme);
    setsA[9] = (u16)(utme >> 16);
    printf("%04X %04X\n\n", setsA[9], setsA[10]);
    if(modbus_write_registers(busA, 1029, 13, setsA) == -1)
    {
        printf("Failed To Write SetsA\n");
        return -1;
    }
}

void writeSetsA(u16 buf)
{
    u32 tme;
    struct tm tmm;
}

int readTrip1(void)
{
    return readMultiBlock(64, 36, 1300, t1_buf);
}

int readTrip2(void)
{
    return readMultiBlock(64, 36, 3604, t2_buf);
}

int readCloseCoil(void)
{
    return readMultiBlock(64, 36, 5908, close_buf);
}

int readVoltage(void)
{
    return readMultiBlock(64, 36, 8212, tbl);
}

int readPhaseACurr(void)
{
    return readMultiBlock(64, 36, 10516, currA_buf);
}

int readPhaseBCurr(void)
{
    return readMultiBlock(64, 36, 12820, currB_buf);
}

int readPhaseCCurr(void)
{
    return readMultiBlock(64, 36, 15124, currC_buf);
}

int readInitiateAndContact(void)
{
    return readMultiBlock(64, 18, 17428, init_cont_buf);
}

int saveFile(u16 *busdata, int fsize, char *fname)
{
    FILE *file;
    file = fopen(fname, "wb");
    fwrite(busdata, sizeof(unsigned char), fsize, file);
    fclose(file);
    return 0;
}

int checkRecordFlags(void)
{
    int res = 0;
    res = readSingleBlock(1, 1028, flag_Records);
    if(res == -1)
    {
        return -1;
    }
    else
    {
        return flag_Records[0];
    }
}

int readAll(void)
{
    //if(openAcqBus() == 0)
    {
        readTrip1();
        ////saveFile(t1_buf, (64*36*2), "/COMTRADE/trip1.raw");
        readTrip2();
        ////saveFile(t2_buf, (64*36*2), "/COMTRADE/trip2.raw");
        readCloseCoil();
        ////saveFile(tbl, (64*36*2), "/COMTRADE/close.raw");
        readVoltage();
        readPhaseACurr();
        readPhaseBCurr();
        readPhaseCCurr();
        readInitiateAndContact();

        //modbus_close(busA);
    }
    return 0;
}

void EraseList(void)
{
    uint16_t erase = 0xFF00;
    if(is_modbus_opened != TRUE)
    {
        printf("ERROR modbus did not opened yet");
        return;
    }

    if(modbus_write_registers(busA, 1043, 1, &erase) == -1)
    {
        printf("Failed To Write SetsA\n");
    }
}

CBEVENT cbevnt;
volatile CB_FILE cb_file;
time_t current_time;



int prepareFileData(void)
{
    time_t now;
    uint32_t now32 = 0;
    now = time(NULL);
	//time_info = localtime(&now);
    localtime(&now);
    now32 = (int32_t)now;

    int idxA = 0;
    readMultiBlock(34, 1, 1200, (u16 *)&cbevnt);
    cb_file.event_type = cbevnt.event_type;

    ////cb_file.event_datetime = now32; // __builtin_bswap32(now32);
    cb_file.event_datetime = cbevnt.event_datetime;
    printf("---Acq.c---prepareFileData()---cbevent.event_datetime : %d\n", cbevnt.event_datetime);
    ////cb_file.event_datetime = __builtin_bswap32(cbevnt.event_datetime);
    
    cb_file.event_millisec = cbevnt.event_millisec; // __builtin_bswap32(cbevnt.event_millisec);

    cb_file.alert_level = 0;

    cb_file.coil_max_current_t1 = cbevnt.tc1_peak_curr; // __builtin_bswap32(cbevnt.tc1_peak_curr);
    cb_file.coil_max_current_t2 = cbevnt.tc2_peak_curr; // __builtin_bswap32(cbevnt.tc2_peak_curr);
    cb_file.coil_max_current_close = cbevnt.cc_peak_curr; // __builtin_bswap32(cbevnt.cc_peak_curr);

    cb_file.coil_integral_t1 = cbevnt.tc1_curr_flow;
    cb_file.coil_integral_t2 = cbevnt.tc2_curr_flow;
    cb_file.coil_integral_close = cbevnt.cc_curr_flow;

    cb_file.coil_female_time_t1 = (uint32_t)cbevnt.tc1_curr_flow;
    cb_file.coil_female_time_t2 = (uint32_t)cbevnt.tc2_curr_flow;
    cb_file.coil_female_time_close = (uint32_t)cbevnt.cc_curr_flow;

    cb_file.contact_optime_A = cbevnt.contact_optime_A;
    cb_file.contact_optime_B = cbevnt.contact_optime_B;

    cb_file.block_close_time_A = cbevnt.block_close_time_A;
    cb_file.block_close_time_B = cbevnt.block_close_time_B;
    cb_file.block_close_time_C = cbevnt.block_close_time_C;

    cb_file.op_cnt = cbevnt.event_number;
    cb_file.smp_per_cyc = 128;
    cb_file.cyc_count = 18;
    cb_file.contact_duty_A = cbevnt.contact_duty_A;
    cb_file.contact_duty_B = cbevnt.contact_duty_B;
    cb_file.contact_duty_C = cbevnt.contact_duty_C;
    cb_file.contact_optime_A = cbevnt.contact_optime_A;
    cb_file.contact_optime_B = cbevnt.contact_optime_B;
    cb_file.block_close_time_A = cbevnt.block_close_time_A;
    cb_file.block_close_time_B = cbevnt.block_close_time_B;
    cb_file.block_close_time_C = cbevnt.block_close_time_C;

    readAll();

    for(idxA=0;idxA<2304;idxA++)
    {
        
        if(t1_buf[idxA] > 0xEFFF)
        {
            //__builtin_bswap16(t1_buf[idxA]);
            cb_file.cb_data.trip1_coil_current[idxA] = __builtin_bswap16((int16_t)((t1_buf[idxA] & 0x7FFF) - 0x7FFF));
        }
        else
        {
            cb_file.cb_data.trip1_coil_current[idxA] = __builtin_bswap16(t1_buf[idxA]);
        }

        if(t2_buf[idxA] > 0x7FFF)
        {
            cb_file.cb_data.trip2_coil_current[idxA] = __builtin_bswap16((int16_t)((t2_buf[idxA] & 0x7FFF) - 0x7FFF));
        }
        else
        {
            cb_file.cb_data.trip2_coil_current[idxA] = __builtin_bswap16(t2_buf[idxA]);
        }

        if(currA_buf[idxA] > 0x7FFF)
        {
            cb_file.cb_data.phase_current_A[idxA] = __builtin_bswap16((int16_t)((currA_buf[idxA] & 0x7FFF) - 0x7FFF));
        }
        else
        {
            cb_file.cb_data.phase_current_A[idxA] = __builtin_bswap16(currA_buf[idxA]);
        }

        if(currB_buf[idxA] > 0x7FFF)
        {
            cb_file.cb_data.phase_current_B[idxA] = __builtin_bswap16((int16_t)((currB_buf[idxA] & 0x7FFF) - 0x7FFF));
        }
        else
        {
            cb_file.cb_data.phase_current_B[idxA] = __builtin_bswap16(currB_buf[idxA]);
        }

        if(currC_buf[idxA] > 0x7FFF)
        {
            cb_file.cb_data.phase_current_C[idxA] = __builtin_bswap16((int16_t)((currC_buf[idxA] & 0x7FFF) - 0x7FFF));
        }
        else
        {
            cb_file.cb_data.phase_current_C[idxA] = __builtin_bswap16(currC_buf[idxA]);
        }

        if(close_buf[idxA] > 0x7FFF)
        {
            cb_file.cb_data.close_coil_current[idxA] = __builtin_bswap16((int16_t)((close_buf[idxA] & 0x7FFF) - 0x7FFF));
        }
        else
        {
            cb_file.cb_data.close_coil_current[idxA] = __builtin_bswap16(close_buf[idxA]);
        }

        cb_file.cb_data.initiate_and_contact[idxA] = init_buf[idxA];
    }

    return 0;
}

void save_file(struct tm *tmm)
{
    //struct tm *time_info;
    char time_string[45];
    char filename[60];
    size_t size_file =0;
    //time(&current_time);
    //time_info = localtime(&current_time);

    uint8_t fbuf[_FILE_SIZE];
    uint8_t *ubuf = (uint8_t *)&cb_file;

    // Following code to surpass Compiler bug. Compiler has an issue of memory alignment with the processor
    // which effects memcpy() function. Following numbers cannot be replaced by specific names.
    fbuf[0] = ubuf[0];
    memcpy(&fbuf[1], &ubuf[4], 8);
    fbuf[9] = ubuf[15];
    memcpy(&fbuf[10], &ubuf[16], _FILE_SIZE);

    printf("Sizeof->%d\n", sizeof(cb_file));
    // printf below to show each byte value for comparison only. It helped detects the compiler bug
    printf("%X %X %X %X %X %X %X %X %X %X %X %X\n", fbuf[0], fbuf[1], fbuf[2], fbuf[3], fbuf[4], fbuf[5], fbuf[6], fbuf[7], fbuf[8], fbuf[9], fbuf[10], fbuf[11]);

    strftime(time_string, sizeof(time_string), "%Y%m%d%H%M%S", tmm);
    sprintf(filename, "/COMTRADE/01_22_%s%03d.dat", time_string, cb_file.event_millisec);
    printf("--time string : %s\n", time_string);
    printf("--tmm->hour : %d\n", tmm->tm_hour);

    //*tmm = atoi(time_string);

    FILE *file;
    file = fopen(filename, "w+b");
    //size_file = fwrite(&cb_file, 1 /*sizeof(unsigned char)*/, 30050, file);
    size_file = fwrite(&fbuf, 1 /*sizeof(unsigned char)*/, 30050, file);
    //printf("fwrite->%d\n", size_file);
    fclose(file);
}

void testAcqConnection(void)
{
    modbus_t *ctx;
    int rc;
    int addr = 1028;
    int nb = 1;
    unsigned short tab_rp_registers[100];
    ctx = modbus_new_tcp("192.168.247.100", 100);
    modbus_set_debug(ctx, TRUE);

    if (modbus_connect(ctx) == -1) {
        //fprintf(stderr, "Modbus Connection failed: %s\n", modbus_strerror(errno));
        modbus_free(ctx);
        //return -1;
    }
    else
    {
        printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\nModbus Connected\n");
        rc = modbus_read_registers(ctx, addr, 1, tab_rp_registers);
        if (rc != 1) 
        {
            printf("ERROR modbus_read_registers single (%d)\n", rc);
            printf("Address = %d\n", addr);
        }
        else
        {
            printf("\n\n\n\n\n\n\n\n\n\n\n\n DATA %04X", tab_rp_registers[0]);
        }
        modbus_close(ctx);
        modbus_free(ctx);
    }

    return;
}

