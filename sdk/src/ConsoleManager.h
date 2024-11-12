/******************************************************************************
 **                     Copyright(C) 2014 SANION Co. Ltd.                    **
 **                  --------------------------------------                  **
 **                              WWW.SANION.COM                              **
 ******************************************************************************
    PROJECT     : UR-IED
 ******************************************************************************
    FILE NAME   : ConsolManager.h
    AUTHOR      : sootoo23
    DATE        : 2015.07.01
    REVISION    : V1.00
    DESCRIPTION : Console Display & Command Line (CLI + CDM)
 ******************************************************************************
    HISTORY     :
        2015-07-01 Create (ConsoleDisplayManager + CommandLintInterface)
 ******************************************************************************/

#ifndef CONSOLEMANAGER_H_
#define CONSOLEMANAGER_H_

#ifdef __cplusplus
extern "C" {
#endif

// symoon : ����� DI�� DO ���¸� ǥ���ϴ� ��ɸ� �ֳ�?
typedef enum {
    CDM_DISPLAY_NONE,
	CDM_DISPLAY_DISTS,
	CDM_DISPLAY_DOSTS,
	CDM_DISPLAY_MEASSTS,	// added by symoon
	CDM_DISPLAY_OLTCSTS,	// added by symoon
	CDM_DISPLAY_BSHSTS,		// added by symoon
	CDM_DISPLAY_DGASTS,		// added by symoon
	CDM_DISPLAY_ACQSTS,		// added by symoon
	CDM_DISPLAY_CSVLEAFS		//added by jibon
}CDM_DISPLAY_CMD;

// symoon : ����UR�� DSP�� I/O�� �����̴�.
typedef enum {
	CDM_DISP_VIEW_ALL,
	CDM_DISP_VIEW_DSP,
	CDM_DISP_VIEW_STB,
	CDM_DISP_VIEW_PCIF,
	CDM_DISP_VIEW_IO,
	CDM_DISP_VIEW_DB,
	CDM_DISP_VIEW_HMI,
	CDM_DISP_VIEW_SMO,	// symoon : what?
	CDM_DISP_VIEW_EVT
}CDM_DISP_VIEW;

extern volatile int g_s32CDM_command;

/*
 * sootoo23 - console command ���� ����ü
 */
typedef struct {
    char  pcCommand[16];	///< Console�� �Է¹޴� ���ɾ�
    void* pFunction;		///< ���� �Լ�
    int   pcParamCnt;		///< Input Param Cnt
    char  pcMessage[64];	///< Help String
    int	  pcAccessMode;		///< �ش� ������ ���� level
} T_COMMAND_LIST;

int linux_kbhit(void);
void Console_Init(void);
void Console_DeInit();
void CDM_Routine();
int CLI_GetCommandLine(void);
int CLI_MakeArguments_v3(const char *s, char *token, char **argvp);
T_COMMAND_LIST *CLI_SearchCommand(char* argv);
void *CLI_SystemCmd(void);
void CLI_Routine();
void *CLI_FactorResetCmd(int argc, void *argv[]);

// MOD New Command addition
void *CDM_SetFlagCSVLEAFS(int argc, void *argv[]);

void CDM_Logo(void);
void CDM_DispPrompt(void);
void *CDM_HelpList(int argc, void *argv[]);
void *CDM_DispVersion(int argc, void *argv[]);
void *CDM_SetFlagDI(int argc, void *argv[]);
void *CDM_SetFlagDO(int argc, void *argv[]);
void *CDM_SetFlagMEAS(int argc, void *argv[]);				// added by symoon
void *CDM_DisplayOLTCSTS(int argc, void *argv[]);			// added by symoon
void *CDM_DisplayBSHSTS(int argc, void *argv[]);			// added by symoon
void *CDM_DisplayDGASTS(int argc, void *argv[]);			// added by symoon
void *CDM_DisplayACQSTS(int argc, void *argv[]);			// added by symoon
void *CDM_DisplayPDSTS(int argc, void *argv[]);			// added by symoon
void CDM_Display_DI();
void CDM_Display_DO();
void CDM_Display_MEAS();									// added by symoon
void *Console_SetDebugMode(int argc, char * argv[]);
void *CDM_SetViewMode(int argc, void *argv[]);
int CDM_GetViewMode(CDM_DISP_VIEW type);
void *Exit_App(int argc, void *argv[]);
void *CDM_SetDate(int argc, void *argv[]);
void *CDM_SetTime(int argc, void *argv[]);
void *CDM_SetMAC(int argc, void * argv [ ]);
void *CDM_SetIp(int argc, void* argv[]);
void *CDM_SetSubnet(int argc, void* argv[]);
void *CDM_SetGateway(int argc, void* argv[]);
void *CDM_DI_Unlock_Execute(int argc, void *argv[]);
void *Console_ProcessMain(void *arg);
void CDM_Display_CSVLEAFS();

// symoon : ����μ��� MO�� ���� command�� ���ٰ� ������

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CONSOLEMANAGER_H_ */
