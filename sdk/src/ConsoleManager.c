/******************************************************************************
 **                     Copyright(C) 2016 SANION Co. Ltd.                    **
 **                  --------------------------------------                  **
 **                              WWW.SANION.COM                              **
 ******************************************************************************
    PROJECT     : UR-IED
 ******************************************************************************
    FILE NAME   : ConsolManager.c
    AUTHOR      : sootoo23
    DATE        : 2016.11.10
    REVISION    : V1.00
    DESCRIPTION : Console Display & Command Line (CLI + CDM)
 ******************************************************************************
    HISTORY     :
        2016-11-10 Create (ConsoleDisplayManager + CommandLintInterface)
 ******************************************************************************/

/**
 * Update: Oct-24-2023 Default prompt mode : DEVELOPER
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>  
#include <sys/time.h>
#include <time.h>
#include <ctype.h>
#include <stdint.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <glob.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/if_arp.h> // Include this header

#include "Common.h"
#include "VT100.h"
#include "UR_SDK.h"
#include "main.h"
#include "wdt_ds1832.h"
#include "SharedMemMgr.h"
#include "DBManager.h"
#include "SystemSettingModule.h"
#include "ConsoleManager.h"
#include "IOManager.h"
#include "MSManager.h"
#include "PowerSystem.h"

#include "PDMonitoring.h"
#include "GISMonitoring.h"
#include "MTrMonitoring.h"

#include "cidcsvmanager.h"

#define HELP_VIEW "\
USAGE: \n\
VIEW [Type] [On:1/Off:0]\n\
Example:\n\
    \n"


#define CNTLQ      			0x11
#define CNTLS      			0x13

#define MODEMDEVICE "/dev/ttyS1"

#define CLI_STRCNT 256

dsTotalDataToCU*	pd1data;
dsTotalDataToCU*	pd2data;
dsTotalDataToCU*	pd3data;
dsTotalDataToCU*	pd4data;
dsTotalDataToCU*	pd5data;
dsTotalDataToCU*	pd6data;
dsTotalDataToCU*	pd7data;
dsTotalDataToCU*	pd8data;
dsTotalDataToCU*	pd9data;
dsTotalDataToCU*	pd10data;
dsTotalDataToCU*	pd11data;
dsTotalDataToCU*	pd12data;
dsTotalDataToCU*	pd13data;
dsTotalDataToCU*	pd14data;
dsTotalDataToCU*	pd15data;
dsTotalDataToCU*	pd16data;
dsTotalDataToCU*	pd17data;
dsTotalDataToCU*	pd18data;
dsTotalDataToCU*	pd19data;
dsTotalDataToCU*	pd20data;
dsTotalDataToCU*	pd21data;
dsTotalDataToCU*	pd22data;
dsTotalDataToCU*	pd23data;
dsTotalDataToCU*	pd24data;

// for DGAs
dsTotalDataToCU*	dga1data;
dsTotalDataToCU*	dga2data;
dsTotalDataToCU*	dga3data;
dsTotalDataToCU*	dga4data;
dsTotalDataToCU*	dga5data;
dsTotalDataToCU*	dga6data;

// for OLTCs
dsTotalDataToCU*	oltc1data;
dsTotalDataToCU*	oltc2data;
dsTotalDataToCU*	oltc3data;

// for Bushings
dsTotalDataToCU*	bsh1data;
dsTotalDataToCU*	bsh2data;

// for ACQs
 dsTotalDataToCU*	acq1data;
 dsTotalDataToCU*	acq2data;
 dsTotalDataToCU*	acq3data;
 dsTotalDataToCU*	acq4data;
 dsTotalDataToCU*	acq5data;
 dsTotalDataToCU*	acq6data;
 dsTotalDataToCU*	acq7data;
 dsTotalDataToCU*	acq8data;
 dsTotalDataToCU*	acq9data;

static struct termios org_settings;
static struct termios new_settings;

static char strBuffer[CLI_STRCNT];			// stdin ���ڿ� ������ ����Ǵ� ����
static char strOldBuffer[10][CLI_STRCNT];	// ���� ���� ���ڿ��� ����Ǵ� ����
static char strToken[CLI_STRCNT];			// ?�큰 ?�태�?바꿔 ?�?�되??버퍼
static char str_TokenParm1[CLI_STRCNT];		// Param ���� ����
static int Debug_AccessMode;

int terminal_fd			= 0;
int Console_ThreadExit	= 1;
int Console_ViewMode	= 0;
int Console_OldCmdCnt	= 10;	// ���� ���� ���� ?�전 명령 ?�행???�청?? cnt

volatile int g_s32CDM_command = 0;
volatile int g_s32CDM_PrevCmd = 0;	//Used Initialize

static const T_COMMAND_LIST g_tCommandList[] =
{
	{"---------------",	NULL,						0,	"--------------------------------------",	DBG_MODE_USER},
	{"?",				CDM_HelpList,				1,	"HELP",										DBG_MODE_USER},
	{"HELP",			CDM_HelpList,				1,	"����[����:0..3]",								DBG_MODE_USER},
	{"VER",				CDM_DispVersion,			1,	"���α׷� ����",									DBG_MODE_USER},
	{"DEBUG",			Console_SetDebugMode,		1,	"����� ���[���� �ڵ�]",							DBG_MODE_USER},
	{"VIEW",			CDM_SetViewMode,			2,	"Console Debugging",						DBG_MODE_USER},
	{"KILL",			Exit_App,					0,	"Exit",										DBG_MODE_SUPERVISOR},
	{"DATE",			CDM_SetDate,				3,	"Display Date [Y/M/D]",						DBG_MODE_OPERATOR},
	{"TIME",			CDM_SetTime,				3,	"Display Time [H:M:S]",						DBG_MODE_OPERATOR},
	{"SETMAC",			CDM_SetMAC,					2,	"Set MAC Address",							DBG_MODE_OPERATOR},
	{"SETIP",			CDM_SetIp,		 			3,  "Set IP Address for MPU/SNTP", 	   DBG_MODE_OPERATOR},
	{"SETSM",			CDM_SetSubnet,	 			3,  "Set Subnet Mask for MPU/SNTP",	   DBG_MODE_OPERATOR},
	{"SETGW",			CDM_SetGateway,  			3,  "Set Gateway for MPU/SNTP",		   DBG_MODE_OPERATOR},


	{"UPDATE",			IOM_IOBdUpdate,				1,	"F/W Update",								DBG_MODE_OPERATOR},
	{"UPDATE IO",		IOM_IOBdUpdate,				0,	"IO Board F/W Update",						DBG_MODE_OPERATOR},

	
	{"---------------", NULL,						0,	"--------------------------------------",	DBG_MODE_SUPERVISOR},
	{"DIINFO",			IOM_DispDboTm,				0,	"Display DI Debounce Time Info",			DBG_MODE_USER},
	{"DIMO",			CDM_SetFlagDI,				0,	"DI Status Monitor",						DBG_MODE_USER},
	{"DOMO",			CDM_SetFlagDO,				0,	"DO Status Monitor",						DBG_MODE_USER},
	{"MEASMO",			CDM_SetFlagMEAS,			0,	"Measurement Status Monitor",				DBG_MODE_USER},		// added by symoon
	{"DBO",				IOM_CLISetDebounceTm,		2,	"Set to IO Debounce Time",					DBG_MODE_OPERATOR},
	{"DI_UNLOCK",		CDM_DI_Unlock_Execute,		0,	"DI Lock ����",								DBG_MODE_SUPERVISOR},
	{"DOCTRL",			IOM_DOControl,				2,	"DO ���� ����",									DBG_MODE_SUPERVISOR},
	{"STSO",			CDM_DisplayOLTCSTS,			1,	"OLTC Status Monitor",						DBG_MODE_USER},		// added by symoon
	{"STSB",			CDM_DisplayBSHSTS,			1,	"Bushing Status Monitor",					DBG_MODE_USER},		// added by symoon
	{"STSD",			CDM_DisplayDGASTS,			1,	"DGA Status Monitor",						DBG_MODE_USER},		// added by symoon
	{"STSA",			CDM_DisplayACQSTS,			1,	"ACQ Status Monitor",						DBG_MODE_USER},		// added by symoon
	{"STSP",			CDM_DisplayPDSTS,			1,	"PD Status Monitor",						DBG_MODE_USER},		// added by symoon

	{"---------------",	NULL,						0,	"--------------------------------------",	DBG_MODE_SUPERVISOR},
	{"AVF", 			MSM_SetAutoVoltFactor,		3,	"Auto Voltage Factor Setting",				DBG_MODE_SUPERVISOR},
	{"AOF", 			MSM_SetAutoOCTCurrFactor,	3,	"Auto OCT Current Factor Setting",			DBG_MODE_SUPERVISOR},
	{"ABF", 			MSM_SetAutoBCTCurrFactor,	3,	"Auto BCT Current Factor Setting",			DBG_MODE_SUPERVISOR},
	{"FACTOR",			MSM_Factor, 				3,	"Factor Display/Modification",				DBG_MODE_SUPERVISOR},
	{"ADCADJ",			MSM_SetACDAutoFactor,		2,	"ADC OFFSET",								DBG_MODE_SUPERVISOR},
	
	{"---------------",	NULL,						0,	"<<<<<<<<< [4~20mA TD FACTOR] >>>>>>>>>",	DBG_MODE_SUPERVISOR},
	{"TDF", 			Measurment_SetTDFactor, 	2,	"4~20mA Factor Setting",					DBG_MODE_SUPERVISOR},
	{"TDF SET", 		Measurment_SetTDFactor, 	2,	"4~20mA Factor Setting",					DBG_MODE_SUPERVISOR},
	{"TDF RESET",		Measurment_SetTDFactor,		2,	"4~20mA Factor Setting",					DBG_MODE_SUPERVISOR},
	{"TD",				Measurment_PrintTDFactor,	0,	"4~20mA Factor Display",					DBG_MODE_SUPERVISOR},

	{"---------------",	NULL,						0,	"<<<<<<<<< [4~20mA TD FACTOR] >>>>>>>>>",	DBG_MODE_SUPERVISOR},
	{"FRESET",			CLI_FactorResetCmd,			1,	"All Channel Factor Value 1",				DBG_MODE_SUPERVISOR},
	{"CSVLEAF",			CDM_SetFlagCSVLEAFS,		0,	"Read Leafs From CSV FILE",					DBG_MODE_USER}		// added by jibon
};



extern dsOLTCMonInfo			gdsOLTCMonInfo[MAX_MTR_OLTCS];					// added by symoon
extern dsBshMonCalRes			gdsBshMonCalRes[MAX_MTR_BUSHINGS];					// added by symoon
extern dsDgaMonCalRes			gdsDgaMonCalRes[MAX_MTR_TANKS];						// added by symoon

dsDgaBufferForIEC61850			gdsTempDgaBufferForIEC61850[MAX_MTR_TANKS][SIZE_OF_DGA_DATA_LIST_TO_CU];		// added by symoon for testing
dsOLTCBufferForIEC61850			gdsTempOLTCBufferForIEC61850[MAX_MTR_OLTCS][SIZE_OF_OLTC_DATA_LIST_TO_CU];		// added by symoon for testing
dsBSHBufferForIEC61850			gdsTempBSHBufferForIEC61850[MAX_MTR_BUSHINGS][SIZE_OF_BSH_DATA_LIST_TO_CU];		// added by symoon for testing
dsACQBufferForIEC61850			gdsTempACQBufferForIEC61850[MAX_GIS_ACQS][SIZE_OF_ACQ_DATA_LIST_TO_CU];			// added by symoon for testing
dsPDBufferForIEC61850			gdsTempPDBufferForIEC61850[MAX_PD_CHANNELS][SIZE_OF_PD_DATA_LIST_TO_CU];		// added by symoon for testing


//sootoo23 - 20150702: Keboard Input Check Function Add
int linux_kbhit(void)
{
	struct timeval tv = { 0L, 0L };
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(0, &fds);
	return select(1, &fds, NULL, NULL, &tv);
}

void Console_Init(void)
{
	// O_NOCTTY = Ctrl key Not used.
	terminal_fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY); 
    if (terminal_fd <0) {perror(MODEMDEVICE); exit(-1); }
	
	tcgetattr(terminal_fd,&org_settings);
	memcpy((void *)&new_settings, (void *)&org_settings, sizeof(struct termios));

	new_settings.c_cc[VINTR]    = 0;     /* Ctrl-c */ 
	new_settings.c_cc[VQUIT]    = 0;     /* Ctrl-\ */
	new_settings.c_cc[VERASE]   = 0;     /* del */
	new_settings.c_cc[VERASE] = KEY_BACKSPACE;
	//new_settings.c_cc[VKILL]    = 0;     /* @ */
	new_settings.c_cc[VEOF]     = 4;     /* Ctrl-d */
	new_settings.c_cc[VTIME]    = 0;     /* inter-character timer unused */
	new_settings.c_cc[VMIN]     = 1;     /* blocking read until 1 character arrives */
	//new_settings.c_cc[VSWTC]    = 0;     /* '\0' */
	new_settings.c_cc[VSTART]   = 0;     /* Ctrl-q */ 
	new_settings.c_cc[VSTOP]    = 0;     /* Ctrl-s */
	new_settings.c_cc[VSUSP]    = 0;     /* Ctrl-z */
	//new_settings.c_cc[VEOL]     = 0;     /* '\0' */
	new_settings.c_cc[VREPRINT] = 0;     /* Ctrl-r */
	new_settings.c_cc[VDISCARD] = 0;     /* Ctrl-u */
	new_settings.c_cc[VWERASE]  = 0;     /* Ctrl-w */
	new_settings.c_cc[VLNEXT]   = 0;     /* Ctrl-v */
	//new_settings.c_cc[VEOL2]    = 0;     /* '\0' */	

	if(tcsetattr(terminal_fd, TCSANOW, &new_settings) !=0)
	{
		fprintf(stderr, "could not set attributes\n");
	}

	Debug_AccessMode = DBG_MODE_USER;
	//Debug_AccessMode = DBG_MODE_DEVELOPER;
	Console_ThreadExit = 0;
	Console_ViewMode = 0;

	//MO Data init
	//DispMData = (dsMeasInfo*)&(g_pCoreSharedMem->m_dsMeasValue);

	Init();
}

