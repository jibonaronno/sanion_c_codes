
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

uint16_t setsA[30];
uint16_t flag_Records[2];
u16 tbl[3664];
modbus_t *busA;
bool_type is_modbus_opened;

int openAcqBus(void)
{
    busA = modbus_new_tcp("192.168.10.100", 100);
    is_modbus_opened = FALSE;
    if (modbus_connect(busA) == -1) 
    {
        fprintf(stderr, "Modbus Connection failed: %s\n", modbus_strerror(errno));
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
    printf("R.");
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

    for(i=0;i<numberOfBlocks;i += blkSize)
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
    printf("now: %d-%02d-%02d %02d:%02d:%02d\n", tim.tm_year + 1900, tim.tm_mon + 1, tim.tm_mday, tim.tm_hour, tim.tm_min, tim.tm_sec);
    return tim;
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
    return readMultiBlock(64, 36, 1300, tbl);
}

int readTrip2(void)
{
    return readMultiBlock(64, 36, 3604, tbl);
}

int readCloseCoil(void)
{
    return readMultiBlock(64, 36, 5908, tbl);
}

int readVoltage(void)
{
    return readMultiBlock(64, 36, 8212, tbl);
}

int readPhaseACurr(void)
{
    return readMultiBlock(64, 36, 10516, tbl);
}

int readPhaseBCurr(void)
{
    return readMultiBlock(64, 36, 12820, tbl);
}

int readPhaseCCurr(void)
{
    return readMultiBlock(64, 36, 15124, tbl);
}

int readInitiateAndContact(void)
{
    return readMultiBlock(64, 18, 17428, tbl);
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
        saveFile(tbl, (64*36*2), "/COMTRADE/trip1.raw");
        readTrip2();
        saveFile(tbl, (64*36*2), "/COMTRADE/trip2.raw");
        readCloseCoil();
        saveFile(tbl, (64*36*2), "/COMTRADE/close.raw");
        readPhaseACurr();
        readPhaseBCurr();
        readPhaseCCurr();
        readInitiateAndContact();

        //modbus_close(busA);
    }
    return 0;
}

void testAcqConnection(void)
{
    modbus_t *ctx;
    int rc;
	int addr = 1028;
	int nb = 1;
	unsigned short tab_rp_registers[100];
	ctx = modbus_new_tcp("192.168.10.100", 100);
    modbus_set_debug(ctx, TRUE);

	if (modbus_connect(ctx) == -1) {
        fprintf(stderr, "Modbus Connection failed: %s\n", modbus_strerror(errno));
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

