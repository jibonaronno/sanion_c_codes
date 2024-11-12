/******************************************************************************
 **                     Copyright(C) 2016 SANION Co. Ltd.                    **
 **                  --------------------------------------                  **
 **                              WWW.SANION.COM                              **
 ******************************************************************************
    PROJECT     : UR-IED
 ******************************************************************************
    FILE NAME   : DBManager.h
    AUTHOR      : sootoo23
    DATE        : 2016.11.21
    REVISION    : V1.00
    DESCRIPTION : DB Managemant (use SQLITE3)
 ******************************************************************************
    HISTORY     :
        2016-11-21 Create
        2017-11-23 Logic DB Add
 ******************************************************************************/

#ifndef DBMANAGER_H_
#define DBMANAGER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include <sys/time.h>
#include "sqlite3.h"

/******************************************************************************************************
 DB Manager Func
******************************************************************************************************/
#define SystemDB_Name "/.db/systemDB.sqlite"
#define DULSystemDB_Name "/.dul/DULsystemDB.sqlite"
#define DEFSystemDB_Name "/data/DefaultsystemDB.sqlite"

//semaphore signal
extern sqlite3 *SystemDB; 
extern sqlite3 *DULSystemDB;
extern sqlite3 *DEFSystemDB;
extern int system_wait_sig; 

int DBM_GetSetting_DBName(sqlite3* DBName,char *table, char *name, char *GetData);
int DBM_Init(void);
void DBM_DeInit(void);
int DBM_DBOpen(char *dbfile_name, sqlite3 **DB);
int DBM_IsNewDB(void);
int DBM_SysDBInit(void);
void DBM_SystemDB_Attach_Execute(void);
int DBM_SaveSetting(char *table, char *setdata, char *name);
int DBM_GetSetting(char *table, char *name, char *GetData);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* DBM_H_ */