void Console_DeInit()
{
	if(tcsetattr(terminal_fd, TCSANOW, &org_settings) !=0)
	{
		fprintf(stderr, "could not set attributes\n");
	}

	Console_ThreadExit = 1;
}

void CDM_Routine()
{	
	switch(g_s32CDM_command)
	{
		case CDM_DISPLAY_DISTS :
		{
			CDM_Display_DI();
			VT100_SaveCursor();
			break;
		}
		case CDM_DISPLAY_DOSTS :
		{
			CDM_Display_DO();
			VT100_SaveCursor();
			break;
		}
		// added by symoon
		case CDM_DISPLAY_MEASSTS :
		{
			CDM_Display_MEAS();
			VT100_SaveCursor();
			break;
		}
		case CDM_DISPLAY_CSVLEAFS:
		{
			CDM_Display_CSVLEAFS();
			VT100_SaveCursor();
			//g_s32CDM_command = CDM_DISPLAY_NONE;
			break;
		}
		
		default : 
			break;
	}

	g_s32CDM_PrevCmd = g_s32CDM_command;
}

void *CLI_FactorResetCmd(int argc, void *argv[])
{
	U16 u16PassCode;
	
	if(argc == 0)
	{
		printf("FRESET [PASSCODE]\n ");
		return 0;
	}
	else	 u16PassCode = strtoul(argv[0], NULL, 10);
	
	if(u16PassCode == CON_PASSCODE)
	{
		printf("\n Factor Setting Default--\n");
		MSM_FactorReset();
	}
	else
	{
		printf("?�못??PassCode?�니??\n");
		return 0;
	}
	
	return 0;
}


int CLI_GetCommandLine(void)
{
	//sootoo23-20150716
	//Command Input Routine Modify
	int ch = 0 ,buf_cnt = 0;
	
	memset(strBuffer, 0x0, sizeof(strBuffer));

	CDM_DispPrompt();
	for(; !(ch=='\n');)
	{
		if((ch = getch()) == 27 && (ch = getch()) == 91) //arrow & etc function key ignore
		{
			ch = getch();
			#if 0
			if((ch == KEY_LEFT) && (buf_cnt > 0))
			{
				buf_cnt--;
				printf("%c%c%c", 27,91,ch);
			}
			#endif
			if (ch == KEY_UP)
			{
				/*
				*	sootoo23-20160125: key up event (?�전 명령 불러?�기)
				*	Key Up -> "\n" 1byte char remove
				*/
				Console_OldCmdCnt--;
				if(Console_OldCmdCnt < 0)
					Console_OldCmdCnt = 0;

				// ?�전 ?�용 지?�기.
				while(buf_cnt > 0)
				{
					buf_cnt--;
					strBuffer[buf_cnt] = 0x0;
					printf("%c %c", KEY_BACKSPACE, KEY_BACKSPACE);
				}

				// ?�전 명령 copy
				memcpy((char *)strBuffer, (char *)(strOldBuffer[Console_OldCmdCnt]), (strlen(strOldBuffer[Console_OldCmdCnt])-1));
				printf("%s", strBuffer);
				buf_cnt = strlen(strBuffer);
			}
			else if (ch == KEY_DOWN)
			{
				/*
				*	sootoo23-20190225: key down event (?�음 명령 불러?�기)
				*	Key down -> "\n" 1byte char remove
				*/
				Console_OldCmdCnt++;
				if(Console_OldCmdCnt > 10)
					Console_OldCmdCnt = 10;

				// ?�전 ?�용 지?�기.
				while(buf_cnt > 0)
				{
					buf_cnt--;
					strBuffer[buf_cnt] = 0x0;
					printf("%c %c", KEY_BACKSPACE, KEY_BACKSPACE);
				}

				// ?�음 명령 copy
				memcpy((char *)strBuffer, (char *)(strOldBuffer[Console_OldCmdCnt]), (strlen(strOldBuffer[Console_OldCmdCnt])-1));
				printf("%s", strBuffer);
				buf_cnt = strlen(strBuffer);
			}
			
			continue;
		}

		// sootoo23-190410: ASCII code�?벗어??값�? 버린??
		if(ch > 0x7F || ch < 0)
			return 0;
		
		if(ch == KEY_BACKSPACE ) //back space key
		{
			if(buf_cnt > 0)
			{
				buf_cnt--;
				strBuffer[buf_cnt] = 0x0;
				printf("%c %c", KEY_BACKSPACE, KEY_BACKSPACE);
			}
			else
			{
				continue;
			}
		}
		else
		{
			if(((strncmp(strBuffer,"debug ",6)==0) || (strncmp(strBuffer,"DEBUG ",6)==0))
				&& ch != '\n')
				printf("*");
			else
				printf("%c", ch);
			
			strBuffer[buf_cnt] += (char)ch;
			buf_cnt++;
		}
	}

	//Command Save
	if(strlen(strBuffer) > 1)
	{
		strcpy(strOldBuffer[0], strOldBuffer[1]);
		strcpy(strOldBuffer[1], strOldBuffer[2]);
		strcpy(strOldBuffer[2], strOldBuffer[3]);
		strcpy(strOldBuffer[3], strOldBuffer[4]);
		strcpy(strOldBuffer[4], strOldBuffer[5]);
		strcpy(strOldBuffer[5], strOldBuffer[6]);
		strcpy(strOldBuffer[6], strOldBuffer[7]);
		strcpy(strOldBuffer[7], strOldBuffer[8]);
		strcpy(strOldBuffer[8], strOldBuffer[9]);
		
		strcpy(strOldBuffer[9], strBuffer);
		Console_OldCmdCnt = 10;
	}
	
	return 0;
}

int CLI_MakeArguments_v3(const char *s, char *token, char **argvp)
{
    char delimiters[] = " \r\n";
    char *pToken;
    int numtokens;

    strcpy(token, s);

    numtokens = 0;
    pToken = strtok(token, delimiters);
    while(pToken != NULL)
    {
        argvp[numtokens] = pToken;
        if(numtokens == 1)
        {
            int charoffset = pToken - token;
            strcpy(str_TokenParm1 , s + charoffset);
        }
		
		if(*pToken == '\"') // 공백?�함??문자??처리
		{
#if 0   //문자???�작??" 기호가 붙음
			int charoffset = pToken - token;
			strcpy(pToken , s + charoffset);

			numtokens++;
			pToken = strchr((char*)(pToken+1), '\"');
			pToken = strchr(pToken, '\"');
			if(pToken == NULL) return 0;
			*pToken = '\0';
			pToken = strtok(pToken+1, delimiters);
#endif

#if 1   // 문자?�도 ?�큰??추�?
			char *pCheck;
			int charoffset = pToken - token;
			strcpy(pToken , s + charoffset);
			pCheck = strchr((char*)(pToken+1), '\"');
			if(pCheck == NULL) return -1;
			pToken = strtok(pToken, "\"");
#endif
		}
		else
		{
			numtokens++;
			pToken = strtok(NULL, delimiters);
		}
	}

	return numtokens;
}

T_COMMAND_LIST *CLI_SearchCommand(char* argv)
{
    int i;
    int nCmdLength;

    nCmdLength = strlen(argv);

	//printf("Command: %s Length:%d\n", (char *)argv, nCmdLength);
    for(i = 0; i < sizeof(g_tCommandList) / sizeof(T_COMMAND_LIST); i++)
    {
    	//printf("pcCommand:%s\n", g_tCommandList[i].pcCommand);
        if(strlen(g_tCommandList[i].pcCommand) != nCmdLength) 
			continue;
        else if(strncasecmp(g_tCommandList[i].pcCommand, argv, nCmdLength) == 0)
        {
        	//printf("[%s] msg:%s\n", __func__, g_tCommandList[i].pcMessage);
            return (T_COMMAND_LIST *)&g_tCommandList[i];
        }
    }
    return NULL;
}

void *CLI_SystemCmd(void)
{
	system(strBuffer);	
	return 0;
}

void CLI_Routine()
{
	int numtokens;
	int cmdstatus = 0;
	T_COMMAND_LIST *cmd_list;
	void* (*function)(int argc, void *argv[]);
	char *myargev[20];
	
	if(linux_kbhit() || g_s32CDM_command == CDM_DISPLAY_NONE)
	{	
		if(g_s32CDM_command != CDM_DISPLAY_NONE)
		{
			VT100_RestoreCursor();
			g_s32CDM_command = CDM_DISPLAY_NONE;
		}
		
		cmdstatus = CLI_GetCommandLine();
		if(cmdstatus == 0)
		{		
			numtokens = CLI_MakeArguments_v3(strBuffer, strToken, myargev);
			if(numtokens < 0)
			{
				printf("Error : Invalid command\n");
				goto CLI_EXIT;
			}
			else if(numtokens > 0)
			{
				//printf("%s %d %s\n", __func__, __LINE__, myargev[0]);
				cmd_list = CLI_SearchCommand(myargev[0]);
				if(cmd_list != NULL)
				{
					if(Debug_AccessMode < cmd_list->pcAccessMode)
					{
						printf("Error : Invalid command\n");
						goto CLI_EXIT;
					}
					
					function = cmd_list->pFunction;
					function((numtokens-1), (void**)&myargev[1]);//Execute;
				}
				else if(Debug_AccessMode == DBG_MODE_DEVELOPER)
					CLI_SystemCmd();
				else
					printf("Error : Invalid command\n");
			}
		}

	}	

CLI_EXIT:
	return;
}

////////////////////////////////////////////////////////////////////////////
//						  Console Function								  //
////////////////////////////////////////////////////////////////////////////
void CDM_Logo(void)
{
	usleep(500000);    //wait 500ms
	VT100_scrclr();
	VT100_goto(1,1);

	printf("\n");
	printf("************************************************************\n");
	printf("**             Copyright(C) SANION Co., Ltd.              **\n");
	printf("**         -------------------------------------          **\n");
	printf("**            Product: DACU           V: 1.00             **\n");
	printf("************************************************************\n");
	printf("\n");
}

void CDM_DispPrompt(void)
{
	if(Debug_AccessMode == DBG_MODE_USER)
		printf("PROMPT> ");
	else if(Debug_AccessMode == DBG_MODE_OPERATOR)
		printf("OPERATOR> ");
	else if(Debug_AccessMode == DBG_MODE_SUPERVISOR)
		printf("SUPERVISOR> ");
	else if(Debug_AccessMode == DBG_MODE_DEVELOPER)
		printf("DEVELOPER> ");
}

void *CDM_HelpList(int argc, void *argv[])
{
	int i = 0, s32DispLevel=0, s32LevelCnt=0;
	int dispCnt = 0;

	VT100_scrclr();
	VT100_goto(0,50);

	if(argc <= 0)
		s32DispLevel = 0;
	else
		s32DispLevel = convert_Str_to_DEC((char *)argv[0]);

	printf ("\n================+=========================================\n");
	printf (" %-*s\t|    %s\n", 8,"CMD","Comment");
	printf ("================+=========================================\n");
	for(i = 0; i < (sizeof(g_tCommandList)/sizeof(T_COMMAND_LIST)); i++)
	{
		if(strncmp((char*)g_tCommandList[i].pcCommand, ">>>LEVEL", 8) == FALSE) 
			s32LevelCnt++;

		if(s32LevelCnt > s32DispLevel)
			break;
	
		if((s32LevelCnt == s32DispLevel)&&(g_tCommandList[i].pcAccessMode <= Debug_AccessMode))
		{
			if(strncmp((char*)g_tCommandList[i].pcCommand, ">>>LEVEL", 8) == FALSE)
				continue;
			
			printf ("[%-15s]%d: %s\n", g_tCommandList[i].pcCommand, g_tCommandList[i].pcParamCnt, g_tCommandList[i].pcMessage);
			dispCnt++;
		}
	}

	if(dispCnt == 0)
		printf("\n\tThere is no data to be displayed.\n\n");
	
	printf ("================+=========================================\n\n");
	return 0;
}

