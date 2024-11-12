/******************************************************************************************************
 **                     Copyright(C) 2016 SANION Co. Ltd.                    **
 **                  --------------------------------------                  **
 **                              WWW.SANION.COM                              **
 ******************************************************************************************************
    PROJECT     : UR-IED
 ******************************************************************************************************
    FILE NAME   : DBManager.c
    AUTHOR      : sootoo23
    DATE        : 2016.11.21
    REVISION    : V1.00
    DESCRIPTION : DB Managemant (use SQLITE3)
 ******************************************************************************************************
    HISTORY     :
        2016-11-21 sootoo23 - Create
        2017-01-24 sootoo23 - DB Table Design
        2017-11-23 sootoo23 - Logic DB Add
 ******************************************************************************************************/
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "global.h"
#include <math.h> 


#include "VT100.h"
#include "Common.h"
#include "ConsoleManager.h"
#include "DBManager.h"

/*
*  sootoo23 - sqlite3는 sqlite3_open()를 통해 할당되고,
*  sqlite3_stmt는 sqlite3_prepare()를 통해 생성된다.
*  반대로 sqlite3는 sqlite3_close()로 해제하고, 
*  sqlite3_stmt 는 sqlite3_finalize()로 해제한다. 
*/
sqlite3 *SystemDB, *DULSystemDB, *DEFSystemDB;
sqlite3_stmt *system_stmt;
int system_wait_sig; 

int DBM_Init(void)
{
	char u8CopyCmd[128]={0,};

	//sootoo23
	//sqlite journal mode: DELETE | TRUNCATE | PERSIST | MEMORY | WAL | OFF
	//journal mode access version: SQLITE Library Version:3.8.10.2
	// Not Working Version: 3.5.*
	printf("SQLITE Library Version:%s\n", sqlite3_libversion()); 

	if (DBM_DBOpen(SystemDB_Name, &SystemDB))
		return -1;

	//sootoo23-DB Table이 없을경우 새로 생성한다.
	if(DBM_IsNewDB()) 
	{
		if (DBM_SysDBInit()) //System DB 생성.
			return -1;

		//copy system DB
		sprintf((char *)u8CopyCmd, "cp %s %s;", SystemDB_Name, DULSystemDB_Name);
		system(u8CopyCmd);
		sync();
		
		sprintf((char *)u8CopyCmd, "cp %s %s;", SystemDB_Name, DEFSystemDB_Name);
		system(u8CopyCmd);
		sync();
	}

	//Dualization DB Open
	if (DBM_DBOpen(DULSystemDB_Name, &DULSystemDB))
		return -1;

	if (DBM_DBOpen(DEFSystemDB_Name, &DEFSystemDB))
		return -1;

	system_wait_sig = CreateSemaphore(1);

	DBM_SystemDB_Attach_Execute();
	
	return 0;
}

void DBM_DeInit(void)
{
	SemWait(system_wait_sig);

	if(CDM_GetViewMode(CDM_DISP_VIEW_DB))
		printf("[%s] Stmt Finalize Success. \n", __func__);


	if(DULSystemDB)
		sqlite3_close(DULSystemDB);
	if(SystemDB)
		sqlite3_close(SystemDB);

	if(CDM_GetViewMode(CDM_DISP_VIEW_DB))
		printf("[%s] DB Close Success. \n", __func__);
	
	SemPost(system_wait_sig);

	DeleteSemaphore(system_wait_sig);

	if(CDM_GetViewMode(CDM_DISP_VIEW_DB))
		printf("[%s] Semaphore Delte Success. \n", __func__);
}

int DBM_DBOpen(char *dbfile_name, sqlite3 **DB)
{
	printf("[%s] DB Open Name: %s\n", __FUNCTION__, dbfile_name);

	if(sqlite3_open(dbfile_name, DB)!= SQLITE_OK)
	{
		printf("[%s] Fail to sqlite3 open!\n", __FUNCTION__);
		return -1;
	}
	
	return 0;
}

