
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

#include "Acq.h"

#define _FILE_SIZE 30050

#define MIN_AMPL_THRESHOLD      100.0

uint16_t setsA[SETTINGS_ARRAY_SIZE];
uint16_t flag_Records[2];
u16 tbl[ACQ_RAWDATA_ALLOCATION_SIZE];
u16 t1_buf[ACQ_RAWDATA_ALLOCATION_SIZE];
u16 t2_buf[ACQ_RAWDATA_ALLOCATION_SIZE];
u16 close_buf[ACQ_RAWDATA_ALLOCATION_SIZE];
u16 currA_buf[ACQ_RAWDATA_ALLOCATION_SIZE];
u16 currB_buf[ACQ_RAWDATA_ALLOCATION_SIZE];
u16 currC_buf[ACQ_RAWDATA_ALLOCATION_SIZE];
u16 init_cont_buf[ACQ_RAWDATA_ALLOCATION_SIZE];
u8 *init_buf = (u8 *)init_cont_buf;
modbus_t *busA;
bool_type is_modbus_opened;

float gidxA = 0;
float t1_sum = 0;
float t2_sum = 0;
float cc_sum = 0;
uint32_t t1_time_span = 0;
uint32_t t2_time_span = 0;
uint32_t cc_time_span = 0;

extern ds61850IfcInfo* g_p61850IfcInfo;

void Construct(void *acqobj)
{

}

ACQ_STRUCT *AddNewAcq(char *ip)
{
    ACQ_STRUCT *acqobj;
    acqobj->_Construct = Construct;
    return acqobj;
}

#ifdef CPP_BLOCK_ENABLED

//extern dsLUConfig pdsluConfig;

static dsLUConfig pdsluConfig;

/**
 * Defining The Class _ACQ.
 */
_ACQ::_ACQ(void)
{
    acq_index = 0;
}

/**
 * Test function to proof of OOP.
 */
void _ACQ::printSample(int lidxA)
{
    printf("*************-------------------Printing from ACQ Class \r\n");
}

/**
 * Open ACQ Modbus TCP over IP address and Port.
 * @param ip_addr Device IP Address. 
 * @param Port Port to connect with for Modbus comms.
 */
int _ACQ::openAcqBus(const char *ip_addr, int Port)
{
    this->busA = modbus_new_tcp(ip_addr, Port);
    this->is_modbus_opened = FALSE;
    if (modbus_connect(this->busA) == -1) 
    {
        //fprintf(stderr, "Modbus Connection failed: %s\n", modbus_strerror(errno));
        modbus_free(this->busA);
        return -1;
    }
    this->is_modbus_opened = TRUE;
    return 0;
}

/**
 * Close Modbus Connection.
 */
void _ACQ::closeModbus(void)
{
    if(this->is_modbus_opened == TRUE)
    {
        modbus_close(this->busA);
        modbus_free(this->busA);
        this->is_modbus_opened = FALSE;
    }
}

/**
 * Write Single Block only at a single address.
 * @param address Register Address to write.
 * @param mems Port to connect with for Modbus comms.
 */
int _ACQ::writeSingleBlock(int sz, u16 address, uint16_t *mems)
{
    if(this->is_modbus_opened != TRUE)
    {
        printf("ERROR modbus did not opened yet");
        return -1;
    }
    
    if(modbus_write_registers(this->busA, address, sz, mems) == -1)
    {
        return -1;
    }
    
    return 0;
}

/**
 * Read Multiple Blocks or Registers.
 * @param  Register Address to write.
 * @param mems Port to connect with for Modbus comms.
 */