void *CDM_DispVersion(int argc, void *argv[])
{	
	int i=0;
	U16 u16PassCode;
	struct tm *pTime;

	if(argc == 1)	u16PassCode = strtoul(argv[0], NULL, 10);
	
	VT100_scrclr();
	VT100_goto(0,50);

	// symoon : actual function needed to be implemented

	return 0;
}

void *Console_SetDebugMode(int argc, char * argv[])
{
	char delimiters[] = " \r\n";
	char PW_buffer[512] = {0,};
	char *token_string = NULL;
	char query_ret[20] = {0,};
	int i = 0;

	if( argc >= 2)
	{
		Debug_AccessMode = DBG_MODE_USER;
		printf("\n[Console Mode]: USER MODE...\n");
		return 0;
	}
	
	strcpy(PW_buffer,str_TokenParm1);
	memset(str_TokenParm1, 0x0, sizeof(str_TokenParm1));

	if(strlen(PW_buffer) == 0)
	{
		new_settings.c_lflag &= ~ECHO;
		new_settings.c_cc[VINTR] = 0;
		if(tcsetattr(fileno(stdin), TCSANOW, &new_settings) !=0)
		{
		    fprintf(stderr, "could not set attributes\n");
		}

		printf("\nPassword : ");
		if(fgets(PW_buffer, sizeof(PW_buffer), stdin) != NULL)
		{
			for(i=0; i < sizeof(PW_buffer); i++)
			{
				if(PW_buffer[i]>= 'a' && PW_buffer[i]<='z')
				{
					PW_buffer[i] = PW_buffer[i]-32;
				}
			}
			
			token_string = strtok(PW_buffer, delimiters);
		}
		new_settings.c_lflag |= ECHO;
		if(tcsetattr(fileno(stdin), TCSANOW, &new_settings) !=0)
		{
		    fprintf(stderr, "could not set attributes\n");
		}
		if(token_string == NULL)
		{
			Debug_AccessMode = DBG_MODE_USER;
			printf("\n[Console Mode]:USER MODE...\n");
			return 0;
		}	
	}
	else
	{
		for(i=0; i < sizeof(PW_buffer); i++)
		{
			if(PW_buffer[i]>= 'a' && PW_buffer[i]<='z')
			{
				PW_buffer[i] = PW_buffer[i]-32;
			}
		}
		token_string = strtok(PW_buffer, delimiters);
	}
	printf("HopHop4!!!!\n");
	DBM_GetSetting((char *)"IEDSetting", (char *)"SuperVisor", (char *)query_ret);
	printf("HopHop5!!!!\n");
	if(!strcmp(token_string, query_ret))
	{
		Debug_AccessMode = DBG_MODE_SUPERVISOR;
		printf("\n[Console Mode]: SUPERVISOR MODE...\n");
		return 0;
	}
	
	DBM_GetSetting((char *)"IEDSetting", (char *)"Operator", (char *)query_ret);
	if(!strcmp(token_string, query_ret))
	{
		Debug_AccessMode = DBG_MODE_OPERATOR;
		printf("\n[Console Mode]: OPERATOR MODE...\n");
		return 0;
	}

	DBM_GetSetting((char *)"IEDSetting", (char *)"Developer", (char *)query_ret);
	if(!strcmp(token_string, query_ret))
	{
		Debug_AccessMode = DBG_MODE_DEVELOPER;
		printf("\n[Console Mode]: DEVELOPER MODE...\n");
		WDT_AppStop();
		return 0;
	}

	Debug_AccessMode = DBG_MODE_USER;
	printf("\n[Console Mode]: USER MODE...\n");
	return 0;
}

void *CDM_SetViewMode(int argc, void *argv[])
{
	U8 au8Type[5] = {0,};
	U8 u8Pos = 0;
	U32 u32Val = 0;
	
	if(argc < 2)
	{
		printf(HELP_VIEW);
		printf("Cur View: 0x%x\n", Console_ViewMode);
		return 0;
	}
	
	strcpy((char *)au8Type, (char *)argv[0]);
	u32Val = convert_Str_to_DEC((char *)argv[1]);

	if(u32Val < 0 || u32Val > 1)
	{
		printf(HELP_VIEW);
		return 0;
	}

	printf("View Type: %s, Display: %s\n", (char *)au8Type, (u32Val==0 ? "OFF":"ON"));

	/*if(strncasecmp((char *)au8Type, "ALL", sizeof("ALL")) == 0)
	{
		Console_ViewMode = (u32Val == 1 ? 0xff : 0x0);
		return 0;
	}
	else if(strncasecmp((char *)au8Type, "DSP", sizeof("DSP")) == 0)
		u8Pos = 0x1;
	else if(strncasecmp((char *)au8Type, "STB", sizeof("STB")) == 0)
		u8Pos = 0x2;
	else if(strncasecmp((char *)au8Type, "PCIF", sizeof("PCIF")) == 0)
		u8Pos = 0x4;
	else if(strncasecmp((char *)au8Type, "IO", sizeof("IO")) == 0)
		u8Pos = 0x8;
	else if(strncasecmp((char *)au8Type, "DB", sizeof("DB")) == 0)
		u8Pos = 0x10;
	else if(strncasecmp((char *)au8Type, "HMI", sizeof("HMI")) == 0)
		u8Pos = 0x20;
	else if(strncasecmp((char *)au8Type, "SMO", sizeof("SMO")) == 0)
		u8Pos = 0x40;
	else if(strncasecmp((char *)au8Type, "EVT", sizeof("EVT")) == 0)
		u8Pos = 0x80;*/
	
	Console_ViewMode = (u32Val == 1 ? (Console_ViewMode | u8Pos) : (Console_ViewMode & ~u8Pos));
	
	return 0;
}

int CDM_GetViewMode(CDM_DISP_VIEW type)
{
	int ret = Console_ViewMode;

	if(type != CDM_DISP_VIEW_ALL)
		ret = (int)(ret & (int)pow(2,(type - 1)));

	//printf("[%s] Type:%d / Result:0x%x / %f\n", __func__, type, ret, pow(2,(type - 1)));
	return ret;
}

void *CDM_SetFlagDI(int argc, void *argv[])
{
	g_s32CDM_command = CDM_DISPLAY_DISTS;
	
	VT100_scrclr();
	//DBM_SysDBDispCfg("DI");
	
	VT100_goto(0,10);

	printf("\n\n==========+=======================================");
	printf("\n    Num   |        D I    S t a t u s");
	printf("\n==========+=======================================");
		
	return 0;
}

void *CDM_SetFlagDO(int argc, void *argv[])
{
	g_s32CDM_command = CDM_DISPLAY_DOSTS;

	VT100_scrclr();
	VT100_goto(0,10);

	printf("\n\n========+====================================");
	printf("\n   Num  |     D O  S t a t u s");
	printf("\n========+====================================");

	return 0;
}

// added by symoon
void *CDM_SetFlagMEAS(int argc, void *argv[])
{
	g_s32CDM_command = CDM_DISPLAY_MEASSTS;

	VT100_scrclr();
	VT100_goto(0,1);

	printf("========+==================================================================\n");
	printf(" Name   |  Values                   || Name    |  Values\n");
	printf("========+==================================================================\n");

	return 0;
}

void *CDM_SetFlagCSVLEAFS(int argc, void *argv[])
{
	g_s32CDM_command = CDM_DISPLAY_CSVLEAFS;

	VT100_scrclr();
	VT100_goto(0,1);

	printf("========+==================================================================\n");
	printf("========+==================================================================\n\n");

	return 0;
}