int DBM_IsNewDB(void)
{
	//char query[] = "SELECT count(*) FROM sqlite_master WHERE type = 'table' AND name = 'AccountInfo'";
	char query[] = "SELECT count(tbl_name) FROM sqlite_master WHERE type = 'table' and not tbl_name = 'sqlite_sequence'";
	int table_cnt = 0;
	
	if(sqlite3_prepare(SystemDB, query, strlen(query), &system_stmt, NULL) != SQLITE_OK)
	{
		printf("[%s] %d Fail\n", __FUNCTION__, __LINE__);
		SemPost(system_wait_sig);
		return -1;
	}

	while(sqlite3_step(system_stmt) == SQLITE_ROW)  //result
    {
 		table_cnt = (int)sqlite3_column_int(system_stmt,0); //sootoo23: 0 -> first data value
	}

	sqlite3_finalize(system_stmt);

	return (table_cnt > 0 ? 0 : 1);
}

int DBM_SysDBInit(void)
{
	char *errMsg=0;
	int err_code =0;

	//sootoo23-160216:
	//TODO: 추후 DB 암호화를 고려해 보자.
	char query[] = 
		"  pragma journal_mode = MEMORY; "
		"CREATE TABLE IF NOT EXISTS NormalSetting("
		"  IndexNo		integer PRIMARY KEY AUTOINCREMENT,"
		"  Name 		varchar(50) default NULL,"
		"  data 		varchar(50) default NULL,"
		"  Min			integer not NULL default 0,"
		"  Max			integer not NULL default 0,"
		"  Step			integer not NULL default 0,"
		"  DataType		varchar(10) default 'string'"
		");"

		
		"INSERT INTO NormalSetting (Name, data) VALUES('Product Name'	,'Prototype LU');"
		"INSERT INTO NormalSetting (Name, data) VALUES('Model Name'		,'GLU');"
		"INSERT INTO NormalSetting (Name, data) VALUES('MPU Version'		,'000');"
		"INSERT INTO NormalSetting (Name, data) VALUES('MPU Update Version','000');"
		"INSERT INTO NormalSetting (Name, data) VALUES('HW Version'		,'000');"
		"INSERT INTO NormalSetting (Name, data) VALUES('MPU Version Year','2024');"
		"INSERT INTO NormalSetting (Name, data) VALUES('MPU Version Mon'	,'00');"
		"INSERT INTO NormalSetting (Name, data) VALUES('MPU Version Day'	,'00');"
		"INSERT INTO NormalSetting (Name, data) VALUES('Site Location'	,'Incheon');"
		"INSERT INTO NormalSetting (Name, data) VALUES('Site Name'		,'Sanion');"
		"INSERT INTO NormalSetting (Name, data) VALUES('MacAddr'			,'00:1B:4B:DD:EE:FF');" 

		"INSERT INTO NormalSetting (Name, data, Max, DataType) VALUES('SN Year'		,'2021', 9999, 'int');"
		"INSERT INTO NormalSetting (Name, data, Max, DataType) VALUES('SN ID'			,'0000', 9999, 'int');"
		
		"INSERT INTO NormalSetting (Name, data, DataType) VALUES('Time Sync Interval'	,'10800', 'int');" // 3hour > 10800 sec

		"CREATE TABLE IF NOT EXISTS DevInfo ("
		"  IndexNo		integer PRIMARY KEY AUTOINCREMENT,"
		"  Name 		varchar(50) default NULL,"
		"  data 		varchar(50) default NULL,"
		"  Min			integer not NULL default 0,"
		"  Max			integer not NULL default 0,"
		"  Step			integer not NULL default 0,"
		"  DataType		varchar(10) default 'string'"
		");"

		"INSERT INTO DevInfo (Name, data, Step, DataType) VALUES('MPU IPAddr'	,'192.168.247.107'	, 1, 'int');"
		"INSERT INTO DevInfo (Name, data, Step, DataType) VALUES('MPU Subnet'	,'255.255.255.0'	, 1, 'int');"
		"INSERT INTO DevInfo (Name, data, Step, DataType) VALUES('MPU Gateway'	,'192.168.247.1'	, 1, 'int');"
		//"INSERT INTO DevInfo (Name, data, Step, DataType) VALUES('NTP IPAddr'	,'203.248.240.140'	, 1, 'int');"
		"INSERT INTO DevInfo (Name, data, Step, DataType) VALUES('NTP IPAddr'	,'192.168.247.172'	, 1, 'int');"		
		"INSERT INTO DevInfo (Name, data, Step, DataType) VALUES('NTP Subnet'	,'255.255.255.0'	, 1, 'int');"
		//"INSERT INTO DevInfo (Name, data, Step, DataType) VALUES('NTP Gateway'	,'203.248.240.1'	, 1, 'int');"
		"INSERT INTO DevInfo (Name, data, Step, DataType) VALUES('NTP Gateway'	,'192.168.247.1'	, 1, 'int');"
		"INSERT INTO DevInfo (Name, data, Step, DataType) VALUES('PDDAU IPAddr'	,'192.168.247.142'	, 1, 'int');"
		"INSERT INTO DevInfo (Name, data, Step, DataType) VALUES('PDDAU Subnet'	,'255.255.255.0'	, 1, 'int');"
		"INSERT INTO DevInfo (Name, data, Step, DataType) VALUES('PDDAU Gateway','192.168.247.1'	, 1, 'int');"		

		"CREATE TABLE IF NOT EXISTS IEDSetting ("
		"  IndexNo		integer PRIMARY KEY AUTOINCREMENT,"
		"  Name 		varchar(50) default NULL,"
		"  data 		varchar(50) default NULL,"
		"  Min			integer not NULL default 0,"
		"  Max			integer not NULL default 0,"
		"  DataType		varchar(10) default 'string'"
		");"

		"INSERT INTO IEDSetting (Name, data) VALUES('User'		, 'user');"
		"INSERT INTO IEDSetting (Name, data) VALUES('Operator'	, 'SIEDX');"
		"INSERT INTO IEDSetting (Name, data) VALUES('SuperVisor', 'SIEDMT');"
		"INSERT INTO IEDSetting (Name, data) VALUES('Developer'	, 'SIEDRND');" 
		;

	err_code = sqlite3_exec(SystemDB, query, 0, 0, &errMsg);
	if(err_code!= SQLITE_OK)
	{
		printf("[%s] Initialize Fail: %s[code:%d]\n", __FUNCTION__, errMsg, err_code);
		sqlite3_free(errMsg);
		return -1;
	}

	return 0;
}