int _ACQ::readMultiBlock(int blkSize, int numberOfBlocks, u16 address, uint16_t *mems)
{
    int i;
    int incr = 0;

    if(this->is_modbus_opened != TRUE)
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

/**
 * Read 4608 bytes of trip1 data to t1_buf. Offset Address 1300.
 */
int _ACQ::readTrip1(void)
{
    return readMultiBlock(BLOCKSIZE, NUMBEROFBLOCKS_A, TRIP1_OFFSET, this->t1_buf);
}

/**
 * Read 4608 bytes of trip1 data to t2_buf. Offset Address 1300.
 */
int _ACQ::readTrip2(void)
{
    return readMultiBlock(BLOCKSIZE, NUMBEROFBLOCKS_A, TRIP2_OFFSET, this->t2_buf);
}

/**
 * Read 4608 bytes of trip1 data to close_buf. Offset Address 5908.
 */
int _ACQ::readCloseCoil(void)
{
    return readMultiBlock(BLOCKSIZE, NUMBEROFBLOCKS_A, CLOSECOIL_OFFSET, this->close_buf);
}

/**
 * Read 4608 bytes of trip1 data to tbl_buf. Offset Address 8212.
 */
int _ACQ::readVoltage(void)
{
    return readMultiBlock(BLOCKSIZE, NUMBEROFBLOCKS_A, VOLTAGE_OFFSET, this->tbl);
}

/**
 * Read 4608 bytes of trip1 data to currA_buf. Offset Address 10516.
 */
int _ACQ::readPhaseACurr(void)
{
    return readMultiBlock(BLOCKSIZE, NUMBEROFBLOCKS_A, PHASEA_CURR_OFFSET, this->currA_buf);
}

/**
 * Read 4608 bytes of trip1 data to currB_buf. Offset Address 12820.
 */
int _ACQ::readPhaseBCurr(void)
{
    return readMultiBlock(BLOCKSIZE, NUMBEROFBLOCKS_A, PHASEB_CURR_OFFSET, this->currB_buf);
}

/**
 * Read 4608 bytes of trip1 data to currC_buf. Offset Address 15124.
 */
int _ACQ::readPhaseCCurr(void)
{
    return readMultiBlock(BLOCKSIZE, NUMBEROFBLOCKS_A, PHASEC_CURR_OFFSET, this->currC_buf);
}

/**
 * Read 2304 bytes of trip1 data to init_cont_buf. Offset Address 17428.
 */
int _ACQ::readInitiateAndContact(void)
{
    return readMultiBlock(BLOCKSIZE, NUMBEROFBLOCKS_B, INITANDCONTACT_OFFSET, this->init_cont_buf);
}

int _ACQ::readAll(void)
{
    return 0;
}

/**
 * Test function that uses some data structures from lib61850 to parse json and related data structure.
 * @param _filename Name of the JSON file to parse.
 */
void _ACQ::parseIPAddressesFromJson(const char *_filename)
{
    int lidxA = 0;
    const ST_CHAR *ps8configfile = "LUConfig.json";

	if(!parseLUConfig(ps8configfile, &pdsluConfig))
	{
        printf("Error parsing 'LUConfig.json'\n");
	}
	else
	{
		printf("-----\n");
		printf("----------------\n");
		printf("[Acq.c] - SUCCESS parsing 'LUConfig.json'\n");
		printf("[Acq.c] - LU Count :  %d\n\n", pdsluConfig.s8numLUs);
        printf("----------------\n");
        printf("-----\n");

        for (ST_INT i = 0; i < pdsluConfig.s8numLUs; i++)
        {
            //g_p61850IfcInfo->ps8IPAddr
            if (strcmp(pdsluConfig.LUList[i].ps8luIP, g_p61850IfcInfo->ps8IPAddr) == 0)
            {
                printf("[Acq.c] - #################################################\n  Found IP : %s\n", pdsluConfig.LUList[i].ps8luIP);
                for (int j = 0; j < pdsluConfig.LUList[i].s8numLNs; j++)
                {
                    // printf("  LN IP Address: %s Name: %s ID : %d\n", pdsluConfig.LUList[i].LNList[j].ps8ip_in, pdsluConfig.LUList[i].LNList[j].ps8name, pdsluConfig.LUList[i].LNList[j].s16id_ln);
                    if(strstr(pdsluConfig.LUList[i].LNList[j].ps8name, "SCBR"))
                    {
                        //printf("  LN IP Address: %s Name: %s ID : %d\n", pdsluConfig.LUList[i].LNList[j].ps8ip_in, pdsluConfig.LUList[i].LNList[j].ps8name, pdsluConfig.LUList[i].LNList[j].s16id_ln);
                        strcpy(this->ip_address, pdsluConfig.LUList[i].LNList[j].ps8ip_in);
                        printf("[Acq.c] -  LN IP Address: %s Name: %s ID : %d\n", this->ip_address, pdsluConfig.LUList[i].LNList[j].ps8name, pdsluConfig.LUList[i].LNList[j].s16id_ln);

                        break;
                    }
                }
                break;
            }
        }
	}
}

#endif

int openAcqBus(char *ap_addr)
{
    busA = modbus_new_tcp(ap_addr, ACQ_PORT);
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

    if(readSingleBlock(SETTINGS_BLOCK_SIZE, SETTINGS_BLOCK_OFFSET, setsA) == 0)
    {
        
        tme = ((u32)setsA[9]) << 16; // Changing Endianess, because data came from another processor architechture.
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
    if(modbus_write_registers(busA, SETTINGS_BLOCK_OFFSET, SETTINGS_BLOCK_SIZE, setsA) == -1)
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
    return readMultiBlock(BLOCKSIZE, NUMBEROFBLOCKS_A, TRIP1_OFFSET, t1_buf);
}

int readTrip2(void)
{
    return readMultiBlock(BLOCKSIZE, NUMBEROFBLOCKS_A, TRIP2_OFFSET, t2_buf);
}

int readCloseCoil(void)
{
    return readMultiBlock(BLOCKSIZE, NUMBEROFBLOCKS_A, CLOSECOIL_OFFSET, close_buf);
}

int readVoltage(void)
{
    return readMultiBlock(BLOCKSIZE, NUMBEROFBLOCKS_A, VOLTAGE_OFFSET, tbl);
}

int readPhaseACurr(void)
{
    return readMultiBlock(BLOCKSIZE, NUMBEROFBLOCKS_A, PHASEA_CURR_OFFSET, currA_buf);
}

int readPhaseBCurr(void)
{
    return readMultiBlock(BLOCKSIZE, NUMBEROFBLOCKS_A, PHASEB_CURR_OFFSET, currB_buf);
}

int readPhaseCCurr(void)
{
    return readMultiBlock(BLOCKSIZE, NUMBEROFBLOCKS_A, PHASEC_CURR_OFFSET, currC_buf);
}

int readInitiateAndContact(void)
{
    return readMultiBlock(BLOCKSIZE, NUMBEROFBLOCKS_B, INITANDCONTACT_OFFSET, init_cont_buf);
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

CBEVENT cbevnt;
volatile CB_FILE cb_file;
time_t current_time;


// Wave data coming from ACQ in 16bit format where the Value is +32767 to -32767 . But since our variable is 32 bit
// wide, it cannot detect Negative values. So by checking the flag is_twoscompl, it also check if the value is greater
// than 32767, it will convert it to negative value. Similar code is applied to the LU code and Python acq code.

int prepareFileData(void)
{
    time_t now;
    uint32_t now32 = 0;
    now = time(NULL);
	//time_info = localtime(&now);
    localtime(&now);
    now32 = (int32_t)now;

    int idxA = 0;
    readMultiBlock(CBEVENT_BLOCK_SIZE, CBEVENT_NUMBEROFBLOCKS, CBEVENT_BLOCK_OFFSET, (u16 *)&cbevnt);
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

    cb_file.coil_female_time_t1 = cbevnt.tc1_curr_flow;
    cb_file.coil_female_time_t2 = cbevnt.tc2_curr_flow;
    cb_file.coil_female_time_close = cbevnt.cc_curr_flow;

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

    t1_sum = 0;
    t2_sum = 0;
    cc_sum = 0;
    t1_time_span = 0;
    t2_time_span = 0;
    cc_time_span = 0;

    readAll();

    for(idxA=0;idxA<CB_ELEMENT_ARRAY_SIZE;idxA++)
    {
        
        if(t1_buf[idxA] > 0xEFFF)
        {
            //__builtin_bswap16(t1_buf[idxA]);
            cb_file.cb_data.trip1_coil_current[idxA] = __builtin_bswap16((int16_t)((t1_buf[idxA] & 0x7FFF) - 0x7FFF));
            gidxA = (float)(int32_t)((t1_buf[idxA] & 0x7FFF) - 0x7FFF);
        }
        else
        {
            cb_file.cb_data.trip1_coil_current[idxA] = __builtin_bswap16(t1_buf[idxA]);
            gidxA = (float)(int32_t)(t1_buf[idxA]);
        }

        if(gidxA > MIN_AMPL_THRESHOLD)
        {
            t1_sum += gidxA * DT;
            t1_time_span++;
        }

        gidxA = 0;

        if(t2_buf[idxA] > 0x7FFF)
        {
            cb_file.cb_data.trip2_coil_current[idxA] = __builtin_bswap16((int16_t)((t2_buf[idxA] & 0x7FFF) - 0x7FFF));
            gidxA = (float)(int32_t)((t2_buf[idxA] & 0x7FFF) - 0x7FFF);
        }
        else
        {
            cb_file.cb_data.trip2_coil_current[idxA] = __builtin_bswap16(t2_buf[idxA]);
            gidxA = (float)(int32_t)(t2_buf[idxA]);
        }

        if(gidxA > MIN_AMPL_THRESHOLD)
        {
            t2_sum += gidxA * DT;
            t2_time_span++;
        }

        gidxA = 0.0;

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
            gidxA = (float)(int32_t)((close_buf[idxA] & 0x7FFF) - 0x7FFF);
        }
        else
        {
            cb_file.cb_data.close_coil_current[idxA] = __builtin_bswap16(close_buf[idxA]);
            gidxA = (float)(int32_t)(close_buf[idxA]);
        }

        if(gidxA > MIN_AMPL_THRESHOLD) 
        {
            cc_sum += gidxA * DT;
            cc_time_span++;
        }

        gidxA = 0.0;

        cb_file.cb_data.initiate_and_contact[idxA] = init_buf[idxA];
    }

    cb_file.coil_integral_t1 = (float)(t1_sum);
    cb_file.coil_integral_t2 = (float)t2_sum;
    cb_file.coil_integral_close = (float)cc_sum;

    return 0;
}

void save_file(struct tm *tmm)
{
    //struct tm *time_info;
    char time_string[45];
    char filename[255];
    size_t size_file =0;
    //time(&current_time);
    //time_info = localtime(&current_time);

    uint8_t fbuf[_FILE_SIZE];
    uint8_t *ubuf = (uint8_t *)&cb_file;

    // #Compiler Error
    // Following code to surpass Compiler bug. Compiler has an issue of memory alignment with the processor
    // which effects memcpy() function. Following numbers cannot be replaced by specific names.
    fbuf[0] = ubuf[0];
    memcpy(&fbuf[1], &ubuf[4], 8);
    fbuf[9] = ubuf[15];
    memcpy(&fbuf[10], &ubuf[16], _FILE_SIZE);

    //printf("Sizeof->%d\n", sizeof(cb_file));
    // printf below to show each byte value for comparison only. It helped detects the compiler bug
    //printf("%X %X %X %X %X %X %X %X %X %X %X %X\n", fbuf[0], fbuf[1], fbuf[2], fbuf[3], fbuf[4], fbuf[5], fbuf[6], fbuf[7], fbuf[8], fbuf[9], fbuf[10], fbuf[11]);

    strftime(time_string, sizeof(time_string), "%Y%m%d%H%M%S", tmm);
    sprintf(filename, "/data/COMTRADE/EVENT/CBCM/CH01_22_%s%03d.dat", time_string, cb_file.event_millisec);
    //printf("--time string : %s\n", time_string);
    //printf("--tmm->hour : %d\n", tmm->tm_hour);

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
    ctx = modbus_new_tcp("192.168.10.100", 100);
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