// added by symoon
void *CDM_DisplayOLTCSTS(int argc, void *argv[])
{
	U8	u8OLTCNo;
	U16	u16CRC = 0;

	dsTotalDataToCU*	pdsTotalDataToCU;


	if (argc < 1) {
		printf("STSO [OLTC number (0~2)]\n");
		printf("EX) STSO 2\n");
		return 0;
	}
	else {
		u8OLTCNo = (U8)atoi((char *)argv[0]);

		if (u8OLTCNo == 0) {
			pdsTotalDataToCU	= oltc1data;
		}
		else if (u8OLTCNo == 1) {
			pdsTotalDataToCU	= oltc2data;
		}
		else {
			pdsTotalDataToCU	= oltc3data;
		}

		if (u8OLTCNo > (MAX_MTR_OLTCS-1)) {
			printf("Invalid argument value. Usage: STSO [OLTC number (0~2)]\n");
		}
		else {
			VT100_scrclr();
			VT100_goto(0,1);

			memcpy(&(gdsTempOLTCBufferForIEC61850[u8OLTCNo][pdsTotalDataToCU->m_u32ReadIndex]), pdsTotalDataToCU->m_dsDataList[pdsTotalDataToCU->m_u32ReadIndex].m_pData, SIZE_OF_OLTC_DATA_UNIT_IN_BYTES);

			if (CheckCRC((U8*)(&(gdsTempOLTCBufferForIEC61850[u8OLTCNo][pdsTotalDataToCU->m_u32ReadIndex])), (SIZE_OF_OLTC_DATA_UNIT_IN_BYTES-4), (U16)(gdsTempOLTCBufferForIEC61850[u8OLTCNo][pdsTotalDataToCU->m_u32ReadIndex].m_u32CRC), &u16CRC)) {
				printf("Checksum good for read index %d of OLTC #%1d (read:0x%x / calculated:0x%x)\n", pdsTotalDataToCU->m_u32ReadIndex, (u8OLTCNo+1), (U16)(gdsTempOLTCBufferForIEC61850[u8OLTCNo][pdsTotalDataToCU->m_u32ReadIndex].m_u32CRC), u16CRC);
				printf("=======+==============================+===========================+=============\n");
				printf(" Monitoring Status of OLTC driving motor #%1d\n", (u8OLTCNo+1));
				printf("-------+------------------------------+---------------------------+-------------\n");
				printf(" Name  |  Values          || Name     |  Values         || Name      |  Values\n");
				printf("=======+==============================+===========================+=============\n");
				printf(" Torq  |  %7.2f         || Drv. I   |  %7.2f        || Abrasion  |  %7.2f\n", (float)(gdsTempOLTCBufferForIEC61850[u8OLTCNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsOLTCStatus[ENUM_OLTC_TORQUE].m_u32Value/100.), (float)(gdsTempOLTCBufferForIEC61850[u8OLTCNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsOLTCStatus[ENUM_OLTC_MOTOR_DRIVE_I].m_u32Value/100.), (float)(gdsTempOLTCBufferForIEC61850[u8OLTCNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsOLTCStatus[ENUM_OLTC_ABRASION_LEVEL].m_u32Value/100.));
				printf("-------+------------------------------+---------------------------+-------------\n");
				printf(" OpDur |  %7.2f         || TempDiff |  %7.2fcelsius || OutTemp   |  %7.2fcelsius\n", (float)(gdsTempOLTCBufferForIEC61850[u8OLTCNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsOLTCStatus[ENUM_OLTC_OP_DURATION].m_u32Value/100.), (float)(gdsTempOLTCBufferForIEC61850[u8OLTCNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsOLTCStatus[ENUM_OLTC_OIL_TEMPERATURE_DIFF].m_u32Value/100.), (float)(gdsTempOLTCBufferForIEC61850[u8OLTCNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsOLTCStatus[ENUM_OLTC_OUTSIDE_TEMPERATURE].m_u32Value/100.));
				printf("-------+------------------------------+---------------------------+-------------\n");
				printf(" DrvIA |  %7.2fA        || DrvIB    |  %7.2fA       || DrvIC     |  %7.2fA\n", (float)(gdsTempOLTCBufferForIEC61850[u8OLTCNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsOLTCStatus[ENUM_OLTC_MOTOR_DRIVE_I_PHASE_A].m_u32Value/100.), (float)(gdsTempOLTCBufferForIEC61850[u8OLTCNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsOLTCStatus[ENUM_OLTC_MOTOR_DRIVE_I_PHASE_B].m_u32Value/100.), (float)(gdsTempOLTCBufferForIEC61850[u8OLTCNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsOLTCStatus[ENUM_OLTC_MOTOR_DRIVE_I_PHASE_C].m_u32Value/100.));
				printf("-------+------------------------------+---------------------------+-------------\n");
				printf(" Temp  |  %7.2fcelsius  || Pressure |  %7.2f        || Moisture  |  %7.2f\n", (float)(gdsTempOLTCBufferForIEC61850[u8OLTCNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsOLTCStatus[ENUM_OLTC_OIL_FILTER_TEMPERATURE].m_u32Value/100.), (float)(gdsTempOLTCBufferForIEC61850[u8OLTCNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsOLTCStatus[ENUM_OLTC_OIL_FILTER_PRESSURE].m_u32Value/100.), (float)(gdsTempOLTCBufferForIEC61850[u8OLTCNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsOLTCStatus[ENUM_OLTC_OIL_FILTER_MOISTURE].m_u32Value/100.));
				printf("=======+==============================+===========================+=============\n");
			}
			else {
				printf("Checksum error for read index %d of OLTC #%1d (read:0x%x / calculated:0x%x)\n", pdsTotalDataToCU->m_u32ReadIndex, (u8OLTCNo+1), (U16)(gdsTempOLTCBufferForIEC61850[u8OLTCNo][pdsTotalDataToCU->m_u32ReadIndex].m_u32CRC), u16CRC);
			}
		
			if (++pdsTotalDataToCU->m_u32ReadIndex == SIZE_OF_OLTC_DATA_LIST_TO_CU) {
				pdsTotalDataToCU->m_u32ReadIndex = 0;
			};
		}

		return 0;
	}
}

// added by symoon
void *CDM_DisplayBSHSTS(int argc, void *argv[])
{
	U8	u8BSHNo;
	U16	u16CRC = 0;

	dsTotalDataToCU*	pdsTotalDataToCU;


	if (argc < 1) {
		printf("STSB [Bushing number (0~1)]\n");
		printf("EX) STSB 1\n");
		return 0;
	}
	else {
		u8BSHNo = (U8)atoi((char *)argv[0]);

		if (u8BSHNo == 0) {
			pdsTotalDataToCU	= oltc1data;
		}
		else {
			pdsTotalDataToCU	= oltc3data;
		}

		if (u8BSHNo > (MAX_MTR_BUSHINGS-1)) {
			printf("Invalid argument value. Usage: STSB [Bushing number (0~1)]\n");
		}
		else {
			VT100_scrclr();
			VT100_goto(0,1);

			memcpy(&(gdsTempBSHBufferForIEC61850[u8BSHNo][pdsTotalDataToCU->m_u32ReadIndex]), pdsTotalDataToCU->m_dsDataList[pdsTotalDataToCU->m_u32ReadIndex].m_pData, SIZE_OF_BSH_DATA_UNIT_IN_BYTES);

			if (CheckCRC((U8*)(&(gdsTempBSHBufferForIEC61850[u8BSHNo][pdsTotalDataToCU->m_u32ReadIndex])), (SIZE_OF_BSH_DATA_UNIT_IN_BYTES-4), (U16)(gdsTempBSHBufferForIEC61850[u8BSHNo][pdsTotalDataToCU->m_u32ReadIndex].m_u32CRC), &u16CRC)) {
				printf("Checksum good for read index %d of bushing #%1d (read:0x%x / calculated:0x%x)\n", pdsTotalDataToCU->m_u32ReadIndex, (u8BSHNo+1), (U16)(gdsTempBSHBufferForIEC61850[u8BSHNo][pdsTotalDataToCU->m_u32ReadIndex].m_u32CRC), u16CRC);
				printf("=======+==============================+===========================+=============\n");
				printf(" Monitoring Status of bushing #%1d\n", (u8BSHNo+1));
				printf("-------+------------------------------+---------------------------+-------------\n");
				printf(" Name  |  Values          || Name     |  Values         || Name      |  Values\n");
				printf("=======+==============================+===========================+=============\n");
				printf(" React |  %7.2f         || AbsReact |  %7.2f        || LossFact  |  %7.2f\n", (float)(gdsTempBSHBufferForIEC61850[u8BSHNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsBSHStatus[ENUM_BSH_REACTANCE].m_u32Value/100.), (float)(gdsTempBSHBufferForIEC61850[u8BSHNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsBSHStatus[ENUM_BSH_ABSREACTANCE].m_u32Value/100.), (float)(gdsTempBSHBufferForIEC61850[u8BSHNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsBSHStatus[ENUM_BSH_LOSSFACTOR].m_u32Value/100.));
				printf("-------+------------------------------+---------------------------+-------------\n");
				printf(" Volt  |  %7.2fV        || Displ I  |  %7.2fA       || Leakage I |  %7.2fA\n", (float)(gdsTempBSHBufferForIEC61850[u8BSHNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsBSHStatus[ENUM_BSH_VOLTAGE].m_u32Value/100.), (float)(gdsTempBSHBufferForIEC61850[u8BSHNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsBSHStatus[ENUM_BSH_DISPLACEMENT_I].m_u32Value/100.), (float)(gdsTempBSHBufferForIEC61850[u8BSHNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsBSHStatus[ENUM_BSH_LEAKAGE_I].m_u32Value/100.));
				printf("-------+------------------------------+---------------------------+-------------\n");
				printf(" LkAng |  %7.2fdegrees  || Volt Ang |  %7.2fdegrees || PF        |  %7.2f\n", (float)(gdsTempBSHBufferForIEC61850[u8BSHNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsBSHStatus[ENUM_BSH_LEAKAGE_I_ANG].m_u32Value/100.), (float)(gdsTempBSHBufferForIEC61850[u8BSHNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsBSHStatus[ENUM_BSH_VOLTAGE_ANG].m_u32Value/100.), (float)(gdsTempBSHBufferForIEC61850[u8BSHNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsBSHStatus[ENUM_BSH_POWER_FACTOR].m_u32Value/100.));
				printf("-------+------------------------------+---------------------------+-------------\n");
				printf(" UnbI  |  %7.2fA        || UnbI Ang |  %7.2fdegrees\n", (float)(gdsTempBSHBufferForIEC61850[u8BSHNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsBSHStatus[ENUM_BSH_UNBALANCE_I].m_u32Value/100.), (float)(gdsTempBSHBufferForIEC61850[u8BSHNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsBSHStatus[ENUM_BSH_UNBALANCE_I_ANG].m_u32Value/100.));
				printf("=======+==============================+===========================+=============\n");
			}
			else {
				printf("Checksum error for read index %d of bushing #%1d (read:0x%x / calculated:0x%x)\n", pdsTotalDataToCU->m_u32ReadIndex, (u8BSHNo+1), (U16)(gdsTempBSHBufferForIEC61850[u8BSHNo][pdsTotalDataToCU->m_u32ReadIndex].m_u32CRC), u16CRC);
			}
		
			if (++pdsTotalDataToCU->m_u32ReadIndex == SIZE_OF_BSH_DATA_LIST_TO_CU) {
				pdsTotalDataToCU->m_u32ReadIndex = 0;
			};
		}

		return 0;
	}
}

// added by symoon
void *CDM_DisplayDGASTS(int argc, void *argv[])
{
	U8	u8DGANo;
	U16	u16CRC = 0;

	dsTotalDataToCU*	pdsTotalDataToCU;


	if (argc < 1) {
		printf("STSD [DGA number (0~5)]\n");
		printf("EX) STSD 5\n");
		return 0;
	}
	else {
		u8DGANo = (U8)atoi((char *)argv[0]);

		if (u8DGANo == 0) {
			pdsTotalDataToCU	= dga1data;
		}
		else if (u8DGANo == 1) {
			pdsTotalDataToCU	= dga2data;
		}
		else if (u8DGANo == 2) {
			pdsTotalDataToCU	= dga3data;
		}
		else if (u8DGANo == 3) {
			pdsTotalDataToCU	= dga4data;
		}
		else if (u8DGANo == 4) {
			pdsTotalDataToCU	= dga5data;
		}
		else {
			pdsTotalDataToCU	= dga6data;
		}

		if (u8DGANo > (MAX_MTR_TANKS-1)) {
			printf("Invalid argument value. Usage: STSD [DGA number (0~5)]\n");
		}
		else {
			VT100_scrclr();
			VT100_goto(0,1);

			// ������ ���� �ӽ� �ڵ� ����
			#if (0)
			printf("=======+==============================+===========================+=============\n");
			printf(" Monitoring Status of Dissolved Gas Analyzer #%1d\n", (u8DGANo+1));
			printf("-------+------------------------------+---------------------------+-------------\n");
			printf(" Name  |  Values          || Name     |  Values      || Name      |  Values\n");
			printf("=======+==============================+===========================+=============\n");

			// ������ ����
			printf(" Temp  |  %7.2fcelsius  || Level    |  %7.2f     || Pressure  |  %7.2f\n", gdsDgaMonCalRes[u8DGANo].m_fTemperature, gdsDgaMonCalRes[u8DGANo].m_fLevel, gdsDgaMonCalRes[u8DGANo].m_fPressure);
			// Gas�� ��
			printf("-------+------------------------------+---------------------------+-------------\n");
			printf(" H2O   |  %7.2fppm      || H2       |  %7.2fppm  || N2        |  %7.2fppm\n", gdsDgaMonCalRes[u8DGANo].m_fH2Oppm, gdsDgaMonCalRes[u8DGANo].m_fH2ppm, gdsDgaMonCalRes[u8DGANo].m_fN2ppm);
			printf("-------+------------------------------+---------------------------+-------------\n");
			printf(" CO    |  %7.2fppm      || CO2      |  %7.2fppm  || CH4       |  %7.2fppm\n", gdsDgaMonCalRes[u8DGANo].m_fCOppm, gdsDgaMonCalRes[u8DGANo].m_fCO2ppm, gdsDgaMonCalRes[u8DGANo].m_fCH4ppm);
			printf("-------+------------------------------+---------------------------+-------------\n");
			printf(" C2H2  |  %7.2fppm      || C2H4     |  %7.2fppm  || C2H6      |  %7.2fppm\n", gdsDgaMonCalRes[u8DGANo].m_fC2H2ppm, gdsDgaMonCalRes[u8DGANo].m_fC2H4ppm, gdsDgaMonCalRes[u8DGANo].m_fC2H6ppm);
			printf("-------+------------------------------+---------------------------+-------------\n");
			printf(" O2    |  %7.2fppm      || C3H8     |  %7.2fppm  ||\n", gdsDgaMonCalRes[u8DGANo].m_fO2ppm, gdsDgaMonCalRes[u8DGANo].m_fC3H8ppm);
			printf("=======+==============================+===========================+=============\n");
			#else
			memcpy(&(gdsTempDgaBufferForIEC61850[u8DGANo][pdsTotalDataToCU->m_u32ReadIndex]), pdsTotalDataToCU->m_dsDataList[pdsTotalDataToCU->m_u32ReadIndex].m_pData, SIZE_OF_DGA_DATA_UNIT_IN_BYTES);

			if (CheckCRC((U8*)(&(gdsTempDgaBufferForIEC61850[u8DGANo][pdsTotalDataToCU->m_u32ReadIndex])), (SIZE_OF_DGA_DATA_UNIT_IN_BYTES-4), (U16)(gdsTempDgaBufferForIEC61850[u8DGANo][pdsTotalDataToCU->m_u32ReadIndex].m_u32CRC), &u16CRC)) {
				printf("Checksum good for read index %d of DGA #%1d (read:0x%x / calculated:0x%x)\n", pdsTotalDataToCU->m_u32ReadIndex, (u8DGANo+1), (U16)(gdsTempDgaBufferForIEC61850[u8DGANo][pdsTotalDataToCU->m_u32ReadIndex].m_u32CRC), u16CRC);
				printf("=======+==============================+===========================+=============\n");
				printf(" Monitoring Status of Dissolved Gas Analyzer #%1d\n", (u8DGANo+1));
				printf("-------+------------------------------+---------------------------+-------------\n");
				printf(" Name  |  Values          || Name     |  Values      || Name      |  Values\n");
				printf("=======+==============================+===========================+=============\n");
				
				// ������ ����
				printf(" Temp  |  %7.2fcelsius  || Level    |  %7.2f     || Pressure  |  %7.2f\n", (float)(gdsTempDgaBufferForIEC61850[u8DGANo][pdsTotalDataToCU->m_u32ReadIndex].m_dsDgaStatus[ENUM_DGA_TEMPERATURE].m_u32Value/100.), (float)(gdsTempDgaBufferForIEC61850[u8DGANo][pdsTotalDataToCU->m_u32ReadIndex].m_dsDgaStatus[ENUM_DGA_LEVEL].m_u32Value/100.), (float)(gdsTempDgaBufferForIEC61850[u8DGANo][pdsTotalDataToCU->m_u32ReadIndex].m_dsDgaStatus[ENUM_DGA_PRESSURE].m_u32Value/100.));
				// Gas�� ��
				printf("-------+------------------------------+---------------------------+-------------\n");
				printf(" H2O   |  %7.2fppm      || H2       |  %7.2fppm  || N2        |  %7.2fppm\n", (float)(gdsTempDgaBufferForIEC61850[u8DGANo][pdsTotalDataToCU->m_u32ReadIndex].m_dsDgaStatus[ENUM_DGA_H2O_PPM].m_u32Value/100.), (float)(gdsTempDgaBufferForIEC61850[u8DGANo][pdsTotalDataToCU->m_u32ReadIndex].m_dsDgaStatus[ENUM_DGA_H2_PPM].m_u32Value/100.), (float)(gdsTempDgaBufferForIEC61850[u8DGANo][pdsTotalDataToCU->m_u32ReadIndex].m_dsDgaStatus[ENUM_DGA_N2_PPM].m_u32Value/100.));
				printf("-------+------------------------------+---------------------------+-------------\n");
				printf(" CO    |  %7.2fppm      || CO2      |  %7.2fppm  || CH4       |  %7.2fppm\n", (float)(gdsTempDgaBufferForIEC61850[u8DGANo][pdsTotalDataToCU->m_u32ReadIndex].m_dsDgaStatus[ENUM_DGA_CO_PPM].m_u32Value/100.), (float)(gdsTempDgaBufferForIEC61850[u8DGANo][pdsTotalDataToCU->m_u32ReadIndex].m_dsDgaStatus[ENUM_DGA_CO2_PPM].m_u32Value/100.), (float)(gdsTempDgaBufferForIEC61850[u8DGANo][pdsTotalDataToCU->m_u32ReadIndex].m_dsDgaStatus[ENUM_DGA_CH4_PPM].m_u32Value/100.));
				printf("-------+------------------------------+---------------------------+-------------\n");
				printf(" C2H2  |  %7.2fppm      || C2H4     |  %7.2fppm  || C2H6      |  %7.2fppm\n", (float)(gdsTempDgaBufferForIEC61850[u8DGANo][pdsTotalDataToCU->m_u32ReadIndex].m_dsDgaStatus[ENUM_DGA_C2H2_PPM].m_u32Value/100.), (float)(gdsTempDgaBufferForIEC61850[u8DGANo][pdsTotalDataToCU->m_u32ReadIndex].m_dsDgaStatus[ENUM_DGA_C2H4_PPM].m_u32Value/100.), (float)(gdsTempDgaBufferForIEC61850[u8DGANo][pdsTotalDataToCU->m_u32ReadIndex].m_dsDgaStatus[ENUM_DGA_C2H6_PPM].m_u32Value/100.));
				printf("-------+------------------------------+---------------------------+-------------\n");
				printf(" O2    |  %7.2fppm      || C3H8     |  %7.2fppm  ||\n", (float)(gdsTempDgaBufferForIEC61850[u8DGANo][pdsTotalDataToCU->m_u32ReadIndex].m_dsDgaStatus[ENUM_DGA_O2_PPM].m_u32Value/100.), (float)(gdsTempDgaBufferForIEC61850[u8DGANo][pdsTotalDataToCU->m_u32ReadIndex].m_dsDgaStatus[ENUM_DGA_C3H8_PPM].m_u32Value/100.));
				printf("=======+==============================+===========================+=============\n");
			}
			else {
				printf("Checksum error for read index %d of DGA #%1d (read:0x%x / calculated:0x%x)\n", pdsTotalDataToCU->m_u32ReadIndex, (u8DGANo+1), (U16)(gdsTempDgaBufferForIEC61850[u8DGANo][pdsTotalDataToCU->m_u32ReadIndex].m_u32CRC), u16CRC);
			}
		
			if (++pdsTotalDataToCU->m_u32ReadIndex == SIZE_OF_DGA_DATA_LIST_TO_CU) {
				pdsTotalDataToCU->m_u32ReadIndex = 0;
			};
			#endif
		}

		return 0;
	}
}

// added by symoon
void *CDM_DisplayACQSTS(int argc, void *argv[])
{
	U8	u8ACQNo;
	U16	u16CRC = 0;

	dsTotalDataToCU*	pdsTotalDataToCU;


	if (argc < 1) {
		printf("STSA [ACQ number (0~8)]\n");
		printf("EX) STSA 3\n");
		return 0;
	}
	else {
		u8ACQNo = (U8)atoi((char *)argv[0]);

		if (u8ACQNo == 0) {
			pdsTotalDataToCU	= acq1data;
		}
		else if (u8ACQNo == 1) {
			pdsTotalDataToCU	= acq2data;
		}
		else if (u8ACQNo == 2) {
			pdsTotalDataToCU	= acq3data;
		}
		else if (u8ACQNo == 3) {
			pdsTotalDataToCU	= acq4data;
		}
		else if (u8ACQNo == 4) {
			pdsTotalDataToCU	= acq5data;
		}
		else if (u8ACQNo == 5) {
			pdsTotalDataToCU	= acq6data;
		}
		else if (u8ACQNo == 6) {
			pdsTotalDataToCU	= acq7data;
		}
		else if (u8ACQNo == 7) {
			pdsTotalDataToCU	= acq8data;
		}
		else {
			pdsTotalDataToCU	= acq9data;
		}

		if (u8ACQNo > (MAX_GIS_ACQS-1)) {
			printf("Invalid argument value. Usage: STSA [ACQ number (0~8)]\n");
		}
		else {
			VT100_scrclr();
			VT100_goto(0,1);

			memcpy(&(gdsTempACQBufferForIEC61850[u8ACQNo][pdsTotalDataToCU->m_u32ReadIndex]), pdsTotalDataToCU->m_dsDataList[pdsTotalDataToCU->m_u32ReadIndex].m_pData, SIZE_OF_ACQ_DATA_UNIT_IN_BYTES);

			if (CheckCRC((U8*)(&(gdsTempACQBufferForIEC61850[u8ACQNo][pdsTotalDataToCU->m_u32ReadIndex])), (SIZE_OF_ACQ_DATA_UNIT_IN_BYTES-4), (U16)(gdsTempACQBufferForIEC61850[u8ACQNo][pdsTotalDataToCU->m_u32ReadIndex].m_u32CRC), &u16CRC)) {
				printf("Checksum good for read index %d of ACQ #%1d (read:0x%x / calculated:0x%x)\n", pdsTotalDataToCU->m_u32ReadIndex, (u8ACQNo+1), (U16)(gdsTempACQBufferForIEC61850[u8ACQNo][pdsTotalDataToCU->m_u32ReadIndex].m_u32CRC), u16CRC);
				printf("=======+==============================+===========================+=============\n");
				printf(" Monitoring Status of CB Monitoring Device #%1d\n", (u8ACQNo+1));
				printf("-------+------------------------------+---------------------------+-------------\n");
				printf(" Name  |  Values          || Name     |  Values      || Name      |  Values\n");
				printf("=======+==============================+===========================+=============\n");
				
				printf(" ClsOp |  %9d         || OpTmAlm  |  %9d     || RsOpCnt   |  %9d\n", gdsTempACQBufferForIEC61850[u8ACQNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsACQStatus[ENUM_ACQ_CLOSE_OPERATION].m_u32Value, gdsTempACQBufferForIEC61850[u8ACQNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsACQStatus[ENUM_ACQ_OP_TIME_ALARM].m_u32Value, gdsTempACQBufferForIEC61850[u8ACQNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsACQStatus[ENUM_ACQ_RSOPCNT].m_u32Value);
				printf("-------+------------------------------+---------------------------+-------------\n");
				printf(" ORctTm|  %9d         || OOpSpd   |  %9d     || OOpDur    |  %9d\n", gdsTempACQBufferForIEC61850[u8ACQNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsACQStatus[ENUM_ACQ_OPEN_REACTION_TIME].m_u32Value, gdsTempACQBufferForIEC61850[u8ACQNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsACQStatus[ENUM_ACQ_OPEN_OP_SPEED].m_u32Value, gdsTempACQBufferForIEC61850[u8ACQNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsACQStatus[ENUM_ACQ_OPEN_OP_DURATION].m_u32Value);
				printf("-------+------------------------------+---------------------------+-------------\n");
				printf(" CRctTm|  %9d         || COpSpd   |  %9d     || COpDur    |  %9d\n", gdsTempACQBufferForIEC61850[u8ACQNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsACQStatus[ENUM_ACQ_CLOSE_REACTION_TIME].m_u32Value, gdsTempACQBufferForIEC61850[u8ACQNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsACQStatus[ENUM_ACQ_CLOSE_OP_SPEED].m_u32Value, gdsTempACQBufferForIEC61850[u8ACQNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsACQStatus[ENUM_ACQ_CLOSE_OP_DURATION].m_u32Value);
				printf("-------+------------------------------+---------------------------+-------------\n");
				printf(" CoilI |  %9d         || Temp.    |  %9d     || CoilV     |  %9d\n", gdsTempACQBufferForIEC61850[u8ACQNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsACQStatus[ENUM_ACQ_COIL_I].m_u32Value, gdsTempACQBufferForIEC61850[u8ACQNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsACQStatus[ENUM_ACQ_TEMPERATURE].m_u32Value, gdsTempACQBufferForIEC61850[u8ACQNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsACQStatus[ENUM_ACQ_COIL_V].m_u32Value);
				printf("-------+------------------------------+---------------------------+-------------\n");
				printf(" BIA   |  %9d         || BIB      |  %9d     || BIC       |  %9d\n", gdsTempACQBufferForIEC61850[u8ACQNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsACQStatus[ENUM_ACQ_BUS_IA].m_u32Value, gdsTempACQBufferForIEC61850[u8ACQNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsACQStatus[ENUM_ACQ_BUS_IB].m_u32Value, gdsTempACQBufferForIEC61850[u8ACQNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsACQStatus[ENUM_ACQ_BUS_IC].m_u32Value);
				printf("=======+==============================+===========================+=============\n");
			}
			else {
				printf("Checksum error for read index %d of ACQ #%1d (read:0x%x / calculated:0x%x)\n", pdsTotalDataToCU->m_u32ReadIndex, (u8ACQNo+1), (U16)(gdsTempACQBufferForIEC61850[u8ACQNo][pdsTotalDataToCU->m_u32ReadIndex].m_u32CRC), u16CRC);
			}
		
			if (++pdsTotalDataToCU->m_u32ReadIndex == SIZE_OF_ACQ_DATA_LIST_TO_CU) {
				pdsTotalDataToCU->m_u32ReadIndex = 0;
			};
		}

		return 0;
	}
}

// added by symoon
void *CDM_DisplayPDSTS(int argc, void *argv[])
{

	U8	u8PDChNo;
	U16	u16CRC = 0;

	dsTotalDataToCU*	pdsTotalDataToCU;


	if (argc < 1) {
		printf("STSP [PD channel number (0~23)]\n");
		printf("EX) STSP 10\n");
		return 0;
	}
	else {
		u8PDChNo = (U8)atoi((char *)argv[0]);

		if (u8PDChNo == 0) {
			pdsTotalDataToCU	= pd1data;
		}
		else if (u8PDChNo == 1) {
			pdsTotalDataToCU	= pd2data;
		}
		else if (u8PDChNo == 2) {
			pdsTotalDataToCU	= pd3data;
		}
		else if (u8PDChNo == 3) {
			pdsTotalDataToCU	= pd4data;
		}
		else if (u8PDChNo == 4) {
			pdsTotalDataToCU	= pd5data;
		}
		else if (u8PDChNo == 5) {
			pdsTotalDataToCU	= pd6data;
		}
		else if (u8PDChNo == 6) {
			pdsTotalDataToCU	= pd7data;
		}
		else if (u8PDChNo == 7) {
			pdsTotalDataToCU	= pd8data;
		}
		else if (u8PDChNo == 8) {
			pdsTotalDataToCU	= pd9data;
		}
		else if (u8PDChNo == 9) {
			pdsTotalDataToCU	= pd10data;
		}
		else if (u8PDChNo == 10) {
			pdsTotalDataToCU	= pd11data;
		}
		else if (u8PDChNo == 11) {
			pdsTotalDataToCU	= pd12data;
		}
		else if (u8PDChNo == 12) {
			pdsTotalDataToCU	= pd13data;
		}
		else if (u8PDChNo == 13) {
			pdsTotalDataToCU	= pd14data;
		}
		else if (u8PDChNo == 14) {
			pdsTotalDataToCU	= pd15data;
		}
		else if (u8PDChNo == 15) {
			pdsTotalDataToCU	= pd16data;
		}
		else if (u8PDChNo == 16) {
			pdsTotalDataToCU	= pd17data;
		}
		else if (u8PDChNo == 17) {
			pdsTotalDataToCU	= pd18data;
		}
		else if (u8PDChNo == 18) {
			pdsTotalDataToCU	= pd19data;
		}
		else if (u8PDChNo == 19) {
			pdsTotalDataToCU	= pd20data;
		}
		else if (u8PDChNo == 20) {
			pdsTotalDataToCU	= pd21data;
		}
		else if (u8PDChNo == 21) {
			pdsTotalDataToCU	= pd22data;
		}
		else if (u8PDChNo == 22) {
			pdsTotalDataToCU	= pd23data;
		}
		else {
			pdsTotalDataToCU	= pd24data;
		}

		if (u8PDChNo > (MAX_PD_CHANNELS-1)) {
			printf("Invalid argument value. Usage: STSP [PD channel number (0~23)]\n");
		}
		else {
			VT100_scrclr();
			VT100_goto(0,1);

			memcpy(&(gdsTempPDBufferForIEC61850[u8PDChNo][pdsTotalDataToCU->m_u32ReadIndex]), pdsTotalDataToCU->m_dsDataList[pdsTotalDataToCU->m_u32ReadIndex].m_pData, SIZE_OF_PD_DATA_UNIT_IN_BYTES);

			if (CheckCRC((U8*)(&(gdsTempPDBufferForIEC61850[u8PDChNo][pdsTotalDataToCU->m_u32ReadIndex])), (SIZE_OF_PD_DATA_UNIT_IN_BYTES-4), (U16)(gdsTempPDBufferForIEC61850[u8PDChNo][pdsTotalDataToCU->m_u32ReadIndex].m_u32CRC), &u16CRC)) {
				printf("Checksum good for read index %d of PD channel #%1d (read:0x%x / calculated:0x%x)\n", pdsTotalDataToCU->m_u32ReadIndex, (u8PDChNo+1), (U16)(gdsTempPDBufferForIEC61850[u8PDChNo][pdsTotalDataToCU->m_u32ReadIndex].m_u32CRC), u16CRC);
				printf("=======+==============================+===========================+=============\n");
				printf(" Monitoring Status of PD channel #%1d\n", (u8PDChNo+1));
				printf("-------+------------------------------+---------------------------+-------------\n");
				printf(" Name  |  Values          || Name     |  Values      || Name      |  Values\n");
				printf("=======+==============================+===========================+=============\n");
				
				printf(" Type0 |  %9d         || Type1    |  %9d     || Type2     |  %9d\n", gdsTempPDBufferForIEC61850[u8PDChNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsPDStatus[ENUM_PD_TYPE_0].m_u32Value, gdsTempPDBufferForIEC61850[u8PDChNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsPDStatus[ENUM_PD_TYPE_1].m_u32Value, gdsTempPDBufferForIEC61850[u8PDChNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsPDStatus[ENUM_PD_TYPE_2].m_u32Value);
				printf("-------+------------------------------+---------------------------+-------------\n");
				printf(" Type3 |  %9d         || Type4    |  %9d     || Type5     |  %9d\n", gdsTempPDBufferForIEC61850[u8PDChNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsPDStatus[ENUM_PD_TYPE_3].m_u32Value, gdsTempPDBufferForIEC61850[u8PDChNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsPDStatus[ENUM_PD_TYPE_4].m_u32Value, gdsTempPDBufferForIEC61850[u8PDChNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsPDStatus[ENUM_PD_TYPE_5].m_u32Value);
				printf("-------+------------------------------+---------------------------+-------------\n");
				printf(" Type6 |  %9d         || Type7    |  %9d     || Type8     |  %9d\n", gdsTempPDBufferForIEC61850[u8PDChNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsPDStatus[ENUM_PD_TYPE_6].m_u32Value, gdsTempPDBufferForIEC61850[u8PDChNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsPDStatus[ENUM_PD_TYPE_7].m_u32Value, gdsTempPDBufferForIEC61850[u8PDChNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsPDStatus[ENUM_PD_TYPE_8].m_u32Value);
				printf("-------+------------------------------+---------------------------+-------------\n");
				printf(" Type9 |  %9d         || Type10   |  %9d     || Type11    |  %9d\n", gdsTempPDBufferForIEC61850[u8PDChNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsPDStatus[ENUM_PD_TYPE_9].m_u32Value, gdsTempPDBufferForIEC61850[u8PDChNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsPDStatus[ENUM_PD_TYPE_10].m_u32Value, gdsTempPDBufferForIEC61850[u8PDChNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsPDStatus[ENUM_PD_TYPE_11].m_u32Value);
				printf("-------+------------------------------+---------------------------+-------------\n");
				printf(" PDMag |  %9d         || Avg.Dis.I|  %9d     || Max.PD Cnt|  %9d\n", gdsTempPDBufferForIEC61850[u8PDChNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsPDStatus[ENUM_PD_MAX_APPARENT_PD_MAG].m_u32Value, gdsTempPDBufferForIEC61850[u8PDChNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsPDStatus[ENUM_PD_AVG_DISCHARGE_I].m_u32Value, gdsTempPDBufferForIEC61850[u8PDChNo][pdsTotalDataToCU->m_u32ReadIndex].m_dsPDStatus[ENUM_PD_MAX_PD_MAG_CNT].m_u32Value);
				printf("=======+==============================+===========================+=============\n");
			}
			else {
				printf("Checksum error for read index %d of PD channel #%1d (read:0x%x / calculated:0x%x)\n", pdsTotalDataToCU->m_u32ReadIndex, (u8PDChNo+1), (U16)(gdsTempPDBufferForIEC61850[u8PDChNo][pdsTotalDataToCU->m_u32ReadIndex].m_u32CRC), u16CRC);
			}
		
			if (++pdsTotalDataToCU->m_u32ReadIndex == SIZE_OF_PD_DATA_LIST_TO_CU) {
				pdsTotalDataToCU->m_u32ReadIndex = 0;
			};
		}

		return 0;
	}
}

void CDM_Display_DI()
{
	U32 i=0;
	VT100_goto(0,14);

	printf("\n  001-006 | ");
	for(i=0; i<MAX_DI_COUNT; i++)
	{
		printf("[%3s] ", g_pIOSharedMem->m_dsIOInfo.u8DIStsVal[i] == 0? "OFF":"ON");
	}

	printf("\n==========+=======================================\n");
}

void CDM_Display_DO()
{
	U32 i=0;
	VT100_goto(0,14);

	for(i=0; i< MAX_DO_COUNT; i++)
	{
		printf("[%3s] ", g_pIOSharedMem->m_dsIOInfo.u8DOStsVal[i] == 0? "OFF":"ON");
	}

	printf("\n========+====================================\n");
}

// added by jibon
void CDM_Display_CSVLEAFS()
{
	VT100_goto(0,4);
	printf("Starting to read file Sanion_154kV_GIS_D01_1A.csv \n\n");
	if(ReadFile("Sanion_154kV_GIS_D01_1A.csv") == -1)
	{
		printf("\n\n\n\nError Reading CSV File\n\n\n\n\n");
	}
	else
	{
		printf("\n\n\n\nReading CSV File\n\n\n\n\n");
	}

	PrintFiltered();
}

// added by symoon
// void CDM_Display_MEAS()
// {
// 	U32 i=0;

// 	float fReferenceAng[3];
// 	float fRelativeAng[MAX_OCT_COUNT];

	
// 	// OCT current display
// 	fReferenceAng[0]	= g_pAISharedMem->m_dsMeasValueData.m_dsOCTDataInfo.m_f32Ang[0];
// 	fReferenceAng[1]	= g_pAISharedMem->m_dsMeasValueData.m_dsOCTDataInfo.m_f32Ang[3];
// 	fReferenceAng[2]	= g_pAISharedMem->m_dsMeasValueData.m_dsOCTDataInfo.m_f32Ang[6];

// 	for(i=0; i<MAX_OCT_COUNT; i++) 
// 	{
// 		VT100_goto(0,4+i);
// 		fRelativeAng[i]	= g_pAISharedMem->m_dsMeasValueData.m_dsOCTDataInfo.m_f32Ang[i]-fReferenceAng[i/3];
		
// 		if (fRelativeAng[i] > 0) 
// 		{
// 			fRelativeAng[i] -= 360.0;
// 		}
// 		printf(" OCT %1d  |  %8.2fA  %8.2fdeg", (i+1), g_pAISharedMem->m_dsMeasValueData.m_dsOCTDataInfo.m_f32Mag[i], fRelativeAng[i]);
// 	}

// 	// TD current display
// 	for(i=0; i<(MAX_TD_COUNT/2); i++) {
// 		VT100_goto(37,4+i);
// 		printf("|| TD %1d/%1d  |  %8.2fmA  %8.2fmA", (2*i), ((2*i)+1), g_pAISharedMem->m_dsMeasValueData.m_dsTDDataInfo.m_f32Mag[(2*i)], g_pAISharedMem->m_dsMeasValueData.m_dsTDDataInfo.m_f32Mag[((2*i)+1)]);
// 	}

// 	VT100_goto(37,8);
// 	printf("---------------------------------------");

// 	VT100_goto(0,13);
// 	printf("--------+--------------------------");

// 	// BCT current display
// 	fReferenceAng[0]	= g_pAISharedMem->m_dsMeasValueData.m_dsBCTDataInfo.m_f32Ang[0];
// 	fReferenceAng[1]	= g_pAISharedMem->m_dsMeasValueData.m_dsBCTDataInfo.m_f32Ang[3];
	
// 	for(i=0; i<MAX_BCT_COUNT; i++) 
// 	{
// 		VT100_goto(0,14+i);
// 		fRelativeAng[i]	= g_pAISharedMem->m_dsMeasValueData.m_dsBCTDataInfo.m_f32Ang[i]-fReferenceAng[i/3];
// 		if (fRelativeAng[i] > 0) 
// 		{
// 			fRelativeAng[i] -= 360.0;
// 		}
// 		printf(" BCT %1d  |  %8.2fA  %8.2fdeg", (i+1), g_pAISharedMem->m_dsMeasValueData.m_dsBCTDataInfo.m_f32Mag[i], fRelativeAng[i]);
// 	}

// 	// PT current display
// 	fReferenceAng[0]	= g_pAISharedMem->m_dsMeasValueData.m_dsPTDataInfo.m_f32Ang[0];
// 	fReferenceAng[1]	= g_pAISharedMem->m_dsMeasValueData.m_dsPTDataInfo.m_f32Ang[3];
// 	for(i=0; i<MAX_PT_COUNT; i++) {
// 		VT100_goto(37,9+i);
// 		fRelativeAng[i]	= g_pAISharedMem->m_dsMeasValueData.m_dsPTDataInfo.m_f32Ang[i]-fReferenceAng[i/3];
// 		if (fRelativeAng[i] > 0) {
// 			fRelativeAng[i] -= 360.;
// 		}
// 		printf("|| PT %1d    |  %8.2fV  %8.2fdeg", (i+1), g_pAISharedMem->m_dsMeasValueData.m_dsPTDataInfo.m_f32Mag[i], fRelativeAng[i]);
// 	}

// 	VT100_goto(37,15);
// 	printf("---------------------------------------");

// 	VT100_goto(0,20);
// 	printf("--------+------------------------------------------------------------------");

// 	// Internal voltage display
// 	VT100_goto(37,16);
// 	printf("|| Int. V  |  %8.2fV  %8.2fV", g_pAISharedMem->m_dsMeasValueData.m_fInternalDCVoltage[0], g_pAISharedMem->m_dsMeasValueData.m_fInternalDCVoltage[1]);
// 	VT100_goto(37,17);
// 	printf("||         |  %8.2fV  %8.2fV", g_pAISharedMem->m_dsMeasValueData.m_fInternalDCVoltage[2], g_pAISharedMem->m_dsMeasValueData.m_fInternalDCVoltage[3]);
// 	VT100_goto(37,18);
// 	printf("||         |  %8.2fV  %8.2fV", g_pAISharedMem->m_dsMeasValueData.m_fInternalDCVoltage[4], g_pAISharedMem->m_dsMeasValueData.m_fInternalDCVoltage[5]);
// 	VT100_goto(37,19);
// 	printf("||         |  %8.2fV  %8.2fV", g_pAISharedMem->m_dsMeasValueData.m_fInternalDCVoltage[6], g_pAISharedMem->m_dsMeasValueData.m_fInternalDCVoltage[7]);

// 	VT100_goto(0,21);
// 	printf(" DI     |  %3s  %3s  %3s  %3s  %3s  %3s", (g_pIOSharedMem->m_dsIOInfo.u8DIStsVal[0]==0? "OFF":"ON"), (g_pIOSharedMem->m_dsIOInfo.u8DIStsVal[1]==0? "OFF":"ON"), (g_pIOSharedMem->m_dsIOInfo.u8DIStsVal[2]==0? "OFF":"ON"), (g_pIOSharedMem->m_dsIOInfo.u8DIStsVal[3]==0? "OFF":"ON"), (g_pIOSharedMem->m_dsIOInfo.u8DIStsVal[4]==0? "OFF":"ON"), (g_pIOSharedMem->m_dsIOInfo.u8DIStsVal[5]==0? "OFF":"ON"));

// 	VT100_goto(0,22);
// 	printf(" DO     |  %3s  %3s  %3s", (g_pIOSharedMem->m_dsIOInfo.u8DOStsVal[0]==0? "OFF":"ON"), (g_pIOSharedMem->m_dsIOInfo.u8DOStsVal[1]==0? "OFF":"ON"), (g_pIOSharedMem->m_dsIOInfo.u8DOStsVal[2]==0? "OFF":"ON"));

// 	VT100_goto(0,23);
// 	printf("========+==================================================================");
// }



void CDM_Display_MEAS()
{
	U32 i=0;

	float fReferenceAng[3];
	float fRelativeAng[MAX_OCT_COUNT];

	float fOctAng[MAX_OCT_COUNT];
	float ptAng[MAX_OCT_COUNT];
	float fBctAng[MAX_BCT_COUNT];

	
	// OCT current display
	fReferenceAng[0]	= g_pAISharedMem->m_dsMeasValueData.m_dsOCTDataInfo.m_f32Ang[0];
	fReferenceAng[1]	= g_pAISharedMem->m_dsMeasValueData.m_dsOCTDataInfo.m_f32Ang[3];
	fReferenceAng[2]	= g_pAISharedMem->m_dsMeasValueData.m_dsOCTDataInfo.m_f32Ang[6];

	fOctAng[0] = g_pAISharedMem->m_dsPhasorDataInfo._oct[0];
	fOctAng[1] = g_pAISharedMem->m_dsPhasorDataInfo._oct[1];
	fOctAng[2] = g_pAISharedMem->m_dsPhasorDataInfo._oct[2];
	fOctAng[3] = 0;
	fOctAng[4] = 0;
	fOctAng[5] = 0;
	ptAng[0] = g_pAISharedMem->m_dsPhasorDataInfo._pt[0];
	ptAng[1] = g_pAISharedMem->m_dsPhasorDataInfo._pt[1];
	ptAng[2] = g_pAISharedMem->m_dsPhasorDataInfo._pt[2];
	ptAng[3] = 0;
	ptAng[4] = 0;
	ptAng[5] = 0;

	fBctAng[0] = g_pAISharedMem->m_dsPhasorDataInfo._bct[0];
	fBctAng[1] = g_pAISharedMem->m_dsPhasorDataInfo._bct[1];
	fBctAng[2] = g_pAISharedMem->m_dsPhasorDataInfo._bct[2];
	fBctAng[3] = 0;
	fBctAng[4] = 0;
	fBctAng[5] = 0;

	for(i=0; i<MAX_OCT_COUNT; i++) 
	{
		VT100_goto(0,4+i);
		fRelativeAng[i]	= g_pAISharedMem->m_dsMeasValueData.m_dsOCTDataInfo.m_f32Ang[i]-fReferenceAng[i/3];
		
		if (fRelativeAng[i] > 0) 
		{
			fRelativeAng[i] -= 360.0;
		}
		//printf(" OCT %1d  |  %8.2fA  %8.2fdeg", (i+1), g_pAISharedMem->m_dsMeasValueData.m_dsOCTDataInfo.m_f32Mag[i], fRelativeAng[i]);
		printf(" OCT %1d  |  %8.2fA  %8.2fdeg", (i+1), g_pAISharedMem->m_dsMeasValueData.m_dsOCTDataInfo.m_f32Mag[i], fOctAng[i]);
	}

	// TD current display
	for(i=0; i<(MAX_TD_COUNT/2); i++) {
		VT100_goto(37,4+i);
		printf("|| TD %1d/%1d  |  %8.2fmA  %8.2fmA", (2*i), ((2*i)+1), g_pAISharedMem->m_dsMeasValueData.m_dsTDDataInfo.m_f32Mag[(2*i)], g_pAISharedMem->m_dsMeasValueData.m_dsTDDataInfo.m_f32Mag[((2*i)+1)]);
	}

	VT100_goto(37,8);
	printf("---------------------------------------");

	VT100_goto(0,13);
	printf("--------+--------------------------");

	// BCT current display
	fReferenceAng[0]	= g_pAISharedMem->m_dsMeasValueData.m_dsBCTDataInfo.m_f32Ang[0];
	fReferenceAng[1]	= g_pAISharedMem->m_dsMeasValueData.m_dsBCTDataInfo.m_f32Ang[3];
	
	for(i=0; i<MAX_BCT_COUNT; i++) 
	{
		VT100_goto(0,14+i);
		fRelativeAng[i]	= g_pAISharedMem->m_dsMeasValueData.m_dsBCTDataInfo.m_f32Ang[i]-fReferenceAng[i/3];
		if (fRelativeAng[i] > 0) 
		{
			fRelativeAng[i] -= 360.0;
		}
		// printf(" BCT %1d  |  %8.2fA  %8.2fdeg", (i+1), g_pAISharedMem->m_dsMeasValueData.m_dsBCTDataInfo.m_f32Mag[i], fRelativeAng[i]);
		printf(" BCT %1d  |  %8.3fA  %8.2fdeg", (i+1), g_pAISharedMem->m_dsMeasValueData.m_dsBCTDataInfo.m_f32Mag[i], fBctAng[i]);

	}

	// PT current display
	fReferenceAng[0]	= g_pAISharedMem->m_dsMeasValueData.m_dsPTDataInfo.m_f32Ang[0];
	fReferenceAng[1]	= g_pAISharedMem->m_dsMeasValueData.m_dsPTDataInfo.m_f32Ang[3];
	for(i=0; i<MAX_PT_COUNT; i++) {
		VT100_goto(37,9+i);
		fRelativeAng[i]	= g_pAISharedMem->m_dsMeasValueData.m_dsPTDataInfo.m_f32Ang[i]-fReferenceAng[i/3];
		if (fRelativeAng[i] > 0) {
			fRelativeAng[i] -= 360.0;
		}
		// printf("|| PT %1d    |  %8.2fV  %8.2fdeg", (i+1), g_pAISharedMem->m_dsMeasValueData.m_dsPTDataInfo.m_f32Mag[i], fRelativeAng[i]);
		printf("|| PT %1d    |  %8.2fV  %8.2fdeg", (i+1), g_pAISharedMem->m_dsMeasValueData.m_dsPTDataInfo.m_f32Mag[i], ptAng[i]);
	}

	VT100_goto(37,15);
	printf("---------------------------------------");

	VT100_goto(0,20);
	printf("--------+------------------------------------------------------------------");

	printf("  PT[i] - BCT[i] = %8.2f , %8.2f , %8.2f  ", (ptAng[0]-fBctAng[0]), (ptAng[1]-fBctAng[1]), (ptAng[2]-fBctAng[2]));

	double theta = (double)(ptAng[0]-fBctAng[0]);
	double radians = theta * M_PI / 180;
	// Internal voltage display
	VT100_goto(37,16);
	printf("|| Int. V  |  %8.2fV  %8.2fV", g_pAISharedMem->m_dsMeasValueData.m_fInternalDCVoltage[0], g_pAISharedMem->m_dsMeasValueData.m_fInternalDCVoltage[1]);
	VT100_goto(37,17);
	printf("||         |  %8.2fV  %8.2fV", g_pAISharedMem->m_dsMeasValueData.m_fInternalDCVoltage[2], g_pAISharedMem->m_dsMeasValueData.m_fInternalDCVoltage[3]);
	VT100_goto(37,18);
	printf("||         |  %8.2fV  %8.2fV", g_pAISharedMem->m_dsMeasValueData.m_fInternalDCVoltage[4], g_pAISharedMem->m_dsMeasValueData.m_fInternalDCVoltage[5]);
	VT100_goto(37,19);
	printf("||         |  %8.2fV  %8.2fV", g_pAISharedMem->m_dsMeasValueData.m_fInternalDCVoltage[6], g_pAISharedMem->m_dsMeasValueData.m_fInternalDCVoltage[7]);

	VT100_goto(0,21);
	printf(" DI     |  %3s  %3s  %3s  %3s  %3s  %3s", (g_pIOSharedMem->m_dsIOInfo.u8DIStsVal[0]==0? "OFF":"ON"), (g_pIOSharedMem->m_dsIOInfo.u8DIStsVal[1]==0? "OFF":"ON"), (g_pIOSharedMem->m_dsIOInfo.u8DIStsVal[2]==0? "OFF":"ON"), (g_pIOSharedMem->m_dsIOInfo.u8DIStsVal[3]==0? "OFF":"ON"), (g_pIOSharedMem->m_dsIOInfo.u8DIStsVal[4]==0? "OFF":"ON"), (g_pIOSharedMem->m_dsIOInfo.u8DIStsVal[5]==0? "OFF":"ON"));

	VT100_goto(0,22);
	printf(" DO     |  %3s  %3s  %3s", (g_pIOSharedMem->m_dsIOInfo.u8DOStsVal[0]==0? "OFF":"ON"), (g_pIOSharedMem->m_dsIOInfo.u8DOStsVal[1]==0? "OFF":"ON"), (g_pIOSharedMem->m_dsIOInfo.u8DOStsVal[2]==0? "OFF":"ON"));

	VT100_goto(0,23);
	printf("========+==================================================================");

	printf("\n  cos theta = %8.4f  ", cos(radians));
}

// added by symoon
void CDM_Display_OLTCSTS()
{
	U32 i=0;

	float fReferenceAng[3];
	float fRelativeAng[MAX_OCT_COUNT];

	
	// OCT current display
	fReferenceAng[0]	= gdsOLTCMonInfo[0].m_dsMotorI[PHASE_A].m_fAng;
	fReferenceAng[1]	= gdsOLTCMonInfo[1].m_dsMotorI[PHASE_A].m_fAng;
	fReferenceAng[2]	= gdsOLTCMonInfo[2].m_dsMotorI[PHASE_A].m_fAng;
	for(i=0; i<MAX_OCT_COUNT; i++) {
		VT100_goto(0,4+i);
		fRelativeAng[i]	= gdsOLTCMonInfo[i/3].m_dsMotorI[i%NUM_BASIC_PHASE].m_fAng-fReferenceAng[i/3];
		if (fRelativeAng[i] > 0) {
			fRelativeAng[i] -= 360.;
		}
		printf(" OCT %1d  |  %8.2fA  %8.2fdeg", (i+1), gdsOLTCMonInfo[i/3].m_dsMotorI[i%NUM_BASIC_PHASE].m_fMag, fRelativeAng[i]);
	}

	// �ܱ� �µ�
	for(i=0; i<3; i++) {
		VT100_goto(37,4+i);
		printf("|| Temp %1d  |  %8.2f Celsius", (i+1), gdsOLTCMonInfo[i].m_fOutsideTemp);
	}

	VT100_goto(37,7);
	printf("---------------------------------------");

	// �ܱ� �µ�
	for(i=0; i<3; i++) {
		VT100_goto(37,8+i);
		printf("|| Time %1d  |  %8.2f sec", (i+1), ((float)gdsOLTCMonInfo[0].m_u32MotorOperationTime)/100.);
	}

	VT100_goto(0,13);
	printf("========+==================================================================");
}

// added by symoon
void CDM_Display_BSHSTS()
{
	U32 i=0;

	float fReferenceAng[3];
	float fRelativeAng[MAX_OCT_COUNT];

	
	// BCT current display
	fReferenceAng[0]	= gdsBshMonCalRes[0].m_dsBshLeakI[PHASE_A].m_fAng;
	fReferenceAng[1]	= gdsBshMonCalRes[1].m_dsBshLeakI[PHASE_A].m_fAng;
	for(i=0; i<MAX_BCT_COUNT; i++) {
		VT100_goto(0,4+i);
		fRelativeAng[i]	= gdsBshMonCalRes[i/3].m_dsBshLeakI[i%3].m_fAng-fReferenceAng[i/3];
		if (fRelativeAng[i] > 0) {
			fRelativeAng[i] -= 360.;
		}
		printf(" BCT %1d  |  %8.2fA  %8.2fdeg", (i+1), gdsBshMonCalRes[i/3].m_dsBshLeakI[i%3].m_fMag, fRelativeAng[i]);
	}

	// PT current display
	fReferenceAng[0]	= gdsBshMonCalRes[0].m_dsBusV[PHASE_A].m_fAng;
	fReferenceAng[1]	= gdsBshMonCalRes[1].m_dsBusV[PHASE_A].m_fAng;
	for(i=0; i<MAX_PT_COUNT; i++) {
		VT100_goto(37,4+i);
		fRelativeAng[i]	= gdsBshMonCalRes[i/3].m_dsBusV[i%3].m_fAng-fReferenceAng[i/3];
		if (fRelativeAng[i] > 0) {
			fRelativeAng[i] -= 360.;
		}
		printf("|| PT %1d    |  %8.2fV  %8.2fdeg", (i+1), gdsBshMonCalRes[i/3].m_dsBusV[i%3].m_fMag, fRelativeAng[i]);
	}

	VT100_goto(0,10);
	printf("--------+------------------------------------------------------------------");

	// 3I0 current display
	VT100_goto(0,11);
	fRelativeAng[0]	= gdsBshMonCalRes[0].m_dsBshLeak3I0.m_fAng-gdsBshMonCalRes[0].m_dsBshLeakI[PHASE_A].m_fAng;
	if (fRelativeAng[0] > 0) {
		fRelativeAng[0] -= 360.;
	}
	printf(" 3I0 1  |  %8.2fA  %8.2fdeg", gdsBshMonCalRes[0].m_dsBshLeak3I0.m_fMag, fRelativeAng[0]);

	VT100_goto(37,11);
	fRelativeAng[1]	= gdsBshMonCalRes[1].m_dsBshLeak3I0.m_fAng-gdsBshMonCalRes[1].m_dsBshLeakI[PHASE_A].m_fAng;
	if (fRelativeAng[1] > 0) {
		fRelativeAng[1] -= 360.;
	}
	printf("|| 3I0 2   |	%8.2fA", gdsBshMonCalRes[1].m_dsBshLeak3I0.m_fMag, fRelativeAng[1]);


	VT100_goto(0,12);
	printf("--------+------------------------------------------------------------------");

	for(i=0; i<MAX_BCT_COUNT; i++) {
		// Capacitance values display
		VT100_goto(0,13+i);
		printf(" Cap. %1d | %6.2fuF", (i+1), gdsBshMonCalRes[i/3].m_fCapVal[i%3]);

		// Displacement current display
		VT100_goto(20,13+i);
		printf("|| DispI %1d | %8.2fA", (i+1), gdsBshMonCalRes[i/3].m_fDisplI[i%3]);

		// Power factors display
		VT100_goto(43,13+i);
		printf("|| PF %1d | %4.2f", (i+1), gdsBshMonCalRes[i/3].m_fPwrFactor[i%3]);

		// Dissipation factors display
		VT100_goto(58,13+i);
		printf("|| DissF %1d | %4.2f", (i+1), gdsBshMonCalRes[i/3].m_fDisFactor[i%3]);
	}

	VT100_goto(0,19);
	printf("========+==================================================================");
}

void *Exit_App(int argc, void *argv[])
{
	int nReturn = -1;
	glob_t stFileSearchResult;

	DspManager_DSPOff();
	Console_DeInit();
	WDT_DeInit();

	nReturn  = glob("*.cid", 0, NULL, &stFileSearchResult); //if exist: 0, no match: 3
	//	nReturn += glob("*.icd", 0, NULL, &stFileSearchResult);
	globfree( &stFileSearchResult); //stFileSearchResult wasn't used
	//if(nReturn == 0) // || nReturn == GLOB_NOMATCH)
		//Kill_61850();
	
	//Exit_Task();
	exit(0);
	return 0;
}

void *CDM_SetDate(int argc, void *argv[])
{
	struct tm *tm;
	struct timeval tv;
	FILE *read_fp;
	char buffer[1024];
	
	if(argc < 3)
	{
		printf("\nEX)DATE [Year] [Mon] [Day]\n");
		SSM_DisplayOsTime();
		return NULL;
	}

	//Get Time
	gettimeofday(&tv, NULL);
	tm = localtime(&tv.tv_sec);

	tm->tm_year = strtoul(argv[0], NULL, 10) - 1900;     // 1900 ?�하값이 ?�력?�면 ?�됨, 8->10 digit
	tm->tm_mon = strtoul(argv[1], NULL, 10) - 1;         // 0???�력?�면 ?�됨, 8->10 digit
	tm->tm_mday = strtoul(argv[2], NULL, 10);            // 0???�력?�면 ?�됨, 8->10 digit

	// OS Time update
	strftime(buffer, sizeof(buffer), "date -s %Y%m%d%H%M.%S", tm);
	read_fp = popen(buffer, "r");
	if(read_fp != NULL)
	{
		fread(buffer, sizeof(char), 1024+1, read_fp);
		pclose(read_fp);
	}

	// RTC Update
	read_fp = popen(CMD_TIMETORTC, "r");
	if(read_fp != NULL)
	{
		memset(buffer, '\0', sizeof(buffer));
		fread(buffer, sizeof(char), 1024+1, read_fp);
		pclose(read_fp);
	}

	SSM_DisplayOsTime();
	return NULL;
}

void *CDM_SetTime(int argc, void *argv[])
{
	struct tm *tm;
	struct timeval tv;
	FILE *read_fp;
	char buffer[1024];

	if(argc < 3)
	{
		printf("\nEX)TIME [Hour] [Min] [Sec]\n");
		SSM_DisplayOsTime();
		return NULL;
	}

	//Get Time
	gettimeofday(&tv, NULL);
	tm = localtime(&tv.tv_sec);
	tm->tm_hour = strtoul(argv[0], NULL, 10);        // 0 ~ 24
	tm->tm_min  = strtoul(argv[1], NULL, 10);         // 0 ~ 60
	tm->tm_sec  = strtoul(argv[2], NULL, 10);         // 0 - 60


	// OS Time update
	strftime(buffer, sizeof(buffer), "date -s %Y%m%d%H%M.%S", tm);
	read_fp = popen(buffer, "r");
	if(read_fp != NULL)
	{
		fread(buffer, sizeof(char), 1024+1, read_fp);
		pclose(read_fp);
	}

	// RTC Update
	read_fp = popen(CMD_TIMETORTC, "r");
	if(read_fp != NULL)
	{
		memset(buffer, '\0', sizeof(buffer));
		fread(buffer, sizeof(char), 1024+1, read_fp);
		pclose(read_fp);
	}

	SSM_DisplayOsTime();
	return NULL;
}

void *CDM_SetMAC(int argc, void*argv[])
{
    int s;
    struct ifreq ifr;
	U8 au8MACAddr[20] = {0};
	char *au8newMac = (char *)argv[0]; 

	if (argc < 1)
	{
		printf("\nUsage: SETMAC [No1:No2:No3:No4:No5:No6] for MAC Setting\n");
		printf("Ex) SETMAC 00:1B:4B:DD:EE:FF\n"); 
		
		if(DBM_GetSetting("NormalSetting", "MacAddr",(char *)au8MACAddr) == 0)
		{
			printf("Current MAC Addr is set to be %s\n", au8MACAddr);
		}
		return NULL;		
	}
    // Open a socket to perform the ioctl operations.
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        perror("socket() failed");
        return EXIT_FAILURE;
    }

    // Prepare the ifreq structure
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ);

    // Specify the action: set the hardware (MAC) address
    ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
    memcpy(ifr.ifr_hwaddr.sa_data, au8newMac, 6);

    // Set the MAC address
    if (ioctl(s, SIOCSIFHWADDR, &ifr) < 0) {
        perror("SIOCSIFHWADDR failed");
        close(s);
        return EXIT_FAILURE;
    }
	DBM_SaveSetting("NormalSetting", au8newMac, "MacAddr");

    printf("MAC address set successfully.\n");

    // Close the socket
    close(s);
    return EXIT_SUCCESS;

}

void *CDM_SetIp(int argc, void* argv[])
{
    U32 u32IpAddrTemp;
    U32 u32Hex_IpAddr, u32IPAddr;
    U8 au8IpAddr[20] = {0}, au8OrgIPAddr[20] = {0};
    
    if(argc < 2) // Check for minimum number of arguments
    {
        printf("\nUsage: SETIP -M [No1.No2.No3.No4] for MPU IP Setting\n");
        printf("       SETIP -S [No1.No2.No3.No4] for SNTP IP Setting\n");
        printf("Ex) SETIP -M 192.168.90.255\n"); 
        printf("Ex) SETIP -S 192.168.90.255\n");

		if(DBM_GetSetting("DevInfo", "MPU IPAddr",(char *)au8IpAddr) == 0)
		{
			printf("Current MPU IP is set to be %s\n", au8IpAddr);
		}
		
		
		if(DBM_GetSetting("DevInfo", "NTP IPAddr",(char *)au8IpAddr) == 0)
		{
			printf("Current NTP IP is set to be %s\n", au8IpAddr);
		}

		
		//inet_pton( AF_INET, (char *)au8IpAddr, (void*)&u32IpAddrTemp);
		
		//u32IPAddr = ntohl(u32IpAddrTemp);

		/*
        u32IpAddrTemp = SSM_GetKernelIpAddr();
        u32Hex_IpAddr = htonl(u32IpAddrTemp);

        if(inet_ntop(AF_INET, &u32Hex_IpAddr, (char *)au8OrgIPAddr, sizeof(au8OrgIPAddr)) == NULL)
            printf("Current Ip Setting read error\n");
        else
            printf("Current IP: %s\n", au8OrgIPAddr); 
        
        return NULL;
		*/
		return NULL;
    }

	    char *option = (char *)argv[0]; // -M or -S
	    char *ipAddress = (char *)argv[1]; // IP Address

	    if(inet_pton( AF_INET, (char *)ipAddress, (void*)&u32IpAddrTemp) <= 0)
	    {
	        printf("Invalid Ip Address: %s\n", ipAddress);
	        return NULL;
	    }

	    u32Hex_IpAddr = ntohl(u32IpAddrTemp);

	    if(strcmp(option, "-M") == 0) {
	        // Save the MPU IP address setting
	        if(DBM_SaveSetting("DevInfo", ipAddress, "MPU IPAddr") == 0) {
	            SSM_UpdateIpAddr(u32Hex_IpAddr);
	            printf("MPU IP Address updated to %s\n", ipAddress);
	        }
	    } else if(strcmp(option, "-S") == 0) {
	        // Save the SNTP IP address setting
	        if(DBM_SaveSetting("DevInfo", ipAddress, "NTP IPAddr") == 0) {
	            //SSM_UpdateIpAddr(u32Hex_IpAddr);
	            printf("SNTP IP Address updated to %s\n", ipAddress);
	        }
	    } else {
	        printf("Invalid option: %s\n", option);
	        printf("Use -M for MPU and -S for SNTP IP settings.\n");
	    }
	    
	    return NULL;
}


void *CDM_SetSubnet(int argc, void* argv[])
{
    U32 IpAddr_temp;
    U32 u32Hex_IpAddr;
    U8  au8IpAddr[20]={0,}; 

    if(argc < 2) // Adjust for new argument structure
    {
        printf("\nUsage: SETSM -M [No1.No2.No3.No4] for MPU Subnet Setting\n");
        printf("       SETSM -S [No1.No2.No3.No4] for SNTP Subnet Setting\n");
        printf("Ex) SETSM -M 255.255.255.0\n"); 
        printf("Ex) SETSM -S 255.255.255.0\n");

		if(DBM_GetSetting("DevInfo", "MPU Subnet",(char *)au8IpAddr) == 0)
		{
			printf("Current MPU Subnet is set to be %s\n", au8IpAddr);
		}
		
		
		if(DBM_GetSetting("DevInfo", "NTP Subnet",(char *)au8IpAddr) == 0)
		{
			printf("Current NTP Subnet is set to be %s\n", au8IpAddr);
		}		
        return NULL;
    }

    char *option = (char *)argv[0]; // -M or -S
    char *subnet = (char *)argv[1]; // Subnet Address

	if(inet_pton(AF_INET, subnet, (void*)&IpAddr_temp) <= 0)
	{
		printf("Invalid Subnet Address: %s\n", subnet);
		return NULL;
	}


    u32Hex_IpAddr = ntohl(IpAddr_temp);

    if(strcmp(option, "-M") == 0) {
        if(DBM_SaveSetting("DevInfo", subnet, "MPU Subnet") == 0) {
            SSM_UpdateSubnet(u32Hex_IpAddr);
            printf("MPU Subnet updated to %s\n", subnet);
        }
    } else if(strcmp(option, "-S") == 0) {
        if(DBM_SaveSetting("DevInfo", subnet, "NTP Subnet") == 0) {
            SSM_UpdateSubnet(u32Hex_IpAddr); // Assuming SSM_UpdateSubnet works for SNTP as well
            printf("SNTP Subnet updated to %s\n", subnet);
        }
    } else {
        printf("Invalid option: %s\n", option);
        printf("Use -M for MPU and -S for SNTP subnet settings.\n");
    }

    return NULL;
}



void *CDM_SetGateway(int argc, void* argv[])
{
    U8  au8IpAddr[20]={0,};
    U32 u32Hex_IpAddr=0;
    U32 IpAddr_temp=0;

    if(argc < 2) // Adjust for new argument structure
    {
        printf("\nUsage: SETGW -M [No1.No2.No3.No4] for MPU Gateway Setting\n");
        printf("       SETGW -S [No1.No2.No3.No4] for SNTP Gateway Setting\n");
        printf("Ex) SETGW -M 192.168.1.1\n"); 
        printf("Ex) SETGW -S 192.168.1.1\n");
		if(DBM_GetSetting("DevInfo", "MPU Gateway",(char *)au8IpAddr) == 0)
		{
			printf("Current MPU GW is set to be %s\n", au8IpAddr);
		}
		
		
		if(DBM_GetSetting("DevInfo", "NTP Gateway",(char *)au8IpAddr) == 0)
		{
			printf("Current NTP GW is set to be %s\n", au8IpAddr);
		}				
        return NULL;
    }

    char *option = (char *)argv[0]; // -M or -S
    char *gateway = (char *)argv[1]; // Gateway Address
    
	if(inet_pton(AF_INET, gateway, (void*)&IpAddr_temp) <= 0)
	{
		printf("Invalid Subnet Address: %s\n", gateway);
		return NULL;
	}

    u32Hex_IpAddr = ntohl(IpAddr_temp);

    if(strcmp(option, "-M") == 0) {
        if(DBM_SaveSetting("DevInfo", gateway, "MPU Gateway") == 0) {
            SSM_UpdateGateway(u32Hex_IpAddr);
            printf("MPU Gateway updated to %s\n", gateway);
        }
    } else if(strcmp(option, "-S") == 0) {
        if(DBM_SaveSetting("DevInfo", gateway, "SNTP Gateway") == 0) {
            SSM_UpdateGateway(u32Hex_IpAddr); // Assuming SSM_UpdateGateway works for SNTP as well
            printf("SNTP Gateway updated to %s\n", gateway);
        }
    } else {
        printf("Invalid option: %s\n", option);
        printf("Use -M for MPU and -S for SNTP gateway settings.\n");
    }

    return NULL;
}

void *CDM_DI_Unlock_Execute(int argc, void *argv[])
{
	IOM_DI_Unlock_Tx();
}


void *Console_ProcessMain( void *arg )
{
	int i=0;


	printf("[%s] Task Start.\n", __func__);

	Console_Init();

	g_s32CDM_command = CDM_DISPLAY_NONE;
	usleep(100000);    //wait 100ms
    printf(" ==========================================Console_ProcessMain PID: %lu\n", pthread_self());

	while(!Console_ThreadExit)
	{		
		//Console Display Routine
		// modified by symoon
		if (i==0) {
			CDM_Routine();
			i++;
		}
		else {
			i++;
			if (i>10000) {
				i=0;
			}
		}

		//Command Line Interface Routine
		CLI_Routine();

		sched_yield();

		usleep(20);    // 50000uS added by symoon (unit is microsecond.)
		// printf("Console_ProcessMain Running \r\n");
	}
	
	pthread_exit(NULL);
	printf("[%s] Task Close.\n", __func__);
	return 0;
}