int DBM_GetSetting_DBName(sqlite3* DBName,char *table, char *name, char *GetData)
{
	char query[128] = {0,};
	int ret=0, n_cols=0, type=0, i=0;

	if(table == NULL)
		return -1;
	else if(name == NULL)
	{
		name = "*";
	}

	sprintf(query, "SELECT data FROM %s WHERE Name = '%s'", table, name);

	SemWait(system_wait_sig);
	if(sqlite3_prepare(DBName, query, strlen(query), &system_stmt, NULL) != SQLITE_OK)
	{
		printf("[%s] %s Fail\n", __FILE__, __FUNCTION__);
		SemPost(system_wait_sig);
		return -1;
	}

	while (1)
	{
		ret = sqlite3_step (system_stmt);
		if (ret == SQLITE_DONE)
	      		break;		//- end of result set
	      		
		n_cols = sqlite3_column_count (system_stmt);
		//printf("[%s]colums count!: %d\n", __FUNCTION__, n_cols);
		for (i = 0; i < n_cols; i++)
		{
			//- update the DBF export fields analyzing fetched data *-
			type = sqlite3_column_type (system_stmt, i);
			if (type == SQLITE_TEXT)
			{
				strcpy(GetData, (char *)sqlite3_column_text (system_stmt, i));
			}
			else if(type == SQLITE_INTEGER )
			{
				int nValue = sqlite3_column_int(system_stmt, i );
				sprintf(GetData, "%d", nValue);
			}
			else if(type == SQLITE_FLOAT )
			{
				double dlValue = sqlite3_column_double(system_stmt, i);
				sprintf(GetData, "%f", dlValue);
			}
			
		}
	}

	sqlite3_finalize(system_stmt);
	SemPost(system_wait_sig);
	
	return 0;
}


void DBM_SystemDB_Attach_Execute(void)
{
	U8 query[500]={0,};
	char *errMsg=0;
	int err_code=0;

	sprintf((char *)query,	"ATTACH DATABASE '/.dul/DULsystemDB.sqlite' As 'BackupSystem';"
					"ATTACH DATABASE '/data/DefaultsystemDB.sqlite' As 'DefaultSystem';");

	if(CDM_GetViewMode(CDM_DISP_VIEW_DB))
		printf("[%s] %s\n", __func__, (char *)query);

	SemWait(system_wait_sig);
	err_code = sqlite3_exec(SystemDB, query, 0, 0, &errMsg);
	if(err_code!= SQLITE_OK)
	{
		printf("[%s] sqlite3_exec Fail: %s [code:%d]\n", __FUNCTION__, errMsg, err_code);
		sqlite3_free(errMsg);
		SemPost(system_wait_sig);
		return;
	}

	SemPost(system_wait_sig);
}

int DBM_SaveSetting(char *table, char *setdata, char *name)
{
	char query[128]={0,};
	//char event[128]={0,};
	char *errMsg=0;
	int err_code =0;

	if(table == NULL)
		return -1;
	else if(name == NULL)
	{
		name = "*";
	}

	sprintf(query, "UPDATE %s SET data = '%s' WHERE Name = '%s';", table, setdata, name);
	//sprintf(event, "UPDATE %s SET data = %s WHERE Name = %s", table, setdata, name);

	if(CDM_GetViewMode(CDM_DISP_VIEW_DB))
		printf("Query: %s\n", query);

	SemWait(system_wait_sig);

	sqlite3_exec(SystemDB, "BEGIN;", NULL, NULL, NULL);
	err_code = sqlite3_exec(SystemDB, query, 0, 0, &errMsg);
	if(err_code!= SQLITE_OK)
	{
		sqlite3_exec(SystemDB, "ROLLBACK;", NULL, NULL, NULL);
		printf("[%s] sqlite3_exec Fail: %s[code:%d]\n", __FUNCTION__, errMsg, err_code);
		sqlite3_free(errMsg);

		SemPost(system_wait_sig);
		return -1;
	}
	else
	{
		sqlite3_exec(SystemDB, "COMMIT;", NULL, NULL, NULL);
		sqlite3_exec(DULSystemDB, "BEGIN;", NULL, NULL, NULL);
		err_code = sqlite3_exec(DULSystemDB, query, 0, 0, &errMsg);
		if(err_code!= SQLITE_OK)
			sqlite3_exec(DULSystemDB, "ROLLBACK;", NULL, NULL, NULL);
		else
			sqlite3_exec(DULSystemDB, "COMMIT;", NULL, NULL, NULL);
	}

	SemPost(system_wait_sig);

	return 0;
}

int DBM_GetSetting(char *table, char *name, char *GetData)
{
	char query[128] = {0,};
	int ret=0, n_cols=0, type=0, i=0;

	if(table == NULL)
		return -1;
	else if(name == NULL)
	{
		name = "*";
	}

	sprintf(query, "SELECT data FROM %s WHERE Name = '%s'", table, name);

	if(CDM_GetViewMode(CDM_DISP_VIEW_DB))
		printf("Query: %s\n", query);

	SemWait(system_wait_sig);
	if(sqlite3_prepare(SystemDB, query, strlen(query), &system_stmt, NULL) != SQLITE_OK)
	{
		printf("[%s] %s Fail\n", __FILE__, __FUNCTION__);
		SemPost(system_wait_sig);
		return -1;
	}

	while (1)
	{
		ret = sqlite3_step (system_stmt);
		if (ret == SQLITE_DONE)
	      		break;		//- end of result set
	      		
		n_cols = sqlite3_column_count (system_stmt);
		//printf("[%s]colums count!: %d\n", __FUNCTION__, n_cols);
		for (i = 0; i < n_cols; i++)
		{
			//- update the DBF export fields analyzing fetched data *-
			type = sqlite3_column_type (system_stmt, i);
			if (type == SQLITE_TEXT)
			{
				strcpy(GetData, (char *)sqlite3_column_text (system_stmt, i));
			}
			else if(type == SQLITE_INTEGER )
			{
				int nValue = sqlite3_column_int(system_stmt, i );
				sprintf(GetData, "%d", nValue);
			}
			else if(type == SQLITE_FLOAT )
			{
				double dlValue = sqlite3_column_double(system_stmt, i);
				sprintf(GetData, "%f", dlValue);
			}
			
		}
	}

	sqlite3_finalize(system_stmt);
	SemPost(system_wait_sig);
	
	return 0;
}


